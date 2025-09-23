OUTPUT_ARCH("riscv")
ENTRY(_entry)

SECTIONS {
  /* 内核起始地址 */
  . = 0x80200000;

  .text : { *(.text) }
  .rodata : { *(.rodata) }
  .data : { *(.data) }

  .bss : {
    __bss_start = .;
    *(.bss)
    __bss_end = .;
  }
}