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
 * $Id: mmap.c,v 10.4.1.2 1998/05/09 19:27:09 alc Exp $
 *
 * Description:    
 *	page management routines
 *
 * External Functions:
 *			Tmk_mmap,
 *			Tmk_msync
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	 8-Jan-1997	Vivek Pai	Created from a skeleton
 *					 by Alan L. Cox
 *	10-Jan-1997	Alan L. Cox	Support addr == NULL behavior and
 *					 errnop
 *	Version 0.10.2
 */
#include "Tmk.h"

#include <sys/types.h>
#include <sys/mman.h>

#include "Tmk_mman.h"

#define PAGE_ALIGNED(_addr)	( ! ((unsigned long) _addr & \
				     (unsigned long)(Tmk_page_size - 1)))

#define RANGE_WITHIN_BOUNDS(_addr, _len) \
				((_addr >= page_array_[0].vadr) && \
				 (_addr + _len > _addr) && \
				 (_addr + _len <= page_array_[Tmk_npages].vadr))

/*
 *
 */
static
caddr_t
do_mmap(
	caddr_t	addr,
	size_t	len,
	int	prot,
	int	flags,
	int	fd,
	off_t	offset,
	int    *errnop)	/*OUT*/
{
	page_t	page;

	sigset_t
		mask;
    
	sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

	/*
	 * Needs comment.  :-)
	 */
	for (page = &page_array_[(addr - page_array_[0].vadr) >> page_shift];
	     page->vadr < addr + len;
	     page++) {

		if (page->state == private)
			continue;
		else if (page->twin == NULL) {

			if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ|PROT_WRITE))
				Tmk_perrexit("do_mmap<mprotect>");

			switch (page->state) {
			case empty:
				Tmk_page_request(page);
			case invalid:
				page->state = valid;

				Tmk_diff_request(page);
			case valid:
				Tmk_twin_alloc_and_copy(page);
	  
				Tmk_page_dirty_merge(page);

				break;
			default:
				assert((page->state == empty)  ||
				       (page->state == invalid) || 
				       (page->state == valid));
			}
		}
		else
			assert(page->state == valid);
	}
	if (errnop)
		*errnop = 0;

	/*
	 * Assert: munmap only fails if addr isn't page aligned or len < 0.
	 */
	if (0 > munmap(addr, len)) {

		addr = (caddr_t) MAP_FAILED;

		if (errnop)
			*errnop = errno;
	}
	else {
		flags |= MAP_FIXED;

		/*
		 * Assert: mmap won't fail as a consequence of MAP_FIXED.
		 */
		if ((addr = mmap(addr, len, prot, flags, fd, offset)) == (caddr_t) MAP_FAILED) {

			if (errnop)
				*errnop = errno;
		}
	}
	sigio_mutex(SIG_SETMASK, &mask, NULL);

	return addr;
}

/*
 * A limitation of Tmk_mmap is that we don't enforce "prot" access on
 * any processor but the one that performed the Tmk_mmap.  A program
 * may FAIL UNPREDICTABLY if a processor writes to a PROT_READ page. 
 */
caddr_t
Tmk_mmap(
	caddr_t	addr,
	size_t	len,
	int	prot,
	int	flags,
	int	fd,
	off_t	offset,
	int    *errnop)	/*OUT*/
{
	if ((prot & PROT_WRITE) == 0)
		Tmk_err("Warning: Tmk_mmap doesn't (yet) enforce write protection.\n");

	if (flags & MAP_FIXED) {

		/*
		 * If the address is page aligned and the address range is
		 * within the bounds of the shared memory, ...
		 */
		if (PAGE_ALIGNED(addr) && RANGE_WITHIN_BOUNDS(addr, len)) {

			addr = do_mmap(addr, len, prot, flags, fd, offset, errnop);
		}
		else {
			if (errnop)
				*errnop = EINVAL;

			addr = (caddr_t) MAP_FAILED;
		}
	}
	else {
		if (addr)
			Tmk_err("Warning: Tmk_mmap doesn't (yet) use the address hint.\n");

		/*
		 * Assert: Tmk_malloc returns a page aligned address
		 * if the request is for one or more pages.
		 */
		if (addr = Tmk_malloc((len + (Tmk_page_size - 1)) &~ (Tmk_page_size - 1))) {

			addr = do_mmap(addr, len, prot, flags, fd, offset, errnop);
		}
		else {
			if (errnop) {
				/*
				 * It's not evident which error to return.  ENOMEM is
				 * used to indicate that the range isn't available for
				 * mapping, which makes some sense.
				 */
				*errnop = ENOMEM;
			}
			addr = (caddr_t) MAP_FAILED;
		}
	}
	return addr;
}

/*
 * N.B.  The program is responsible for synchronizing the processor
 * performing the msync with any processors that have modified the
 * specified address range.
 */
int
Tmk_msync(
	caddr_t	addr,
	size_t	len,
	int	flags,
	int    *errnop)	/*OUT*/
{
	int	err;

	/*
	 * If the address is page aligned and the address range is
	 * within the bounds of the shared memory, ...
	 */
	if (PAGE_ALIGNED(addr) && RANGE_WITHIN_BOUNDS(addr, len)) {

		caddr_t	cur_addr;

		sigset_t
			mask;
    
		sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

		/*
		 * Touch each page to trigger an update.
		 */
		for (cur_addr = addr; cur_addr < addr + len; cur_addr += Tmk_page_size)
			*(volatile char *) cur_addr;

#if	defined(__bsdi)
		if (flags)
			Tmk_err("Warning: BSD/OS doesn't (yet) support flags on (Tmk_)msync.\n");

		if ((err = msync(addr, len)) < 0) {
#else
		if ((err = msync(addr, len, flags)) < 0) {
#endif
			if (errnop)
				*errnop = errno;
		}
		sigio_mutex(SIG_SETMASK, &mask, NULL);
	}
	else {
		/*
		 * Simulate the behavior of msync, which uses EINVAL
		 * to indicate that the address range is invalid.
		 */
		if (errnop)
			*errnop = EINVAL;

		err = -1;
	}
	return err;
}
