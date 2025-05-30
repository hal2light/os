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

static struct frame_entry *
frame_lookup(void *frame)
{
  struct list_elem *e;
  for (e = list_begin(&frame_list); e != list_end(&frame_list); e = list_next(e)) {
    struct frame_entry *entry = list_entry(e, struct frame_entry, elem);
    if (entry->frame == frame)
      return entry;
  }
  return NULL;
}

static void* 
frame_evict(void)
{
  size_t tries = 0;
  struct frame_entry *victim = NULL;

  while (tries < 2 * list_size(&frame_list)) {
    if (clock_hand == NULL || clock_hand == list_end(&frame_list))
      clock_hand = list_begin(&frame_list);

    victim = list_entry(clock_hand, struct frame_entry, elem);
    clock_hand = list_next(clock_hand);
    
    if (!victim->pinned && victim->page != NULL) {
      struct thread *t = thread_current();
      
      if (!pagedir_is_accessed(t->pagedir, victim->page->addr)) {
        void *frame = victim->frame;
        
        if (page_out(victim->page)) {
          list_remove(&victim->elem);
          free(victim);
          return frame;
        }
      } else {
        pagedir_set_accessed(t->pagedir, victim->page->addr, false);
      }
    }
    tries++;
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
