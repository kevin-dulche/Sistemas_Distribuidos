/*****************************************************************************
 *                                                                           *
 *  Copyright (c) 1991-1997						     *
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
 * $Id: Tmk_mman.h,v 10.3.1.3 1997/11/08 20:26:31 alc Exp $
 *
 * Description:    
 *	user include file for mapping files into shared memory
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	 9-Jan-1997	Alan L. Cox	Created
 *
 *	Version 0.10.2
 */
#ifndef	__Tmk_mman_h
#define	__Tmk_mman_h

#if	defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
/*
 * N.B. Any user of this file must include the system header files
 * required by mmap before #include'ing this file.  These are system
 * dependent. On most systems these are:
 *
 * #include <sys/types.h>
 * #include <sys/mman.h>
 *
 * Errno is an "OUT" parameter for thread safety.  If errnop is NULL,
 * Tmk_mmap doesn't return errno.
 */
#if	defined(__cplusplus) || __STDC__ == 1
extern	caddr_t		Tmk_mmap( caddr_t addr, size_t len, int prot, int flags, int fd, off_t off, int *errnop );
extern	int		Tmk_msync( caddr_t addr, size_t len, int flags, int *errnop );
#else
extern	caddr_t		Tmk_mmap();
extern	int		Tmk_msync();
#endif

#if   ! defined(MAP_FAILED)
#	define	MAP_FAILED	((caddr_t) -1)
#endif
#if	defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* __Tmk_mman_h */
