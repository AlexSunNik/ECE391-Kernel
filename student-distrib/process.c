#include "process.h"
#include "keyboard.h"
#include "lib.h"
#include "system_call.h"
#include "scheduling.h"

#define FILE_ARR_LENGTH 8
#define STD_RANGE       2
#define MAX_PID         5

/* 
 * init_prog
 *   DESCRIPTION: initialize program counter and exception flag to 0
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_prog() {
    prog_counter = 0;
    program_exception_flag = 0;
    int32_t count = 0;
    int32_t base_pcb = PROG0_KSTACK_BOTTOM - KERNEL_STACK_SIZE;
    pcb_t* cur_pcb = NULL;
    /* initalize all 6 pcbs*/
    for(count = 0; count <= MAX_PID; count++){
        cur_pcb = (pcb_t*)(base_pcb - count * KERNEL_STACK_SIZE);
        cur_pcb -> status = 0;
        cur_pcb -> video_mem_flag = 0;
    }
}

/* 
 * create_pcb
 *   DESCRIPTION: create a program control block for a program
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if not
 *   SIDE EFFECTS: none
 */
int32_t create_pcb() {
    if (prog_counter > MAX_PID) 
        return -1;

    /* pid is the pid of next active pcb */
    /* pcb is the pointer to next active pcb */
    int32_t pid = -1;
    pcb_t* pcb = get_active_pcb(&pid);
    if (pcb == NULL)
        return -1;
    pcb->pid = pid;
    /* set up file array: STDIN, STDOUT */
    int i;
    for (i = 0; i < FILE_ARR_LENGTH; i++) {
        if (i < STD_RANGE) {
            pcb->file_array[i].flags = 1;
            pcb->file_array[i].op_ptr = &terminal_op_table;
        } else
            pcb->file_array[i].flags = 0;
        pcb->file_array[i].file_position = 0;
    }

    /* link to parent process */
    if (terminals[cur_term_id].term_prog_counter == 0)
        pcb->parent_pcb_pointer = NULL;
    else {
        uint32_t parent_pid = terminals[cur_term_id].prog_pids[terminals[cur_term_id].term_prog_counter - 1];
        pcb->parent_pcb_pointer = (pcb_t*) (PROG0_KSTACK_BOTTOM - (parent_pid + 1) * KERNEL_STACK_SIZE);
    }
    pcb->status = 1;
    pcb->term_idx = cur_term_id; 
    prog_counter++;
    /* switch file_array to point to the new process's file array */
    file_array = pcb->file_array;

    /* change terminal attributes*/
    int term_prog_idx = terminals[cur_term_id].term_prog_counter;
    terminals[cur_term_id].prog_pids[term_prog_idx] = pcb -> pid;
    terminals[cur_term_id].term_prog_counter ++;

    return pid;
}

/* 
 * get_active_pcb
 *   DESCRIPTION: get the current program's pcb
 *   INPUTS: int *pid -- pid passed by pointer to be modified
 *   OUTPUTS: none
 *   RETURN VALUE: address of current pcb
 *   SIDE EFFECTS: *pid is filled with the pid of found pcb
 */
pcb_t* get_active_pcb(int32_t *pid){
    if(prog_counter > MAX_PID) return 0;
    int32_t base_pcb = PROG0_KSTACK_BOTTOM - KERNEL_STACK_SIZE;
    pcb_t* cur_pcb = NULL;
    int32_t count = 0;
    /* iterate over 6 pcbs until find an empty pcb */
    for(count = 0; count <= MAX_PID; count++){
        cur_pcb = (pcb_t*)(base_pcb - count * KERNEL_STACK_SIZE);
        if(cur_pcb -> status == 0){
            *pid = count;
            return cur_pcb;
        }
    }
    return 0;
}

/* 
 * context_switch
 *   DESCRIPTION: prepare and switch to the user level program
 *   INPUTS: eip -- instruction pointer to jump
 *           cs -- code segment
 *           esp -- stack pointer to jump
 *           ss -- stack segment
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void context_switch(uint32_t eip, uint32_t cs, uint32_t esp, uint32_t ss){  
    /*
        old ebp <--- ebp
        ret addr
        eip
        cs
        esp
        ss
    */

    //Prepare the stack for the "iret instruction"
    //Push SS, ESP, EFLAGS, CS, EIP
    uint32_t term_prog_num = terminals[cur_term_id].term_prog_counter;
    uint32_t new_proc_pid = terminals[cur_term_id].prog_pids[term_prog_num - 1];
    tss.esp0 = PROG0_KSTACK_BOTTOM - KERNEL_STACK_SIZE * new_proc_pid;
    tss.ss0 = KERNEL_DS;

   /* orl $0x200 to set IF flag */
   asm(
        "xorl %eax, %eax;"
        "movl 20(%ebp), %eax;"
        "pushl %eax;"
        "movw %ax, %ds;"
        "movl 16(%ebp), %eax;"
        "pushl %eax;"
        "pushfl;"
        "popl %eax;"
        "orl $0x200, %eax;"
        "pushl %eax;"
        "movl 12(%ebp), %eax;"
        "pushl %eax;"
        "movl 8(%ebp), %eax;"
        "pushl %eax;"
        "iret;"
   );
}
