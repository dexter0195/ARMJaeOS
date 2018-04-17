/* This is the os scheduler
 *
 * A didactic simulation of an arm OS running on the uarm emulator.
 * Copyright (C) 2016 Carlo De Pieri, Alessio Koci, Gianmaria Pedrini,
 * Alessio Trivisonno
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// project specific consts and types, includes uARM consts and types
#include <const.h>
#include <types.h>
// phase 1 libs
#include <pcb.h>
#include <clist.h>
// phase 2 libs
#include <scheduler.h>
// uARM libs
#include <libuarm.h>

#ifdef DEBUG
#include <debug.h>
#endif

// these two int are usefull to decide how much time to set the timer to
int next_time_slice;
int curr_proc_time_left;
// this indicates if we're in a wait processor pattern
bool nearwait = FALSE;
extern struct clist* ready_queue;
extern int proc_count;
extern int softblock_count;
extern struct pcb_t* curr_proc;

/* Main scheduler function. It's argument its used to take different action based on where the
 * scheduler is called from.
 * This is a simple round robin scheduler. */
void schedule (int state){
    if(state==SCHED_TIME_SLICE_ENDED){
        // time slice for the curr_proc has endend, we must move it to the ready_queue
        // first copy the process' processor state from INT_OLDAREA
        curr_proc->p_s = *((state_t*) INT_OLDAREA);
        // enquee the curr_proc into ready_queue 
        insertProcQ(ready_queue, curr_proc);
    }

    //ready_queue is empty
    if(clist_empty(*ready_queue)){
        
        if(proc_count == 0){
            // no process to execute
            HALT();
        }
        if(proc_count > 0 && softblock_count == 0){
            // deadlock detection
            PANIC();
        }
        if(proc_count > 0 && softblock_count > 0){
            // process waiting an interrupt 
            // this flag will let the interrupt handler know that we are approaching a waiting state
            nearwait = TRUE;
            // enable normal interrupt
            setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));
            WAIT();
            // will never get here, because an interrupt caught with nearwait will call schedule()
        } 

    }

    // set the curr_proc to the first ready pcb_t and remove head from ready_queue 
    curr_proc = removeProcQ(ready_queue);
    if(curr_proc == NULL)
        // should never happen
        PANIC();
    curr_proc->user_enter_timestamp = getTODLO();
    if(state == SCHED_INIT){
        curr_proc_time_left = 0;
        setTIMER(SCHED_TIME_SLICE);
    }
    else if(state==SCHED_PROC_KILLED || state == SCHED_PROC_BLOCKED || state == SCHED_NEARWAIT){
        extern int pseudo_clock_start;
        int pseudoclock = getTODLO()-pseudo_clock_start;
        int time = SCHED_TIME_SLICE;
        // be sure to reset curr_proc_time_left if the process had a splitted time slice
        curr_proc_time_left = 0;
        if(pseudoclock>(SCHED_PSEUDO_CLOCK-SCHED_TIME_SLICE) && pseudoclock< SCHED_PSEUDO_CLOCK){
            // This process got killed between 95ms and 100ms. We want the next interrupt to come near
            // the pseudo clock tick
            time = SCHED_PSEUDO_CLOCK - pseudoclock;
            curr_proc_time_left = SCHED_TIME_SLICE - time;
        }
        setTIMER(time);
    }
    else if(state == SCHED_TIME_SLICE_ENDED){
            setTIMER(next_time_slice);
    }
    // load the pcb_t processor state into the processor
    LDST((void*) &curr_proc->p_s);
}
