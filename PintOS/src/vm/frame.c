#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include <list.h>

static struct list frame_list;
static struct lock frame_lock;

// Clock hand for eviction
static struct list_elem *clock_hand;

void
frame_init(void) 
{
  list_init(&frame_list);
  lock_init(&frame_lock);
  clock_hand = NULL;
}

void*
frame_allocate(enum palloc_flags flags) 
{
  lock_acquire(&frame_lock);
  
  void *frame = palloc_get_page(flags);
  if (frame != NULL) {
    struct frame_entry *entry = malloc(sizeof(struct frame_entry));
    if (entry) {
      entry->frame = frame;
      entry->page = NULL;
      entry->pinned = false;
      list_push_back(&frame_list, &entry->elem);
    }
  } else {
    // Need to evict
    frame = frame_evict();
  }
  
  lock_release(&frame_lock);
  return frame;
}

void 
frame_free(void *frame)
{
  lock_acquire(&frame_lock);
  
  struct list_elem *e;
  for (e = list_begin(&frame_list); e != list_end(&frame_list); e = list_next(e)) {
    struct frame_entry *entry = list_entry(e, struct frame_entry, elem);
    if (entry->frame == frame) {
      list_remove(&entry->elem);
      palloc_free_page(frame);
      free(entry);
      break;
    }
  }
  
  lock_release(&frame_lock);
}

static void* 
frame_evict(void)
{
  struct frame_entry *victim = NULL;
  struct list_elem *e = clock_hand;
  
  if (e == NULL)
    e = list_begin(&frame_list);
    
  while (true) {
    if (e == list_end(&frame_list))
      e = list_begin(&frame_list);
      
    victim = list_entry(e, struct frame_entry, elem);
    if (!victim->pinned) {
      // Check accessed bit
      if (!pagedir_is_accessed(victim->page->owner->pagedir, victim->page->addr)) {
        // Found victim
        break;
      }
      // Clear accessed bit and continue
      pagedir_set_accessed(victim->page->owner->pagedir, victim->page->addr, false);
    }
    e = list_next(e);
  }
  
  clock_hand = list_next(e);
  
  if (victim != NULL) {
    void *frame = victim->frame;
    if (victim->page != NULL) {
      page_out(victim->page);
    }
    list_remove(&victim->elem);
    free(victim);
    return frame;
  }
  
  return NULL;
}

void
frame_pin(void *frame)
{
  lock_acquire(&frame_lock);
  struct frame_entry *entry = frame_lookup(frame);
  if (entry != NULL)
    entry->pinned = true;
  lock_release(&frame_lock);
}

void
frame_unpin(void *frame) 
{
  lock_acquire(&frame_lock);
  struct frame_entry *entry = frame_lookup(frame);
  if (entry != NULL)
    entry->pinned = false;
  lock_release(&frame_lock);
}
