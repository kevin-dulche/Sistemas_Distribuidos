/*****************************************************************************
 *                                                                           *
 *  Copyright (c) 1991-1994                                                  *
 *  by TreadMarks, L.L.C. (TREADMARKS), Houston, Texas	  		     *
 *                                                                           *
 *  This software is furnished under a license and may be used and  copied   *
 *  only  in  accordance  with  the  terms  of  such  license and with the   *
 *  inclusion of the above copyright notice.  This software or  any  other   *
 *  copies  thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to or ownership of  the  software  is  hereby    *
 *  transferred.                                                             *
 *									     *
 *  The recipient of this software (RECIPIENT) acknowledges and agrees that  *
 *  the software contains information and trade secrets that are	     *
 *  confidential and proprietary to TREADMARKS.  RECIPIENT agrees to take    *
 *  all reasonable steps to safeguard the software, and to prevent its       *
 *  disclosure.								     * 
 *                                                                           *
 *  The information in this software is subject to change  without  notice   *
 *  and should  not  be  construed  as  a commitment by TREADMARKS.	     *
 *                                                                           *
 *  This software is furnished AS IS, without warranty of any kind, either   *
 *  express or implied (including, but not limited to, any implied warranty  *
 *  of merchantability or fitness), with regard to the software.  	     *
 *  TREADMARKS assumes no responsibility for the use or reliability of its   *
 *  software.  TREADMARKS shall not be liable for any special, incidental,   *
 *  or consequential damages, or any damages whatsoever due to causes beyond *
 *  the reasonable control of TREADMARKS, loss of use, data or profits, or   *
 *  from loss or destruction of materials provided to TREADMARKS by	     *
 *  RECIPIENT.								     *
 *									     *
 *  TREADMARKS's liability for damages arising out of or in connection with  *
 *  the use or performance of this software, whether in an action of	     *
 *  contract or tort including negligence, shall be limited to the purchase  *
 *  price, or the total amount paid by RECIPIENT, whichever is less.	     *
 *                                                                           *
 *****************************************************************************/
/*****************************************************************************
 * File:		open_atm.c
 * Description:    
 *	establishes connections on the Fore ATM network
 *
 * External Functions:
 *			Tmk_connect,
 *			Tmk_accept_initialize,
 *			Tmk_accept
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	21-Jun-1993	Alan L. Cox	Created
 *	 7-Jan-1994	Alan L. Cox	Correct MTU for UDP over ATM
 *	 1-Aug-1994	Alan L. Cox	Dynamic SAP allocation
 *
 *	Version 0.9.1
 *
 *	10-Dec-1994	Alan L. Cox	Changed to a single seqno counter
 *					 (suggested by Pete Keleher)
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *
 *****************************************************************************/
#include "Tmk.h"

#include <fore_atm/fore_atm_user.h>

#if defined(ultrix)
#define	AAL_TYPE_x	aal_type_4
#else
#define AAL_TYPE_x	aal_type_5
#endif

/*
 * AAL3/4 MTU
 */
unsigned	tmk_MTU = MTU;

unsigned	Tmk_port_[NPROCS][NPROCS];

char		req_fd_[NPROCS];
fd_set		req_fds;
int		req_maxfdp1;
unsigned	req_seqno;

char		rep_fd_[NPROCS];
fd_set		rep_fds;
int		rep_maxfdp1;
unsigned	rep_seqno_[NPROCS];

static	struct	req_typ	req_typ = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_CONNECT };
static	struct	iovec	req_iov[2] = { { (caddr_t)&req_typ, sizeof(req_typ) }, { 0, sizeof(Tmk_port_[0]) } };

static	struct	req_typ	rep_typ;
static	struct	iovec	rep_iov[2] = { { (caddr_t)&rep_typ, sizeof(rep_typ) }, { 0, sizeof(Tmk_port_[0]) } };

static	char   *device_name = "/dev/fa0";

static	Atm_qos	QoS = { { 256, 128 }, { 128, 64 }, { 2, 1 } };

/*
 *
 */
void	Tmk_connect(i)
	int		i;
{
	int		fd;
	Atm_info	info;
	Atm_sap		ssap;
	Atm_endpoint	dst;
	Atm_qos_sel	qos_selected;

	struct	{
		unsigned
			seqno;
	}		rep;

	int		mask = sigblock(sigmask(SIGALRM)|sigmask(SIGIO));

	if ((fd = atm_open(device_name, O_RDWR, &info)) < 0) {
		atm_error("<atm_open>Tmk_connect");
		exit(1);
	}
#if defined(ultrix)
	if (0 > ioctl(fd, ATMIOC_SET_MTU, &tmk_MTU))
		Tmk_perrexit("<ioctl>Tmk_connect");
#endif
	if (0 > atm_bind(fd, 0, &ssap, 0)) {
		atm_error("<atm_bind>Tmk_connect");
		exit(1);
	}
	req_fd_[i] = fd;

	if (0 > atm_gethostbyname(Tmk_hostlist[i], &dst.nsap))
		Tmk_errexit("Tmk_connect: atm_gethostbyname failed.\n");

	dst.asap = Tmk_port_[i][Tmk_proc_id];

	if (0 > atm_connect(fd, &dst, &QoS, &qos_selected, AAL_TYPE_x, duplex)) {
		atm_error("<atm_connect>Tmk_connect");
		exit(1);
	}
	req_typ.seqno = req_seqno += NPROCS;

	req_iov[1].iov_base = (caddr_t) Tmk_port_[Tmk_proc_id];
rexmit:
	if (0 > writev(fd, req_iov, sizeof(req_iov)/sizeof(req_iov[0])))
		Tmk_perrexit("<writev>Tmk_connect");

	Tmk_tout_flag = 0;

	setitimer(ITIMER_REAL, &Tmk_tout, NULL);

	sigsetmask(mask);
 retry:
	if (0 > read(fd, &rep, sizeof(rep)))
		if (Tmk_tout_flag) {

			if (Tmk_debug)
				Tmk_err("<timeout: %d>Tmk_connect: seqno == %d\n", i, req_typ.seqno);

			sigblock(sigmask(SIGALRM)|sigmask(SIGIO));

			goto rexmit;
		}
		else if (errno == EINTR)
			goto retry;
		else
			Tmk_perrexit("<read>Tmk_connect");

	if (req_typ.seqno != rep.seqno) {

		if (Tmk_debug)
			Tmk_err("<bad seqno: %d>Tmk_connect: seqno == %d (received: %d)\n", i, req_typ.seqno, rep.seqno);

		goto retry;
	}
	FD_SET(fd, &req_fds);

	req_maxfdp1 = MAX(fd + 1, req_maxfdp1);
}

/*
 *
 */
void	Tmk_connect_sigio_duplicate_handler(req)
	struct	req_con	       *req;
{
	if (0 > write(rep_fd_[req->from], &req->seqno, sizeof(req->seqno)))
		Tmk_perrexit("<write>Tmk_connect_sigio_duplicate_handler");
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
	int		i;
{
	int		fd;
	Atm_info	info;
	Atm_sap		ssap;

	if ((fd = atm_open(device_name, O_RDWR, &info)) < 0) {
		atm_error("<atm_open>Tmk_accept_initialize");
		exit(1);
	}
#if defined(ultrix)
	if (0 > ioctl(fd, ATMIOC_SET_MTU, &tmk_MTU))
		Tmk_perrexit("<ioctl>Tmk_accept_initialize");
#endif
	if (0 > atm_bind(fd, 0, &ssap, 1)) {
		atm_error("<atm_bind>Tmk_accept_initialize");
		exit(1);
	}
	rep_fd_[i] = fd;

	if (Tmk_debug)
		Tmk_err("SAP[%d][%d] assigned=%d\n", Tmk_proc_id, i, ssap);

	Tmk_port_[Tmk_proc_id][i] = ssap;
}

/*
 *
 */
void	Tmk_accept(i)
	int		i;
{
	int		fd = rep_fd_[i];

	int		conn_id;
	Atm_endpoint	calling;
	Atm_qos		qos;
	Aal_type	aal;

	int		mask = sigblock(sigmask(SIGALRM)|sigmask(SIGIO));

	if (0 > atm_listen(fd, &conn_id, &calling, &qos, &aal)) {
		atm_error("<atm_listen>Tmk_accept");
		exit(1);
	}
	if (Tmk_debug)
		Tmk_err("conn_id=%d aal=%d\n", conn_id, aal);

	if (0 > atm_accept(fd, fd, conn_id, &QoS, duplex)) {
		atm_error("<atm_accept>Tmk_accept");
		exit(1);
	}
	sigsetmask(mask);

	rep_iov[1].iov_base = (caddr_t) Tmk_port_[i];
 retry:
	if (0 > readv(fd, rep_iov, sizeof(rep_iov)/sizeof(rep_iov[0])))
		if (errno == EINTR)
			goto retry;
		else
			Tmk_perrexit("<recvmsg>Tmk_accept");

	rep_seqno_[i] = rep_typ.seqno;

	if (0 > write(fd, &rep_typ.seqno, sizeof(rep_typ.seqno)))
		Tmk_perrexit("<write>Tmk_accept");

	FD_SET(fd, &rep_fds);

	rep_maxfdp1 = MAX(fd + 1, rep_maxfdp1);

	if (0 > fcntl(fd, F_SETOWN, getpid()))
		Tmk_perrexit("<fcntl>Tmk_accept");

	if (0 > fcntl(fd, F_SETFL, FASYNC))
		Tmk_perrexit("<fcntl>Tmk_accept");
}
