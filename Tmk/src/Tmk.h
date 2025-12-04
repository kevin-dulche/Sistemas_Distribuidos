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
 * $Id: Tmk.h,v 10.27.1.23.0.3 1998/11/02 23:40:07 alc Exp $
 *
 * Description:    
 *	external type, variable and function declarations
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox	Created
 *	17-Nov-1993	Alan L. Cox	Adapted for RS/6000
 *	 7-Jan-1994	Alan L. Cox	Corrected MTU for UDP over ATM
 *	16-Jun-1994	Alan L. Cox	Adapted for Alpha
 *					 (changes provided by Povl T. Koch)
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	10-Dec-1994	Alan L. Cox	Changed to a single seqno counter
 *					 (suggested by Pete Keleher)
 *	Version 0.9.2
 *
 *	 4-Apr-1995	Alan L. Cox	Adapted for SGI/IRIX
 *	10-Apr-1995	Cristiana Amza	Adapted for HPPA/HPUX
 *
 *	Version 0.9.3
 *
 *	14-Jul-1995	Alan L. Cox	Reduced the MTU for HPUX and changed
 *					 the included files for IRIX 6.0
 *	Version 0.9.4
 *
 *	11-Nov-1995	Alan L. Cox	Adapted for AIX 4.1
 *	16-Nov-1995	Alan L. Cox	Added fields to the page structure to
 *					 implement mprotect merging
 *	29-Nov-1995	Sandhya Dwarkadas
 *					Extended the page array by one entry
 *					 for mprotect merging to work
 *	Version 0.9.6			 on the last real page
 *
 *	15-Jan-1996	Alan L. Cox	Adapted for FreeBSD 2.1/Intel x86
 *	21-Jan-1996	Alan L. Cox	Defined getpagesize as sysconf
 *					 for HP-UX and Solaris
 *	Version 0.9.7
 *
 *	27-Jan-1996	Alan L. Cox	Changed to POSIX signal handling
 *
 *	Version 0.10
 *
 *	20-Apr-1996	Robert J. Fowler
 *					Adapted for Linux 1.2.13/Intel x86
 *	Version 0.10.1
 */
#if	defined(PTHREADS)
/*
 * Note: To use PTHREADS under Solaris, use the compiler flag
 * -D_POSIX_C_SOURCE=199506L
 */
#if	defined(__osf__)
#define	_PTHREAD_USE_INLINE
#elif	defined(__sun) && defined(__SVR4)
#define	__EXTENSIONS__
#endif
#include <pthread.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

/*
 * Required before <fcntl.h> which includes <sys/types.h>
 */
#if   ! defined(__linux)
#ifdef	FD_SETSIZE
#undef	FD_SETSIZE
#endif
#define	FD_SETSIZE	128
#endif

#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>	/* for inline memcpy */
#if	defined(__sun) && ! defined(__SVR4)
#include <memory.h>	/* SunOS 4.1.3 defines memcpy and memset in memory.h */
#endif
#include <unistd.h>

/*
 *
 */
#if	defined(_AIX)
#include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>

/*
 * Required before <sys/ioctl.h>.
 */
#if	defined(__sun) && defined(__SVR4)
#	define	BSD_COMP
#endif
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/param.h>

#if   ! defined(MAX)
#	define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#if   ! defined(MIN)
#	define	MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

/*
 * Include the TreadMarks public interface definitions.
 */
#include <Tmk.h>

/*
 * Include emulation code required by some systems.
 */
#if	defined(_AIX) && defined(MPL)
#include "mpl.h"
#elif	defined(__linux)
#include <linux/version.h>
#if	(LINUX_VERSION_CODE < 131072)
#error in Tmk.h: unsupported kernel
#endif
#endif

/*
 *
 */
#if	defined(__hpux)
typedef int    *fd_set_t;
#else
typedef fd_set *fd_set_t;
#endif

/*
 *
 */
#if	defined(__hpux)
#	define	getpagesize()	sysconf(_SC_PAGE_SIZE)	/* defined by unistd.h */
#elif	defined(__sun) && defined(__SVR4)
#	define	getpagesize()	sysconf(_SC_PAGESIZE)
#endif

/*
 * Define the internal synchronization macros.
 */
#if	defined(_AIX) && defined(MPL)
#	define	sigio_mutex(how, set, oset) \
			if (oset == NULL) { \
 \
				int old_dummy; \
 \
				if (how == SIG_BLOCK) \
					mpc_lockrnc(1, &old_dummy); \
				else if (how == SIG_UNBLOCK) \
					mpc_lockrnc(0, &old_dummy); \
				else if (how == SIG_SETMASK) { \
 \
					int	new = *(int *) set; \
 \
					mpc_lockrnc(new, &old_dummy); \
				} else \
					assert(0); \
			} else if (how == SIG_BLOCK) \
				mpc_lockrnc(1, (int *) oset); \
			else if (how == SIG_UNBLOCK) \
				mpc_lockrnc(0, (int *) oset); \
			else if (how == SIG_SETMASK) { \
 \
				int	new = *(int *) set; \
 \
				mpc_lockrnc(new, (int *) oset); \
			} else \
				assert(0)
#elif	defined(PTHREADS)
# if	defined(_AIX)
#	define	pthread_sigmask(how, set, oset) \
			sigthreadmask(how, set, oset)
# endif
#	define	sigio_mutex(how, set, oset) \
			if (how == SIG_BLOCK) { \
				pthread_sigmask(how, set, oset); \
				pthread_mutex_lock(&Tmk_sigio_lock); \
			} else if (how == SIG_UNBLOCK) { \
				pthread_mutex_unlock(&Tmk_sigio_lock); \
				pthread_sigmask(how, set, oset); \
			} else if (how == SIG_SETMASK) { \
				if (sigismember(set, SIGIO)) { \
					pthread_sigmask(how, set, oset); \
					pthread_mutex_lock(&Tmk_sigio_lock); \
				} else { \
					pthread_mutex_unlock(&Tmk_sigio_lock); \
					pthread_sigmask(how, set, oset); \
				} \
			} else \
				assert(0)
#else
#	define	sigio_mutex(how, set, oset) \
			sigprocmask(how, set, oset)
#endif

/*
 * Define the maximum number of scatter/gather entries.
 */
#if	defined(MSG_MAXIOVLEN)
/*
 * The actual value is one less than advertised by most systems.
 */
#	undef	MSG_MAXIOVLEN
# if	defined(_AIX41)
#	define	MSG_MAXIOVLEN	1024
# else
#	define	MSG_MAXIOVLEN	15
# endif

#elif	defined(__bsdi) || defined(__FreeBSD__)
#	define	MSG_MAXIOVLEN	1024
#elif	defined(__linux)
#	define	MSG_MAXIOVLEN	UIO_MAXIOV
#else
#error in Tmk.h: incomplete port
#endif

/*
 * Define the MTU for each system.
 */
#if	defined(__linux) || defined(__sgi) || defined(__sun)
/*
 * Linux (alpha/x86), SGI, and SunOS/Solaris (SPARC/x86)
 *
 * UDP MTU
 */
#	define	MTU	32768

#elif	defined(__bsdi) || defined(__FreeBSD__) || defined(__hpux) || defined(__osf__)
/*
 * BSD/OS, FreeBSD, HPUX and DEC UNIX (__osf__)
 *
 * UDP MTU
 */
#	define	MTU	49152

#elif	defined(_AIX)
/*
 * AIX RS/6000, specifically SP2
 */
#	define	MTU	65492

#else
#error in Tmk.h: incomplete port
#endif

/*
 *
 */
#define NPROCS		TMK_NPROCS
#define	NPAGES		TMK_NPAGES /* Update NBUCKETS (in sh_malloc.c) if NPAGES changes */
#define NBARRIERS	TMK_NBARRIERS
#define	NCONDS		TMK_NCONDS
#define NLOCKS		TMK_NLOCKS

#if	defined(MPL)
#	define	SEQNO_INCR	0
#else
#	define	SEQNO_INCR	NPROCS
#endif

enum	req_typ_type	{ REQ_ARRIVAL, REQ_ARRIVAL_REPO, REQ_COND_BROADCAST, REQ_COND_SIGNAL, REQ_COND_WAIT, REQ_CONNECT, REQ_DIFF, REQ_DISTRIBUTE, REQ_EXIT, REQ_JOIN, REQ_JOIN_REPO, REQ_LOCK, REQ_PAGE, REQ_REPO };

struct	req_typ	{
	unsigned	seqno;
	unsigned char	from;
	unsigned char	type;
	unsigned short	id;
};

struct	req_con	{
	unsigned	seqno;
	unsigned char	from;
	unsigned char	type;
	unsigned short	id;	/* unused */
	unsigned	port_[NPROCS];
};

struct	req_dif	{
	unsigned	seqno;
	unsigned char	from;
	unsigned char	type;
	unsigned short	id;
	unsigned short	data[(MTU - sizeof(struct req_typ))/sizeof(unsigned short)];
};

struct	req_dis	{
	unsigned	seqno;
	unsigned char	from;
	unsigned char	type;
	unsigned short	UnUsed;
	unsigned	size;
	caddr_t		ptr;
};

struct	req_syn	{
	unsigned	seqno;
	unsigned char	from;
	unsigned char	type;
	unsigned short	id;
	unsigned short	vector_time[NPROCS];
};

struct  req_cond        {  
        unsigned        seqno;
        unsigned char   from;
        unsigned char   type;
        unsigned short  id;	/* lock */
        unsigned short  vector_time[NPROCS];
        unsigned short  id2;	/* cond */
};

typedef	struct	req_entry
			req_entry_t;

struct	req_entry	{
	struct req_syn *req;
	unsigned	size;
};

/*
 *
 */
typedef
struct	interval       *interval_t;

typedef
struct	write_notice   *write_notice_t;

struct	write_notice	{
	write_notice_t	next;

	caddr_t		diff;
	unsigned short	diff_size;

	interval_t	interval;
};

typedef
struct	write_notice_range
		       *write_notice_range_t;

struct	write_notice_range {
	write_notice_range_t
			next;
	unsigned short	first_page_id;
	unsigned short	last_page_id;
};

struct	interval	{
	interval_t	prev;
	interval_t	next;

	int		id;

	unsigned short	vector_time_[NPROCS];

	struct	write_notice_range
			head;
};

/*
 *  The four page states:  A page's initial state is either "private"
 *  or "empty."  A page is "private" on its manager, and "empty" everywhere
 *  else.  When the manager receives the first request for a page, it changes
 *  the page's state to "valid."
 */
enum	state	{ valid, invalid, private, empty };

/*
 *  Warning!  Member "next" is used to implement the dirty page ("page_dirty")
 *  list and the invalidate merging list ("Tmk_page_inval_merge").
 */
typedef
struct	page	       *page_t;

struct	page	{
	page_t		prev;
	page_t		next;
	page_t		partner;

	caddr_t		vadr;
#if	defined(PTHREADS)
	caddr_t		v_alias;
#endif
	caddr_t		twin;

	unsigned char	state;
	unsigned char	manager;
	unsigned char	inval_toggle;
	unsigned char	dirty_toggle;

	write_notice_t	write_notice_[NPROCS];
};

/*
 *
 */
extern	char		req_fd_[NPROCS];
extern	fd_set		req_fds;
extern	int		req_maxfdp1;
extern	unsigned	req_seqno;

extern	req_entry_t	req_from_[NPROCS];

extern	char		rep_fd_[NPROCS];
extern	fd_set		rep_fds;
extern	int		rep_maxfdp1;
extern	unsigned	rep_seqno_[NPROCS];

extern	struct	page	page_array_[NPAGES + 1];
extern	unsigned	page_shift;
extern	struct	page	page_dirty;

extern	struct interval proc_array_[NPROCS];
extern	unsigned short	proc_vector_time_[NPROCS];

extern	unsigned short	inverse_time_[NPROCS];

extern	sigset_t	ALRM_and_IO_mask;

extern	unsigned	Tmk_spinmask;

extern	int		Tmk_debug;

extern	unsigned	Tmk_port_[NPROCS][NPROCS];

extern	char		Tmk_hostlist[NPROCS][MAXHOSTNAMELEN];

extern struct itimerval	Tmk_tout;

extern	unsigned	Tmk_tout_flag;

extern	unsigned	tmk_stat_flag;

extern	unsigned	Tmk_page_init_to_valid;

#if	defined(PTHREADS)

extern  pthread_mutex_t Tmk_sigio_lock;

extern  pthread_mutex_t Tmk_monitor_lock;

#endif

/*
 * Function Prototypes
 */
void	Tmk_accept( int i );

void	Tmk_accept_initialize( int i );

void	Tmk_barrier_initialize( void );

void	Tmk_barrier_sigio_handler();

void	Tmk_barrier_sigio_duplicate_handler();

void	Tmk_cond_broadcast_sigio_handler();

void	Tmk_cond_signal_sigio_handler();

void	Tmk_cond_wait_sigio_handler();

void	Tmk_cond_wait_sigio_duplicate_handler();

void	Tmk_connect( int i );

void	Tmk_connect_initialize( void );

void	Tmk_connect_sigio_duplicate_handler();

void	Tmk_diff_create( page_t page, write_notice_t write_notice );

void	Tmk_diff_initialize( void );

void	Tmk_diff_repo( void );

int	Tmk_diff_repo_test( void );

void	Tmk_diff_request( page_t page );

void	Tmk_diff_sigio_handler();

void	Tmk_distribute_initialize( void );

void	Tmk_distribute_sigio_handler();

void	Tmk_distribute_sigio_duplicate_handler();

void	Tmk_exit_sigio_handler( const struct req_typ *req );

void	Tmk_err( const char *format, ... );

void	Tmk_errno_check( const char *format, ... );

void	Tmk_errexit( const char *format, ... ) __attribute__((__noreturn__));

void	Tmk_herrexit( const char *format, ... ) __attribute__((__noreturn__));

void	Tmk_interval_create( unsigned short vector_time_[] );

void	Tmk_interval_incorporate( caddr_t msg, int size, unsigned short vector_time_[] );

void	Tmk_interval_initialize( void );

void	Tmk_interval_repo( void );

int	Tmk_interval_repo_test( void );

caddr_t Tmk_interval_request( caddr_t msg, const unsigned short vector_time_[] );

caddr_t	Tmk_interval_request_proc( caddr_t msg, int proc_id, unsigned time );

void	Tmk_lock_initialize( void );

void	Tmk_lock_sigio_handler();

void	Tmk_lock_sigio_duplicate_handler();

void	Tmk_page_dirty_merge( page_t page );

void	Tmk_page_initialize( void );

void	Tmk_page_inval_merge( page_t page );

void	Tmk_page_inval_perform( void );

void	Tmk_page_request( page_t page );

void	Tmk_page_sigio_handler( const struct req_typ *req );

void	Tmk_perrexit( const char *format, ... ) __attribute__((__noreturn__));

void	Tmk_repo( void );

void	Tmk_repo_initialize( void );

void	Tmk_repo_sigio_handler();

void	Tmk_repo_sigio_duplicate_handler();

void	Tmk_sched_initialize( void );

void	Tmk_sched_sigio_handler();

void	Tmk_sched_sigio_duplicate_handler();

void	Tmk_segv_initialize( void );

void	Tmk_sigio_buffer_release();

void	Tmk_sigio_initialize( void );

void	Tmk_tout_initialize( void );

void	Tmk_twin_alloc_and_copy( page_t page );

void    Tmk_twin_free( page_t page );
