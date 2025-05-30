#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void* check_addr(const void*);
struct proc_file* list_search(struct list* files, int fd);

extern bool running;

struct proc_file {
	struct file* ptr;
	int fd;
	struct list_elem elem;
};

struct mmap_file {
  mapid_t mapid;
  struct file *file;
  void *addr;
  size_t size;
  struct list_elem elem;
};

static struct list mmap_list;
static mapid_t next_mapid = 1;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init(&mmap_list);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp;

	check_addr(p);



  int system_call = * p;
	switch (system_call)
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
		check_addr(p+5);
		check_addr(*(p+4));
		acquire_filesys_lock();
		f->eax = filesys_create(*(p+4),*(p+5));
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
		check_addr(p+1);
		check_addr(*(p+1));

		acquire_filesys_lock();
		struct file* fptr = filesys_open (*(p+1));
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
			if(fptr==NULL)
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

		case SYS_MMAP:
		check_addr(p+2);
		check_addr(*(p+2));
		f->eax = mmap(*(p+1),*(p+2));
		break;

		case SYS_MUNMAP:
		check_addr(p+1);
		munmap(*(p+1));
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

mapid_t
mmap (int fd, void *addr)
{
  if (addr == NULL || pg_ofs(addr) != 0 || fd <= 1)
    return -1;
    
  struct file *file = fd_to_file(fd);
  if (file == NULL)
    return -1;
    
  size_t size = file_length(file);
  if (size == 0)
    return -1;
    
  // Check for overlap with existing mappings
  void *end_addr = addr + size;
  for (void *page = addr; page < end_addr; page += PGSIZE) {
    if (page_lookup(page) != NULL)
      return -1;
  }
  
  struct mmap_file *mf = malloc(sizeof(struct mmap_file));
  if (mf == NULL)
    return -1;
    
  mf->mapid = next_mapid++;
  mf->file = file_reopen(file);
  mf->addr = addr;
  mf->size = size;
  
  // Create page table entries
  size_t offset = 0;
  for (void *page = addr; page < end_addr; page += PGSIZE) {
    size_t read_bytes = size - offset < PGSIZE ? size - offset : PGSIZE;
    size_t zero_bytes = PGSIZE - read_bytes;
    
    struct page *p = malloc(sizeof(struct page));
    if (p == NULL) {
      munmap(mf->mapid);
      return -1;
    }
    
    p->addr = page;
    p->writable = true;
    p->loaded = false;
    p->status = PAGE_FILE;
    p->file = mf->file;
    p->file_offset = offset;
    p->file_bytes = read_bytes;
    p->frame = NULL;
    
    if (!page_insert(&thread_current()->spt, p)) {
      free(p);
      munmap(mf->mapid);
      return -1;
    }
    
    offset += read_bytes;
  }
  
  list_push_back(&mmap_list, &mf->elem);
  return mf->mapid;
}

void
munmap (mapid_t mapping)
{
  struct list_elem *e;
  for (e = list_begin(&mmap_list); e != list_end(&mmap_list); e = list_next(e)) {
    struct mmap_file *mf = list_entry(e, struct mmap_file, elem);
    if (mf->mapid == mapping) {
      // Write back dirty pages
      void *end_addr = mf->addr + mf->size;
      for (void *page = mf->addr; page < end_addr; page += PGSIZE) {
        struct page *p = page_lookup(page);
        if (p != NULL && p->loaded && pagedir_is_dirty(thread_current()->pagedir, page)) {
          file_write_at(mf->file, page, p->file_bytes, p->file_offset);
        }
      }
      
      // Remove pages from supplemental page table
      for (void *page = mf->addr; page < end_addr; page += PGSIZE) {
        struct page *p = page_lookup(page);
        if (p != NULL) {
          hash_delete(&thread_current()->spt, &p->hash_elem);
          page_free(p);
        }
      }
      
      // Clean up
      file_close(mf->file);
      list_remove(&mf->elem);
      free(mf);
      break;
    }
  }
}