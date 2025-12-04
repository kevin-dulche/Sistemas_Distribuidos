/*****************************************************************************
 *                                                                           *
 *  Copyright (c) 1991-1996						     *
 *  by ParallelTools, L.L.C. (PTOOLS), Houston, Texas			     *
 *                                                                           *
 *  This software is furnished under a license and may be used and copied    *
 *  only in accordance with the terms of such license and with the	     *
 *  inclusion of the above copyright notice.  This software or any other     *
 *  copies thereof may not be provided or otherwise made available to any    *
 *  other person.  No title to or ownership of the software is hereby	     *
 *  transferred.                                                             *
 *									     *
 *  The recipient of this software (RECIPIENT) acknowledges and agrees that  *
 *  the software contains information and trade secrets that are	     *
 *  confidential and proprietary to PTOOLS.  RECIPIENT agrees to take all    *
 *  reasonable steps to safeguard the software, and to prevent its	     *
 *  disclosure.								     * 
 *                                                                           *
 *  The information in this software is subject to change without notice     *
 *  and should not be construed as a commitment by PTOOLS.		     *
 *                                                                           *
 *  This software is furnished AS IS, without warranty of any kind, either   *
 *  express or implied (including, but not limited to, any implied warranty  *
 *  of merchantability or fitness), with regard to the software.  PTOOLS     *
 *  assumes no responsibility for the use or reliability of its software.    *
 *  PTOOLS shall not be liable for any special,	incidental, or		     *
 *  consequential damages, or any damages whatsoever due to causes beyond    *
 *  the reasonable control of PTOOLS, loss of use, data or profits, or from  *
 *  loss or destruction of materials provided to PTOOLS by RECIPIENT.	     *
 *									     *
 *  PTOOLS's liability for damages arising out of or in connection with the  *
 *  use or performance of this software, whether in an action of contract    *
 *  or tort including negligence, shall be limited to the purchase price,    *
 *  or the total amount paid by RECIPIENT, whichever is less.		     *
 *                                                                           *
 *****************************************************************************/

/*
 * $Id: interval.c,v 10.5.1.5 1998/05/31 00:39:46 alc Exp $
 *
 * Description:    
 *	manages consistency data
 *
 * External Functions:
 *			Tmk_interval_create,
 *			Tmk_interval_incorporate,
 *			Tmk_interval_initialize,
 *			Tmk_interval_repo,
 *			Tmk_interval_repo_test,
 *			Tmk_interval_request,
 *			Tmk_interval_request_proc
 *
 * External Variables:
 *			proc_array_[NPROCS]
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	11-Apr-1993	Alan L. Cox 	Created
 *	27-Jun-1993	Alan L. Cox	Unified heap for intervals and write
 *					 notices
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	Version 0.9.2
 *
 *	 3-May-1995	Alan L. Cox	Modified heap to allow on-demand
 *					 expansion
 *	Version 0.9.3
 *
 *	16-Nov-1995	Alan L. Cox	Modified interval incorporate to merge
 *					 mprotects for page invalidation
 *	Version 0.9.6
 */
#include "Tmk.h"

/*
 * Initialized below
 */
struct	interval	proc_array_[NPROCS];

#if defined(__alpha) || _MIPS_SZPTR == 64
#define H_SIZE	524288
#else
#define H_SIZE	262144
#endif

typedef	struct	heap	{
	struct	heap   *next;
	char	heap_[H_SIZE - sizeof(struct interval) - sizeof(struct heap *)];
	char	heap_overflow_[sizeof(struct interval)];
}      *heap_t;

static	heap_t	heap_start;

static	heap_t	heap_current;

static	heap_t	heap_expand( void )
{
	heap_t	heap = (heap_t) malloc(sizeof(struct heap));

	if (heap == 0)
		Tmk_errexit("<malloc>Tmk_heap_expand: out of memory\n");

	heap->next = 0;

	return heap;
}

static	char   *heap_brk;

static	char   *heap_end;

static	void	heap_ptr_initialize(heap)
	heap_t	heap;
{
	heap_current = heap;

	heap_brk = heap->heap_;

	heap_end = heap->heap_overflow_;
}

static	char   *heap_alloc(size)
	size_t	size;
{
	char   *heap_ptr = heap_brk;

	if ((heap_brk = heap_ptr + size) >= heap_end) {

		heap_t	heap = heap_current->next;

		if (heap == 0) {

			heap = heap_expand();

			heap_current->next = heap;
		}
		heap_ptr_initialize(heap);
	}
	return heap_ptr;
}

/*
 * Called by Tmk_barrier
 */
int	Tmk_interval_repo_test()
{
	return heap_start != heap_current;
}

/*
 * Called by Tmk_repo and Tmk_interval_initialize
 */
void	Tmk_interval_repo()
{
	interval_t	interval = proc_array_;

	interval_t	last_interval = &proc_array_[Tmk_nprocs];

	do {
		interval->prev = interval->next = interval;
		interval++;
	} while (interval < last_interval);

	heap_ptr_initialize(heap_start);
}

/*
 * The caller must block sigio.  Called by
 */
void	Tmk_interval_create(vector_time_)
	unsigned short *vector_time_;
{
	interval_t	interval__prev;
	interval_t	interval__head;

	interval_t	interval;

	write_notice_range_t
			write_notice_range;

	/*
	 * Create an interval (and record) if a page is dirty
	 */
	page_t		end = &page_dirty;

	page_t		page = end->prev;

	if (page != end) {

		end->prev = end->next = end;

		interval = (interval_t) heap_alloc(sizeof(struct interval));
		interval->id = Tmk_proc_id;

		/*
		 * Enqueue the interval record
		 */
		interval->next = interval__head = &proc_array_[Tmk_proc_id];
		interval->prev = interval__prev = interval__head->prev;

		interval__head->prev = interval;
		interval__prev->next = interval;

		vector_time_[Tmk_proc_id] += 1;

		memcpy(interval->vector_time_, vector_time_, sizeof(interval->vector_time_));

		/*
		 * Create the write notice records
		 */
		write_notice_range = &interval->head;

		goto skip;

		do {
			page_t partner;

			write_notice_range = write_notice_range->next = (write_notice_range_t) heap_alloc(sizeof(struct write_notice_range));
		skip:
			write_notice_range->first_page_id = page - page_array_;
			write_notice_range->last_page_id = page->partner - page_array_;

			partner = page->partner;
			partner[1].dirty_toggle = page->dirty_toggle = 0;

			do {
				write_notice_t	write_notice = (write_notice_t) heap_alloc(sizeof(struct write_notice));
				write_notice->next = partner->write_notice_[Tmk_proc_id];
				
				partner->write_notice_[Tmk_proc_id] = write_notice;
				
				write_notice->diff = 0;
				write_notice->interval = interval;

			} while ((partner -= 1) >= page);

		} while ((page = page->prev) != end);

		write_notice_range->next = 0;
	}
}

/*
 * All fields of the message are unsigned shorts.  The pid is
 * complemented to distinguish it from a page id.  Thus, the
 * highest page id is 65536-NPROCS-1.
 */

/*
 * Called by Tmk_barrier (slave).
 */
caddr_t	Tmk_interval_request_proc(msg, i, time)
	caddr_t		msg;
	int		i;
	unsigned	time;
{
	unsigned short *msgp = (unsigned short *) msg;

	interval_t	interval = &proc_array_[i];
	interval_t	interval__prev;
	interval_t	ihdr = interval;

	if (time < (interval__prev = interval->prev)->vector_time_[i]) {

		do {
			interval = interval__prev;
			interval__prev = interval->prev;
		} while (time < interval__prev->vector_time_[i]);

		do {
			write_notice_range_t
					write_notice_range;

			*msgp++ = ~i;

			memcpy(msgp, interval->vector_time_, sizeof(interval->vector_time_));

			msgp += NPROCS;

			write_notice_range = &interval->head;

			do {
				int	j = write_notice_range->last_page_id - write_notice_range->first_page_id;

				if (j > 0) {

					if (j > 1)
						*msgp++ = ~NPROCS;

					*msgp++ = write_notice_range->first_page_id;
				}
				*msgp++ = write_notice_range->last_page_id;

			} while (write_notice_range = write_notice_range->next);

		} while ((interval = interval->next) != ihdr);
	}
	return (caddr_t) msgp;
}

/*
 * Called by Tmk_barrier (master) and Tmk_lock_release.
 */
caddr_t
Tmk_interval_request(
	caddr_t		msg,
	const unsigned short
			vector_time_[])
{
	int		i = 0;

	do {
		msg = Tmk_interval_request_proc(msg, i, vector_time_[i]);
	} while (++i < Tmk_nprocs);

	return msg;
}

/*
 *
 */
void	Tmk_interval_incorporate(msg, size, vector_time_)
	caddr_t		msg;
	int		size;
	unsigned short	vector_time_[];
{
	unsigned short *msgp = (unsigned short *) msg;
	unsigned short *msg_end = (unsigned short *)(msg + size);

	if (Tmk_debug) {

		int	i;

		Tmk_err("incorporate:");

		for (i = 0; &msgp[i] < msg_end; i++)
			Tmk_err(" %d", msgp[i]);

		Tmk_err("\n");
	}
	while (msgp < msg_end) {

		/*
		 * Perform sign extension before the one's complement.
		 */
		int		pid = ~*(short *) msgp++;

		unsigned	u, v;

		/*
		 * Insert new intervals; skip old intervals
		 */
		if ((u = msgp[pid]) > proc_vector_time_[pid]) {

			interval_t	interval;
			interval_t	interval__prev;
			interval_t	interval__head;

			write_notice_range_t
					write_notice_range;

			proc_vector_time_[pid] = u;

			interval = (interval_t) heap_alloc(sizeof(struct interval));
			interval->id = pid;

			/*
			 * Enqueue the interval record
			 */
			interval->next = interval__head = &proc_array_[pid];
			interval->prev = interval__prev = interval__head->prev;

			interval__head->prev = interval;
			interval__prev->next = interval;

			memcpy(interval->vector_time_, msgp, sizeof(interval->vector_time_));

			msgp += NPROCS;

			/*
			 * Create write notice records
			 */
			write_notice_range = &interval->head;

			u = *msgp;

			goto skip;

			do {
				page_t	page, page_end;

				write_notice_range = write_notice_range->next = (write_notice_range_t) heap_alloc(sizeof(struct write_notice_range));
			skip:
				v = u;
				if (u == (unsigned short) ~NPROCS) {
					u = *++msgp;
					v = *++msgp;
				}
				write_notice_range->first_page_id = u;
				write_notice_range->last_page_id = v;

				page = &page_array_[u];
				page_end = &page_array_[v];

				do {
					write_notice_t	write_notice = (write_notice_t) heap_alloc(sizeof(struct write_notice));
					write_notice->next = page->write_notice_[pid];
	
					page->write_notice_[pid] = write_notice;

					write_notice->diff = 0;
					write_notice->interval = interval;

					/*
					 * Change page status
					 */
					if (page->state == valid) {

						page->state = invalid;

						while (page->twin) {

							write_notice = page->write_notice_[Tmk_proc_id];

							if (write_notice && write_notice->diff == 0) {

								Tmk_diff_create(page, write_notice);

								Tmk_twin_free(page);
							}
							else {
								Tmk_interval_create(vector_time_);

								proc_vector_time_[Tmk_proc_id] = vector_time_[Tmk_proc_id];
							}
						}
						Tmk_page_inval_merge(page);
					}
				} while ((page += 1) <= page_end);

			} while ((msgp += 1) < msg_end && (u = *msgp) < (unsigned short) -NPROCS);

			write_notice_range->next = 0;

			if ((u = interval->vector_time_[Tmk_proc_id]) > inverse_time_[pid])
				inverse_time_[pid] = u;
		}
		else
			for (msgp += NPROCS; msgp < msg_end && *msgp < (unsigned short) -NPROCS; msgp++)
				;
	}
	if (vector_time_)
		Tmk_page_inval_perform();
}

/*
 *
 */
void	Tmk_interval_initialize()
{
	heap_start = heap_expand();

	Tmk_interval_repo();
}
