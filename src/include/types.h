/* Our main data structures
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
#ifndef _TYPES
#define _TYPES

#include <uARMtypes.h>
#include <const.h>

#ifndef phase0
// phase0 test includes stdlib where is already defined pid_t
typedef unsigned int pid_t;
#endif /* end of include guard: phase0 */

/* size_t was defined as unsigned int in the clist_template.h from phase0 and commented out. 
 * We decided to define it as unsigned long anyway, because that's the implementation of 
 * stdio.h */

typedef unsigned long size_t;
typedef int bool;
typedef unsigned int cputime_t;

/* struct clist definition. It is at the same time the type of the tail
 * pointer of the circular list and the type of field used to link the elements.
 * 
 * clist declaration is here to allow including only the declaration, without the whole 
 * library (e.g. in this file, pcb_t declaration only needs to know clist structure). */

struct clist {
    struct clist *next;
};

struct semd_t {
        int *s_semAdd; /* pointer to the semaphore */
        struct clist s_link; /* ASL linked list */
        struct clist s_procq; /* blocked process queue */
};

struct pcb_t {
    struct pcb_t *p_parent; /* pointer to parent */
    struct semd_t *p_cursem; /* pointer to the semd_t on
                                which process blocked */
    pid_t p_pid;
    state_t p_s; /* processor state */
    state_t p_excpvec[EXCP_COUNT]; /*exception states vector*/
    bool handler_defined[3]; //0 sys, 1 tlb, 2 pgmt
    cputime_t sys_time;
    cputime_t usr_time;
    // stores the number of initial resources requested from the semaphore on which
    // the process is currently blocked on. (the value of weight with which semop was called)
    // should be 0 otherwise.
    int s_req_weight;
    int user_enter_timestamp;
    struct clist p_list; /* process list */
    struct clist p_children; /* children list entry point*/
    struct clist p_siblings; /* children list: links to the siblings */
};

#endif
