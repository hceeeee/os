// kernel/main.c
#include "pmm.h"
#include "printf.h"
#include "pagetable.h"
#include <stdint.h>
#include <stddef.h>
#define PHYS_MEM_START 0x80000000UL
#define PHYS_MEM_END   (PHYS_MEM_START + 128 * 1024 * 1024)
void test_physical_memory(void) {
    printf("=== Physical Memory Test Start ===\n");

    void *page1 = alloc_page();
    void *page2 = alloc_page();

    printf("page1=%p, page2=%p\n", page1, page2);

    if (page1 == 0 || page2 == 0) {
        printf("ERROR: alloc returned NULL\n");
    }

    if (((uint64_t)page1 & 0xFFF) == 0) {
        printf("page1 aligned OK\n");
    } else {
        printf("page1 alignment ERROR\n");
    }

    /* 写入测试（注意：page1 不能为 NULL） */
    if (page1) {
        *(int*)page1 = 0x12345678;
        if (*(int*)page1 == 0x12345678) {
            printf("write/read OK: 0x%x\n", *(int*)page1);
        } else {
            printf("write/read ERROR\n");
        }
    }

    free_page(page1);
    void *page3 = alloc_page();
    printf("page3=%p (may equal page1)\n", page3);

    free_page(page2);
    free_page(page3);

    printf("=== Physical Memory Test End ===\n");
}

void test_pagetable(void) {
    pagetable_t pt = create_pagetable();
    if (!pt) { printf("create_pagetable failed\n"); return; }

    /* allocate a physical page to map */
    void *p = alloc_page();
    if (!p) { printf("alloc_page failed\n"); return; }

    uint64_t va = 0x40000000UL; /* test virtual address (page aligned) */
    uint64_t pa = (uint64_t)p;

    if (map_page(pt, va, pa, PTE_R | PTE_W)) {
        printf("map_page failed\n");
        return;
    }

    dump_pagetable(pt);

    /* lookup */
    pte_t *pte = walk_lookup(pt, va);
    if (pte && (*pte & PTE_V)) {
        uint64_t mapped_pa = ((*pte) >> PPN_SHIFT) << 12;
        printf("lookup: va=%p -> pa=%p\n", (void*)va, (void*)mapped_pa);
    } else {
        printf("lookup: not found\n");
    }

    /* cleanup */
    /* free the mapped physical page */
    free_page((void*)pa);
    destroy_pagetable(pt);
}

void main(void) {
    //pmm_init(PHYS_MEM_START, PHYS_MEM_END);
    //test_physical_memory();
    pmm_init(0,0);
    test_pagetable();
    while (1) {
        asm volatile("wfi");
    }
}
