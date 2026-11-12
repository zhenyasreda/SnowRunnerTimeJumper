/*
 * srt_scan.c — поиск процесса, модуля и точки перехвата в памяти игры.
 *
 * Точка перехвата ищется по инструкции
 *
 *     F3 41 0F 11 95 <disp32>    movss dword ptr [r13+disp32], xmm2
 *
 * Первые пять байт жёстко фиксированы, а смещение часов — нет: так
 * программа не сломается, если в следующей версии игры разработчики
 * переставят счётчик часов в другое место структуры.
 *
 * ПОРЯДОК ПОИСКА (важно для безопасности):
 *
 *   1. Сначала ищутся ТОЧНЫЕ сигнатуры, известные для проверенных версий
 *      игры: смещение 0x138 и 0x150. Это ровно те байты, которые ищет
 *      оригинальная программа, — они находятся в коде единственный раз.
 *
 *   2. Только если точных совпадений нет, выполняется свободный поиск
 *      (смещение неизвестно). Он допускается лишь тогда, когда кандидат
 *      ровно один: иначе подключение не выполняется. Перехватить не ту
 *      инструкцию — значит испортить игру, поэтому программа предпочитает
 *      честно сообщить, что версия игры изменилась.
 *
 * Дополнительно проверяется, что найденное место лежит в исполняемой
 * секции модуля — это отсекает случайные совпадения в данных.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>
#include "srt_priv.h"

#define MAX_CANDIDATES 16
#define CHUNK_SIZE      (1024u * 1024u)
#define PATTERN_LEN     10u

/* Байты искомой инструкции (без смещения). */
static const uint8_t SRT_PATTERN[5] = { 0xF3, 0x41, 0x0F, 0x11, 0x95 };

/* Смещения часов в проверенных версиях игры. */
static const uint32_t KNOWN_DISP[] = { 0x138u, 0x150u };

/* ------------------------------------------------------------------ */
/*  Процессы и модули                                                   */
/* ------------------------------------------------------------------ */

static void ascii_to_wide(const char *src, WCHAR *dst, size_t dst_chars)
{
    size_t i = 0;
    if (dst_chars == 0) return;
    for (; src[i] && i + 1 < dst_chars; i++)
        dst[i] = (WCHAR)(unsigned char)src[i];
    dst[i] = 0;
}

DWORD srt_find_process(const char *name, int *out_count)
{
    PROCESSENTRY32W entry;
    WCHAR wname[MAX_PATH];
    DWORD pid = 0;
    int count = 0;

    ascii_to_wide(name, wname, MAX_PATH);

    entry.dwSize = sizeof(entry);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        if (out_count) *out_count = 0;
        return 0;
    }
    if (Process32FirstW(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, wname) == 0) {
                count++;
                pid = entry.th32ProcessID;   /* берём последний найденный */
            }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
    if (out_count) *out_count = count;
    return count > 0 ? pid : 0;
}

uintptr_t srt_module_base(DWORD pid, const char *name)
{
    MODULEENTRY32W me;
    WCHAR wname[MAX_PATH];
    uintptr_t base = 0;

    ascii_to_wide(name, wname, MAX_PATH);

    me.dwSize = sizeof(me);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, wname) == 0) {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

/* ------------------------------------------------------------------ */
/*  Разбор PE- заголовков                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t rva;
    uint32_t size;
} Section;

typedef struct {
    Section sections[32];
    int     count;
    uint32_t image_size;   /* SizeOfImage, 0 — неизвестно */
} ModuleInfo;

/* Читает PE-заголовки: список исполняемых секций и SizeOfImage. */
static int read_module_info(HANDLE proc, uintptr_t base, ModuleInfo *info)
{
    uint8_t hdr[0x1000];
    SIZE_T got = 0;

    memset(info, 0, sizeof(*info));

    if (!ReadProcessMemory(proc, (LPCVOID)base, hdr, sizeof(hdr), &got))
        return 0;
    if (got < 0x40) return 0;

    uint32_t lfanew = *(uint32_t *)(hdr + 0x3C);
    if (lfanew + 0x100 > got) return 0;
    if (memcmp(hdr + lfanew, "PE\0\0", 4) != 0) return 0;

    const uint8_t *fh = hdr + lfanew + 4;                 /* IMAGE_FILE_HEADER */
    uint16_t number_of_sections = *(uint16_t *)(fh + 2);
    uint16_t size_of_optional  = *(uint16_t *)(fh + 16);
    const uint8_t *opt = fh + 20;                        /* IMAGE_OPTIONAL_HEADER */
    uint16_t magic = *(uint16_t *)opt;
    if (magic != 0x20B && magic != 0x10B) return 0;      /* PE32+ или PE32 */

    if (magic == 0x20B)
        info->image_size = *(uint32_t *)(opt + 0x38);     /* SizeOfImage */
    else
        info->image_size = *(uint32_t *)(opt + 0x38);     /* то же смещение в PE32 */

    const uint8_t *sections = opt + size_of_optional;
    if ((size_t)(sections - hdr) + (size_t)number_of_sections * 40 > got)
        return 0;

    for (uint16_t i = 0; i < number_of_sections && info->count < 32; i++) {
        const uint8_t *sh = sections + (size_t)i * 40;
        uint32_t virtual_size  = *(uint32_t *)(sh + 8);
        uint32_t virtual_addr  = *(uint32_t *)(sh + 12);
        uint32_t size_of_raw   = *(uint32_t *)(sh + 16);
        uint32_t characteristics = *(uint32_t *)(sh + 36);
        if (!(characteristics & 0x20000000u)) continue;   /* IMAGE_SCN_MEM_EXECUTE */
        uint32_t size = virtual_size ? virtual_size : size_of_raw;
        if (size == 0) continue;
        info->sections[info->count].rva  = virtual_addr;
        info->sections[info->count].size = size;
        info->count++;
    }
    return 1;
}

/* Адрес находится в исполняемой секции? */
static int is_executable(const ModuleInfo *info, uintptr_t addr)
{
    if (info->count == 0) return 1;         /* сведений нет — не фильтруем */
    for (int i = 0; i < info->count; i++) {
        uintptr_t start = (uintptr_t)info->sections[i].rva;
        uintptr_t end   = start + info->sections[i].size;
        if (addr >= start && addr < end) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Поиск по памяти                                                     */
/* ------------------------------------------------------------------ */

/* Поиск точной последовательности байт в диапазоне [start, start+size). */
static int scan_exact(HANDLE proc, uintptr_t start, uint32_t size,
                      const uint8_t *pat, int patlen,
                      SrtHookCandidate *out, int max_out)
{
    uint8_t *buf = (uint8_t *)malloc(CHUNK_SIZE + (size_t)patlen);
    int found = 0;
    if (!buf) return 0;

    uint32_t pos = 0;
    while (pos < size && found < max_out) {
        uint32_t want = size - pos;
        if (want > CHUNK_SIZE) want = CHUNK_SIZE;

        SIZE_T got = 0;
        if (!ReadProcessMemory(proc, (LPCVOID)(start + pos), buf, want, &got) || got < 5)
            break;

        for (SIZE_T i = 0; i + (SIZE_T)patlen <= got && found < max_out; i++) {
            if (memcmp(buf + i, pat, (size_t)patlen) != 0) continue;
            uintptr_t addr = start + pos + i;
            out[found].addr = addr;
            out[found].disp = (int32_t)(uint32_t)pat[5] |
                              ((int32_t)(uint32_t)pat[6] << 8) |
                              ((int32_t)(uint32_t)pat[7] << 16) |
                              ((int32_t)(uint32_t)pat[8] << 24);
            out[found].exact = 1;
            found++;
        }
        if (got <= (SIZE_T)patlen - 1) break;
        pos += (uint32_t)got - ((uint32_t)patlen - 1);
        if (got < want) break;              /* дошли до конца */
    }
    free(buf);
    return found;
}

/* Свободный поиск: префикс инструкции и любое небольшое смещение.
 * Используется только как запасной вариант. */
static int scan_loose(HANDLE proc, uintptr_t start, uint32_t size,
                      const ModuleInfo *info,
                      SrtHookCandidate *out, int max_out)
{
    uint8_t *buf = (uint8_t *)malloc(CHUNK_SIZE + PATTERN_LEN);
    int found = 0;
    if (!buf) return 0;

    uint32_t pos = 0;
    while (pos < size && found < max_out) {
        uint32_t want = size - pos;
        if (want > CHUNK_SIZE) want = CHUNK_SIZE;

        SIZE_T got = 0;
        if (!ReadProcessMemory(proc, (LPCVOID)(start + pos), buf, want, &got) || got < 5)
            break;

        for (SIZE_T i = 0; i + PATTERN_LEN <= got && found < max_out; i++) {
            if (memcmp(buf + i, SRT_PATTERN, sizeof(SRT_PATTERN)) != 0) continue;

            uint32_t disp = (uint32_t)buf[i + 5] | ((uint32_t)buf[i + 6] << 8) |
                            ((uint32_t)buf[i + 7] << 16) | ((uint32_t)buf[i + 8] << 24);
            if (disp > 0x1000u) continue;                 /* слишком большое смещение */
            if (buf[i + 9] != 0xF3) continue;             /* как в исходной программе */

            uintptr_t addr = start + pos + i;
            if (!is_executable(info, addr - (uintptr_t)start)) continue;

            out[found].addr = addr;
            out[found].disp = (int32_t)disp;
            out[found].exact = 0;
            found++;
        }
        if (got <= PATTERN_LEN - 1) break;
        pos += (uint32_t)got - (PATTERN_LEN - 1);
        if (got < want) break;
    }
    free(buf);
    return found;
}

SrtStatus srt_find_hook(HANDLE proc, uintptr_t base,
                         SrtHookCandidate *out_list, int *out_count,
                         char *err, size_t errsz)
{
    ModuleInfo info;
    SrtHookCandidate tmp[MAX_CANDIDATES];
    int total = 0;
    int n;

    read_module_info(proc, base, &info);

    /* Размер модуля: если SizeOfImage неизвестен — читаем, пока не кончится. */
    uint32_t size = info.image_size ? info.image_size : 0x4000000u;   /* 64 МБ — с запасом */

    /* --- проход 1: точные сигнатуры для известных версий игры --- */
    for (size_t k = 0; k < sizeof(KNOWN_DISP) / sizeof(KNOWN_DISP[0]); k++) {
        uint8_t pat[PATTERN_LEN];
        memcpy(pat, SRT_PATTERN, 5);
        pat[5] = (uint8_t)(KNOWN_DISP[k] & 0xFF);
        pat[6] = (uint8_t)((KNOWN_DISP[k] >> 8) & 0xFF);
        pat[7] = (uint8_t)((KNOWN_DISP[k] >> 16) & 0xFF);
        pat[8] = (uint8_t)((KNOWN_DISP[k] >> 24) & 0xFF);
        pat[9] = 0xF3;

        n = scan_exact(proc, base, size, pat, (int)PATTERN_LEN,
                       tmp + total, MAX_CANDIDATES - total);
        total += n;
        if (total > 0) break;              /* точная сигнатура уже найдена */
    }

    if (total > 0) {
        memcpy(out_list, tmp, sizeof(SrtHookCandidate) * (size_t)total);
        if (out_count) *out_count = total;
        srt_log("scan: точных совпадений %d", total);
        for (int i = 0; i < total; i++)
            srt_log("scan:   адрес=%llX смещение=0x%X",
                    (unsigned long long)out_list[i].addr,
                    (unsigned)(unsigned int)out_list[i].disp);
        return SRT_OK;
    }

    /* --- проход 2: свободный поиск (смещение неизвестно) --- */
    total = 0;
    n = scan_loose(proc, base, size, &info, tmp + total, MAX_CANDIDATES - total);
    total += n;

    if (total == 0) {
        if (err) snprintf(err, errsz,
            "В модуле игры не найдено инструкции обновления часов.\n"
            "Похоже, игра обновилась: напишите разработчику программы.");
        return SRT_E_PATTERN;
    }

    if (total > 1) {
        if (err) snprintf(err, errsz,
            "В коде игры найдено несколько похожих мест (%d), а известной "
            "сигнатуры нет.\n\nПодключение не выполняется, чтобы не нарушить "
            "работу игры. Похоже, игра обновилась: напишите разработчику.",
            total);
        return SRT_E_AMBIGUOUS;
    }

    memcpy(out_list, tmp, sizeof(SrtHookCandidate));
    if (out_count) *out_count = 1;
    srt_log("scan: свободный поиск дал единственное совпадение адрес=%llX смещение=0x%X",
            (unsigned long long)out_list[0].addr,
            (unsigned)(unsigned int)out_list[0].disp);
    return SRT_OK;
}
