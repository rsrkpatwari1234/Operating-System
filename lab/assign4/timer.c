#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
  
/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Assignment 3 : Code added */
/*prototype function for the comparator*/
static bool compare_ticks(struct list_elem *a, struct list_elem *b);   

/*List to keep track of threads*/
struct list sleeping_threads;    
/* Assignment 3 : Code ended */

/* Assignment 4 : Part 1 : Code added */
// stores the earliest absolute time ticks, at which a thread needs to be unblocked. It is basically equal to the value of the front of the priority queue
int64_t NEXT_WAKEUP_TIME = -1;
// function to share the next wakeup time across files
int64_t get_next_wakeup_time();
// function executed by the wakeup_thread 
void wakeup_thread_function();
/* Assignment 4 : Part 1 : Code ended */

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void) 
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");

  /* Assignment 3 : Code added */
  list_init(&sleeping_threads);
  /* Assignment 3 : Code ended */

  /* Assignment 4 : Part 1 : Code added */
  // wakeup thread is only created for part 1, (when mlfqs is not being tested)
  if(!thread_mlfqs)
    thread_create("wakeup_thread",PRI_MAX,&wakeup_thread_function,NULL); //creating the wakeup thread
  /* Assignment 4 : Part 1 : Code ended */

}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void) 
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1)) 
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (high_bit | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}


/* Assignment 3 : Code added */
/* A comparator for comparing the thread's sleep time for the ordered lists */
bool
compare_ticks(struct list_elem *a, struct list_elem *b){
  return list_entry(a, struct thread, elem)->abs_ticks < list_entry(b, struct thread, elem)->abs_ticks;
}
/* Assignment 3 : Code ended*/

/* Assignment 4 : Part 1 : Code added */
// function definition of get_next_wakeup_time
int64_t get_next_wakeup_time(){
  return NEXT_WAKEUP_TIME;
}

//function for the managerial wakeup thread that wakes up the sleeping threads
void wakeup_thread_function(){

  while(1){
    /* Check and wake up sleeping threads. */
    struct thread *t;
    while(!list_empty(&sleeping_threads)) {
      
      t = list_entry(list_front(&sleeping_threads), struct thread, elem);
      if (timer_ticks() < t->abs_ticks)
        break;
      
      list_pop_front (&sleeping_threads);
      t->abs_ticks = 0;
      thread_unblock(t);
    }
    //updating the next wakeup time   
    if(!list_empty(&sleeping_threads))
      NEXT_WAKEUP_TIME = list_entry(list_front(&sleeping_threads),struct thread, elem)->abs_ticks;
    else
      NEXT_WAKEUP_TIME = -1;

    intr_disable(); //disabling interrupts
    thread_block();
    intr_enable(); //enabling the interrupts
  }
  
}
/* Assignment 4 : Part 1 : Code ended */

/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks) 
{
  /* Assignment 4 : Part 2 : Code added and removed */

  struct thread *curr;
  enum intr_level old_level;

  ASSERT (intr_get_level () == INTR_ON);

  if (ticks <= 0)
    return;

  old_level = intr_disable ();

  /* Get current thread and set wakeup ticks. */
  curr = thread_current ();
  curr->abs_ticks = timer_ticks () + ticks;

  /* Insert current thread to ordered sleeping list */
  list_insert_ordered(&sleeping_threads, &curr->elem, compare_ticks, NULL);
  
  /* Assignment 4 : Part 1 : Code added */

  /* Updating the vaule of the next wake up time, according to the value of the front of the priority queue 
     The front has least value of abs_ticks, i.e. it is to be woken first out of all sleeping threads */
  NEXT_WAKEUP_TIME = list_entry(list_front(&sleeping_threads),struct thread, elem)->abs_ticks;
  
  /* Assignment 4 : Part 1 : Code ended */
  
  thread_block ();

  intr_set_level (old_level);

  /* Assignment 3 : Code removed */
  /*int64_t start = timer_ticks ();
  ASSERT (intr_get_level () == INTR_ON);
  while (timer_elapsed (start) < ticks) 
    thread_yield ();*/
  /* Assignment 3 : Code ended */

  //============================================

  /* Assignment 3 : Code added */
  
  /*struct thread *t = thread_current();
  int64_t start = timer_ticks();  //start of the timer
    
  ASSERT (intr_get_level () == INTR_ON);    //make sure interrupts are on
  t->abs_ticks = start + ticks;   // assign absolute time, for which the process should sleep
  
  intr_disable();   
  list_insert_ordered(&sleeping_threads, &t->elem, compare_ticks, NULL);
  thread_block();
  intr_enable();*/
  
  /* Assignment 3 : Code ended */

  /* Assignment 4 : Part 2 : Code ended */

}

/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void
timer_msleep (int64_t ms) 
{
  real_time_sleep (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void
timer_usleep (int64_t us) 
{
  real_time_sleep (us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void
timer_nsleep (int64_t ns) 
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.
   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void
timer_mdelay (int64_t ms) 
{
  real_time_delay (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.
   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void
timer_udelay (int64_t us) 
{
  real_time_delay (us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.
   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void
timer_ndelay (int64_t ns) 
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  /* Assignment 4 : Part 2 : Code added and removed */
	
  ticks++;
  thread_tick ();

  /* Actions for Multilevel feedback queue scheduler. */
  if (thread_mlfqs)
  {
	  thread_mlfqs_incr_recent_cpu ();
	  if (ticks % TIMER_FREQ == 0)
	    thread_mlfqs_refresh ();
	  else if (ticks % 4 == 0)
	    thread_mlfqs_update_priority (thread_current ());

    bool preempt = false;

    /* Check and wake up sleeping threads. */
    struct thread *t;
    while(!list_empty(&sleeping_threads)) {
      
      t = list_entry(list_front(&sleeping_threads), struct thread, elem);
      if (timer_ticks() < t->abs_ticks)
        break;
      
      list_pop_front (&sleeping_threads);
      thread_unblock(t);
      preempt = true;
    }

    if (preempt)
      intr_yield_on_return ();
  }

  /* Assignment 3 : Code removed */
  /*ticks++;
  thread_tick ();*/
  /* Assignment 3 : Code ended */

  /* Assignment 3 : Code added */
  /*ticks++;
  thread_tick ();
  
  struct thread *t;
  while(!list_empty(&sleeping_threads)) {
    
    t = list_entry(list_front(&sleeping_threads), struct thread, elem);
    if (timer_ticks() < t->abs_ticks)
      break;
    
    list_pop_front (&sleeping_threads);
    thread_unblock(t);
  }*/
  /* Assignment 3 : Code ended */

  /* Assignment 4 : Part 2 : Code ended */
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops) 
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait (loops);

  /* If the tick count changed, we iterated too long. */
  barrier ();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.
   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops) 
{
  while (loops-- > 0)
    barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom) 
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.
          
        (NUM / DENOM) s          
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks. 
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         timer_sleep() because it will yield the CPU to other
         processes. */                
      timer_sleep (ticks); 
    }
  else 
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      real_time_delay (num, denom); 
    }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000)); 
}