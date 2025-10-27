#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <stdint.h>

/* Basic types */
typedef uint64_t pte_t;
typedef uint64_t* pagetable_t;

/* PAGE_SIZE and related */
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096UL
#endif

#ifndef PPN_SHIFT
#define PPN_SHIFT 10UL
#endif

/* PTE flag bits */
#define PTE_V (1ULL << 0)
#define PTE_R (1ULL << 1)
#define PTE_W (1ULL << 2)
#define PTE_X (1ULL << 3)
#define PTE_U (1ULL << 4)
#define PTE_G (1ULL << 5)
#define PTE_A (1ULL << 6)
#define PTE_D (1ULL << 7)

/* Va -> VPN extraction for Sv39 (levels 2,1,0) */
#define VPN_SHIFT(level) (12 + 9 * (level))
#define VPN_MASK(va, level) (((va) >> VPN_SHIFT(level)) & 0x1FFUL)

/* Helper macros for address manipulation */
#define PAGE_ROUND_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ROUND_UP(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* SATP register encoding for Sv39 */
#define SATP_MODE_SV39 (8UL << 60)
#define MAKE_SATP(pt) (SATP_MODE_SV39 | (((uint64_t)(pt)) >> 12))

/* Interface */
pagetable_t create_pagetable(void);
void destroy_pagetable(pagetable_t pt);

pte_t* walk_create(pagetable_t pt, uint64_t va); /* alloc intermediate pages if needed */
pte_t* walk_lookup(pagetable_t pt, uint64_t va); /* no alloc */

int map_page(pagetable_t pt, uint64_t va, uint64_t pa, int perm);
void dump_pagetable(pagetable_t pt);

#endif /* PAGETABLE_H */
