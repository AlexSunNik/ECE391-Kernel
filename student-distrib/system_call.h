#ifndef _SYS_H
#define _SYS_H

#define FIND_PCB(pid) (pcb_t*) (PROG0_KSTACK_BOTTOM - (pid + 1) * KERNEL_STACK_SIZE);

int32_t program_exception_flag;

/* terminate a process */
extern int32_t halt(uint8_t status);

/* attempt to load and execute a new program */
extern int32_t execute(const uint8_t* command);
int32_t _execute(const uint8_t* command);

/* Jump to return of execute system call */
int32_t jump_to_exec_ret(int32_t ebp, uint32_t status);

/* read data from the keyboard, a file, device (RTC), or directory */
extern int32_t read(int32_t fd, void* buf, int32_t nbytes);

/* write data to the terminal or to a device (RTC) */
extern int32_t write(int32_t fd, const void* buf, int32_t nbytes);

/* provide access to the file system */
extern int32_t open(const uint8_t* filename);

/* close the specified file descriptor */
extern int32_t close(int32_t fd);

/* read the program's command line arguments into a user-level buffer */
extern int32_t getargs(uint8_t* buf, int32_t nbytes);

/* map the text-mode video memory into user space at a pre-set virtual address */
extern int32_t vidmap(uint8_t** screen_start);


/**
 * ______________________________________________________
 * reserved for extra credit
*/

extern int32_t set_handler(int32_t signum, void* handler_address);

extern int32_t sigreturn(void);

#endif
