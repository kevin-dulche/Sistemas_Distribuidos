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
 * $Id: barrier.c,v 10.6.1.10 1998/08/17 16:31:41 alc Exp $
 *
 * Description:    
 *	barrier synchronization routines
 *
 * External Functions:
 *			Tmk_barrier,
 *			Tmk_barrier_initialize
 *
 * External Variables:
 *			Tmk_barrier_sigio_handler
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox	Created
 *	26-Jul-1993	Alan L. Cox	Added reliable message protocol
 *	26-Oct-1993	Alan L. Cox	Update inverse time in barrier
 *	 7-Jan-1994	Alan L. Cox	Shrink data structures
 *
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *
 *	Version	0.9.2
 *
 *	21-May-1995	Alan L. Cox	Adapted for SGI/IRIX
 *
 *	Version 0.9.3
 *
 *	17-Jul-1995	Alan L. Cox	Eliminated the message size parameter
 *					 to the barrier duplicate handler
 *	22-Jul-1995	Alan L. Cox	#ifdef'ed the Ultrix/ATM specific code
 *
 *	Version 0.9.4
 *
 *	16-Nov-1995	Alan L. Cox	Modified the barrier to merge (and
 *					  delay) the mprotects
 *	Version 0.9.6
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

static	struct	barrier	{
	unsigned char		manager;
	unsigned volatile	mask;
}			barrier_[NBARRIERS];

static	struct	req_typ	req_typ = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_ARRIVAL };

static	char		req_data[MTU - sizeof(req_typ) - sizeof(proc_vector_time_)];

static	struct	iovec	req_iov[3] = {
	{ (caddr_t)&req_typ, sizeof(req_typ) },
	{ (caddr_t) proc_vector_time_, sizeof(proc_vector_time_) },
	{ req_data, 0 } };

static	struct	msghdr	req_hdr = { 0, 0, req_iov, sizeof(req_iov)/sizeof(req_iov[0]), 0, 0 };

static	struct	rep_typ	{
	unsigned	seqno;
	unsigned short	repo;
	char		data[MTU - sizeof(unsigned) - sizeof(unsigned short)];
}			rep;

/*
 *
 */
void
Tmk_barrier(
	unsigned	id)
{
#if defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
	Tmk_stat.arrivals++;

	if (Tmk_debug)
		Tmk_err("barrier: %d ", id);

	if (id < NBARRIERS) {

		struct barrier *barrier = &barrier_[id];
		sigset_t	mask;
		int		j, manager, repo, size;

		sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

		Tmk_interval_create(proc_vector_time_);

		if ((repo = Tmk_diff_repo_test()) == 0)
			repo = Tmk_interval_repo_test();

		if ((manager = barrier->manager) == Tmk_proc_id) {

			struct req_syn *req;

			rep.repo = repo;

			for (j = 0; j < Tmk_nprocs; j++) {
				if (j == Tmk_proc_id)
					continue;

				if ((1 << j) & barrier->mask) {

					req = req_from_[j].req;

					Tmk_interval_incorporate((caddr_t)&req->vector_time[NPROCS],
								 req_from_[j].size - sizeof(struct req_syn),
								 NULL);

					if (req->type == REQ_ARRIVAL_REPO)
						rep.repo = 1;
				}
			}
			barrier->mask |= 1 << Tmk_proc_id;

			while (barrier->mask ^ Tmk_spinmask)
				sigio_handler();

			barrier->mask = 0;

			for (j = 0; j < Tmk_nprocs; j++) {
				if (j == Tmk_proc_id)
					continue;

				req = req_from_[j].req;

				rep.seqno = req->seqno;

				size = Tmk_interval_request(rep.data, req->vector_time) - (caddr_t)&rep;

				Tmk_sigio_buffer_release(req);

				while (0 > send(rep_fd_[j], (char *)&rep, size, 0))
					Tmk_errno_check("Tmk_barrier<send>");
			}
			/*
			 * Perform the merged mprotects.
			 */
			Tmk_page_inval_perform();
		}
		else {
			req_typ.type = repo ? REQ_ARRIVAL_REPO : REQ_ARRIVAL;
			req_typ.id = id;

			req_iov[2].iov_len = Tmk_interval_request_proc(req_data, Tmk_proc_id, inverse_time_[manager]) - req_data;

			req_typ.seqno = req_seqno += SEQNO_INCR;
	       rexmit:
			while (0 > sendmsg(req_fd_[manager], &req_hdr, 0))
				Tmk_errno_check("Tmk_barrier<sendmsg>");

			Tmk_tout_flag = 0;

			setitimer(ITIMER_REAL, &Tmk_tout, NULL);

			sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);
		retry:
			if ((size = recv(req_fd_[manager], (char *)&rep, sizeof(rep), 0)) < 0)
				if (Tmk_tout_flag) {

					if (Tmk_debug)
						Tmk_err("Tmk_barrier: %d timed out (seqno %d)\n",
							manager, req_typ.seqno);

					sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

					goto rexmit;
				}
				else if (errno == EINTR)
					goto retry;
				else
					Tmk_perrexit("Tmk_barrier<recv>");

			if (rep.seqno != req_typ.seqno) {

				if (Tmk_debug)
					Tmk_err("Tmk_barrier: bad seqno %d from %d (received %d)\n",
						req_typ.seqno, manager, rep.seqno);

				goto retry;
			}
			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

			Tmk_stat.messages++;
			Tmk_stat.bytes += size;

			Tmk_interval_incorporate(rep.data, size - sizeof(rep.seqno) - sizeof(rep.repo), proc_vector_time_);
		}
		for (j = 0; j < Tmk_nprocs; j++) {
			if (j == Tmk_proc_id)
				continue;

			inverse_time_[j] = proc_vector_time_[Tmk_proc_id];
		}
		if (rep.repo)
			Tmk_repo();

		sigio_mutex(SIG_SETMASK, &mask, NULL);
	}
	else
		Tmk_errexit("Tmk_barrier: id == %d\n", id);
#if defined(PTHREADS)
	pthread_mutex_unlock(&Tmk_monitor_lock);
#endif
}

/*
 *
 */
void
Tmk_barrier_sigio_handler(
	const struct req_syn *req,
	int		size)
{
	if ((barrier_[req->id].mask |= (1 << req->from)) & (1 << Tmk_proc_id)) {

		Tmk_interval_incorporate((caddr_t)&req->vector_time[NPROCS], size - sizeof(struct req_syn), NULL);

		if (req->type == REQ_ARRIVAL_REPO)
			rep.repo = 1;
	}
}

/*
 *
 */
void
Tmk_barrier_sigio_duplicate_handler(
	const struct req_syn *req,
	int		size)
{
	int		from = req->from;

	if ((barrier_[req->id].mask & (1 << from)) == 0) {

		int	size = Tmk_interval_request(rep.data, req->vector_time) - (caddr_t)&rep;

		rep.seqno = req->seqno;
	/*	rep.repo = ? */

		while (0 > send(rep_fd_[from], (char *)&rep, size, 0))
			Tmk_errno_check("Tmk_barrier_sigio_duplicate_handler<send>");

		if (Tmk_debug)
			Tmk_err("Tmk_barrier_sigio_duplicate_handler: repeated seqno %d from %d\n",
				req->seqno, from);
	}
}

/*
 *
 */
void
Tmk_barrier_initialize( void )
{
	int	i;

	for (i = 0; i < NBARRIERS; i++)
		barrier_[i].manager = i % Tmk_nprocs;

	req_typ.from = Tmk_proc_id;
}
