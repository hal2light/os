#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/cache.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
    fs_device = block_get_role (BLOCK_FILESYS);
    if (fs_device == NULL)
        PANIC ("No file system device found, can't initialize file system.");

    cache_init ();
    inode_init ();
    free_map_init ();

    if (format) 
        do_format ();

    free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
    cache_flush ();
    free_map_close ();
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/* Helper function to get directory name and file name from path */
static void
get_dirname (const char *path, char *name)
{
    char *last_slash = strrchr (path, '/');
    if (last_slash != NULL)
        strlcpy (name, last_slash + 1, NAME_MAX + 1);
    else
        strlcpy (name, path, NAME_MAX + 1);
}

/* Creates a file or directory named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir)
{
    block_sector_t inode_sector = 0;
    struct dir *dir = dir_open_path (name);
    char file_name[NAME_MAX + 1];
    
    /* Get the directory name from the path */
    get_dirname (name, file_name);
    
    bool success = (dir != NULL
                   && free_map_allocate (1, &inode_sector)
                   && inode_create (inode_sector, initial_size, is_dir)
                   && dir_add (dir, file_name, inode_sector, is_dir));
    if (!success && inode_sector != 0)
        free_map_release (inode_sector, 1);
    dir_close (dir);

    return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer otherwise. */
struct file *
filesys_open (const char *name)
{
    struct dir *dir = dir_open_path (name);
    struct inode *inode = NULL;
    char file_name[NAME_MAX + 1];

    get_dirname (name, file_name);
    
    if (dir != NULL)
        dir_lookup (dir, file_name, &inode);
    dir_close (dir);

    return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure. */
bool
filesys_remove (const char *name) 
{
    struct dir *dir = dir_open_path (name);
    char file_name[NAME_MAX + 1];
    bool success = false;

    get_dirname (name, file_name);
    
    if (dir != NULL)
        success = dir_remove (dir, file_name);
    dir_close (dir);

    return success;
}

/* Creates a directory named NAME.
   Returns true if successful, false on failure. */
bool
filesys_create_dir (const char *name)
{
    return filesys_create (name, 16 * sizeof (struct dir_entry), true);
}

/* Changes the current working directory to NAME.
   Returns true if successful, false on failure. */
bool
filesys_chdir (const char *name)
{
    struct dir *dir = dir_open_path (name);
    if (dir == NULL)
        return false;

    dir_close (thread_current()->cwd);
    thread_current()->cwd = dir;
    return true;
}
