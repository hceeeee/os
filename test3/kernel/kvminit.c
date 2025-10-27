/**
 * kernel/kvminit.c - 内核虚拟内存初始化
 * 
 * 实现原理：
 * 1. kvminit(): 创建内核页表并映射必需的物理内存区域
 * 2. kvminithart(): 激活内核页表（设置SATP寄存器）
 * 
 * 核心概念：
 * - 恒等映射：VA = PA，简化页表设置
 * - 权限控制：文本段(RX)，数据段(RW)，设备(RW)
 * - SATP寄存器：控制MMU模式和页表基址
 */
#include "kvminit.h"
#include "pmm.h"
#include "printf.h"
#include <stdint.h>

/* QEMU virt 机器常量 */
#ifndef KERNBASE
#define KERNBASE 0x80000000UL  /* 内核基址 */
#endif
#ifndef MEMSIZE
#define MEMSIZE (128UL * 1024 * 1024)  /* 内存大小 */
#endif
#ifndef UART0
#define UART0 0x10000000UL  /* UART设备地址 */
#endif

/* 全局内核页表指针 */
pagetable_t kernel_pagetable = 0;

/**
 * w_satp - 写SATP寄存器（启用/禁用MMU）
 * 
 * SATP寄存器格式：
 * - [63:60]: MODE（8 = Sv39）
 * - [59:44]: ASID（地址空间ID）
 * - [43:0]: PPN（页表物理页号）
 */
static inline void w_satp(uint64_t satp) {
    asm volatile("csrw satp, %0" :: "r"(satp));
}

/**
 * sfence_vma - 刷新TLB（Translation Lookaside Buffer）
 * 
 * 作用：清除页表缓存，使新的页表设置立即生效
 * 必须在设置SATP后调用
 */
static inline void sfence_vma(void) {
    asm volatile("sfence.vma" ::: "memory");
}

/**
 * map_region - 映射连续的物理内存区域
 * 
 * 功能：将 [va, va+size) 映射到 [pa, pa+size)
 * 
 * 特性：
 * - 自动页对齐：向下对齐起始地址，向上对齐结束地址
 * - 已映射跳过：如果页面已映射，跳过而不是失败
 * - 逐页映射：以4KB为单位建立映射
 */
int map_region(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    if (!pt) return -1;

    /* 页对齐：起始地址向下对齐，结束地址向上对齐 */
    uint64_t va_start = PAGE_ROUND_DOWN(va);
    uint64_t pa_start = PAGE_ROUND_DOWN(pa);
    uint64_t va_end   = PAGE_ROUND_UP(va + size);

    /* 逐页建立映射 */
    for (uint64_t a = va_start, p = pa_start; a < va_end; a += PAGE_SIZE, p += PAGE_SIZE) {
        /* 检查是否已映射，避免重复映射 */
        pte_t *existing = walk_lookup(pt, a);
        if (existing && (*existing & PTE_V)) {
            /* 已存在映射，跳过 */
            continue;
        }
        /* 建立新映射 */
        if (map_page(pt, a, p, perm) != 0) {
            printf("map_region: map_page failed va=%p pa=%p\n", (void*)a, (void*)p);
            return -1;
        }
    }
    return 0;
}


/**
 * kvminit - 创建并初始化内核页表
 * 
 * 功能：创建内核页表，并映射所有必需的内存区域
 * 
 * 映射策略（恒等映射，VA=PA）：
 * 1. 内核代码段：_text到_etext（R|X：可读可执行）
 * 2. 只读数据段：_rodata到_erodata（R：只读）
 * 3. 数据+BSS段：_data到_end（R|W：可读可写）
 * 4. 额外物理内存：内核结束后10页（R|W：可读可写）
 * 5. 设备：UART（R|W：可读可写）
 * 
 * 为什么使用恒等映射？
 * - 简化页表设置
 * - 避免启用分页后立即访问失败
 * - 内核代码期望直接物理地址访问
 */
void kvminit(void) {
    /* 创建根页表 */
    kernel_pagetable = create_pagetable();
    if (!kernel_pagetable) {
        printf("kvminit: create_pagetable failed\n");
        return;
    }

    /* 获取链接器定义的内核布局符号 */
    extern char _text[], _etext[], _rodata[], _erodata[];
    extern char _data[], _edata[], _end[];

    uint64_t text = (uint64_t)_text;
    uint64_t etext = (uint64_t)_etext;
    uint64_t rodata = (uint64_t)_rodata;
    uint64_t erodata = (uint64_t)_erodata;
    uint64_t data = (uint64_t)_data;
    uint64_t edata = (uint64_t)_edata;
    uint64_t end = (uint64_t)_end;

    /* 1. 映射内核代码段（可读可执行，不能写）*/
    map_region(kernel_pagetable, text, text, (uint64_t)(etext - text), PTE_R | PTE_X);

    /* 2. 映射只读数据段（只读）*/
    if (erodata > rodata)
        map_region(kernel_pagetable, rodata, rodata, (uint64_t)(erodata - rodata), PTE_R);

    /* 3. 映射数据+BSS段（可读可写）*/
    map_region(kernel_pagetable, data, data, (uint64_t)(end - data), PTE_R | PTE_W);

    /* 4. 映射额外的物理内存（为内核预留）*/
    map_region(kernel_pagetable, end, end, PAGE_SIZE * 10, PTE_R | PTE_W);

    /* 5. 映射设备区域（UART等）*/
    map_region(kernel_pagetable, UART0, UART0, PAGE_SIZE, PTE_R | PTE_W);

    printf("kvminit: kernel_pagetable created and regions mapped\n");
}

/**
 * kvminithart - 激活内核页表（当前核心）
 * 
 * 功能：设置SATP寄存器，启用MMU分页
 * 
 * 工作流程：
 * 1. 检查内核页表是否存在
 * 2. 构建SATP值（MODE=8 + PPN）
 * 3. 写入SATP寄存器（启用MMU）
 * 4. 刷新TLB（使新设置生效）
 * 
 * SATP寄存器详细格式：
 * - MODE[63:60]=8：Sv39分页模式
 * - ASID[59:44]=0：地址空间ID（单内核，不需要）
 * - PPN[43:0]：页表基址的物理页号
 * 
 * 注意事项：
 * - 必须在所有内存映射完成后调用
 * - sfence.vma是必需的，确保TLB刷新
 * - 调用后CPU自动进行地址翻译
 */
void kvminithart(void) {
    if (!kernel_pagetable) {
        printf("kvminithart: kernel_pagetable is NULL\n");
        return;
    }
    
    /* 构建SATP值：MODE=8 (Sv39) + 页表物理页号 */
    uint64_t satp = MAKE_SATP(kernel_pagetable);
    
    /* 写SATP寄存器：启用MMU */
    w_satp(satp);
    
    /* 刷新TLB：使新的页表设置生效 */
    sfence_vma();
    
    printf("kvminithart: satp set %p\n", (void*)satp);
}
