#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "devices/block.h"
#include "filesys/off_t.h"

enum page_status {
    PAGE_MEMORY,     /* Page is in memory */
    PAGE_FILE,       /* Page is in file */
    PAGE_SWAP        /* Page is in swap */
};

struct page {
    void *addr;                  /* User virtual address */
    bool writable;               /* True if writable */
    bool loaded;                 /* True if loaded */
    struct hash_elem hash_elem;  /* Hash table element */
    
    enum page_status status;     /* Location of page */
    struct file *file;          /* File for page */
    off_t file_offset;          /* Offset in file */
    size_t file_bytes;          /* Bytes to read/write */
    
    size_t swap_slot;           /* Swap slot if swapped */
    struct frame_entry *frame;  /* Frame containing page */
};

void page_init(struct hash *);
bool page_in(struct page *);
bool page_out(struct page *);
void page_free(struct page *);
struct page* page_lookup(void *vaddr);
bool page_insert(struct hash *spt, struct page *page);
void page_table_destroy(struct hash *spt);

#endif
