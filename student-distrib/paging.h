#ifndef _PAGE_H
#define _PAGE_H

#include "types.h"

#define NUM_ENTRY   1024
#define PAGE_SIZE   4096
#define FOUR_MB     0x400000
#define EIGHT_MB    0x800000

#define PROG_VID_ENTRY 33
#define PROGRAM_DIRECTORY_VIRTUAL_ADDR  0x08048000

#define VIDEO       0xB8000

#define PD_1_ADDR   0x400
#define SHIFT_4K    12
#define SHIFT_4M    22
#define SHIFT_PHY   11

/* page directory entry */
typedef struct page_dicr_entry {
    uint32_t present    :1;
    uint32_t r_w        :1;
    uint32_t u_s        :1;
    uint32_t write_t    :1;
    uint32_t cache_dis  :1;
    uint32_t access     :1;
    uint32_t reserve_0  :1;
    uint32_t page_size  :1;
    uint32_t ignored    :1;
    uint32_t reserve_1  :3;             // bit 9 - 11
    uint32_t page_table_addr   :20;     // bit 12 - 32
} page_dicr_entry_t;

/* page table entry */
typedef struct page_table_entry{
    uint32_t present    :1;
    uint32_t r_w        :1;
    uint32_t u_s        :1;
    uint32_t write_t    :1;
    uint32_t cache_dis  :1;
    uint32_t access     :1;
    uint32_t dirty      :1;
    uint32_t ptai       :1;
    uint32_t global     :1;
    uint32_t reserve_1  :3;            // bit 9 - 11
    uint32_t page_base_addr   :20;     // bit 12 - 32
} page_table_entry_t;

page_dicr_entry_t page_directory[NUM_ENTRY] __attribute__((aligned(PAGE_SIZE)));
/* page table: 0~4MB*/
page_table_entry_t page_table_0[NUM_ENTRY] __attribute__((aligned(PAGE_SIZE)));
/* page table: 128~132MB*/
page_table_entry_t page_table_video[NUM_ENTRY] __attribute__((aligned(PAGE_SIZE)));

/* initialize paging (driver function) */
void init_paging();

/* setup paging for a program */
void setup_paging(int32_t prog_counter);

/* initialize the page directory */
void init_directory();

/* enable a program page */
void enable_program_page(int32_t prog_counter);

/* initialize the page table 0 */
void init_table_0();

/* save previous terminal's video memory to its backup page */
void save_video_to_backup(int prev_id);

/* restore video memory from the new terminal's video memory backup page */
void restore_backup_to_video(int next_id);

/* initialize video memory page */
void init_prog_vid();

/* enable video memory page */
void enable_prog_vid_page();

/* disable video memory page */
void disable_prog_vid_page();

/* flush_tlb. Called after changing virtual memory mapping */
void flush_tlb();

/* load page directory address to cr3 */
void load_page_directory(page_dicr_entry_t* addr);

/* enable paging with mixtured page sizes */
void enable_paging();

/* Change the program video mem mapping */
void change_prog_vid_mapping(int32_t term_id);

#endif
