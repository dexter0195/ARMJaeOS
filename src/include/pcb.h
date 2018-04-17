/* Pcb maintenance functions
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
#ifndef _PCB
#define _PCB

#include <types.h>

/************************************************
 * The Allocation and Deallocation of ProcBlk's *
 ************************************************/

/*insert the element pointed to by p onto the pcbFree list*/
void freePcb(struct pcb_t *p);

/*return an element from the freePcb list*/
struct pcb_t *allocPcb();

/*allocate pcbs and fill freePcb - run once*/
void initPcbs(void);

/*****************************
 * Process Queue Maintenance *
 *****************************/

/*Insert the ProcBlk pointed to by p into the process queue whose list-tail*/
/*pointer is q.*/
void insertProcQ(struct clist *q, struct pcb_t *p);

/*Remove the first (i.e. head) element from the process queue whose list-*/
/*tail pointer is q. Return NULL if the process queue was initially empty;*/
/*otherwise return the pointer to the removed element.*/
struct pcb_t *removeProcQ(struct clist *q);

/*Remove the ProcBlk pointed to by p from the process queue whose list-tail*/
/*pointer is q. If the desired entry is not in the indicated queue (an error*/
/*condition), return NULL; otherwise, return p. Note that p can point to*/
/*any element of the process queue.*/
struct pcb_t *outProcQ(struct clist *q, struct pcb_t *p);

/*Return a pointer to the first ProcBlk from the process queue whose list-*/
/*tail pointer is q. Do not remove this ProcBlk from the process queue.*/
/*Return NULL if the process queue is empty.*/
struct pcb_t *headProcQ(struct clist *q);

/****************************
 * Process Tree Maintenance *
 ****************************/

/* Return TRUE if the ProcBlk pointed to by p has no children. Return FALSE otherwise */
int emptyChild(struct pcb_t *p);

/* Make the ProcBlk pointed to by p a child of the ProcBlk pointed to by parent */
void insertChild(struct pcb_t *parent, struct pcb_t *p);

/* Make the first child of the ProcBlk pointed to by p no longer a child of p.
 * Return NULL if initially there were no children of p. Otherwise, return a
 * pointer to this removed first child ProcBlk */
struct pcb_t *removeChild(struct pcb_t *p);

/* Make the ProcBlk pointed to by p no longer the child of its parent.
 * If the ProcBlk pointed to by p has no parent, return NULL; otherwise, return p.
 * Note that the element pointed to by p need not be the first child of its parent */
struct pcb_t *outChild(struct pcb_t *p);

#endif
