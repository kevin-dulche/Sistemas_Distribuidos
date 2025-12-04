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
 * $Id: fort_if.c,v 10.4 1997/06/03 06:32:38 alc Exp $
 *
 * Description:    
 *	FORTRAN interface routines
 *
 * External Functions:
 *
 * External Variables:
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	17-Jul-1994	Sandhya Dwarkadas
 *					Created
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	Version 0.9.2
 *
 *	30-Apr-1995	Alan L. Cox	Adapted for Solaris 2.x
 *	 3-Jun-1995	Alan L. Cox	Changed names to lower case
 *	 4-Jun-1995	Alan L. Cox	Adapted for RS6K/AIX
 *
 *	Version 0.9.3
 *
 *	 6-Jul-1995	Alan L. Cox	Adapted for HPPA/HPUX
 *
 *	Version 0.9.4
 *
 *	20-Apr-1996	Robert J. Fowler
 *					Adapted for Linux 1.2.13
 *	Version 0.10.1
 */
#include "Tmk.h"

#if defined(__STDC__)
# if defined(__i386__)	/* both FreeBSD and Linux f2c */
#  define	F77( NAME )	tmk_ ## NAME ## __
# else
#  define	F77( NAME )	tmk_ ## NAME ## _
# endif
#else
# if defined(__i386__)	/* both FreeBSD and Linux f2c */
#  define	F77( NAME )	tmk_/**/NAME/**/__
# else
#  define	F77( NAME )	tmk_/**/NAME/**/_
# endif
#endif

int	iargc_( void );

#define	IARGC()	iargc_()

void	getarg_( int *i, char *s, int slen );

#define	GETARG( I, S )	getarg_(&I, S, sizeof(S))

extern
char	F77(shared_common)[];

/*
 * A FORTRAN callable startup procedure
 *
 * Warning: the method used to locate argv is machine and compiler
 * specific.  It supports a maximum of 128 arguments. 
 */
void	F77(startup)( void )
{
	int		i, c;
        int     	argc = 1 + IARGC();
	char	       *argv[128];
	char		string_[sizeof(argv)/sizeof(argv[0])][192];

	assert(argc < sizeof(argv)/sizeof(argv[0]));

	for (i = 0; i < argc; i++) {

		char   *cp;

		GETARG(i, string_[i]);

		if ((cp = memchr(string_[i], ' ', sizeof(string_[i]))) == 0)
			cp = &string_[i][sizeof(string_[i]) - 1];

		*cp = '\0';

		argv[i] = string_[i];
	}
	argv[i] = NULL;

	page_array_[0].vadr = F77(shared_common);

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		case '?':
			continue;
		default:
			Tmk_errexit("F77(startup)<getopt>\n");
		}
  	Tmk_startup(argc, argv);
}

void	F77(exit)(value_ref)
        int             *value_ref;
{
	Tmk_exit(*value_ref);
}

void	F77(barrier)(barrier_id_ref)
	int    *barrier_id_ref;
{
	Tmk_barrier(*barrier_id_ref);
}

void	F77(lock_acquire)(lock_id_ref)
        unsigned       *lock_id_ref;
{
	Tmk_lock_acquire(*lock_id_ref);
}

void	F77(lock_release)(lock_id_ref)
        unsigned       *lock_id_ref;
{
	Tmk_lock_release(*lock_id_ref);
}

void	F77(lock_cond_broadcast)(lock_id_ref, cond_id_ref)
        unsigned       *lock_id_ref;
        unsigned       *cond_id_ref;
{
	Tmk_lock_cond_broadcast(*lock_id_ref, *cond_id_ref);
}

void	F77(lock_cond_signal)(lock_id_ref, cond_id_ref)
        unsigned       *lock_id_ref;
        unsigned       *cond_id_ref;
{
	Tmk_lock_cond_signal(*lock_id_ref, *cond_id_ref);
}

void	F77(lock_cond_wait)(lock_id_ref, cond_id_ref)
        unsigned       *lock_id_ref;
        unsigned       *cond_id_ref;
{
	Tmk_lock_cond_wait(*lock_id_ref, *cond_id_ref);
}

void	F77(distribute)(data, size_ref)
        char   *data;
        int    *size_ref;
{
	Tmk_distribute(data, *size_ref);
}

char   *F77(malloc)(size_ref)
	unsigned       *size_ref;
{
	Tmk_err("Tmk_malloc: not supported with FORTRAN\n");

	return NULL;
}

void	F77(free)(ptr_ref)
	void	      **ptr_ref;
{
	Tmk_err("Tmk_free: not supported with FORTRAN\n");
}

char   *F77(sbrk)(size_ref)
	unsigned       *size_ref;
{
	Tmk_err("Tmk_sbrk: not supported with FORTRAN\n");

	return NULL;
}

int	F77(sched_start)(proc_id_ref)
	unsigned       *proc_id_ref;
{
	return Tmk_sched_start(*proc_id_ref);
}

void	F77(sched_fork)(func, arg_ref)
	void	(*func)(Tmk_sched_arg_t arg);
	Tmk_sched_arg_t
		       *arg_ref;
{
	Tmk_sched_fork(func, *arg_ref);
}

void	F77(sched_exit)( void )
{
	Tmk_sched_exit();
}
