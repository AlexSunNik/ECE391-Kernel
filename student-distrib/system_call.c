#include "lib.h"
#include "system_call.h"
#include "process.h"
#include "file_system.h"
#include "rtc.h"
#include "keyboard.h"
#include "paging.h"
#include "scheduling.h"

#define PROGRAM_PAGE_VIRTUAL_ADDR  0x08000000
#define OFFSET   0x400000
#define MAX_PROGRAM_COUNT   6

#define PROG_COUNTER_OFFSET 2
#define EIP_OFFSET          24
#define NAME_LENGTH         32
#define PARAM_LENGTH        128

#define PROG_LIMIT_REACHED  1
#define EXP_ERROR           256

#define MB_128 0x08000000
#define MB_132 0x08400000

/*  
 * halt
 *   DESCRIPTION: syscall that terminate a process
 *   INPUTS: status -- return value to parent program
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t halt(uint8_t status) {
    /* critical section begins */
    pcb_t* parent_pcb_ptr;
    pcb_t* child_pcb_ptr;

    /* get parent and child pcb */
    uint32_t term_prog_num = terminals[cur_term_id].term_prog_counter;
    uint32_t child_pcb_pid = terminals[cur_term_id].prog_pids[term_prog_num - 1];
    child_pcb_ptr = FIND_PCB(child_pcb_pid);
    parent_pcb_ptr = child_pcb_ptr -> parent_pcb_pointer;
    /* set current process to dead */
    child_pcb_ptr->status = 0;
    child_pcb_ptr->parent_pcb_pointer = NULL;

    /* close all files open by program */
    int fd;   
    for (fd = 0; fd < FILE_LIMIT; fd++) {
        if (file_array[fd].flags)
            close(fd);
    }

    /* re-launch the shell if halt the shell */
    if(parent_pcb_ptr == NULL){
        prog_counter--;
        terminals[cur_term_id].term_prog_counter = 0;
        clear();
        _execute((uint8_t *)"shell");
    }
    /* disable the video memory page that the program was using */
    disable_prog_vid_page();
    child_pcb_ptr->video_mem_flag = 0;

    /* switch to parent's file array */
    file_array = parent_pcb_ptr->file_array;
    prog_counter--;
    terminals[cur_term_id].term_prog_counter--;

    /* switch to parent process program page */
    setup_paging(parent_pcb_ptr -> pid);

    // handle program terminates by exception
    if(program_exception_flag) {
        program_exception_flag = 0;
        jump_to_exec_ret(child_pcb_ptr->ebp, EXP_ERROR);
    }
    /* jump to execute's return */
    else
        jump_to_exec_ret(child_pcb_ptr->ebp, (uint32_t)status);

    // never reach here
    return 0;
}

/*  
 * jump_to_exec_ret
 *   DESCRIPTION: Jump to return of execute system call
 *   INPUTS: ebp -- ebp of the current program execution
 *        status -- return value of the current program
 *   OUTPUTS: none
 *   RETURN VALUE: 0 (not used)
 *   SIDE EFFECTS: jump to execution return
 */
int32_t jump_to_exec_ret(int32_t ebp, uint32_t status) {
    uint32_t term_prog_num = terminals[cur_term_id].term_prog_counter;
    uint32_t new_proc_pid = terminals[cur_term_id].prog_pids[term_prog_num - 1];
    tss.esp0 = PROG0_KSTACK_BOTTOM - KERNEL_STACK_SIZE * new_proc_pid;
    tss.ss0 = KERNEL_DS;
    // 12: 2nd argument, 8: 1st argument
    asm(
        "movl 12(%ebp), %eax;"
        "movl 8(%ebp), %ebp;"
        "leave;"
        "ret;"
    );
    // will never reach here
    return 0;
}

/*  
 * execute
 *   DESCRIPTION: syscall that attempt to load and execute a new program
 *   INPUTS: command -- space-separated sequence of words: [filename] [other arguments]
 *   OUTPUTS: none
 *   RETURN VALUE: 256 if the program dies by an exception
 *                 0 to 255 if the program executes a halt instruction, in which case the value is halt's return value
 *                 -1 if the command cannot be executed
 *   SIDE EFFECTS: none
 */
int32_t execute(const uint8_t* command){
    int32_t ret_val = _execute(command);
    return ret_val;
}

/* Helper function to execute, functionality described in execute's function header */
int32_t _execute(const uint8_t* command) {
    /* check command validity */
    if (!command)
        return -1;

    uint8_t filename[NAME_LENGTH + 1];
    uint8_t params[PARAM_LENGTH];
    dentry_t exec_dentry;
    int32_t pid;
    pcb_t* cur_pcb;

    /* check if exceed max program number limit */
    if (terminals[cur_term_id].term_prog_counter >= MAX_PROG_NUM_PER_TERM) {
        printf("FAIL: cannot execute more than 4 tasks in a terminal!\n");
        return PROG_LIMIT_REACHED;
    }
    
    if(prog_counter >= MAX_PROGRAM_COUNT) {
        printf("FAIL: cannot execute more than 6 tasks!\n");
        return PROG_LIMIT_REACHED;
    }

    /* parse command */
    parse_command(command, filename, params);

    /* read dentry */
    if (read_dentry_by_name(filename, &exec_dentry) == -1)
        return -1;

    /* check validity */
    if (check_validity(&exec_dentry) == -1) 
        return -1;

    /* create pcb */
    pid = create_pcb();
    /* cannot allocate a pcb for a new task */
    if (pid == -1){
        return -1;
    }  
    cur_pcb = FIND_PCB(pid);

    /* setup paging */
    setup_paging(pid);

    /* load the program */
    program_loader(&exec_dentry);

    /* store the program's argument, the maximum # of chars for parameters is 128 */
    memcpy(cur_pcb->params, params, 128);

    /* get program's eip value */
    int32_t eip = *(int32_t*)(PROGRAM_DIRECTORY_VIRTUAL_ADDR + EIP_OFFSET);

    /* store ebp for context switch from halt */
    asm volatile(
        "movl %%ebp, %0"
        :"=r"(cur_pcb -> ebp)
    );

    /* context_switch */
    /* beginning esp -4 to prevent page fault when dereferencing at 0x84000000 */
    context_switch(eip, USER_CS, PROGRAM_PAGE_VIRTUAL_ADDR + OFFSET - 4, USER_DS);
    // will never reach
    return 0;
}

/*  
 * read
 *   DESCRIPTION: syscall that read data from the keyboard, a file, device (RTC), or directory
 *   INPUTS: fd -- the file descriptor in which we want to read
 *           buf -- buffer that we want to write into
 *           nbytes -- number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes successfully read
 *                 0 if we're at or after the end
 *                 -1 if the input isn't valid
 *   SIDE EFFECTS: none
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    /* check validity */
    /* file array valid range 0-7 */
    if (fd < 0 || fd >= FILE_LIMIT || buf == 0 || nbytes < 0 || !file_array){
        return -1;
    }
    if (file_array[fd].flags == 0){
        return -1;
    }
    /* invoke file specific read */
    return ((file_array[fd].op_ptr)->read)(fd, buf, nbytes);
}

/*  
 * write
 *   DESCRIPTION: syscall that write data to the terminal or to a device (RTC)
 *   INPUTS: fd -- the file descriptor in which we want to read
 *           buf -- buffer that we want to write
 *           nbytes -- number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes successfully write to the terminal
 *                 0 if RTC write is successful
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    /* check validity */
    /* file array valid range 0-7 */
    if (fd < 0 || fd >= FILE_LIMIT || buf == 0 || nbytes < 0|| (!file_array) || (!(file_array[fd].flags))){
        return -1;
    }
    
    /* invoke file specific write */
    return ((file_array[fd].op_ptr)->write)(fd, buf, nbytes);
}

/*  
 * open
 *   DESCRIPTION: syscall that provide access to the file system
 *                1. set up a file descriptor, 2. invoke file specific open function
 *   INPUTS: filename -- the file that we want to open
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if named file doesn't exist or no descrriptors are free
 *   SIDE EFFECTS: none
 */
int32_t open(const uint8_t* filename) {
    /* find the first file descriptor that is not in use */
    int32_t fd;
    /* file array valid range 0-7 */
    for (fd = 2; fd < FILE_LIMIT && file_array[fd].flags; fd++);
    /* if reach 8, no room for new file */
    if (fd == FILE_LIMIT)
        return -1;

    /* find the dentry corresponding to the filename */
    dentry_t dentry;
    if (read_dentry_by_name(filename, &dentry) == -1)
        return -1;
    uint32_t file_type = dentry.file_type;

    /* note: file_type: 0 - rtc, 1 - dir, 2 - normal */
    file_array[fd].inode_idx = dentry.inode_idx;
    file_array[fd].flags = 1;
   
    /* set up the operation table */
    switch (file_type) {
        case 0: // - rtc
            file_array[fd].op_ptr = &rtc_op_table;
            break;
        case 1: // - dir
            file_array[fd].op_ptr = &dirt_op_table;
            break;
        case 2: // - regular
            file_array[fd].op_ptr = &regular_file_op_table;
            break;
        default:
            return -1;
    }

    /* invoke file specific system call */
    ((file_array[fd].op_ptr)->open)(fd);

    return fd;
}

/*  
 * close
 *   DESCRIPTION: syscall that close the specified file descriptor
 *   INPUTS: fd -- the file descriptor in which we want to close
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if fd is not valid or the file was already closed
 *   SIDE EFFECTS: none
 */
int32_t close(int32_t fd) {
    /* check validity */
    /* file array valid range 0-7 */
    /* but stdin and stdout should not be closed */
    if ((fd < 2) || (fd >= FILE_LIMIT) || (!file_array) || (!(file_array[fd].flags)))
        return -1;

    /* close the file */
    file_array[fd].flags = 0;
    return ((file_array[fd].op_ptr)->close)(fd);
}

/*  
 * getargs
 *   DESCRIPTION: syscall that read the program's command line arguments into a user-level buffer
 *   INPUTS: buf -- a user-level buffer
 *           nbytes -- number of bytes that we want to read
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if there are no arguments or the arguments + terminal NULL do not fit into the buffer
 *   SIDE EFFECTS: none
 */
int32_t getargs(uint8_t* buf, int32_t nbytes) {
    /* find current process's pcb */
    uint32_t term_prog_num = terminals[cur_term_id].term_prog_counter;
    uint32_t cur_pid = terminals[cur_term_id].prog_pids[term_prog_num - 1];
    pcb_t* cur_pcb = FIND_PCB(cur_pid);

    /* check validity */
    if (nbytes < strlen((int8_t*)cur_pcb->params) + 1 || *cur_pcb->params == '\0')
        return -1;

    /* copy into current process's pcb's buffer */
    strcpy((int8_t*)buf, (int8_t*)cur_pcb->params);
    return 0;
}

/*  
 * vidmap
 *   DESCRIPTION: syscall that map the text-mode video memory into user space at a pre-set virtual address
 *   INPUTS: screen_start -- pointing to the pointer that we want to modify
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if the location is invalid (not in user-space)
 *   SIDE EFFECTS: none
 */
int32_t vidmap(uint8_t** screen_start) {
    /* check for range */
    if ((uint32_t) screen_start < MB_128 || (uint32_t) screen_start >= MB_132)
        return -1;
    
    /* enable user-access video memory */
    enable_prog_vid_page();
    /* store virtual address in the ptr passed by user */
    *screen_start = (uint8_t *)MB_132;

    /* find current process pcb and set video_mem_flag*/
    uint32_t term_prog_num = terminals[cur_term_id].term_prog_counter;
    uint32_t cur_pid = terminals[cur_term_id].prog_pids[term_prog_num - 1];
    pcb_t* cur_pcb = FIND_PCB(cur_pid);
    cur_pcb -> video_mem_flag = 1;

    return 0;
}


/**
 * ______________________________________________________
 * reserved for extra credit
*/

/* set_handler(int32_t signum, void* handler_address) */
int32_t set_handler(int32_t signum, void* handler_address) {
    signum = signum;
    handler_address = handler_address;
    return -1;
}

/* sigreturn(void) */
int32_t sigreturn(void) {
    return -1;
}
