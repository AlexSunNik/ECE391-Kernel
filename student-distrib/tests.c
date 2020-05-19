#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "file_system.h"
#include "rtc.h"
#include "keyboard.h"
#include "process.h"
#include "system_call.h"

#define PASS 1
#define FAIL 0

#define VIDEO       0xB8000
#define LARGE_MEM	0x80000000
#define KERNEL_MEM	0x400000
#define KERNEL_MEM_OFFSET	0x400
#define DELAY		for (i = 0; i < 1000000; i++)

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

/* 
	   Checkpoint 1 Tests:
	   6.1.2 - load the GDT/LDT: 
	        	1. escape bootloop
	   6.1.3 - initialize the IDT:
	   			1. idt_test
	   			2. div_error_test
				3. invalid_opcode_test
				4. overflow_test
				5. bound_range_test
				6. system_call_test
	   6.1.4 - initialize the devices
	   			1. echo the input of keyboard
				2. test_interrupts enabled [in interrupts.c]
	   6.1.5 - initialize paging
	   			1. deref_NULL_test
				2. deref_test_large_location
				3. deref_test_video_mem
				4. deref_test_video_mem_plus_2kb
				5. deref_test_video_mem_end
				6. deref_test_video_mem_plus_4kb
				7. deref_test_kernel

		Checkpoint 2 Tests:
		6.2.1 - Terminal Driver:
				1. terminal_read_128_full
				2. terminal_read_16_partial
				3. terminal_write_128_full
				4. terminal_write_16_partial
				5. terminal_read_write_loop
		6.2.2 - Read Only File System:
				1. list_all_file
				2. print_file_txt_small
				3. print_file_txt_large
				4. print_file_exe
				5. print_invalid
		6.2.3 - Real-Time Clock Driver:
				1. rtc_freq_change
				2. rtc_invalid_freq

	enter TEST_ID for corresponding test, 6.1.5.1 -> TEST_ID = 6151
*/

#define TEST_ID		6222

/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;
	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	return result;
}

/* 
 * div_error_test
 *   DESCRIPTION: testing 6.1.3 - IDT exception DIVIDED_BY_ZERO
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise an exception
 */
void div_error_test() {
	TEST_HEADER;
	/* fix compilation warning: divide by 0 error */
	int a = 0;
	int b = 1/a;
	/* fix compilation warning: unused variable */
	b = b;
}

/* 
 * invalid_opcode_test
 *   DESCRIPTION: testing 6.1.3 - IDT exception INVALID_OPCODE
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise an exception
 */
void invalid_opcode_test() {
	TEST_HEADER;
	// Execute rsm while not in SMM mode
	asm (
		"rsm"
	);
}

/* 
 * overflow_test
 *   DESCRIPTION: testing 6.1.3 - IDT exception OVERFLOW
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise an exception
 */
void overflow_test() {
	TEST_HEADER;
	asm (
		"movl $0x7FFFFFFF, %eax;"
		"addl %eax, %eax;"
		"into;"
	);
}

/* 
 * bound_range_test
 *   DESCRIPTION: testing 6.1.3 - IDT exception bound_range_test
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise an exception
 */
void bound_range_test() {
	TEST_HEADER;
	asm (
		"movl $5, %eax;"
		"bound %eax, LABEL;"
		"LABEL:;"
		".long 3;"
	);
}

/* 
 * system_call_test
 *   DESCRIPTION: testing 6.1.3 - test system call
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: call a system call
 */
int system_call_test() {
	TEST_HEADER;
	asm (
		"movl $1, %eax;"
		"int $0x80;"
	);
	return PASS;
}

/* 
 * keyboard_echo_test
 *   DESCRIPTION: this function itself does nothing. Key can interrupt this function
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: ready to accecpt keyboard interrupts
 */
void keyboard_echo_test(){
	printf("Keyboard echo test:\n");
	while(1);
}

/* 
 * deref_NULL_test
 *   DESCRIPTION: testing 6.1.5 - enabled paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise PAGE_FAULT
 */
void deref_NULL_test() {
	TEST_HEADER;
	char a = *((int*) NULL);
	/* fix compilation warning: unused variable */
	a = a;
}

/* 
 * deref_test_large_location
 *   DESCRIPTION: testing 6.1.5 - enabled paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise PAGE_FAULT
 */
void deref_test_large_location() {
	TEST_HEADER;
	char a = *((int *) LARGE_MEM);
	/* fix compilation warning: unused variable */
	a = a;
}

/* 
 * deref_test_video_mem
 *   DESCRIPTION: testing 6.1.5 - enabled paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int deref_test_video_mem() {
	TEST_HEADER;
	char a = *((int *) VIDEO);
	/* fix compilation warning: unused variable */
	a = a;
	return PASS;
}

/* 
 * deref_test_video_mem_plus_2kb
 *   DESCRIPTION: testing 6.1.5 - enabled paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int deref_test_video_mem_plus_2kb() {
	TEST_HEADER;
	char a = *((int *) (VIDEO + 0x0800));		/* 0x0800 => 2048B = 2kB */
	/* fix compilation warning: unused variable */
	a = a;
	return PASS;
}

/* 
 * deref_test_video_mem_end
 *   DESCRIPTION: testing 6.1.5 - enabled paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int deref_test_video_mem_end() {
	TEST_HEADER;
	char a = *((char *) (VIDEO + 0x1000 - 1));		/* 0x1000 => 4096B = 4kB */
	/* fix compilation warning: unused variable */
	a = a;
	return PASS;
}

/* 
 * deref_test_video_mem_plus_4kb
 *   DESCRIPTION: testing 6.1.5 - enabled paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise PAGE_FAULT
 */
void deref_test_video_mem_plus_4kb() {
	TEST_HEADER;
	char a = *((int *) (VIDEO + 0x1000));		/* 0x1000 => 4096B = 4kB */
	/* fix compilation warning: unused variable */
	a = a;
}

/* 
 * deref_test_kernel
 *   DESCRIPTION: testing 6.1.5 - enabled paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int deref_test_kernel() {
	TEST_HEADER;
	char a = *((int *) (KERNEL_MEM + KERNEL_MEM_OFFSET));
	/* fix compilation warning: unused variable */
	a = a;
	return PASS;
}

/* Checkpoint 2 tests */

/* 
 * terminal_read_128_full
 *   DESCRIPTION: testing 6.2.1 - terminal read
 *                maximum buffer size of 128
 *   INPUTS: none
 *   OUTPUTS: echo while reading, print after reading
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int terminal_read_128_full() {
	TEST_HEADER;
	char buf[128];
	read(STDIN, buf, 128);
	printf("Finish reading.\nPrint content in buffer to terminal.\n");
	int i;
	for(i = 0; i < 128; i++)
		putc(buf[i]);
	return PASS;
}

/* 
 * terminal_read_16_partial
 *   DESCRIPTION: testing 6.2.1 - terminal read
 *                maximum buffer size of 37 (16 + 21)
 *   INPUTS: none
 *   OUTPUTS: echo while reading, print after reading
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int terminal_read_16_partial() {
	TEST_HEADER;
	char buf[36 + 1];	// add 1 for '\0'
	char* str = "Should stay at here\n";
	strncpy(buf + 16, str, 21);	// 21 is the str length
	printf("Enter more than 16 characters:\n");
	read(STDIN, buf, 16);
	printf("Finish reading.\nPrint content in buffer to terminal.\n");
	int i;
	for(i = 0; i < 36 + 1; i++)
		putc(buf[i]);
	return PASS;
}

/* 
 * terminal_write_128_full
 *   DESCRIPTION: testing 6.2.1 - terminal write
 *   INPUTS: none
 *   OUTPUTS: print 128(all) characters in the buffer to screen
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int terminal_write_128_full(){
	TEST_HEADER;
	char buf[128];
	/* initailize the buf */
	int i;
	for(i = 0; i < 128; i ++){
		buf[i] = 'A' + (char)(i % 4);	//loop print ABCD
	}
	write(STDOUT, buf, 128);
	return PASS;
}

/* 
 * terminal_write_16_partial
 *   DESCRIPTION: testing 6.2.1 - terminal write
 *   INPUTS: none
 *   OUTPUTS: print 16(part of) characters in the buffer to screen 
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int terminal_write_16_partial(){
	TEST_HEADER;
	char buf[128];
	/* initailize the buf */
	int i;
	for(i = 0; i < 128; i ++){
		buf[i] = 'A' + (char)(i%4);	//loop print ABCD
	}
	write(STDOUT, buf, 16);
	return PASS;
}

/* 
 * clear_buf
 *   DESCRIPTION: helper function to clear buffer while testing
 *   INPUTS: buf pointer, buffer soze
 *   OUTPUTS: clear the buffer to 0
 *   RETURN VALUE: none
 *   SIDE EFFECTS: if size is incorrect, buffer may be not clean
 */
void clear_buf(char * buf, uint32_t size) {
	uint32_t i;
	for (i = 0; i < size; i++) 
		*(buf + i) = 0;
}

/* 
 * terminal_read_write_loop
 *   DESCRIPTION: testing 6.2.1 - terminal read & terminal write
 *   INPUTS: none
 *   OUTPUTS: echoes while reading, print content of buffer when writing
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int terminal_read_write_loop(){
	TEST_HEADER;
	char buf[128];
	while(1){
		clear_buf(buf, 128);
		printf("Begin reading...\n");
		read(STDIN, buf, 128);
		printf("Finish reading, begin writing.\n");
		write(STDOUT, buf, 128);
	}
	return PASS;
}

/* 
 * terminal_read_128_full
 *   DESCRIPTION: testing 6.2.2 - directory read
 *                also print information such as file type and file size
 *   INPUTS: none
 *   OUTPUTS: print file names onto the screen
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int list_all_file() {
	TEST_HEADER;
	int fd;
	fd = open((uint8_t *)".");
	uint8_t buf[32 + 1];	// filename length is 32 B, add 1 for '\0'
	buf[32] = 0;
	while (read(fd, buf, 32)) {
		printf("File name: %s  ", buf);
		dentry_t file_info;
		read_dentry_by_name(buf, &file_info);
		if (file_info.file_type == 2) {
			printf("File type: %d  ", file_info.file_type);
			printf("File size: %d Byte\n", get_file_size(&file_info));
		} else
			printf("File type: %d  \n", file_info.file_type);
	}
	return PASS;
}

/* 
 * print_file_txt_small
 *   DESCRIPTION: testing 6.2.2 - file read
 *                read one character at a time and display it onto the screen
 *   INPUTS: none
 *   OUTPUTS: print frame0.txt onto the screen
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int print_file_txt_small() {
	TEST_HEADER;
	int fd;
	fd = open((uint8_t *)"frame0.txt");
	
	char buf;
	while (read(fd, &buf, 1) == 1) {
		if (buf != 0)
			printf(" %x", buf);
	}
	
	// char buf[1024];
	// int count = read(fd, &buf, 1024);
	// //printf("BUF:\n%s",buf);
	// write(1, buf, count);

	return PASS;
}

/* 
 * print_file_txt_large
 *   DESCRIPTION: testing 6.2.2 - file read
 *                read one character at a time and display it onto the screen
 *   INPUTS: none
 *   OUTPUTS: print verylargetextwithverylongname.txt onto the screen
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int print_file_txt_large() {
	TEST_HEADER;
	int fd;
	fd = open((uint8_t *)"verylargetextwithverylongname.txt");
	char buf;
	while (read(fd, &buf, 1) == 1) {
		if (buf != 0)
			putc(buf);
	}
	return PASS;
}

/* 
 * print_file_exe
 *   DESCRIPTION: testing 6.2.2 - file read
 *                read one character at a time and display it onto the screen
 *   INPUTS: none
 *   OUTPUTS: print testprint onto the screen
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int print_file_exe() {
	TEST_HEADER;
	int fd;
	fd = open((uint8_t *)"verylargetextwithverylongname.txt");
	char buf;
	while (read(fd, &buf, 1) == 1) {
		if (buf != 0)
			putc(buf);
	}
	return PASS;
}

/* 
 * print_invalid
 *   DESCRIPTION: testing 6.2.2 - file read
 *                read one character at a time and display it onto the screen
 *   INPUTS: none
 *   OUTPUTS: print the invalid code
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int print_invalid() {
	TEST_HEADER;
	int fd;
	fd = open((uint8_t *)"invalid.txt");
	if (fd)
		return PASS;
	else
		return FAIL;
}

/* 
 * rtc_freq_change
 *   DESCRIPTION: testing 6.2.3 - rtc freq
 *                adjust the rtc freq into different values and print @
 *   INPUTS: none
 *   OUTPUTS: print @ with different frequency onto the screen
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int rtc_freq_change() {
	TEST_HEADER;
	int fd;
	fd = open((uint8_t *)"rtc");
	// test 9 levels of frequency
	int freq_level = 9;
	int freq[9] = {2, 8, 32, 128, 512, 128, 32, 8, 2};	// freq levels
	int i;
	int garbage;
	printf("Virtual RTC test");
	for (i = 0; i < freq_level; i ++){
		write(fd, freq + i, 4);
		int print_times;
		printf("\nCurrent print freq: %d \n", freq[i]);
		// print @ for 5 seconds under each frequency
		for (print_times = 0; print_times < freq[i] * 5; print_times++) {
			read(fd, &garbage, 4);
			putc('@');
		}
	}
	close(fd);
	return PASS;
}

/* 
 * rtc_invalid_freq
 *   DESCRIPTION: testing 6.2.3 - rtc freq
 *                adjust the rtc freq to an invalid value
 * 				  rtc_write should return -1
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS on success
 *   SIDE EFFECTS: none
 */
int rtc_invalid_freq() {
	TEST_HEADER;
	int fd;
	//printf("enter test");
	fd = open((uint8_t *)"rtc");
	//printf("current fd:%d\n", fd);
	int freq = 182;	// invalid freq
	if (write(fd, &freq, 4) == -1)
		return PASS;
	else
		return FAIL;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point */
void launch_tests(){
	clear();

	/* 
	   Checkpoint 1 Tests:
	   6.1.2 - load the GDT/LDT: 
	        	1. escape bootloop
	   6.1.3 - initialize the IDT:
	   			1. idt_test
	   			2. div_error_test
				3. invalid_opcode_test
				4. overflow_test
				5. bound_range_test
				6. system_call_test
	   6.1.4 - initialize the devices
	   			1. echo the input of keyboard
				2. test_interrupts enabled [in interrupts.c]
	   6.1.5 - initialize paging
	   			1. deref_NULL_test
				2. deref_test_large_location
				3. deref_test_video_mem
				4. deref_test_video_mem_plus_2kb
				5. deref_test_video_mem_end
				6. deref_test_video_mem_plus_4kb
				7. deref_test_kernel

		Checkpoint 2 Tests:
		6.2.1 - Terminal Driver:
				1. terminal_read_128_full
				2. terminal_read_16_partial
				3. terminal_write_128_full
				4. terminal_write_16_partial
				5. terminal_read_write_loop
		6.2.2 - Read Only File System:
				1. list_all_file
				2. print_file_txt_small
				3. print_file_txt_large
				4. print_file_exe
				5. print_invalid
		6.2.3 - Real-Time Clock Driver:
				1. rtc_freq_change
				2. rtc_invalid_freq
	*/
	
	/* TEST_ID is specified at the LINE 47 */

	/* TEST_ID 6131 for idt_test */
	#if (TEST_ID == 6131)
		TEST_OUTPUT("idt_test", idt_test());
	#endif

	/* TEST_ID 6132 for div_error_test */
	#if (TEST_ID == 6132)
		div_error_test();
	#endif

	/* TEST_ID 6133 for invalid_opcode_test */
	#if (TEST_ID == 6133)
		invalid_opcode_test();
	#endif

	/* TEST_ID 6134 for overflow_test */
	#if (TEST_ID == 6134)
		overflow_test();
	#endif
	
	/* TEST_ID 6135 for bound_range_test */
	#if (TEST_ID == 6135)
		bound_range_test();
	#endif

	/* TEST_ID 6136 for system_call_test */
	#if (TEST_ID == 6136)
		TEST_OUTPUT("system_call_test", system_call_test());
	#endif

	/* TEST_ID 6141 for keyboard_echo_test */
	#if (TEST_ID == 6141)
		keyboard_echo_test();
	#endif

	/* TEST_ID 6151 for deref_NULL_test */
	#if (TEST_ID == 6151)
		deref_NULL_test();
	#endif

	/* TEST_ID 6152 for deref_test_large_location */
	#if (TEST_ID == 6152)
		deref_test_large_location();
	#endif

	/* TEST_ID 6153 for deref_test_video_mem */
	#if (TEST_ID == 6153)
		TEST_OUTPUT("deref_test_video_mem", deref_test_video_mem());
	#endif

	/* TEST_ID 6154 for deref_test_video_mem_plus_2kb */
	#if (TEST_ID == 6154)
		TEST_OUTPUT("deref_test_video_mem_plus_2kb", deref_test_video_mem_plus_2kb());
	#endif

	/* TEST_ID 6155 for deref_test_video_mem_plus_2kb */
	#if (TEST_ID == 6155)
		TEST_OUTPUT("deref_test_video_mem_end", deref_test_video_mem_end());
	#endif

	/* TEST_ID 6156 for deref_test_video_mem_plus_4kb */
	#if (TEST_ID == 6156)
		deref_test_video_mem_plus_4kb();
	#endif

	/* TEST_ID 6157 for deref_test_kernel */
	#if (TEST_ID == 6157)
		TEST_OUTPUT("deref_test_kernel", deref_test_kernel());
	#endif

	/* TEST_ID 6211 for terminal_read_128_full */
	#if (TEST_ID == 6211)
		TEST_OUTPUT("terminal_read_128_full", terminal_read_128_full());
	#endif

	/* TEST_ID 6212 for terminal_read_16_partial */
	#if (TEST_ID == 6212)
		TEST_OUTPUT("terminal_read_16_partial", terminal_read_16_partial());
	#endif

	/* TEST_ID 6213 for terminal_write_128_full */
	#if (TEST_ID == 6213)
		TEST_OUTPUT("terminal_write_128_full", terminal_write_128_full());
	#endif

	/* TEST_ID 6214 for terminal_write_16_partial */
	#if (TEST_ID == 6214)
		TEST_OUTPUT("terminal_write_16_partial", terminal_write_16_partial());
	#endif

	/* TEST_ID 6215 for terminal_read_write_loop */
	#if (TEST_ID == 6215)
		TEST_OUTPUT("terminal_read_write_loop", terminal_read_write_loop());
	#endif

	/* TEST_ID 6221 for list_all_file */
	#if (TEST_ID == 6221)
		TEST_OUTPUT("list_all_file", list_all_file());
	#endif

	/* TEST_ID 6222 for print_file_txt_small */
	#if (TEST_ID == 6222)
		TEST_OUTPUT("print_file_txt_small", print_file_txt_small());
	#endif

	/* TEST_ID 6223 for print_file_txt_large */
	#if (TEST_ID == 6223)
		TEST_OUTPUT("print_file_txt_large", print_file_txt_large());
	#endif

	/* TEST_ID 6224 for print_file_exe */
	#if (TEST_ID == 6224)
		TEST_OUTPUT("print_file_exe", print_file_exe());
	#endif

	/* TEST_ID 6225 for print_invalid */
	#if (TEST_ID == 6225)
		TEST_OUTPUT("print_invalid", print_invalid());
	#endif

	/* TEST_ID 6231 for rtc_freq_change */
	#if (TEST_ID == 6231)
		TEST_OUTPUT("rtc_freq_change", rtc_freq_change());
	#endif

	/* TEST_ID 6232 for rtc_invalid_freq */
	#if (TEST_ID == 6232)
		TEST_OUTPUT("rtc_invalid_freq", rtc_invalid_freq());
	#endif
}
