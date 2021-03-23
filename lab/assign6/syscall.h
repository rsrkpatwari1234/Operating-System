#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* Assignment 6 : 2.4 started */

#include "threads/thread.h"

#define ERROR -1
#define USER_VADDR_BOTTOM ((void *) 0x08048000)

int getpage_ptr (const void *vaddr);
void syscall_exit (int status);

/* Assignment 6 : 2.4 ended */

void syscall_init (void);

#endif /* userprog/syscall.h */
