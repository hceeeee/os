// kernel/pagetable.c
#include "pagetable.h"
#include "pmm.h"
#include "printf.h"
#include <stddef.h>
#include <stdint.h>

#define NPTE (PAGE_SIZE / sizeof(pte_t)) /* 512 */

/* helper: convert a PTE (table pointer PPN) into pagetable_t pointer
   assumes physical==virtual mapping for page-table pages in this experiment */
static inline pagetable_t pte_to_table(pte_t pte) {
    uint64_t ppn = (pte >> PPN_SHIFT);
    uint64_t addr = (ppn << 12);
    return (pagetable_t)addr;
}

/* helper: encode child page table pte */
static inline pte_t make_pte_for_table(void* child_page) {
    uint64_t ppn = ((uint64_t)child_page) >> 12;
    return (ppn << PPN_SHIFT) | PTE_V;
}

/* helper: make leaf pte for mapping pa with flags */
static inline pte_t make_leaf_pte(uint64_t pa, int perm) {
    uint64_t ppn = pa >> 12;
    return (ppn << PPN_SHIFT) | (uint64_t)(perm) | PTE_V;
}

/* allocate and zero a page for a page-table page */
static void* alloc_pagetable_page(void) {
    void *p = alloc_page();
    if (!p) return NULL;
    unsigned char *b = (unsigned char*)p;
    for (size_t i = 0; i < PAGE_SIZE; i++) b[i] = 0;
    return p;
}

/* Create a new root page table (one page) */
pagetable_t create_pagetable(void) {
    void *p = alloc_pagetable_page();
    return (pagetable_t)p;
}

/* walk_create: return pointer to PTE for VA at level 0;
   allocate intermediate tables if needed.
   returns NULL on allocation failure or if conflict (leaf at higher level). */
pte_t* walk_create(pagetable_t pt, uint64_t va) {
    if (!pt) return NULL;
    pagetable_t table = pt;
    for (int level = 2; level > 0; level--) {
        uint64_t idx = VPN_MASK(va, level);
        pte_t pte = table[idx];
        if (pte & PTE_V) {
            /* If this PTE is a leaf (has R/W/X), it's a conflict */
            if (pte & (PTE_R|PTE_W|PTE_X)) {
                return NULL;
            }
            table = pte_to_table(pte);
        } else {
            /* allocate child page table */
            void *child = alloc_pagetable_page();
            if (!child) return NULL;
            table[idx] = make_pte_for_table(child);
            table = (pagetable_t)child;
        }
    }
    /* return pointer to PTE at level 0 */
    return &table[VPN_MASK(va, 0)];
}

/* walk_lookup: like walk_create but does not allocate; may return a leaf PTE at higher level */
pte_t* walk_lookup(pagetable_t pt, uint64_t va) {
    if (!pt) return NULL;
    pagetable_t table = pt;
    for (int level = 2; level > 0; level--) {
        uint64_t idx = VPN_MASK(va, level);
        pte_t pte = table[idx];
        if (!(pte & PTE_V)) return NULL;
        if (pte & (PTE_R|PTE_W|PTE_X)) {
            /* higher-level leaf mapping */
            return &table[idx];
        }
        table = pte_to_table(pte);
    }
    return &table[VPN_MASK(va, 0)];
}

/* map_page: create leaf mapping va->pa with perm flags */
int map_page(pagetable_t pt, uint64_t va, uint64_t pa, int perm) {
    if ((va & (PAGE_SIZE-1)) || (pa & (PAGE_SIZE-1))) {
        printf("map_page: addresses must be page aligned\n");
        return -1;
    }
    pte_t *pte = walk_create(pt, va);
    if (!pte) {
        printf("map_page: walk_create failed for va %p\n", (void*)va);
        return -1;
    }
    if ((*pte & PTE_V) && (*pte & (PTE_R|PTE_W|PTE_X))) {
        printf("map_page: VA %p already mapped\n", (void*)va);
        return -1;
    }
    *pte = make_leaf_pte(pa, perm);
    return 0;
}

/* destroy_level: recursively destroy page table pages (only page-table pages).
   We DO NOT free physical pages that were mapped as leaves here. */
static void destroy_level(pagetable_t table, int level) {
    if (!table) return;
    for (int i = 0; i < NPTE; i++) {
        pte_t pte = table[i];
        if (!(pte & PTE_V)) continue;
        /* If it's a leaf (has R/W/X), skip (do not free mapped physical memory) */
        if ( (pte & (PTE_R|PTE_W|PTE_X)) ) {
            table[i] = 0;
            continue;
        }
        /* Non-leaf: child page table */
        pagetable_t child = pte_to_table(pte);
        /* clear entry first to avoid re-entrance issues */
        table[i] = 0;
        /* recurse into child and then free the child page */
        destroy_level(child, level - 1);
        free_page((void*)child);
    }
}

/* destroy entire pagetable rooted at pt (including freeing root page) */
void destroy_pagetable(pagetable_t pt) {
    if (!pt) return;
    destroy_level(pt, 2);
    free_page((void*)pt);
}

/* Dump helpers: compute va_base at this level and recurse */
static void dump_level(pagetable_t table, int level, uint64_t va_base) {
    if (!table) return;
    for (int i = 0; i < NPTE; i++) {
        pte_t pte = table[i];
        if (!(pte & PTE_V)) continue;
        uint64_t va = va_base | ((uint64_t)i << VPN_SHIFT(level));
        if (pte & (PTE_R|PTE_W|PTE_X)) {
            uint64_t pa = (pte >> PPN_SHIFT) << 12;
            int perm = pte & (PTE_R|PTE_W|PTE_X);
            printf("MAP: va=%p -> pa=%p perm=0x%x\n", (void*)va, (void*)pa, perm);
        } else {
            /* non-leaf: recurse to child */
            pagetable_t child = pte_to_table(pte);
            dump_level(child, level - 1, va);
        }
    }
}

void dump_pagetable(pagetable_t pt) {
    printf("Dump pagetable:\n");
    dump_level(pt, 2, 0UL);
}
