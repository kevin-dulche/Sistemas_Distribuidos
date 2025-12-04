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
 * $Id: distribute.c,v 10.6.1.4.0.3 1998/07/17 04:20:40 alc Exp $
 *
 * Description:    
 *	user initialization routines
 *
 * External Functions:
 *			Tmk_distribute,
 *			Tmk_distribute_sigio_handler,
 *			Tmk_distribute_sigio_duplicate_handler
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	13-Aug-1993	Alan L. Cox	Created
 *	17-Nov-1993	Alan L. Cox	Adapted for RS/6000 (AIX 3.2.5)
 *
 *	Version 0.9.1
 *
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *
 *	Version 0.9.2
 *
 *	21-May-1995	Alan L. Cox	Adapted for SGI/IRIX
 *
 *	Version 0.9.3
 *
 *	10-Nov-1995	Alan L. Cox	Adapted for AIX 4.1 (See the #if's.)
 *
 *	Version 0.9.6
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

/*
 * On the RS/6000 under AIX 3.2.5, the data segment may appear at a different
 * virtual address in each process.  Consequently, we use the offset from the
 * start of the data segment instead of the absolute address.
 */
#if defined(_AIX) && defined(_AIX32) && ! defined(_AIX41)
extern	char	_data[];
#endif

static	struct	req_dis	req_typ = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_DISTRIBUTE };
static	struct	iovec	req_iov[2] = {
	{ (caddr_t)&req_typ, sizeof(req_typ) },
	{                 0, 0 } };
static	struct	msghdr	req_hdr = { 0, 0, req_iov, sizeof(req_iov)/sizeof(req_iov[0]), 0, 0 };

/*
 *
 */
void
Tmk_distribute(
	const void     *ptr,
	int		size)
{
	int		i, rep_seqno;
	sigset_t	mask;

#if defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
	sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

	req_typ.size = req_iov[1].iov_len = size;
#if defined(_AIX) && defined(_AIX32) && ! defined(_AIX41)
	req_typ.ptr  = (caddr_t)((req_iov[1].iov_base = (void *) ptr) - _data);
#else
	req_typ.ptr  = req_iov[1].iov_base = (void *) ptr;
#endif
	for (i = 0; i < Tmk_nprocs; i++) {
		if (i == Tmk_proc_id)
			continue;

		req_typ.seqno = req_seqno += SEQNO_INCR;
	rexmit:
		while (0 > sendmsg(req_fd_[i], &req_hdr, 0))
			Tmk_errno_check("Tmk_distribute<sendmsg>");

		Tmk_tout_flag = 0;

		setitimer(ITIMER_REAL, &Tmk_tout, NULL);

		sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);

		if (Tmk_debug)
			Tmk_err("Tmk_distribute: vadr == %8X (to: %d)\n", ptr, i);
	retry:
		if (0 > recv(req_fd_[i], (char *)&rep_seqno, sizeof(rep_seqno), 0))
			if (Tmk_tout_flag) {

				if (Tmk_debug)
					Tmk_err("<timeout: %d>Tmk_distribute: seqno == %d\n", i, req_typ.seqno);

				sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

				goto rexmit;
			}
			else if (errno == EINTR)
				goto retry;
			else
				Tmk_perrexit("<recv>Tmk_distribute");

		if (rep_seqno != req_typ.seqno) {

			if (Tmk_debug)
				Tmk_err("<bad seqno: %d>Tmk_distribute: seqno == %d (received: %d)\n", i, req_typ.seqno, rep_seqno);

			goto retry;
		}
		sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

		Tmk_stat.messages++;
		Tmk_stat.bytes += sizeof(rep_seqno);
	}
	sigio_mutex(SIG_SETMASK, &mask, NULL);
#if defined(PTHREADS)
	pthread_mutex_unlock(&Tmk_monitor_lock);
#endif
}

/*
 *
 */
void
Tmk_distribute_sigio_handler(
	const struct req_dis *req)
{
#if defined(_AIX) && defined(_AIX32) && ! defined(_AIX41)
	memcpy((int) _data + req->ptr, &req[1], req->size);
#else
	memcpy(              req->ptr, &req[1], req->size);
#endif
	while (0 > send(rep_fd_[req->from], (char *) req, sizeof(req->seqno), 0))
		Tmk_errno_check("Tmk_distribute_sigio_handler<send>");
}

/*
 *
 */
void
Tmk_distribute_initialize( void )
{
	req_typ.from = Tmk_proc_id;
}
