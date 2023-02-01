// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the ` thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//----------------------------------------------------------------------

void 
Alarm::CallBack() 
{
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    //modified MP3 109062320
    // modify MP3 109062233
    /*(h) The operations of preemption and priority updating MUST be delayed until the
    next timer alarm interval in alarm.cc Alarm::Callback.*/
    if (status != IdleMode) {
        kernel->scheduler->update_priority();
        //re-schedule the threads in L2, L3 after updating priority
        kernel->scheduler->scheduling();
        // preempt L1
        // for current thread belong to L1
        if(kernel->currentThread->getPriority() > 99){
            if(kernel->scheduler->L1_empty() == false){
                Thread* L1_front = kernel->scheduler->front_of_L1();
                Thread* cur_t = kernel->currentThread;
                int rem_burst_time1 = (cur_t -> getBurstTime()) - (cur_t -> getAccumulatedTime());
                int rem_burst_time2 = (L1_front -> getBurstTime()) - (L1_front -> getAccumulatedTime());
                if(rem_burst_time1 > rem_burst_time2)
                    interrupt->YieldOnReturn();
            }
        }
        // preempt L2 (although L2 is non-premmptive, L1 can preempt it!)
        else if(kernel->currentThread->getPriority() > 49){
            if(kernel->scheduler->L1_empty() == false)
                interrupt->YieldOnReturn();
        }
        // preempt L3
        //for current thread belong to L3
        //L3 is round-robin
        else{
            if((kernel->scheduler->L1_empty() == false)
            || (kernel->scheduler->L2_empty() == false)
            || (kernel->scheduler->L3_empty() == false))
            interrupt->YieldOnReturn();
        }

        
    }
}
