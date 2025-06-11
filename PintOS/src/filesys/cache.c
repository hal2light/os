#include "filesys/cache.h"
#include "devices/block.h"
#include "threads/synch.h"
#include <string.h>

#define CACHE_SIZE 64

struct cache_entry {
    block_sector_t sector;      /* Sector number */
    uint8_t data[BLOCK_SECTOR_SIZE]; /* Cached data */
    bool valid;                 /* Is entry valid? */
    bool dirty;                 /* Has entry been modified? */
    bool accessed;             /* Has entry been accessed recently? */
    struct lock lock;          /* Entry lock */
};

static struct cache_entry cache[CACHE_SIZE];
static struct lock cache_lock;

void
cache_init(void)
{
    lock_init(&cache_lock);
    for (int i = 0; i < CACHE_SIZE; i++) 
    {
        cache[i].valid = false;
        lock_init(&cache[i].lock);
    }
}

static int
cache_find(block_sector_t sector)
{
    for (int i = 0; i < CACHE_SIZE; i++)
        if (cache[i].valid && cache[i].sector == sector)
            return i;
    return -1;
}

static int
cache_evict(void)
{
    static int clock = 0;
    while (true)
    {
        if (!cache[clock].valid)
            return clock;
        if (!cache[clock].accessed)
        {
            if (cache[clock].dirty)
                block_write(fs_device, cache[clock].sector, cache[clock].data);
            return clock;
        }
        cache[clock].accessed = false;
        clock = (clock + 1) % CACHE_SIZE;
    }
}

void
cache_read(block_sector_t sector, void *buffer)
{
    lock_acquire(&cache_lock);
    int index = cache_find(sector);

    if (index < 0)
    {
        index = cache_evict();
        cache[index].sector = sector;
        cache[index].valid = true;
        cache[index].dirty = false;
        block_read(fs_device, sector, cache[index].data);
    }

    cache[index].accessed = true;
    memcpy(buffer, cache[index].data, BLOCK_SECTOR_SIZE);
    lock_release(&cache_lock);
}

void
cache_write(block_sector_t sector, const void *buffer)
{
    lock_acquire(&cache_lock);
    int index = cache_find(sector);

    if (index < 0)
    {
        index = cache_evict();
        cache[index].sector = sector;
        cache[index].valid = true;
    }

    cache[index].accessed = true;
    cache[index].dirty = true;
    memcpy(cache[index].data, buffer, BLOCK_SECTOR_SIZE);
    lock_release(&cache_lock);
}

void
cache_flush(void)
{
    lock_acquire(&cache_lock);
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (cache[i].valid && cache[i].dirty)
        {
            block_write(fs_device, cache[i].sector, cache[i].data);
            cache[i].dirty = false;
        }
    }
    lock_release(&cache_lock);
}