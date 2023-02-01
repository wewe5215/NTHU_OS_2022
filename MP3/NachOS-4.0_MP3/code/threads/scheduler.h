// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();		// Initialize list of ready threads 
    ~Scheduler();		// De-allocate ready list
    void update_priority(); // MP3 109062233
    void scheduling();// MP3 109062320
    void Re_Sort_List();
    void ReadyToRun(Thread* thread);	
    				// Thread can be dispatched.
    Thread* FindNextToRun();	// Dequeue first thread on the ready 
				// list, if any, and return thread.
    void Run(Thread* nextThread, bool finishing);
    				// Cause nextThread to start running
    void CheckToBeDestroyed();// Check if thread that had been
    				// running needs to be deleted
    void Print();		// Print contents of ready list
    bool L1_empty(){
      return L1_list->IsEmpty();
    };
    bool L2_empty(){
      return L2_list->IsEmpty();
    };
    bool L3_empty(){
      return L3_list->IsEmpty();
    };
    //modified MP3 109062320
    Thread* front_of_L1(){
      return L1_list->Front();
    };
    Thread* front_of_L2(){
      return L2_list->Front();
    };
    Thread* front_of_L3(){
      return L3_list->Front();
    };
    // SelfTest for scheduler is implemented in class Thread
    
  private:
    // added MP3
    // List<Thread *> *readyList;  // queue of threads that are ready to run,
				// but not running
    SortedList<Thread *> *L1_list;
    SortedList<Thread *> *L2_list;
    List<Thread *> *L3_list;
    
    // end adding MP3 109062233 
    Thread *toBeDestroyed;	// finishing thread to be destroyed
    				// by the next thread that runs
};

#endif // SCHEDULER_H
