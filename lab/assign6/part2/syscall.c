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

#define STD_INPUT 0
#define STD_OUTPUT 1
/* definitions of write, exec and exit syscalls 
and an auxillary function to validate the value 
of the pagedir pointer of the process thread */
void syscall_exit(int status);
int syscall_write (int filedes, int buffer, int byte_size);
int syscall_exec(char *file_name);
void* check_addr(const void*);

/* Assignment 6 : 2.4 ended */

/* Assignment 6 : Part 2 started */

struct process_file* get_file(struct list* files, int fd);

struct process_file {
	int fd;
	struct file* file_ptr;
	struct list_elem elem;
};

/* Assignment 6 : Part 2 ended */

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

  /* Assignment 6 : 2.4 : Part 1 : Part 2 started */

  int* p = f->esp; // pointer to system call code
  check_addr(p); // validate the address of the pointer to the system call
  int system_call = *p; // integer code for matching 
  //printf("\nsyscall : %d\n",system_call);
  // exit, write and wait implemented
  switch (system_call)
  { // return value is stored in the eax register of the interrupt frame
    /* each case first validates the stack pointer value and then calls the 
    relevant system call. The eax value is set, if the syscall returns a value */
    case SYS_EXIT:
		check_addr(p+1);
		syscall_exit(*(p+1));
		break;

    case SYS_EXEC:
		check_addr(p+1);
		check_addr(*(p+1));
		f->eax = syscall_exec(*(p+1));
		break;

    case SYS_WAIT:
		check_addr(p+1);
		f->eax = syscall_wait(*(p+1));
		//f->eax = process_wait(*(p+1));
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
		/*if(filesys_remove(*(p+1))==NULL)
			f->eax = false;
		else
			f->eax = true;*/
		f->eax =  filesys_remove(*(p+1));
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
			struct process_file *pfile = malloc(sizeof(*pfile));
			pfile->file_ptr = fptr;
			pfile->fd = thread_current()->fd_count;
			thread_current()->fd_count++;
			list_push_back (&thread_current()->files, &pfile->elem);
			f->eax = pfile->fd;
		}
		break;

	case SYS_FILESIZE:
		check_addr(p+1);
		acquire_filesys_lock();
		f->eax = file_length (get_file(&thread_current()->files, *(p+1))->file_ptr);
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
			struct process_file* fptr = get_file(&thread_current()->files, *(p+5));
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				acquire_filesys_lock();
				f->eax = file_read (fptr->file_ptr, *(p+6), *(p+7));
				release_filesys_lock();
			}
		}
		break;

	case SYS_WRITE:
		check_addr(p+7);
		check_addr(*(p+6));
		f->eax = syscall_write(*(p+5), *(p+6), *(p+7));
		break;

	case SYS_SEEK:
		check_addr(p+5);
		acquire_filesys_lock();
		file_seek(get_file(&thread_current()->files, *(p+4))->file_ptr,*(p+5));
		release_filesys_lock();
		break;

	case SYS_TELL:
		check_addr(p+1);
		acquire_filesys_lock();
		f->eax = file_tell(get_file(&thread_current()->files, *(p+1))->file_ptr);
		release_filesys_lock();
		break;

	case SYS_CLOSE:
		check_addr(p+1);
		acquire_filesys_lock();
		struct list* files = &thread_current()->files;
		int fd = *(p+1);
		close_file(files, fd);
		release_filesys_lock();
		break;

    default:
    	printf("Default %d\n",*p);
      	break;
  }

  /* Assignment 6 : 2.4 : Part 1 : Part 2 ended */
}

/* Assignment 6 : 2.4 started */

/* System call exit 
 * Checks if the current thread to exit is a child.
 * If so update the child's parent information accordingly.
 */

void syscall_exit(int status)
{
  //printf("Exit : %s %d %d\n",thread_current()->name, thread_current()->tid, status);

  /* Assignment 6 : Part 1 started */

  struct list_elem *e;

  for (e = list_begin (&thread_current()->parent->child_processes); e != list_end (&thread_current()->parent->child_processes);
       e = list_next (e))
  { // remove child from list of child processes, of the parent thread
    struct child *f = list_entry (e, struct child, elem);
    if(f->tid == thread_current()->tid)
    {
      f->used = true; // setting this flag to true to inform parent to not call the semaphore.
      f->error_code = status; // whether exit was due to an error or under normal circumstances.
    }
  }
  thread_current()->error_code = status; // set error_code according to the status

  // unblock the parent process
  if(thread_current()->parent->waiting_on_child == thread_current()->tid) // check if the parent of the current thread is waiting on it
    sema_up(&thread_current()->parent->sema_child); // increase value of the semaphore to unblock parent process

  /* Assignment 6 : Part 1 ended */

  printf("%s: exit(%d)\n", thread_current()->name, status);

  thread_exit();
}

/* syscall_write */
int syscall_write (int filedes, int buffer, int byte_size)
{
    if (byte_size <= 0) // don't do anything for non-positive byte-size.
    {
      return byte_size;
    }
    if(filedes == STD_OUTPUT) // check if the file descriptor is for STD_OUTPUT i.e. the console
    {
      putbuf(buffer, byte_size);
      return byte_size;
    }
    else
    {
      	struct process_file* fptr = get_file(&thread_current()->files, filedes);
		if(fptr==NULL)
			return ERROR;
		else
		{	
			int status;
			acquire_filesys_lock();
			status = file_write (fptr->file_ptr, buffer, byte_size);
			release_filesys_lock();
			return status;
		}
    }
    return ERROR;
}

int syscall_exec(char *file_name) // inside ths we ensure that the executable exists and can be opened even before calling process_execute.
{
  acquire_filesys_lock(); // because filesys_open is a critical step.
  char * fn_cp = malloc (strlen(file_name)+1); // save a copy of the filename, which we use for parsing the true filename/executable name and command line args.
  strlcpy(fn_cp, file_name, strlen(file_name)+1);
  
  char * save_ptr; // required by strtok_r to keep track of current location.
  fn_cp = strtok_r(fn_cp," ",&save_ptr); // get name of the executable.

  struct file* f = filesys_open (fn_cp); // open executable file.

  if(f == NULL) // in case file open fails, e.g. file doesn't exist.
  {
    release_filesys_lock(); // release lock.
    syscall_exit(0); // exit with error in case file can't be loaded. (CONFIRM)
  }
  else
  {
    file_close(f);
    release_filesys_lock(); // release the file system.
    return process_execute(file_name); // execute the file process (we now know that the executable can be loaded). 
  }
}

/* this function is to check if the process is inside user virtual address space.
It also makes sure that a vitrual address is associated with the pointer to the
pagedirectory owned by the callee user process. In case of any of these conditions 
fail, it triggers and escape system call with ERROR status */
void* check_addr(const void *vaddr) 
{
  if (!is_user_vaddr(vaddr)) // need to check for this before calling pagedir_get page, or it will result in kernel panic
  {                          // as there is an ASSERT for this (i.e. to check whether the address is inside the user space or not)
    syscall_exit(ERROR);  // exit with ERROR status.
    return 0;
  }
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr); 
  if (!ptr) // if pointer is NULL, this means that the physical address lies outsied the virtual address space.
  {
    syscall_exit(ERROR); // exit with ERROR status.
  }
  return ptr;
}

/* Assignment 6 : 2.4 ended */

/* Assignment 6 : Part 2 started */

int syscall_wait(tid_t child_tid)
{
	return process_wait(child_tid);
}
/* Get the reference to the process_file struct type having a particular fd from a list of files */
struct process_file* get_file(struct list* files, int fd)
{
	struct list_elem *e;
	for (e = list_begin (files); e != list_end (files); e = list_next (e))
	{
		struct process_file *f = list_entry (e, struct process_file, elem);
		if(f->fd == fd)
			return f;
	}
   return NULL; // return NULL if file not found.
}

void close_file(struct list* files, int fd)
{
	struct list_elem *e;
	struct process_file *f;
	for (e = list_begin (files); e != list_end (files); e = list_next (e))
	{
		f = list_entry (e, struct process_file, elem);
		if(f->fd == fd)
		{
			file_close(f->file_ptr);
			list_remove(e);
		}
	}
    free(f);
}

void close_files(struct list* files)
{
	struct list_elem *e;
	while(!list_empty(files))
	{
		e = list_pop_front(files);
		struct process_file *f = list_entry (e, struct process_file, elem);
      	file_close(f->file_ptr);
      	list_remove(e);
      	free(f);
	} 
}

/* Assignment 6 : Part 2 ended */