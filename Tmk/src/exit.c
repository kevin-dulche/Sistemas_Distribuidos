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
 * $Id: exit.c,v 10.6.1.12 1998/05/26 05:34:10 alc Exp $
 *
 * Description:    
 *	collect execution statistics and exit
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	10-Sep-1993	Alan L. Cox	Created
 *
 *	Version 0.9.0
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
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

struct	Tmk_stat	Tmk_stat;

unsigned		tmk_stat_flag;

static	int volatile	exit_flag;

static	struct	req_typ	req_typ = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_EXIT };

static	unsigned	rep_seqno;
static	struct	iovec	rep_iov[2] = {
	{ (caddr_t)&rep_seqno, sizeof(rep_seqno) },
	{ (caddr_t)&Tmk_stat,  sizeof(struct Tmk_stat) } };
static	struct	msghdr	rep_hdr = { 0, 0, rep_iov, sizeof(rep_iov)/sizeof(rep_iov[0]), 0, 0 };

static
void	print_stat( int proc_id, struct Tmk_stat *stat )
{
	if (proc_id == Tmk_nprocs)
		Tmk_err("[ Tmk: cumulative statistics\n"
			"\n"
			"  barriers:   %9d\n"
			"  repos:      %9d\n",
			stat->arrivals,
			stat->repos);
	else
		Tmk_err("[ Tmk: %s statistics\n",
			Tmk_hostlist[proc_id]);

	Tmk_err("\n"
		"  acquires:   %9d (acquire messages: %d)\n"
		"  broadcasts: %9d\n"
		"  signals:    %9d\n"
		"  waits:      %9d\n"
		"  pages:      %9d\n"
		"  bytes:      %9d (application data: %d)\n"
		"  messages:   %9d (average size: %d)\n"
		"  rexmits:    %9d\n"
		"  diffs:      %9d (average size: %d)\n"
		"  twins:      %9d ]\n"
		"\n",
		stat->acquires,	stat->messages_for_acquires,
		stat->broadcasts,
		stat->signals,
		stat->waits,
		stat->cold_misses,
		stat->bytes,	stat->bytes_of_data,
		stat->messages,	stat->messages ? (stat->bytes/stat->messages) : 0,
		stat->rexmits,
		stat->diffs,	stat->diffs ? (stat->total_diff_size/stat->diffs) : 0,
		stat->twins);
}

void	Tmk_exit(value)
	int			value;
{
	struct	Tmk_stat	stat;
	sigset_t		mask;

#if defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
	sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

	if (Tmk_proc_id) {

		while (exit_flag == 0)
			sigio_handler();

		while (0 > sendmsg(rep_fd_[0], &rep_hdr, 0))
			Tmk_errno_check("Tmk_exit<sendmsg>");
	}
	else {
		int	i;

		Tmk_stat.messages += Tmk_stat.messages_for_acquires + Tmk_stat.cold_misses;

		if (tmk_stat_flag > 1)
			print_stat(Tmk_proc_id, &Tmk_stat);

		for (i = 1; i < Tmk_nprocs; i++) {

			req_typ.seqno = req_seqno += SEQNO_INCR;
		rexmit:
			while (0 > send(req_fd_[i], (char *)&req_typ, sizeof(req_typ), 0))
				Tmk_errno_check("Tmk_exit<send>");

			Tmk_tout_flag = 0;

			setitimer(ITIMER_REAL, &Tmk_tout, NULL);

			sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);

			if (Tmk_debug)
				Tmk_err("Tmk_exit: %d\n", i);

			rep_iov[1].iov_base = (caddr_t)&stat;
		retry:
			if (0 > recvmsg(req_fd_[i], &rep_hdr, 0))
				if (Tmk_tout_flag) {

					if (Tmk_debug)
						Tmk_err("<timeout>Tmk_exit\n");

					sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

					goto rexmit;
				}
				else if (errno == EINTR)
					goto retry;
				else
					Tmk_perrexit("<recvmsg>Tmk_exit");

			if (rep_seqno != req_typ.seqno) {

				if (Tmk_debug)
					Tmk_err("<bad seqno: %d>Tmk_exit: seqno == %d (received: %d)\n", i, req_typ.seqno, rep_seqno);

				goto retry;
			}
			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

			stat.messages += stat.messages_for_acquires + stat.cold_misses;

			if (tmk_stat_flag > 1)
				print_stat(i, &stat);

			Tmk_stat.acquires		+= stat.acquires;
			Tmk_stat.acquire_time		+= stat.acquire_time;
			Tmk_stat.bytes			+= stat.bytes;
			Tmk_stat.bytes_of_data		+= stat.bytes_of_data;
			Tmk_stat.cold_misses		+= stat.cold_misses;
			Tmk_stat.messages		+= stat.messages;
			Tmk_stat.messages_for_acquires	+= stat.messages_for_acquires;
			Tmk_stat.rexmits		+= stat.rexmits;
			Tmk_stat.diffs			+= stat.diffs;
			Tmk_stat.twins			+= stat.twins;
			Tmk_stat.total_diff_size	+= stat.total_diff_size;
		}
		if (tmk_stat_flag)
			print_stat(Tmk_nprocs, &Tmk_stat);
	}
	exit(value);
}

void
Tmk_exit_sigio_handler(
	const struct req_typ *req)
{
	exit_flag = 1;

	rep_seqno = req->seqno;
}
