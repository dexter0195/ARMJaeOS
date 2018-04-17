/* Active Semaphore List maintenance functions
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
#ifndef _ASL
#define _ASL

#include <types.h>

/*****************************************
 * Active Semaphore List Maintenance     *
 *****************************************/

/* Insert the ProcBlk pointed to by p at the tail of the process queue asso-
 * ciated with the semaphore whose physical address is semAdd and set the
 * semaphore address of p to semAdd. If the semaphore is currently not ac-
 * tive (i.e. there is no descriptor for it in the ASL), allocate a new descriptor
 * from the semdFree list, insert it in the ASL (at the appropriate position),
 * initialize all of the fields (i.e. set s_semAdd to semAdd, and s_procq), and
 * proceed as above. If a new semaphore descriptor needs to be allocated
 * and the semdFree list is empty, return TRUE. In all other cases return
 * FALSE. */

int insertBlocked(int *semAdd, struct pcb_t *p);

/* Search the ASL for a descriptor of this semaphore. If none is found, return
 * NULL; otherwise, remove the first (i.e. head) ProcBlk from the process
 * queue of the found semaphore descriptor and return a pointer to it. If the
 * process queue for this semaphore becomes empty remove the semaphore
 * descriptor from the ASL and return it to the semdFree list */

struct pcb_t *removeBlocked(int *semAdd);

/* Remove the ProcBlk pointed to by p from the process queue associated
 * with p's semaphore on the ASL. If ProcBlk pointed to by p does not
 * appear in the process queue associated with p's semaphore, which is an
 * error condition, return NULL; otherwise, return p. If the
 * process queue for this semaphore becomes empty we decided to remove the 
 * semaphore descriptor from the ASL and return it to the semdFree list */


struct pcb_t *outBlocked(struct pcb_t *p);

/* Return a pointer to the ProcBlk that is at the head of the process queue
 * associated with the semaphore semAdd. Return NULL if semAdd is not
 * found on the ASL or if the process queue associated with semAdd is empty */

struct pcb_t *headBlocked(int *semAdd);

/* Initialize the semdFree list to contain all the elements of the array */

void initASL(void);

#endif
