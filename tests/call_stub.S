/*
 * call_stub.S — тестовая обёртка, вызывающая стаб с нужными регистрами.
 *
 * Значения передаются не через аргументы, а через глобальные переменные:
 * так результат не зависит от того, как компилятор передаёт параметры
 * (для naked-функций GCC порядок регистров отличается от стандартного).
 *
 *   srt_test_stub — адрес стаба;
 *   srt_test_obj  — адрес «объекта игры» (в R13);
 *   srt_test_valp — адрес нового значения времени (в XMM2).
 *
 * Возвращает битовую маску:
 *   бит 0 — EFLAGS сохранены стабом;
 *   бит 1 — RAX сохранён стабом;
 *   бит 2 — посторонний регистр (R11) не затронут стабом.
 */
    .data
    .align 8
    .globl  srt_test_stub
srt_test_stub:
    .quad   0
    .globl  srt_test_obj
srt_test_obj:
    .quad   0
    .globl  srt_test_valp
srt_test_valp:
    .quad   0

    .text
    .globl  srt_test_call
    .def    srt_test_call; .scl 2; .type 32; .endef

srt_test_call:
    pushq   %r13                  /* R13 держит адрес часов — сохраняем */
    movq    srt_test_obj(%rip), %rsi
    movq    %rsi, %r13            /* R13 = «объект игры»                  */
    movq    srt_test_valp(%rip), %rdx
    movss   (%rdx), %xmm2         /* XMM2 = новое значение времени        */
    pushfq
    popq    %r10                  /* запомнили флаги до вызова стаба      */
    movabsq $0x1234567890, %r11
    movabsq $0x1234567890, %rax
    pushq   %rax
    call    *srt_test_stub(%rip)  /* вызов стаба                          */
    popq    %rcx                  /* RAX после вызова                     */
    pushfq
    popq    %rdx                  /* флаги после вызова                   */
    xorq    %rax, %rax
    cmpq    %r10, %rdx
    jne     1f
    movq    $1, %rax              /* бит 0: флаги сохранены               */
1:
    movabsq $0x1234567890, %rdx
    cmpq    %rdx, %rcx
    jne     2f
    orq     $2, %rax               /* бит 1: RAX сохранён                  */
2:
    movabsq $0x1234567890, %rdx
    cmpq    %rdx, %r11
    jne     3f
    orq     $4, %rax               /* бит 2: R11 не затронут               */
3:
    popq    %r13
    ret
