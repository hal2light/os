#include "filesys/fsutil.h"
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/disk.h"
#include "threads/malloc.h"

/* List files in the root directory. */
void
fsutil_ls (char **argv UNUSED) 
{
    struct dir *dir;
    char name[NAME_MAX + 1];

    dir = dir_open_root ();
    if (dir == NULL)
        PANIC ("root dir open failed");
    while (dir_readdir (dir, name))
        printf ("%s\n", name);
    dir_close (dir);
}

/* Copies a file from the host to PintOS. */
void
fsutil_cp (char **argv)
{
    const char *from = argv[1];
    const char *to = argv[2];
    struct file *src;
    struct file *dst;
    off_t size;
    void *buffer;

    printf ("Copying '%s' to '%s'...\n", from, to);

    /* Open source file. */
    src = filesys_open (from);
    if (src == NULL)
        PANIC ("%s: open failed", from);
    size = file_length (src);

    /* Create destination file. */
    if (!filesys_create (to, size, false))
        PANIC ("%s: create failed", to);
    dst = filesys_open (to);
    if (dst == NULL)
        PANIC ("%s: open failed", to);

    /* Copy block by block. */
    buffer = malloc (DISK_SECTOR_SIZE);
    if (buffer == NULL)
        PANIC ("malloc failed");
    while (size > 0) 
    {
        int chunk_size = size > DISK_SECTOR_SIZE ? DISK_SECTOR_SIZE : size;
        if (file_read (src, buffer, chunk_size) != chunk_size)
            PANIC ("%s: read failed with %"PROTd" bytes unread", from, size);
        if (file_write (dst, buffer, chunk_size) != chunk_size)
            PANIC ("%s: write failed", to);
        size -= chunk_size;
    }
    free (buffer);

    /* Close files. */
    file_close (dst);
    file_close (src);
}
