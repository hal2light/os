#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"

/* An open file. */
struct file 
{
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
};

/* Opens a file for the given INODE, of which it takes ownership.
   Returns the new file if successful or a null pointer if an
   allocation fails. */
struct file *
file_open (struct inode *inode) 
{
    struct file *file = calloc (1, sizeof *file);
    if (inode != NULL && file != NULL)
    {
        file->inode = inode;
        file->pos = 0;
        file->deny_write = false;
        return file;
    }
    else
    {
        inode_close (inode);
        free (file);
        return NULL;
    }
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at the file's current position.
   Returns the number of bytes actually read. */
off_t
file_read (struct file *file, void *buffer, off_t size) 
{
    off_t bytes_read = inode_read_at (file->inode, buffer, size, file->pos);
    file->pos += bytes_read;
    return bytes_read;
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at the file's current position.
   Returns the number of bytes actually written. */
off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
    off_t bytes_written = inode_write_at (file->inode, buffer, size, file->pos);
    file->pos += bytes_written;
    return bytes_written;
}

/* Sets the current position in FILE to NEW_POS bytes from the
   start of the file. */
void
file_seek (struct file *file, off_t new_pos)
{
    ASSERT (file != NULL);
    file->pos = new_pos;
}

/* Returns the current position in FILE as a byte offset from the
   start of the file. */
off_t
file_tell (struct file *file) 
{
    ASSERT (file != NULL);
    return file->pos;
}

/* Closes FILE. */
void
file_close (struct file *file) 
{
    if (file != NULL)
    {
        file_allow_write (file);
        inode_close (file->inode);
        free (file);
    }
}
