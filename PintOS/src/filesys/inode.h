#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"

#define DIRECT_BLOCKS 123
#define INDIRECT_BLOCKS_PER_SECTOR 128

struct inode_disk {
    block_sector_t direct_blocks[DIRECT_BLOCKS];
    block_sector_t indirect_block;
    block_sector_t doubly_indirect_block;
    off_t length;
    bool is_dir;
    unsigned magic;
};

void inode_init(void);
bool inode_create(block_sector_t, off_t, bool);
struct inode *inode_open(block_sector_t);
void inode_close(struct inode *);
bool inode_is_dir(const struct inode *);

#endif /* filesys/inode.h */
