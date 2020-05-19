#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "process.h"
#include "lib.h"

#define KEYBOARD_DATA   0x60
#define KEYBOARD_CMD    0x64
#define KEYBOARD_PIC    1

#define NUM_SCANCODE    0x3A
#define NEW_LINE        0x0A

#define KB_BUF_SIZE 128
#define CTRL_L      -2
#define CTRL_C      -3

#define MAX_TERMINAL_NUM 3
#define MAX_PROG_NUM_PER_TERM 4

#define terminal0       terminals[active_term_idx]

/* the terminal that is currently being displayed */
int32_t active_term_idx;

/* handle a keyboard interrupt */
extern void keyboard_handler();
/* handle a scancode input */
char scancode_handler(unsigned char scancode);

/* terminal structure defination 
 * there are 3 terminals in 391OS running in parallel
 */
typedef struct terminal
{   
    int32_t active;
    /* store user's keystrokes to the terminal */
    char keyboard_buf[KB_BUF_SIZE];
    /* track the position of latest char in the buffer */
    int buf_index;
    /* store cursor position */
    int32_t cursor_x;
    int32_t cursor_y;
    /* pids of running programs for this terminal */
    int32_t prog_pids[MAX_PROG_NUM_PER_TERM];
    /* number of running programs for this terminal */
    int32_t term_prog_counter;
    /* whether the terminal is reading from keystrokes */
    volatile int32_t TERMINAL_READ_FLAG;
    
} terminal_t;

/* open a terminal (do nothing) */
int32_t terminal_open(int32_t fd);
/* read from a terminal, update buf (max size 128) */
int32_t terminal_read(int32_t fd, char* buf, int32_t nbytes);
/* write to the screen from buffer*/
int32_t terminal_write(int32_t fd, char* buf, int32_t nbytes);
/* close the terminal */
int32_t terminal_close(int32_t fd);

/* Keyboard OP Table */
file_op_table_t terminal_op_table;
void terminal_op_init();

/* a global terminal array */
terminal_t terminals[MAX_TERMINAL_NUM];

#endif
