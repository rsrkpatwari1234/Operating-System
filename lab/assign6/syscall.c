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

#define MAX_ARGS 3
#define STD_INPUT 0
#define STD_OUTPUT 1

void get_args (struct intr_frame *f, int *arg, int num_of_args);
int syscall_write (int filedes, const void * buffer, unsigned byte_size);
void validate_ptr (const void* vaddr);
void validate_buffer (const void* buf, unsigned byte_size);

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

  int arg[MAX_ARGS];
  int esp = getpage_ptr((const void *) f->esp);
  switch (* (int *) esp)
  {    
    case SYS_EXIT:
      // fill arg with the amount of arguments needed
      get_args(f, &arg[0], 1);
      syscall_exit(arg[0]);
      break;
       
    case SYS_WRITE:
      
      // fill arg with the amount of arguments needed
      get_args(f, &arg[0], 3);
      printf("-1-");
      /* Check if the buffer is valid.
       * We do not want to mess with a buffer that is out of our
       * reserved virtual memory 
       */
      validate_buffer((const void*)arg[1], (unsigned)arg[2]);
      printf("-2-");
      // get page pointer
      arg[1] = getpage_ptr((const void *)arg[1]); 
      printf("-3-");
      /* syscall_write (int filedes, const void * buffer, unsigned bytes)*/
      f->eax = syscall_write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
      printf("-4-");
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
void
syscall_exit (int status)
{
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

/* syscall_write */
int 
syscall_write (int filedes, const void * buffer, unsigned byte_size)
{
    if (byte_size <= 0)
    {
      return byte_size;
    }
    if (filedes != STD_OUTPUT)
    {
      printf("Writing to console only for now!");
      syscall_exit(ERROR);
    }
    putbuf (buffer, byte_size); // from stdio.h
    return byte_size;
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


/* Assignment 6 : 2.4 ended */