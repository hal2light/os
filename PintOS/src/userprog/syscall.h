#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

typedef int mapid_t;
mapid_t mmap(int fd, void *addr);
void munmap(mapid_t mapping);

#endif /* userprog/syscall.h */
