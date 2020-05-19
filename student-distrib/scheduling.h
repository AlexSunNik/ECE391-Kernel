#ifndef _SCHEDULE_H
#define _SCHEDULE_H

//#define cur_term_id active_term_idx

#define PIT_FREQ_SET    0x36
#define PIT_CTR_PORT    0x43
#define PIT_FREQ        11931
#define PIT_CHA0_PORT   0x40
#define LOWER_8         0xFF
#define UPPER_8         0xFF00
#define SHIFT_8         8

#define TERM_NUM 3

/* launch the first terminal */
void start_terminal0();
/* switch terminal display */
void switch_terminal(int prev_id, int term_id);
/* initialize 3 terminals */
void init_terminals();
/* scheduler switch process */
void switch_process();
/* initialize pit */
void pit_init();
/* pit interrupt handler */
extern void pit_handler();

/* the index of the terminal which is currently scheduled */
int32_t cur_term_id;
/* the index of the terminal which is lastly scheduled */
int32_t prev_term_id;
/* note: active_term_idx defined in keyboard.h is the terminal
 * which is being displayed. They are NOT the same index.
 */
#endif
