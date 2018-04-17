/* Our constants
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
/********************************
 * phase 2 constants definitions
 ********************************/

#ifndef CONST_H
#define CONST_H

#include <arch.h>
#include <uARMconst.h>

#define HIDDEN static
#define memaddr unsigned int

#ifndef TRUE
	#define TRUE 1
	#define FALSE 0
#endif

/* Maximum number of overall (eg, system, daemons, user) concurrent processes */
#define MAXPROC 20

/* Scheduling constants */
#define SCHED_TIME_SLICE 5000     /* in microseconds, aka 5 milliseconds */
#define SCHED_PSEUDO_CLOCK 100000 /* pseudo-clock tick "slice" length */
#define SCHED_BOGUS_SLICE 500000  /* just to make sure */

/* nucleus (phase2)-handled SYSCALL values */
#define CREATEPROCESS 1
#define TERMINATEPROCESS 2
#define SEMOP 3
#define SPECSYSHDL 4
#define SPECTLBHDL 5
#define SPECPGMTHDL 6
#define EXITTRAP 7
#define GETCPUTIME 8
#define WAITCLOCK 9
#define IODEVOP 10
#define GETPID 11

#define SYSCALL_MIN 1
#define SYSCALL_MAX 11

/* pcb exception states vector constants */
#define EXCP_SYS_OLD 0
#define EXCP_TLB_OLD 1
#define EXCP_PGMT_OLD 2
#define EXCP_SYS_NEW 3
#define EXCP_TLB_NEW 4
#define EXCP_PGMT_NEW 5

#define EXCP_COUNT 6

/* device types count with separate terminal read and write devs */
#define N_DEV_TYPES (N_EXT_IL+1)
// first empty interrupt line
#define FIRST_EMPTY_INT 8

// device codes (missing from uARMconst.h)
#define DEV_BUSY 3
#define DEV_C_RESET 0
#define TERM_RECV 1
#define TERM_TRASM 0
#define TERM_SUBDEV 2

#endif
