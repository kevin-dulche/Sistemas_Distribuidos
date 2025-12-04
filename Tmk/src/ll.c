/*****************************************************************************
 *                                                                           *
 *  Copyright (c) 1991-1995						     *
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
 * File:		ll.c
 * Description:    
 *	IBM LoadLeveler support
 *
 * External Functions:
 *
 * External Variables:
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *
 *	10-Jun-1995	Rob Fowler	Created
 *
 *	Version 0.9.5
 *
 *****************************************************************************/
#include "Tmk.h"

#include <llapi.h>

/*
 * Tmk_ll_error
 *
 * This code is taken from the example in the LoadL manual.  It prints error
 * messages from the ll_get_hostlist and the ll_start_host API's.
 */
void	Tmk_ll_errexit(int code)
{
	switch (code) {
	case PARALLEL_CANT_FORK:
		Tmk_errexit("Remote host can't fork new process.\n");
	case PARALLEL_BAD_ENVIRONMENT:
		Tmk_errexit("Cannot get ll job id from environment.\n");
	case PARALLEL_CANT_GET_HOSTNAME:
		Tmk_errexit("Cannot get my hostname.\n");
	case PARALLEL_CANT_RESOLVE_HOST:
		Tmk_errexit("Nameserve can't resolve host.\n");
	case PARALLEL_CANT_MAKE_SOCKET:
		Tmk_errexit("Can't make socket.\n");
	case PARALLEL_CANT_CONNECT:
		Tmk_errexit("Cannot connect to host.\n");
	case PARALLEL_CANT_PASS_SOCKET:
		Tmk_errexit("Cannot send PASS_OPEN_SOCKET command to remote startd.\n");
	case PARALLEL_CANT_GET_HOSTLIST:
		Tmk_errexit("Cannot get hostlist.\n");
	case PARALLEL_CANT_START_CMD:
		Tmk_errexit("Cannot start command.\n");
	default:
		Tmk_errexit("Unknown error code: %d\n", code);
	}
} 
