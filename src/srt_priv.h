/*
 * srt_priv.h — внутренние объявления ядра.
 * Не включается из графической части программы.
 */
#ifndef SRT_PRIV_H_INCLUDED
#define SRT_PRIV_H_INCLUDED

#include "srt.h"

/* Длина машинного кода стаба (90 байт). */
#define SRT_STUB_LEN 0x5Au

/* Собрать машинный код стаба.
 *   code     — буфер не менее SRT_CODE_BYTES байт;
 *   disp     — смещение часов в объекте игры (адрес = r13 + disp);
 *   code_addr— адрес, по которому стаб будет размещён в игре;
 *   hook_ret — адрес возврата в игру (hook_addr + 9).
 * Все RIP-relative смещения внутри стаба — константы: блок данных всегда
 * находится на 0x1000 байт выше кода. */
void srt_build_stub(uint8_t *code, int32_t disp, uintptr_t code_addr, uintptr_t hook_ret);

/* Найти инструкцию обновления часов в модуле игры.
 * Возвращает список кандидатов (до 16) в out_list.
 * exact = 1 — точное совпадение с проверенной сигнатурой (безопасно),
 * exact = 0 — свободный поиск (используется только если кандидат один). */
typedef struct {
    uintptr_t addr;
    int32_t   disp;
    int       exact;
} SrtHookCandidate;

SrtStatus srt_find_hook(HANDLE proc, uintptr_t base,
                         SrtHookCandidate *out_list, int *out_count,
                         WCHAR *err, int err_chars);

/* Найти PID процесса по имени exe. out_count — сколько таких процессов. */
DWORD srt_find_process(const char *name, int *out_count);

/* База модуля внутри процесса (0 — не найден). */
uintptr_t srt_module_base(DWORD pid, const char *name);

/* Записать DWORD в блок данных (атомарно). */
BOOL srt_data_write32(SrtSession *s, size_t offset, uint32_t value);
BOOL srt_data_read32(SrtSession *s, size_t offset, uint32_t *value);

#endif /* SRT_PRIV_H_INCLUDED */
