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
#ifndef _EXCEPTIONS
#define _EXCEPTIONS
#include <types.h>

/* This is the system syscall handler.
 * It uses helper functions to actually do the syscall, and then decide how to 
 * proceed based on the helper function return value.  */
void Syscall_Handler();

/* The system TLB handler */
void TLB_Handler();

/* The system PGMT handler */
void PGMT_Handler();

/********************
 * Helper functions *
 ********************/

/* Create a new pcb, insert it into the ready_queue and return its pid */
int sys_createprocess();

/* Kill the process with the given pid; start looking for it from pcb */
void sys_terminateprocess(pid_t p, struct pcb_t* pcb);

/* Perform various operation on sempahores.
 *
 * We adopted a FIFO policy.  
 * The value of the semaphore indicates how many resources the first blocked process still need, while
 * how many resources was originally requested is saved in a pcb field. 
 * Subsequent blocked process do not alter semaphore value, which get updated when it gets a new head. */
int sys_semaphoreop(int * semaddr, int weight);

/* Generic function to prepare the handlers.
 * exc_const are constants specifying which handler we're preparing (syscall, tlb or program trap).  */
int sys_define_handler(memaddr pc, memaddr sp, unsigned int flags, unsigned int exc_const, unsigned int check_exc);

/* Manage devices operation.
 * One operation per device is allowed; if a second operation request comes in, the calling process
 * get notified that the device is busy (and it's up to that process to eventually try again).
 * An eventual higher level library may manage a queue on a device to avoid busy waiting (like it
 * is done in the print function in p2test).  */
int sys_iodevop(unsigned int command, int intlNo, unsigned int dnum);

/* Save into given location user and global time */
void sys_cputime(cputime_t *global, cputime_t *user);

/* Update the user/system time fields in the pcb, using a given timestamp
 * as starting time */
void update_usr_time(unsigned int end_timestamp, struct pcb_t* pcb);
void update_sys_time(unsigned int start_timestamp, struct pcb_t* pcb);

/* Check if a pcb is blocked on our device or pseudoclock semaphores.
 * We assume arrays are allocated contiguously.  */
bool is_device_sem(memaddr* addr);

/* We chose to allocate the lowest free pid from a pool of MAXPROC pids. */
pid_t generatePID();

/* Return the process pid */
pid_t getPID();

#define CREATE_PROCESS_ERROR -1

#define SEM_PROCESS_GO_ON 0
#define SEM_PROCESS_ON_WAIT 1
#define SEM_PROCESS_SCHEDULE_NEW 2

#define SPECHDL_GO_ON 0
#define SPECHDL_SCHEDULE_NEW 1

#define IO_DEV_NOT_INSTALLED 0
#define IO_DEV_BUSY 1
#define IO_PROCESS_ON_WAIT 2

#define CHECK_SYS_HDL 0
#define CHECK_TLB_HDL 1
#define CHECK_PGMT_HDL 2

#endif
