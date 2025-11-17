/* Linker script for minimal bare-metal RISC-V kernel */
ENTRY(_start)

/* ???¦Ë DRAM ? */
PROVIDE(_stack_top = 0x80020000);

SECTIONS
{
  /*  0x80000000 ??§µQEMU virt ? DRAM ? */
  . = 0x80000000;

  .text : {
    *(.text*)
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  /* BSS ¦²??/?NOLOAD ? ELF ??¦Ë */
  .bss (NOLOAD) : {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  }
}