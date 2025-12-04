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
 * $Id: segv.c,v 10.10.1.19 1998/07/14 05:53:53 alc Exp $
 *
 * Description:    
 *	page fault handler
 *
 * External Functions:
 *			Tmk_segv_initialize,
 *			Tmk_twin_alloc_and_copy,
 *			Tmk_twin_free
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox	Created
 *	 5-Jul-1993	Alan L. Cox	Adapted for SPARC
 *	26-Oct-1993	Alan L. Cox	Made segv handler non-reentrant
 *	17-Nov-1993	Alan L. Cox	Adapted for RS/6000
 *      21-Apr-1994     Povl T. Koch    Adapted for Alpha
 *	18-Jul-1994	Sandhya Dwarkadas
 *					Added Tmk_npages
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	Version 0.9.2
 *
 *	10-Apr-1995	Cristiana Amza	Adapted for HPPA/HPUX
 *	21-May-1995	Alan L. Cox	Adapted for SGI/IRIX
 *
 *	Version 0.9.3
 *
 *	 6-Jul-1995	Alan L. Cox	Optimized page copy for HPPA and SPARC
 *					 (others still use bcopy)
 *	15-Jul-1995	Alan L. Cox	Corrected the read versus write
 *					 differentiating code for HPPA
 *	Version 0.9.4
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
 *					Adapted for Linux 1.2.13/Intel x86
 *	Version 0.10.1
 */
#include "Tmk.h"

// ANTONIO LOPEZ JAIMES
#ifndef sigcontext_struct
   /* Kernel headers before 2.1.1 define a struct sigcontext_struct, but
      we need sigcontext.  Some packages have come to rely on
      sigcontext_struct being defined on 32-bit x86, so define this for
      their benefit.  */
   #define sigcontext_struct sigcontext
#endif

//#define __i386 // ANTONIO LOPEZ JAIMES lo tuvo que incluir porque no supo por qué le makefile no lo ponía

#if	defined(_AIX)
#include <sys/machine.h>
#elif	defined(__bsdi) || defined(__FreeBSD__)
#include <vm/vm.h>
#include <vm/pmap.h>
#elif	defined(__linux)
//#include <asm/sigcontext.h> /* COMENTADO POR Antonio LJ */
#elif	defined(__sgi)
#include <sys/sbd.h>
#elif	defined(__sun) && defined(__SVR4)	/* both i386 and sparc */
#include <ucontext.h>
#if	defined(__i386)
#include <sys/trap.h>
#endif
#endif

#if	defined(__FreeBSD__)
#	define	SIGBUS_or_SEGV	SIGBUS
#else
#	define	SIGBUS_or_SEGV	SIGSEGV
#endif

#if   ! defined(__GNUC__)
#	define	__inline__
#endif

#if	defined(__i386)
#	define	p5	/* Capitalize the "p" only if you're using a Pentium (MMX). */
#	define	p6_PRO	/* Capitalize the "p" only if you're using a Pentium Pro. */
#	define	P6_MMX	/* Capitalize the "p" only if you're using a Pentium II. */
#endif

/*
 * An optimized page copy function for the HP PA, Pentium, Pentium Pro,
 * and SPARC.  The Pentium version requires gcc.
 */
#if	defined(__hppa) || defined(__sparc)
__inline__
static	void	page_copy(caddr_t twin, caddr_t vadr)
{
	long long *twin_    = (long long *) twin;
	long long *vadr_    = (long long *) vadr;
	long long *vadr_end = (long long *)(vadr + Tmk_page_size);

	do {
		long long value0 = vadr_[0];
		long long value1 = vadr_[1];
		long long value2 = vadr_[2];
		long long value3 = vadr_[3];
		long long value4 = vadr_[4];
		long long value5 = vadr_[5];
		long long value6 = vadr_[6];
		long long value7 = vadr_[7];

		vadr_ += 8;

		twin_[0] = value0;
		twin_[1] = value1;
		twin_[2] = value2;
		twin_[3] = value3;
		twin_[4] = value4;
		twin_[5] = value5;
		twin_[6] = value6;
		twin_[7] = value7;

		twin_ += 8;

	} while (vadr_ != vadr_end);
}
#elif	defined(__i386)
#if	defined(P6_MMX)
#	define	page_copy(twin, vadr)	memcpy((twin), (vadr), Tmk_page_size)
#else
__inline__
static	void	page_copy(caddr_t twin, caddr_t vadr)
#if	defined(P5) && defined(__GNUC__)
{
	struct	{
		char	state[108];
	}	fpu;

	asm volatile ("fsave %0" : "=m" (fpu));
	{
		double *twin_    = (double *) twin;
		double *vadr_    = (double *) vadr;
		double *vadr_end = (double *)(vadr + Tmk_page_size);
		volatile
		int    *tadr     = (int *) vadr;

		/*
		 * Preload the page into the cache to take advantage
		 * of page mode access.
		 */
		do
			*tadr;
		while ((tadr += 32/sizeof(*tadr)) != (int *) vadr_end);

		do {
			asm volatile ("fildq %0" : : "m" (vadr_[0]));
			asm volatile ("fildq %0" : : "m" (vadr_[1]));
			asm volatile ("fildq %0" : : "m" (vadr_[2]));
			asm volatile ("fildq %0" : : "m" (vadr_[3]));
			asm volatile ("fildq %0" : : "m" (vadr_[4]));
			asm volatile ("fildq %0" : : "m" (vadr_[5]));
			asm volatile ("fildq %0" : : "m" (vadr_[6]));
			asm volatile ("fildq %0" : : "m" (vadr_[7]));

			vadr_ += 8;

			asm volatile ("fistpq %0" : "=m" (twin_[7]));
			asm volatile ("fistpq %0" : "=m" (twin_[6]));
			asm volatile ("fistpq %0" : "=m" (twin_[5]));
			asm volatile ("fistpq %0" : "=m" (twin_[4]));
			asm volatile ("fistpq %0" : "=m" (twin_[3]));
			asm volatile ("fistpq %0" : "=m" (twin_[2]));
			asm volatile ("fistpq %0" : "=m" (twin_[1]));
			asm volatile ("fistpq %0" : "=m" (twin_[0]));

			twin_ += 8;

		} while (vadr_ != vadr_end);
	}
	asm volatile ("frstor %0" : : "m" (fpu));
}
#else
{
	int    *twin_    = (int *) twin;
	int    *vadr_    = (int *) vadr;
	int    *vadr_end = (int *)(vadr + Tmk_page_size);
	volatile
	int    *tadr     = (int *) vadr;

	/*
	 * Preload the page into the cache to take advantage
	 * of page mode access.
	 */
	do
		*tadr;
	while ((tadr += 32/sizeof(*tadr)) != vadr_end);

#if	defined(P6_PRO)
	do {
		int	value0 = vadr_[0];
		int	value1 = vadr_[1];
		int	value2 = vadr_[2];
		int	value3 = vadr_[3];

		vadr_ += 4;

		twin_[0] = value0;
		twin_[1] = value1;
		twin_[2] = value2;
		twin_[3] = value3;
		twin_ += 4;

	} while (vadr_ != vadr_end);
#else
	memcpy(twin_, vadr_, Tmk_page_size);
#endif
}
#endif
#endif
#else
#	define	page_copy(twin, vadr)	memcpy((twin), (vadr), Tmk_page_size)
#endif

/*
 * The twin free list is managed in LIFO order, like a stack.  Hopefully,
 * this results in decent temporal locality.
 */
static
caddr_t	twin_free_list;

/*
 *
 */
void
Tmk_twin_free(
	page_t		page)
{
	caddr_t	       *twin = (caddr_t *) page->twin;

	page->twin = 0;

	*twin = twin_free_list;

	twin_free_list = (caddr_t) twin;
}

/*
 * Should insure that the twin and the original don't collide in the cache.
 *
 * N.B.  We never fault inside malloc.  Otherwise, this code wouldn't work.
 *
 * Called by segv_handler and Tmk_mmap.
 */
void
Tmk_twin_alloc_and_copy(
	page_t		page)
{
	caddr_t		twin = twin_free_list;

	if (twin)
		twin_free_list = *(caddr_t *) twin;
	else {
		if ((twin = malloc(Tmk_page_size)) == NULL)
			Tmk_errexit("<malloc>twin_alloc: out of memory/swap\n");

		Tmk_stat.twins++;
	}
	page->twin = twin;
#if	defined(PTHREADS)
	page_copy(twin, page->v_alias);
#else
	page_copy(twin, page->vadr);
#endif
}     

#if	defined(__sun) && defined(__SVR4)	/* both i386 and sparc */
static	void	segv_handler(sig, sip, uap)
	int		     sig;
	siginfo_t	    *sip;
	ucontext_t	    *uap;
#elif	defined(__i386) && defined(__linux)
static	void	segv_handler(sig, scs)
	int		     sig;
	struct sigcontext_struct scs; // ANTONIO LOPEZ JAIME cambio de sigcontext_struct a sigcontext
#else
static	void	segv_handler(sig, code, scp, vaddr)
	int		     sig;
	int		     code;
	struct	sigcontext  *scp;
	char		    *vaddr;
#endif
{
   

   
#if defined(__alpha)
   #if defined(__linux) || defined(__osf__)
      unsigned i = (scp->sc_traparg_a0 - (long) page_array_[0].vadr) >> page_shift;
   #else
      #error in segv_handler: incomplete port under defined(__alpha)
   #endif
#elif defined(__hppa)
   unsigned i = ((caddr_t) scp->sc_sl.sl_ss.ss_cr21 - page_array_[0].vadr) >> page_shift;
#elif defined(__i386)
   #if defined(__bsdi)
      unsigned i = ((caddr_t) scp->sc_ap - page_array_[0].vadr) >> page_shift;	/* non-standard: kernel modification */
   #elif defined(__FreeBSD__)
      unsigned i = (vaddr - page_array_[0].vadr) >> page_shift;
   #elif defined(__linux)
      unsigned i = ((caddr_t) scs.cr2 - page_array_[0].vadr) >> page_shift;
   #elif defined(__sun) && defined(__SVR4)
      unsigned i = ((caddr_t) sip->si_addr - page_array_[0].vadr) >> page_shift;
   #else
      #error in segv_handler: incomplete port under defined(__i386)
   #endif
#elif defined(_IBMR2)
	unsigned i = (scp->sc_jmpbuf.jmp_context.o_vaddr - (ulong_t) page_array_[0].vadr) >> page_shift;
#elif	defined(__mips)
	unsigned i = ((caddr_t) scp->sc_badvaddr - page_array_[0].vadr) >> page_shift;
#elif	defined(__sparc)
   #if	defined(__sun) && defined(__SVR4)
      unsigned i = ((caddr_t) sip->si_addr - page_array_[0].vadr) >> page_shift;
   #else
      unsigned i = (vaddr - page_array_[0].vadr) >> page_shift;
   #endif
#else
   #error in segv_handler: incomplete port
#endif
      
if (i < Tmk_npages) {
   page_t	page = &page_array_[i];
      
#if defined(MPL)
		sigset_t	mask;

		sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);
#endif
#if defined(PTHREADS)
		pthread_mutex_lock(&Tmk_monitor_lock);

		pthread_mutex_lock(&Tmk_sigio_lock);
#else
		if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ|PROT_WRITE))
			Tmk_perrexit("<mprotect>segv_handler");
#endif
		if (page->state == valid) {
#if defined(PTHREADS)
			/*
			 * Two or more threads might write page fault
			 * on the page (nearly) simultaneously.  If the twin
			 * exists, ignore the page fault.
			 */
			if (page->twin)
				goto unlock_and_return;

			/*
			 * If a write access caused the page fault, ...
			 */
#if	defined(__alpha)
#if	defined(__linux) || defined(__osf__)
			else if (*(unsigned *) scp->sc_pc & (1 << 28)) {
#else
#error in segv_handler: incomplete port under defined(__alpha)
#endif
#elif	defined(__hppa)
			else if (((scp->sc_sl.sl_ss.ss_cr19 & 3758096384) == 1610612736) ||
				 ((scp->sc_sl.sl_ss.ss_cr19 & 512) == 512)) {
#elif	defined(__i386)
#if	defined(__bsdi)
			else if (0) {	/* bsdi: XXX */
#elif	defined(__FreeBSD__)
			else if (code & (PGEX_W << 5)) {	/* non-standard: kernel modification */
#elif	defined(__linux) 
			else if (scs.err & 2) {
#elif	defined(__sun) && defined(__SVR4)
			else if (uap->uc_mcontext.gregs[ERR] & PF_ERR_WRITE) {
#else
#error in segv_handler: incomplete port under defined(__i386)
#endif
#elif	defined(_IBMR2)
			else if (scp->sc_jmpbuf.jmp_context.except[1] & DSISR_ST) {
#elif	defined(__mips)
			else if (scp->sc_cause & EXC_CODE(1)) {
#elif	defined(__sparc)
#if	defined(__sun) && defined(__SVR4)
			else if (*(unsigned *) uap->uc_mcontext.gregs[REG_PC] & (1 << 21)) {
#else
			else if (*(unsigned *) scp->sc_pc & (1 << 21)) {
#endif
#else
#error in segv_handler: incomplete port
#endif
#endif
	twin:
			Tmk_twin_alloc_and_copy(page);

			Tmk_page_dirty_merge(page);
#if	defined(PTHREADS)
				if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ|PROT_WRITE))
					Tmk_perrexit("segv_handler<mprotect>");
			}
			/*
			 * else ignore the page fault.
			 */
#endif
		}
		else {
			if (Tmk_debug)
				Tmk_err("page: %d ", i);

			if (page->state == empty)
				Tmk_page_request(page);

			page->state = valid;

			Tmk_diff_request(page);

			/*
			 * If a write access caused the page fault, ...
			 */
#if	defined(__alpha)
#if	defined(__linux) || defined(__osf__)
			if (*(unsigned *) scp->sc_pc & (1 << 28))
#else
#error in segv_handler: incomplete port under defined(__alpha)
#endif
#elif	defined(__hppa)
			if (((scp->sc_sl.sl_ss.ss_cr19 & 3758096384) == 1610612736) ||
			    ((scp->sc_sl.sl_ss.ss_cr19 & 512) == 512))
#elif	defined(__i386)
#if	defined(__bsdi)
			if (0)	/* bsdi: XXX */
#elif	defined(__FreeBSD__)
			if (code & (PGEX_W << 5))	/* non-standard: kernel modification */
#elif	defined(__linux) 
			if (scs.err & 2)
#elif	defined(__sun) && defined(__SVR4)
			if (uap->uc_mcontext.gregs[ERR] & PF_ERR_WRITE)
#else
#error in segv_handler: incomplete port under defined(__i386)
#endif
#elif	defined(_IBMR2)
			if (scp->sc_jmpbuf.jmp_context.except[1] & DSISR_ST)
#elif	defined(__mips)
			if (scp->sc_cause & EXC_CODE(1))
#elif	defined(__sparc)
#if	defined(__sun) && defined(__SVR4)
			if (*(unsigned *) uap->uc_mcontext.gregs[REG_PC] & (1 << 21))
#else
			if (*(unsigned *) scp->sc_pc & (1 << 21))
#endif
#else
#error in segv_handler: incomplete port
#endif
				goto twin;
			else
				if (0 > mprotect(page->vadr, Tmk_page_size, PROT_READ))
					Tmk_perrexit("<mprotect>segv_handler");
		}
#if defined(PTHREADS)
	unlock_and_return:
		pthread_mutex_unlock(&Tmk_sigio_lock);

		pthread_mutex_unlock(&Tmk_monitor_lock);
#endif
#if defined(MPL)
		sigio_mutex(SIG_UNBLOCK, &mask, NULL);
#endif
	}
	else {
		struct	sigaction sa;

		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;

		sigemptyset(&sa.sa_mask);

		sigaction(SIGBUS_or_SEGV, &sa, NULL);

		/*
		 * To obtain a complete core file, TreadMarks must unprotect all of shared memory.
		 */
		if (0 > mprotect(page_array_[0].vadr, Tmk_npages*Tmk_page_size, PROT_READ))
			Tmk_perrexit("<mprotect>segv_handler");
	}
}

/*
 *
 */
void	Tmk_segv_initialize( void )
{
	struct	sigaction sa;

#if	defined(__sun) && defined(__SVR4)	/* both i386 and sparc */
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = (void (*)(int, siginfo_t *, void *)) segv_handler;
#else
	sa.sa_flags = 0;
	sa.sa_handler = (void (*)(int)) segv_handler;
#endif
	sigemptyset(&sa.sa_mask);
#if ! defined(MPL)
	sigaddset(&sa.sa_mask, SIGALRM);
	sigaddset(&sa.sa_mask, SIGIO);
#endif
	sigaction(SIGBUS_or_SEGV, &sa, NULL);
}
