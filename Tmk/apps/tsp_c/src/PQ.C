/*
 * pq.c:
 *
 *	Contains the routines for inserting an item into the priority
 * queue and removing the lowest priority item from the queue.
 *
 */
#include <stdio.h>

#include <Tmk.h>

#include "tsp.h"

#define LEFT_CHILD(x)	((x)<<1)
#define RIGHT_CHILD(x)	(((x)<<1)+1)

#define less_than(x,y)	(((x)->priority  < (y)->priority) || \
			 ((x)->priority == (y)->priority) && \
			  (global->Tours[(x)->index].last > global->Tours[(y)->index].last))

/*
 * DumpPrioQ():
 *
 * Dump the contents of PrioQ in some user-readable format (for debugging).
 *
 */
void
DumpPrioQ()
{
    int pq, ind;

    for (pq = 1; pq <= global->PrioQLast; pq++) {
	ind = global->PrioQ[pq].index;
	MakeTourString(global->Tours[ind].last, global->Tours[ind].prefix);
    }
}


/*
 * split_tour():
 *
 *  Break current tour into subproblems, and stick them all in the priority
 *  queue for later evaluation.
 *
 */
void
split_tour(curr_ind)
    int curr_ind;
{
    int n_ind, last_node, wt;
    register int i, pq, parent, index, priority;
    register TourElement *curr;
    register PrioQElement *cptr, *pptr;

    curr = &global->Tours[curr_ind];

    if (debug) {
	MakeTourString(curr->last, curr->prefix);
    }

    /* Create a tour and add it to the priority Q for each possible
       path that can be derived from the current path by adding a single
       node while staying under the current minimum tour length. */
    if (curr->last != (TspSize - 1)) {
	int t1, t2, t3;
	register TourElement *new;
	
	last_node = curr->prefix[curr->last];
	for (i = 0; i < TspSize; i++) {
	    /*
	     * Check: 1. Not already in tour
	     *	      2. Edge from last entry to node in graph
	     *	and   3. Weight of new partial tour is less than cur min.
             */
	    wt = global->weights[last_node][i];
	    t1 = ((curr->conn & (1<<i)) == 0);
	    t2 = (wt != 0);
	    t3 = (curr->lower_bound + wt) <= global->MinTourLen;
	    if (t1 && t2 && t3) {
		if ((n_ind = new_tour(curr_ind, i)) == END_TOUR) {
		    continue;
		}
		/*
		 * If it's shorter than the current best tour, or equal
		 * to the current best tour and we're reporting all min
		 * length tours, put it on the priority q.
		 */
		new = &global->Tours[n_ind];

		if (global->PrioQLast >= MAX_NUM_TOURS-1) {
		    fprintf(stderr, "pqLast %d\n", global->PrioQLast);
		    fflush(stderr);
		    exit(-1);
		}

		if (debugPrioQ) {
		    MakeTourString(new->last, new->prefix);
		}

		pq = ++global->PrioQLast;
		cptr = &(global->PrioQ[pq]);
		cptr->index = n_ind;
		cptr->priority = new->lower_bound;

		/* Bubble the entry up to the appropriate level to maintain
		   the invariant that a parent is less than either of it's
		   children. */
		for (parent = pq >> 1, pptr = &(global->PrioQ[parent]);
		     (pq > 1) && less_than(cptr,pptr);
		     pq = parent, cptr = pptr,
		     parent = pq >> 1, pptr = &(global->PrioQ[parent])) {

		    /* PrioQ[pq] lower priority than parent -> SWITCH THEM. */
		    index = cptr->index;
		    priority = cptr->priority;
		    cptr->index = pptr->index;
		    cptr->priority = pptr->priority;
		    pptr->index = index;
		    pptr->priority = priority;
		}
	    }
	    else if (debug) {
		/* Failed. */
		sprintf(tour_str, " [%d + %d > %d]",
			curr->lower_bound, wt, global->MinTourLen);
	    }
	}
    }
}


/*
 * find_solvable_tour():
 *
 * Used by both the normal TSP program (called by get_tour()) and
 * the RPC server (called by RPCserver()) to return the next solvable
 * (sufficiently short) tour.
 *
 */
find_solvable_tour()
{
    register int curr, i, left, right, child, index;
    int priority, last;
    register PrioQElement *pptr, *cptr, *lptr, *rptr;

    if (global->Done) return(-1);

    for (; global->PrioQLast != 0; ) {
	pptr = &(global->PrioQ[1]);
	curr = pptr->index;
	if (pptr->priority >= global->MinTourLen) {
	    /* We're done -- there's no way a better tour could be found. */
	    MakeTourString(global->Tours[curr].last, global->Tours[curr].prefix);
	    global->Done = 1;
	    return(-1);
	}

	/* Bubble everything maintain the priority queue's heap invariant. */
	/* Move last element to root position. */
	cptr = &(global->PrioQ[global->PrioQLast]);
	pptr->index    = cptr->index;
	pptr->priority = cptr->priority;
	global->PrioQLast--;

	/* Push previous last element down tree to restore heap structure. */
	for (i = 1; i <= (global->PrioQLast >> 1); ) {
	    /* Find child with lowest priority. */
	    left  = LEFT_CHILD(i);
	    right = RIGHT_CHILD(i);

	    lptr = &(global->PrioQ[left]);
	    rptr = &(global->PrioQ[right]);

	    if (left == global->PrioQLast || less_than(lptr,rptr)) {
		child = left;
		cptr = lptr;
	    }
	    else {
		child = right;
		cptr = rptr;
	    }

	    /* Exchange parent and child, if necessary. */
	    if (less_than(cptr,pptr)) {
		/* global->PrioQ[child] has lower prio than its parent - switch 'em. */
		index = pptr->index;
		priority = pptr->priority;
		pptr->index = cptr->index;
		pptr->priority = cptr->priority;
		cptr->index = index;
		cptr->priority = priority;
		i = child;
		pptr = cptr;
	    }
	    else break;
	}

	last = global->Tours[curr].last;
	
	if (debug) {
	}

	/* If we're within `NodesFromEnd' nodes of a complete tour, find
	   minimum solutions recursively.  Else, split the tour. */
	if (last < TspSize || last < 1) {
	    if (last >= (TspSize - NodesFromEnd - 1)) return(curr);
	    else split_tour(curr);	/* The current tour is too long, */
	}				/* to solve now, break it up.	 */
	else {
	    /* Bogus tour index. */
	    MakeTourString(TspSize, global->Tours[curr].prefix);
	    fprintf(stderr, "\t%d: %s\n", Tmk_proc_id, tour_str);
	}
	global->TourStack[++global->TourStackTop] = curr; /* Free tour. */
    }
    /* Ran out of candidates - DONE! */
    global->Done = 1;
    return(-1);
}


get_tour(curr)
    register int curr;
{
#ifdef	LOCK_PREFETCH
    PREFETCH2(global, global + 1);
#endif	/*LOCK_PREFETCH*/
    Tmk_lock_acquire(global->TourLock);
    if (curr != -1) global->TourStack[++global->TourStackTop] = curr;
    curr = find_solvable_tour();
    Tmk_lock_release(global->TourLock);

    return(curr);
}
