/*
 * srt_core.c — подключение к игре и управление игровыми часами.
 *
 * Порядок работы:
 *   1) найти процесс SnowRunner.exe и открыть его с минимальными правами;
 *   2) найти в модуле инструкцию обновления часов;
 *   3) выделить в игре 8 КБ (страница кода + страница данных);
 *   4) один раз записать машинный код стаба в страницу кода;
 *   5) заменить найденную инструкку игры на JMP в стаб (9 байт -> 9 байт);
 *   6) закрыть страницу кода на запись (PAGE_EXECUTE_READ).
 *
 * После этого код в игре больше не меняется. Все дальнейшие действия —
 * это запись четырёх байт в блок данных, что атомарно и безопасно.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "srt_priv.h"
#include "i18n.h"

#define HOOK_LEN 9u
#define VERIFY_DELAY_MS 350u

/* Права, которых действительно достаточно:
 * чтение и запись памяти + операции с виртуальной памятью. */
#define SRTC_ACCESS (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | \
                     PROCESS_QUERY_INFORMATION)

struct SrtSession {
    SrtInfo   info;
    HANDLE    proc;

    /* Текущий режим часов: NATURAL / FROZEN / FORCED. */
    SrtMode   mode;

    /* Движок скорости. */
    int       rate_kind;        /* 0 — нет, 1 — фиксированная скорость, 2 — по часам */
    double    ms_per_minute;
    float     base_hours;
    uint64_t  base_tick;
};

/* ------------------------------------------------------------------ */
/*  Вспомогательные функции                                            */
/* ------------------------------------------------------------------ */

BOOL srt_data_write32(SrtSession *s, size_t offset, uint32_t value)
{
    if (!s || !s->info.attached) return FALSE;
    SIZE_T written = 0;
    if (!WriteProcessMemory(s->proc, (LPVOID)(s->info.data_addr + offset),
                            &value, sizeof(value), &written))
        return FALSE;
    return written == sizeof(value);
}

BOOL srt_data_read32(SrtSession *s, size_t offset, uint32_t *value)
{
    if (!s || !s->info.attached) return FALSE;
    SIZE_T got = 0;
    if (!ReadProcessMemory(s->proc, (LPCVOID)(s->info.data_addr + offset),
                           value, sizeof(*value), &got))
        return FALSE;
    return got == sizeof(*value);
}

BOOL srt_data_read64(SrtSession *s, size_t offset, uint64_t *value)
{
    if (!s || !s->info.attached) return FALSE;
    SIZE_T got = 0;
    if (!ReadProcessMemory(s->proc, (LPCVOID)(s->info.data_addr + offset),
                           value, sizeof(*value), &got))
        return FALSE;
    return got == sizeof(*value);
}

float srt_wrap_hours(float h)
{
    if (!(h == h)) return 0.0f;              /* NaN */
    h = fmodf(h, 24.0f);
    if (h < 0.0f) h += 24.0f;
    return h;
}

double srt_local_time(void)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    return (double)st.wHour
         + (double)st.wMinute / 60.0
         + (double)st.wSecond / 3600.0
         + (double)st.wMilliseconds / 3600000.0;
}

/* ------------------------------------------------------------------ */
/*  Выделение памяти рядом с модулем игры                              */
/* ------------------------------------------------------------------ */

/* JMP rel32 позволяет прыгнуть только в пределах ±2 ГБ, поэтому блок
 * стараемся разместить рядом с кодом игры. */
static BOOL reachable(uintptr_t a, uintptr_t b)
{
    int64_t d = (int64_t)a - (int64_t)b;
    if (d < 0) d = -d;
    return d < 0x7FF00000LL;
}

static uintptr_t alloc_block(HANDLE proc, uintptr_t base, uintptr_t hook)
{
    static const uintptr_t hints[] = {
        0x1000, 0x2000, 0x4000, 0x8000, 0x10000, 0x20000, 0x40000, 0x80000
    };
    /* Страница кода должна быть ИСПОЛНЯЕМОЙ, а страница данных — записываемой,
     * поэтому выделяем всё с правами RWX, а после записи стаба разделяем права:
     * код -> EXECUTE_READ, данные -> READWRITE. */
    for (size_t i = 0; i < sizeof(hints) / sizeof(hints[0]); i++) {
        uintptr_t hint = base - hints[i];
        void *p = VirtualAllocEx(proc, (LPVOID)hint, SRT_BLOCK_BYTES,
                                 MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!p) continue;
        if (reachable((uintptr_t)p, hook)) return (uintptr_t)p;
        VirtualFreeEx(proc, p, 0, MEM_RELEASE);
    }
    void *p = VirtualAllocEx(proc, NULL, SRT_BLOCK_BYTES,
                             MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (p && reachable((uintptr_t)p, hook)) return (uintptr_t)p;
    if (p) VirtualFreeEx(proc, p, 0, MEM_RELEASE);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Установка перехвата                                                */
/* ------------------------------------------------------------------ */

static BOOL write_stub(SrtSession *s, int32_t disp)
{
    uint8_t *code = (uint8_t *)calloc(1, SRT_CODE_BYTES);
    uint8_t *data = (uint8_t *)calloc(1, SRT_DATA_BYTES);
    if (!code || !data) { free(code); free(data); return FALSE; }

    srt_build_stub(code, disp, s->info.code_addr, s->info.hook_addr + HOOK_LEN);

    *(uint32_t *)(data + SRT_DATA_MAGIC)   = 0x43545253u;   /* 'SRTC' */
    *(uint32_t *)(data + SRT_DATA_VERSION) = 1u;
    *(uint32_t *)(data + SRT_DATA_MODE)    = (uint32_t)SRT_MODE_NATURAL;

    SIZE_T w1 = 0, w2 = 0;
    BOOL ok = WriteProcessMemory(s->proc, (LPVOID)s->info.code_addr,
                                 code, SRT_CODE_BYTES, &w1) && w1 == SRT_CODE_BYTES;
    ok = ok && WriteProcessMemory(s->proc, (LPVOID)s->info.data_addr,
                                  data, SRT_DATA_BYTES, &w2) && w2 == SRT_DATA_BYTES;
    free(code);
    free(data);
    return ok;
}

static BOOL patch_hook(SrtSession *s)
{
    SIZE_T got = 0;
    if (!ReadProcessMemory(s->proc, (LPCVOID)s->info.hook_addr,
                           s->info.orig_bytes, HOOK_LEN, &got) || got != HOOK_LEN)
        return FALSE;

    /* Проверяем, что на месте действительно та инструкция, которую мы
     * собирались перехватить: F3 41 0F 11 95 <disp32>. Иначе — это не
     * обновление часов, и трогать код нельзя. */
    if (s->info.orig_bytes[0] != 0xF3 || s->info.orig_bytes[1] != 0x41 ||
        s->info.orig_bytes[2] != 0x0F || s->info.orig_bytes[3] != 0x11 ||
        s->info.orig_bytes[4] != 0x95) {
        log_msg(S_LOG_PATCH_BYTES);
        return FALSE;
    }
    uint32_t disp = (uint32_t)s->info.orig_bytes[5] |
                     ((uint32_t)s->info.orig_bytes[6] << 8) |
                     ((uint32_t)s->info.orig_bytes[7] << 16) |
                     ((uint32_t)s->info.orig_bytes[8] << 24);
    if ((int32_t)disp != s->info.disp) {
        log_msg(S_LOG_PATCH_MISMATCH,
                (unsigned)disp, (unsigned)s->info.disp);
        return FALSE;
    }

    DWORD old_prot = 0;
    if (!VirtualProtectEx(s->proc, (LPVOID)s->info.hook_addr, HOOK_LEN,
                           PAGE_EXECUTE_READWRITE, &old_prot))
        return FALSE;
    s->info.orig_protect = old_prot;

    uint8_t patch[HOOK_LEN];
    patch[0] = 0xE9;                                   /* JMP rel32 */
    int32_t rel = (int32_t)((int64_t)s->info.code_addr - (int64_t)(s->info.hook_addr + 5));
    memcpy(patch + 1, &rel, 4);
    memset(patch + 5, 0x90, 4);                        /* 4 x NOP */

    SIZE_T written = 0;
    BOOL ok = WriteProcessMemory(s->proc, (LPVOID)s->info.hook_addr,
                                 patch, HOOK_LEN, &written) && written == HOOK_LEN;

    DWORD ignored = 0;
    VirtualProtectEx(s->proc, (LPVOID)s->info.hook_addr, HOOK_LEN, old_prot, &ignored);
    return ok;
}

static void unpatch_hook(SrtSession *s)
{
    if (!s->info.attached) return;
    DWORD old_prot = 0;
    if (VirtualProtectEx(s->proc, (LPVOID)s->info.hook_addr, HOOK_LEN,
                          PAGE_EXECUTE_READWRITE, &old_prot)) {
        SIZE_T written = 0;
        WriteProcessMemory(s->proc, (LPVOID)s->info.hook_addr,
                           s->info.orig_bytes, HOOK_LEN, &written);
        DWORD ignored = 0;
        VirtualProtectEx(s->proc, (LPVOID)s->info.hook_addr, HOOK_LEN,
                         s->info.orig_protect ? s->info.orig_protect : PAGE_EXECUTE_READ,
                         &ignored);
    }
}

/* Проверка: стаб действительно выполняется игрой и значение похоже на часы. */
static BOOL verify_hook(SrtSession *s)
{
    Sleep(VERIFY_DELAY_MS);

    uint64_t clock_addr = 0;
    if (!srt_data_read64(s, SRT_DATA_CLOCK, &clock_addr)) return FALSE;
    if (clock_addr == 0) return FALSE;                 /* стаб не вызывался */

    uint32_t natural_bits = 0;
    if (!srt_data_read32(s, SRT_DATA_NATURAL, &natural_bits)) return FALSE;
    float natural = 0.0f;
    memcpy(&natural, &natural_bits, sizeof(natural));
    if (!(natural == natural)) return FALSE;                  /* NaN */
    if (natural < -0.01f || natural > 24.5f) {
        /* Значение не похоже на часы (0..24) — значит перехвачена не та
         * инструкция. Откатываем подмену. */
        log_msg(S_LOG_VERIFY_BAD,
                (double)natural);
        return FALSE;
    }

        log_msg(S_LOG_VERIFY,
            (unsigned long long)clock_addr, (double)natural);
    return TRUE;
}
/* ------------------------------------------------------------------ */
/*  Публичные функции                                                   */
/* ------------------------------------------------------------------ */

SrtStatus srt_attach(SrtSession **out, WCHAR *err, int err_chars)
{
    if (out) *out = NULL;
    if (err && err_chars > 0) err[0] = 0;

    int count = 0;
    DWORD pid = srt_find_process(SRT_GAME_PROCESS, &count);
    if (pid == 0) {
        WCHAR wname[64];
        MultiByteToWideChar(CP_UTF8, 0, SRT_GAME_PROCESS, -1, wname, 64);
        if (err) lang_format(err, err_chars, S_ERR_NOT_FOUND, wname);
        return SRT_E_NOT_FOUND;
    }
    if (count > 1) {
        srt_log("warning: %d processes %s found, using PID %lu",
                count, SRT_GAME_PROCESS, (unsigned long)pid);
    }

    HANDLE proc = OpenProcess(SRTC_ACCESS, FALSE, pid);
    if (!proc) {
        if (err) lang_format(err, err_chars, S_ERR_OPEN, (unsigned long)GetLastError());
        return SRT_E_OPEN;
    }

    uintptr_t base = srt_module_base(pid, SRT_GAME_PROCESS);
    if (!base) {
        CloseHandle(proc);
        if (err) lang_format(err, err_chars, S_ERR_MODULE);
        return SRT_E_PATTERN;
    }

    SrtHookCandidate cands[16];
    int ncand = 0;
    SrtStatus st = srt_find_hook(proc, base, cands, &ncand, err, err_chars);
    if (st != SRT_OK) {
        CloseHandle(proc);
        return st;
    }
    log_msg(S_LOG_ATTACH_PID, (unsigned long)pid, (unsigned long long)base, ncand);

    SrtSession *s = (SrtSession *)calloc(1, sizeof(SrtSession));
    if (!s) {
        CloseHandle(proc);
        if (err) lang_format(err, err_chars, S_ERR_ALLOC);
        return SRT_E_ALLOC;
    }
    s->proc = proc;
    s->info.pid = pid;
    strncpy(s->info.process, SRT_GAME_PROCESS, sizeof(s->info.process) - 1);
    s->info.candidates = ncand;

    SrtStatus result = SRT_E_PATTERN;
    int tries = ncand < 8 ? ncand : 8;

    for (int i = 0; i < tries; i++) {
        WCHAR dispbuf[16];
        swprintf(dispbuf, 16, L"%X", (unsigned)(unsigned int)cands[i].disp);
        log_msg(S_LOG_ATTACH_CAND, i + 1, tries,
                (unsigned long long)cands[i].addr, dispbuf);
        s->info.hook_addr = cands[i].addr;
        s->info.disp      = cands[i].disp;
        s->info.code_addr = alloc_block(proc, base, cands[i].addr);
        if (!s->info.code_addr) {
            if (err) lang_format(err, err_chars, S_ERR_ALLOC);
            result = SRT_E_ALLOC;
            break;
        }
        s->info.data_addr = s->info.code_addr + SRT_CODE_BYTES;
        log_msg(S_LOG_ATTACH_BLOCK,
                (unsigned long long)s->info.code_addr,
                (unsigned long long)s->info.data_addr);

        if (!write_stub(s, cands[i].disp)) {
            VirtualFreeEx(proc, (LPVOID)s->info.code_addr, 0, MEM_RELEASE);
            if (err) lang_format(err, err_chars, S_ERR_IO);
            result = SRT_E_IO;
            break;
        }
        if (!patch_hook(s)) {
            /* На месте не та инструкция — пробуем следующего кандидата. */
            VirtualFreeEx(proc, (LPVOID)s->info.code_addr, 0, MEM_RELEASE);
            s->info.code_addr = 0;
            s->info.data_addr = 0;
            continue;
        }

        s->info.attached = TRUE;

        BOOL good = verify_hook(s);
        if (good) {
            /* Код стаба больше не меняется — закрываем страницу на запись.
             * Данные остаются записываемыми: туда пишет программа. */
            DWORD old = 0;
            VirtualProtectEx(proc, (LPVOID)s->info.code_addr, SRT_CODE_BYTES,
                             PAGE_EXECUTE_READ, &old);
            VirtualProtectEx(proc, (LPVOID)s->info.data_addr, SRT_DATA_BYTES,
                             PAGE_READWRITE, &old);
            result = SRT_OK;
            log_msg(S_LOG_ATTACH_OK,
                    (unsigned long long)s->info.hook_addr,
                    (unsigned)(unsigned int)cands[i].disp,
                    (unsigned long long)s->info.code_addr);
            break;
        }

        /* Не то место — откатываем и пробуем следующего кандидата. */
        log_msg(S_LOG_ATTACH_RETRY, i + 1);
        unpatch_hook(s);
        s->info.attached = FALSE;
        VirtualFreeEx(proc, (LPVOID)s->info.code_addr, 0, MEM_RELEASE);
        s->info.code_addr = 0;
        s->info.data_addr = 0;
    }

    if (result != SRT_OK) {
        if (err && err[0] == 0)
            lang_format(err, err_chars, S_ERR_GENERIC);
        CloseHandle(proc);
        free(s);
        return result;
    }

    srt_set_mode(s, SRT_MODE_NATURAL);
    if (out) *out = s;
    return SRT_OK;
}

void srt_detach(SrtSession *s)
{
    if (!s) return;
    if (s->info.attached) {
        srt_set_mode(s, SRT_MODE_NATURAL);
        Sleep(120);
        unpatch_hook(s);
        Sleep(200);                       /* дать игре выйти из стаба */
        if (s->info.code_addr)
            VirtualFreeEx(s->proc, (LPVOID)s->info.code_addr, 0, MEM_RELEASE);
        log_msg(S_LOG_DETACH, (unsigned long)s->info.pid);
    }
    if (s->proc) CloseHandle(s->proc);
    free(s);
}

BOOL srt_is_attached(const SrtSession *s)
{
    return s && s->info.attached;
}

const SrtInfo *srt_info(const SrtSession *s)
{
    return s ? &s->info : NULL;
}

BOOL srt_set_mode(SrtSession *s, SrtMode mode)
{
    if (!s || !s->info.attached) return FALSE;
    if (mode != SRT_MODE_FORCED) s->rate_kind = 0;
    if (!srt_data_write32(s, SRT_DATA_MODE, (uint32_t)mode)) return FALSE;
    s->mode = mode;
    return TRUE;
}

/* Записать значение часов, не трогая настройку скорости. */
static BOOL write_value(SrtSession *s, float hours)
{
    if (!s || !s->info.attached) return FALSE;
    uint32_t bits = 0;
    memcpy(&bits, &hours, sizeof(bits));
    if (!srt_data_write32(s, SRT_DATA_VALUE, bits)) return FALSE;
    if (!srt_data_write32(s, SRT_DATA_MODE, (uint32_t)SRT_MODE_FORCED)) return FALSE;
    s->mode = SRT_MODE_FORCED;
    return TRUE;
}

BOOL srt_force_value(SrtSession *s, float hours)
{
    if (!s || !s->info.attached) return FALSE;
    s->rate_kind = 0;                 /* фиксированное значение снимает скорость */
    return write_value(s, hours);
}

SrtMode srt_mode(const SrtSession *s)
{
    return s ? s->mode : SRT_MODE_NATURAL;
}

BOOL srt_read_clock(SrtSession *s, float *hours)
{
    if (!s || !s->info.attached) return FALSE;
    uint64_t clock_addr = 0;
    if (!srt_data_read64(s, SRT_DATA_CLOCK, &clock_addr)) return FALSE;
    if (clock_addr == 0) return FALSE;
    uint32_t bits = 0;
    SIZE_T got = 0;
    if (!ReadProcessMemory(s->proc, (LPCVOID)clock_addr, &bits, 4, &got)) return FALSE;
    if (got != 4) return FALSE;
    memcpy(hours, &bits, sizeof(*hours));
    return TRUE;
}

BOOL srt_read_natural(SrtSession *s, float *hours)
{
    if (!s || !s->info.attached) return FALSE;
    uint32_t bits = 0;
    if (!srt_data_read32(s, SRT_DATA_NATURAL, &bits)) return FALSE;
    memcpy(hours, &bits, sizeof(*hours));
    return TRUE;
}

BOOL srt_rate_start_at(SrtSession *s, double ms_per_game_minute, float base_hours)
{
    if (!s || !s->info.attached) return FALSE;
    s->base_hours    = base_hours;
    s->base_tick     = GetTickCount64();
    s->ms_per_minute = ms_per_game_minute;
    if (!write_value(s, base_hours)) return FALSE;
    s->rate_kind = 1;
    return TRUE;
}

BOOL srt_rate_start(SrtSession *s, double ms_per_game_minute)
{
    if (!s || !s->info.attached) return FALSE;
    float cur = 0.0f;
    if (!srt_read_clock(s, &cur)) cur = 12.0f;
    return srt_rate_start_at(s, ms_per_game_minute, cur);
}

BOOL srt_rate_sync(SrtSession *s)
{
    if (!s || !s->info.attached) return FALSE;
    s->base_tick = GetTickCount64();
    if (!write_value(s, (float)srt_local_time())) return FALSE;
    s->rate_kind = 2;
    return TRUE;
}

void srt_rate_stop(SrtSession *s)
{
    if (!s) return;
    s->rate_kind = 0;
}

BOOL srt_rate_tick(SrtSession *s)
{
    if (!s || !s->info.attached || s->rate_kind == 0) return FALSE;
    if (s->rate_kind == 2) {
        return write_value(s, (float)srt_local_time());
    }
    uint64_t now = GetTickCount64();
    double elapsed = (double)(now - s->base_tick);
    double game_minutes = elapsed / s->ms_per_minute;
    float v = srt_wrap_hours(s->base_hours + (float)(game_minutes / 60.0));
    return write_value(s, v);
}

BOOL srt_rate_active(const SrtSession *s)
{
    return s && s->rate_kind != 0;
}

BOOL srt_rate_is_sync(const SrtSession *s)
{
    return s && s->rate_kind == 2;
}

double srt_rate_ms_per_minute(const SrtSession *s)
{
    return s ? s->ms_per_minute : 0.0;
}
