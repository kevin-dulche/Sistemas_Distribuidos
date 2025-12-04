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
 * $Id: page.c,v 10.15.1.9 1998/05/26 05:34:10 alc Exp $
 *
 * Description:    
 *	page management routines
 *
 * External Functions:
 *			Tmk_page_request,
 *			Tmk_page_sigio_handler,
 *			Tmk_page_inval_perform,
 *			Tmk_page_inval_merge,
 *			Tmk_page_dirty_merge,
 *			Tmk_page_initialize
 *
 * External Variables:
 *			Tmk_page_init_to_valid,
 *			Tmk_page_size,
 *			Tmk_npages
 *
 *			page_array_,
 *			page_shift,
 *			page_dirty
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox	Created
 *	22-Jul-1993	Alan L. Cox	Added reliable message protocol
 *	26-Oct-1993	Alan L. Cox	Validated page early to avoid race
 *	17-Nov-1993	Alan L. Cox	Adapted for RS/6000
 *	16-Jun-1994	Alan L. Cox	Adapted for Alpha
 *					 (changes provided by Povl T. Koch)
 *	18-Jul-1994	Sandhya Dwarkadas
 *					Adapted for FORTRAN
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *
 *	Version 0.9.2
 *
 *	 4-Apr-1995	Alan L. Cox	Adapted for SGI/IRIX
 *	10-Apr-1995	Cristiana Amza	Adapted for HPPA/HPUX
 *	29-Apr-1995	Alan L. Cox	Adapted for Solaris
 *
 *	Version	0.9.3
 *
 *	30-Jul-1995	Alan L. Cox	Changed the page sigio handler to send
 *					 the twin when it exists
 *	Version 0.9.4
 *
 *	18-Aug-1995	Alan L. Cox	Changed the page initialization to use
 *					 a single mprotect
 *	Version 0.9.5
 *
 *	16-Nov-1995	Alan L. Cox	Added mprotect merging
 *	29-Nov-1995	Sandhya Dwarkadas
 *					Extended the page array by one entry
 *					 for mprotect merging to work
 *	Version 0.9.6			 on the last real page
 *
 *	15-Jan-1996	Alan L. Cox	Adapted for FreeBSD 2.1/Intel x86
 *
 *	Version 0.9.7
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 *
 *	20-Apr-1996	Robert J. Fowler
 *					Adapted to Linux 1.2.13
 *	Version 0.10.1
 */
#include "Tmk.h"

#if (defined(__sgi) || defined(__sun)) && defined(PTHREADS)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if   ! defined(MAP_ANONYMOUS)
#if	defined(MAP_ANON)
#	define	MAP_ANONYMOUS	MAP_ANON	/* FreeBSD */
#else
#	define	MAP_ANONYMOUS	0	/* sgi, sun */
#endif
#endif

#if   ! defined(MAP_FILE)
#	define	MAP_FILE	0	/* sgi, sun */
#endif

#if   ! defined(MAP_NORESERVE)
#if	defined(MAP_AUTORESRV)
#	define	MAP_NORESERVE	MAP_AUTORESRV	/* sgi */
#else
#	define	MAP_NORESERVE	0	/* AIX, linux, osf */
#endif
#endif

#if   ! defined(MAP_VARIABLE)
#	define	MAP_VARIABLE	0	/* FreeBSD, linux, sgi, sun */
#endif

unsigned	Tmk_page_init_to_valid;

/*
 * Allocate one extra entry at the end of the array for mprotect merging
 * to use as a partner in case the last real page is invalidated. 
 *
 * Warning!  The extra entry also serves as the head of the list used
 * for mprotect merging.
 */
struct	page	page_array_[NPAGES + 1];

/*
 * The default page size on AIX/RS/6K is double the operating system
 * page size, currently, 8K bytes.
 */
#if	defined(_AIX)
#	define	PAGE_SHIFT_DEFAULT	1
#else
#	define	PAGE_SHIFT_DEFAULT	0
#endif

/*
 * Before Tmk_page_initialize, page_shift is the number of bits that the
 * operating system page size (getpagesize()) is left shifted to compute
 * Tmk_page_size.  After Tmk_page_initialize, page_shift is the log2 of
 * Tmk_page_size.  It's used by segv_handler.
 */
unsigned	page_shift = PAGE_SHIFT_DEFAULT;

struct	page	page_dirty;

static	struct	req_typ	req_typ = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_PAGE };

static	unsigned	rep_seqno;
static	struct	iovec	rep_iov[] = {
	{ (caddr_t)&rep_seqno, sizeof(rep_seqno) },
	{                   0, 0 } };
static	struct	msghdr	rep_hdr = { 0, 0, rep_iov, sizeof(rep_iov)/sizeof(rep_iov[0]), 0, 0 };

static	struct	iovec	sig_iov[] = {
	{ 0, sizeof(unsigned) },
	{ 0, 0 } };
static	struct	msghdr	sig_hdr = { 0, 0, sig_iov, sizeof(sig_iov)/sizeof(sig_iov[0]), 0, 0 };

/*
 * The caller must block SIGALRM and SIGIO
 */
void	Tmk_page_request(page)
	page_t		page;
{
	sigset_t	mask;
	int		size;

	req_typ.id = page - page_array_;
	req_typ.seqno = req_seqno += SEQNO_INCR;
rexmit:
	while (0 > send(req_fd_[page->manager], (char *)&req_typ, sizeof(req_typ), 0))
		Tmk_errno_check("Tmk_page_request<send>");

	Tmk_tout_flag = 0;

	setitimer(ITIMER_REAL, &Tmk_tout, NULL);

	sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, &mask);

#if defined(PTHREADS)
	rep_iov[1].iov_base = page->v_alias;
#else
	rep_iov[1].iov_base = page->vadr;
#endif
 retry:
	if ((size = recvmsg(req_fd_[page->manager], &rep_hdr, 0)) < 0)
		if (Tmk_tout_flag) {

			if (Tmk_debug)
				Tmk_err("<timeout: %d>page: seqno == %d\n", page->manager, req_typ.seqno);

			sigio_mutex(SIG_SETMASK, &mask, NULL);

			goto rexmit;
		}
		else if (errno == EINTR)
			goto retry;
		else
			Tmk_perrexit("<recvmsg>page_request");

	if (rep_seqno != req_typ.seqno) {

		if (Tmk_debug)
			Tmk_err("<bad seqno: %d>Tmk_page_request: seqno == %d (received: %d)\n", page->manager, req_typ.seqno, rep_seqno);

		goto retry;
	}
	if (Tmk_debug)
		Tmk_err("size: %d\n", size);

	sigio_mutex(SIG_SETMASK, &mask, NULL);

	Tmk_stat.bytes += size;
	Tmk_stat.bytes_of_data += Tmk_page_size;
	Tmk_stat.cold_misses++;
}

/*
 *
 */
void
Tmk_page_sigio_handler(
	const struct req_typ *req)
{
	page_t		page = &page_array_[req->id];

	sig_iov[0].iov_base = (caddr_t) req;	/* seqno */

	if (page->twin == 0) {
#if defined(PTHREADS)
		sig_iov[1].iov_base = page->v_alias;

		if (page->state == private) {

			page->state = valid;

			if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ))
                                Tmk_perrexit("Tmk_page_sigio_handler<mprotect>");
		}
#else
		sig_iov[1].iov_base = page->vadr;

		if (page->state == invalid)
			if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ))
				Tmk_perrexit("<mprotect>Tmk_page_sigio_handler");
#endif
	}
	else
		sig_iov[1].iov_base = page->twin;

	while (0 > sendmsg(rep_fd_[req->from], &sig_hdr, 0))
		Tmk_errno_check("Tmk_page_sigio_handler<sendmsg>");
#if ! defined(PTHREADS)
	if (page->state == invalid) {

		if (0 > mprotect(page->vadr, Tmk_page_size, 0))
			Tmk_perrexit("<mprotect>Tmk_page_sigio_handler");
	}
	else if (page->state == private) {

		page->state = valid;

		assert(page->twin == 0);

		if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ))
			Tmk_perrexit("<mprotect>Tmk_page_sigio_handler");
	}
#endif
}

#if defined(_AIX)
/*
 * In order to use the list head (page_array_[NPAGES]) as a sentinel,
 * we must sort in descending order.
 */
static
void	merge_sort(page_t curr)
{
	caddr_t	vadr = curr->vadr;

	for (;;) {

		page_t	run1start = curr;

	static	struct	page	dummy;	/* dummy.vadr must == 0 */

		caddr_t	prev__vadr;

		do {
			prev__vadr = vadr;
			curr = curr->next;
			vadr = curr->vadr;
		} while (prev__vadr >= vadr);
	
		if (curr == run1start)
			return;

		(dummy.prev = curr->prev)->next = &dummy;

		while (run1start->vadr >= vadr)
			run1start = run1start->next;

		(curr->prev = run1start->prev)->next = curr;

		for (;;) {

			page_t	run1ptr;

			prev__vadr = vadr;
			curr = curr->next;
			vadr = curr->vadr;

			if (prev__vadr < vadr)
				break;

			run1ptr = run1start;

			while (run1start->vadr >= vadr)
				run1start = run1start->next;

			if (run1start != run1ptr) {
				(run1ptr->prev = curr->prev)->next = run1ptr;
				(curr->prev = run1start->prev)->next = curr;
			}
		}
		if (run1start != &dummy) {
			(run1start->prev = curr->prev)->next = run1start;
			(curr->prev = dummy.prev)->next = curr;
		}
	}
}
#endif

/*
 *  The head of the list used for mprotect merging is the extra page entry.
 */
#define	head	page_array_[NPAGES]

/*
 * Called by Tmk_barrier and Tmk_interval_incorporate.
 */
void	Tmk_page_inval_perform( void )
{
	page_t	end = &head;

	page_t	page = end->prev;

	if (page != end) {
#if defined(_AIX)
		if (end->next != page) {
			/*
			 * Merge sort the invalidate list on AIX.  Performing the mprotects
			 * in order takes advantage of the vm_map hint.
			 */
			merge_sort(end);

			/*
			 * Sorting the list may change end->prev.
			 */
			page = end->prev;
		}
#endif
		end->prev = end->next = end;

		do {
			page_t	partner = page->partner + 1;

			partner->inval_toggle = 0;

			if (0 > mprotect(page->vadr, partner->vadr - page->vadr, 0))
				Tmk_perrexit("<mprotect>Tmk_interval_invalidate");

			page->inval_toggle = 0;

		} while ((page = page->prev) != end);
	}
}

/*
 * Called by Tmk_interval_incorporate.
 */
void	Tmk_page_inval_merge(page1)
	page_t	page1;
{
	page_t	page2 = page1 + 1;

	if (page1->inval_toggle ^= 1) {

		page_t	next = &head;

		page_t	prev = next->prev;

		page1->prev = prev;
		page1->next = next;

		next->prev = prev->next = page1;
	}
	else {
		page1--;
		page1 = page1->partner;
	}
	if ((page2->inval_toggle ^= 1) == 0) {

		page_t	page2__prev = page2->prev;
		page_t	page2__next = page2->next;

		page2__next->prev = page2__prev;
		page2__prev->next = page2__next;

		page2 = page2->partner;
	}
	else
		page2--;

	page1->partner = page2;
	page2->partner = page1;
}

/*
 * Called by segv_handler.
 */
void	Tmk_page_dirty_merge(page1)
	page_t	page1;
{
	page_t	page2 = page1 + 1;

	if (page1->dirty_toggle ^= 1) {

		page_t	next = &page_dirty;

		page_t	prev = next->prev;

		page1->prev = prev;
		page1->next = next;

		next->prev = prev->next = page1;
	}
	else {
		page1--;
		page1 = page1->partner;
	}
	if ((page2->dirty_toggle ^= 1) == 0) {

		page_t	page2__prev = page2->prev;
		page_t	page2__next = page2->next;

		page2__next->prev = page2__prev;
		page2__prev->next = page2__next;

		page2 = page2->partner;
	}
	else
		page2--;

	page1->partner = page2;
	page2->partner = page1;
}

/*
 * Warning!  The shared heap must begin above &end.
 * See Tmk_startup.
 */
void	Tmk_page_initialize()
{
	int	i;

	Tmk_page_size = getpagesize();
	Tmk_page_size <<= page_shift;

	for (i = 1 << page_shift; i < Tmk_page_size; i <<= 1)
		page_shift++;

	/*
	 * Initialize one extra entry but don't allocate a page
	 * underneath it.  Its vadr is used if the last real page
	 * is invalidated.
	 */
	for (i = 0; i <= Tmk_npages; i++) {

		page_t	page = &page_array_[i];

		if (i) {
			page->vadr = page_array_[i - 1].vadr + Tmk_page_size;
#if defined(PTHREADS)
			page->v_alias = page_array_[i - 1].v_alias + Tmk_page_size;
#endif
		}
		else {
			size_t	len  = Tmk_npages*Tmk_page_size;

			caddr_t	addr = page->vadr;

#if defined(PTHREADS) && (defined(__sgi) || defined(__sun))

			int	shmid;

			if ((shmid = shmget(IPC_PRIVATE, len, IPC_CREAT|0600)) < 0)
				Tmk_perrexit("Tmk_page_initialize<shmget>: can't allocate the shared memory");

			if ((page->vadr = shmat(shmid, addr, 0)) == (caddr_t) -1L)
				Tmk_perrexit("Tmk_page_initialize<shmat>: can't map the shared memory");

			if ((page->v_alias = shmat(shmid, page->vadr - 2*len, 0)) == (caddr_t) -1L)
				Tmk_perrexit("Tmk_page_initialize<shmat>: can't create the alias mapping");

			if (0 > shmctl(shmid, IPC_RMID, NULL))
				Tmk_perrexit("Tmk_page_initialize<shmctl>");
#else
			int	prot = PROT_READ|PROT_WRITE;
#if defined(PTHREADS)
			char	name[MAXPATHLEN];
			char   *tmpdir;
			char	c = 0;
			int	flags = MAP_SHARED;
#else
			int	flags = MAP_PRIVATE|MAP_NORESERVE;
#endif
			int	fd;

			if (addr)
				flags |= MAP_FIXED;
			else {

			extern	char	end[];

				addr = (caddr_t)((long)(end + 0x4FFFFFFFL) &~ 0x0FFFFFFFL);

				flags |= MAP_VARIABLE;
			}
#if defined(PTHREADS)
			if ((tmpdir = getenv("SWAPDIR")) == 0) {

				tmpdir = P_tmpdir;

				Tmk_err("Tmk_startup: SWAPDIR=(null), putting swap file in \"%s\".\n", tmpdir);
			}
			sprintf(name, "%s/Tmk_swap.%d", tmpdir, getpid());	/* An extra "/" won't hurt */

			if ((fd = open(name, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)
				Tmk_perrexit("Tmk_page_initialize<open>: open(\"%s\", ... )", name);
			
			if (0 > unlink(name))
				Tmk_perrexit("Tmk_page_initialize<unlink>");

			if ((len - sizeof(c)) != lseek(fd, len - sizeof(c), SEEK_END))
				Tmk_perrexit("Tmk_page_initialize<lseek>");

			if (0 > write(fd, &c, sizeof(c)))
				Tmk_perrexit("Tmk_page_initialize<write>");
#else
			if ((fd = open("/dev/zero", O_RDWR)) == -1)
				flags |= MAP_ANONYMOUS;
			else
				flags |= MAP_FILE;
#endif
			if ((page->vadr = mmap(addr, len, prot, flags, fd, 0)) == (caddr_t) -1L)
				Tmk_perrexit("<mmap>Tmk_page_initialize: can't allocate the shared memory");
#if defined(PTHREADS)
			if ((page->v_alias = mmap(addr+2*len, len, prot, flags&~MAP_FIXED, fd, 0)) == (caddr_t) -1L)
				Tmk_perrexit("Tmk_page_initialize<mmap>: can't allocate the shared memory");
#endif
			if (fd != -1)
				close(fd);
#endif
			if (Tmk_page_init_to_valid) {

				if (Tmk_nprocs > 1)
					if (0 > mprotect(page->vadr, len, PROT_READ))
						Tmk_perrexit("<mprotect>Tmk_page_initialize");
			}
			else {
				if (Tmk_proc_id)
					if (0 > mprotect(page->vadr, len, 0))
						Tmk_perrexit("<mprotect>Tmk_page_initialize");
			}
		}
		if (Tmk_page_init_to_valid)
			page->state = valid;
		else {
			if (Tmk_proc_id == 0)
				page->state = private;
			else
				page->state = empty;
		}
		page->manager = 0;
	}
	head.prev = head.next = &head;

	page_dirty.prev = page_dirty.next = &page_dirty;

	req_typ.from = Tmk_proc_id;

	rep_iov[1].iov_len = sig_iov[1].iov_len = Tmk_page_size;
}
