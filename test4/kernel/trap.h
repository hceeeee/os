// kernel/trap.h
#pragma once
#include <stdint.h>

typedef void (*interrupt_handler_t)(void);

void trap_init(void);
void register_interrupt(int irq, interrupt_handler_t h);
void enable_interrupt(int irq);
void disable_interrupt(int irq);

// 提供给测试与时钟
uint64_t get_time(void);
void timer_interrupt(void);

// 供汇编入口调用
void kerneltrap(void);
void usertrap(void);   // 占位
int  devintr(void);    // 返回是否为设备中断
