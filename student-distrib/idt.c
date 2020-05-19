#include "lib.h"
#include "x86_desc.h"

#include "idt.h"

#include "exception_linkage.h"
#include "interrupt_linkage.h"
#include "system_call_linkage.h"
/* 
 * idt_init
 *   DESCRIPTION: initialize the interrupt descriptor table
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set the interrupt descriptor table entries
 *                 for exceptions, interrupts, and system calls
 */
void idt_init(void) {
    /* set exceptions entries, from 0x00 to 0x1F*/
    set_intr_gate(0x00, Divide_by_zero_Error);
    set_intr_gate(0x01, Debug);
    set_intr_gate(0x02, Non_maskable_interrupt);
    set_system_intr_gate(0x03, Breakpoint);
    set_system_gate(0x04, Overflow);
    set_system_gate(0x05, Bound_Range_Exceeded);
    set_intr_gate(0x06, Invalid_Opcode);
    set_intr_gate(0x07, Device_Not_Available);
    set_intr_gate(0x08, Double_Fault);
    set_intr_gate(0x09, Coprocessor_Segment_Overrun);
    set_intr_gate(0x0A, Invalid_TSS);
    set_intr_gate(0x0B, Segment_Not_Present);
    set_intr_gate(0x0C, Stack_Segment_Fault);
    set_intr_gate(0x0D, General_Protection_Fault);
    set_intr_gate(0x0E, Page_Fault);
    set_intr_gate(0x0F, Reserved1);
    set_intr_gate(0x10, x87_Floating_Point_Exception);
    set_intr_gate(0x11, Alignment_Check);
    set_intr_gate(0x12, Machine_Check);
    set_intr_gate(0x13, SIMD_Floating_Point_Exception);
    set_intr_gate(0x14, Virtualization_Exception);
    set_intr_gate(0x15, Reserved2);
    set_intr_gate(0x16, Reserved3);
    set_intr_gate(0x17, Reserved4);
    set_intr_gate(0x18, Reserved5);
    set_intr_gate(0x19, Reserved6);
    set_intr_gate(0x1A, Reserved7);
    set_intr_gate(0x1B, Reserved8);
    set_intr_gate(0x1C, Reserved9);
    set_intr_gate(0x1D, ReservedA);
    set_intr_gate(0x1E, Security_Exception);
    set_intr_gate(0x1F, ReservedB);

    /* set interrupts entries */
    set_intr_gate(0x20, PIT_int);           // 0x20 is the entry for PIT
    set_intr_gate(0x21, Keyboard_int);      // Ox21 is the entry for keyboard interrupt
    set_intr_gate(0x28, RTC_int);           // Ox28 is the entry for rtc interrupt

    /* system call */
    set_system_intr_gate(0x80, System_Call);     // 0x80 is reserved for system calls
   
}

/* 
 * set_intr_gate
 *   DESCRIPTION: initialize the entry as an interrupt gate
 *   INPUTS: idx - index of the idt
 *           addr - function ptr for the interrupt handler
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set the properties of an idt entry to be
 *                 an interrupt gate with DPL = 0
 */
void set_intr_gate(int idx, void* addr) {
    idt[idx].seg_selector = KERNEL_CS;
    idt[idx].reserved4 = 0;
    idt[idx].reserved3 = 0;
    idt[idx].reserved2 = 1;
    idt[idx].reserved1 = 1;
    idt[idx].size = 1;
    idt[idx].reserved0 = 0;
    idt[idx].dpl = KERNEL_LEVEL;
    idt[idx].present = 1;
    SET_IDT_ENTRY(idt[idx], addr);
}

/* 
 * set_system_gate
 *   DESCRIPTION: initialize the entry as a system gate
 *   INPUTS: idx - index of the idt
 *           addr - function ptr for the interrupt handler
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set the properties of an idt entry to be
 *                 a trap gate with DPL = 3
 */
void set_system_gate(int idx, void* addr) {
    idt[idx].seg_selector = KERNEL_CS;
    idt[idx].reserved4 = 0;
    idt[idx].reserved3 = 1;
    idt[idx].reserved2 = 1;
    idt[idx].reserved1 = 1;
    idt[idx].size = 1;
    idt[idx].reserved0 = 0;
    idt[idx].dpl = USER_LEVEL;
    idt[idx].present = 1;
    SET_IDT_ENTRY(idt[idx], addr);
}

/* 
 * set_system_intr_gate
 *   DESCRIPTION: initialize the entry as a system interrupt gate
 *   INPUTS: idx - index of the idt
 *           addr - function ptr for the interrupt handler
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set the properties of an idt entry to be
 *                 an interrupt gate with DPL = 3
 */
void set_system_intr_gate(int idx, void* addr) {
    idt[idx].seg_selector = KERNEL_CS;
    idt[idx].reserved4 = 0;
    idt[idx].reserved3 = 0;
    idt[idx].reserved2 = 1;
    idt[idx].reserved1 = 1;
    idt[idx].size = 1;
    idt[idx].reserved0 = 0;
    idt[idx].dpl = USER_LEVEL;
    idt[idx].present = 1;
    SET_IDT_ENTRY(idt[idx], addr);
}
