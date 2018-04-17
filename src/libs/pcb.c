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
#include <const.h>
#include <types.h>
#include <helplib.h>
#include <libuarm.h>

#include <clist.h>


/************************************************
 * The Allocation and Deallocation of ProcBlk's *
 ************************************************/

static struct clist pcbFree=CLIST_INIT;

/*insert the element pointed to by p onto the pcbFree list*/
void freePcb(struct pcb_t *p){
        clist_push(p, &pcbFree, p_list)
}

/*return an element from the freePcb list*/
struct pcb_t *allocPcb(){
        if (clist_empty(pcbFree)) return NULL;
        else {
                struct pcb_t *newPcb;
                newPcb = clist_head(newPcb, pcbFree, p_list);
                clist_pop(&pcbFree);
                mymemset(newPcb, 0, sizeof(struct pcb_t));
                return newPcb;
        }
}

/*allocate pcbs and fill freePcb - run once*/
void initPcbs(void){
        static struct pcb_t pcbArray[MAXPROC]; //visible only here, accessible outside with poiters
        int i = 0;
        for(i;i<MAXPROC;i++){
                clist_push(&pcbArray[i], &pcbFree, p_list);
        }
}


/*****************************
 * Process Queue Maintenance *
 *****************************/

/*Insert the ProcBlk pointed to by p into the process queue whose list-tail*/
/*pointer is q.*/
void insertProcQ(struct clist *q, struct pcb_t *p){

        clist_enqueue(p, q, p_list);
}

/*Remove the first (i.e. head) element from the process queue whose list-*/
/*tail pointer is q. Return NULL if the process queue was initially empty;*/
/*otherwise return the pointer to the removed element.*/
struct pcb_t *removeProcQ(struct clist *q){

        if (clist_empty(*q)) return NULL;
        else {
                struct pcb_t *head = clist_head(head, *q, p_list);
                clist_dequeue(q);
                head->p_list.next = NULL;
                return head;
        }
}

/*Remove the ProcBlk pointed to by p from the process queue whose list-tail*/
/*pointer is q. If the desired entry is not in the indicated queue (an error*/
/*condition), return NULL; otherwise, return p. Note that p can point to*/
/*any element of the process queue.*/
struct pcb_t *outProcQ(struct clist *q, struct pcb_t *p){

        if (clist_delete(p, q, p_list) == 1) 
                return NULL; 
        else {
                p->p_list.next = NULL;
                return p;
        }

}

/*Return a pointer to the first ProcBlk from the process queue whose list-*/
/*tail pointer is q. Do not remove this ProcBlk from the process queue.*/
/*Return NULL if the process queue is empty.*/
struct pcb_t *headProcQ(struct clist *q){
        struct pcb_t *head = clist_head(head, *q, p_list);
        return head;
}


/****************************
 * Process Tree Maintenance *
 ****************************/

/* Return TRUE if the ProcBlk pointed to by p has no children. Return FALSE otherwise */
int emptyChild(struct pcb_t *p){
        return clist_empty(p->p_children);
}

/* Make the ProcBlk pointed to by p a child of the ProcBlk pointed to by parent */
void insertChild(struct pcb_t *parent, struct pcb_t *p){
        clist_push(p, &(parent->p_children), p_siblings);
        p->p_parent = parent;
}

/* Make the first child of the ProcBlk pointed to by p no longer a child of p.
 * Return NULL if initially there were no children of p. Otherwise, return a
 * pointer to this removed first child ProcBlk */
struct pcb_t *removeChild(struct pcb_t *p){
        if (emptyChild(p)) return NULL;
        else {
                struct pcb_t *child = clist_head(p, p->p_children, p_siblings);
                clist_pop(&(p->p_children));
                /* doesnt need to search for it since its always the first element */
                child->p_parent = NULL;
                return child;
        }
}

/* Make the ProcBlk pointed to by p no longer the child of its parent.
 * If the ProcBlk pointed to by p has no parent, return NULL; otherwise, return p.
 * Note that the element pointed to by p need not be the first child of its parent */
struct pcb_t *outChild(struct pcb_t *p){
        if ( !(p->p_parent) ) return NULL;
        else {
                struct pcb_t *parent = p->p_parent;
                clist_delete(p, &(parent->p_children), p_siblings);
                p->p_parent = NULL;
                return p;
        }
}
