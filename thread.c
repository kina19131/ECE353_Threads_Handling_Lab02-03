#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>

#include <math.h> 
#include <stdbool.h>

#include "thread.h"
#include "interrupt.h"

/* 4 POSSIBLE THREAD STATES + 1 INITIAL STATE */
enum{ THREAD_INIT = 0, 
THREAD_READY = 1,
THREAD_RUNNING = 2,
THREAD_KILLED = 3,
THREAD_EXITED = 4
}; 

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
};

/* STRUCTURES */
struct thread {
	Tid tid; //Thread ID 
	int state; //State (Enumerated above)
	ucontext_t context; //Context
	void* sp; //stack pointer
	struct thread* next;
	struct thread* prev; 
};

struct queue{
	struct thread* first;
	struct thread* last; 
} queue;


/* GLOBAL VARIABLE */ 
static struct thread Thread_Array[THREAD_MAX_THREADS]; // List of Threads - dataset
static struct queue ready_queue; // Ready Queue
volatile static Tid my_thread_tid; // Tid of the currently running thread
volatile int count_threads; // Current number of threads 


/* ########## QUEUE FUNCTIONS ########## */
/* GRAB THE THREAD FROM READY QUEUE */
Tid thread_pop(Tid id, struct queue* que){
	struct thread* thr = que->first;
	/* pop any; the first in the ready queue */
	if (id == THREAD_MAX_THREADS+1){
		if(thr == NULL){ 
			return (THREAD_NONE);
		}
		que->first = thr->next; 
		if(que->first == NULL){
			que->last=NULL;
		}
		else{
			que->first->prev = NULL; 
		}
		Tid ret_id = thr -> tid; 
		free(thr); 
		return(ret_id);
	}

	/* Have a specific one to pop */
	while (thr != NULL){
		if (thr -> tid == id){
			// what's thr's prev thread
			if(thr -> next == NULL){
				que -> last  = thr -> prev; 
			}
			else{
				thr -> next -> prev = thr -> prev;
			}
			// what's thr's next thread
			if (thr -> prev == NULL){
				que -> first = thr -> next;
			}
			else{
				thr -> prev -> next = thr -> next;
			}
			Tid ret_id = thr -> tid; 
			free(thr);
			return (ret_id); 
		}
		thr = thr -> next;
	}
	return (THREAD_INVALID);
}

/* PUT INTO THE READY QUEUE */
void thread_push(Tid id, struct queue* que){
	struct thread* thr = (struct thread*)malloc(sizeof(struct thread)); 
	assert(thr);  
	thr->next = NULL; // set its own next to NULL
	thr->tid = id; 
	if (que -> first == NULL){
		que ->first = thr;
	}
	else{
		que->last->next = thr;
	}
	thr -> prev = que -> last; 
	que -> last = thr;
}

/* ############# MAIN ############# */

void
thread_stub(void (*thread_main)(void *), void *arg)
{
	/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
	thread_main(arg); // call thread_main() function with arg
	thread_exit();
	exit(0);
}

void
thread_init(void)
{ 
	// INCREMENTING GLOBAL THREAD COUNT
	count_threads = 1;
	// INITIALIZE THREADS IN Theard_Array
	for (int i = 0; i < THREAD_MAX_THREADS; i++){
		Thread_Array[i].state = THREAD_INIT;
	}
	
	// SET UP FIRST THREAD 'main()'
	my_thread_tid = 0; 
	Thread_Array[my_thread_tid].state = THREAD_RUNNING; 
	int err = getcontext(&Thread_Array[my_thread_tid].context);
	assert(!err);
}

Tid
thread_id()
{
	if (&Thread_Array[my_thread_tid] != NULL){
		return(my_thread_tid);
	}
	else{
		return THREAD_INVALID;
	}
}


Tid thread_create(void (*fn) (void *), void *parg)
{	
	if(count_threads >= THREAD_MAX_THREADS){ // Edge Case: Thread Count Beyond Max Count
		return(THREAD_NOMORE);
	}
	
	// MALLOC STACK
	void *sp = malloc(THREAD_MIN_STACK); 
	if (sp == NULL){ // Edge Case: stack is full
		return(THREAD_NOMEMORY);
	}

	unsigned int create_ind; 

	// FIND THE INDEX FOR NEW THREAD
	for (unsigned int i = 0 ; i < THREAD_MAX_THREADS; i++){ //ALREADY 'TOUCHED' UPON THREADS
		if ((Thread_Array[i].state == THREAD_INIT || Thread_Array[i].state == THREAD_KILLED || Thread_Array[i].state == THREAD_EXITED)){
			getcontext(&Thread_Array[i].context);
			create_ind = i; 
			break;
		}
	}

	// SET STATE
	Thread_Array[create_ind].sp = sp; // Setting stack pointer
	Thread_Array[create_ind].state = THREAD_READY; //changing the state
	Thread_Array[create_ind].context.uc_stack.ss_sp = Thread_Array[create_ind].sp; //store 
	Thread_Array[create_ind].context.uc_stack.ss_size = THREAD_MIN_STACK;
	Thread_Array[create_ind].context.uc_stack.ss_flags = 0; 
	Thread_Array[create_ind].context.uc_link = NULL; 
	
	// UPDATE SP 
	sp = Thread_Array[create_ind].context.uc_stack.ss_sp;
	sp += Thread_Array[create_ind].context.uc_stack.ss_size; 
	sp -= (unsigned long long) sp % 16; 
	sp -= 8;

	// SET KEY REGISTERS
	Thread_Array[create_ind].context.uc_mcontext.gregs[REG_RSP] = (long long int)sp;
	Thread_Array[create_ind].context.uc_mcontext.gregs[REG_RIP] = (long long int)&thread_stub;
	Thread_Array[create_ind].context.uc_mcontext.gregs[REG_RDI] = (long long int)fn;
	Thread_Array[create_ind].context.uc_mcontext.gregs[REG_RSI] = (long long int)parg;
	
	// CREATED THREAD PUT INTO READY QUEUE, INCREMENT # OF THREADS
	thread_push(create_ind, &ready_queue); 
	count_threads++; 

	return(create_ind);
}

Tid
thread_yield(Tid want_tid)
{	
	volatile bool popped = false; 

	/* when exiting the program */
	if (want_tid == -(THREAD_MAX_THREADS+1)){
		for (unsigned int i = 1; i < THREAD_MAX_THREADS; i++){
			// exluding the current one, cleaning EXITED THREADS
			if (i != my_thread_tid && Thread_Array[i].state == THREAD_EXITED){
				Thread_Array[i].state = THREAD_INIT;
				free(Thread_Array[i].sp);
			}
		}
		free(Thread_Array[my_thread_tid].sp);
		return(THREAD_NONE);
	}

	/* tid: THREAD_SELF */
	if (want_tid == THREAD_SELF || want_tid == my_thread_tid) {
		return (my_thread_tid); 
	}

	/* tid: THREAD_ANY */
	else if (want_tid == THREAD_ANY){
		// Edge case: no other thread to yield to 
		if (count_threads == 1 && Thread_Array[my_thread_tid].state == THREAD_RUNNING){
			return (THREAD_NONE);
		}
		else{
			want_tid = thread_pop(THREAD_MAX_THREADS+1, &ready_queue); //grab from ready queue
			popped = true; 
		}
	}

	/* THREAD_INVALID cases */
	if (want_tid > THREAD_MAX_THREADS || want_tid < -1 ){ // OUt of Range
		return(THREAD_INVALID);
	}
	
	if (Thread_Array[want_tid].state != THREAD_READY && Thread_Array[want_tid].state != THREAD_RUNNING && Thread_Array[want_tid].state != THREAD_KILLED){ //state invalid
		return (THREAD_INVALID);
	}

	/* Specific Tid given */
	ucontext_t uc; 
	volatile bool repetition = false; 
	int err = getcontext(&uc);
	assert(!err);

	// Taking care of Thread_Array prior to main yield operation 
	for (unsigned i = 1; i < THREAD_MAX_THREADS; i++){ 
		// exluding the current one, cleaning EXITED THREADS
		if (i != my_thread_tid && Thread_Array[i].state == THREAD_EXITED){
			Thread_Array[i].state = THREAD_INIT;
			free(Thread_Array[i].sp);
		}
	}

	// Checking my_thread_tid, Assuring its not KILLED Thread
	if (Thread_Array[my_thread_tid].state == THREAD_KILLED){
		thread_exit(); 
	}
	
	if (!repetition){
		repetition = true; // Making sure no infinite loop 
		// If RUNNING, change to READY and add into READY QUEUE
		if (Thread_Array[my_thread_tid].state == THREAD_RUNNING){
			Thread_Array[my_thread_tid].state = THREAD_READY; //change current one to READY state
			thread_push(my_thread_tid, &ready_queue);//put into the ready queue
		}
		
		// State of the wanted thread check and changing its state to RUNNING
		if (Thread_Array[want_tid].state != THREAD_KILLED){
			Thread_Array[want_tid].state = THREAD_RUNNING; 
		}

		Thread_Array[my_thread_tid].context = uc; 
		uc = Thread_Array[want_tid].context;
		my_thread_tid = want_tid; //update the global TID of current running thread
		
		if (!popped){ // if was THREADY_ANY, already popped from above
			thread_pop(want_tid, &ready_queue);
		}
		err = setcontext(&uc);
	}
	return (want_tid);
}

void
thread_exit()
{
	if (count_threads > 1){
		Thread_Array[my_thread_tid].state = THREAD_EXITED;
		count_threads--;
		thread_yield(THREAD_ANY);
	}

	else if (count_threads == 1 && Thread_Array[my_thread_tid].next == NULL){
		Thread_Array[my_thread_tid].state = THREAD_EXITED;
		thread_yield(-(THREAD_MAX_THREADS+1));
	}

}

Tid
thread_kill(Tid tid)
{	
	/* invalid tid */
	if (tid <= 0 || tid >= THREAD_MAX_THREADS){
		return THREAD_INVALID;
	}

	/* can't kill currently running or haven't been used thread */
	else if (Thread_Array[tid].state == THREAD_RUNNING ||Thread_Array[tid].state == THREAD_INIT){
		return (THREAD_INVALID);
	}


	else if (Thread_Array[tid].state == THREAD_READY){
		Thread_Array[tid].state = THREAD_KILLED;
	}

	return (tid);
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
