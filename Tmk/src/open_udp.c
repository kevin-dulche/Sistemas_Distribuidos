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
 * $Id: open_udp.c,v 10.7.1.5.0.1 1998/07/17 18:37:43 alc Exp $
 *
 * Description:    
 *	establishes connections
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	21-Jun-1993	Alan L. Cox	Created
 *	 7-Jan-1994	Alan L. Cox	Correct MTU for UDP over ATM
 *	14-Jun-1994	Alan L. Cox	Included <arpa/inet.h> for Alpha
 *	 1-Aug-1994	Alan L. Cox	Dynamic port allocation
 *					 (changes provided by Cristiana Amza)
 *	Version 0.9.1
 *
 *	10-Dec-1994	Alan L. Cox	Changed to a single seqno counter
 *					 (suggested by Pete Keleher)
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *	21-Jan-1995	Alan L. Cox	Changed the name from open_ether.c
 *
 *	Version 0.9.2
 *
 *	10-Apr-1995	Cristiana Amza	Added ioctl's for HPPA/HPUX
 *	21-May-1995	Alan L. Cox	Adapted for SGI/IRIX
 *
 *	Version 0.9.3
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

#include <netinet/in.h>

#include <arpa/inet.h>

static	struct	req_typ	req_typ = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_CONNECT };

static	struct	iovec	req_iov[2] = {
	{ (caddr_t)&req_typ, sizeof(req_typ) },
	{                 0, sizeof(Tmk_port_[0]) } };
static	struct	msghdr	req_hdr = { 0, 0, req_iov, sizeof(req_iov)/sizeof(req_iov[0]), 0, 0 };

unsigned	Tmk_port_[NPROCS][NPROCS];

/*
 * Never use sequence numbers 0, 1, ..., NPROCS-1.  Sequence number 0
 * would break sigio_handler.
 */
char		req_fd_[NPROCS];
fd_set		req_fds;
int		req_maxfdp1;
unsigned	req_seqno;

char		rep_fd_[NPROCS];
fd_set		rep_fds;
int		rep_maxfdp1;
unsigned	rep_seqno_[NPROCS];

#if	defined(__sun) && ! defined(__SVR4)
/*
 * Maximum allowed by SunOS 4.1.3
 */
#	define	fd_RCVBUF_size	52428
#elif	defined(__linux)
/*
 * Maximum allowed by Linux 2.0.0
 */
#	define  fd_RCVBUF_size  65535
#else
#	define	fd_RCVBUF_size	2*(MTU + sizeof(struct sockaddr))
#endif

#	define	fd_SNDBUF_size	MTU

/*
 *
 */
static	int	fd_create( void )
{
	int	fd;

	int	optval;
	size_t	optlen = sizeof(optval);

	struct	sockaddr_in addr;

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		Tmk_perrexit("<socket>fd_create");

	if (0 > getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optlen))
		Tmk_perrexit("<getsockopt>fd_create");

	if (optval < fd_RCVBUF_size) {

		optval = fd_RCVBUF_size;

		while (0 > setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof(optval)))
			if ((errno == ENOBUFS) &&
			    (optval == fd_RCVBUF_size)) {

				optval = MTU + sizeof(struct sockaddr);

				Tmk_err("<setsockopt>fd_create: Low buffer space. Increase MAXUSERS or NMBCLUSTERS.\n");
			}
			else
				Tmk_perrexit("<setsockopt>fd_create");
	}
	if (0 > getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&optval, &optlen))
		Tmk_perrexit("<getsockopt>fd_create");

	if (optval < fd_SNDBUF_size) {

		optval = fd_SNDBUF_size;

		if (0 > setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&optval, sizeof(optval)))
			Tmk_perrexit("<setsockopt>fd_create");
	}
	addr.sin_family      = AF_INET;
 	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port        = htons(0);

	if (0 > bind(fd, (struct sockaddr *)&addr, sizeof(addr)))
		Tmk_perrexit("<bind>fd_create");

	return fd;
}

/*
 *
 */
void	Tmk_connect(i)
	int			i;
{
	int			fd = fd_create();

        struct	hostent	       *hp;
	struct	sockaddr_in	addr;
#if defined(PTHREADS)
	sigset_t		mask;
#endif
	unsigned		rep_seqno;

	req_fd_[i] = fd;

	if ((hp = gethostbyname(Tmk_hostlist[i])) == NULL)
		Tmk_herrexit("Tmk_connect: %s", Tmk_hostlist[i]);

	addr.sin_family = AF_INET;

	memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

	addr.sin_port = htons(Tmk_port_[i][Tmk_proc_id]);

        if (0 > connect(fd, (struct sockaddr *)&addr, sizeof(addr)))
		Tmk_perrexit("<connect>Tmk_connect");

	if (Tmk_debug)
		Tmk_err("Tmk_connect address: %s port: %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	req_typ.seqno = req_seqno += NPROCS;

	req_iov[1].iov_base = (caddr_t) Tmk_port_[Tmk_proc_id];
rexmit:
	while (0 > sendmsg(fd, &req_hdr, 0))
		Tmk_errno_check("Tmk_connect<sendmsg>");

	Tmk_tout_flag = 0;

	setitimer(ITIMER_REAL, &Tmk_tout, NULL);
#if defined(PTHREADS)
	sigprocmask(SIG_UNBLOCK, &ALRM_and_IO_mask, &mask);
#endif
 retry:
	if (0 > recv(fd, (char *)&rep_seqno, sizeof(rep_seqno), 0))
		if (Tmk_tout_flag) {

			if (Tmk_debug)
				Tmk_err("<timeout: %d>Tmk_connect: seqno == %d\n", i, req_typ.seqno);
#if defined(PTHREADS)
			sigprocmask(SIG_SETMASK, &mask, NULL);
#endif
			goto rexmit;
		}
		else if (errno == EINTR)
			goto retry;
		else
			Tmk_perrexit("<recv>Tmk_connect");

	if (req_typ.seqno != rep_seqno) {

		if (Tmk_debug)
			Tmk_err("<bad seqno: %d>Tmk_connect: seqno == %d (received: %d)\n", i, req_typ.seqno, rep_seqno);

		goto retry;
	}
#if defined(PTHREADS)
	sigprocmask(SIG_SETMASK, &mask, NULL);
#endif
	FD_SET(fd, &req_fds);

	req_maxfdp1 = MAX(fd + 1, req_maxfdp1);
}

/*
 *
 */
void	Tmk_connect_initialize()
{
	req_typ.from = req_seqno = Tmk_proc_id;
}

/*
 *
 */
void	Tmk_accept_initialize(i)
	int			i;
{
	int			fd = fd_create();

	struct	sockaddr_in	addr;
	size_t			addrlen = sizeof(addr);

	rep_fd_[i] = fd;

	if (0 > getsockname(fd, (struct sockaddr *)&addr, &addrlen))
		Tmk_perrexit("<getsockname>Tmk_create");

	Tmk_port_[Tmk_proc_id][i] = ntohs(addr.sin_port);
}

/*
 * Recvmsg in SGI/IRIX doesn't return the sockaddr_in when a signal occurs.
 */
void	Tmk_accept(i)
	int			i;
{
	int			fd = rep_fd_[i];
#if  defined(sgi)
	sigset_t		mask;
#endif
static	struct	req_typ		rep_typ;
static	struct	iovec		rep_iov[2] = { { (caddr_t)&rep_typ, sizeof(rep_typ) }, { 0, sizeof(Tmk_port_[0]) } };
static	struct	sockaddr_in	rep_addr;
static	struct	msghdr		rep_hdr = { (caddr_t)&rep_addr, sizeof(rep_addr), rep_iov, 2, 0, 0 };

	rep_iov[1].iov_base = (caddr_t) Tmk_port_[i];
#if defined(sgi)
	sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);
#endif
 retry:
	if (0 > recvmsg(fd, &rep_hdr, 0))
		if (errno == EINTR)
			goto retry;
		else
			Tmk_perrexit("<recvmsg>Tmk_accept");
#if defined(sgi)
	sigio_mutex(SIG_SETMASK, &mask, NULL);
#endif
	rep_seqno_[i] = rep_typ.seqno;

	if (0 > connect(fd, (struct sockaddr *)&rep_addr, sizeof(rep_addr)))
		Tmk_perrexit("<connect>Tmk_accept");

	if (Tmk_debug)
		Tmk_err("Tmk_accept address: %s port: %d\n", inet_ntoa(rep_addr.sin_addr), ntohs(rep_addr.sin_port));

	while (0 > send(fd, (char *)&rep_typ.seqno, sizeof(rep_typ.seqno), 0))
		Tmk_errno_check("Tmk_accept<send>");

	FD_SET(fd, &rep_fds);

	rep_maxfdp1 = MAX(fd + 1, rep_maxfdp1);

#if defined(_AIX) || defined(__hpux)
	{
		int	pid = getpid();

		if (0 > ioctl(fd, SIOCSPGRP, &pid))
			Tmk_perrexit("<ioctl>Tmk_accept");
	}
#else
	if (0 > fcntl(fd, F_SETOWN, getpid()))
		Tmk_perrexit("<fcntl>Tmk_accept");
#endif

#if defined(_AIX) || defined(__hpux)
	{
		int	flag = 1;

		if (0 > ioctl(fd, FIOASYNC, &flag))
			Tmk_perrexit("<ioctl>Tmk_accept");
	}
#else
	if (0 > fcntl(fd, F_SETFL, FASYNC))
		Tmk_perrexit("<fcntl>Tmk_accept");
#endif
}
