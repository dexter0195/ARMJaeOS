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
#include <types.h>
#include <pcb.h>
#include <clist.h>



static struct clist aslh, semdFree;


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

int insertBlocked(int *semAdd, struct pcb_t *p){
        struct semd_t *scan, *new;
        void *tmp = NULL;
        bool found = FALSE;
        clist_foreach(scan, &aslh, s_link, tmp){
                if (scan->s_semAdd == semAdd) {
                        found = TRUE;
                        break;
                }
        }
        new = scan;
        /* this could also be if (clist_foreach_all(scan, &aslh, s_link, tmp))
         * and avoid using bool */
        if (!found) {
                /* take first elem from semFree */
                new = clist_head(new, semdFree, s_link);
                if (new == NULL) return TRUE; //semFree empty, error
                clist_dequeue(&semdFree);
                /* reset its fields */
                new->s_semAdd = semAdd;
                new->s_procq.next = NULL;
                /* add it to ASL */
                clist_foreach(scan, &aslh, s_link, tmp){
                        if (new->s_semAdd < scan->s_semAdd){
                                clist_foreach_add(new, scan, &aslh, s_link, tmp);
                                break;
                        }
                }
                if (clist_foreach_all(scan, &aslh, s_link, tmp))
                        clist_enqueue(new, &aslh, s_link);
        }
        insertProcQ( &(new->s_procq), p);
        p->p_cursem = new;
        return FALSE;
}

/* Search the ASL for a descriptor of this semaphore. If none is found, return
 * NULL; otherwise, remove the first (i.e. head) ProcBlk from the process
 * queue of the found semaphore descriptor and return a pointer to it. If the
 * process queue for this semaphore becomes empty remove the semaphore
 * descriptor from the ASL and return it to the semdFree list */

struct pcb_t *removeBlocked(int *semAdd){
        struct pcb_t *head = NULL;
        struct semd_t *scan;
        void *tmp = NULL;
        clist_foreach(scan, &aslh, s_link, tmp){
                if (scan->s_semAdd == semAdd){
                        head = removeProcQ( &(scan->s_procq) );
                        /* head should never be NULL here because if a sem is
                         * in ASL its s_procq is not empty, but just in case */
                        if (head != NULL) head->p_cursem = NULL;
                        if (headProcQ( &(scan->s_procq) ) == NULL){
                                clist_foreach_delete(scan, &aslh, s_link, tmp);
                                clist_push(scan, &semdFree, s_link);
                        }
                        break;
                }
        }
        /* no need to check if the for loop finished without finding semAdd,
         * because if it did so, the head is already NULL */
        return head;
}

/* Remove the ProcBlk pointed to by p from the process queue associated
 * with p's semaphore on the ASL. If ProcBlk pointed to by p does not
 * appear in the process queue associated with p's semaphore, which is an
 * error condition, return NULL; otherwise, return p
 *
 * NOTE: in the original function description there is no mention about
 * what to do if a semaphore process queue becomes empty after outBlocked removes 
 * the last blocked pcb_t. 
 * We decided to have this function behave like removeBlocked does: it will remove 
 * semaphore descriptors with an empty process queue from the ASL and return it to
 * the semdFree list. 
 * Not doing so could eventually result in an empty semaphore in the A(ctive)SL and 
 * we would like to avoid that.
 * If outBlocked is not supposed to behave like this for some phase 2 reason we 
 * will modify it accordingly. */

struct pcb_t *outBlocked(struct pcb_t *p){
        struct pcb_t *ret = p;
        struct semd_t *scan;
        void *tmp = NULL;
        bool found = FALSE;
        clist_foreach(scan, &aslh, s_link, tmp){
                if (scan == p->p_cursem){
                        found = TRUE;
                        ret = outProcQ( &(scan->s_procq), p);
                        /* p->p_cursem is reset anyway */
                        p->p_cursem = NULL;
                        /* p might have been the only element in scan->s_procq
                         * if so we should free it */
                        if (headProcQ( &(scan->s_procq) ) == NULL){
                                clist_foreach_delete(scan, &aslh, s_link, tmp);
                                clist_push(scan, &semdFree, s_link);
                        }
                        break;
                }
        }
        /* if p cursem is not in the ASL (should never happen) */
        if (!found)
                return NULL;
        /* ret is NULL if p was not in its cursem procq */
        return ret;
}

/* Return a pointer to the ProcBlk that is at the head of the process queue
 * associated with the semaphore semAdd. Return NULL if semAdd is not
 * found on the ASL or if the process queue associated with semAdd is empty */

struct pcb_t *headBlocked(int *semAdd){
        struct pcb_t *head = NULL;
        struct semd_t *scan;
        void *tmp = NULL;
        clist_foreach(scan, &aslh, s_link, tmp){
                if (scan->s_semAdd == semAdd){
                        head = headProcQ( &(scan->s_procq) );
                        break;
                }
        }
        return head;
}

/* Initialize the semdFree list to contain all the elements of the array */

void initASL(void){
        static struct semd_t semdTable[MAXPROC]; //visible only here, accessible outside with poiters
        int i = 0;
        for(i;i<MAXPROC;i++){
                clist_push(&semdTable[i], &semdFree, s_link);
        }
}
