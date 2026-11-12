/*
 * srt_stub.c — сборка машинного кода стаба.
 *
 * Стаб перехватывает инструкцию игры
 *
 *     F3 41 0F 11 95 <disp32>    movss dword ptr [r13+disp32], xmm2
 *
 * и подменяет её на себя. Логика стаба:
 *
 *   1) запомнить адрес часов (r13+disp32) и «родное» время игры (xmm2);
 *   2) посмотреть режим в блоке данных:
 *        NATURAL — выполнить исходную инструкку (обычный ход времени);
 *        FROZEN  — ничего не писать (время стоит);
 *        FORCED  — записать в часы значение из блока данных.
 *
 * Стаб не трогает состояние игры: сохраняет и восстанавливает RAX и EFLAGS,
 * регистр R13 и XMM-регистры не изменяет, стек сбалансирован.
 *
 * Код стаба записывается в игру ровно один раз. Переключение режимов —
 * это запись одного DWORD в блок данных, что атомарно и безопасно.
 */
#include <string.h>
#include "srt_priv.h"

/* Смещения внутри стаба. */
#define OFF_PUSHFQ      0x00
#define OFF_PUSH_RAX    0x01
#define OFF_LEA         0x02   /* lea rax, [r13+disp32]            (7 байт) */
#define OFF_SAVE_CLOCK  0x09   /* mov [rip+..], rax  -> D+0x08     (7 байт) */
#define OFF_SAVE_NAT    0x10   /* movss [rip+..], xmm2 -> D+0x10   (8 байт) */
#define OFF_LOAD_MODE   0x18   /* movzx eax, byte [rip+..] -> D+0x00 (7 байт) */
#define OFF_CMP_FREEZE  0x1F   /* cmp al, 1                        (2 байта) */
#define OFF_JE_FREEZE   0x21   /* je freeze                        (2 байта) */
#define OFF_CMP_FORCE   0x23   /* cmp al, 2                        (2 байта) */
#define OFF_JE_FORCE    0x25   /* je force                         (2 байта) */
#define OFF_POP_RAX     0x27
#define OFF_POPFQ       0x28
#define OFF_ORIG        0x29   /* movss [r13+disp32], xmm2          (9 байт) */
#define OFF_JMP_BACK_1  0x32   /* jmp back  (путь NATURAL)          (5 байт) */
#define OFF_FREEZE      0x3B   /* pop rax / popfq / jmp back       (7 байт) */
#define OFF_JMP_BACK_2  0x3D
#define OFF_FORCE       0x46   /* mov eax,[rip+..] -> D+0x04       (6 байт) */
#define OFF_STORE_FORCE 0x4C   /* mov [r13+disp32], eax             (7 байт) */
#define OFF_POP_RAX2    0x53
#define OFF_POPFQ2      0x54
#define OFF_JMP_BACK_3  0x55   /* jmp back  (путь FORCED)           (5 байт) */
#define OFF_END         0x5A

/* RIP-relative смещения до блока данных (данные = код + 0x1000). */
#define REL_CLOCK   0xFF8u   /* 0x1000+0x08 - 0x10 */
#define REL_NATURAL 0xFF8u   /* 0x1000+0x10 - 0x18 */
#define REL_MODE    0xFE1u   /* 0x1000+0x00 - 0x1F */
#define REL_VALUE   0xFB8u   /* 0x1000+0x04 - 0x4C */

static void w32(uint8_t *b, size_t off, uint32_t v) {
    memcpy(b + off, &v, sizeof(v));
}

void srt_build_stub(uint8_t *code, int32_t disp, uintptr_t code_addr, uintptr_t hook_ret)
{
    memset(code, 0x90, SRT_STUB_LEN);   /* NOP-заполнитель между блоками */

    /* pushfq */
    code[OFF_PUSHFQ] = 0x9C;
    /* push rax */
    code[OFF_PUSH_RAX] = 0x50;

    /* lea rax, [r13+disp32] : 49 8D 85 disp32 */
    code[OFF_LEA + 0] = 0x49;
    code[OFF_LEA + 1] = 0x8D;
    code[OFF_LEA + 2] = 0x85;
    w32(code, OFF_LEA + 3, (uint32_t)disp);

    /* mov [rip+rel32], rax : 48 89 05 rel32  -> D+0x08 (адрес часов) */
    code[OFF_SAVE_CLOCK + 0] = 0x48;
    code[OFF_SAVE_CLOCK + 1] = 0x89;
    code[OFF_SAVE_CLOCK + 2] = 0x05;
    w32(code, OFF_SAVE_CLOCK + 3, REL_CLOCK);

    /* movss [rip+rel32], xmm2 : F3 0F 11 15 rel32 -> D+0x10 (родное время)
     * Внимание: в форме записи поле reg кодирует ИСТОЧНИК (xmm2 = 010),
     * поэтому ModRM = 15, а не 05 (иначе сохранялся бы XMM0). */
    code[OFF_SAVE_NAT + 0] = 0xF3;
    code[OFF_SAVE_NAT + 1] = 0x0F;
    code[OFF_SAVE_NAT + 2] = 0x11;
    code[OFF_SAVE_NAT + 3] = 0x15;
    w32(code, OFF_SAVE_NAT + 4, REL_NATURAL);

    /* movzx eax, byte [rip+rel32] : 0F B6 05 rel32 -> D+0x00 (режим) */
    code[OFF_LOAD_MODE + 0] = 0x0F;
    code[OFF_LOAD_MODE + 1] = 0xB6;
    code[OFF_LOAD_MODE + 2] = 0x05;
    w32(code, OFF_LOAD_MODE + 3, REL_MODE);

    /* cmp al, 1 ; je freeze */
    code[OFF_CMP_FREEZE + 0] = 0x3C;
    code[OFF_CMP_FREEZE + 1] = 0x01;
    code[OFF_JE_FREEZE + 0] = 0x74;
    code[OFF_JE_FREEZE + 1] = (uint8_t)(OFF_FREEZE - (OFF_JE_FREEZE + 2));

    /* cmp al, 2 ; je force */
    code[OFF_CMP_FORCE + 0] = 0x3C;
    code[OFF_CMP_FORCE + 1] = 0x02;
    code[OFF_JE_FORCE + 0] = 0x74;
    code[OFF_JE_FORCE + 1] = (uint8_t)(OFF_FORCE - (OFF_JE_FORCE + 2));

    /* NATURAL: pop rax ; popfq ; movss [r13+disp32], xmm2 ; jmp back */
    code[OFF_POP_RAX] = 0x58;
    code[OFF_POPFQ] = 0x9D;
    code[OFF_ORIG + 0] = 0xF3;
    code[OFF_ORIG + 1] = 0x41;
    code[OFF_ORIG + 2] = 0x0F;
    code[OFF_ORIG + 3] = 0x11;
    code[OFF_ORIG + 4] = 0x95;
    w32(code, OFF_ORIG + 5, (uint32_t)disp);
    code[OFF_JMP_BACK_1] = 0xE9;
    w32(code, OFF_JMP_BACK_1 + 1,
        (uint32_t)(int32_t)(hook_ret - (code_addr + OFF_JMP_BACK_1 + 5)));

    /* FROZEN: pop rax ; popfq ; jmp back */
    code[OFF_FREEZE + 0] = 0x58;
    code[OFF_FREEZE + 1] = 0x9D;
    code[OFF_JMP_BACK_2] = 0xE9;
    w32(code, OFF_JMP_BACK_2 + 1,
        (uint32_t)(int32_t)(hook_ret - (code_addr + OFF_JMP_BACK_2 + 5)));

    /* FORCED: mov eax,[rip+rel32] ; mov [r13+disp32], eax ; pop rax ; popfq ; jmp back */
    code[OFF_FORCE + 0] = 0x8B;
    code[OFF_FORCE + 1] = 0x05;
    w32(code, OFF_FORCE + 2, REL_VALUE);
    code[OFF_STORE_FORCE + 0] = 0x41;
    code[OFF_STORE_FORCE + 1] = 0x89;
    code[OFF_STORE_FORCE + 2] = 0x85;
    w32(code, OFF_STORE_FORCE + 3, (uint32_t)disp);
    code[OFF_POP_RAX2] = 0x58;
    code[OFF_POPFQ2] = 0x9D;
    code[OFF_JMP_BACK_3] = 0xE9;
    w32(code, OFF_JMP_BACK_3 + 1,
        (uint32_t)(hook_ret - (code_addr + OFF_JMP_BACK_3 + 5)));
}
