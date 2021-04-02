#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

/* Assignment 6 : Part 2 started */
extern struct list all_list;
/* Assignment 6 : Part 2 ended */

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0); // acquire page for the process
  if (fn_copy == NULL) // if page can't be acquired an error is returned, by sending tid_t of -1
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Assignment 6 : 2.4 started */

  char *save_ptr; // needed by strtok_r
  char *f_name; // fname is where the name of executable is stored.
  f_name = malloc(strlen(file_name)+1);

  strlcpy (f_name, file_name, strlen(file_name)+1); // initialize fname as the executable_name+command_line_args

  f_name = strtok_r (f_name," ",&save_ptr); // extract just the executable name into f_name
  
  //printf("string : %s",fn_copy);
  //printf("%d\n", thread_current()->tid);
  // start thread with thread function as start_process, which loads the executable file.
  tid = thread_create (f_name, PRI_DEFAULT, start_process, fn_copy); // create thread with name of the executable file.
  free(f_name);

  /* Assignment 6 : 2.4 ended */

  if (tid == TID_ERROR) // if thread creation fails, free up the pagedir.
    palloc_free_page (fn_copy); 

  /* Assignment 6 : Part 1 started */
  sema_down(&thread_current()->sema_child); // acquire the semaphore for the child.

  if(!thread_current()->success)
    return -1;

  /* Assignment 6 : Part 1 ended */

  return tid; // sucessful execution returns tid of created process
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp); // check if file could be loaded sucessfully.

  /* If load failed, quit. */
  palloc_free_page (file_name);

  /* Assignment 6 : 2.4 started : not used in Part 1*/
  /*if (!success) 
    thread_exit ();*/
  /* Assignment 6 : 2.4 ended */

  /* Assignment 6 : Part 1 started */
  if (!success) {
    //printf("%d %d\n",thread_current()->tid, thread_current()->parent->tid);
    thread_current()->parent->success=false; // parent's success attribute is set to false if the executable fails to load. (this gives the parent info about whether the executable can be loaded.)
    sema_up(&thread_current()->parent->sema_child); // the parent is freed from this child.
    thread_exit(); // the child process exits when file fails to load.
  }
  else
  {
    thread_current()->parent->success=true; // parent is passed the info that the child was able to open/load the executable.
    sema_up(&thread_current()->parent->sema_child);
  }
  /* Assignment 6 : Part 1 ended */

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid)  // the function is called by parent process, and it supplies the child tid
{
  /* Assignment 6 : 2.4 started */
  //while(1);  
  /* Assignment 6 : 2.4 ended */
  //return -1;

  /* Assignment 6 : Part 1 started */
  // printf("waiting: %s %d\n",thread_current()->name, child_tid);
  struct list_elem *iter; 

  struct child *ch=NULL; // variable to store child process once it is located.
  struct list_elem *e=NULL; // store the child element once it is found.
  // this loop locates the child process in the list of child process of the caller process.
  for (iter = list_begin (&thread_current()->child_processes); iter != list_end (&thread_current()->child_processes);
           iter = list_next (iter))
        {
          struct child *f = list_entry (iter, struct child, elem);
          if(f->tid == child_tid)
          {
            ch = f;
            e = iter;
          }
        }


  if(!ch || !e) // if the child process is never found
    return -1;

  thread_current()->waiting_on_child = ch->tid; // confirm that parent process is waiting on the child process (required by process exit)
    
  if(!ch->used) // if the used flag for the child process is still set to false
    sema_down(&thread_current()->sema_child); 
    /* we wait for the child process to finish exiting, i.e. we wait for it to call the 
    sema_up on the sema_child semaphore */

  int temp = ch->error_code; // return value of the function, i.e. the exit status
  list_remove(e); // remove the found entry for the child process
  
  return temp;

  /* Assignment 6 : Part 1 ended */
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Assignment 6 : Part 1 started */

  if(cur->error_code==-100) // exit with error if error_code is -100
      syscall_exit(-1);

  int exit_code = cur->error_code;
  //printf("%s: exit(%d)\n",cur->name,exit_code);

  /* Assignment 6 : Part 1 ended */

  /* Assignment 6 : Part 2 started */
  acquire_filesys_lock();
  file_close(thread_current()->self);
  close_all_files(&thread_current()->files);
  release_filesys_lock();
  /* Assignment 6 : Part 2 ended */

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir; 
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL; // destroy pages
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp, char *file_name);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Assignment 6 : Part 1 started */
  acquire_filesys_lock(); // file loading is critical section.
  /* Assignment 6 : Part 1 ended */

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();
  
  /* Open executable file. */

  /* Assignment 6 : 2.4 started */
  char * fn_cp = malloc (strlen(file_name)+1);
  strlcpy(fn_cp, file_name, strlen(file_name)+1); 
  
  char * save_ptr;
  fn_cp = strtok_r(fn_cp," ",&save_ptr);

  file = filesys_open (fn_cp); // open executable.

  free(fn_cp);

  /* Assignment 6 : 2.4 ended */

  if (file == NULL) // message when file load saves.
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Assignment 6 : 2.4 started */
  /* Set up stack. */
  if (!setup_stack (esp, file_name)) // check if setupstack fails.
    goto done;
  /* Assignment 6 : 2.4 ended */

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry; 

  success = true;

  /* Assignment 6 : Part 2 started */
  file_deny_write(file);

  thread_current()->self = file;
  /* Assignment 6 : Part 2 ended */

 done:
  /* We arrive here whether the load is successful or not. */
  //file_close (file);

  /* Assignment 6 : Part 1 started */
  release_filesys_lock(); // release file system locks
  /* Assignment 6 : Part 1 ended */

  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp, char *file_name) 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        /* Assignment 6 : 2.4 started */ 
        *esp = PHYS_BASE;    /* Assignment 6 : 2.4 -12*/
        /* Assignment 6 : 2.4 started */
      else
        palloc_free_page (kpage);
    }

  /* Assignment 6 : Part 1 started */
  char *token, *save_ptr;
  int argc = 0,i;

  char * copy = malloc(strlen(file_name)+1);
  strlcpy (copy, file_name, strlen(file_name)+1);


  for (token = strtok_r (copy, " ", &save_ptr); token != NULL;
    token = strtok_r (NULL, " ", &save_ptr))
    argc++;


  int *argv = calloc(argc,sizeof(int));

  for (token = strtok_r (file_name, " ", &save_ptr),i=0; token != NULL;
    token = strtok_r (NULL, " ", &save_ptr),i++)
    {
      *esp -= strlen(token) + 1;
      memcpy(*esp,token,strlen(token) + 1);

      argv[i]=*esp;
    }

  while((int)*esp%4!=0)
  {
    *esp-=sizeof(char);
    char x = 0;
    memcpy(*esp,&x,sizeof(char));
  }

  int zero = 0;

  *esp-=sizeof(int);
  memcpy(*esp,&zero,sizeof(int));

  for(i=argc-1;i>=0;i--)
  {
    *esp-=sizeof(int);
    memcpy(*esp,&argv[i],sizeof(int));
  }

  int pt = *esp;
  *esp-=sizeof(int);
  memcpy(*esp,&pt,sizeof(int));

  *esp-=sizeof(int);
  memcpy(*esp,&argc,sizeof(int));

  *esp-=sizeof(int);
  memcpy(*esp,&zero,sizeof(int));

  free(copy);
  free(argv);

  /* Assignment 6 : Part 1 ended */

  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}