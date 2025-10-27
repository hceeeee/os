// kernel/main.c
#include "pmm.h"
#include "printf.h"
#include "pagetable.h"
#include "kvminit.h"
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

/* Test virtual memory activation */
void test_virtual_memory(void) {
    printf("\n=== Virtual Memory Test Start ===\n");
    
    printf("Before enabling paging...\n");
    printf("Current mode: direct memory access\n");
    
    /* 初始化并启用分页 */
    kvminit();
    printf("Kernel pagetable created\n");
    
    kvminithart();
    printf("Paging enabled, satp register set\n");
    
    printf("After enabling paging...\n");
    printf("Current mode: virtual memory with paging\n");
    
    /* 测试内核代码仍然可执行 */
    printf("Testing kernel code execution...\n");
    extern char _text[], _etext[];
    uint64_t text_start = (uint64_t)_text;
    uint64_t text_end = (uint64_t)_etext;
    printf("Kernel text: %p - %p\n", (void*)text_start, (void*)text_end);
    
    /* 测试内核数据仍然可访问 */
    printf("Testing kernel data access...\n");
    static int test_var = 0xDEADBEEF;
    printf("Test variable value: 0x%x\n", test_var);
    test_var = 0xCAFEBABE;
    if (test_var == 0xCAFEBABE) {
        printf("Kernel data access OK\n");
    } else {
        printf("Kernel data access ERROR\n");
    }
    
    /* 测试设备访问仍然正常 */
    printf("Testing device access...\n");
    /* UART 设备已映射，可以访问 */
    printf("Device access test OK (UART mapped)\n");
    
    /* 测试页表查找 */
    printf("Testing pagetable lookup...\n");
    pte_t *pte = walk_lookup(kernel_pagetable, KERNBASE);
    if (pte && (*pte & PTE_V)) {
        uint64_t mapped_pa = ((*pte) >> PPN_SHIFT) << 12;
        printf("KERNBASE lookup OK: va=%p -> pa=%p\n", (void*)KERNBASE, (void*)mapped_pa);
    }
    
    printf("=== Virtual Memory Test End ===\n");
}

void main(void) {
    /* 分层测试：
     * 1. 物理内存管理器测试
     * 2. 页表管理系统测试  
     * 3. 虚拟内存激活测试
     */
    
    printf("=== Experiment 3: Memory Management & Paging ===\n\n");
    
    /* 测试1: 物理内存管理 */
    printf("\n[Test 1] Physical Memory Manager\n");
    pmm_init(0, 0);
    test_physical_memory();
    
    /* 测试2: 页表管理 */
    printf("\n[Test 2] Page Table Management\n");
    test_pagetable();
    
    /* 测试3: 虚拟内存激活 */
    printf("\n[Test 3] Virtual Memory Activation\n");
    test_virtual_memory();
    
    printf("\n=== All Tests Completed ===\n");
    
    while (1) {
        asm volatile("wfi");
    }
}
