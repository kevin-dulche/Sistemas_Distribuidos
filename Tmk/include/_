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
 * $Id: Tmk.h,v 10.7.1.7.0.1 1998/07/17 04:16:04 alc Exp $
 *
 * Description:    
 *	user include file
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox	Created
 *	16-Jun-1994	Alan L. Cox	Adapted for Alpha
 *					 (changes provided by Povl T. Koch)
 *	28-Aug-1994	Alan L. Cox	Adapted for C++
 */
#ifndef	__Tmk_h
#define	__Tmk_h

#if ! defined(__GNUC__)
#define __attribute__(dummy)
#endif

/*#define TMK_NPROCS 8*/
#define TMK_NPROCS	32
#if _mips == 4
#define	TMK_NPAGES	8192	/* Tmk_page_size == 16384 */
#else
#define	TMK_NPAGES	16384	/* Tmk_page_size == 4096 or 8192 */
#endif
#define TMK_NBARRIERS	32
#define TMK_NCONDS	4
#define TMK_NLOCKS	1024

struct	Tmk_stat	{
	unsigned	acquires;
	unsigned	acquire_time; /* in microseconds */
	unsigned	arrivals;
	unsigned	broadcasts;
	unsigned	signals;
	unsigned	waits;
	unsigned	bytes;
	unsigned	bytes_of_data;
	unsigned	cold_misses;
	unsigned	messages;
	unsigned	messages_for_acquires;
	unsigned	rexmits;
	unsigned	diffs;
	unsigned	twins;
	unsigned	repos;
	unsigned	total_diff_size; /* in bytes */
};

struct	Tmk_sched_arg	{
	int		sharedLen;
	int		byvalueLen;
	void	       *shared[15];
	union {
		short	short_by_value;
		int	int_by_value;
		long	long_by_value;
		float	float_by_value;
		double	double_by_value;
	}	byvalue[15];
};

typedef
struct	Tmk_sched_arg  *Tmk_sched_arg_t;

struct	Tmk		{
	unsigned	proc_id;
	unsigned	nprocs;
	unsigned	page_size;
	unsigned	npages;
};

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
extern	struct	Tmk	tmk_;	/* accessible thru a Fortran common block */

#define Tmk_proc_id	tmk_.proc_id
#define Tmk_nprocs	tmk_.nprocs

#define Tmk_page_size	tmk_.page_size
#define Tmk_npages	tmk_.npages

extern	struct	Tmk_stat
			Tmk_stat;

#if defined(__cplusplus) || __STDC__ == 1
extern	void		Tmk_startup( int argc, char * const *argv );
extern	void		Tmk_exit( int status ) __attribute__((__noreturn__));
extern	void		Tmk_barrier( unsigned barrier_id );
extern	void		Tmk_lock_acquire( unsigned lock_id );
extern	void		Tmk_lock_release( unsigned lock_id );
extern	void		Tmk_lock_cond_broadcast( unsigned lock_id,
						 unsigned cond_id );
extern	void		Tmk_lock_cond_signal( unsigned lock_id,
					      unsigned cond_id );
extern	void		Tmk_lock_cond_wait( unsigned lock_id,
					    unsigned cond_id );
extern	void		Tmk_distribute( const void *ptr, int size );
extern	void	       *Tmk_malloc( unsigned size );
extern	void		Tmk_free( void *ptr );
extern	void	       *Tmk_sbrk( unsigned incr );
extern	int		Tmk_sched_start( unsigned proc_id );
extern	void		Tmk_sched_fork( void (*func)( Tmk_sched_arg_t arg ),
					Tmk_sched_arg_t arg );
extern	void		Tmk_sched_exit( void );
#else
extern	void		Tmk_startup();
extern	void		Tmk_exit();
extern	void		Tmk_barrier();
extern	void		Tmk_lock_acquire();
extern	void		Tmk_lock_release();
extern	void		Tmk_lock_cond_broadcast();
extern	void		Tmk_lock_cond_signal();
extern	void		Tmk_lock_cond_wait();
extern	void		Tmk_distribute();
extern	void	       *Tmk_malloc();
extern	void		Tmk_free();
extern	void	       *Tmk_sbrk();
extern	int		Tmk_sched_start();
extern	void		Tmk_sched_fork();
extern	void		Tmk_sched_exit();
#endif
#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* __Tmk_h */
