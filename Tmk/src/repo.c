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
 * $Id: repo.c,v 10.10.1.13.0.2 1998/09/02 04:18:54 alc Exp $
 *
 * Description:    
 *	storage reclamation routines
 *
 * External Functions:
 *			Tmk_repo
 *			Tmk_repo_sigio_handler
 *			Tmk_repo_initialize
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	17-Jun-1993	Alan L. Cox	Created
 *	26-Oct-1993	Alan L. Cox	Updated the manager for empty pages
 *	18-Jul-1994	Alan L. Cox	Reduced the number of page replicas
 *
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	Version 0.9.2
 *
 *	29-Apr-1995	Alan L. Cox	Adapted for STREAMS
 *	15-Jun-1995	Cristiana Amza	Adapted for HPPA/HPUX
 *
 *	Version 0.9.3
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

static	struct	req	{
	unsigned	seqno;
	unsigned char	from;
	unsigned char	type;
}			req_typ = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_REPO };

static volatile
	unsigned	repo_mask = 1;

/*
 *
 */
void
Tmk_repo_sigio_handler(
	const struct req *req)
{
	repo_mask |= 1 << req->from;
}

/*
 *
 */
void
Tmk_repo_sigio_duplicate_handler(
	const struct req *req)
{
	if ((repo_mask & (1 << req->from)) == 0)
		while (0 > send(rep_fd_[req->from], (char *)&req->seqno, sizeof(req->seqno), 0))
			Tmk_errno_check("Tmk_repo_sigio_duplicate_handler<send>");
}

/*
 *
 */
static
int	repo_update_manager(
	page_t	page)
{
	int	j;

	for (j = 0; j < Tmk_nprocs; j++) {

		write_notice_t	write_notice;

		if (write_notice = page->write_notice_[j]) {

			interval_t	interval = write_notice->interval;

			for (j++; j < Tmk_nprocs; j++)
				if (write_notice = page->write_notice_[j]) {

					int	k = 0;

					do {
						if (interval->vector_time_[k] > write_notice->interval->vector_time_[k])
							goto gtr;
					} while (++k < Tmk_nprocs);

					interval = write_notice->interval;
				gtr:
					continue;
				}
			page->manager = interval->id;

			return 1;
		}
	}
	return 0;
}

/*
 *
 */
static
void	repo_barrier( void )
{
	if (Tmk_proc_id == 0) {

		int	j;

		while (repo_mask ^ Tmk_spinmask)
			sigio_handler();

		repo_mask = 1;

		for (j = 1; j < Tmk_nprocs; j++)
			while (0 > send(rep_fd_[j], (char *)&rep_seqno_[j], sizeof(rep_seqno_[j]), 0))
				Tmk_errno_check("repo_barrier<send>");
	}
	else {
		sigset_t	mask;

		unsigned	rep_seqno;

		req_typ.seqno = req_seqno += SEQNO_INCR;
	rexmit:
		while (0 > send(req_fd_[0], (char *)&req_typ, sizeof(req_typ), 0))
			Tmk_errno_check("repo_barrier<send>");

		Tmk_tout_flag = 0;

		setitimer(ITIMER_REAL, &Tmk_tout, NULL);

		sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, &mask);
	retry:
		if (0 > recv(req_fd_[0], (char *)&rep_seqno, sizeof(rep_seqno), 0))
			if (Tmk_tout_flag) {

				if (Tmk_debug)
					Tmk_err("<timeout: 0>repo_barrier: seqno == %d\n", req_typ.seqno);

				sigio_mutex(SIG_SETMASK, &mask, NULL);

				goto rexmit;
			}
			else if (errno == EINTR)
				goto retry;
			else
				Tmk_perrexit("<recv>repo_barrier");

		if (rep_seqno != req_typ.seqno) {

			if (Tmk_debug)
				Tmk_err("<bad seqno: 0>repo_barrier: seqno == %d (received: %d)\n", req_typ.seqno, rep_seqno);

			goto retry;
		}
		sigio_mutex(SIG_SETMASK, &mask, NULL);

		Tmk_stat.messages++;
		Tmk_stat.bytes += sizeof(rep_seqno);
	}
}

/*
 * The caller must block sigio
 */
void	Tmk_repo( void )
{
	int	first_page_id, i, j, last_page_id;

	if (Tmk_debug)
		Tmk_err("repo: begin\n");

	Tmk_stat.repos++;

	last_page_id  = -1;
	first_page_id = Tmk_npages;

	for (i = 0; i < Tmk_nprocs; i++) {

		interval_t	interval = &proc_array_[i];

		while ((interval = interval->prev) != &proc_array_[i]) {

			write_notice_range_t	write_notice_range = &interval->head;

			do {
				if ((int) write_notice_range->first_page_id < first_page_id)
					first_page_id = write_notice_range->first_page_id;

				if ((int) write_notice_range->last_page_id > last_page_id)
					last_page_id = write_notice_range->last_page_id;

			} while ((write_notice_range = write_notice_range->next) != NULL);
		}
	}
	for (i = first_page_id; i <= last_page_id; i++) {

		page_t	page = &page_array_[i];

		if (page->state == valid) {

			if (page->twin) {

				Tmk_twin_free(page);

				page->state = private;
			}
		}
		else if (page->state == invalid) {

			write_notice_t	write_notice;

			if (Tmk_debug)
				Tmk_err("page: %d ", i);

			if (write_notice = page->write_notice_[Tmk_proc_id]) {

				unsigned short	time = write_notice->interval->vector_time_[Tmk_proc_id];

				for (j = 0; j < Tmk_nprocs; j++) {
					if (j == Tmk_proc_id)
						continue;

					if (write_notice = page->write_notice_[j]) {

						if (write_notice->interval->vector_time_[Tmk_proc_id] >= time)
							goto discard;
					}
				}
#if ! defined(PTHREADS)
				if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ|PROT_WRITE))
					Tmk_perrexit("<mprotect>Tmk_repo");
#endif
				page->state = valid;

				Tmk_diff_request(page);

				if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ))
					Tmk_perrexit("<mprotect>Tmk_repo");
			}
			else {
		discard:
				page->state = empty;

				if (0 == repo_update_manager(page))
					Tmk_errexit("Tmk_repo\n");

				if (Tmk_debug)
					Tmk_err("discarded\n");
			}
		}
		else if (page->state == empty)
			repo_update_manager(page);
	}
	repo_barrier();

	memset(proc_vector_time_, 0, sizeof(proc_vector_time_));

	memset(inverse_time_, 0, sizeof(inverse_time_));

	for (i = first_page_id; i <= last_page_id; i++) {

		page_t	page = &page_array_[i];

		memset(page->write_notice_, 0, sizeof(page->write_notice_));
	}
	Tmk_diff_repo();

	Tmk_interval_repo();

	/*
	 * Prevent a post repo lock request from receiving out-of-date
	 * consistency information.  Without the following repo
	 * barrier, a post repo lock request might be handled within
	 * the earlier repo barrier (before the consistency
	 * information is flushed). 
	 */
	repo_barrier();

	if (Tmk_debug)
		Tmk_err("repo: end\n");
}

/*
 *
 */
void	Tmk_repo_initialize( void )
{
	req_typ.from = Tmk_proc_id;
}
