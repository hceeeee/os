/* Linker script for minimal bare-metal RISC-V kernel */
ENTRY(_start)

/* 定义栈顶地址（你可以按需调整） */
PROVIDE(_stack_top = 0x80020000);

SECTIONS
{
  /* 程序从 0x80000000 开始执行（QEMU virt 默认机器映射） */
  . = 0x80000000;

  .text : {
    *(.text*)
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  /* BSS 段：记录开始/结束符号，NOLOAD 表示不在 ELF 文件中占位 */
  .bss (NOLOAD) : {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  }
}
