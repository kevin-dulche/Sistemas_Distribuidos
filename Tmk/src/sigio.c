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
 * $Id: sigio.c,v 10.16.1.18 1998/06/16 22:29:32 alc Exp $
 *
 * Description:    
 *	handle asynchronous request messages
 *
 * External Functions:
 *			Tmk_sigio_initialize
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox	Created
 *
 *	Version 0.9.1
 *
 *	10-Dec-1994	Alan L. Cox	Changed to a single seqno counter
 *					 (suggested by Pete Keleher)
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *
 *	Version 0.9.2
 *
 *	15-Jun-1995	Cristiana Amza	Adapted for HPPA/HPUX
 *
 *	Version 0.9.3
 *
 *	 3-Jul-1995	Alan L. Cox	Adapted for Power Challenge
 *
 *	17-Jul-1995	Alan L. Cox	Eliminated the message size parameter
 *					 to the barrier duplicate handler
 *	Version 0.9.4
 *
 *	17-Dec-1995	Alan L. Cox	Simplified the sigio handler
 *
 *	Version 0.9.7
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigvec with sigaction
 *
 *	Version 0.10
 *
 *	19-Apr-1996	Robert J. Fowler
 *					Adapted for Linux 1.2.13
 *	Version 0.10.1
 */
#include "Tmk.h"

#if defined(PTHREADS)
pthread_mutex_t Tmk_sigio_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

req_entry_t	req_from_[NPROCS];

/*
 * In order to load the vadr pointer in Tmk_distribute_sigio_handler,
 * the Power Challenge needs 64-bit alignment for the message buffer.
 */
static	long	sigio_buffer_[NPROCS][MTU/sizeof(long)];
static	long   *sigio_buffer_stack_[NPROCS];
static	long  **sigio_buffer_tos = sigio_buffer_stack_;

/*
 * Pushes the supplied buffer onto the free buffer stack.
 */
void
Tmk_sigio_buffer_release(
	long	*req)
{
	*--sigio_buffer_tos = req;
}

/*
 *
 */
static	void
Tmk_connect_sigio_handler(
	const struct req_typ *req,
	int		size)
{
	Tmk_errexit("sigio_handler: unexpected REQ_CONNECT\n");
}

/*
 * Used by sigio_handler to vector requests.
 */
static
void  (*const sigio_handlers_[])() = {
	Tmk_barrier_sigio_handler,
	Tmk_barrier_sigio_handler,
	Tmk_cond_broadcast_sigio_handler,
	Tmk_cond_signal_sigio_handler,
	Tmk_cond_wait_sigio_handler,
	Tmk_connect_sigio_handler,
	Tmk_diff_sigio_handler,
	Tmk_distribute_sigio_handler,
	Tmk_exit_sigio_handler,
	Tmk_sched_sigio_handler,
	Tmk_sched_sigio_handler,
	Tmk_lock_sigio_handler,
	Tmk_page_sigio_handler,
	Tmk_repo_sigio_handler
};

/*
 * Request types that require the buffer to persist.
 */
#define SIGIO_BUFFER_HOLD	((1 << REQ_ARRIVAL)|(1 << REQ_ARRIVAL_REPO)|\
				 (1 << REQ_JOIN)|(1 << REQ_JOIN_REPO))

#if defined(MPL)

static	int		MPL_id;

static	struct req_typ *MPL_req;

static	int		MPL_type = MPL_REQ;

static	void		MPL_rnc_initialize( void );

static	void
MPL_rnc_handler(
	int	       *id)
{
	struct req_typ *req = MPL_req;

	size_t		size;

	if (0 > mpc_wait(id, &size)) 
		Tmk_MPLerrexit("MPL_rnc_handler<mpc_wait>");

	if ((1 << req->type) & SIGIO_BUFFER_HOLD) {

		req_entry_t    *entry = &req_from_[req->from];

		entry->req  = (struct req_syn *) req;
		entry->size = size + sizeof(req->seqno);

		MPL_req = (struct req_typ *) *sigio_buffer_tos++;
	}

	(*sigio_handlers_[req->type])(req, size + sizeof(req->seqno));

	Tmk_stat.messages++;
	Tmk_stat.bytes += size;

	MPL_rnc_initialize();
}

static	void
MPL_rnc_initialize( void )
{
	struct req_typ *req = MPL_req;

	req->seqno = DONTCARE;

	if (0 > mpc_rcvncall(&req->from, MTU - sizeof(req->seqno), (int *) req, &MPL_type, &MPL_id, MPL_rnc_handler))
		Tmk_MPLerrexit("MPL_rnc_initialize<mpc_rcvncall>");
}
#endif

/*
 * Used by duplicate REQ_COND_BROADCAST, REQ_COND_SIGNAL, and REQ_DISTRIBUTE.
 */
static	void
Tmk_ack(
	struct req_typ *req,
	int		size)
{
	while (0 > send(rep_fd_[req->from], (char *)&req->seqno, sizeof(req->seqno), 0))
		Tmk_errno_check("Tmk_ack<send>");
}

/*
 * Used by sigio_handler to vector duplicate requests.
 */
static
void  (*const sigio_duplicate_handlers_[])() = {
	Tmk_barrier_sigio_duplicate_handler,
	Tmk_barrier_sigio_duplicate_handler,
	Tmk_ack,
	Tmk_ack,
	Tmk_cond_wait_sigio_duplicate_handler,
	Tmk_ack,
	Tmk_diff_sigio_handler,
	Tmk_ack,
	Tmk_exit_sigio_handler,
	Tmk_sched_sigio_duplicate_handler,
	Tmk_sched_sigio_duplicate_handler,
	Tmk_lock_sigio_duplicate_handler,
	Tmk_page_sigio_handler,
	Tmk_repo_sigio_duplicate_handler
};

/*
 * Passed by sigio_handler to select.
 */
static	struct	timeval	timeout = { 0, 0 };

/*
 * Called by SIGIO, Tmk_barrier, and Tmk_sched_*.
 */
void
sigio_handler(
	int	sig)
{
#if ! defined(MPL)
	fd_set	readfds;
	int	er, fd, i, size;

	/*
	 * While select is non-zero, ...
	 */
 redo:
	readfds = rep_fds;

	if ((er = select(rep_maxfdp1, (fd_set_t)&readfds, NULL, NULL, &timeout)) == 0)
		return;
	else if (er < 0)
		Tmk_perrexit("sigio_handler<select>");

	for (i = 0; i < Tmk_nprocs; i++) {
		if (i == Tmk_proc_id)
			continue;

		fd = rep_fd_[i];

		if (FD_ISSET(fd, &readfds)) {

			struct req_typ *req = (struct req_typ *) *sigio_buffer_tos;
			unsigned	seqno, type;

			if ((size = recv(fd, (char *) req, MTU, 0)) < 0)
				Tmk_perrexit("sigio_handler<recv>");

			seqno = req->seqno;

			if (rep_seqno_[req->from] < seqno) {

				rep_seqno_[req->from] = seqno;

				type = req->type;

				if ((1 << type) & SIGIO_BUFFER_HOLD) {

					req_entry_t    *entry = &req_from_[req->from];

					entry->req  = (struct req_syn *) req;
					entry->size = size;

					sigio_buffer_tos++;
				}

				(*sigio_handlers_[type])(req, size);
			}
			else
				(*sigio_duplicate_handlers_[req->type])(req, size);

			Tmk_stat.messages++;
			Tmk_stat.bytes += size;

			if (--er == 0)
				goto redo;
		}
	}
	if (Tmk_debug)
		Tmk_err("sigio_handler: er == %d\n", er);
#else
	MPL_rnc_handler(&MPL_id);
#endif
}

#if defined(PTHREADS)
/*
 *
 */
static	void
sigio_handler_pthreads(int sig)
{
	pthread_mutex_lock(&Tmk_sigio_lock);

	sigio_handler(sig);

	pthread_mutex_unlock(&Tmk_sigio_lock);
}
#endif

/*
 *
 */
void
Tmk_sigio_initialize( void )
{
	int	i;
#if ! defined(MPL)
	struct	sigaction sa;
#endif
	/*
	 * Initializes the free buffer stack.  The top of stack is
	 * statically initialized.
	 */
	for (i = 0; i < NPROCS; i++)
		sigio_buffer_stack_[i] = sigio_buffer_[i];

#if ! defined(MPL)
	/*
	 * Installs the SIGIO handler.
	 */
#if defined(PTHREADS)
#if defined(__linux)
	sa.sa_handler = (__sighandler_t) sigio_handler_pthreads;
#else
	sa.sa_handler = sigio_handler_pthreads;
#endif
#else
#if defined(__linux)
	sa.sa_handler = (__sighandler_t) sigio_handler;
#else
	sa.sa_handler = sigio_handler;
#endif
#endif
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
#if defined(__hpux) || (defined(__sun) && ! defined(__SVR4))
	sa.sa_flags = 0;
#else
	sa.sa_flags = SA_RESTART;
#endif
	if (0 > sigaction(SIGIO, &sa, NULL))
		Tmk_perrexit("Tmk_sigio_initialize<sigaction>");
#else
	/*
	 * Installs the RNC handler.
	 */
	MPL_req = (struct req_typ *) *sigio_buffer_tos++;

	MPL_rnc_initialize();
#endif
}
