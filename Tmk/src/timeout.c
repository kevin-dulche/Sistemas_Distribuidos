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
 * $Id: timeout.c,v 10.1.1.3 1998/11/04 04:15:53 alc Exp $
 *
 * Description:    
 *	timer routines
 *
 * External Functions:
 *			Tmk_tout_initialize
 *
 * External Variables:
 *			Tmk_tout,
 *			Tmk_tout_flag
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	30-Jul-1994	Alan L. Cox	Created
 *
 *	Version 0.9.1
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigvec with sigaction
 *
 *	Version 0.10
 */
#include "Tmk.h"

struct	itimerval
		Tmk_tout = { { 1, 0 }, { 1, 0 } };

unsigned	Tmk_tout_flag;

/*
 *
 */
static	void
to_alarm(
	int	sig)
{
	Tmk_tout_flag = 1;
}

/*
 *
 */
void
Tmk_tout_initialize()
{
	struct	sigaction sa;

	sa.sa_handler = to_alarm;

	sigemptyset(&sa.sa_mask);
#if defined(__sun) && ! defined(__SVR4)	/* SunOS 4.x */
	sa.sa_flags = SA_INTERRUPT;
#else
	sa.sa_flags = 0;
#endif
	if (0 > sigaction(SIGALRM, &sa, NULL))
		Tmk_perrexit("Tmk_tout_initialize<sigaction>");
#if defined(PTHREADS)
	{
		/*
		 * Leave SIGALRM blocked by default.  Only the thread waiting
		 * to receive a message will have SIGALRM unblocked.
		 */
		sigset_t  mask;

		sigemptyset(&mask);
		sigaddset(&mask, SIGALRM);

		sigprocmask(SIG_BLOCK, &mask, NULL);
	}
#endif
}
