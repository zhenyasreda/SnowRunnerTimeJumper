/*
 * fake_game.S — «имитация игры» для сквозного теста.
 *
 * Функция fake_update_clock содержит ровно ту же инструкцию, которую
 * ищет программа в настоящей игре:
 *
 *     F3 41 0F 11 95 38 01 00 00     movss dword ptr [r13+0x138], xmm2
 *
 * После неё стоит «rep nop» (F3 0F 1F 00) — так чтобы байт, следующий за
 * найденной инструкцией, был 0xF3 (как в сигнатуре оригинальной программы).
 */
    .text
    .globl  fake_update_clock
    .def    fake_update_clock; .scl 2; .type 32; .endef

fake_update_clock:
    pushq   %r13                 /* R13 — регистр callee-saved, его надо сохранить */
    movq    %rcx, %r13          /* R13 = объект (1-й аргумент в RCX по ABI Windows x64) */
    movss   %xmm1, %xmm2        /* XMM2 = новое время (1-й float в XMM1) */
    movss   %xmm2, 0x138(%r13)  /* ← искомая инструкция */
    rep     nop                  /* F3 0F 1F 00 */
    popq    %r13
    ret
