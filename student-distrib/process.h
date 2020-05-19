#ifndef _PROCESS_H
#define _PROCESS_H

#include "types.h"
#include "x86_desc.h"
#define STDIN 0
#define STDOUT 1

#define USER_STACK_BOTTOM 0xC00000
#define PROG0_KSTACK_BOTTOM 0x800000
#define PROG1_KSTACK_BOTTOM 0x7FE000   
#define KERNEL_STACK_SIZE    0x2000
#define FILE_NUM 8 
#define PARAMS_LEN 128

/* operation table for files */
typedef struct file_op_table{
    int32_t (*open) (int32_t);
    int32_t (*read) (int32_t, void*, int32_t);
    int32_t (*write) (int32_t, const void*, int32_t);
    int32_t (*close) (int32_t);
}file_op_table_t;

/* file abstract entry, what stored in the file array */
typedef struct file_abs_entry
{
    /* points to the operation table of this file */
    file_op_table_t* op_ptr;
    uint32_t inode_idx;
    /* track which char to read next for common files 
       or store frequency ration for rtc file*/
    uint32_t file_position;
    /* alive or dead */
    uint32_t flags;
} file_abs_entry_t;

/* process control block */
typedef struct pcb{
    /* store ebp for execute and halt */
    int32_t ebp;
    /* file_array of process-access files */
    file_abs_entry_t file_array[FILE_NUM];
    /* running or dead */
    int32_t status;
    /* process identifier */
    int32_t pid;
    /* point to the parent process's pcb */
    struct pcb* parent_pcb_pointer;
    /* store user program argument */
    uint8_t params[PARAMS_LEN]; 
    /* which terminal is this process running */
    uint32_t term_idx;
    /* store ebp for scheduling */
    int32_t term_ebp;
    /* acquire user video memory? */
    int32_t video_mem_flag;
} pcb_t;

/* Where should we place the file descriptor array (for each task)? */
/* Each task/process has its own pcb, which has a file descriptor array inside */
/* each time, when we 1. start to execute, 2. switch back to a process,
   we do file_array = pcb.file_array */
file_abs_entry_t* file_array;
/* a global program counter: how many processes are running in total */
int32_t prog_counter;

/* a kernel file array, 8 is the number of files */
file_abs_entry_t kernel_file_array[8];

/* initialize program counter to 0 */
void init_prog();

/* initialize a program's pcb */
int32_t create_pcb();

/* get the current pcb */
pcb_t* get_active_pcb(int32_t *pcb);

/* prepare and switch a user level program */
void context_switch(uint32_t eip, uint32_t cs, uint32_t esp, uint32_t ss);

/* pcb & kernel stack structure
    ------------------------------
        | process control block |
8kb     |-----------------------|
        |                       |
        |     growing up        |
        |-----------------------|
        |    kernel stack       |
        |                       |
----------------------------------
*/

#endif

