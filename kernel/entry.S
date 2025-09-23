    .section .text
    .globl _start
    .align 2
_start:
    /* 设置栈指针为链接脚本定义的 _stack_top */
    la      sp, _stack_top

    /* 清零 .bss: 使用字节循环，保证对任意长度都安全 */
    la      t0, __bss_start
    la      t1, __bss_end
1:  beq     t0, t1, 2f
    sb      x0, 0(t0)
    addi    t0, t0, 1
    j       1b
2:
    /* 调用 C 的 main */
    call    main

    /* 如果 main 返回，进入死循环（wfi + jump） */
3:
    wfi
    j       3b
