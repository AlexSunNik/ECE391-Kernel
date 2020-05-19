#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "system_call.h"
#include "paging.h"
#include "process.h"
#include "scheduling.h"

/*  
 * start_terminal0
 *   DESCRIPTION: launch the first terminal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: shell is running on the first terminal;
 *          The first terminal is being displayed;
 *          Scheduler starts from the first terminal 
 */
void start_terminal0(){
    active_term_idx = 0;
    terminals[0].active = 1;
    cur_term_id = 0;
    execute((uint8_t*)"shell");
}

/*  
 * restore_ebp
 *   DESCRIPTION: switch to another process, called by scheduler
 *   INPUTS: uint32_t ebp --- ebp of the process to switch to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: "ret" jumps to another process rather than return from restore_ebp
 *               
 */
void restore_ebp(uint32_t ebp){
    asm(
        "movl 8(%ebp), %ebp;"
        "leave;"
        "ret;"
    );
}
 
/*  
 * switch_terminal
 *   DESCRIPTION: switch to display another terminal, called when Alt + Fn are pressed
 *   INPUTS: int prev_id --- index of currently-displayed terminal
 *           int term_id --- index of to-be-displayed terminal
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video display is changed from the previous terminal to the new terminal             
 */
void switch_terminal(int prev_id, int term_id){
    /* Save current video mem into prev_id's page */
    save_video_to_backup(prev_id);
    /* Restore current video mem backup into video mem */
    restore_backup_to_video(term_id);
    /* mark the current terminal to be displayed*/
    active_term_idx = term_id;
    update_cursor(terminals[active_term_idx].cursor_x, terminals[active_term_idx].cursor_y);

}

/*  
 * switch_procss
 *   DESCRIPTION: switch process routine of our scheduler
 *   INPUTS: int prev_id --- index of previous-scheduled terminal
 *           int term_id --- index of next-scheduled terminal
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: jump to the new process rather than return from switch_process
 *              "shell" may be launched for the terminal if no program is running  
 */
void switch_process(int prev_id, int term_id){
    /* mark the term_id as the scheduled terminal index */
    cur_term_id = term_id;

    /*** SAVE EBP ***/
    /* find lastly-scheduled process's pcb */
    uint32_t prev_prog_num = terminals[prev_id].term_prog_counter;
    uint32_t prev_proc_pid = terminals[prev_id].prog_pids[prev_prog_num - 1];
    pcb_t* prev_pcb = FIND_PCB(prev_proc_pid);
    /* store previous process pcb */
    asm volatile(
        "movl %%ebp, %0 \n\
        "
        :"=rm"(prev_pcb->term_ebp)
        :
        :"cc"
    );

    /*** SWITCH PROCESS'S VIDEO MEM ***/
    /* if the process is being displayed, video_mem is the real video memory page */
    if(term_id == active_term_idx)
        video_mem = (char*)VIDEO;
    /* if the process is not being displayed, video_mem is the backup video memory page */
    else
        video_mem = (char*)((uint32_t)VIDEO + ((1 + term_id) << SHIFT_4K));

    /*** PREPARE TO SWITCH (SET TSS, SETUP PAGING, RESTORE EBP) ***/
    /* launch a shell if no program is running */
    if(terminals[term_id].term_prog_counter == 0){
        execute((uint8_t*)"shell");
    }
    else{
        /* find next-scheduled process pcb */
        uint32_t term_prog_num = terminals[term_id].term_prog_counter;
        uint32_t new_proc_pid = terminals[term_id].prog_pids[term_prog_num - 1];
        pcb_t* cur_pcb = FIND_PCB(new_proc_pid);

        // restore new process's paging scheme
        setup_paging(new_proc_pid);
        // restore new process's stack frame
        tss.esp0 = PROG0_KSTACK_BOTTOM - KERNEL_STACK_SIZE * new_proc_pid;
        tss.ss0 = KERNEL_DS;
        /* enable video page if the process already requests*/
        if(cur_pcb -> video_mem_flag){
            enable_prog_vid_page();
            change_prog_vid_mapping(term_id);
        }
        /* switch file array to the new process's file array*/
        file_array = cur_pcb -> file_array;
        /* jump to the new process */
        if((cur_pcb -> term_ebp) != NULL)
            restore_ebp(cur_pcb -> term_ebp);
    }
}

/*  
 * init_terminals
 *   DESCRIPTION: initialze 3 terminals, called in boot time
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: all 3 terminals' backup video memory pages are also initialized
 */
void init_terminals(){
    int i;
    int j;
    int term;
    for(i = 0; i < MAX_TERMINAL_NUM; i++){
        terminals[i].active = 0;
        terminals[i].buf_index = 0;
        terminals[i].cursor_x = 0;
        terminals[i].cursor_y = 0;
        terminals[i].TERMINAL_READ_FLAG = 0;
        memset(terminals[i].keyboard_buf, 0, KB_BUF_SIZE);
        memset(terminals[i].prog_pids, 0, MAX_PROG_NUM_PER_TERM);
        /* initialize 3 backup video page for 3 terminals */
        for(term = 1; term <= TERM_NUM; term++){
            for (j = 0; j < NUM_ROWS * NUM_COLS; j++) {
                if (j % NUM_COLS == 0)
                    *(uint8_t *)((VIDEO + (term << SHIFT_4K)) + (j << 1)) = ' ';
                else
                    *(uint8_t *)((VIDEO + (term << SHIFT_4K)) + (j << 1)) = 0;
                *(uint8_t *)((VIDEO + (term << SHIFT_4K)) + (j << 1) + 1) = ATTRIB;
            }
        }
        /* set running program number of current terminal to 0 */
        terminals[i].term_prog_counter = 0;
    }
    /* schedule terminal 0 at first */
    cur_term_id = 0;
}

/*  
 * pit_init
 *   DESCRIPTION: initialize pit hardware, called in boot time
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pit ticks every 10ms(frequency = 100Hz)
 */
void pit_init() {
    outb(PIT_FREQ_SET, PIT_CTR_PORT);
    outb((uint8_t)(PIT_FREQ & LOWER_8),PIT_CHA0_PORT);
    outb((uint8_t)((PIT_FREQ & UPPER_8) >> SHIFT_8), PIT_CHA0_PORT);
}

/*  
 * pit_handler
 *   DESCRIPTION: pit interrupt handler, called when pit interrupts.
 *              call switch_process to achieve scheduling
 *   INPUTS:  none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void pit_handler() {
    if (!prog_counter) {
        send_eoi(0);
        return;
    }
    /* round-robin scheduling */
    prev_term_id = cur_term_id;
    cur_term_id = (cur_term_id + 1) % MAX_TERMINAL_NUM;

    send_eoi(0);
    /* switch to the new process */
    switch_process(prev_term_id, cur_term_id);
    return;
}
