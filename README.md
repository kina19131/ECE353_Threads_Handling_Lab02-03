 you will construct user threads by implementing a set of functions that your program will call directly to provide the illusion of concurrency.


*User-level vs. Kernel-level Threads*
Multiprocessing
  user-level threads provide the illusion of concurrency

  you will allow programs to multiplex some number (e.g., m) of user-level threads on one kernel thread. 

  At most one user-level thread is running at a time.
  -> your runtime system has complete control over the interleaving of user-level threads with each other. 

Asynchronous I/O
  When a user-level thread makes a system call that blocks (e.g., reading a file from disk), the kernel scheduler moves the process to the Blocked state and will not schedule it until the I/O has completed. Thus, even if there are other user-level threads within that process, they have to wait.

Timer interrupts
  smt



*Using Threads*
  Typical program with 'threads library' look like: 
    int
    main(int argc, char **argv)
    {
        // Some initialization
        // Create some threads
        // wait for threads to finish
        // exit
    }

    // "main" function for thread i
    thread_main_i (...)
    {
        // do some work
        // yield
        // do some more work
        // return (implicit thread exit)
    }

  Thread exits by: 
    1. explicitly: when calls the 'thread_exit' fnc in the thread library
    2. implicitly: when 'thread_main' fnc returns
    3. 'thread_kill' - to force other threads to exit as well 


*Thread Context*
  Each thread has:
    per-thread state =  working state of the thread
    -> includes: 
    -> program counter, local var, stack, etc. 
    -> 'thread context': subset of this state, 
        must be saved/restored from the processor when switching threads.
    