#include "vm/swap.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

static struct bitmap *swap_bitmap;
static struct block *swap_block;
static struct lock swap_lock;

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

void
swap_init(void)
{
  swap_block = block_get_role(BLOCK_SWAP);
  if (swap_block == NULL) 
    PANIC("No swap device found!");
    
  swap_bitmap = bitmap_create(block_size(swap_block) / SECTORS_PER_PAGE);
  if (swap_bitmap == NULL)
    PANIC("Failed to create swap bitmap!");
    
  lock_init(&swap_lock);
}

size_t
swap_out(void *frame)
{
  lock_acquire(&swap_lock);
  size_t slot = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
  if (slot == BITMAP_ERROR)
    PANIC("Swap partition full!");

  for (size_t i = 0; i < SECTORS_PER_PAGE; i++) {
    block_write(swap_block, 
                slot * SECTORS_PER_PAGE + i,
                frame + i * BLOCK_SECTOR_SIZE);
  }
  
  lock_release(&swap_lock);
  return slot;
}

void
swap_in(size_t slot, void *frame)
{
  lock_acquire(&swap_lock);
  
  if (!bitmap_test(swap_bitmap, slot))
    PANIC("Trying to swap in unused slot!");
    
  for (size_t i = 0; i < SECTORS_PER_PAGE; i++) {
    block_read(swap_block,
               slot * SECTORS_PER_PAGE + i,
               frame + i * BLOCK_SECTOR_SIZE);
  }
  
  bitmap_flip(swap_bitmap, slot);
  lock_release(&swap_lock);
}

void
swap_free(size_t slot)
{
  lock_acquire(&swap_lock);
  bitmap_reset(swap_bitmap, slot);
  lock_release(&swap_lock);
}
