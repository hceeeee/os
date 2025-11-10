    .section .text
    .globl _start
    .align 2
_start:
    # Set up the boot stack. _stack_top is provided by the linker script.
    la      sp, _stack_top

    # Zero the .bss section so that global variables start with a clean slate.
    la      t0, __bss_start
    la      t1, __bss_end
1:
    beq     t0, t1, 2f
    sb      x0, 0(t0)
    addi    t0, t0, 1
    j       1b
2:
    # Jump into the C bootstrap located in start.c.
    call    start

    # We should never come back here. If we do, park the core.
3:
    wfi
    j       3b