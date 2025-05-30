#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"

static unsigned page_hash(const struct hash_elem *e, void *aux);
static bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
static void page_destructor(struct hash_elem *e, void *aux);

void 
page_init(struct hash *spt)
{
  hash_init(spt, page_hash, page_less, NULL);
}

bool
page_in(struct page *page)
{
  void *frame = frame_allocate(PAL_USER);
  if (frame == NULL)
    return false;
    
  // Pin the frame while we work with it
  frame_pin(frame);
  
  bool success = false;
  switch (page->status) {
    case PAGE_FILE:
      // Load from file
      file_seek(page->file, page->file_offset);
      if (file_read(page->file, frame, page->file_bytes) 
          == (int) page->file_bytes) {
        // Zero the rest of the page
        if (page->file_bytes < PGSIZE)
          memset(frame + page->file_bytes, 0, PGSIZE - page->file_bytes);
        success = true;
      }
      break;
      
    case PAGE_SWAP:
      swap_in(page->swap_slot, frame);
      success = true;
      break;
      
    default:
      break;
  }
  
  if (success && install_page(page->addr, frame, page->writable)) {
    page->frame = frame;
    page->loaded = true;
    page->status = PAGE_MEMORY;
    frame_unpin(frame);
    return true;
  }
  
  frame_unpin(frame);
  frame_free(frame);
  return false;
}

bool
page_out(struct page *page)
{
  bool success = false;
  
  switch (page->status) {
    case PAGE_MEMORY:
      if (pagedir_is_dirty(thread_current()->pagedir, page->addr)) {
        if (page->file != NULL) {
          // Write back to file
          file_write_at(page->file, page->addr, page->file_bytes, page->file_offset);
        } else {
          // Write to swap
          page->swap_slot = swap_out(page->addr);
          page->status = PAGE_SWAP;
        }
      }
      success = true;
      break;
      
    default:
      break;
  }
  
  if (success) {
    frame_free(page->frame);
    page->frame = NULL;
  }
  
  return success;
}

void
page_free(struct page *page)
{
  if (page->status == PAGE_SWAP)
    swap_free(page->swap_slot);
  free(page);
}

struct page*
page_lookup(void *vaddr) 
{
  struct thread *t = thread_current();
  struct page p;
  struct hash_elem *e;

  p.addr = pg_round_down(vaddr);
  e = hash_find(&t->spt, &p.hash_elem);
  return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

bool
page_insert(struct hash *spt, struct page *page) 
{
  return hash_insert(spt, &page->hash_elem) == NULL;
}

void 
page_table_destroy(struct hash *spt)
{
  hash_destroy(spt, page_destructor);
}

static void 
page_destructor(struct hash_elem *e, void *aux UNUSED)
{
  struct page *p = hash_entry(e, struct page, hash_elem);
  page_free(p);
}

static unsigned
page_hash(const struct hash_elem *e, void *aux UNUSED)
{
  const struct page *p = hash_entry(e, struct page, hash_elem);
  return hash_bytes(&p->addr, sizeof p->addr);
}

static bool
page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  const struct page *pa = hash_entry(a, struct page, hash_elem);
  const struct page *pb = hash_entry(b, struct page, hash_elem);
  return pa->addr < pb->addr;
}
