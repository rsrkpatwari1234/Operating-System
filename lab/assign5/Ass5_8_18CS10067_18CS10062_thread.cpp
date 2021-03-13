// Atharva Naik, 18CS10067
// Radhika Patwari, 18CS10062
// Assignment 5
// Part 2

#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <bits/stdc++.h>

// ranges of process variables
#define MIN_PRIORITY 1
#define MAX_PRIORITY 10
#define MIN_JOB_ID 1
#define MAX_JOB_ID 100000
#define MIN_COMPUTE_TIME 1
#define MAX_COMPUTE_TIME 4
#define MIN_SLEEP_TIME 1000000 // in microseconds 
#define MAX_SLEEP_TIME 3000000 // in microseconds

using namespace std;

// check if mutex is defined/suported in the system
#ifndef _POSIX_THREAD_PROCESS_SHARED
    #error No definition found for mutex
    exit(EXIT_FAILURE);
#endif

const int QUEUE_SIZE = 8;
pthread_mutex_t    *mutex_ptr; // pointer to mutex
pthread_mutexattr_t mutex_attr; // mutex attribute

struct process_t{
    int pid; // process id
    int producer_number; // process number corresponding to no. of producers 
    int priority; // priority of the number ( between 1 and 10 )
    int compute_time; // the compute time
    int jid; // the job id
};

// comparing priority of jobs while inserting them in priority queue of the buffer
// insert : O(log QUEUE_SIZE) ; delete and rearrangement : O(log QUEUE_SIZE)
struct ComparePriority {
    bool operator()(process_t const& job1, process_t const& job2)
    {
        // return "true" if "job1" is ordered 
        // before "job2", ensures job with highest priority is 
        // at the top of the heap (max heap implementation) 
        return job1.priority < job2.priority;
    }
};

struct process_queue_buffer{
    int size; // current buffer size (number of elements stored in buffer currently)
    int job_created; // number of jobs created
    int job_completed; // number of jobs completed
    pthread_mutex_t mutex; // mutex
    priority_queue<process_t, vector<process_t>, ComparePriority> process_queue; // priority queue of processes
};

process_queue_buffer* buffer = new process_queue_buffer;
int NJOBS, producer_count = 0, consumer_count = 0;

// populate process structure with given pid and producer number
process_t create_job(int pid, int producer_number)
{
    //srand(pid*time(0)); // seed random number generator with the pid
    process_t job; // create process structure
    // populate data members of the process
    job.pid = pid;
    job.producer_number = producer_number;
    job.priority = rand() % MAX_PRIORITY + MIN_PRIORITY; // value ranges from 1 to 10
    job.compute_time = rand() % MAX_COMPUTE_TIME + MIN_COMPUTE_TIME; // value ranges from 1 to 4
    job.jid = rand() % MAX_JOB_ID + MIN_JOB_ID; // value ranges from 1 to 100000
    
    return job;
}

// inserting job in the priority queue such that job with highest priority is at the root
// of the max heap
void insert_in_buffer(process_t job)
{
    (buffer->process_queue).push(job);  // pushing job in priority queue
    (buffer->size)++;  // increment buffer sizes
}

// remove job with maximum priority value as it is consumed by one of the consumers
process_t remove_from_buffer()
{
    process_t job_with_max_priority = (buffer->process_queue).top();
    (buffer->process_queue).pop();  // popping the max priority job
    (buffer->size)--; // reduce size (number of processes present in buffer)

    cout<<"\x1b[44;1m Killing job of priority ="<<job_with_max_priority.priority<<" \x1b[0m"<<endl; // #REMOVE#l;

    return job_with_max_priority;
}

void *producer_task(void *param){

    int producer_num = producer_count;
    producer_count++;
    // producer code
    while (1)
    {   
        process_t job = create_job(getpid(), producer_num);
        usleep(rand() % MAX_SLEEP_TIME);
        pthread_mutex_lock(mutex_ptr);
        // check if more jobs are to be crated and buffer has remaining space
        if ((buffer->job_created) < NJOBS && buffer->size < QUEUE_SIZE) {
            insert_in_buffer(job);
            (buffer->job_created)++;
            cout << "\x1b[34;1mProducer: " << producer_num << "\x1b[0m\x1b[32m pid=" << getpid() << ", priority=" << (job.priority) << ", jid=" << job.jid << ", compute_time=" << job.compute_time << ";\x1b[0m" << endl;
            cout<<"jobs_completed="<<(buffer->job_completed)<<", jobs_created="<< buffer->job_created << ";" << endl;
            pthread_mutex_unlock(mutex_ptr); // unlock mutex
        }
        // check if buffer is full
        else if (buffer->size >= QUEUE_SIZE) {
            cout<<"\x1b[34;1mProducer: "<< producer_num <<"\x1b[0m\x1b[33m Waiting, (queue full)\x1b[0m"<<endl;
            pthread_mutex_unlock(mutex_ptr); // unlock mutex
            usleep(MIN_SLEEP_TIME); // sleep
        }
        // all jobs have been created
        else {
        	cout << "\x1b[31;1mKilling Producer: " << producer_num << "\x1b[0m" << endl;
            pthread_mutex_unlock(mutex_ptr);
            pthread_exit(0);
        }
    }
}

void *consumer_task(void *param){
    int consumer_num = consumer_count;
    consumer_count++;
    // consumer code
    while (1)
    {   
        pthread_mutex_lock(mutex_ptr); // lock mutex
        if (buffer->job_completed < NJOBS && buffer->size > 0)
        {
            process_t job = remove_from_buffer(); // get new job from buffer
            (buffer->job_completed)++;

            cout << "\x1b[37m\x1b[7m\x1b[1m<JOB>\x1b(B\x1b[m " << "\x1b[34;1mConsumer: " << consumer_num << "\x1b[0m\x1b[32m pid=" << getpid() << ";\x1b[0m" << " consumed " << "\x1b[34;1mProducer: " << job.producer_number << "\x1b[0m\x1b[32m pid= " << job.pid << ", priority=" << (job.priority) << " jid=" << job.jid << " compute_time=" << job.compute_time << ";\x1b[0m \x1b[37m\x1b[7m\x1b[1m</JOB>\x1b(B\x1b[m" << endl; 
            cout << "jobs_completed=" << (buffer->job_completed) << ", jobs_created=" << buffer->job_created << ";" << endl;

            pthread_mutex_unlock(mutex_ptr);
            usleep(job.compute_time * MIN_SLEEP_TIME);
        }
        // check if all jobs have been consumed
        else if (buffer->job_completed >= NJOBS) {
            cout << "\x1b[31;1mKilling Consumer: " << consumer_num << "\x1b[0m" <<endl;
            pthread_mutex_unlock(mutex_ptr); // unlock mutex
            pthread_exit(0);
        }
        // no process in buffer, but more processes will be created (number of jobs created is less than NJOBS)
        else {
        	cout<<"\x1b[34;1mConsumer: "<< consumer_num <<"\x1b[0m\x1b[33m Waiting, (queue empty)\x1b[0m"<<endl;
            pthread_mutex_unlock(mutex_ptr); // unlock mutex
            usleep(MIN_SLEEP_TIME);          
        }
    }
}

int main()
{   
    int i, NC, NP;

    cout << "number of producers=";
    cin >> NP;
    cout << "number of consumers=";
    cin >> NC;
    cout << "number of jobs=";
    cin >> NJOBS;

    buffer->size = 0;
    buffer->job_created=0;
    buffer->job_completed = 0;
    mutex_ptr = &(buffer->mutex);

    int error_code;
    // setup mutex and catch any errors that might occur
    if (error_code = pthread_mutexattr_init(&mutex_attr))
    {
        fprintf(stderr,"pthread_mutexattr_init: %s", strerror(error_code)); 
        exit(EXIT_FAILURE);
    }
    if (error_code = pthread_mutexattr_setpshared(&mutex_attr,PTHREAD_PROCESS_SHARED))
    {
        fprintf(stderr,"pthread_mutexattr_setpshared %s",strerror(error_code));
        exit(EXIT_FAILURE);
    }
    if (error_code = pthread_mutex_init(mutex_ptr, &mutex_attr))
    {
        fprintf(stderr,"pthread_mutex_init %s",strerror(error_code)); 
        exit(EXIT_FAILURE);
    }
    time_t start_time = time(NULL); 

    pthread_t producer_thread[NP];
    pthread_t consumer_thread[NC];

    pthread_attr_t attr;
    pthread_attr_init (&attr); // get default attributes

    //cout<<"start creating producers...."<<endl;
    // create producer threads
    for (int i = 0; i < NP; i++){
        pthread_create(&producer_thread[i], &attr, producer_task, &producer_thread[i]); // create the thread
        //cout<<"\n"<<producer_thread[i]<<endl;
    }

    //cout<<"start creating consumers...."<<endl;
    // create consumer threads
    for (i = 0; i < NC; i++)
        pthread_create(&consumer_thread[i], &attr, consumer_task, NULL); // create the thread
    // join producer threads (kill them)
    for (i = 0; i < NP; i++)
        pthread_join(producer_thread[i], NULL); //wait for the threads to exit
    // join consumer threads (kill them)
    for (i = 0; i < NC; i++)
        pthread_join(consumer_thread[i], NULL); //wait for the threads to exit

    time_t end_time = time(NULL); 
    
    cout << "\x1b[37m\x1b[7m\x1b[1m time_elapsed=" << end_time - start_time << " \x1b(B\x1b[m" << endl;

    return 0;
}