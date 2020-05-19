#ifndef _IDT_H
#define _IDT_H

#define KERNEL_LEVEL    0
#define USER_LEVEL      3


/* initialize the interrupt descriptor table */
extern void idt_init(void);

/* set an interrupt gate with DPL = 0 */
void set_intr_gate(int idx, void* addr);
/* set a trap gate with DPL = 3 */
void set_system_gate(int idx, void* addr);
/* set an interrupt gate with DPL = 3 */
void set_system_intr_gate(int idx, void* addr);
/* This is an interrupt gate with DPL = 0 */
void set_trap_gate(int idx, void* addr);
/* This is a task gate */
void set_task_gate(int idx, void* addr, uint16_t gdt);

#endif
