#ifndef _FILE_SYS_H
#define _FILE_SYS_H

#include "types.h"
#include "process.h"

#define NAME_LENGTH_MAX 32
#define RESERVE_LENGTH  24
#define VALID_BLOCK     1023
#define DATA_LENGTH     4096

#define FILE_LIMIT      8

typedef struct dentry{
    uint8_t file_name[NAME_LENGTH_MAX];
    uint32_t file_type;
    uint32_t inode_idx;
    uint8_t reserved[RESERVE_LENGTH];
} dentry_t;

typedef struct inode{
    uint32_t length;
    uint32_t block_idx[VALID_BLOCK];
} inode_t;

typedef struct data_block{
    uint8_t data[DATA_LENGTH];
} data_block_t;


/* test function to print file names */
void print_file_names();
/* initialize the file system */
void init_file_system(uint32_t fs_addr);

/* fill up the input dentry corresponding to fname */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
/* fill up the input dentry corresponding to the dentry index */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

/* get the file size */
int32_t get_file_size(dentry_t * dentry);
/* reading up to 'length' bytes of data from file system into buf */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, int32_t length);

/* helper funtions for syscall execute */
void parse_command(const uint8_t* command, uint8_t* filename, uint8_t* params);
int32_t program_loader(dentry_t* prog_dentry);
int32_t check_validity(dentry_t* prog_dentry);

/* set up the file abstraction struct when we open a file */
int32_t file_open(int32_t fd);
/* sys call convention, do nothing */
int32_t file_close(int32_t fd);
/* try to read nbytes from a file, fill in buf */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
/* sys call convention, do nothing (read-only file system) */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

/* sys call convention, set up information */
int32_t directory_open(int32_t fd);
/* sys call convention, do nothing */
int32_t directory_close(int32_t fd);
/* read directory names one at a time */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);
/* sys call convention, do nothing (read-only file system) */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);

/* file op table for regular file and directory */
file_op_table_t regular_file_op_table;
file_op_table_t dirt_op_table;
void file_op_init();

#endif
