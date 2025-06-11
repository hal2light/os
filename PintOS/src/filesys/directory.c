#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"

/* A directory. */
struct dir 
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

/* A single directory entry. */
struct dir_entry 
{
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
    bool is_dir;                        /* Is this a directory? */
};

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR. Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
    bool success = inode_create (sector, entry_cnt * sizeof (struct dir_entry), true);
    if (success) 
    {
        struct dir *dir = dir_open(inode_open(sector));
        if (dir != NULL) 
        {
            /* Add "." and ".." entries */
            success = dir_add(dir, ".", sector, true);
            if (success)
                success = dir_add(dir, "..", sector, true);
            dir_close(dir);
        }
    }
    return success;
}

/* Opens the directory for the given INODE. */
struct dir *
dir_open (struct inode *inode) 
{
    struct dir *dir = calloc (1, sizeof *dir);
    if (inode != NULL && dir != NULL) 
    {
        dir->inode = inode;
        dir->pos = 0;
        return dir;
    }
    free (dir);
    return NULL;
}

/* Opens and returns a new directory for the same inode as DIR. */
struct dir *
dir_reopen (struct dir *dir) 
{
    return dir_open (inode_reopen (dir->inode));
}

/* Opens the root directory and returns a directory for it. */
struct dir *
dir_open_root (void) 
{
    return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens the directory specified by PATH. Returns NULL if PATH
   cannot be traversed. */
struct dir *
dir_open_path (const char *path)
{
    struct dir *dir;
    struct inode *inode = NULL;
    
    if (path[0] == '/')
        dir = dir_open_root ();
    else
        dir = dir_open (inode_reopen (thread_current()->cwd));
        
    char *token, *save_ptr;
    char *path_copy = malloc(strlen(path) + 1);
    strlcpy(path_copy, path, strlen(path) + 1);
    
    for (token = strtok_r(path_copy, "/", &save_ptr); 
         token != NULL;
         token = strtok_r(NULL, "/", &save_ptr)) 
    {
        if (!dir_lookup (dir, token, &inode)) 
        {
            dir_close (dir);
            free(path_copy);
            return NULL;
        }
        struct dir *next = dir_open (inode);
        dir_close (dir);
        dir = next;
    }
    
    free(path_copy);
    return dir;
}

/* Extracts a file name part from *SRCP into PART, and updates *SRCP so that the
   next call will return the next file name part. Returns 1 if successful, 0 at
   end of string, -1 for a too-long file name part. */
char *
get_next_part (char *part, const char **srcp)
{
    const char *src = *srcp;
    char *dst = part;

    // Skip leading slashes
    while (*src == '/')
        src++;

    // Copy up to NAME_MAX characters
    while (*src != '/' && *src != '\0') 
    {
        if (dst < part + NAME_MAX)
            *dst++ = *src;
        else
            return NULL;
        src++;
    }
    *dst = '\0';
    
    *srcp = src;
    return part;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (e.in_use && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;

  return *inode != NULL;
}

/* Adds a file or directory named NAME to DIR. The file's inode is in sector
   INODE_SECTOR. Returns true if successful, false on failure. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector, bool is_dir)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* Set OFS to offset of free slot. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  e.is_dir = is_dir;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

  if (success && is_dir)
  {
    /* Create "." and ".." entries for the new directory */
    struct dir *new_dir = dir_open(inode_open(inode_sector));
    if (new_dir != NULL)
    {
      /* Add "." entry pointing to itself */
      dir_add(new_dir, ".", inode_sector, true);
      
      /* Add ".." entry pointing to parent */
      dir_add(new_dir, "..", inode_get_inumber(dir->inode), true);
      
      dir_close(new_dir);
    }
  }

 done:
  return success;
}

/* Removes NAME from DIR. Returns true if successful, false if
   the directory entry was not found or if it is a non-empty directory. */
bool
dir_remove (struct dir *dir, const char *name)
{
    struct dir_entry e;
    struct inode *inode = NULL;
    bool success = false;
    off_t ofs;

    ASSERT (dir != NULL);
    ASSERT (name != NULL);

    /* Find directory entry. */
    if (!dir_lookup (dir, name, &e, &ofs))
        goto done;

    /* If it's a directory, check if it's empty */
    if (e.is_dir)
    {
        struct dir *subdir = dir_open (inode_open (e.inode_sector));
        if (subdir != NULL)
        {
            char temp[NAME_MAX + 1];
            for (ofs = 0; inode_read_at (subdir->inode, &temp, sizeof temp, ofs) == sizeof temp; 
                 ofs += sizeof temp)
            {
                if (strcmp(temp, ".") != 0 && strcmp(temp, "..") != 0)
                {
                    dir_close (subdir);
                    goto done;
                }
            }
            dir_close (subdir);
        }
    }

    /* Erase directory entry. */
    e.in_use = false;
    if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e)
        goto done;

    /* Remove inode. */
    inode = inode_open (e.inode_sector);
    if (inode != NULL)
    {
        inode_remove (inode);
        success = true;
    }

done:
    inode_close (inode);
    return success;
}

/* Returns true if the directory is empty. */
bool
dir_is_empty (struct dir *dir)
{
    struct dir_entry e;
    off_t ofs;

    for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
         ofs += sizeof e)
    {
        if (e.in_use && strcmp(e.name, ".") != 0 && strcmp(e.name, "..") != 0)
            return false;
    }
    return true;
}

/* Reads the next directory entry in DIR and stores the name in NAME.
   Returns true if successful, false if no entries are left. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
    struct dir_entry e;

    while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e)
    {
        dir->pos += sizeof e;
        if (e.in_use && strcmp(e.name, ".") != 0 && strcmp(e.name, "..") != 0)
        {
            strlcpy (name, e.name, NAME_MAX + 1);
            return true;
        }
    }
    return false;
}

bool
filesys_create_dir (const char *path)
{
  char *dir_path = malloc (strlen (path) + 1);
  char *last_part = malloc (NAME_MAX + 1);
  struct dir *dir;
  bool success = false;

  /* Split path into directory and last component */
  get_dir_and_name (path, dir_path, last_part);

  /* Open parent directory */
  dir = dir_open_path (dir_path);
  if (dir == NULL)
    goto done;

  /* Create the directory */
  block_sector_t inode_sector;
  if (!free_map_allocate (1, &inode_sector))
    goto done;

  if (!dir_create (inode_sector, 16))
    goto done;

  /* Add it to the parent directory */
  success = dir_add (dir, last_part, inode_sector, true);
  if (!success)
    goto done;

  /* Add "." and ".." entries */
  struct dir *new_dir = dir_open (inode_open (inode_sector));
  if (new_dir != NULL)
  {
    success = dir_add (new_dir, ".", inode_sector, true);
    if (success)
      success = dir_add (new_dir, "..", inode_get_inumber (dir->inode), true);
    dir_close (new_dir);
  }

done:
  dir_close (dir);
  free (dir_path);
  free (last_part);
  return success;
}
