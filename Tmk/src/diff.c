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
 * $Id: diff.c,v 10.10.1.15.0.4 1998/08/06 05:59:08 alc Exp $
 *
 * Description:    
 *	diff management routines
 *
 * External Functions:
 *			Tmk_diff_create,
 *			Tmk_diff_initialize,
 *			Tmk_diff_repo,
 *			Tmk_diff_repo_test,
 *			Tmk_diff_request,
 *			Tmk_diff_sigio_handler
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox 	Created
 *	26-Oct-1993	Alan L. Cox	Optimized diff create
 *	19-Nov-1993	Alan L. Cox	Adapted for RS/6000
 *	 7-Jan-1994	Alan L. Cox	Partial retransmission of diff req
 *	 5-Jun-1994	Alan L. Cox	Reduced diff requests
 *	13-Jun-1994	Alan L. Cox	Overlapped diff requests
 *	14-Jun-1994	Alan L. Cox	Adapted for Alpha
 *					 (changes provided by Povl T. Koch)
 *	24-Jul-1994	Alan L. Cox	Immediate retransmission performed
 *					 after high seqno received
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	10-Dec-1994	Alan L. Cox	Changed to a single seqno counter
 *					 (suggested by Pete Keleher)
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *	28-Jan-1995	Alan L. Cox	Corrected iov merge by sigio handler
 *
 *	Version 0.9.2
 *
 *	21-Apr-1995	Alan L. Cox	Optimized diff create
 *	20-May-1995	Alan L. Cox	Modified heap to allow on-demand
 *					 expansion
 *	21-May-1995	Alan L. Cox	Adapted for SGI/IRIX
 *	 7-Jun-1995	Sandhya Dwarkadas
 *					Corrected request retransmission seqno
 *					 (See 10-Dec-1994 change)
 *	Version 0.9.3
 *
 *	 1-Jul-1995	Alan L. Cox	Added volatile to diff create to
 *					 prevent memory access reordering 
 *	Version 0.9.4
 *
 *	17-Dec-1995	Alan L. Cox	Moved the mprotect in the sigio
 *					 handler after the send
 *	Version 0.9.7
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

#define	H_SIZE	1048576

typedef	struct	heap	{
	struct	heap   *next;
	char	heap_[H_SIZE - MTU - sizeof(struct heap *)];
	char	heap_overflow_[MTU];
}      *heap_t;

static	heap_t	heap_expand( void )
{
	heap_t	heap = (heap_t) malloc(sizeof(struct heap));

	if (heap == 0)
		Tmk_errexit("<malloc>Tmk_heap_expand: out of memory");

	heap->next = 0;

	return heap;
}

static	heap_t	heap_1_start;

static	heap_t	heap_1_current;

static	char   *heap_1_brk;

static	char   *heap_1_end;

static	void	heap_1_ptr_initialize(heap)
	heap_t	heap;
{
	heap_1_current = heap;

	heap_1_brk = heap->heap_;

	heap_1_end = heap->heap_overflow_;
}

static	struct	req_dif	req = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_DIFF };

static	unsigned	rep_seqno;
static	struct	iovec	rep_iov[2] = {
	{ (caddr_t)&rep_seqno, sizeof(rep_seqno) },
	{                   0, MTU - sizeof(rep_seqno) } };
static	struct	msghdr	rep_hdr = { 0, 0, rep_iov, sizeof(rep_iov)/sizeof(rep_iov[0]), 0, 0 };

static	heap_t	heap_2_start;

static	heap_t	heap_2_current;

#	define	heap_2_brk	*(char **)&rep_iov[1].iov_base	/* void *? */

static	char   *heap_2_end;

static	void	heap_2_ptr_initialize(heap)
	heap_t	heap;
{
	heap_2_current = heap;

	heap_2_brk = heap->heap_;

	heap_2_end = heap->heap_overflow_;
}

/*
 * Called by Tmk_repo
 */
void
Tmk_diff_repo( void )
{
	heap_1_ptr_initialize(heap_1_start);

	heap_2_ptr_initialize(heap_2_start);
}

/*
 * Called by Tmk_barrier
 */
int
Tmk_diff_repo_test( void )
{
	return heap_1_current != heap_1_start;
}

/*
 * Contorted to avoid cache conflict misses and pipeline stalls.  On
 * Intel processors, preload the diff into the cache to take advantage
 * of page mode access.
 *
 * See Tmk_diff_create for the description of the diff encoding.
 */
static	caddr_t	diff_apply(caddr_t page, caddr_t diff)
{
	short  *diff_cur = (short *) diff;
	short  *diff_run_end;

	int	run_length;

	while ((run_length = *diff_cur), (diff_cur += 2), (run_length > 0)) {

		int	run_offset = *(diff_cur - 1);
		int    *run_dest_cur = (int *)(page + run_offset);

		if (run_length & 4) {

			int	diff0 = *(int *) diff_cur;

			diff_cur += sizeof(int)/sizeof(*diff_cur);

			run_dest_cur[0] = diff0;

			if ((run_length -= 4) == 0)
				continue;

			run_dest_cur++;
		}
		if (run_length & 8) {

			int	diff0 = *(int *) diff_cur;
			int	diff1 = *(int *)(diff_cur + sizeof(int)/sizeof(*diff_cur));

			diff_cur += 2*sizeof(int)/sizeof(*diff_cur);

			run_dest_cur[0] = diff0;
			run_dest_cur[1] = diff1;

			if ((run_length -= 8) == 0)
				continue;

			run_dest_cur += 2;
		}
#if defined(__i386)
		{
			int	i = 0;

			do
				*(volatile int *)((caddr_t) diff_cur + i);
			while ((i += 32) < run_length);
		}
#endif
		diff_run_end = (short *)((caddr_t) diff_cur + run_length);

		do {
			int	diff0 = *(int *) diff_cur;
			int	diff1 = *(int *)(diff_cur + sizeof(int)/sizeof(*diff_cur));
			int	diff2 = *(int *)(diff_cur + 2*sizeof(int)/sizeof(*diff_cur));
			int	diff3 = *(int *)(diff_cur + 3*sizeof(int)/sizeof(*diff_cur));

			diff_cur += 4*sizeof(int)/sizeof(*diff_cur);

			run_dest_cur[0] = diff0;
			run_dest_cur[1] = diff1;
			run_dest_cur[2] = diff2;
			run_dest_cur[3] = diff3;
			run_dest_cur += 4;

		} while (diff_cur != diff_run_end);
	}
	return (caddr_t) diff_cur;
}

/*
 * Called by segv_handler and Tmk_repo
 *
 * The algorithm for sorting the write notices and requesting the
 * corresponding DIFFs isn't optimal, i.e., it isn't guaranteed to
 * use the minimal number of request messages.  In practice, however,
 * it works well.
 */
void	Tmk_diff_request(page)
	page_t		page;
{
static	struct	interval	Interval;
static	struct	write_notice	Write_notice = { 0, 0, 0, &Interval };

#define NWRITES	((MTU - sizeof(struct req_typ))/(2*sizeof(unsigned short)))

	int		i, j, n = 0;
	write_notice_t	write_[NWRITES + 1];

	write_[0] = &Write_notice;

	i = 0;

	do {
		write_notice_t	write = page->write_notice_[i]; 

		for (j = 0; write && write->diff == 0; j++) {

			interval_t	interval = write->interval;

			int		k;
		next_j:
			k = write_[j]->interval->id;

			if (interval->vector_time_[k] < write_[j]->interval->vector_time_[k]) {
				j++; goto next_j;
			}

			k = (n += 1);

			do {
				write_[k] = write_[k - 1];
			} while (--k > j);

			write_[k] = write;

			write = write->next;
		}
	} while (++i < Tmk_nprocs);

	if (n > 0) {

		struct	preq	{
			int		to;
			unsigned	seqno;
			int		i;
			int		n;	/* i < n */
		}	preq_[NPROCS*NPROCS];

		struct	preq   *preq = preq_;

		unsigned short *req_data_ptr;

		req.seqno = req_seqno += SEQNO_INCR;
		req.id = page - page_array_;

		n--;

		do {
			int		dom = n - 1;

			req_data_ptr = req.data;

			for (preq->n = n;; dom = MIN(dom, n - 1)) {

				do {
					interval_t	interval = write_[n]->interval;

					int		id = interval->id;

					req_data_ptr[0] = id;
					req_data_ptr[1] = interval->vector_time_[id];
					req_data_ptr += 2;

					if (Tmk_debug) {

						Tmk_err("(%d) ", id);

						j = 0;

						do {
							Tmk_err("%d ", interval->vector_time_[j]);
						} while (++j < Tmk_nprocs);
					}

				} while (--n > dom);

				if (n < 0)
					goto xmit;
			redo:
				i = n + 1;

				do {
					j = write_[i]->interval->id;

					if (write_[dom]->interval->vector_time_[j] < write_[i]->interval->vector_time_[j]) {

						if (--dom < 0)
							goto xmit;

						goto redo;
					}
				} while (--i > dom);
			}
		xmit:
			if (Tmk_debug)
				Tmk_err("Sending\n");

			preq->seqno = req_seqno;
			preq->to    = j = req_data_ptr[-2];
			preq->i     = n;
			preq++;

			while (0 > send(req_fd_[j], (char *)&req, (char *) req_data_ptr - (char *)&req, 0))
				Tmk_errno_check("Tmk_diff_request<send>");

		} while (n >= 0);

		preq = preq_;

		req_data_ptr = 0;

		do {
			caddr_t	diff;
			caddr_t	diff_end;
			int	size;
			sigset_t
				mask;
		receive:
			Tmk_tout_flag = 0;

			setitimer(ITIMER_REAL, &Tmk_tout, NULL);

			sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, &mask);
		retry:
			if ((size = recvmsg(req_fd_[preq->to], &rep_hdr, 0)) < 0)
				if (Tmk_tout_flag) {

					if (Tmk_debug)
						Tmk_err("<timeout: %d>Tmk_diff_request: seqno == %d\n", preq->to, preq->seqno);
		rexmit:
					sigio_mutex(SIG_SETMASK, &mask, NULL);

					req_data_ptr = req.data;

					for (i = preq->n; i > preq->i; i--) {

						interval_t	interval = write_[i]->interval;

						int		id = interval->id;

						req_data_ptr[0] = id;
						req_data_ptr[1] = interval->vector_time_[id];
						req_data_ptr += 2;
					}
					req.seqno = preq->seqno;

					while (0 > send(req_fd_[preq->to], (char *)&req, (char *) req_data_ptr - (char *)&req, 0))
						Tmk_errno_check("Tmk_diff_request<send>");

					Tmk_stat.rexmits++;

					goto receive;
				}
				else if (errno == EINTR)
					goto retry;
				else
					Tmk_perrexit("<recvmsg>Tmk_diff_request");

			if (rep_seqno != preq->seqno) {

				if (Tmk_debug)
					Tmk_err("<bad seqno: %d>Tmk_diff_request: seqno == %d (received: %d)\n", preq->to, preq->seqno, rep_seqno);

				/*
				 * If the received seqno is greater than the expected seqno and
				 * the request hasn't been retransmitted, retransmit immediately.
				 * Don't wait for the timeout to retransmit.
				 */
				if (rep_seqno > preq->seqno && req_data_ptr == 0)
					goto rexmit;

				goto retry;
			}
			if (Tmk_debug)
				Tmk_err("size: %d\n", size);

			sigio_mutex(SIG_SETMASK, &mask, NULL);

			req_data_ptr = 0;

			Tmk_stat.messages++;
			Tmk_stat.bytes += size;
			Tmk_stat.bytes_of_data += size -= sizeof(rep_seqno);

			diff = heap_2_brk;

			diff_end = diff + size;

			if ((heap_2_brk = diff_end) >= heap_2_end) {

				heap_t	heap = heap_2_current->next;

				if (heap == 0) {

					heap = heap_expand();

					heap_2_current->next = heap;
				}
				heap_2_ptr_initialize(heap);
			}
			for (;;) {

				write_notice_t	write = write_[preq->n];

				write->diff = diff;
#if defined(PTHREADS)
				write->diff_size = (diff = diff_apply(page->v_alias, diff)) - write->diff;
#else
				write->diff_size = (diff = diff_apply(page->vadr, diff)) - write->diff;
#endif
				if ((preq->n -= 1) == preq->i)
					break;

				if (diff == diff_end) {

					if ((preq->seqno += SEQNO_INCR) > req_seqno)
						req_seqno = preq->seqno;

					goto receive;
				}
			}

		} while (preq++->i >= 0);
	}
}

/*
 * Called by diff_sigio_handler and interval_incorporate
 *
 * The format of a diff is <size in bytes,starting offset>,word,word, ...
 */
void	Tmk_diff_create(page, write_notice)
	page_t		page;
	write_notice_t	write_notice;
{
#if	defined(__GNUC__) && defined(__i386)
	register int   *vadr asm("edi") = (int *) page->vadr;
	register int   *twin asm("esi") = (int *) page->twin;
	int	       *begin = vadr + 1;
#else
	volatile int   *vadr = (volatile int *) page->vadr;
	volatile int   *twin = (volatile int *) page->twin;
	volatile int   *begin = vadr + 1;
#endif
	int	       *end = (int *)((caddr_t) vadr + Tmk_page_size);
	unsigned short *start;
	int	       *diff = (int *) heap_1_brk;

#if	defined(__GNUC__) && defined(__sparc)
	register	vadr0 asm("l0");
	register	vadr1 asm("l1");
	register	vadr2 asm("l2");
	register	vadr3 asm("l3");
#elif ! defined(__i386)
	int		vadr0;
	int		vadr1;
	int		vadr2;
	int		vadr3;
#endif
#if	defined(__GNUC__) && defined(__sparc)
	register	twin0 asm("l4");
	register	twin1 asm("l5");
	register	twin2 asm("l6");
	register	twin3 asm("l7");
#elif ! defined(__i386)
	int		twin0;
	int		twin1;
	int		twin2;
	int		twin3;
#endif

	write_notice->diff = (caddr_t) diff;

#if	defined(__i386)
	{
		volatile int   *vadr_cur = vadr;

		do
			*vadr_cur;
		while ((vadr_cur += 32/sizeof(*vadr_cur)) < end);
	}
#endif

#if	defined(__i386)
	{
		volatile int   *twin_cur = twin;
		volatile int   *twin_end = (volatile int *)((caddr_t) twin + Tmk_page_size);

		do
			*twin_cur;
		while ((twin_cur += 32/sizeof(*twin_cur)) < twin_end);
	}
#endif
	do {
#if	defined(__GNUC__) && defined(__sparc)
		asm ("ldd %2,%0" : "=r" (vadr0), "=r" (vadr1) : "m" (vadr[0]));
		asm ("ldd %2,%0" : "=r" (vadr2), "=r" (vadr3) : "m" (vadr[2]));
#elif	defined(__i386)
#	define	vadr0	vadr[-4]
#	define	vadr1	vadr[-3]
#	define	vadr2	vadr[-2]
#	define	vadr3	vadr[-1]
#else
		vadr0 = vadr[0];
		vadr1 = vadr[1];
		vadr2 = vadr[2];
		vadr3 = vadr[3];
#endif
		vadr += 4;

#if	defined(__GNUC__) && defined(__sparc)
		asm ("ldd %2,%0" : "=r" (twin0), "=r" (twin1) : "m" (twin[0]));
		asm ("ldd %2,%0" : "=r" (twin2), "=r" (twin3) : "m" (twin[2]));
#elif	defined(__i386)
#	define	twin0	twin[-4]
#	define	twin1	twin[-3]
#	define	twin2	twin[-2]
#	define	twin3	twin[-1]
#else
		twin0 = twin[0];
		twin1 = twin[1];
		twin2 = twin[2];
		twin3 = twin[3];
#endif
		twin += 4;

		if (twin0 != vadr0) {

			diff += 2*sizeof(*start)/sizeof(*diff);
			start = (unsigned short *) diff;
			start[-1] = (caddr_t) vadr - (caddr_t)(begin + 3);

			goto diff0;
	same0:
			start[-2] = (caddr_t) diff - (caddr_t) start;
		}
		if (twin1 != vadr1) {

			diff += 2*sizeof(*start)/sizeof(*diff);
			start = (unsigned short *) diff;
			start[-1] = (caddr_t) vadr - (caddr_t)(begin + 2);

			goto diff1;
	same1:
			start[-2] = (caddr_t) diff - (caddr_t) start;
		}
		if (twin2 != vadr2) {

			diff += 2*sizeof(*start)/sizeof(*diff);
			start = (unsigned short *) diff;
			start[-1] = (caddr_t) vadr - (caddr_t)(begin + 1);

			goto diff2;
	same2:
			start[-2] = (caddr_t) diff - (caddr_t) start;
		}
		if (twin3 != vadr3) {

			diff += 2*sizeof(*start)/sizeof(*diff);
			start = (unsigned short *) diff;
			start[-1] = (caddr_t) vadr - (caddr_t) begin;

			goto diff3;
	same3:
			start[-2] = (caddr_t) diff - (caddr_t) start;
		}
	} while (vadr != end);

	goto done;

	do {
#if	defined(__GNUC__) && defined(__sparc)
		asm ("ldd %2,%0" : "=r" (vadr0), "=r" (vadr1) : "m" (vadr[0]));
		asm ("ldd %2,%0" : "=r" (vadr2), "=r" (vadr3) : "m" (vadr[2]));
#elif ! defined(__i386)
		vadr0 = vadr[0];
		vadr1 = vadr[1];
		vadr2 = vadr[2];
		vadr3 = vadr[3];
#endif
		vadr += 4;

#if	defined(__GNUC__) && defined(__sparc)
		asm ("ldd %2,%0" : "=r" (twin0), "=r" (twin1) : "m" (twin[0]));
		asm ("ldd %2,%0" : "=r" (twin2), "=r" (twin3) : "m" (twin[2]));
#elif ! defined(__i386)
		twin0 = twin[0];
		twin1 = twin[1];
		twin2 = twin[2];
		twin3 = twin[3];
#endif
		twin += 4;

		if (twin0 == vadr0)
			goto same0;

	diff0:	*diff++ = vadr0;

		if (twin1 == vadr1)
			goto same1;

	diff1:	*diff++ = vadr1;

		if (twin2 == vadr2)
			goto same2;

	diff2:	*diff++ = vadr2;

		if (twin3 == vadr3)
			goto same3;

	diff3:	*diff++ = vadr3;

	} while (vadr != end);

	start[-2] = (caddr_t) diff - (caddr_t) start;
 done:
	*diff++ = 0;

	write_notice->diff_size = (caddr_t) diff - write_notice->diff;

	if ((heap_1_brk = (caddr_t) diff) >= heap_1_end) {

		heap_t	heap = heap_1_current->next;

		if (heap == 0) {

			heap = heap_expand();

			heap_1_current->next = heap;
		}
		heap_1_ptr_initialize(heap);
	}
	Tmk_stat.diffs++;
	Tmk_stat.total_diff_size += write_notice->diff_size;
}

/*
 *
 */
void
Tmk_diff_sigio_handler(
	const struct req_dif *req,
	int		size)
{
static	unsigned	seqno;

static	struct	iovec	iov[MSG_MAXIOVLEN] = { { (caddr_t)&seqno, sizeof(seqno) } };

static	struct	msghdr	hdr = { 0, 0, iov, 0, 0, 0 };

	int		fd = rep_fd_[req->from];

	page_t		page = &page_array_[req->id];

	const unsigned short
		       *ptr = req->data;
	unsigned short *end = (unsigned short *)((caddr_t) req + size);
#if ! defined(PTHREADS)
	int		perform_mprotect_and_free = 0;
#endif
	int		j = 1;

	seqno = req->seqno;

	size = sizeof(seqno);

	do {
		int		pid = ptr[0];
		int		time = ptr[1];
		write_notice_t	write_notice = page->write_notice_[pid];

		while (write_notice->interval->vector_time_[pid] != time)
			write_notice = write_notice->next;

		if (write_notice->diff == 0) {
#if defined(PTHREADS)
			/*
			 * Prevent further writes by another thread
			 * on this node before diff'ing.
			 */
			if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ))
				Tmk_perrexit("Tmk_diff_sigio_handler<mprotect>");
#endif
			Tmk_diff_create(page, write_notice);
#if defined(PTHREADS)
			Tmk_twin_free(page);
#else
			perform_mprotect_and_free = 1;
#endif
		}
		if ((size += write_notice->diff_size) > MTU) {

			hdr.msg_iovlen = j;

			while (0 > sendmsg(fd, &hdr, 0))
				Tmk_errno_check("Tmk_diff_sigio_handler<sendmsg>");

			j = 1;

			seqno += SEQNO_INCR;

			size = sizeof(seqno) + write_notice->diff_size;
		}
		/*
		 * Account for the seqno that uses entry 0
		 */
		if ((j > 2) && (write_notice->diff == ((caddr_t) iov[j - 1].iov_base + iov[j - 1].iov_len)))
			iov[j - 1].iov_len += write_notice->diff_size;
		else {
			iov[j].iov_base = write_notice->diff;
			iov[j].iov_len  = write_notice->diff_size;

			if ((j += 1) == MSG_MAXIOVLEN) {

				hdr.msg_iovlen = j;

				while (0 > sendmsg(fd, &hdr, 0))
					Tmk_errno_check("Tmk_diff_sigio_handler<sendmsg>");

				j = 1;

				seqno += SEQNO_INCR;

				size = sizeof(seqno);
			}
		}

	} while ((ptr += 2) < end);

	if (j > 1) {

		hdr.msg_iovlen = j;

		while (0 > sendmsg(fd, &hdr, 0))
			Tmk_errno_check("Tmk_diff_sigio_handler<sendmsg>");
	}
#if ! defined(PTHREADS)
	if (perform_mprotect_and_free) {

		if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ))
			Tmk_perrexit("<mprotect>diff_sigio_handler");

		Tmk_twin_free(page);
	}
#endif
}

/*
 *
 */
void
Tmk_diff_initialize( void )
{
	heap_1_ptr_initialize(heap_1_start = heap_expand());

	heap_2_ptr_initialize(heap_2_start = heap_expand());

	req.from = Tmk_proc_id;
}
