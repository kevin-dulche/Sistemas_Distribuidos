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
 * $Id: sched.c,v 10.11.1.12 1998/08/17 16:31:41 alc Exp $
 *
 * Description:    
 *	loop scheduling routines
 *
 * External Functions:
 *			Tmk_sched_start,
 *			Tmk_sched_fork,
 *                      Tmk_sched_terminate
 *
 * External Variables:
 *			Tmk_sched_sigio_handler
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Jul-1996	Zheng-Hua Li	Created
 *
 *	Version 0.10.2
 */
#include "Tmk.h"

static	struct	sched	{
	unsigned char		manager;
	unsigned volatile	mask;
}			sched_[NPROCS];

static	struct	req_typ	req_typ;

static	char		req_data[MTU - sizeof(req_typ) - sizeof(proc_vector_time_)];

static	struct	iovec	req_iov[3] = {
	{ (caddr_t)&req_typ, sizeof(req_typ) },
	{ (caddr_t) proc_vector_time_, sizeof(proc_vector_time_) },
	{ req_data, 0 } };
static	struct	msghdr	req_hdr = { 0, 0, req_iov, sizeof(req_iov)/sizeof(req_iov[0]), 0, 0 };

struct	reply_struct	{
        unsigned	seqno;
	unsigned short	repo;
	unsigned short	dummy;
	void          (*func_ptr)( Tmk_sched_arg_t ); /* the function pointer */
	struct	Tmk_sched_arg   arg;
};

static	struct	rep_typ	{
        unsigned	seqno;
	unsigned short	repo;
	unsigned short	dummy;
	void          (*func_ptr)( Tmk_sched_arg_t ); /* the function pointer */
	struct	Tmk_sched_arg   arg;
	char		data[MTU - sizeof(struct reply_struct)];
}			rep;

/*
 * Called by the master process.  At termination, "sched->mask" is full.
 * This prevents a "duplicate" reply by Tmk_sched_sigio_duplicate_handler
 * until the initial reply is sent by Tmk_sched_fork.
 */
void
Tmk_sched_join( void )
{
	int		j, repo;
	struct sched   *sched = &sched_[Tmk_proc_id];
	struct req_syn *req;
	sigset_t	mask;

	sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

	/*
	 * In order to merge invalidations from different processors, the last
	 * argument to Tmk_interval_incorporate must be NULL.  The last
	 * argument can only be NULL if all dirty pages have write notices.
	 */
	Tmk_interval_create(proc_vector_time_);

	if ((repo = Tmk_diff_repo_test()) == 0)
		repo = Tmk_interval_repo_test();

	rep.repo = repo;

	for (j = 0; j < Tmk_nprocs; j++) {
		if (j == Tmk_proc_id)
			continue;

		if ((1 << j) & sched->mask) {

			req = req_from_[j].req;

			Tmk_interval_incorporate((caddr_t)&req->vector_time[NPROCS],
						 req_from_[j].size - sizeof(struct req_syn),
						 NULL);

			if (req->type == REQ_JOIN_REPO)
				rep.repo = 1;
		}
	}
	sched->mask |= 1 << Tmk_proc_id;

	while (sched->mask ^ Tmk_spinmask)
		sigio_handler();

	/*
	 * Perform the merged mprotects.
	 */
	Tmk_page_inval_perform();

	sigio_mutex(SIG_SETMASK, &mask, NULL);
}

/*
 *
 */
int
Tmk_sched_start(
	unsigned	manager)
{
	if (Tmk_debug)
		Tmk_err("sched: manager %d ", manager);

	if (manager == Tmk_proc_id) {
	
		Tmk_sched_join();

		return 1;
	}
	else if (manager < Tmk_nprocs)
		for (;;) {

			int		j, repo, size;
			sigset_t	mask;

			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

			Tmk_interval_create(proc_vector_time_);

			if ((repo = Tmk_diff_repo_test()) == 0)
				repo = Tmk_interval_repo_test();

			req_typ.type = repo ? REQ_JOIN_REPO : REQ_JOIN;
			req_typ.id = manager;

			req_iov[2].iov_len = Tmk_interval_request_proc(req_data, Tmk_proc_id, inverse_time_[manager]) - req_data;

			req_typ.seqno = req_seqno += SEQNO_INCR;
		rexmit:
			while (0 > sendmsg(req_fd_[manager], &req_hdr, 0))
				Tmk_errno_check("Tmk_sched_start<sendmsg>");

			Tmk_tout_flag = 0;

			setitimer(ITIMER_REAL, &Tmk_tout, NULL);

			sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);
		retry:
			if ((size = recv(req_fd_[manager], (char *)&rep, sizeof(rep), 0)) < 0)
				if (Tmk_tout_flag) {

					if (Tmk_debug)
						Tmk_err("Tmk_sched_start: %d timed out (seqno %d)\n",
							manager, req_typ.seqno);

					sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

					goto rexmit;
				}
				else if (errno == EINTR)
					goto retry;
				else
					Tmk_perrexit("Tmk_sched_start<recv>");

			if (rep.seqno != req_typ.seqno) {

				if (Tmk_debug)
					Tmk_err("Tmk_sched_start: bad seqno %d from %d (received %d)\n",
						req_typ.seqno, manager, rep.seqno);

				goto retry;
			}
			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

			Tmk_stat.messages++;
			Tmk_stat.bytes += size;

			Tmk_interval_incorporate(rep.data, size - sizeof(struct reply_struct), proc_vector_time_);

			for (j = 0; j < Tmk_nprocs; j++) {
				if (j == Tmk_proc_id)
					continue;

				inverse_time_[j] = proc_vector_time_[Tmk_proc_id];
			}
			if (rep.repo)
				Tmk_repo();

			sigio_mutex(SIG_SETMASK, &mask, NULL);
			
			if (rep.func_ptr)
				(*rep.func_ptr)(&rep.arg);
			else
				return 0;
		}
	else
		Tmk_errexit("Tmk_sched_start: manager == %d\n", manager);

	/*
	 * This statement is unreachable but necessary to pacify
	 * some compilers.
	 */
	return -1;
}

/*
 *
 */
void
Tmk_sched_fork(
	void	      (*func_ptr)( Tmk_sched_arg_t arg ),
	Tmk_sched_arg_t arg)
{
	int		j, size;
	struct req_syn *req;
	sigset_t	mask;

	rep.func_ptr = func_ptr;

	if (arg)
		rep.arg = *arg;

	sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

	sched_[Tmk_proc_id].mask = 0;

	Tmk_interval_create(proc_vector_time_);

	for (j = 0; j < Tmk_nprocs; j++) {
		if (j == Tmk_proc_id)
			continue;

		req = req_from_[j].req;

		rep.seqno = req->seqno;

		size = Tmk_interval_request(rep.data, req->vector_time) - (caddr_t)&rep;

		Tmk_sigio_buffer_release(req);

		while (0 > send(rep_fd_[j], (char *)&rep, size, 0))
			Tmk_errno_check("Tmk_sched_fork<send>");
	}
	for (j = 0; j < Tmk_nprocs; j++) {
		if (j == Tmk_proc_id)
			continue;

		inverse_time_[j] = proc_vector_time_[Tmk_proc_id];
	}
	if (rep.repo)
		Tmk_repo();

	sigio_mutex(SIG_SETMASK, &mask, NULL);

	if (func_ptr)
		(*func_ptr)(arg);
}

/*
 *
 */
void
Tmk_sched_exit( void )
{
	Tmk_sched_fork(NULL, NULL);
}

/*
 *
 */
void
Tmk_sched_sigio_handler(
	const struct req_syn *req,
	int		size)
{
	if ((sched_[req->id].mask |= (1 << req->from)) & (1 << Tmk_proc_id)) {

		Tmk_interval_incorporate((caddr_t)&req->vector_time[NPROCS], size - sizeof(struct req_syn), NULL);

		if (req->type == REQ_JOIN_REPO)
			rep.repo = 1;
	}
}

/*
 *
 */
void
Tmk_sched_sigio_duplicate_handler(
	const struct req_syn *req,
	int		size)
{
	int		from = req->from;

	if ((sched_[req->id].mask & (1 << from)) == 0) {

		int	size = Tmk_interval_request(rep.data, req->vector_time) - (caddr_t)&rep;

		rep.seqno = req->seqno;
	/*	rep.repo = ? */

		while (0 > send(rep_fd_[from], (char *)&rep, size, 0))
			Tmk_errno_check("Tmk_sched_sigio_duplicate_handler<send>");

		if (Tmk_debug)
			Tmk_err("Tmk_sched_sigio_duplicate_handler: repeated seqno %d from %d\n",
				req->seqno, from);
	}
}

/*
 *
 */
void
Tmk_sched_initialize( void )
{
	int	i = 0;

	do {
		sched_[i].manager = i % Tmk_nprocs;
	} while (++i < Tmk_nprocs);

	req_typ.from = Tmk_proc_id;
}
