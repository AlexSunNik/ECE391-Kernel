#include "lib.h"
#include "types.h"
#include "file_system.h"
#include "process.h"

#define PROGRAM_DIRECTORY_VIRTUAL_ADDR  0x08048000
const char EXEC_HEAD[4] = {0x7f, 0x45, 0x4c, 0x46};

uint32_t file_system_addr = 0;
uint32_t dentry_num = 0;
uint32_t inode_num = 0;
uint32_t block_num = 0;

#define MAGIC_NUM_LENGTH    4

/*  
 * print_file_names
 *   DESCRIPTION: early-stage test function to print file names
 *                did not use structs, simply calculate the offsets
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void print_file_names() {
    uint32_t dentry_idx = 0;
    uint8_t* dentry_addr = (uint8_t *) file_system_addr;
    //uint8_t file_name[32] = "frame0.txt";

    /* for each directory entry, print the current file name */
    for (dentry_idx = 0; dentry_idx < dentry_num; dentry_idx ++) {
        dentry_addr += 64;
        printf("Current file name: %s \n", dentry_addr);
    }

    while(1);
}

/*  
 * init_file_system
 *   DESCRIPTION: initialize the file system
 *                use the information from boot block
 *   INPUTS: fs_addr -- the starting address of the file system in memory
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_file_system(uint32_t fs_addr) {
    file_system_addr = fs_addr;
    dentry_num = * ((uint32_t *)file_system_addr);
    inode_num = * ((uint32_t *)file_system_addr + 1);
    block_num = * ((uint32_t *)file_system_addr + 2);

    file_array = NULL;
}

/*  
 * read_dentry_by_name
 *   DESCRIPTION: fill up the input dentry corresponding to fname
 *                search through boot block and check for fname
 *   INPUTS: fname -- file name we search for
 *           dentry -- input struct that we would like to update
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
    if (fname == NULL || dentry == NULL || strlen((int8_t*)fname) > 32)
        return -1;
    uint32_t dentry_idx = 0;
    dentry_t* dentry_addr = (dentry_t *)file_system_addr;

    /* search through boot block for a dentry with the input fname */
    for (dentry_idx = 0; dentry_idx < dentry_num; dentry_idx ++) {
        dentry_addr++;

        if (!strncmp((int8_t *)fname, (int8_t *)dentry_addr->file_name, NAME_LENGTH_MAX)) {
            *dentry = *dentry_addr;
            return 0;
        } 
    }
    return -1;
}

/*  
 * read_dentry_by_index
 *   DESCRIPTION: fill up the input dentry corresponding to the dentry index
 *   INPUTS: index -- directory index
 *           dentry -- input struct that we would like to update
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    /* check if index is out of range */
    if (index >= dentry_num || dentry == NULL)
        return -1;

    /* index + 1 to compensate for the boot block */
    dentry_t* dentry_addr = ((dentry_t*)file_system_addr + index + 1);
    *dentry = *dentry_addr;

    return 0;
}

/*  
 * get_file_size
 *   DESCRIPTION: get the file size
 *   INPUTS: dentry -- directory entry struct
 *   OUTPUTS: none
 *   RETURN VALUE: file size if successful
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t get_file_size(dentry_t * dentry) {
    if (dentry == NULL)
        return -1;
    uint32_t inode = dentry->inode_idx;
    if (inode >= inode_num)
        return -1;
    
    inode_t* inode_struct = ((inode_t*)file_system_addr + inode + 1);
    return inode_struct->length;
}


/*  
 * read_data
 *   DESCRIPTION: reading up to 'length' bytes of data from file system into buf
 *                data range: [offset, offset + length), may be in discrete data blocks
 *   INPUTS: inode -- inode index
 *           offset -- the offset for the starting byte
 *           buf -- input struct that we would like to fill
 *           length -- number of bytes we would like to read
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes successful read
 *                 0 if we reach the end of the inode
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, int32_t length) {
    // Check if the given inode is in the valid range
    if (inode >= inode_num || buf == NULL || length < 0) {
        return -1;
    }

    // Find the current inode struct
    inode_t* inode_struct = ((inode_t*)file_system_addr + inode + 1);

    uint32_t i;

    /*********** IMPORTANT ***********/
    /* If return earlier, also return the number of bytes read*/
    // i: index of byte in current inode's data
    for (i = offset; i < (offset + length); i++) {
        // Calculate the pointer to the current data struct
        uint32_t block_idx = *(inode_struct->block_idx + i / DATA_LENGTH);

        // Check if the block idx is in the valid range
        if (block_idx >= block_num) {
            return i - offset - 1;
        }

        data_block_t* data_struct = ((data_block_t*)file_system_addr + 1 + inode_num + block_idx);
        *(buf++) = *(data_struct->data + i % DATA_LENGTH);

        // If we reach the end of the inode 
        if (i == (inode_struct->length)){
            return i - offset;
        }
    }
    return i - offset;
}

/*  
 * program_loader
 *   DESCRIPTION: load a program into memory (virtual addr 128MB-132MB page)
 *   INPUTS: prog_dentry -- the program we want to load
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if not
 *   SIDE EFFECTS: none
 */
int32_t program_loader(dentry_t* prog_dentry) {
    int length = get_file_size(prog_dentry);
    if (length == -1)
        return -1;

    if (read_data(prog_dentry->inode_idx, 0, (uint8_t*)PROGRAM_DIRECTORY_VIRTUAL_ADDR, length) != 0)
        return -1;

    return 0;
}

/*  
 * parse_command
 *   DESCRIPTION: helper function that parse a command
 *   INPUTS: command -- space-separated sequence of words: [filename] [other arguments]
 *         filename -- parsed variable
 *           params -- parsed variable
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: has_params is set to 1 if program has parameters
 */
void parse_command(const uint8_t* command, uint8_t* filename, uint8_t* params){
    /* get the filename */
    int char_count = 0;
    while (*command == ' ')
        command++;
    while (*command != ' ' && *command != '\n' && *command != '\0') {
        if (char_count++ < NAME_LENGTH_MAX)
             *filename++ = *command++;
        else
            command++;
    }

    /* skip the spaces in between */
    while (*command == ' ')
        command++;

    /* get the params */
    while (*command != '\n' && *command != '\0')
        *params++ = *command++;

    *filename = '\0';
    *params = '\0';
}

/*  
 * check_validity
 *   DESCRIPTION: helper function that check if prog_dentry corresponds to a valid executable
 *   INPUTS: prog_dentry -- program directory entry that we want to check
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if valid
 *                 -1 if not
 *   SIDE EFFECTS: none
 */
int32_t check_validity(dentry_t* prog_dentry) {
    /* check file type */
    if (prog_dentry->file_type != 2) {  // 2 is regular file type
        return -1;
    }

    /* check magic header */
    uint8_t magic_buf[MAGIC_NUM_LENGTH];
    read_data(prog_dentry->inode_idx, 0, magic_buf, MAGIC_NUM_LENGTH);
    if (strncmp((int8_t*)magic_buf, (int8_t*)EXEC_HEAD, MAGIC_NUM_LENGTH) != 0)
        return -1;

    return 0;
}

/*  
 * file_open
 *   DESCRIPTION: sys call convention, set up the file abstraction struct when we open a file
 *   INPUTS: fd -- file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t file_open(int32_t fd) {
    /* fill in file abstraction entry */
    file_array[fd].file_position = 0;
    return 0;
}

/*  
 * file_close
 *   DESCRIPTION: sys call convention, do nothing
 *   INPUTS: fd -- file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t file_close(int32_t fd) {
    /* file usable array range: 2-7 */
    if (!file_array || fd < 2 || fd > 7){
        return -1;   
    }
    /* clear file array offset */
    file_array[fd].file_position = 0;
    return 0;
}

/*  
 * file_read
 *   DESCRIPTION: sys call convention, try to read nbytes from a file, fill in buf
 *                leverage read_data to fill in the buffer
 *   INPUTS: fd -- file descriptor
 *           buf -- the buffer we would like to fill
 *           nbytes -- number of bytes to read from the file
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes successfully read
 *                 0 if we reaches the end of the file
 *                 -1 if not successful
 *   SIDE EFFECTS: none
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {

    /* file usable array range: 2-7 */
    if (!file_array || fd < 2 || fd > 7){
        return -1;
    }
    if (buf == NULL || nbytes < 0)
        return 0;
    int ret_val;
    ret_val = read_data(file_array[fd].inode_idx, file_array[fd].file_position, buf, nbytes);

    /* increment file position if we have not reached the end of the file */
    /******* IMPORTANT !!! *********/
    /* Update the file position by actual bytes read instead of nbytes*/
    if (ret_val)
        file_array[fd].file_position += ret_val;

    return ret_val;
}

/*  
 * file_write
 *   DESCRIPTION: sys call convention, do nothing (read-only file system)
 *   INPUTS: fd -- file descriptor
 *           buf -- buffer
 *           nbytes -- bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
    fd = fd;
    buf = buf;
    nbytes = nbytes;
    return -1;
}

/*  
 * directory_open
 *   DESCRIPTION: sys call convention, set up information
 *   INPUTS: fd -- file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t directory_open(int32_t fd) {
    /* start from the first directory */
    file_array[fd].file_position = 0;
    file_array[fd].inode_idx = 0;
    return 0;
}

/*  
 * directory_close
 *   DESCRIPTION: sys call convention, do nothing
 *   INPUTS: fd -- file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t directory_close(int32_t fd) {
    fd = fd;
    return 0;
}

/* 
 * directory_read
 *   DESCRIPTION: sys call convention, read directory names one at a time
 *   INPUTS: fd -- file descriptor
 *           buf -- buffer we would like to fill
 *           nbytes -- number of bytes (idle)
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *                 -1 if not
 *   SIDE EFFECTS: none
 */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes) {
    /* check if there is more file to read */
    if(file_array[fd].file_position == dentry_num || buf == NULL)
        return 0;

    uint8_t* dentry_addr = (uint8_t *)file_system_addr;

    /* 64: size of a directory entry, 32: size of a file name */
    dentry_addr += 64 * (file_array[fd].file_position + 1);
    strncpy((int8_t *)buf, (int8_t *)dentry_addr, 32);
    file_array[fd].file_position++;
    /***** RETURN <= 32 instead of nbytes *****/
    int length = strlen((int8_t*)dentry_addr) > 32 ? 32 : strlen((int8_t*)dentry_addr);

    return length;
}

/*  
 * directory_write
 *   DESCRIPTION: sys call convention, do nothing (read-only file system)
 *   INPUTS: fd -- file descriptor
 *           buf -- buffer
 *           nbytes -- bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes) {
    fd = fd;
    buf = buf;
    nbytes = nbytes;
    return -1;
}

/*  
 * file_op_init
 *   DESCRIPTION: initiate operation pointer in kernel
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: setup operation pointer table
 */
void file_op_init() {
    regular_file_op_table.open = (void*)file_open;
    regular_file_op_table.close = (void*)file_close;
    regular_file_op_table.read = (void*)file_read;
    regular_file_op_table.write = (void*)file_write;

    dirt_op_table.open = (void*)directory_open;
    dirt_op_table.close = (void*)directory_close;
    dirt_op_table.read = (void*)directory_read;
    dirt_op_table.write = (void*)directory_write;
}
