#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include <stdlib.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/directory.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/malloc.h"

// Function prototypes
static void syscall_handler(struct intr_frame *);
void exit_proc(int status);
int exec_proc(char *file_name);
void close_file(struct list* files, int fd);
void close_all_files(struct list* files);
void acquire_filesys_lock(void);
void release_filesys_lock(void);
bool sys_chdir(const char *dir);
bool sys_mkdir(const char *dir);
bool sys_readdir(int fd, char *name);
bool sys_isdir(int fd);
block_sector_t sys_inumber(int fd);

static void
syscall_handler(struct intr_frame *f)
{
    int *p = f->esp;
    int syscall_nr = *p;
    
    // Validate pointer before using
    if (!is_user_vaddr(p) || !is_user_vaddr(p + 1) || !is_user_vaddr(p + 2))
        exit_proc(-1);

    // Declare arguments at the start
    const char *arg1_str;
    char *arg2_str;
    int arg1, arg2;

    switch (syscall_nr)
    {
        case SYS_HALT:
            shutdown_power_off();
            break;

        case SYS_EXIT:
            check_addr(p+1);
            exit_proc(*(p+1));
            break;

        case SYS_EXEC:
            check_addr(p+1);
            check_addr(*(p+1));
            f->eax = exec_proc(*(p+1));
            break;

        case SYS_WAIT:
            check_addr(p+1);
            f->eax = process_wait(*(p+1));
            break;

        case SYS_CREATE:
            check_addr(p + 4);
            acquire_filesys_lock();
            f->eax = filesys_create((const char *)*(p + 4), *(p + 5), false);
            release_filesys_lock();
            break;

        case SYS_REMOVE:
            check_addr(p+1);
            check_addr(*(p+1));
            acquire_filesys_lock();
            if(filesys_remove(*(p+1))==NULL)
                f->eax = false;
            else
                f->eax = true;
            release_filesys_lock();
            break;

        case SYS_OPEN:
            check_addr(p + 1);
            acquire_filesys_lock();
            struct file* fptr = filesys_open((const char *)*(p + 1));
            release_filesys_lock();
            if(fptr==NULL)
                f->eax = -1;
            else
            {
                struct proc_file *pfile = malloc(sizeof(*pfile));
                pfile->ptr = fptr;
                pfile->fd = thread_current()->fd_count;
                thread_current()->fd_count++;
                list_push_back (&thread_current()->files, &pfile->elem);
                f->eax = pfile->fd;

            }
            break;

        case SYS_FILESIZE:
            check_addr(p+1);
            acquire_filesys_lock();
            f->eax = file_length (list_search(&thread_current()->files, *(p+1))->ptr);
            release_filesys_lock();
            break;

        case SYS_READ:
            check_addr(p+7);
            check_addr(*(p+6));
            if(*(p+5)==0)
            {
                int i;
                uint8_t* buffer = *(p+6);
                for(i=0;i<*(p+7);i++)
                    buffer[i] = input_getc();
                f->eax = *(p+7);
            }
            else
            {
                struct proc_file* fptr = list_search(&thread_current()->files, *(p+5));
                iffptr==NULL)
                    f->eax=-1;
                else
                {
                    acquire_filesys_lock();
                    f->eax = file_read (fptr->ptr, *(p+6), *(p+7));
                    release_filesys_lock();
                }
            }
            break;

        case SYS_WRITE:
            check_addr(p+7);
            check_addr(*(p+6));
            if(*(p+5)==1)
            {
                putbuf(*(p+6),*(p+7));
                f->eax = *(p+7);
            }
            else
            {
                struct proc_file* fptr = list_search(&thread_current()->files, *(p+5));
                if(fptr==NULL)
                    f->eax=-1;
                else
                {
                    acquire_filesys_lock();
                    f->eax = file_write (fptr->ptr, *(p+6), *(p+7));
                    release_filesys_lock();
                }
            }
            break;

        case SYS_SEEK:
            check_addr(p+5);
            acquire_filesys_lock();
            file_seek(list_search(&thread_current()->files, *(p+4))->ptr,*(p+5));
            release_filesys_lock();
            break;

        case SYS_TELL:
            check_addr(p+1);
            acquire_filesys_lock();
            f->eax = file_tell(list_search(&thread_current()->files, *(p+1))->ptr);
            release_filesys_lock();
            break;

        case SYS_CLOSE:
            check_addr(p+1);
            acquire_filesys_lock();
            close_file(&thread_current()->files,*(p+1));
            release_filesys_lock();
            break;

        case SYS_CHDIR:
            check_addr(p + 1);
            arg1_str = (const char *)*(p + 1);
            f->eax = sys_chdir(arg1_str);
            break;

        case SYS_MKDIR:
            check_addr(p + 1);
            arg1_str = (const char *)*(p + 1);
            f->eax = sys_mkdir(arg1_str);
            break;

        case SYS_READDIR:
            check_addr(p + 1);
            check_addr(p + 2);
            arg1 = *(p + 1);
            arg2_str = (char *)*(p + 2);
            f->eax = sys_readdir(arg1, arg2_str);
            break;

        case SYS_ISDIR:
            check_addr(p + 1);
            arg1 = *(p + 1);
            f->eax = sys_isdir(arg1);
            break;

        case SYS_INUMBER:
            check_addr(p + 1);
            arg1 = *(p + 1);
            f->eax = sys_inumber(arg1);
            break;


        default:
            printf("Default %d\n",*p);
    }
}

int exec_proc(char *file_name)
{
    acquire_filesys_lock();
    char * fn_cp = malloc (strlen(file_name)+1);
      strlcpy(fn_cp, file_name, strlen(file_name)+1);
      
      char * save_ptr;
      fn_cp = strtok_r(fn_cp," ",&save_ptr);

     struct file* f = filesys_open (fn_cp);

      if(f==NULL)
      {
        release_filesys_lock();
        return -1;
      }
      else
      {
        file_close(f);
        release_filesys_lock();
        return process_execute(file_name);
      }
}

void exit_proc(int status)
{
    //printf("Exit : %s %d %d\n",thread_current()->name, thread_current()->tid, status);
    struct list_elem *e;

      for (e = list_begin (&thread_current()->parent->child_proc); e != list_end (&thread_current()->parent->child_proc);
           e = list_next (e))
        {
          struct child *f = list_entry (e, struct child, elem);
          if(f->tid == thread_current()->tid)
          {
            f->used = true;
            f->exit_error = status;
          }
        }


    thread_current()->exit_error = status;

    if(thread_current()->parent->waitingon == thread_current()->tid)
        sema_up(&thread_current()->parent->child_lock);

    thread_exit();
}

void* check_addr(const void *vaddr)
{
    if (!is_user_vaddr(vaddr))
    {
        exit_proc(-1);
        return 0;
    }
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (!ptr)
    {
        exit_proc(-1);
        return 0;
    }
    return ptr;
}

struct proc_file* list_search(struct list* files, int fd)
{

    struct list_elem *e;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct proc_file *f = list_entry (e, struct proc_file, elem);
          if(f->fd == fd)
            return f;
        }
   return NULL;
}

void close_file(struct list* files, int fd)
{

    struct list_elem *e;

    struct proc_file *f;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          f = list_entry (e, struct proc_file, elem);
          if(f->fd == fd)
          {
              file_close(f->ptr);
              list_remove(e);
          }
        }

    free(f);
}

void close_all_files(struct list* files)
{

    struct list_elem *e;

    while(!list_empty(files))
    {
        e = list_pop_front(files);

        struct proc_file *f = list_entry (e, struct proc_file, elem);
          
              file_close(f->ptr);
              list_remove(e);
              free(f);


    }

      
}

// Add implementations for filesystem syscalls
bool
sys_chdir(const char *dir)
{
    if (dir == NULL)
        return false;
    return filesys_chdir(dir);
}

bool
sys_mkdir(const char *dir)
{
    if (dir == NULL)
        return false;
    return filesys_create(dir, 0, true);
}

bool
sys_readdir(int fd, char *name)
{
    struct proc_file *pf = list_search(&thread_current()->files, fd);
    if (pf == NULL || !inode_is_dir(file_get_inode(pf->ptr)))
        return false;
    struct dir *dir = dir_open(file_get_inode(pf->ptr));
    bool success = dir_readdir(dir, name);
    dir_close(dir);
    return success;
}

bool
sys_isdir(int fd)
{
    struct proc_file *pf = list_search(&thread_current()->files, fd);
    if (pf == NULL)
        return false;
    return inode_is_dir(file_get_inode(pf->ptr));
}

block_sector_t
sys_inumber(int fd)
{
    struct proc_file *pf = list_search(&thread_current()->files, fd);
    if (pf == NULL)
        return -1;
    return inode_get_inumber(file_get_inode(pf->ptr));
}