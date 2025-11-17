    .section .text
    .align 2
    .globl timervec
    .type timervec, @function

/* Machine-mode timer interrupt handler:
   - Program next mtimecmp
   - Set SIP.STIP to forward interrupt to S-mode
   - Return with mret to resume S-mode execution
*/
timervec:
    addi    sp, sp, -32
    sd      ra, 0(sp)
    sd      t0, 8(sp)
    sd      t1, 16(sp)
    sd      t2, 24(sp)

    /* read mtime */
    li      t0, 0x0200bff8          /* CLINT_MTIME */
    ld      t1, 0(t0)               /* t1 = mtime */

    /* t1 = now + interval (TIMEBASE_HZ/HZ) = 10_000_000/100 = 100000 */
    li      t2, 100000
    add     t1, t1, t2

    /* write mtimecmp for this hart: 0x02004000 + 8*mhartid */
    csrr    t0, mhartid
    slli    t0, t0, 3               /* *8 */
    li      t2, 0x02004000          /* CLINT_MTIMECMP base */
    add     t2, t2, t0
    sd      t1, 0(t2)

    /* set S-mode software interrupt pending: SIP.SSIP (bit 1) */
    csrr    t0, sip
    ori     t0, t0, 0x2
    csrw    sip, t0

    ld      t2, 24(sp)
    ld      t1, 16(sp)
    ld      t0, 8(sp)
    ld      ra, 0(sp)
    addi    sp, sp, 32
    mret