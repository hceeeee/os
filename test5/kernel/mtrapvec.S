    .section .text
    .align 2
    .globl machinevec
    .type machinevec, @function

machinevec:
    addi    sp, sp, -16
    sd      ra, 0(sp)
    csrr    a0, mcause
    csrr    a1, mepc
    call    machine_trap
    ld      ra, 0(sp)
    addi    sp, sp, 16
    mret