#ifndef _DEV_INT_H
#define _DEV_INT_H

#define RTC_PIC     8

#define REG_A       0x8A
#define REG_B       0x8B
#define REG_C       0x0C
#define RTC_STATUS  0x70
#define RTC_DATA    0x71

#include "types.h"
#include "process.h"

/* define the maximum and minimum frequency allowed for RTC */
#define MAX_RTC_FREQ 512

/* initialize the rtc */
void rtc_init();
/* set the rtc frequency */
void set_rtc_freq(char rate);
/* handle a rtc interrupt */
extern void rtc_handler();

/* force the program to wait until next virtual rtc interrupt */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
/* change the RTC interrupt frequency(virtual) */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
/* open and initialize the rtc */
int32_t rtc_open(int32_t fd);
/* close the rtc file*/
int32_t rtc_close(int32_t fd);

/* rtc op table */
file_op_table_t rtc_op_table;
void rtc_op_init();

#endif
