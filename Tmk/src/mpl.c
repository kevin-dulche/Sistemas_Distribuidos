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
 * $Id: mpl.c,v 10.8 1996/10/12 20:23:06 alc Exp $
 *
 * Description:    
 *	Provides initialization stuff for the US CSS version.
 *
 * External Functions:
 *			Tmk_send,
 *			Tmk_sendmsg,
 *			Tmk_recv,
 *			Tmk_recvmsg,
 *			Tmk_select
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	 1-Jul-1995	Rob Fowler	Created
 *
 *	Version 0.9.3
 *
 *	29-Jul-1996	Rob Fowler	Socket emulation version
 *
 *	Version 0.10.2
 */
#include "Tmk.h"

#include <stdarg.h>

/*
 *
 */
void	Tmk_MPLerrexit(char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr,
		"\n"
		"Exiting with error number (mperrno) 0032-%d\n"
		"  (See IBM AIX Parallel Environment: Installation, Administration, and Diagnosis.)\n",
		mperrno);

	mpc_stopall(-1);
}

#if defined(_AIX32) && ! defined(_AIX41)
/*
 *  This speeds up MPL code by disabling interrupts and polling.
 *  Unfortunately, it uses undocumented internal procedures and variables
 *  in the css library.
 */
int	interrupts;

void	css_disable_interrupts()
{
	interrupts = 0;

	disablercvint();
}

void	css_enable_interrupts()
{
	int	source = DONTCARE;
	int	type = DONTCARE;
	int	mstat;

	interrupts = 1;

	if (0 > mpc_probe(&source, &type, &mstat))
		Tmk_MPLerrexit("<mpc_probe>css_enable_interrupts");
}
#endif

/* 
 *  Instead of file descriptors, the *_fd_[] arrays contain
 *  encodings of the destination and MPL message type to use.
 *  req_fd_[j] == j 
 *  rep_fd_[j] == j & 128
 *  The choice is critical to make lock work.
 */

/*  WARNING!
 *  These calls are specific to Tmk-0.10.1 (and later revisions).
 *  The first 4 bytes (an int) of every message into and out
 *  of these routines is asumed to be a sequence number that
 *  is redundant on top of a reliable message layer.
 *  This code strips off the seqno's on the sending side and
 *  it adds a dummy seqno on the receiving side.
 */
static
void	css_send(int s, char *msg, int len)
{
	int	msg_type;

	if (s & 0x80) {

		s &= ~0x80;

		msg_type = MPL_REPLY;
	}
	else
		msg_type = MPL_REQ;

	if (0 > mpc_bsend(msg, len, s, msg_type))
		Tmk_MPLerrexit("<mpc_bsend>css_send"); 
}

/*  
 *  WARNING!  Do not be tempted to turn the recv into a brecv
 *  and thus get rid of the spinwait.  Using brecv blocks
 *  the rcvncall handler.  This results in deadlocks.
 */ 
static
int	css_recv(int s, char *msg, int len)
{
	int	msg_type = MPL_REPLY;
	int	msg_id;

	if (s & 0x80)
		Tmk_errexit("<Tmk_recv>Called to receive a request.");

	if (0 > mpc_recv(msg, len, &s, &msg_type, &msg_id))
		Tmk_MPLerrexit("<mpc_recv>: node %d", s);

#if defined(_AIX32) && ! defined(_AIX41)
	css_disable_interrupts();
#endif
	while ((len = mpc_status(msg_id)) < 0)
		;
#if defined(_AIX32) && ! defined(_AIX41)
	css_enable_interrupts();
#endif
	return len;
}

int	Tmk_send(int s, char *msg, int len, int flags)
{
	css_send(s, &msg[sizeof(unsigned)], len - sizeof(unsigned));

	return len;
}

int	Tmk_recv(int s, char *msg, int len, int flags)
{
	int	size = css_recv(s, &msg[sizeof(unsigned)], len - sizeof(unsigned));

	*(unsigned *) msg = Tmk_proc_id;

	return size + sizeof(unsigned);
}

int	Tmk_sendmsg(int s, struct msghdr *msg, int flags)
{
	int	iovlen = msg->msg_iovlen;

	if ((iovlen == 2) && (msg->msg_iov[0].iov_len == sizeof(unsigned))) {

		css_send(s, msg->msg_iov[1].iov_base, msg->msg_iov[1].iov_len);

		return msg->msg_iov[1].iov_len + sizeof(unsigned);
	}
	else {
		char	buffer[MTU];
		char   *cur = buffer;

		int	i;

		int	len = msg->msg_iov[0].iov_len - sizeof(unsigned);

		memcpy(cur, msg->msg_iov[0].iov_base + sizeof(unsigned), len);

		cur += len;
	
		for (i = 1; i < iovlen; i++) { 

			len = msg->msg_iov[i].iov_len;

			memcpy(cur, msg->msg_iov[i].iov_base, len);
	  	
			cur += len;
		}
		css_send(s, buffer, cur - buffer);

		return cur - buffer + sizeof(unsigned);
	}
}

/*
 *
 */
int	Tmk_recvmsg(int s, struct msghdr *msg, int flags)
{
	int	size;

	int	iovlen = msg->msg_iovlen;

	if ((iovlen == 2) && (msg->msg_iov[0].iov_len == sizeof(unsigned))) {
	    
		size = css_recv(s, msg->msg_iov[1].iov_base, msg->msg_iov[1].iov_len);
	}
	else {
		char	buffer[MTU];
		char   *cur = buffer;

		int	i;

		int	len;
		int	rem;

		size = msg->msg_iov[0].iov_len - sizeof(unsigned);

		for (i = 1; i < iovlen; i++)
			size += msg->msg_iov[i].iov_len;

		size = css_recv(s, buffer, size);

		len = MIN(size, msg->msg_iov[0].iov_len - sizeof(unsigned));

		memcpy(msg->msg_iov[0].iov_base + sizeof(unsigned), cur, len);

		rem = size - len;

		for (i = 1; rem > 0 && i < iovlen; i++) { 

			cur += len;

			len = MIN(rem, msg->msg_iov[i].iov_len);

			memcpy(msg->msg_iov[i].iov_base, cur, len);

			rem -= len;
		}
	}
	*(unsigned *) msg->msg_iov[0].iov_base = Tmk_proc_id;

	return size + sizeof(unsigned);
}

/* 
 * The code around this has to deal with the weird semantics
 * in a reasonable way.  Under MPL, the only remaining select 
 * in TreadMarks is in lock.c and it can deal with the dumied-up
 * semantics of this operation.  
 */
int	Tmk_select(width, readfds, writefds, exceptfds, timeout)
	int	width;
	fd_set *readfds, *writefds, *exceptfds;
	struct	timeval *timeout;
{
	int	source;
	int	type = MPL_REPLY;
	int	mstat = 0;

#if defined(_AIX32) && ! defined(_AIX41)
	css_disable_interrupts();
#endif
	do {
		source = DONTCARE;

		if (0 > mpc_probe(&source, &type, &mstat))
			Tmk_perrexit("<mpc_probe>Tmk_select");

	} while (mstat < 0);
#if defined(_AIX32) && ! defined(_AIX41)
	css_enable_interrupts();
#endif
	if (readfds)
		FD_SET(source, readfds);

	return 1;
}
