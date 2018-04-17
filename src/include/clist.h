/* Circular lists macro
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
#ifndef _CLIST_H
#define _CLIST_H

#include <types.h>

#define container_of(ptr, type, member) ({                              \
                        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                        (type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/* constant used to initialize an empty list */
#define CLIST_INIT {NULL}
#define HEAD(clistp) (clistp)->next->next
#define TAIL(clistp) (clistp)->next

/* add the structure pointed to by elem as the last element of a circular list */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */
#define clist_enqueue(elem, clistp, member)             \
        if (TAIL(clistp)) {                             \
                (elem)->member.next=HEAD(clistp);       \
                HEAD(clistp)=&((elem)->member);         \
                TAIL(clistp)= HEAD(clistp);             \
        }                                               \
        else {                                          \
                TAIL(clistp)=&((elem)->member);         \
                HEAD(clistp)=TAIL(clistp);              \
        }                                               \

/* add the structure pointed to by elem as the first element of a circular list */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */
#define clist_push(elem, clistp, member)                \
        if (TAIL(clistp)) {                             \
                (elem)->member.next=HEAD(clistp);       \
                HEAD(clistp)=&((elem)->member);         \
        }                                               \
        else {                                          \
                TAIL(clistp)=&((elem)->member);         \
                HEAD(clistp)=TAIL(clistp);              \
        }

/* clist_empty returns true in the circular list is empty, false otherwise */
/* clistx is a struct clist */
#define clist_empty(clistx) (((clistx).next)==NULL)

/* return the pointer of the first element of the circular queue.
   elem is also an argument to retrieve the type of the element */
/* member is the field of *elem used to link this list */
#define clist_head(elem, clistx, member) ({                             \
                        typeof(elem) __hptr;                            \
                        if ((clistx).next) __hptr = container_of( ((clistx).next)->next, typeof(*elem), member); \
                        else __hptr = NULL;                             \
                        __hptr;                                         \
                })

/* return the pointer of the last element of the circular queue.
   elem is also an argument to retrieve the type of the element */
/* member is the field of *elem used to link this list */
#define clist_tail(elem, clistx, member) ({                             \
                        typeof(elem) __tptr;                            \
                        if ((clistx).next) __tptr = container_of((clistx).next, typeof(*elem), member); \
                        else __tptr = NULL;                             \
                        __tptr;                                         \
                })

/* clist_pop and clist__dequeue are synonyms */
/* delete the first element of the list (this macro does not return any value) */
/* clistp is the address of the tail pointer (struct clist *) */
#define clist_pop(clistp) clist_dequeue(clistp)
#define clist_dequeue(clistp)                                           \
        if (!clist_empty(*clistp)) {                                    \
                if (TAIL(clistp) == HEAD(clistp)) TAIL(clistp) = NULL;  \
                else HEAD(clistp) = HEAD(clistp)->next;                 \
        }

/* delete from a circular list the element whose pointer is elem */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */
#define clist_delete(elem, clistp, member) ({                           \
                        int result = 1;                                 \
                        void *__tmp = NULL;                             \
                        typeof(elem) __scan;                            \
                        clist_foreach(__scan, clistp, member, __tmp){   \
                                if (__scan == elem) {                   \
                                        clist_foreach_delete(__scan, clistp, member, __tmp); \
                                        result = 0;                     \
                                        break;                          \
                                }                                       \
                        }                                               \
                        result;                                         \
                })

/* this macro has been designed to be used as a for instruction,
   the instruction (or block) following clist_foreach will be repeated for each element
   of the circular list. elem will be assigned to each element */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */
/* tmp is a void * temporary variable */
#define clist_foreach(scan, clistp, member, tmp)                        \
        tmp = NULL;                                                     \
        scan = clist_head(scan, *clistp, member);                       \
        for (bool _deleted = FALSE ; tmp!=TAIL(clistp); _clist_foreach_increment(_deleted, tmp, scan, clistp, member) )

/* Increment macro for the clist for loop. 
 *
 * Because of our implementation of clist_foreach_delete (refer to that macro description), 
 * we need to consider 4 limit cases, which we managed with 3 different increments. */

#define _clist_foreach_increment(_deleted, tmp, scan, clistp, member) ({ \
                        /* Cases 2 and 4 */                             \
                        if (_deleted) {                                 \
                                if (tmp == NULL) scan = container_of( HEAD(clistp), typeof(*scan), member); \
                                else  scan=container_of( ( (typeof(clistp)) tmp)->next, typeof(*scan), member ); \
                                _deleted = FALSE;                       \
                        }                                               \
                        /* Case 1 and 3 and generic non-deleting increment */ \
                        else if (tmp!=TAIL(clistp)) {                   \
                                tmp=&(scan->member);                    \
                                scan=container_of( ( (typeof(clistp)) tmp)->next, typeof(*scan), member ); \
                        }                                               \
                })

/* this macro should be used after the end of a clist_foreach cycle
   using the same args. it returns false if the cycle terminated by a break,
   true if it scanned all the elements */
#define clist_foreach_all(scan, clistp, member, tmp) ({ \
                        (tmp == TAIL(clistp));          \
                })

/* this macro should be used *inside* a clist_foreach loop to delete the
 * current element 
 * We need to consider 4 cases:
 *  - deleting the head in a list of only one element
 *  - deleting the head in a list of multiple elements
 *  - deleting the tail in a list of multiple elements
 *  - deleting an generic element
 *
 * In cases 2 and 4 we set a service variable to handle their for loop increment. 
 */

#define clist_foreach_delete(scan, clistp, member, tmp)                 \
        if (tmp == NULL) { /* when scan is head tmp is still NULL */    \
                if (HEAD(clistp) == TAIL(clistp))                       \
                        /* one element list */                          \
                        TAIL(clistp) = NULL;                            \
                else {                                                  \
                        /* delete head */                               \
                        HEAD(clistp) = HEAD(clistp)->next;              \
                        _deleted = TRUE;                                \
                }                                                       \
        }                                                               \
        else {                                                          \
                _deleted = TRUE;                                        \
                /* if deleting tail*/                                   \
                if ( ( (typeof(clistp)) tmp)->next == TAIL(clistp) ) {  \
                        TAIL(clistp) = tmp;                             \
                        _deleted = FALSE;                               \
                }                                                       \
                /* generic case */                                      \
                ( (typeof(clistp)) tmp)->next = ( (typeof(clistp)) tmp)->next->next; \
        }

/* this macro should be used *inside* a clist_foreach loop to add an element
   before the current one */
#define clist_foreach_add(elem, scan, clistp, member, tmp)              \
        if (tmp == NULL) {                                              \
                (elem)->member.next = HEAD(clistp);                     \
                HEAD(clistp) = &((elem)->member);                       \
        }                                                               \
        else {                                                          \
                (elem)->member.next = ( (typeof(clistp)) tmp)->next;    \
                ( (typeof(clistp)) tmp)->next = &((elem)->member);      \
        }

#endif
