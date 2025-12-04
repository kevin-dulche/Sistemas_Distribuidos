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
 * $Id: open_mpl.c,v 10.7 1997/03/07 23:59:28 alc Exp $
 *
 * Description:    
 *	Provides initialization stuff for the US CSS version.
 *
 * External Functions:
 *			Tmk_connect_initialize
 *			Tmk_connect_sigio_duplicate_handler
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *
 *	 1-Jul-1995	Rob Fowler	Created
 *
 *	Version 0.9.4
 *
 *	29-Jul-1996	Rob Fowler	Socket emulation version
 *
 *	Version 0.10.2
 */
#include "Tmk.h"

unsigned	Tmk_port_[NPROCS][NPROCS];

/* 
 * Instead of file descriptors, the *_fd_[] arrays contain
 * encodings of the destination and MPL message type to use.
 * req_fd_[j] == j 
 * rep_fd_[j] == j & 128
 * The choice is critical to make lock work.
 */
char            req_fd_[NPROCS];
fd_set          req_fds;
int             req_maxfdp1;
unsigned        req_seqno;

char            rep_fd_[NPROCS];
fd_set          rep_fds;
int             rep_maxfdp1;
unsigned        rep_seqno_[NPROCS];

/*
 *
 */
void	Tmk_connect_initialize( void )
{
	int	i;
    
	req_seqno = Tmk_proc_id;

	for (i = 0; i < Tmk_nprocs; i++) {
		if (i == Tmk_proc_id)
			continue;

		req_fd_[i] = i;
	    /*
	     *	FD_SET(i, &req_fds);
	     */
		req_maxfdp1 = MAX(i + 1, req_maxfdp1);
	}
	/*
	 * Ensure that every processor is prepared to handle a request.
	 */
	if (0 > mpc_sync(ALLGRP))
		Tmk_MPLerrexit("<mpc_sync>Tmk_connect_initialize");
}

/*
 *
 */
void	Tmk_accept_initialize( int i )
{
	rep_fd_[i] = i | 0x80;
    /*
     *	FD_SET(i, &rep_fds);
     */
	rep_maxfdp1 = MAX(i + 1, rep_maxfdp1);
}
