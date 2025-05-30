#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/palloc.h"
#include "threads/synch.h"

struct frame_entry {
    void *frame;              /* Kernel virtual address */
    struct page *page;        /* Page mapped to frame */
    struct list_elem elem;    /* List element */
    bool pinned;             /* Whether frame is pinned */
};

void frame_init(void);
void* frame_allocate(enum palloc_flags);
void frame_free(void*);
void frame_pin(void*);
void frame_unpin(void*);

#endif
