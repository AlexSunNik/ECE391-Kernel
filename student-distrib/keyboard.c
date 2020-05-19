#include "lib.h"
#include "keyboard.h"
#include "i8259.h"
#include "process.h"
#include "scheduling.h"
#include "system_call.h"

char CAPS_FLAG = 0;
char L_SHIFT_FLAG = 0;
char R_SHIFT_FLAG = 0;
char SHIFT_FLAG = 0;
char CTRL_FLAG = 0;
char ALT_FLAG = 0;

#define CAPS_PRESS      0x3A
#define L_SHIFT_PRESS     0x2A
#define L_SHIFT_RELEASE   0xAA
#define CTRL_PRESS      0x1D
#define CTRL_RELEASE    0x9D
#define R_SHIFT_PRESS     0x36
#define R_SHIFT_RELEASE   0xB6
#define ENTER_PRESS     0x1C
#define BACKSPACE       0x0E
#define TAB             0x0F
#define TAB_SPACE       4

#define ALT_PRESS       0x38
#define ALT_RELEASE     0xB8

#define F1_PRESS        0x3B
#define F2_PRESS        0x3C
#define F3_PRESS        0x3D


/* 
 * keyboard_handler
 *   DESCRIPTION: handle a keyboard interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. get the scancode, 2. print the key in ascii
 *                 3. send eoi to port 1 (for keyboard)
 */
void keyboard_handler(){
    /* fetch keycode */
    unsigned char scancode = inb(KEYBOARD_DATA);
    char ascii_char = scancode_handler(scancode);

    /* invalid keycode: not handle */
    if(ascii_char == -1){
        send_eoi(KEYBOARD_PIC);
        return;
    }
    /* check whether user wants to switch displayed terminal */
    if ((ascii_char == F1_PRESS)||(ascii_char == F2_PRESS)||(ascii_char == F3_PRESS)){
        if (ALT_FLAG) {
            int prev_term_idx = active_term_idx;
            // switch to display terminal 0 
            if(ascii_char == F1_PRESS && active_term_idx != 0)
                active_term_idx = 0;
            // switch to display terminal 1
            else if(ascii_char == F2_PRESS && active_term_idx != 1)
                active_term_idx = 1;
            // switch to display terminal 2
            else if(ascii_char == F3_PRESS && active_term_idx != 2)
                active_term_idx = 2;
            // switch terminal display
            switch_terminal(prev_term_idx, active_term_idx);
            send_eoi(KEYBOARD_PIC);
            return;
        }
        send_eoi(KEYBOARD_PIC);
        return;
    }

    /* terminal_read is executing*/
    if(terminal0.TERMINAL_READ_FLAG){
        /* if ctl+l is pressed, clear keyboard buffer */
        if (ascii_char == CTRL_L) {
            terminal0.buf_index = 0;
            send_eoi(KEYBOARD_PIC);
            return;
        }
        /* if ENTER is pressed, signal terminal_read to continue */
        /* store in buffer and echo to screen */
        if(ascii_char == NEW_LINE) {
            /* signal terminal_read */
            terminal0.TERMINAL_READ_FLAG = 0;
            terminal0.keyboard_buf[terminal0.buf_index] = ascii_char;
            printf_direct("%c", ascii_char);
            terminal0.buf_index++;
            send_eoi(KEYBOARD_PIC);
            return;
        }
        /* if TAB is pressed, push 4 SPACE to buffer unless reach the end */
        /* store in buffer and echo to screen */
        int tab_counter = 0;
        while (terminal0.buf_index < KB_BUF_SIZE-1 && ascii_char == TAB && tab_counter < TAB_SPACE) {
            terminal0.keyboard_buf[terminal0.buf_index] = ' ';
            terminal0.buf_index++;
            printf_direct("%c", ' ');
            tab_counter++;
        }
        /* if other key is pressed, store in buffer and echo to screen*/
        if(terminal0.buf_index < KB_BUF_SIZE-1 && ascii_char != TAB){
            terminal0.keyboard_buf[terminal0.buf_index] = ascii_char;
            terminal0.buf_index++;
            printf_direct("%c", ascii_char);
        }
    }
    /* terminal_read is not executing, simply echo char to screen */
    else{
        if (ascii_char == CTRL_L) {
            send_eoi(KEYBOARD_PIC);
            return;
        }
        int tab_counter = 0;
        while (ascii_char == TAB && tab_counter < TAB_SPACE) {
            printf_direct("%c", ' ');
            tab_counter++;
        }
        if (ascii_char != TAB)
            printf_direct("%c", ascii_char);
    }
    send_eoi(KEYBOARD_PIC);
}

/* key value for scancode set 1 */
/* when CAPS_FLAG =0 and SHIFT_FLAG =0 */
char scancode_set_1[NUM_SCANCODE] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', 0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', 0, 0, 'a', 's', 'd', 'f', 'g', 'h', 
    'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, 0,0,' '
    };

/* key value for scancode set 2 */
/* when CAPS_FLAG = 1 and SHIFT_FLAG = 0*/
char scancode_set_2[NUM_SCANCODE] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', 0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '[', ']', 0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 
    'J', 'K', 'L', ';', '\'', '`', 0, '\\', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.', '/', 0, 0, 0,' '
    };


/* key value for scancode set 3 */
/* when SHIFT_FLAG = 1 and do not care CAPS_LOCK_FLAG */
char scancode_set_3[NUM_SCANCODE] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', 0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', 0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 
    'J', 'K', 'L', ':', '\"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' '
    };

/* 
 * scancode_handler
 *   DESCRIPTION: convert scancode to ascii value
 *   INPUTS: scancode: scancode to be converted
 *   OUTPUTS: none
 *   RETURN VALUE: ascii character if scancode is recorded in scancode set
 *                             -1  if invalid scancode
 *                          CTRL_L if CTRL_L is pressed
 *   SIDE EFFECTS: none
 */
char scancode_handler(unsigned char scancode) {
    /* default: return -1 */
    char character = -1;
    /* check CAPS_LOCK pressed */
    if (scancode == CAPS_PRESS) {
        if (CAPS_FLAG)
            CAPS_FLAG = 0;
        else
            CAPS_FLAG = 1;
    }
    /* check LEFT_SHIFT pressed */
    if (scancode == L_SHIFT_PRESS)
        L_SHIFT_FLAG = 1;
    /* check RIGHT_SHIFT pressed */
    if (scancode == R_SHIFT_PRESS)
        R_SHIFT_FLAG = 1;
    /* check LEFT_SHIFT released */
    if (scancode == L_SHIFT_RELEASE)
        L_SHIFT_FLAG = 0;
    /* check RIGHT_SHIFT released */
    if (scancode == R_SHIFT_RELEASE)
        R_SHIFT_FLAG = 0;
    /* any of left and right shift can set the flag
     * both left and right shift need to be absent to clear the flag
     */
    SHIFT_FLAG = L_SHIFT_FLAG || R_SHIFT_FLAG;
    /* check CTRL pressed */
    if (scancode == CTRL_PRESS)
        CTRL_FLAG = 1;
    /* check CTRL released */
    if (scancode == CTRL_RELEASE)
        CTRL_FLAG = 0;
    /* check ALT pressed */
    if (scancode == ALT_PRESS)
        ALT_FLAG = 1;
    /* check ALT released */
    if (scancode == ALT_RELEASE)
        ALT_FLAG = 0;
    /* check F1 pressed */
    if (scancode == F1_PRESS)
        return F1_PRESS;
    /* check F2 pressed */
    if (scancode == F2_PRESS)
        return F2_PRESS;
    /* check F3 pressed */
    if (scancode == F3_PRESS)
        return F3_PRESS;
    
    /* check CTRL + L pressed, clear the screen, 0x26 : L */
    if (CTRL_FLAG && scancode == 0x26) {
        clear();
        return CTRL_L;
    }

    /* check BACKSPACE pressed */
    if (scancode == BACKSPACE) {
        if (terminal0.TERMINAL_READ_FLAG) {
            if (terminal0.buf_index == 0)
                return -1;
            else
                terminal0.buf_index--;
        }
        backspace();
    }
    /* check ENTER pressed */
    if (scancode == ENTER_PRESS){
        character = NEW_LINE;
    }
    /* check TAB pressed */
    else if (scancode == TAB) {
        character = TAB;
    }
    /* fetch lowercase letter or number */
    else if ((scancode < NUM_SCANCODE) && !CAPS_FLAG && !SHIFT_FLAG) {
        character = scancode_set_1[(int) scancode];
        if (!character)
            return -1;
    }
    /* fetch uppercase letter or number */
    else if ((scancode < NUM_SCANCODE) && CAPS_FLAG && !SHIFT_FLAG) {
        character = scancode_set_2[(int) scancode];
        if (!character)
            return -1;
    }
    /* fetch uppercase letter or symbol */
    else if ((scancode < NUM_SCANCODE) && SHIFT_FLAG) {
        character = scancode_set_3[(int) scancode];
        if (!character)
            return -1;
    }

    return character;
}

/* 
 * terminal_open
 *   DESCRIPTION: open the terminal
 *   INPUTS:  none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
 *   SIDE EFFECTS: none
 */
int32_t terminal_open(int32_t fd){
    fd = fd;
    return 0;
}

/* 
 * terminal_read
 *   DESCRIPTION: read from the terminal. Keystroke will be stored into keyboard
 *                buffer and echo on the screen. Reading terminates when caller 
 *                presses ENTER. the maximum # of bytes read is 128 and the last
 *                byte is '\n'. Caller needs to allocate space for buf
 *   INPUTS: buf: destination of character reading. Must be at least nbytes large
 *        nbytes: # of bytes read from terminal
 *   OUTPUTS: echoes for keystroke
 *   RETURN VALUE: # of elements read (including '\n')
 *   SIDE EFFECTS: arg buf is filled. Keyboard buffer is cleared.
 */
int32_t terminal_read(int32_t fd, char* buf, int32_t nbytes){
    if (buf == 0 || nbytes < 0 || fd != 0)
        return -1;
    fd = fd;
    int32_t i;

    /* signal interrupt handler to store char into buffer */
    terminals[cur_term_id].TERMINAL_READ_FLAG = 1;
    /* clear keyboard buffer */
    terminals[cur_term_id].buf_index = 0;
    /* wait until interrupt handler clear TERMINAL_READ_FLAG
     * i.e. ENTER is pressed by user
     */

    /* cur_term_id -> running process change 10ms */
    sti();
    while (terminals[cur_term_id].TERMINAL_READ_FLAG);
    cli();
    /* copy from keyboard buffer to caller's buffer*/
    int32_t loop_end = (nbytes < (int32_t)terminals[cur_term_id].buf_index) ? 
        nbytes : (int32_t)terminals[cur_term_id].buf_index;
    for(i = 0; i < loop_end - 1; i++)
        buf[i] = terminals[cur_term_id].keyboard_buf[i];
    /* terminate the copy with newline character */
    buf[loop_end - 1] = '\n';
    return loop_end;
}

/* 
 * terminal_write
 *   DESCRIPTION: write to the terminal from arg buf
 *   INPUTS: buf: soruce of character writing. Must be at least nbytes large
 *        nbytes: # of bytes written to terminal
 *   OUTPUTS: characters displayed on terminal
 *   RETURN VALUE: # of bytes on success
 *   SIDE EFFECTS: none
 */
int32_t terminal_write(int32_t fd, char* buf, int32_t nbytes) {
    /* failure: invalid buf */
    if (!buf || nbytes < 0 || fd != 1){
        return -1;
    }

    /* iterate over char in buf and put it on screen */
    int32_t idx;
    for (idx = 0; idx < nbytes; idx++){
        //If nbytes is bigger than len(buf)
        if (!buf[idx]) continue;
        // display
        putc(buf[idx]);
    }
    return nbytes;
}

/* 
 * terminal_close
 *   DESCRIPTION: close the terminal
 *   INPUTS:  none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
 *   SIDE EFFECTS: none
 */
int32_t terminal_close(int32_t fd) {
    fd = fd;
    return 0;
}

/*  
 * terminal_op_init
 *   DESCRIPTION: initiate operation pointer in kernel
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: setup operation pointer table
 */
void terminal_op_init() {
    terminal_op_table.open = (void*)terminal_open;
    terminal_op_table.close = (void*)terminal_close;
    terminal_op_table.read = (void*)terminal_read;
    terminal_op_table.write = (void*)terminal_write;
}
