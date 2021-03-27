#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Assignment 6 : 2.4 started */

#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <user/syscall.h>
#include "list.h"

/*#define MAX_ARGS 3*/
#define STD_INPUT 0
#define STD_OUTPUT 1

void get_args (struct intr_frame *f, int *arg, int num_of_args);
int syscall_write (int filedes, int buffer, int byte_size);
void validate_ptr (const void* vaddr);
void validate_buffer (const void* buf, unsigned byte_size);
void* check_addr(const void*);

struct proc_file* list_search(struct list* files, int fd);

extern bool running;

struct proc_file {
  struct file* ptr;
  int fd;
  struct list_elem elem;
};


/* Assignment 6 : 2.4 ended */

static void syscall_handler (struct intr_frame *);

/*
 * System call initializer
 * It handles the set up for system call operations.
 */
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/*
 * This method handles for various case of system command.
 * This handler invokes the proper function call to be carried
 * out base on the command line.
 */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{

  /* Assignment 6 : 2.4 removal started */
  /*printf ("system call!\n");
  thread_exit ();*/
  /* Assignment 6 : 2.4 removal ended */

  /* Assignment 6 : 2.4 started */

  int* p = f->esp;
  check_addr(p);
  int system_call = *p;
  //printf("\nsyscall : %d\n",system_call);

  switch (system_call)
  {    
    case SYS_EXIT:
      check_addr(p+1);
      syscall_exit(*(p+1));
      break;
    
    case SYS_WRITE:
      check_addr(p+7);
      check_addr(p+6);
      f->eax = syscall_write(*(p+5), *(p+6), *(p+7));
      break;
    
    case SYS_EXEC:
      check_addr(p+1);
      f->eax = syscall_exec(*(p+1));
      break;

    case SYS_WAIT:
      check_addr(p+1);
      f->eax = process_wait(*(p+1));
      break;

    default:
      printf ("Not defined system call!\n");
      break;
  }

  /* Assignment 6 : 2.4 ended */
}

/* Assignment 6 : 2.4 started */

/* get arguments from stack */
void
get_args (struct intr_frame *f, int *args, int num_of_args)
{
  int i;
  int *ptr;
  for (i = 0; i < num_of_args; i++)
  {
    ptr = (int *) f->esp + i + 1;
    validate_ptr((const void *) ptr);
    args[i] = *ptr;
  }
}

/* System call exit 
 * Checks if the current thread to exit is a child.
 * If so update the child's parent information accordingly.
 */
/*void
syscall_exit (int status)
{
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}*/

void syscall_exit(int status)
{
  //printf("Exit : %s %d %d\n",thread_current()->name, thread_current()->tid, status);

  /* Assignment 6 : Part 1 started */

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

  // parent process unblock
  if(thread_current()->parent->waitingon == thread_current()->tid)
    sema_up(&thread_current()->parent->child_lock);

  /* Assignment 6 : Part 1 ended */

  // printf("%s: exit(%d)\n", thread_current()->name, status);

  thread_exit();
}

/* syscall_write */
int syscall_write (int filedes, int buffer, int byte_size)
{
    if (byte_size <= 0)
    {
      return byte_size;
    }
    /*if (filedes != STD_OUTPUT)
    {
      printf("Writing to console only for now!");
      syscall_exit(ERROR);
    }*/
    if(filedes == STD_OUTPUT)
    {
      putbuf(buffer, byte_size);
      return byte_size;
    }
    else
    {
      struct proc_file* fptr = list_search(&thread_current()->files, filedes);
      if(fptr==NULL)
        syscall_exit(ERROR);
      else
      {
        acquire_filesys_lock();
        int val = file_write (fptr->ptr, buffer, byte_size);
        release_filesys_lock();
        return val;
      }
    }
    return 0;
}

int syscall_exec(char *file_name)
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
    syscall_exit(ERROR);
  }
  else
  {
    file_close(f);
    release_filesys_lock();
    return process_execute(file_name);
  }
}

/* function to check if pointer is valid */
void
validate_ptr (const void *vaddr)
{
    if (vaddr < USER_VADDR_BOTTOM || !is_user_vaddr(vaddr))
    {
      // virtual memory address is not reserved for us (out of bound)
      syscall_exit(ERROR);
    }
}

/* function to check if buffer is valid */
void
validate_buffer(const void* buf, unsigned byte_size)
{
  unsigned i = 0;
  char* local_buffer = (char *)buf;
  for (; i < byte_size; i++)
  {
    validate_ptr((const void*)local_buffer);
    local_buffer++;
  }
}

/* get the pointer to page */
int
getpage_ptr(const void *vaddr)
{
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
  {
    syscall_exit(ERROR);
  }
  return (int)ptr;
}

void* check_addr(const void *vaddr)
{
  if (!is_user_vaddr(vaddr))
  {
    syscall_exit(ERROR);
    return 0;
  }
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
  {
    syscall_exit(ERROR);
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
/* Assignment 6 : 2.4 ended */