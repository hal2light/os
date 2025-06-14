#include "filesys/inode.h"
#include "filesys/cache.h"
#include "filesys/free-map.h"
#include <debug.h>
#include <round.h>
#include <string.h>

/* Number of direct blocks in inode */
#define DIRECT_BLOCKS 123
/* Number of block pointers per indirect block */
#define INDIRECT_BLOCKS_PER_SECTOR (BLOCK_SECTOR_SIZE / sizeof(block_sector_t))

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
    block_sector_t direct_blocks[DIRECT_BLOCKS];     /* Direct block pointers */
    block_sector_t indirect_block;                   /* Single indirect block */
    block_sector_t doubly_indirect_block;           /* Doubly indirect block */
    off_t length;                                   /* File size in bytes */
    bool is_dir;                                    /* True if directory */
    unsigned magic;                                 /* Magic number */
    uint32_t unused[125];                          /* Not used */
};

/* Returns the number of sectors to allocate for an inode SIZE bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
    return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* Extends the file to the desired length by allocating new blocks */
static bool
inode_extend (struct inode_disk *disk_inode, off_t length)
{
    static char zeros[BLOCK_SECTOR_SIZE];
    size_t num_sectors = bytes_to_sectors (length);
    size_t old_sectors = bytes_to_sectors (disk_inode->length);

    /* Direct blocks */
    for (size_t i = old_sectors; i < num_sectors && i < DIRECT_BLOCKS; i++)
    {
        if (!free_map_allocate (1, &disk_inode->direct_blocks[i]))
            return false;
        cache_write (disk_inode->direct_blocks[i], zeros);
    }

    if (num_sectors <= DIRECT_BLOCKS)
    {
        disk_inode->length = length;
        return true;
    }

    /* Allocate indirect block if needed */
    if (old_sectors <= DIRECT_BLOCKS)
    {
        if (!free_map_allocate (1, &disk_inode->indirect_block))
            return false;
        cache_write (disk_inode->indirect_block, zeros);
    }

    /* Allocate blocks through indirect block */
    block_sector_t indirect[INDIRECT_BLOCKS_PER_SECTOR];
    size_t indirect_index = old_sectors > DIRECT_BLOCKS ? 
                           old_sectors - DIRECT_BLOCKS : 0;

    cache_read (disk_inode->indirect_block, indirect);
    while (num_sectors > DIRECT_BLOCKS && 
           indirect_index < INDIRECT_BLOCKS_PER_SECTOR)
    {
        if (!free_map_allocate (1, &indirect[indirect_index]))
            return false;
        cache_write (indirect[indirect_index], zeros);
        indirect_index++;
        num_sectors--;
    }
    cache_write (disk_inode->indirect_block, indirect);
    disk_inode->length = length;
    return true;
}

/* Helper function to get the sector number for a given position */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
    if (pos >= inode->data.length)
        return -1;

    size_t sector_idx = pos / BLOCK_SECTOR_SIZE;
    
    /* Direct blocks */
    if (sector_idx < DIRECT_BLOCKS)
        return inode->data.direct_blocks[sector_idx];
    
    sector_idx -= DIRECT_BLOCKS;
    
    /* Indirect blocks */
    if (sector_idx < INDIRECT_BLOCKS_PER_SECTOR)
    {
        block_sector_t indirect[INDIRECT_BLOCKS_PER_SECTOR];
        cache_read(inode->data.indirect_block, indirect);
        return indirect[sector_idx];
    }
    
    /* Doubly indirect blocks */
    sector_idx -= INDIRECT_BLOCKS_PER_SECTOR;
    if (sector_idx < INDIRECT_BLOCKS_PER_SECTOR * INDIRECT_BLOCKS_PER_SECTOR)
    {
        block_sector_t doubly_indirect[INDIRECT_BLOCKS_PER_SECTOR];
        cache_read(inode->data.doubly_indirect_block, doubly_indirect);
        
        block_sector_t indirect[INDIRECT_BLOCKS_PER_SECTOR];
        cache_read(doubly_indirect[sector_idx / INDIRECT_BLOCKS_PER_SECTOR], indirect);
        
        return indirect[sector_idx % INDIRECT_BLOCKS_PER_SECTOR];
    }
    
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      if (free_map_allocate (sectors, &disk_inode->start)) 
        {
          block_write (fs_device, sector, disk_inode);
          if (sectors > 0) 
            {
              static char zeros[BLOCK_SECTOR_SIZE];
              size_t i;
              
              for (i = 0; i < sectors; i++) 
                block_write (fs_device, disk_inode->start + i, zeros);
            }
          success = true; 
        } 
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          free_map_release (inode->data.start,
                            bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
    
  while (size > 0) 
  {
        /* Calculate sector to read, starting byte offset within sector. */
        block_sector_t sector_idx = byte_to_sector (inode, offset);
        int sector_ofs = offset % BLOCK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length (inode) - offset;
        int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually copy out of this sector. */
        int chunk_size = size < min_left ? size : min_left;
        if (chunk_size <= 0)
            break;

        cache_read (sector_idx, buffer + bytes_read);

        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_read += chunk_size;
  }

    return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if an error occurs. */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset) 
{
    const uint8_t *buffer = buffer_;
    off_t bytes_written = 0;

    if (inode->deny_write_cnt)
        return 0;

    /* Extend file if necessary */
    if (offset + size > inode->data.length)
    {
        if (!inode_extend(&inode->data, offset + size))
            return 0;
        cache_write(inode->sector, &inode->data);
    }

    while (size > 0) 
    {
        block_sector_t sector_idx = byte_to_sector(inode, offset);
        int sector_ofs = offset % BLOCK_SECTOR_SIZE;
        int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
        int chunk_size = size < sector_left ? size : sector_left;

        if (chunk_size <= 0)
            break;

        if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
            cache_write(sector_idx, buffer + bytes_written);
        else 
        {
            uint8_t *bounce = malloc(BLOCK_SECTOR_SIZE);
            if (bounce == NULL)
                break;
            
            cache_read(sector_idx, bounce);
            memcpy(bounce + sector_ofs, buffer + bytes_written, chunk_size);
            cache_write(sector_idx, bounce);
            free(bounce);
        }

        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_written += chunk_size;
    }

    return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
    inode->deny_write_cnt++;
    ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
    ASSERT (inode->deny_write_cnt > 0);
    ASSERT (inode->deny_write_cnt <= inode->open_cnt);
    inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
    return inode->data.length;
}

/* Returns true if INODE is a directory. */
bool
inode_is_dir (const struct inode *inode)
{
    return inode->data.is_dir;
}
