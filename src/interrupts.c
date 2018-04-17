/* This file manages the os interrupts and its timers
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
#include <asl.h>
#include <pcb.h>
#include <clist.h>
// phase 2 libs
#include <scheduler.h>
#include <exceptions.h>
#include <interrupts.h>
// uARM libs
#include <libuarm.h>
#include <arch.h>

#ifdef DEBUG
#include <debug.h>
#endif

extern int curr_proc_time_left;
extern int next_time_slice;
extern int s_pseudo_clock_timer;
extern struct clist* ready_queue;
extern int softblock_count;
extern bool nearwait;
extern int s_dev_array[DEV_USED_INTS - 1][DEV_PER_INT];
extern int s_term_array[DEV_PER_INT][TERM_SUBDEV];
extern struct pcb_t* curr_proc;

// a timestamp of the last pseudo-clock start time
int pseudo_clock_start;
// a time slice has been split!
int time_slice_split = 0;

/* System interrupt handler function
 * We handle an interrupt at a time, the higher priority first */
void Interrupt_Handler(){
    // recover processor state and read CP15_Cause
    state_t* oldarea = (state_t*) INT_OLDAREA;
    unsigned int cause = oldarea->CP15_Cause;

    // set the right return address in the pc (manual sec 8.3)
    oldarea->pc = oldarea->pc - 4;

    int which_int;

    // select the higher priority interrupt
    for (which_int = 0; which_int < FIRST_EMPTY_INT; which_int++) {
        if (CAUSE_IP_GET(cause, which_int)){
            break;
        }
    }

    switch (which_int){

        case IL_TIMER:
            // Interval timer interrupt
            {{
                 // manage interval timer
                 int result = manage_timers();
                 if(result==INT_TIME_SLICE_ENDED) {
                     // call the scheduler only if the time slice has ended
                     // AND we're not in nearwait state (that's controlled
                     // inside manage_timers())
                     // update usr time if the interrupt happened when a usr process was running
                     if (!nearwait)
                         update_usr_time(oldarea->TOD_Low, curr_proc);
                     schedule(SCHED_TIME_SLICE_ENDED);
                 }
                 // in every other cases just go on
                 // (those cases are INT_PSEUDO_CLOCK_ENDED and PROCESSOR_TWIDDLING_ITS_THUMBS;
                 // those constants are not used right now, because (later on) we check
                 // the variable nearwait to decide what to do)
             }}
            break;

        case IL_TERMINAL:
            {{
                 terminal_handler();
                 //handle the result differently
             }}
            break;

        default:
            generic_device_handler(which_int);
            break;

    }

    if(nearwait){
        // the scheduler had/was about to put the processor into wait state
        // we need to mask the interrupts again and jump to the scheduler
        // we cant just LDST because we dont know if the interrups is caught
        // before or after entering wait state
        nearwait = FALSE;
        oldarea->pc = (memaddr)schedule;
        oldarea->a1 = SCHED_NEARWAIT;
        oldarea->cpsr = STATUS_ALL_INT_DISABLE(oldarea->cpsr);
    }
    else {
        // update user time if the interrupt happened when a user process was running
        update_usr_time(oldarea->TOD_Low, curr_proc);
    }

    if(time_slice_split){
        // a process time slice was split, the settimer is here to be more accurate
        setTIMER(curr_proc_time_left);
        curr_proc_time_left = 0;
        time_slice_split = 0;
    }
    LDST((void*)oldarea);
}

/* This function manages two system timers:
 *  - time slice: this is currently about 5ms, we assure every process to get its whole timeslice
 *      even if it has been interrupted
 *  - pseudo-clock timer: this is currently about 100ms. Due to several factors, we can't be sure
 *      about delays, but they don't add up since we use timestamps. */
int manage_timers(int which_int){
    if(( getTODLO()-pseudo_clock_start ) >= SCHED_PSEUDO_CLOCK ){
        // pseudo clock ended
        // the pseudo_clock_timer could get to 105ms at most

        // unblock processes on the pseudo_clock_timer
        while(headBlocked(&(s_pseudo_clock_timer))!=NULL){
            insertProcQ(ready_queue, removeBlocked(&(s_pseudo_clock_timer)));
            softblock_count--;
        }
        // reset the pseudo clock timer semaphore
        s_pseudo_clock_timer = 0;

        // adjust the next pseudo_clock_start timer not considering delays
        pseudo_clock_start = SCHED_PSEUDO_CLOCK + pseudo_clock_start;

        if(nearwait) { 
            // the processor was in wait state, the Interrupt Handler will call the scheduler
            // to handle the processes just freed (if any)
            // We set the timer at SCHED_PSEUDO_CLOCK because, even if there's no more process to unblock, we
            // need to keep pseudo_clock_start updated. If a process arrive on the ready_queue somehow,
            // the scheduler will correctly reset the timer
            setTIMER(SCHED_PSEUDO_CLOCK);
            return PROCESSOR_TWIDDLING_ITS_THUMBS;
        }
        else {
            if(curr_proc_time_left>0){
                // this happens if we split a process time slice before therefore the curr_proc has
                // still some time left from his time slice
                // this flag will allow to set the timer later (near the LDST)
                time_slice_split = 1;
                return INT_PSEUDO_CLOCK_ENDED;
            }
            else {
                // we get here if we didn't split a process time slice therefore the curr_proc already
                // had all the time slice
                // this happens if the proc started before 95ms and ended after 100ms because of additional time
                // spent with interrupts masked for whatever reason
                next_time_slice = SCHED_TIME_SLICE;
                // since we are not in wait state we let the scheduler set the timer (therefore sending the ack)
                return INT_TIME_SLICE_ENDED;
            }
        }
    }
    else {
        // time slice ended
        if(nearwait) { 
            // the processor was in wait state, the Interrupt Handler will call the scheduler
            // and it will wait again because we haven't freed any process
            // We set the timer to wake us at the next pseudo clock timer tick since there's no process
            // in the ready queue: there's no need of time slices and we only care about pseudo clock timer
            // (remember: if other [devices] interrutps happen, the scheduler will restart the proper time slice cycle)
            setTIMER(SCHED_PSEUDO_CLOCK - (getTODLO()-pseudo_clock_start));
            return PROCESSOR_TWIDDLING_ITS_THUMBS;
        }        
        else { 
            // check how close we are to the pseudo clock tick
            int pseudo_clock_delta = getTODLO()-pseudo_clock_start;

            if(pseudo_clock_delta > (SCHED_PSEUDO_CLOCK - SCHED_TIME_SLICE)){
                // there's no time for another full time slice, we'll split the next one
                // how much the next process will need to advance AFTER the tick from the pseudo_clock_timer
                curr_proc_time_left = SCHED_TIME_SLICE - (SCHED_PSEUDO_CLOCK - pseudo_clock_delta);
                // how much the next process will need to advance BEFORE the tick from the pseudo_clock_timer
                next_time_slice = SCHED_PSEUDO_CLOCK - pseudo_clock_delta;
            }
            else next_time_slice = SCHED_TIME_SLICE;

            // call the scheduler
            return INT_TIME_SLICE_ENDED;
        }    
    }
}

/* Find out the first device requesting an interrutp on a given line */
int which_device_on_line(int which_int){

    // Decide which dev is raising an interrupt
    // bitmap has the nth bit = 1 if the nth device is raising an interrupt
    unsigned int bitmap;
    int which_dev;
    bitmap = * ((memaddr*) CDEV_BITMAP_ADDR(which_int));
    for (which_dev = 0; which_dev < DEV_PER_INT; which_dev++) {
        if(bitmap & 1) break;
        bitmap = bitmap >> 1;
    }
    return which_dev;
}

/* Check if the process which raised an interrupt and was blocked on the
 * corresponding real device semaphore has been killed */
bool is_process_killed(int which_int){
    int which_dev = which_device_on_line(which_int);
    if(which_int==IL_TERMINAL){
        termreg_t* term = (termreg_t*) DEV_REG_ADDR(IL_TERMINAL,which_dev); 
        int rec_stat = (char) term->recv_status; // take only the first byte
        int transm_stat = (char) term->transm_status;
        if ((transm_stat>1 && headBlocked(&(s_term_array[which_dev][TERM_TRASM]))==NULL)
                || (rec_stat>1 && headBlocked(&(s_term_array[which_dev][TERM_RECV]))==NULL)){
            return TRUE;
        }
    }
    else if (which_int!=IL_TIMER) {
        // we don't care about timer interrupts: either its a normal time slice tick 
        // or its a pseudoclock tick. the former just goes on, while in the latter case
        // we just free all processes waiting on the pseudoclock
        if(headBlocked(&(s_dev_array[which_int][which_dev]))==NULL){
            return TRUE;
        }
    }
    return FALSE;
}

/* Manage all devices but terminals */
void generic_device_handler (int which_int){

    int which_dev = which_device_on_line(which_int);
    devreg_t *dev = (devreg_t*)(DEV_REG_ADDR(which_int,which_dev));

    // Verify if there's still a process blocked on that semaphore (could have been killed!)
    if (!is_process_killed(which_int)){
        // write into pcb -> a1 device status word
        headBlocked(&(s_dev_array[which_int][which_dev]))->p_s.a1 = dev->dtp.status;
        softblock_count--;
    }
    // manage our device semaphores
    sys_semaphoreop(&(s_dev_array[which_int][which_dev]),1);
    // send an ACK to the device
    dev->dtp.command = DEV_C_ACK;
}

/* Manage terminal devices */
void terminal_handler (){
    int which_dev = which_device_on_line(IL_TERMINAL);
    termreg_t *term = (termreg_t*)(DEV_REG_ADDR(IL_TERMINAL,which_dev));
    struct pcb_t* head;
    // check if there has been an operation on this device
    // unblock the first completed operation we found, the second one turn will come as soon as 
    // the interrupt get unmasked again 

    // write has priority over read so we check it first
    if((char)term->transm_status>1){ // take only the first byte of the register
        // manage a write operation
        if(!is_process_killed(IL_TERMINAL)){
            if( (head = headBlocked(&(s_term_array[which_dev][TERM_TRASM]))) == NULL)
                PANIC();
            head->p_s.a1 = term->transm_status;
            softblock_count--;
        }
        sys_semaphoreop(&(s_term_array[which_dev][TERM_TRASM]),1);
        switch ((char)term->transm_status){
            case DEV_TTRS_C_TRSMCHAR:
                //illegal operation
                //reset the device
                term->transm_command = DEV_C_RESET;
                break;
            case DEV_TTRS_S_TRSMERR:
                //device error
                //reset the device
                term->transm_command = DEV_C_RESET;
                break;
            case DEV_TTRS_S_CHARTRSM:
                //char received
                //send ack
                term->transm_command = DEV_C_ACK;
                break;
        }
        return;
    }
    if((char)term->recv_status>1){ // take only the first byte of the register
        // manage a read operation
        if(!is_process_killed(IL_TERMINAL)){
            if( (head = headBlocked(&(s_term_array[which_dev][TERM_TRASM]))) == NULL)
                PANIC();
            head->p_s.a1 = term->recv_status;
            softblock_count--;
        }
        sys_semaphoreop(&(s_term_array[which_dev][TERM_RECV]),1);
        switch ((char)term->recv_status){
            case DEV_TRCV_C_RECVCHAR:
                //illegal operation
                //reset the terminal
                term->recv_command = DEV_C_RESET;
                break;
            case DEV_TRCV_S_RECVERR:
                //device error
                //reset the terminal
                term->recv_command = DEV_C_RESET;
                break;
            case DEV_TRCV_S_CHARRECV:
                //char received
                //send ack
                term->recv_command = DEV_C_ACK;
                break;
        }
        return;
    }
}
