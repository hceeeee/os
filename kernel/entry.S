# kernel/entry.S
# 极简启动汇编：栈初始化 → 清零 BSS → 跳到 C → 死循环

.section .text
.global _entry
_entry:
    # ----------------------------
    # 1. 栈初始化
    # ----------------------------
    la t0, stack0        # t0 = stack0
    li t1, 4096    # t1 = 4096
    add sp, t0, t1       # sp = stack0 + STACK_SIZE

    # ----------------------------
    # 2. 清零 BSS 段
    # ----------------------------
    la t0, __bss_start
    la t1, __bss_end
    li t2, 0
1:  bge t0, t1, 2f
    sw t2, 0(t0)
    addi t0, t0, 4
    j 1b
2:

    # ----------------------------
    # 3. 跳转到 C 主函数 main()
    # ----------------------------
    call main

    # ----------------------------
    # 4. 死循环防止返回
    # ----------------------------
spin:
    j spin

# ----------------------------
# 栈空间定义
# ----------------------------
.section .bss
.global stack0
.equ STACK_SIZE, 4096
stack0:
    .space STACK_SIZE
