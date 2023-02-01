// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"
//modified by 109062320
/* MP3 add the comparison function for the SortedList 109062233*/
//modified because of the change of spec MP3
int sjf(Thread* First , Thread* Second){
    int retVal = 0;
    double rem_burst_time1 = (First -> getBurstTime()) - (First -> getAccumulatedTime());
    double rem_burst_time2 = (Second -> getBurstTime()) - (Second -> getAccumulatedTime());
    if(rem_burst_time1 < rem_burst_time2)
        retVal = -1;
    else if(rem_burst_time1 == rem_burst_time2)
        retVal = 0;
    //only when First->getBurstTime() > Second->getBurstTime() need to switch
    else
        retVal = 1;
    return retVal;
}
int prior_cmp(Thread* First, Thread* Second){
    int retVal = 0;
    if(First->getPriority() > Second->getPriority())
        retVal = -1;
    else if(First->getPriority() == Second->getPriority())
        retVal = 0;
    else
        retVal = 1;
    return retVal;
}
/*End the comp func*/



/* end the function */
//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    // add MP3 109062233
    L1_list = new SortedList<Thread *>(sjf);
    L2_list = new SortedList<Thread *>(prior_cmp);
    L3_list = new List<Thread *>;
    toBeDestroyed = NULL;
} 

//MP3 109062320 modified
/* MP3 109062233 add update_priority */ \
void
Scheduler::update_priority(){

    int tick = kernel->stats->totalTicks;
    
    /* I don't know if I need to set the update the tick or not ~ !!! But I didn't !!! */
    // update the  priority of L1 , L2 and L3
    ListIterator<Thread *> *iter;
    // L1
    if(!L1_list->IsEmpty()){
        for (ListIterator<Thread *> *iter = new ListIterator<Thread* >(L1_list); !iter->IsDone(); iter->Next() ) {
            iter->Item()->aging(tick);
        }
    }
    
    // L2
    if(!L2_list->IsEmpty()){
        for (ListIterator<Thread *> *iter = new ListIterator<Thread* >(L2_list); !iter->IsDone(); iter->Next() ) {
            iter->Item()->aging(tick);
        
        }
    }
    
    // L3
    if(!L3_list->IsEmpty()){
        for (ListIterator<Thread *> *iter = new ListIterator<Thread* >(L3_list); !iter->IsDone(); iter->Next() ) {
            iter->Item()->aging(tick);
        
        }
    }
    
    return;

}
/* MP3 109062320 add scheduling */
void
Scheduler::scheduling(){

    //re-schedule threads in L2, L3
    ListIterator<Thread *> *iter;
    Thread* to_be_prioritized;
    if(!L2_list->IsEmpty()){
        for (ListIterator<Thread *> *iter = new ListIterator<Thread* >(L2_list); !iter->IsDone(); iter->Next() ) {
            to_be_prioritized = iter->Item();
            int thread_priority = iter->Item()->getPriority();
            // transfer the thread from L2 to L1
            if(thread_priority > 99){
                this->L2_list->Remove(to_be_prioritized);
                DEBUG(dbgMP3 , "[B] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << to_be_prioritized->getID() << "] is removed from queue L[2]\n");
                this->L1_list->Insert(to_be_prioritized);
                DEBUG(dbgMP3 , "[A] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << to_be_prioritized->getID() << "] is inserted into queue L[1]\n");
            }
        }
    }
    
    if(!L3_list->IsEmpty()){
        for (ListIterator<Thread *> *iter = new ListIterator<Thread* >(L3_list); !iter->IsDone(); iter->Next() ) {
            to_be_prioritized = iter->Item();
            int thread_priority = iter->Item()->getPriority();
            // put the thread from L3 to L2
            if(thread_priority > 49){
                to_be_prioritized = iter->Item();
                this->L3_list->Remove(iter->Item());
                DEBUG(dbgMP3 , "[B] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << to_be_prioritized->getID() << "] is removed from queue L[3]\n");
                this->L2_list->Insert(to_be_prioritized);
                DEBUG(dbgMP3 , "[A] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << to_be_prioritized->getID() << "] is inserted into queue L[2]\n");
            }
            // put the thread from L3 to L1
            else if(thread_priority > 99){
                this->L3_list->Remove(to_be_prioritized);
                DEBUG(dbgMP3 , "[B] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << to_be_prioritized->getID() << "] is removed from queue L[3]\n");
                this->L1_list->Insert(to_be_prioritized);
                DEBUG(dbgMP3 , "[A] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << to_be_prioritized->getID() << "] is inserted into queue L[1]\n");
            }
        }
    }
    


    
}
/* MP3 109062320 add re sort list*/
//resort L1, L2 
void 
Scheduler::Re_Sort_List(){
    SortedList<Thread *> *tmp_L1  = new SortedList<Thread *>(sjf);;
    SortedList<Thread *> *tmp_L2  = new SortedList<Thread *>(prior_cmp);
    // L1
    if(!L1_list->IsEmpty()){
        for (ListIterator<Thread *> *iter = new ListIterator<Thread* >(L1_list); !iter->IsDone(); iter->Next() ) {
            tmp_L1->Insert(iter->Item());
        }
        delete L1_list;
        L1_list = tmp_L1;
    }
    
    // L2
    if(!L2_list->IsEmpty()){
        for (ListIterator<Thread *> *iter = new ListIterator<Thread* >(L2_list); !iter->IsDone(); iter->Next() ) {
            tmp_L2->Insert(iter->Item());
        }
        delete L2_list;
        L2_list = tmp_L2;
    }
    
}
//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    // add MP3 109062233
    delete L1_list;
    delete L2_list;
    delete L3_list;
    // delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    /* modify and add MP3 109062233*/
    /*In case of
    running to ready (interrupted), its CPU burst time T must keep accumulating after it
    resumes running.*/
    thread->setReadyTick(kernel->stats->totalTicks);
    int priority = thread->getPriority();
    if(priority < 50){
        L3_list->Append(thread);
        DEBUG(dbgMP3 , "[A] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[3]\n");
    }
    else if(priority < 100){
        L2_list->Insert(thread);
        DEBUG(dbgMP3 , "[A] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[2]\n");
    }
    else{
        L1_list->Insert(thread);
        DEBUG(dbgMP3 , "[A] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[1]\n");
    }
    //readyList->Append(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun()
{
    /*MP3 109062320 modified start*/
    Re_Sort_List();
    /*modified end*/
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    // modify MP3 109062233
    Thread* next;
    if(!L1_list->IsEmpty()){
        next =  L1_list->RemoveFront();
        /*MP3 109062320 modified start*/
        //When the thread turns into running state, the waiting time should be reset
        next->setRunTick(kernel->stats->totalTicks);
        /*modified end*/
        DEBUG(dbgMP3 , "[B] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << next->getID() << "] is removed from queue L[1]\n");
        return next;
    }
    else if(!L2_list->IsEmpty()){
        next =  L2_list->RemoveFront();
        /*MP3 109062320 modified start*/
        //When the thread turns into running state, the waiting time should be reset
        next->setRunTick(kernel->stats->totalTicks);
        /*modified end*/
        DEBUG(dbgMP3 , "[B] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << next->getID() << "] is removed from queue L[2]\n");
        return next;
    }
    else if(!L3_list->IsEmpty()){
        next =  L3_list->RemoveFront();
        /*MP3 109062320 modified start*/
        //When the thread turns into running state, the waiting time should be reset
        next->setRunTick(kernel->stats->totalTicks);
        /*modified end*/
        DEBUG(dbgMP3 , "[B] Tick ["<< kernel->stats->totalTicks << "]: Thread [" << next->getID() << "] is removed from queue L[3]\n");
        return next;
    }
    else{
        return NULL;
    }
    /*
    if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }*/
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    // add 109062233 MP3
    /*MP3 109062320 modified start*/
    /**----we have updated the last tick in FindNextToRun---**/
    //the thread start waiting for CPU
    //oldThread->setLastTick(kernel->stats->totalTicks);
    /**-----------------------------------------------------**/
    //{accumulated ticks} not sure
    //maybe oldThread->getAccumulatedTime()
    //or oldThread->getAccumulatedTime() - oldThread->getLastTick()
    DEBUG(dbgMP3 , "[E] Tick [" << kernel->stats->totalTicks << "]: Thread ["<< nextThread->getID() << "] is now selected for execution, thread ["<< oldThread->getID() << 
    "] is replaced, and it has executed [" <<kernel->stats->totalTicks - oldThread->getRunTick()<<"] ticks) \n");
    /*modified end*/    
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
    /*MP3 109062320 modified start*/
    //When the thread turns into running state, the waiting time should be reset
    oldThread->setRunTick(kernel->stats->totalTicks);
    //the thread start waiting for CPU
    // nextThread->setRunTick(kernel->stats->totalTicks);
    /*modified end*/
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    // MP3 109062233 removed
    // readyList->Apply(ThreadPrint);
}
