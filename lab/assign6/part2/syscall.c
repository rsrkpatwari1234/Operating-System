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

struct opened_file* get_file(struct list* files, int fd); // get file from a list of files using given file descriptor.

struct opened_file { // to store details about files opened by a process.
	int fd; // file descriptor.
	struct file* file_ptr; // pointer to the file struct.
	struct list_elem elem; // list element for storing pointer to next and previous element of the list of files.
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
syscall_handler (struct intr_frame *f) 
{

  /* Assignment 6 : 2.4 removal started */
  /*printf ("system call!\n");
  thread_exit ();*/
  /* Assignment 6 : 2.4 removal ended */

  /* Assignment 6 : 2.4 : Part 1 : Part 2 started */

  int* p = f->esp; 					// pointer to system call code
  check_addr(p); 					// validate the address of the pointer to the system call
  int system_call = *p; 			// integer code for matching 
  //printf("\nsyscall : %d\n",system_call);

  switch (system_call)
  { // return value is stored in the eax register of the interrupt frame
    /* each case first validates the stack pointer value and then calls the 
    relevant system call. The eax value is set, if the syscall returns a value */
    
	// we surround all filesys function calls with acquire_filesys_lock and release_filesys_lock calls to make sure that no critical file operations are interrupted.
	
	case SYS_EXIT:
		check_addr(p+1);
		syscall_exit(*(p+1)); // call syscall exit, pass the first argument which is the status.
		break;

    case SYS_EXEC:
		check_addr(p+1);
		check_addr(*(p+1));
		f->eax = syscall_exec(*(p+1)); // call syscall exit, pass the first argument on the stack, which is the filename.
		break;

    case SYS_WAIT:
		check_addr(p+1);
		f->eax = syscall_wait(*(p+1)); // call syscall wait, pass the first argument on the stack, which is the filename.
		break;

    case SYS_CREATE:
		check_addr(p+5);
		check_addr(*(p+4));
		f->eax = syscall_create(*(p+4),*(p+5)); /* call syscall create, pass the value stored at 
		fourth and fifth position on the stack, which are the filename and the initial size of the file */
		break; // set the return value to the status returned by the filesys_create function.

	case SYS_REMOVE:
		check_addr(p+1);
		check_addr(*(p+1));
		f->eax = syscall_remove(*(p+1)); // call syscall remove to delete a file.
		break; // set the return value to status returned by the filesys_remove function.

	case SYS_OPEN:
		check_addr(p+1);
		check_addr(*(p+1));
		f->eax = syscall_open(*(p+1)); // call syscall open to open a file and update the necessary info of the thread struct such as the fd_count and the list of opened_file structs.
		break; // set the return value to the status returned by the filesys_create function.

	case SYS_FILESIZE:
		check_addr(p+1);
		f->eax = syscall_filesize(&thread_current()->files, *(p+1)); // call syscall_filesize to get size of a file in BYTES.
		break; // set the return value to the size of file returned by file_length.

	case SYS_READ:
		check_addr(p+7);
		check_addr(*(p+6));
		f->eax = syscall_read(*(p+5), *(p+6), *(p+7)); // call syscall read to read at most size no. of bytes into the buffer.
		break; // set the return value to the actual number of bytes read.

	case SYS_WRITE:
		check_addr(p+7);
		check_addr(*(p+6));
		f->eax = syscall_write(*(p+5), *(p+6), *(p+7)); // call syscall write to write at most size no. of bytes into the buffer.
		break; // set the return value to the actual number of bytes read.

	case SYS_SEEK:
		check_addr(p+5);
		syscall_seek(&thread_current()->files, *(p+4), *(p+5));  // call syscall seek to move offset of file pointer to a new position.
		break;

	case SYS_TELL: 
		check_addr(p+1);
		f->eax = syscall_tell(&thread_current()->files, *(p+1)); // call syscall tell to get BYTE offset of current position of file pointer from start of file.
		break; // set the return value to the current BYTE offset from the start of the file.

	case SYS_CLOSE:
		check_addr(p+1); 
		syscall_close(&thread_current()->files, *(p+1)); // call syscall close to close a file having a particular fd from the list of files belonging to the process.
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
    if (byte_size <= 0) 		// don't do anything for non-positive byte-size.
    {
      return byte_size;
    }
    if(filedes == STD_OUTPUT) 	// check if the file descriptor is for STD_OUTPUT i.e. the console
    {
      putbuf(buffer, byte_size);
      return byte_size;
    }
    else
    {
      	struct opened_file* fptr = get_file(&thread_current()->files, filedes);
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

int syscall_exec(char *file_name) 	// inside ths we ensure that the executable exists and can be opened even before calling process_execute.
{
  acquire_filesys_lock(); 							// because filesys_open is a critical step.
  char * fn_cp = malloc (strlen(file_name)+1); 		// save a copy of the filename, which we use for parsing the true filename/executable name and command line args.
  strlcpy(fn_cp, file_name, strlen(file_name)+1);
  
  char * save_ptr; 									// required by strtok_r to keep track of current location.
  fn_cp = strtok_r(fn_cp," ",&save_ptr); 			// get name of the executable.

  struct file* f = filesys_open (fn_cp); 			// open executable file.

  if(f == NULL)										// in case file open fails, e.g. file doesn't exist.
  {
    release_filesys_lock(); 						// release lock.
    return ERROR;           						// exit with error in case file can't be loaded.
  }
  else
  {
    file_close(f);
    release_filesys_lock(); 				// release the file system.
    return process_execute(file_name); 		// execute the file process (we now know that the executable can be loaded). 
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
/* we use process_wait to implement this syscall */
int syscall_wait(tid_t child_tid)
{
	return process_wait(child_tid);
}

/* creates a file named "file", with the size "initial_size" */
int syscall_create(const char *file, unsigned initial_size)
{
	acquire_filesys_lock(); // acquire lock for critical section. 
	int status = filesys_create(file, initial_size); // create file and return status of creation.
	release_filesys_lock(); // release lock for critical section.
	return status;
}
/* open file using filename */
int syscall_open(const char *file)
{
	acquire_filesys_lock(); // make sure that file opening is not interrupted.
	struct file* fptr = filesys_open (file); // open file. (a pointer to a file struct is returned)
	release_filesys_lock();

	if(fptr!=NULL) // make sure that the file creation was succesful.
	{	/* an opened_file struct is used to store the following info:
		   pointer of the file struct returned by filesys_open.
		   file descriptor.
		   elem struct, which has the forward and backward pointers for maintaing the list of files. */
		struct opened_file *pfile = malloc(sizeof(*pfile)); // create a process file struct.
		pfile->file_ptr = fptr;
		pfile->fd = thread_current()->fd_count;
		thread_current()->fd_count++; // fd_count is incremented as the file descriptors of the succesive files are like 0,1,2 ..
		list_push_back (&thread_current()->files, &pfile->elem); // add file to the list of files.
		return pfile->fd; // return file descriptor.
	}
	return -1; // -1 indicates failure in opening of the file.
}
/* read from a file descriptor and store in the buffer, upto size number of bytes. */
int syscall_read (int fd, uint8_t *buffer, unsigned size)
{
	if(fd == 0) // stdin case. (read from console)
	{
		for(int i = 0; i < size; i++) // take one byte/char at a time using input_getc till size no. of bytes are read.
			buffer[i] = input_getc(); /* input_getc: Retrieves a key from the input buffer.
   										 If the buffer is empty, waits for a key to be pressed. */
		return size;
	}

	else // read from a file.
	{
		struct opened_file* fptr = get_file(&thread_current()->files, fd); // get file from list of files opened by the current process.
		int val; // val is the number of bytes actually read.
		if(fptr==NULL) // return -1 if file couldn't be opened. 
			return -1;
		else
		{
			acquire_filesys_lock(); // critical section.
			val = file_read (fptr->file_ptr, buffer, size); // returns number of bytes actually read.
			release_filesys_lock();
		}
		return val; // return the number of bytes actually read (maybe be less than size).
	}
}
/* close file having a particular fd. */
void syscall_close(struct list* files, int fd)
{
	acquire_filesys_lock();
	close_file(files, fd); // close file with a particular fd, from the list of files.
	release_filesys_lock();
}
/* syscall to remove/delete a file. */
int syscall_remove(const char *file)
{
	acquire_filesys_lock();
	int status =  filesys_remove(file); // deletes file if it exists, false is returned if it fails.
	release_filesys_lock();
	return status;
}
/* syscall to seek file: set position of file pointer. */
void syscall_seek(struct list* files, int fd, int new_pos) {
	acquire_filesys_lock(); // acquire filesys_lock.
	file_seek(get_file(files, fd)->file_ptr, new_pos); // supply the file pointer and the target offset to the file_seek call.
	// we utilize the file_seek function provided within the filesys/file.c file.
	release_filesys_lock(); // release filesys_lock.
}
/* syscall to tell offset of current file pointer location from start of the file. */
int syscall_tell(struct list* files, int fd) {
	acquire_filesys_lock(); // acquire filesys_lock.
	// get offset (pos attribute of the file struct) from the start of the file, of current FILE pointer (file->pos) 
	int offset = file_tell(get_file(files, fd)->file_ptr); // we utilize the file_tell function provided within the filesys/file.c file.
	release_filesys_lock(); // release filesys_lock.
	
	return offset;
}
/* syscall to get size of the file in BYTES. */
int syscall_filesize(struct list* files, int fd) {
	acquire_filesys_lock(); // acquire filesys_lock.
	/* get length of the file's inode (file->inode) using the inode_length function.
	   the inode_length function expects an inode struct and return the inode.data.length.
	   the data attribute of the inode struct in turn is also a struct of inode_disk type 
	   which stores internal details about how the file's inode is stored on the disk, which
	   includes the length of the file. */
	int offset = file_length (get_file(files, fd)->file_ptr); // we utilize the file_length function provided within the filesys/file.c file.
	release_filesys_lock(); // release filesys_lock.

	return offset;
}

/* Get the reference to the opened_file struct type having a particular fd from a list of files. */
struct opened_file* get_file(struct list* files, int fd)
{
	struct list_elem *e;
	for (e = list_begin (files); e != list_end (files); e = list_next (e))
	{
		struct opened_file *f = list_entry (e, struct opened_file, elem);
		if(f->fd == fd)
			return f;
	}
   return NULL; // return NULL if file not found.
}
/* to close a file according to the file descriptor. */
void close_file(struct list* files, int fd)
{
	struct list_elem *e;
	struct opened_file *f;
	for (e = list_begin (files); e != list_end (files); e = list_next (e))
	{ // iterate over the list of files to get pointer to the file corresponding to the given file descriptor.
		f = list_entry (e, struct opened_file, elem);
		if(f->fd == fd)
		{
			file_close(f->file_ptr); // close file.
			list_remove(e); // remove entry of the file from the thread's list of files.
			break;
		}
	}
    free(f);
}
/* close all files opened by a process.  */
void close_files(struct list* files) // close all files belonging to a process.
{
	struct list_elem *e; 
	while(!list_empty(files)) 
	{
		e = list_pop_front(files);
		struct opened_file *f = list_entry (e, struct opened_file, elem);
      	file_close(f->file_ptr);
      	list_remove(e);
      	free(f);
	} 
}

/* Assignment 6 : Part 2 ended */