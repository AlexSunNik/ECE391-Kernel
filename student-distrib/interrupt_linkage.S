#define ASM     1
#include "interrupt_linkage.h"

/* macro for saving all registers */
.macro SAVE_ALL 
    /* save all general-use register */
    pushl   %fs
    pushl   %es
    pushl   %ds
    pushl   %ebp
    pushl   %edi
    pushl   %esi
    /* push system call arguments */
    pushl   %edx
    pushl   %ecx
    pushl   %ebx
    pushl   %eax
    
.endm

/* macro for restore all registers */
.macro RESTORE_ALL
    /* pop system call arguments */
    popl    %eax
    popl    %ebx
    popl    %ecx
    popl    %edx
    /* restore general-use registers*/
    popl    %esi
    popl    %edi
    popl    %ebp
    popl    %ds
    popl    %es
    popl    %fs

.endm

/*  RTC_int 
 *  Description: wrapper function for RTC interrupt handler
                 invoked by slave PIC, (port 8)
 *  Input: none
 *  Output: none
 *  Return value: none
 *  Side effects: evoke RTC handler
 */
.globl  RTC_int
    RTC_int:
        SAVE_ALL
        call    rtc_handler
        RESTORE_ALL
        iret

/*  Keyboard_int 
 *  Description: wrapper function for Keyboard interrupt handler
                 invoked by PIC port 1
 *  Input: none
 *  Output: none
 *  Return value: none
 *  Side effects: evoke keyboard handler
 */
.globl  Keyboard_int
    Keyboard_int:
        SAVE_ALL
        call    keyboard_handler
        RESTORE_ALL
        iret

/*  PIT_int 
 *  Description: wrapper function for PIT interrupt handler
                 invoked by PIC port 0
 *  Input: none
 *  Output: none
 *  Return value: none
 *  Side effects: evoke PIT handler
 */
.globl  PIT_int
    PIT_int:
        SAVE_ALL
        call    pit_handler
        RESTORE_ALL
        iret
