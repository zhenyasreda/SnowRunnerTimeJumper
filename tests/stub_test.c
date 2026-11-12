/*
 * stub_test.c — автотест машинного кода стаба.
 *
 * Стаб выполняется прямо в нашем процессе: подставляем R13 (указатель на
 * «объект игры») и XMM2 (новое время), вызываем стаб и проверяем:
 *   - значение часов в нужном месте;
 *   - «родное» время и адрес часов, которые стаб сообщает обратно;
 *   - что стаб не портит состояние: RAX, флаги и посторонний регистр.
 *
 * Тест собирается только компилятором GCC (MinGW-w64) — используется
 * файл ассемблера tests/call_stub.S. Он не входит в обычную сборку.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "srt_priv.h"

#define FAKE_DISP 0x138

typedef struct {
    unsigned char pad[FAKE_DISP];
    float        clock;
} FakeObject;

/* Значения для вызова стаба (см. tests/call_stub.S). */
extern void *srt_test_stub;
extern void *srt_test_obj;
extern void *srt_test_valp;
unsigned long long srt_test_call(void);

static unsigned long long call_stub(void *stub, void *obj, const float *value)
{
    srt_test_stub = stub;
    srt_test_obj  = obj;
    srt_test_valp = (void *)value;
    return srt_test_call();
}

static int failures = 0;

static LONG WINAPI crash_handler(EXCEPTION_POINTERS *ep)
{
    CONTEXT *c = ep->ContextRecord;
    HMODULE mod = NULL;
    char path[MAX_PATH] = "?";
    typedef VOID (WINAPI *PC2F)(PVOID, PVOID *);
    PC2F pc2fh = (PC2F)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlPcToFileHeader");
    if (pc2fh) pc2fh((PVOID)c->Rip, (PVOID *)&mod);
    if (mod) GetModuleFileNameA(mod, path, MAX_PATH);
    printf("EXCEPTION: code=%08lX RIP=%llX RSP=%llX RAX=%llX module=%s\n",
           (unsigned long)ep->ExceptionRecord->ExceptionCode,
           (unsigned long long)c->Rip, (unsigned long long)c->Rsp,
           (unsigned long long)c->Rax, path);
    printf("            RBX=%llX RCX=%llX RDX=%llX RSI=%llX RDI=%llX R13=%llX R8=%llX R9=%llX\n",
           (unsigned long long)c->Rbx, (unsigned long long)c->Rcx,
           (unsigned long long)c->Rdx, (unsigned long long)c->Rsi,
           (unsigned long long)c->Rdi, (unsigned long long)c->R13,
           (unsigned long long)c->R8, (unsigned long long)c->R9);
    printf("            fault address=%p\n", ep->ExceptionRecord->ExceptionAddress);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void check(int ok, const char *what)
{
    printf("  [%s] %s\n", ok ? "OK  " : "FAIL", what);
    if (!ok) failures++;
}

static float read_f32(const void *p)
{
    float v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static uint64_t read_u64(const void *p)
{
    uint64_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static void set_u32(void *p, uint32_t v)
{
    memcpy(p, &v, sizeof(v));
}

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, NULL, _IONBF, 0);
    SetUnhandledExceptionFilter(crash_handler);

    uint8_t *block = (uint8_t *)VirtualAlloc(NULL, SRT_BLOCK_BYTES,
                                             MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    uint8_t *retpage = (uint8_t *)VirtualAlloc(NULL, 4096,
                                                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!block || !retpage) {
        printf("failed to allocate memory\n");
        return 2;
    }
    memset(block, 0, SRT_BLOCK_BYTES);
    retpage[0] = 0xC3;                 /* ret — точка возврата из стаба */

    uint8_t *code = block;
    uint8_t *data = block + SRT_CODE_BYTES;

    srt_build_stub(code, FAKE_DISP, (uintptr_t)block, (uintptr_t)retpage);

    printf("Stub machine code:\n");
    for (int i = 0; i < 0x5A; i += 16) {
        printf("  %02X:", i);
        for (int k = 0; k < 16 && i + k < 0x5A; k++) printf(" %02X", code[i + k]);
        printf("\n");
    }

    FakeObject obj;
    memset(&obj, 0, sizeof(obj));

    printf("Stub self-test\n");

    /* --- режим NATURAL: стаб выполняет исходную инструкцию --- */
    set_u32(data + SRT_DATA_MODE, (uint32_t)SRT_MODE_NATURAL);
    float v = 3.5f;
    unsigned long long mask = call_stub(code, &obj, &v);
    check((mask & 1) != 0, "EFLAGS preserved");
    check((mask & 2) != 0, "RAX preserved");
    check((mask & 4) != 0, "unrelated register preserved");
    check(obj.clock == 3.5f, "clock got the value from XMM2 (3.5)");
    check(read_f32(data + SRT_DATA_NATURAL) == 3.5f, "stub reported the natural time");
    check(read_u64(data + SRT_DATA_CLOCK) == (uint64_t)(uintptr_t)&obj.clock,
          "stub reported the clock address");

    /* --- режим FROZEN: часы не меняются --- */
    set_u32(data + SRT_DATA_MODE, (uint32_t)SRT_MODE_FROZEN);
    v = 9.0f;
    call_stub(code, &obj, &v);
    check(obj.clock == 3.5f, "clock kept the previous value");
    check(read_f32(data + SRT_DATA_NATURAL) == 9.0f, "natural time was updated");

    /* --- режим FORCED: часы держатся на заданном значении --- */
    set_u32(data + SRT_DATA_MODE, (uint32_t)SRT_MODE_FORCED);
    float forced = 7.25f;
    uint32_t bits;
    memcpy(&bits, &forced, sizeof(bits));
    set_u32(data + SRT_DATA_VALUE, bits);
    v = 11.0f;
    call_stub(code, &obj, &v);
    check(obj.clock == 7.25f, "clock got the value from the data block (7.25)");

    /* --- проверка байтов стаба --- */
    check(code[0] == 0x9C, "stub starts with pushfq");
    check(code[0x29] == 0xF3 && code[0x2A] == 0x41 && code[0x2B] == 0x0F &&
          code[0x2C] == 0x11 && code[0x2D] == 0x95,
          "original MOVSS [r13+disp], xmm2 instruction is present");

    VirtualFree(block, 0, MEM_RELEASE);
    VirtualFree(retpage, 0, MEM_RELEASE);

    if (failures == 0) {
        printf("All checks passed.\n");
        return 0;
    }
    printf("Failed checks: %d\n", failures);
    return 1;
}
