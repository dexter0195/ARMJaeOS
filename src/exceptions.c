/* Syscall, PGMT and TLB handlers and theirs helper function
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
#include <exceptions.h>
#include <scheduler.h>
// uARM libs
#include <libuarm.h>

#ifdef DEBUG
#include <debug.h>
#endif

extern struct clist *ready_queue;
extern int proc_count;
extern bool free_pidmap[MAXPROC];
extern int softblock_count;
extern struct pcb_t *curr_proc;
extern int s_term_array[DEV_PER_INT][TERM_SUBDEV];
extern int s_dev_array[DEV_USED_INTS-1][DEV_PER_INT];
extern int s_pseudo_clock_timer;

/* This is the syscall system handler.
 * It uses helper functions to actually do the syscall, and then decide how to 
 * proceed based on the helper function return value. */
void Syscall_Handler() {
    state_t *oldarea = (state_t*) SYSBK_OLDAREA;
    // process user time slice has ended, update it
    update_usr_time(oldarea->TOD_Low, curr_proc);

    /* The nucleus will then perform some service on behalf of the process
     * executing the SYSCALL or BREAK instruction depending on the value found in
     * a1. */
    unsigned int sys_num;
    sys_num = oldarea->a1; 

    /* if the process making a SYSCALL request
     * was in kernel-mode and a1 contained a value in the range [1..11] then the
     * nucleus should perform one of the services described below.
     * else se non è in kernel mode va gestito come indicato in 3.3.12 */

    if (sys_num >= SYSCALL_MIN && sys_num <= SYSCALL_MAX) {
        // if the last bit of cpsr is 0 we are in usermode

        if (! (oldarea->cpsr & 0x0F)) { // if the last bit of cpsr is 0 we are in usermode
            // simulate PgmTrap exception
            *((state_t*) PGMTRAP_OLDAREA) = *oldarea;
            ((state_t*) PGMTRAP_OLDAREA)->CP15_Cause = CAUSE_EXCCODE_SET( ((state_t*) PGMTRAP_OLDAREA)->CP15_Cause, EXC_RESERVEDINSTR);
            PGMT_Handler();
        }
        else {
            switch (sys_num) {

                case CREATEPROCESS:
                    {{
                         int result = sys_createprocess((state_t*)oldarea->a2);
                         oldarea->a1 = result;
                         // restore parent process into the processor
                         update_sys_time(oldarea->TOD_Low, curr_proc);
                         LDST(oldarea);
                     }}
                    break;

                case TERMINATEPROCESS:
                    // we set the timer to an high enough value to avoid an interrupt before the scheduler 
                    // gets the chance to reset it for the next process (it would lose its next time slice),
                    // but low enough that if the ready queue is empty we dont miss the pseudo clock timer tick
                    // by too much
                    if (oldarea->a2 == 0) setTIMER(SCHED_TIME_SLICE);

                    // call the recursive murderer function
                    sys_terminateprocess(oldarea->a2, curr_proc);
                    if (curr_proc==NULL){
                        // the process who called SYS2 killed itself, call the scheduler
                        schedule(SCHED_PROC_KILLED);
                    }
                    else {
                        // the calling process only killed a child, it should continue
                        update_sys_time(oldarea->TOD_Low, curr_proc);
                        LDST(oldarea);
                    }
                    break;

                case SEMOP:
                    {{
                         int result = sys_semaphoreop((int*)oldarea->a2,(int)oldarea->a3);
                         switch (result) {
                             case SEM_PROCESS_SCHEDULE_NEW:
                                 // if weight was 0 and the process has been terminated
                                 schedule(SCHED_PROC_KILLED);
                                 break;
                             case SEM_PROCESS_GO_ON:
                                 update_sys_time(oldarea->TOD_Low, curr_proc);
                                 LDST(oldarea);
                                 break;
                             case SEM_PROCESS_ON_WAIT:
                                 update_sys_time(oldarea->TOD_Low, curr_proc);
                                 curr_proc->p_s = *((state_t*)(oldarea));
                                 schedule(SCHED_PROC_BLOCKED); 
                                 break;
                             default:
                                 //error
                                 PANIC();
                                 break;
                         }
                     }}
                    break;

                case SPECSYSHDL:
                    {{
                         //the address of the handler function in a2, the address of the handler’s stack in a3 and the execution flags in a4.
                         int result = sys_define_handler(oldarea->a2,oldarea->a3, oldarea->a4, EXCP_SYS_NEW, CHECK_SYS_HDL);
                         switch (result) {
                             case SPECHDL_GO_ON:
                                 update_sys_time(oldarea->TOD_Low, curr_proc);
                                 LDST(oldarea);
                                 break;
                             case SPECHDL_SCHEDULE_NEW:
                                 schedule(SCHED_PROC_KILLED);
                                 break;
                             default:
                                 //error
                                 PANIC();
                                 break;
                         }
                     }}
                    break;

                case SPECTLBHDL:
                    {{
                         //the address of the handler function in a2, the address of the handler’s stack in a3 and the execution flags in a4.
                         int result = sys_define_handler(oldarea->a2,oldarea->a3, oldarea->a4, EXCP_TLB_NEW, CHECK_TLB_HDL);
                         switch (result) {
                             case SPECHDL_GO_ON:
                                 update_sys_time(oldarea->TOD_Low, curr_proc);
                                 LDST(oldarea);
                                 break;
                             case SPECHDL_SCHEDULE_NEW:
                                 schedule(SCHED_PROC_KILLED);
                                 break;
                             default:
                                 //error
                                 PANIC();
                                 break;
                         }
                     }}
                    break;

                case SPECPGMTHDL:
                    {{
                         //the address of the handler function in a2, the address of the handler’s stack in a3 and the execution flags in a4.
                         int result = sys_define_handler(oldarea->a2,oldarea->a3, oldarea->a4, EXCP_PGMT_NEW, CHECK_PGMT_HDL);
                         switch (result) {
                             case SPECHDL_GO_ON:
                                 update_sys_time(oldarea->TOD_Low, curr_proc);
                                 LDST(oldarea);
                                 break;
                             case SPECHDL_SCHEDULE_NEW:
                                 schedule(SCHED_PROC_KILLED);
                                 break;
                             default:
                                 //error
                                 PANIC();
                                 break;
                         }
                     }}
                    break;

                case EXITTRAP:
                    {{
                         state_t* pcb_oldarea;
                         switch (oldarea->a2) {
                             case EXCP_SYS_OLD:
                                 pcb_oldarea = &(curr_proc->p_excpvec[EXCP_SYS_OLD]);
                                 break;
                             case EXCP_TLB_OLD:
                                 pcb_oldarea = &(curr_proc->p_excpvec[EXCP_TLB_OLD]);
                                 break;
                             case EXCP_PGMT_OLD:
                                 pcb_oldarea = &(curr_proc->p_excpvec[EXCP_PGMT_OLD]);
                                 break;
                             default:
                                 //error
                                 PANIC();
                                 break;
                         }
                         pcb_oldarea->a1 = oldarea->a3;
                         update_sys_time(oldarea->TOD_Low, curr_proc);
                         LDST(pcb_oldarea);
                     }}
                    break;

                case GETCPUTIME:
                    {{
                        update_sys_time(oldarea->TOD_Low, curr_proc);
                        sys_cputime((cputime_t*) (oldarea->a2), (cputime_t*) (oldarea->a3));
                        LDST(oldarea);
                    }}
                    break;

                case WAITCLOCK:
                    {{
                         int result = sys_semaphoreop(&s_pseudo_clock_timer, -1);
                         if (result == SEM_PROCESS_ON_WAIT){
                             softblock_count++;
                             curr_proc->p_s = *((state_t*)(oldarea));
                             update_sys_time(oldarea->TOD_Low, curr_proc);
                             schedule(SCHED_PROC_BLOCKED); 
                         }
                         else {
                             //error, the process should always lock on the timer sem
                             PANIC();
                         }
                     }}
                    break;

                case IODEVOP:
                    {{
                        int result = sys_iodevop(oldarea->a2, oldarea->a3, oldarea->a4);
                        switch (result) {
                             case IO_DEV_BUSY:
                                oldarea->a1 = DEV_BUSY;
                                update_sys_time(oldarea->TOD_Low, curr_proc);
                                LDST(oldarea);
                                break;
                             case IO_DEV_NOT_INSTALLED:
                                oldarea->a1 = DEV_NOT_INSTALLED;
                                update_sys_time(oldarea->TOD_Low, curr_proc);
                                LDST(oldarea);
                                break;
                             case IO_PROCESS_ON_WAIT:
                                curr_proc->p_s = *((state_t*)(oldarea));
                                update_sys_time(oldarea->TOD_Low, curr_proc);
                                schedule(SCHED_PROC_BLOCKED);
                                break;
                             default:
                                 //error
                                 PANIC();
                                 break;
                        }
                    }}
                    break;

                case GETPID:
                    oldarea->a1 = getPID(); 
                    update_sys_time(oldarea->TOD_Low, curr_proc);
                    LDST(oldarea);
                    break;

                default:
                    //error
                    PANIC();
                    break;
            } //switch parenthesis
        } //PGMT else parenthesis
    } //if parenthesis
    else if (sys_num > 11){
        if (curr_proc->handler_defined[CHECK_SYS_HDL]){
            curr_proc->p_excpvec[EXCP_SYS_OLD] = *oldarea;
            curr_proc->p_excpvec[EXCP_SYS_NEW].a1 = oldarea->a1;
            curr_proc->p_excpvec[EXCP_SYS_NEW].a2 = oldarea->a2;
            curr_proc->p_excpvec[EXCP_SYS_NEW].a3 = oldarea->a3;
            curr_proc->p_excpvec[EXCP_SYS_NEW].a4 = oldarea->a4;
            //reset the higher byte of a1
            curr_proc->p_excpvec[EXCP_SYS_NEW].a1 &= 0x0FFFFFFF;
            //copy the lower byte of cpsr in the higher byte of a1
            curr_proc->p_excpvec[EXCP_SYS_NEW].a1 |= oldarea->cpsr << 28; //check this
            update_sys_time(oldarea->TOD_Low, curr_proc);
            LDST(&(curr_proc->p_excpvec[EXCP_SYS_NEW]));
        }
        else {
            //suicide
            sys_terminateprocess(0, curr_proc);
            schedule(SCHED_PROC_KILLED);
        }

    }
} //SyscallHandler parenthesis

/********************
 * Helper functions *
 ********************/

/* Create a new pcb, insert it into the ready_queue and return its pid */
int sys_createprocess(state_t *statep) {
    struct pcb_t *p_child = allocPcb();
    if (p_child == NULL) {
        return CREATE_PROCESS_ERROR;
    }
    p_child->p_pid = generatePID();
    insertChild(curr_proc, p_child);
    insertProcQ(ready_queue, p_child);
    p_child->p_s = *statep;
    proc_count++;
    return p_child->p_pid;
}

/* Kill the process with the given pid; start looking for it from pcb */
void sys_terminateprocess(pid_t pid, struct pcb_t* pcb){
    // pid=0 means kill itself, otherwise one of its children
    if((pid == 0) || (pcb->p_pid == pid)){
        while(!emptyChild(pcb)){
            sys_terminateprocess(0, removeChild(pcb));
        }        
        // we need to remove this pcb from his parent children
        outChild(pcb);
        // NOW LET'S KILL SOME PROCESS >:)
        if(pcb->p_pid != curr_proc->p_pid){
            // the actual process is not the one who called the SYS2
            if (pcb->p_cursem != NULL) {
                if (is_device_sem(pcb->p_cursem->s_semAdd)){
                    // the process is blocked on a system device semaphore,
                    // we should only take out the pcb from the sem, while the interrupt handler will
                    // manage the semaphore value
                    outBlocked(pcb);
                    softblock_count--;
                }
                else {
                    // note: this particular implementation depends on our semaphore design
                    //
                    // if pcb is blocked on a sem and its not a device semaphore, unblock it (pcb->p_cursem would not be NULL)
                    if((headBlocked(pcb->p_cursem->s_semAdd)) == pcb){
                        // we call sys_semaphoreop to manage sem value, since this pcb is HEAD
                        // s_req_weight is <0, we must flip it before use it with sys_semaphoreop
                        sys_semaphoreop(pcb->p_cursem->s_semAdd, ((pcb->s_req_weight)*(-1)));
                        outProcQ(ready_queue, pcb);
                    }
                    else {
                        // the value of the semaphore is unaffected, we can simply remove the process from it
                        outBlocked(pcb);
                    }
                }
            }
            else {
                // the process is on the ready_queue, let's delete it
                outProcQ(ready_queue, pcb);
            }
        }
        else {
            // the actual process is the one who called the SYS2 and wants to die, this is the only 
            // case in which we enter here!
            curr_proc = NULL;
        }
        free_pidmap[pcb->p_pid] = TRUE;
        freePcb(pcb);
        proc_count--;
    }
    else {
        if (!emptyChild(pcb)){
            void *tmp = NULL;
            struct pcb_t * scan;
            // recursively look for a children whose pid is the one we're looking for
            clist_foreach(scan, &pcb->p_children, p_siblings, tmp){
                sys_terminateprocess(pid, scan);
            }
        }
    }
}

/* Check if a pcb is blocked on our device or pseudoclock semaphores.
 * We assume arrays are allocated contiguously. */
bool is_device_sem(memaddr* addr){
    memaddr* start_dev = (memaddr*) s_dev_array;
    memaddr* stop_dev = (memaddr*) &(s_dev_array[DEV_USED_INTS-2][DEV_PER_INT-1]);
    memaddr* start_term = (memaddr*) s_term_array ;
    memaddr* stop_term = (memaddr*) &(s_term_array[DEV_PER_INT-1][TERM_SUBDEV-1]);
    if((addr >= start_dev && addr <= stop_dev) || (addr >= start_term && addr <= stop_term) || addr == (memaddr*) &s_pseudo_clock_timer) return TRUE;
    return FALSE;
}

/* Perform various operation on sempahores.
 *
 * We adopted a FIFO policy.  
 * The value of the semaphore indicates how many resources the first blocked process still need, while
 * how many resources was originally requested is saved in a pcb field. 
 * Subsequent blocked process do not alter semaphore value, which get updated when it gets a new head. */
int sys_semaphoreop (int *semaddr, int weight){
    if (weight == 0){
        //weight 0 should be construed as an error and treated as a SYS2 of the process itself
        sys_terminateprocess(0, curr_proc);
        return SEM_PROCESS_SCHEDULE_NEW;
    }
    else if (weight>0){
        // deallocating resources
        *semaddr += weight;
        if(*semaddr>=0 && headBlocked(semaddr)!=NULL){
            // reset pcb field when freeing the process
            headBlocked(semaddr)->s_req_weight = 0;
            // we can unblock the first process waiting since we reached 0
            insertProcQ(ready_queue, removeBlocked(semaddr));
            // now check if we can unblock more processes
            while(*semaddr>=0 && headBlocked(semaddr)!=NULL){
                // this enters (and goes on) if there are processes blocked on the semaphore
                // AND if there are enough resources to be allocated
                int resource_requested = headBlocked(semaddr)->s_req_weight;
                if((*semaddr + resource_requested) >= 0){
                    // reset pcb field when freeing the process
                    headBlocked(semaddr)->s_req_weight = 0;
                    // return the process to the ready_queue
                    insertProcQ(ready_queue, removeBlocked(semaddr));
                }
                // decrement anyway, 'cause there's a process in queue requesting resources
                *semaddr += resource_requested;
            }
        }
        return SEM_PROCESS_GO_ON;
    }
    else {
        // requesting resources
        if (*semaddr >= 0){
            if (*semaddr + weight < 0){
                // there are not enough resources, put it on wait
                insertBlocked(semaddr, curr_proc);
                *semaddr += weight;
                // update pcb of waiting process
                curr_proc->s_req_weight = weight;
                return SEM_PROCESS_ON_WAIT;
            }
            else {
                // there are enough resources, allocate it
                *semaddr += weight;
                return SEM_PROCESS_GO_ON;
            }
        }
        else {
            // do not decrement, there's already other processes on the semaphore
            insertBlocked(semaddr, curr_proc);
            // update pcb of waiting process
            curr_proc->s_req_weight = weight;
            return SEM_PROCESS_ON_WAIT;
        }
    }
}

/* Generic function to prepare the handlers.
 * exc_const are constants specifying which handler we're preparing (syscall, tlb or program trap).  */
int sys_define_handler(memaddr pc, memaddr sp, unsigned int flags, unsigned int exc_const, unsigned int check_exc){
    if (!curr_proc->handler_defined[check_exc]) {
        curr_proc->p_excpvec[exc_const].pc = pc; //handler
        curr_proc->p_excpvec[exc_const].sp = sp; //stack pointer
        curr_proc->p_excpvec[exc_const].cpsr = flags & 0xFF; //flags-cpsr
        if (flags >> 31){
            curr_proc->p_excpvec[exc_const].CP15_Control = CP15_ENABLE_VM(curr_proc->p_excpvec[exc_const].CP15_Control);
        }
        else {
            curr_proc->p_excpvec[exc_const].CP15_Control = CP15_DISABLE_VM(curr_proc->p_excpvec[exc_const].CP15_Control);
        }
        ENTRYHI_ASID_SET(curr_proc->p_excpvec[exc_const].CP15_EntryHi, ENTRYHI_ASID_GET(curr_proc->p_s.CP15_EntryHi));
        curr_proc->handler_defined[check_exc] = TRUE;
        return SPECHDL_GO_ON;
    }
    else {
        sys_terminateprocess(0, curr_proc);
        return SPECHDL_SCHEDULE_NEW;
    }
}

/* Manage devices operation.
 * One operation per device is allowed; if a second operation request comes in, the calling process
 * get notified that the device is busy (and it's up to that process to eventually try again).
 * An eventual higher level library may manage a queue on a device to avoid busy waiting (like it
 * is done in the print function in p2test).  */
int sys_iodevop(unsigned int command, int intlNo, unsigned int dnum){
    // 0) declare anonymous registers variables
    int* s_dev;
    memaddr* comm_addr;
    // this is char* because in the terminal case we must consider only the least significant byte,
    // in other devs it doesnt matter since the state value always occupy less than one byte (as of
    // now)
    char* state_addr;

    // 1) setting the variables depending on the actual device
    // get device base address
    devreg_t* device = (devreg_t*)DEV_REG_ADDR(intlNo, dnum & 0xFF); // args (interrupt line, device number)
    if (intlNo == IL_TERMINAL)
    {
        // choose the right state and comm
        if (dnum >> 31){ // READ
            state_addr = (char*)&(device->term.recv_status);
            comm_addr = &(device->term.recv_command);
        }
        else{ // WRITE
            state_addr = (char*)&(device->term.transm_status);
            comm_addr = &(device->term.transm_command);
        }
        // choose the right semaphore
        s_dev = &(s_term_array[dnum & 0x7FFFFFFF][dnum >> 31]);
    }
    else{ // other devices interval line
        // choose the right state and comm
        state_addr = (char*)&(device->dtp.status);
        comm_addr = &(device->dtp.command);
        // choose the right semaphore. INT_LOWEST is the first real device
        s_dev = &(s_dev_array[intlNo-INT_LOWEST][dnum]);
    }

    // 2) do the operation, if possible
    if (*state_addr != DEV_S_READY){
        // check if we can issue a command to the device (i.e. its ready)
        if (*state_addr == DEV_NOT_INSTALLED){
            // device not installed return error code
            return IO_DEV_NOT_INSTALLED;
        }
        else{
            // device busy (or operation not acked yet)
            // return busy error code
            return IO_DEV_BUSY;
        }
        // return immediately
    }
    else{
        // the device is ready, send the command
        *comm_addr = command;
    }

    // 3) lock on the device semaphore
    int result = sys_semaphoreop(s_dev, -1);
    if (result == SEM_PROCESS_ON_WAIT){
        softblock_count++;
        return IO_PROCESS_ON_WAIT;
    }
    else{
        // error, the process should always lock on the device semaphore
        PANIC();
    }
}

/* Save into given location user and global time */
void sys_cputime(cputime_t *global, cputime_t *user){
    *global = curr_proc->sys_time + curr_proc->usr_time; //global time
    *user = curr_proc->usr_time;
}

/* Return the process pid */
pid_t getPID(){
    return curr_proc->p_pid;
}

/* We chose to allocate the lowest free pid from a pool of MAXPROC pids. */
pid_t generatePID(){
    int i;
    for(i=0; i<MAXPROC; i++){
        //scan for a free pid
        if (free_pidmap[i] == TRUE){
            free_pidmap[i] = FALSE;
            break;
        }
    }
    if (i == MAXPROC){
        //all the pids were taken. 
        PANIC();
    }

    return i;
}

/* Update the user/system time fields in the pcb, using a given timestamp
 * as starting time */
void update_usr_time(unsigned int end_timestamp, struct pcb_t* pcb){
    pcb->usr_time += end_timestamp - pcb->user_enter_timestamp;
    // update it in case we exit with LDST instead of a scheduler call
    pcb->user_enter_timestamp = getTODLO();
}
void update_sys_time(unsigned int start_timestamp, struct pcb_t* pcb){
    pcb->sys_time += getTODLO() - start_timestamp;
    // update it in case we exit with LDST instead of a scheduler call
    pcb->user_enter_timestamp = getTODLO();
}

/* The system TLB handler */
void TLB_Handler() {
    unsigned int tlb_enter_timestamp = getTODLO();
    if (curr_proc->handler_defined[CHECK_TLB_HDL]){
        // handle tlb
        curr_proc->p_excpvec[EXCP_TLB_OLD] = *((state_t*) TLB_OLDAREA);
        curr_proc->p_excpvec[EXCP_TLB_NEW].a1 = curr_proc->p_excpvec[EXCP_TLB_OLD].CP15_Cause;
        update_sys_time(tlb_enter_timestamp, curr_proc);
        LDST(&(curr_proc->p_excpvec[EXCP_TLB_NEW]));
    }
    else {
        //suicide
        sys_terminateprocess(0, curr_proc);
        schedule(SCHED_PROC_KILLED);
    }
}

/* The system PGMT handler */
void PGMT_Handler() {
    unsigned int pgmt_enter_timestamp = getTODLO();
    if (curr_proc->handler_defined[CHECK_PGMT_HDL]){
        // handle pgmt
        curr_proc->p_excpvec[EXCP_PGMT_OLD] = *((state_t*) PGMTRAP_OLDAREA);
        curr_proc->p_excpvec[EXCP_PGMT_NEW].a1 = curr_proc->p_excpvec[EXCP_PGMT_OLD].CP15_Cause;
        update_sys_time(pgmt_enter_timestamp, curr_proc);
        LDST(&(curr_proc->p_excpvec[EXCP_PGMT_NEW]));
    }
    else {
        //suicide
        sys_terminateprocess(0, curr_proc);
        schedule(SCHED_PROC_KILLED);
    }
}
