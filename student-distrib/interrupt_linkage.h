#ifndef _INT_LINK_H
#define _INT_LINK_H

#ifndef ASM

/* define the wrapper assembly code as functions */
extern void RTC_int             ();
extern void Keyboard_int        ();
extern void PIT_int             ();

#endif
#endif
