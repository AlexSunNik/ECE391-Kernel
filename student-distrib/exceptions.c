#include "lib.h"
#include "exceptions.h"
#include "system_call.h"
#include "process.h"

#define PAGE_FAULT  14

/* 0x20: the total number of exceptions */
char* exp_msg[EXP_NUM] = {
    "Divide-by-zero Error",             /* 0x00 */
    "Debug",                            /* 0x01 */
    "Non-maskable interrupt",           /* 0x02 */
    "Breakpoint",                       /* 0x03 */
    "Overflow",                         /* 0x04 */
    "Bound Range Exceeded",             /* 0x05 */
    "Invalid Opcode",                   /* 0x06 */
    "Device Not Available",             /* 0x07 */
    "Double Fault",                     /* 0x08 */
    "Coprocessor Segment Overrun",      /* 0x09 */
    "Invalid TSS",                      /* 0x0A */
    "Segment Not Present",              /* 0x0B */
    "Stack Segment Fault",              /* 0x0C */
    "General Protection Fault",         /* 0x0D */
    "Page Fault",                       /* 0x0E */
    "Assertion Failure",                /* 0x0F */
    "x87 Floating-Point Exception",     /* 0x10 */
    "Alignment Check",                  /* 0x11 */
    "Machine Check",                    /* 0x12 */
    "SIMD Floating-Point Exception",    /* 0x13 */
    "Virtualization Exception",         /* 0x14 */
    "Reserved",                         /* 0x15 */
    "Reserved",                         /* 0x16 */
    "Reserved",                         /* 0x17 */
    "Reserved",                         /* 0x18 */
    "Reserved",                         /* 0x19 */
    "Reserved",                         /* 0x1A */
    "Reserved",                         /* 0x1B */
    "Reserved",                         /* 0x1C */
    "Reserved",                         /* 0x1D */
    "Security Exception",               /* 0x1E */
    "Reserved"                          /* 0x1F */
};

/*  
 * exception_handler
 *   DESCRIPTION: handle an exception based on its id
 *   INPUTS: exp_id - a unique id associated with an exception
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. print out its name, 2. halt the program
 */
void exception_handler(int exp_id) {
    /* disable interrupt */
    clear();
    printf("An exception has occurred:\n");
    /* print the evoked exception message*/
    printf("%s\n", exp_msg[exp_id]);

    if (exp_id == PAGE_FAULT) {
        uint32_t return_val;
        asm volatile
        ("movl %%cr2, %0"
            :"=r"(return_val)
            :
        );
        printf("Page fault occur at %#x\n", return_val);
    }
    
    if(exp_id == 0x0D){
        printf("%x\n", tss.ss0);
        printf("%x\n", tss.esp0);
    }

    // Halt the current process and return
    // exception status
    if(prog_counter) {
        program_exception_flag = 1;
        halt(0);
    }
    /* according to piazza, a while(1) loop is enough*/
    while(1);
    /*
    asm (
        "hlt"
    );
    */
}
