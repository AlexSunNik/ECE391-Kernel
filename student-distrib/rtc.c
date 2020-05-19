#include "rtc.h"
#include "lib.h"
#include "i8259.h"
#include "process.h"



/* set RTC_TEST_ENABLE to enable the test_interrupts() */
#define RTC_TEST_ENABLE     0
#define ratio file_position

volatile int rtc_counter_global = 0;
volatile int rtc_exe_flag = 1;

/* 
 * rtc_handler
 *   DESCRIPTION: handle a rtc interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. print a counter (DEBUG), 2. test interrupts
 *                 3. safeguard register C, 4. send eoi to port 8 (for rtc)
 */
void rtc_handler(){
    //Re-enable the interrupt
    /* increment rtc counter every time a physical interrupt is received */
    rtc_counter_global++;
    /* rtc interrupt has not been handled by task*/
    rtc_exe_flag = 1;

    #if (RTC_TEST_ENABLE == 1)
    test_interrupts();
    #endif

    /* safeguard register C */
    outb(REG_C, RTC_STATUS);
    inb(RTC_DATA);
    
    /* send eoi to port 8 */
    send_eoi(RTC_PIC);
}


/* 
 * rtc_init
 *   DESCRIPTION: initialize the rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. select register B and enable interrupt
 */
void rtc_init(){
    char prev;
    // outb(0x8A, RTC_STATUS); //Disable NMI
    // outb(0x20, RTC_DATA);   //write CMOS/RTC RAM

    // read from register B
    outb(REG_B, RTC_STATUS);
    prev = inb(RTC_DATA);
    
    // write to register B (0x40: enable the interrupt bit)
    outb(REG_B, RTC_STATUS);
    outb(prev | 0x40, RTC_DATA);
    
    //Init with the highest freq
    set_rtc_freq(3);    // value 3 for highest freq
    // Remember to read from register C at the end of
    // RTC handler code to get another interrupt
    // rtc_counter_global = 0;
}

/* 
 * set_rtc_freq
 *   DESCRIPTION: set a customized frequnecy for rtc
 *   !!!IMPORTANT NOTE: 
 *      1). The rate setting must be a value from 
 *          1 to 15
 *      2). A rate of 0 will disable the interrupt
 *      3). New frequnecy is calculate by:
 *          freq = 32768 >> (rate - 1);
 *      4). The highest frequency in reality is 512
 *      5). Highest rate setting is 3!!
 *   INPUTS:  rate:  the rate used to calculate frequency of RTC
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. select register B and enable interrupt
 *                 2. change rtc interrupt frequency
 */
void set_rtc_freq(char rate){
    char prev;
    rate &= 0x0F;               // Lower 4 bits for rate, & with 0x0F
    outb(REG_A, RTC_STATUS);
    prev = inb(RTC_DATA);       //Read the currrent val of regA
    outb(REG_A, RTC_STATUS);
    outb((prev & 0xF0) | rate, RTC_DATA);       // Higher 4 bits from regA, & with 0xF0
}

/* 
 * rtc_read
 *   DESCRIPTION: force the program to wait until next virtual rtc interrupt
 *   INPUTS:  v_rtc: a virtual rtc struct associated with the current task
 *              buf: unused
             nbytes: unused
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
*                 -1 on invalid v_rtc
 *   SIDE EFFECTS: none
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    buf = buf;
    nbytes = nbytes;
    /* failure: invalid fd */
    /* valid array range: 2-7 */
    if (fd < 2 || fd > 7) return -1;
    /* wait until next physical rtc interrupt */
    sti();
    while(!rtc_exe_flag);
    cli();
    /* 3 is the total terminal number */
    int div = (file_array[fd].ratio / 3);
    if(!div)
        div = 1;
    /* wait until next virtual rtc interrupt */
    sti();
    while(rtc_counter_global % div);
    cli();
    /* rtc interrupt is received */
    rtc_exe_flag = 0;

    return 0;
}

/* 
 * rtc_write
 *   DESCRIPTION: change the RTC interrupt frequency(virtual)
 *   INPUTS:  v_rtc: a virtual rtc struct associated with the current task
 *              buf: a pointer to the desired frequency in Hz
             nbytes: the length of buf
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
 *                 -1 on invalid v_rtc, invalid frequency or invalid nbytes
 *   SIDE EFFECTS: change the virtual rtc interrupt frequency of current task
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    /* check if buffer and nbytes are valid */
    if(buf == NULL || nbytes <= 1) return -1;
    /* failure: invalid fd */
    /* valid array range: 2-7 */
    if (fd < 2 || fd > 7) return -1;
    uint32_t freq = *(uint32_t *)buf; 
    /* check if frequency is valid */
    //freq & (freq - 1) == 0 to check power of two
    if((freq & (freq-1)) != 0 || freq > MAX_RTC_FREQ) return -1;
    
    /* derive ratio from frequency */
    /* ratio is relative to the maximum freq of RTC */
    file_array[fd].ratio = MAX_RTC_FREQ / freq;
    return 0;
}

/* 
 * rtc_open
 *   DESCRIPTION: open and initialize the rtc 
 *   INPUTS:  filename: the file name of rtc
 *               v_rtc: a virtual rtc struct associated with the current task
 *   OUTPUTS: none
 *   RETURN VALUE:  0 on success
 *                 -1 on invalid v_rtc, invalid filename or rtc file not found
 *   SIDE EFFECTS: initialize the virtual rtc interrupt frequency to 2Hz
 */
int32_t rtc_open(int32_t fd) {
    /* failure: invalid fd */
    /* valid array range: 2-7 */
    if (fd < 2 || fd > 7) return -1;
    /* rtc does not have inode */ 
    file_array[fd].inode_idx = 0;
    /* we use file_position field to store ratio */
    /* write virual rtc interrupt frequency */
    file_array[fd].ratio= MAX_RTC_FREQ / 2;
    return 0;      
}

/* 
 * rtc_close
 *   DESCRIPTION: close the rtc file
 *   INPUTS:    fd: file descriptor(not used)
 *   OUTPUTS: none
 *   RETURN VALUE:  0 on success
 *   SIDE EFFECTS:  none
 */
int32_t rtc_close(int32_t fd) {
    fd = fd;
    return 0;
}

/*  
 * rtc_op_inits
 *   DESCRIPTION: initiate operation pointer in kernel
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: setup operation pointer table
 */
void rtc_op_init() {
    rtc_op_table.open = (void*)rtc_open;
    rtc_op_table.read = (void*)rtc_read;
    rtc_op_table.write = (void*)rtc_write;
    rtc_op_table.close = (void*)rtc_close;
}
