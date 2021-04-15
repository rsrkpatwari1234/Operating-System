/* 
		SHARING MEMORY BETWEEN PROCESSES

    In this example, we show how two processes can share a common
    portion of the memory. Recall that when a process forks, the
    new child process has an identical copy of the variables of
    the parent process. After fork the parent and child can update
    their own copies of the variables in their own way, since they
    dont actually share the variable. Here we show how they can
    share memory, so that when one updates it, the other can see
    the change.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>	/*  This file is necessary for using shared
			    memory constructs
			*/

int main()
{
	int shmid,status;
	int *a, *b, *c;
	pid_t pid1, pid2;
	int i;

	/*  
	    The operating system keeps track of the set of shared memory
	    segments. In order to acquire shared memory, we must first
	    request the shared memory from the OS using the shmget()
      	    system call. The second parameter specifies the number of
	    bytes of memory requested. shmget() returns a shared memory
	    identifier (SHMID) which is an integer. Refer to the online
	    man pages for details on the other two parameters of shmget()
	*/
	shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0777|IPC_CREAT);
			/* We request an array of two integers */
	if (shmid < 0) {
          printf("*** shmget error ***\n");
          exit(1);
     }

     printf("Server has received a shared memory of four integers...\n");


	/* 
	    After forking, the parent and child must "attach" the shared
	    memory to its local data segment. This is done by the shmat()
	    system call. shmat() takes the SHMID of the shared memory
	    segment as input parameter and returns the address at which
	    the segment has been attached. Thus shmat() returns a char
	    pointer.
	*/
    pid1 = fork();
    pid2 = fork();
    if (pid1 < 0) {
      printf("*** fork 1 failed ***\n");
      exit(1);
    }
    	

    if(pid2 < 0){
    	printf("fork 2 failed");
    	exit(1);
    }

    if(pid1>0 && pid2>0){
    	printf("parent process");
    	a = (int *) shmat(shmid, 0, 0);
		
	    printf("Parent process has attached the shared memory...\n");
		a[0] = 0; a[1] = 1;
		for( i=0; i< 10; i++) {
			sleep(1);
			printf("Parent reads: %d,%d\n",a[0],a[1]);
			a[0] = a[0] + 1;
			a[1] = a[1] + 1;
			printf("Parent writes: %d,%d\n",a[0],a[1]);
		}
		wait(&status);
		wait(&status);
		/* each process should "detach" itself from the 
		   shared memory after it is used */

		shmdt(a);

		/* Child has exited, so parent process should delete
		   the cretaed shared memory. Unlike attach and detach,
		   which is to be done for each process separately,
		   deleting the shared memory has to be done by only
		   one process after making sure that noone else
		   will be using it 
		 */

		shmctl(shmid, IPC_RMID, 0);
    }
    else if(pid1>0 && pid2==0){
    	printf("First child process");
    	/*  shmat() returns a char pointer which is typecast here
		    to int and the address is stored in the int pointer b. */
		b = (int *) shmat(shmid, 0, 0);
		
	    printf("First child has attached the shared memory...\n");
		for( i=0; i< 10; i++) {
			sleep(1);
			printf("\t\t\t First Child reads: %d,%d\n",b[0],b[1]);
			b[0] = b[0] + 1;
			b[1] = b[1] + 1;
			printf("First Child writes: %d,%d\n",b[0],b[1]);
		}
		/* each process should "detach" itself from the 
		   shared memory after it is used */

		shmdt(b);
		exit(0);
    }
    else if(pid1==0 && pid2>0){
    	printf("Second child process");
    	/*  shmat() returns a char pointer which is typecast here
		    to int and the address is stored in the int pointer b. */
		c = (int *) shmat(shmid, 0, 0);
		
	    printf("Second child has attached the shared memory...\n");
		for( i=0; i< 10; i++) {
			sleep(1);
			printf("\t\t\t Second Child reads: %d,%d\n",c[0],c[1]);
			c[0] = c[0] + 1;
			c[1] = c[1] + 1;
			printf("Second Child writes: %d,%d\n",c[0],c[1]);
		}
		/* each process should "detach" itself from the 
		   shared memory after it is used */

		shmdt(c);
		exit(0);
    }
    else{
    	printf("Grand child process");
    	/*  shmat() returns a char pointer which is typecast here
		    to int and the address is stored in the int pointer b. */
		printf("do nothing");
		exit(0);
    }
	return 0;
}

/*
        POINTS TO NOTE:

	In this case we find that the child reads all the values written
	by the parent. Also the child does not print the same values
	again.

	1. Modify the sleep in the child process to sleep(2). What
	   happens now?

	2. Restore the sleep in the child process to sleep(1) and modify
           the sleep in the parent process to sleep(2). What happens now?

	Thus we see that when the writer is faster than the reader, then
	the reader may miss some of the values written into the shared
	memory. Similarly, when the reader is faster than the writer, then
	the reader may read the same values more than once. Perfect 
	inter-process communication requires synchronization between the
	reader and the writer. You can use semaphores to do this.

	Further note that "sleep" is not a synchronization construct.
        We use "sleep" to model some amount of computation which may
	exist in the process in a real world application.

	Also, we have called the different shared memory related
	functions such as shmget, shmat, shmdt, and shmctl, assuming
	that they always succeed and never fail. This is done to
	keep this proram simple. In practice, you should always check for
        the return values from this function and exit if there is 
	an error. 	
*/

