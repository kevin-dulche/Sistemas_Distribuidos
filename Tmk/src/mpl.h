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
 * $Id: mpl.h,v 10.4.1.2 1998/11/02 23:35:12 alc Exp $
 *
 * Description:    
 *	header file specific to IBM MPL support
 *
 * External Functions:
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *
 *	29-Jul-1996	Rob Fowler	Socket emulation version
 */
#include <mpproto.h>

enum	mpl_msg_type { MPL_UNUSED, MPL_REPLY, MPL_REQ };

/*
 *
 */
typedef	int	Tmk_fd_set;

#undef	fd_set
#define	fd_set	Tmk_fd_set

#undef	FD_ZERO
#define	FD_ZERO(x)	*(x) = 0

#undef	FD_SET
#define	FD_SET(x,y)	*(y) = x

#undef	FD_CLR
#define	FD_CLR(x,y)	*(y) = -1

#undef	FD_ISSET
#define	FD_ISSET(x,y)	((x) == (*(y)))

/*
 *
 */
extern	int	Tmk_send(int, char*, int, int);

extern	int	Tmk_sendmsg(int, struct msghdr*, int); 

extern	int	Tmk_recv(int, char*, int, int);

extern	int	Tmk_recvmsg(int, struct msghdr*, int); 

extern	int	Tmk_select(int, fd_set*, fd_set*, fd_set*, struct timeval*);


#undef	send
#define	send	Tmk_send

#undef	sendmsg
#define	sendmsg	Tmk_sendmsg

#undef	recv
#define	recv	Tmk_recv

#undef	recvmsg
#define	recvmsg	Tmk_recvmsg

#undef	select
#define select	Tmk_select

#undef	setitimer
#define setitimer(which, value, ovalue)
