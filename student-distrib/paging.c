#include "lib.h"
#include "paging.h"
#include "scheduling.h"
#include "keyboard.h"


#define PROGRAM_DIRECTORY_INDEX         PROGRAM_DIRECTORY_VIRTUAL_ADDR >> SHIFT_4M

/* 
 * init_paging
 *   DESCRIPTION: initialize paging (driver function)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initialize one directory, one table, cr3, and cr4+cr0
 */
void init_paging() {
    init_directory();
    init_table_0();
    init_prog_vid();
    load_page_directory(page_directory);
    enable_paging();
}

/* 
 * setup_paging
 *   DESCRIPTION: setup paging for the current program
 *   INPUTS: prog_counter -- current program page index
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: enable one directory entry and flush tlb (set cr3)
 */
void setup_paging(int32_t prog_counter) {
    enable_program_page(prog_counter);
    flush_tlb();
    //enable_paging();
}

/* 
 * init_directory
 *   DESCRIPTION: initialize the page directory
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. initialize all the entries, 2. set the kernel entry
 */
void init_directory() {
    /* set every entry to be not present, read/write enabled, and only kernel-mode */
    int i;
    for (i = 0; i < NUM_ENTRY; i++) {
        page_directory[i].present = 0;
        page_directory[i].r_w = 1;
        page_directory[i].u_s = 0;
        page_directory[i].write_t = 0;
        page_directory[i].cache_dis = 0;
        page_directory[i].access = 0;
        page_directory[i].reserve_0 = 0;
        page_directory[i].page_size = 0;
        page_directory[i].ignored = 0;
        page_directory[i].reserve_1 = 0;
        page_directory[i].page_table_addr = 0;
    }

    /* initialize the kernel 4MB page */
    //4MB is starting of Kernel
    page_directory[1].present = 1;
    page_directory[1].r_w = 1;
    page_directory[1].u_s = 0;
    page_directory[1].write_t = 0;
    page_directory[1].cache_dis = 0;
    page_directory[1].access = 0;
    page_directory[1].reserve_0 = 0;
    page_directory[1].page_size = 1; //4MB
    page_directory[1].ignored = 1;
    page_directory[1].reserve_1 = 0;
    page_directory[1].page_table_addr = PD_1_ADDR;
}

/* 
 * enable_program_page
 *   DESCRIPTION: enable 8mb-12mb page
 *   INPUTS: prog_counter -- current program page index
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: enable 8mb-12mb directory entry and flush tlb (set cr3)
 */
void enable_program_page(int32_t prog_counter) {
    int32_t physical_addr = prog_counter * FOUR_MB + EIGHT_MB;

    /* initialize the program 4MB page */
    page_directory[PROGRAM_DIRECTORY_INDEX].present = 1;
    page_directory[PROGRAM_DIRECTORY_INDEX].r_w = 1;
    page_directory[PROGRAM_DIRECTORY_INDEX].u_s = 1; //user mode
    page_directory[PROGRAM_DIRECTORY_INDEX].write_t = 0;
    page_directory[PROGRAM_DIRECTORY_INDEX].cache_dis = 0;
    page_directory[PROGRAM_DIRECTORY_INDEX].access = 0;
    page_directory[PROGRAM_DIRECTORY_INDEX].reserve_0 = 0;
    page_directory[PROGRAM_DIRECTORY_INDEX].page_size = 1;
    page_directory[PROGRAM_DIRECTORY_INDEX].ignored = 1;
    page_directory[PROGRAM_DIRECTORY_INDEX].reserve_1 = 0;
    page_directory[PROGRAM_DIRECTORY_INDEX].page_table_addr = physical_addr >> SHIFT_4K; //0x800, 0xC00
}

/* 
 * init_table_0
 *   DESCRIPTION: initialize the page table for 0MB-4MB
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. initialize all the entries, 2. set the entry for video memory page
 *                 3. set the page directory entry for this page table
 */
void init_table_0() {
    /* How to setup? */
    /* set every entry to be not present, read/write enabled, and only kernel-mode */
    int i;
    for (i = 0; i < NUM_ENTRY; i++) {
        page_table_0[i].present = 0;
        page_table_0[i].r_w = 1;
        page_table_0[i].u_s = 0;
        page_table_0[i].write_t = 0;
        page_table_0[i].cache_dis = 0;
        page_table_0[i].access = 0;
        page_table_0[i].dirty = 0;
        page_table_0[i].ptai = 0;
        page_table_0[i].global = 0;
        page_table_0[i].reserve_1 = 0;
        page_table_0[i].page_base_addr = i;
    }

    /* set video memory and three backup video pages to be present */ 
    page_table_0[VIDEO >> SHIFT_4K].present = 1;
    page_table_0[(VIDEO >> SHIFT_4K) + 1].present = 1;
    page_table_0[(VIDEO >> SHIFT_4K) + 2].present = 1;
    page_table_0[(VIDEO >> SHIFT_4K) + 3].present = 1;

    /* put the table into our directory */
    page_directory[0].present = 1;
    page_directory[0].r_w = 1;
    page_directory[0].page_table_addr = (unsigned int)page_table_0 >> SHIFT_4K;
}

/*  
 * save_video_to_backup
 *   DESCRIPTION: save previous terminal's video memory to its backup page 
 *   INPUTS: int prev_id --- previous terminal id 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void save_video_to_backup(int prev_id) {
    memcpy((void *) (VIDEO + ((1 + prev_id) << SHIFT_4K)), (void *)VIDEO, (1 << SHIFT_4K));
}

/*  
 * restore_backup_to_video
 *   DESCRIPTION: restore video memory from the new terminal's video memory backup page
 *   INPUTS: int next_id --- new(current) terminal id 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory(display) changes to reflect restoring
 */
void restore_backup_to_video(int next_id) {
    memcpy((void *)VIDEO, (void *) (VIDEO + ((1 + next_id) << SHIFT_4K)), (1 << SHIFT_4K));
}

/*  
 * init_prog_vid
 *   DESCRIPTION: initialize a page table for text-mode video memory (not present yet)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_prog_vid() {
    /* How to setup? */
    /* set every entry to be not present, read/write enabled, and user mode */
    int i;
    for (i = 0; i < NUM_ENTRY; i++) {
        page_table_video[i].present = 0;
        page_table_video[i].r_w = 1;
        page_table_video[i].u_s = 1;
        page_table_video[i].write_t = 0;
        page_table_video[i].cache_dis = 0;
        page_table_video[i].access = 0;
        page_table_video[i].dirty = 0;
        page_table_video[i].ptai = 0;
        page_table_video[i].global = 0;
        page_table_video[i].reserve_1 = 0;
        page_table_video[i].page_base_addr = i;
    }

     /* put the table into our directory */
    page_directory[PROG_VID_ENTRY].present = 0;
    page_directory[PROG_VID_ENTRY].r_w = 1;
    page_directory[PROG_VID_ENTRY].u_s = 1;
    page_directory[PROG_VID_ENTRY].page_table_addr = (unsigned int) page_table_video >> SHIFT_4K;

    // set up the page for video memory
    page_table_video[0].page_base_addr = VIDEO >> SHIFT_4K;
}

/*  
 * enable_prog_vid_page
 *   DESCRIPTION: enable the video memory page, then flush tlb
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: flush tlb
 */
void enable_prog_vid_page() {
    page_directory[PROG_VID_ENTRY].present = 1;
    page_table_video[0].present = 1;
    flush_tlb();
}

/*  
 * disable_prog_vid_page
 *   DESCRIPTION: disable the video memory page, then flush tlb
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: flush tlb
 */
void disable_prog_vid_page() {
    page_directory[PROG_VID_ENTRY].present = 0;
    page_table_video[0].present = 0;
    flush_tlb();
}

/*  
 * flush_tlb
 *   DESCRIPTION: flush_tlb. Called after changing virtual memory mapping
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void flush_tlb() {
    asm (
        "movl %cr3, %eax;"
        "movl %eax, %cr3;"
    );
}

/* 
 * load_page_directory
 *   DESCRIPTION: load the addr of page directory to cr3
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set cr3
 */
void load_page_directory(page_dicr_entry_t* addr) {
    addr = addr;
    // add 8 to %ebp to get arg addr
    asm (
        "movl 8(%ebp), %eax;"
        "movl %eax, %cr3;"
    );
}

/* 
 * enable_paging
 *   DESCRIPTION: enable paging with mixtured page sizes allowed
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. set cr4 to allow mixture of page size, 2. set cr3 to enable paging
 */
void enable_paging() {
    asm(
        // Enable Mixture of 4kb and 4mb access
        "movl %cr4, %eax;"
        "orl $0x00000010, %eax;"
        "movl %eax, %cr4;"

        // MSE: enable paging; LSE: enable protection mode
        "movl %cr0, %eax;"
        "orl $0x80000001, %eax;"
        "movl %eax, %cr0;"
    );
}

/*  
 *change_prog_vid_mapping
 *   DESCRIPTION: change program user-access video memory mapping when process switch
 *     active terminal maps to real video memory page; inactive terminal maps to backup page
 *   INPUTS: int term_id --- current terminal id 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: flush tlb
 */
void change_prog_vid_mapping(int32_t term_id) {
    /*  active terminal maps to real video memory page */ 
    if(term_id == active_term_idx)
        page_table_video[0].page_base_addr = VIDEO >> SHIFT_4K;
    /* inactive terminal maps to backup page */
    else
        page_table_video[0].page_base_addr = (VIDEO >> SHIFT_4K) + term_id + 1;
    flush_tlb();
}
