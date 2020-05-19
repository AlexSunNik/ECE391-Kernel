#define ASM     1
#include "system_call_linkage.h"

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
    
.endm

/* macro for restore all registers */
.macro RESTORE_ALL
    /* pop system call arguments */
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

/*  System_Call 
 *  Description: wrapper function for system call handler 
 *  called by INT 0x80
 *  Input: system call id: passed by eax
 *  Output: none
 *  Return value: return value of system call
 *  Side effects: evoke system call handler
 */
.globl System_Call
System_Call:
    SAVE_ALL
    call    dispatch_syscall
    RESTORE_ALL
    iret

/* dispath_syscall
 * Description: system call dispatcher, jump to specified system
 *              call according to system call id stored in %eax
 * Input: system call id: passed by eax
 * Output: none
 * Return value: return -1 if system call id is invalid
 * Side effects: evoke system call handler
 */
dispatch_syscall:
    /* system call # check */
    cmpl    $1, %eax
    jl      invalid_arg
    cmpl    $10, %eax
    jg      invalid_arg

    jmp     *function_table(, %eax, 4)

invalid_arg:
    movl    $-1, %eax
    ret

/* jumptable for all system calls */
function_table:
.long   0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
