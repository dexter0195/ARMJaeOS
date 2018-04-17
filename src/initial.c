/* This initializes everything needed by the scheduler for its first
 * run and prepares the first process
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
#include <asl.h>
#include <clist.h>
#include <helplib.h>
// phase 2 libs
#include <interrupts.h>
#include <exceptions.h>
#include <scheduler.h>
// uARM libs
#include <arch.h>
#include <libuarm.h>

#ifdef DEBUG
#include <debug.h>
#endif

//init scheduler variables
int proc_count = 0;
int softblock_count = 0;
bool free_pidmap[MAXPROC];
static struct clist ready_queue_list = CLIST_INIT;
struct clist* ready_queue = &ready_queue_list;
struct pcb_t* curr_proc;

//init device and clock semaphores
// semaphores for devices, except terminals, which get their personal s_array
int s_dev_array[DEV_USED_INTS-1][DEV_PER_INT]; // [int-line][dev-num]
int s_term_array[DEV_PER_INT][TERM_SUBDEV]; // [term-num][0 == WRITE 1 == READ]
int s_pseudo_clock_timer = 0;

extern int pseudo_clock_start;
extern void test();
extern pid_t generatePID();

int main(){
//initialize nucleus

    //initialize New Areas pointers
    state_t* Interrupt_New =(state_t*) INT_NEWAREA;
    state_t* TLB_New = (state_t*) TLB_NEWAREA;
    state_t* PGMT_New = (state_t*) PGMTRAP_NEWAREA;
    state_t* Syscall_New = (state_t*) SYSBK_NEWAREA;

    //initialize to 0 all the structs
    //this might not be necessary, but it doesnt hurt
    mymemset(Interrupt_New, 0, sizeof(state_t));
    mymemset(TLB_New, 0, sizeof(state_t));
    mymemset(PGMT_New, 0, sizeof(state_t));
    mymemset(Syscall_New, 0, sizeof(state_t));

    //set the pc to the right handler
    Interrupt_New->pc = (memaddr) Interrupt_Handler;
    TLB_New->pc = (memaddr) TLB_Handler;
    PGMT_New->pc = (memaddr) PGMT_Handler;
    Syscall_New->pc = (memaddr) Syscall_Handler;
    
    //set sp to RAMTOP
    memaddr ramtop = (memaddr) RAM_TOP;
    Interrupt_New->sp = ramtop;
    TLB_New->sp = ramtop;
    PGMT_New->sp = ramtop;
    Syscall_New->sp = ramtop;

    //set System Mode in cpsr and disable interrupts (both normal and fast)
    Interrupt_New->cpsr = STATUS_SYS_MODE;
    Interrupt_New->cpsr = STATUS_ALL_INT_DISABLE(Interrupt_New->cpsr);
    TLB_New->cpsr = STATUS_SYS_MODE;
    TLB_New->cpsr = STATUS_ALL_INT_DISABLE(TLB_New->cpsr);
    PGMT_New->cpsr = STATUS_SYS_MODE;
    PGMT_New->cpsr = STATUS_ALL_INT_DISABLE(PGMT_New->cpsr);
    Syscall_New->cpsr = STATUS_SYS_MODE;
    Syscall_New->cpsr = STATUS_ALL_INT_DISABLE(Syscall_New->cpsr);

    //allocate pcbs and semaphores
    initPcbs();
    initASL();
    
    //initialize to 0 all free_pidmap elements
    for(int i=0; i<MAXPROC; i++)
        free_pidmap[i] = TRUE;

    //instantiate test process
    struct pcb_t* test_pcb = allocPcb();
    if(test_pcb == NULL)
        PANIC();
    //set System Mode with interrupts enabled
    test_pcb->p_s.cpsr = STATUS_SYS_MODE;
    // disable virtual memory
    test_pcb->p_s.CP15_Control = CP15_DISABLE_VM(test_pcb->p_s.CP15_Control);
    test_pcb->p_s.sp = ramtop - FRAMESIZE;
    test_pcb->p_s.pc = (memaddr) test;
    test_pcb->p_pid = generatePID();
    insertProcQ(ready_queue, test_pcb);
    proc_count++;

    //initialize pseudoclock timestamp
    pseudo_clock_start = getTODLO();

    //call the scheduler
    schedule(SCHED_INIT);
}
