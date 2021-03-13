// Atharva Naik, 18CS10067
// Radhika Patwari, 18CS10062
// Assignment 5

#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <bits/stdc++.h>

// ranges of process variables
#define MIN_PRIORITY 1 // min priority value
#define MAX_PRIORITY 10 // max priority value
#define MIN_JOB_ID 1 // minimum numerical value of JOB_ID (ids start from here)
#define MAX_JOB_ID 100000 // maximum numerical value of JOB_ID (ids end here)
#define MIN_COMPUTE_TIME 1 // minimum value of compute time
#define MAX_COMPUTE_TIME 4 // maximum value of compute time
#define MIN_SLEEP_TIME 1000000 // in μs 
#define MAX_SLEEP_TIME 3000000 // in μs

using namespace std;
const int QUEUE_SIZE = 8;

// check if mutex is defined/suported in the system
#ifndef _POSIX_THREAD_PROCESS_SHARED
    #error No definition found for mutex
    exit(EXIT_FAILURE);
#endif

struct process_t{
    int pid; // process id
    int producer_number; // process number corresponding to no. of producers 
    int priority; // priority of the number ( between 1 and 10 )
    int compute_time; // the compute time
    int jid; // the job id
};

// populate process structure with given pid and producer number
process_t create_job(int pid, int producer_number, int job_created)
{
    srand(pid + job_created); // seed random number generator with the pid
    process_t job; // create process structure
    // populate data members of the process
    job.pid = pid;
    job.producer_number = producer_number;
    job.priority = rand() % MAX_PRIORITY + MIN_PRIORITY; // value ranges from 1 to 10
    job.compute_time = rand() % MAX_COMPUTE_TIME + MIN_COMPUTE_TIME; // value ranges from 1 to 4
    job.jid = rand() % MAX_JOB_ID + MIN_JOB_ID; // value ranges from 1 to 100000
    
    return job;
}

struct process_queue_buffer{
    int size; // current buffer size (number of elements stored in buffer currently)
    int job_created; // number of jobs created
    int job_completed; // number of jobs completed
    pthread_mutex_t mutex; // mutex
    process_t  process_queue[QUEUE_SIZE]; // queue of processes
};

pthread_mutex_t    *mutex_ptr; // pointer to mutex
pthread_mutexattr_t mutex_attr; // mutex attribute

void insert_sorted_buffer(process_t job, process_queue_buffer* buffer) {
    // if queue is full
    if (buffer->size == QUEUE_SIZE)
        return;
 	
 	// insertion sort to maintain increasing order of priorities     
    (buffer->process_queue[buffer->size]) = job;  // set job to a valid element of process queue array

    for (int i = (buffer->size)-1; i >= 0; i--)
    {
    	// giving more importance to job of >= new job priority
    	if ((buffer->process_queue[i]).priority < job.priority)
    		break;

    	// swapping new job with old job of >= new job priority
        (buffer->process_queue)[i+1] = (buffer->process_queue)[i];
        (buffer->process_queue)[i] = job;
    }
    (buffer->size)++; // increment buffer size
}

 	
// as elements are stored in increasing order of priorities in buffer
// remove job with highest priority
process_t remove_sorted_buffer(process_queue_buffer* buffer) {

    process_t job = (buffer->process_queue)[(buffer->size)-1];
    (buffer->size)--;

    cout<<"\x1b[44;1m Killing job of priority ="<<job.priority<<" \x1b[0m"<<endl; // #REMOVE#

    return job;
}

int main()
{
    int NC, NP, NJOBS; // NC is number of consumers, NP is number of producers, NJOBS is number of jobs
    cout << "number of producers=";
    cin >> NP;
    cout << "number of consumers=";
    cin >> NC;
    cout << "number of jobs=";
    cin >> NJOBS;

    key_t key = ftok("shmfile", 65); // generate key for creating shared memory
    int shmid = shmget(key, 128+64*QUEUE_SIZE, 0666|IPC_CREAT); // create shared memory segment 
    process_queue_buffer * buffer=(process_queue_buffer *) shmat(shmid, (void*)0,0); 
    // initialize buffer, which acts as a process_queue_manager
    buffer->size = 0; 
    buffer->job_created = 0;
    buffer->job_completed = 0;
    mutex_ptr = &(buffer->mutex);
    int error_code; // store error code, to catch and identify errors
    // setup mutex and catch any errors that might occur
    if (error_code = pthread_mutexattr_init(&mutex_attr)) // check if error occured in initalization
    {
        fprintf(stderr,"error in initialization : %s", strerror(error_code)); 
        exit(EXIT_FAILURE);
    }
    if (error_code = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED)) 
    /* The process-shared attribute is set to PTHREAD_PROCESS_SHARED to permit a mutex to be operated 
    upon by any thread that has access to the memory where the mutex is allocated, even if the mutex 
    is allocated in memory that is shared by multiple processes. (taken from docs)*/ 
    {
        fprintf(stderr,"pthread_mutexattr_setpshared %s",strerror(error_code));
        exit(EXIT_FAILURE);
    }
    if (error_code = pthread_mutex_init(mutex_ptr, &mutex_attr))
    /* The pthread_mutex_init() function initialises the mutex referenced by mutex with attributes 
    specified by attr. If attr is NULL, the default mutex attributes are used; the effect is the 
    same as passing the address of a default mutex attributes object. Upon successful initialisation, 
    the state of the mutex becomes initialised and unlocked. */
    {
        fprintf(stderr,"pthread_mutex_init %s",strerror(error_code)); 
        exit(EXIT_FAILURE);
    }
  
    time_t start_time = time(NULL); // get time before process creation starts
    // create producer and consumer processes
    for (int i = 0; i < NC+NP; i++)
    {
        // check if inside parent, and create new child process
        if (fork() == 0)
        {
            if (i < NP) // when index is less than NP create producer process
            {
                // producer code
                while (1)
                {
                    process_t job = create_job(getpid(), i, buffer->job_created);
                    usleep(rand() % MAX_SLEEP_TIME); // sleep for less than 3s
                    pthread_mutex_lock(mutex_ptr); // lock mutex
                    // check if more jobs are to be crated and buffer has remaining space
                    if ((buffer->job_created) < NJOBS && buffer->size < QUEUE_SIZE) {
                        insert_sorted_buffer(job, buffer); // add job to buffer
                        (buffer->job_created)++; // increment jobs created counter
                        cout << "\x1b[34;1mProducer: " << i << "\x1b[0m\x1b[32m pid=" << getpid() << ", priority=" << (job.priority) << ", jid=" << job.jid << ", compute_time=" << job.compute_time << ";\x1b[0m" << endl;
                        cout<<"jobs_completed="<<(buffer->job_completed)<<", jobs_created="<< buffer->job_created << ";" << endl;
                        pthread_mutex_unlock(mutex_ptr); // unlock mutex
                    }
                    // check if buffer is full
                    else if (buffer->size >= QUEUE_SIZE) {
                        cout<<"\x1b[34;1mProducer: "<<i<<"\x1b[0m\x1b[33m Waiting, (queue full)\x1b[0m"<<endl;
                        pthread_mutex_unlock(mutex_ptr); // unlock mutex
                        usleep(MIN_SLEEP_TIME); // sleep to wait for buffer to free up
                    }
                    // all jobs have been created
                    else {
                        // kill process by exiting
                        cout << "\x1b[31;1mKilling Producer: " << i << "\x1b[0m" << endl;
                        pthread_mutex_unlock(mutex_ptr);
                        exit(0);
                    }
                }
                
            }
            else // when index is greater than or equal to NP, create a consumer process
            {
                // consumer code
                while (1)
                {   
                    pthread_mutex_lock(mutex_ptr); // lock mutex
                    /* if buffer is non empty and number of jobs completed is less than NJOBS 
                    (number of jobs to be completed) */
                    if (buffer->job_completed < NJOBS && buffer->size > 0)
                    {
                        process_t job = remove_sorted_buffer(buffer); // get new job from buffer, and remove it from buffer 
                        (buffer->job_completed)++; // increment number of jobs completed counter

                        cout << "\x1b[37m\x1b[7m\x1b[1m<JOB>\x1b(B\x1b[m " << "\x1b[34;1mConsumer: " << i-NP << "\x1b[0m\x1b[32m pid=" << getpid() << ";\x1b[0m" << " consumed " << "\x1b[34;1mProducer: " << job.producer_number << "\x1b[0m\x1b[32m pid= " << job.pid << ", priority=" << (job.priority) << " jid=" << job.jid << " compute_time=" << job.compute_time << ";\x1b[0m \x1b[37m\x1b[7m\x1b[1m</JOB>\x1b(B\x1b[m" << endl; 
                        cout << "jobs_completed=" << (buffer->job_completed) << ", jobs_created=" << buffer->job_created << ";" << endl;

                        pthread_mutex_unlock(mutex_ptr); // unlock mutex
                        usleep(job.compute_time * MIN_SLEEP_TIME); 
                        /* sleep for time equal to compute time
                         min sleep time is quite simply one second in microseconds
                         so the process sleeps for compute_time seconds */
                    }
                    // check if all jobs have been consumed
                    else if (buffer->job_completed >= NJOBS) {
                        cout << "\x1b[31;1mKilling Consumer: " << i-NP << "\x1b[0m" <<endl;
                        pthread_mutex_unlock(mutex_ptr); // unlock mutex
                        exit(0);
                    }
                    // no process in buffer, but more processes will be created (number of jobs created is less than NJOBS)
                    else {
                        cout<<"\x1b[34;1mConsumer: "<<i-NP<<"\x1b[0m\x1b[33m Waiting, (queue empty)\x1b[0m"<<endl;
                        pthread_mutex_unlock(mutex_ptr); // unlock mutex
                        usleep(MIN_SLEEP_TIME);          
                    }
                }
            }
        }
    }
    // make parent process wait for each of the child processes to complete
    for (int i = 0; i < NC+NP; i++)
        wait(NULL);

    time_t end_time = time(NULL); // caclulate time after which all processes are killed
    cout << "\x1b[37m\x1b[7m\x1b[1m time_elapsed=" << end_time - start_time << " \x1b(B\x1b[m" << endl;
    
    // detach shared memory and mark it for deletion
    shmdt(buffer); // detach shared memory segment
    shmctl(shmid, IPC_RMID, NULL); // mark shared memory segment for deletion
}
