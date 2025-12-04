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
 * $Id: lock.c,v 10.20.1.7.0.3 1998/07/17 04:41:57 alc Exp $
 *
 * Description:    
 *	lock synchronization routines
 *
 * External Functions:
 *			Tmk_lock_acquire,
 *			Tmk_lock_release,
 *			Tmk_lock_initialize,
 *			Tmk_lock_sigio_handler,
 *			Tmk_lock_sigio_duplicate_handler
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox 	Created
 *	 8-Aug-1993	Alan L. Cox	Added reliable message protocol
 *	19-Jul-1994	Alan L. Cox	Reduced
 *
 *	Version 0.9.1
 *
 *	14-Jan-1995	Alan L. Cox	Adapted for STREAMS
 *
 *	Version 0.9.2
 *
 *	21-May-1995	Alan L. Cox	Adapted for SGI/IRIX
 *
 *	Version 0.9.3
 *
 *	15-Jul-1995	Alan L. Cox	Simplified the lock sigio handler
 *
 *	Version 0.9.4
 *
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

typedef
struct	waitq_entry    *waitq_entry_t;

static
struct	waitq_entry {
	waitq_entry_t	next;
	struct req_cond	req;
	volatile int	queued;
}	waitq_[NPROCS];

struct	cond	{
	waitq_entry_t	waitq_head;
	waitq_entry_t	waitq_tail;
	unsigned short	signal_vector_time_[NPROCS];
};

static
unsigned char	tail_[NPROCS];

static
unsigned short	vector_time[NPROCS][NPROCS];

static
struct	lock	{
	unsigned char	manager;
	unsigned char	tail;
	unsigned char	held;
	unsigned char	next;
	unsigned	release_seqno;
	unsigned	time;
	struct cond	cond_[NCONDS];
}		lock_array_[NLOCKS];

static
struct	req_typ	req_acquire = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_LOCK };

static
struct	iovec	req_acquire_iov[] = {
    { (caddr_t)&req_acquire, sizeof(req_acquire) },
    { (caddr_t) proc_vector_time_, sizeof(proc_vector_time_) } };

static
struct	msghdr	req_acquire_hdr = { 0, 0, req_acquire_iov, sizeof(req_acquire_iov)/sizeof(req_acquire_iov[0]), 0, 0 };

static
struct	req_typ	req_broadcast = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_COND_BROADCAST };

static
unsigned short	req_broadcast_id2;

static
struct	iovec	req_broadcast_iov[] = {
    { (caddr_t)&req_broadcast, sizeof(req_broadcast)},
    { (caddr_t) proc_vector_time_, sizeof(proc_vector_time_) },
    { (caddr_t)&req_broadcast_id2, sizeof(req_broadcast_id2)} };

static
struct	msghdr	req_broadcast_hdr = { 0, 0, req_broadcast_iov, sizeof(req_broadcast_iov)/sizeof(req_broadcast_iov[0]), 0, 0 };

static
struct	req_typ	req_signal = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_COND_SIGNAL };

static
unsigned short	req_signal_id2;

static
struct	iovec	req_signal_iov[] = {
    { (caddr_t)&req_signal, sizeof(req_signal)},
    { (caddr_t) proc_vector_time_, sizeof(proc_vector_time_) },
    { (caddr_t)&req_signal_id2, sizeof(req_signal_id2)} };

static
struct	msghdr	req_signal_hdr = { 0, 0, req_signal_iov, sizeof(req_signal_iov)/sizeof(req_signal_iov[0]), 0, 0 };

static
struct	req_typ	req_wait = { /*seqno=*/0, /*from=*/0, /*type=*/REQ_COND_WAIT };

static
unsigned short	req_wait_id2;

static
struct	iovec	req_wait_iov[] = {
    { (caddr_t)&req_wait, sizeof(req_wait) },
    { (caddr_t) proc_vector_time_, sizeof(proc_vector_time_) },
    { (caddr_t)&req_wait_id2, sizeof(req_wait_id2) } };

static
struct	msghdr	req_wait_hdr = { 0, 0, req_wait_iov, sizeof(req_wait_iov)/sizeof(req_wait_iov[0]), 0, 0 };

struct	rep_typ {
	unsigned	seqno;
	char		data[MTU - sizeof(unsigned)];
};

/*
 *
 */
void	Tmk_lock_acquire(lock_id)
	unsigned	lock_id;
{
#if defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
	if (lock_id < NLOCKS) {

		struct	lock   *lock = &lock_array_[lock_id];
		struct	timeval	start, end;
		struct	rep_typ	rep;
		int		fd, from, j, size;
		unsigned short	vector_time_[NPROCS];
		sigset_t	mask;

		sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

		Tmk_stat.acquires++;

		if (lock->held)
			Tmk_errexit("Tmk_lock_acquire: %d REacquired (value: %d)\n", lock_id, lock->held);

		lock->held = 1;

		if (lock->next != Tmk_proc_id) {

			lock->next = Tmk_proc_id;

			if ((from = lock->manager) == Tmk_proc_id) {

				from = lock->tail;

				lock->tail = Tmk_proc_id;
			}
			if (tmk_stat_flag > 2)
				gettimeofday(&start, NULL);

			req_acquire.id = lock_id;
			req_acquire.seqno = req_seqno += SEQNO_INCR;
		rexmit:
			while (0 > sendmsg(req_fd_[from], &req_acquire_hdr, 0))
				Tmk_errno_check("Tmk_lock_acquire<sendmsg>");

			Tmk_tout_flag = 0;

			setitimer(ITIMER_REAL, &Tmk_tout, NULL);

			memcpy(vector_time_, proc_vector_time_, sizeof(proc_vector_time_));

			sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);

			if (Tmk_debug)
				Tmk_err("lock: %d (to: %d) ", lock_id, from);
	         retry:
			if (lock->manager == Tmk_proc_id)
				fd = req_fd_[from];
			else {
				fd_set	readfds = req_fds;

				if (0 > select(req_maxfdp1, (fd_set_t)&readfds, NULL, NULL, NULL))
					if (Tmk_tout_flag) {

						if (Tmk_debug)
							Tmk_err("Tmk_lock_acquire<timeout: %d>: %d seqno == %d\n",
								from, lock_id, req_acquire.seqno);

						sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

						goto rexmit;
					}
					else if (errno == EINTR)
						goto retry;
#if defined(__sun) && defined(__SVR4)
					else if (errno == 0)
						goto retry;
#endif
					else
						Tmk_perrexit("Tmk_lock_acquire<select>");

				for (j = 0; j < Tmk_nprocs; j++) {
					if (j == Tmk_proc_id)
						continue;

					fd = req_fd_[j];

					if (FD_ISSET(fd, &readfds))
						goto receive;
				}
#if defined(_AIX) || defined(__hpux)
				goto retry;
#else
				Tmk_errexit("Tmk_lock_acquire<req_fds>");
#endif
			}
	       receive:
			if ((size = recv(fd, (char *)&rep, sizeof(rep), 0)) < 0)
				if (Tmk_tout_flag) {

					if (Tmk_debug)
						Tmk_err("Tmk_lock_acquire<timeout: %d>: %d seqno == %d\n",
							from, lock_id, req_acquire.seqno);

					sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

					goto rexmit;
				}
				else if (errno == EINTR)
					goto retry;
				else
					Tmk_perrexit("Tmk_lock_acquire<recv>");

			if (rep.seqno != req_acquire.seqno) {

				if (Tmk_debug)
					Tmk_err("Tmk_lock_acquire<bad seqno: %d>: %d seqno == %d (received: %d)\n",
						j, lock_id, req_acquire.seqno, rep.seqno);

				goto retry;
			}
			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

			if (tmk_stat_flag > 2) {

				gettimeofday(&end, NULL);

				Tmk_stat.acquire_time += 1000000*(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
			}
			Tmk_stat.bytes += size;
			Tmk_stat.messages_for_acquires++;

			Tmk_interval_incorporate(rep.data, size - sizeof(rep.seqno), vector_time_);
		}
		sigio_mutex(SIG_SETMASK, &mask, NULL);
	}
	else
		Tmk_errexit("Tmk_lock_acquire: lock id == %d\n", lock_id);
#if defined(PTHREADS)
	pthread_mutex_unlock(&Tmk_monitor_lock);
#endif
}

/*
 *
 */
void
Tmk_lock_sigio_handler(
	const struct req_syn *req)
{
	struct rep_typ	rep;

	struct lock    *lock = &lock_array_[req->id];

	int		from = req->from;

	if (lock->next == Tmk_proc_id) {

		if ( ! lock->held) {

			if (lock->time == proc_vector_time_[Tmk_proc_id])
				Tmk_interval_create(proc_vector_time_);

			lock->release_seqno = rep.seqno = req->seqno;

			while (0 > send(rep_fd_[from], (char *)&rep, Tmk_interval_request(rep.data, req->vector_time) - (caddr_t)&rep, 0))
				Tmk_errno_check("Tmk_lock_sigio_handler<send>");

			inverse_time_[from] = proc_vector_time_[Tmk_proc_id];
		}
		else
			memcpy(vector_time[from], req->vector_time, sizeof(req->vector_time));

		lock->next = from;

		if (lock->manager == Tmk_proc_id)
			lock->tail = from;
	}
	else if (lock->manager == Tmk_proc_id) {

		int	tail = lock->tail;

		while (0 > send(req_fd_[tail], (char *) req, sizeof(struct req_syn), 0))
			Tmk_errno_check("Tmk_lock_sigio_handler<send>");

		tail_[from] = tail;

		lock->tail = from;
	}
	else
		Tmk_errexit("Tmk_lock_sigio_handler<next == %d>: id == %d\n", lock->next, req->id);
}

/*
 *
 */
void
Tmk_lock_sigio_duplicate_handler(
	const struct req_syn *req)
{
	struct rep_typ	rep;

	struct lock    *lock = &lock_array_[req->id];

	int		from = req->from;

	if (lock->release_seqno == req->seqno) {

		rep.seqno = req->seqno;

		while (0 > send(rep_fd_[from], (char *)&rep, Tmk_interval_request(rep.data, req->vector_time) - (caddr_t)&rep, 0))
			Tmk_errno_check("Tmk_lock_sigio_duplicate_handler<send>");
	}
	else if (lock->held && lock->next == from) {
		/*
		 * This is a repeated request for a held lock.  Do nothing.
		 */
	}
	else if (lock->manager == Tmk_proc_id) {

		while (0 > send(req_fd_[tail_[from]], (char *) req, sizeof(struct req_syn), 0))
			Tmk_errno_check("Tmk_lock_sigio_duplicate_handler<send>");
	}
	else
		Tmk_err("Tmk_lock_sigio_duplicate_handler: lock->release_seqno == %d && req->seqno == %d\n",
			lock->release_seqno, req->seqno);
}

/*
 *
 */
void	Tmk_lock_release(lock_id)
	unsigned	lock_id;
{
	if (lock_id < NLOCKS) {

		struct	lock   *lock = &lock_array_[lock_id];

		if (lock->held) {

			int		to;

			sigset_t	mask;

			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

			lock->held = 0;

			if ((to = lock->next) != Tmk_proc_id) {

				struct	rep_typ	rep;

				Tmk_interval_create(proc_vector_time_);

				lock->release_seqno = rep.seqno = rep_seqno_[to];

				while (0 > send(rep_fd_[to], (char *)&rep, Tmk_interval_request(rep.data, vector_time[to]) - (caddr_t)&rep, 0))
					Tmk_errno_check("Tmk_lock_release<send>");

				inverse_time_[to] = proc_vector_time_[Tmk_proc_id];
			}
			else
				lock->time = proc_vector_time_[Tmk_proc_id];

			sigio_mutex(SIG_SETMASK, &mask, NULL);
		}
		else
			Tmk_errexit("Tmk_lock_release: %d REreleased\n", lock_id);
	}
	else
		Tmk_errexit("Tmk_lock_release: lock id == %d\n", lock_id);
}

/*
 *
 */
static void
update_time(
	struct cond    *cond,
	const unsigned short
			vector_time_[])
{
	int		i = 0;

	do {
		if (vector_time_[i] > cond->signal_vector_time_[i])
			cond->signal_vector_time_[i] = vector_time_[i];
	} while (++i < Tmk_nprocs);
}
    
/*
 *
 */
static
waitq_entry_t
	waitq_dequeue(struct cond *cond)
{
	waitq_entry_t	we;

	if ((we = cond->waitq_head) != NULL) {

		if ((cond->waitq_head = we->next) == NULL)
			cond->waitq_tail = (waitq_entry_t)&cond->waitq_head;

		we->queued = 0;

		if (we->req.from != Tmk_proc_id) {

			we->req.type = REQ_LOCK;

			Tmk_lock_sigio_handler((struct req_syn *)&we->req);
		}
	}
	return we;
}

/*
 *
 */
void	Tmk_lock_cond_broadcast(lock_id, cond_id)
	unsigned	lock_id;
	unsigned	cond_id;
{
#if defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
	if (lock_id < NLOCKS) {

		if (cond_id < NCONDS) {

			struct	lock   *lock;

			unsigned	manager;

			sigset_t	mask;

			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

			Tmk_stat.broadcasts++;

			lock = &lock_array_[lock_id];

			if ((manager = lock->manager) == Tmk_proc_id) {

				struct	cond   *cond = &lock->cond_[cond_id];

				while (waitq_dequeue(cond));

				update_time(cond, proc_vector_time_);
			}
			else {
				unsigned	rep_seqno;

				int		fd = req_fd_[manager], size;

				req_broadcast.seqno = req_seqno += SEQNO_INCR;
				req_broadcast.id = lock_id;
				req_broadcast_id2 = cond_id;
			rexmit:
				while (0 > sendmsg(fd, &req_broadcast_hdr, 0))
					Tmk_errno_check("Tmk_lock_cond_broadcast<sendmsg>");

				Tmk_tout_flag = 0;

				setitimer(ITIMER_REAL, &Tmk_tout, NULL);

				sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);

				if (Tmk_debug)
					Tmk_err("broadcast: %d %d (to: %d)\n", lock_id, cond_id, manager);
			retry:
				if ((size = recv(fd, (char *)&rep_seqno, sizeof(rep_seqno), 0)) < 0)
					if (Tmk_tout_flag) {

						if (Tmk_debug)
							Tmk_err("Tmk_lock_cond_broadcast<timeout: %d>: %d %d seqno == %d\n",
								manager, lock_id, cond_id, req_broadcast.seqno);

						sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

						goto rexmit;
					}
					else if (errno == EINTR)
						goto retry;
					else
						Tmk_perrexit("Tmk_lock_cond_broadcast<recv>");

				if (rep_seqno != req_broadcast.seqno) {

					if (Tmk_debug)
						Tmk_err("Tmk_lock_cond_broadcast<bad seqno: %d>: %d %d seqno == %d (received: %d)\n", 
							manager, lock_id, cond_id, req_broadcast.seqno, rep_seqno);

					goto retry;
				}
				sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

				Tmk_stat.messages++;
				Tmk_stat.bytes += size;
			}
			sigio_mutex(SIG_SETMASK, &mask, NULL);
		}
		else
			Tmk_errexit("Tmk_lock_cond_broadcast: cond id == %d\n", cond_id);
	}
	else
		Tmk_errexit("Tmk_lock_cond_broadcast: lock id == %d\n", lock_id);
#if defined(PTHREADS)
	pthread_mutex_unlock(&Tmk_monitor_lock);
#endif
}

/*
 *
 */
void
Tmk_cond_broadcast_sigio_handler(
	const struct req_cond *req)
{
	struct	cond   *cond = &lock_array_[req->id].cond_[req->id2];

	while (waitq_dequeue(cond));

	update_time(cond, req->vector_time);

	while (0 > send(rep_fd_[req->from], (char *) req, sizeof(req->seqno), 0))
		Tmk_errno_check("Tmk_cond_broadcast_sigio_handler<send>");
}

/*
 *
 */
void	Tmk_lock_cond_signal(lock_id, cond_id)
	unsigned	lock_id;
	unsigned	cond_id;
{
#if defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
	if (lock_id < NLOCKS) {

		if (cond_id < NCONDS) {

			struct	lock   *lock;

			unsigned	manager;

			sigset_t	mask;

			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

			Tmk_stat.signals++;

			lock = &lock_array_[lock_id];

			if ((manager = lock->manager) == Tmk_proc_id) {

				struct	cond   *cond = &lock->cond_[cond_id];

				waitq_dequeue(cond);

				update_time(cond, proc_vector_time_);
			}
			else {
				unsigned	rep_seqno;

				int		fd = req_fd_[manager], size;

				req_signal.seqno = req_seqno += SEQNO_INCR;
				req_signal.id = lock_id;
				req_signal_id2 = cond_id;
			rexmit:
				while (0 > sendmsg(fd, &req_signal_hdr, 0))
					Tmk_errno_check("Tmk_lock_cond_signal<sendmsg>");

				Tmk_tout_flag = 0;

				setitimer(ITIMER_REAL, &Tmk_tout, NULL);

				sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);

				if (Tmk_debug)
					Tmk_err("signal: %d %d (to: %d)\n", lock_id, cond_id, manager);
			retry:
				if ((size = recv(fd, (char *)&rep_seqno, sizeof(rep_seqno), 0)) < 0)
					if (Tmk_tout_flag) {

						if (Tmk_debug)
							Tmk_err("Tmk_lock_cond_signal<timeout: %d>: %d %d seqno == %d\n",
								manager, lock_id, cond_id, req_signal.seqno);

						sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

						goto rexmit;
					}
					else if (errno == EINTR)
						goto retry;
					else
						Tmk_perrexit("Tmk_lock_cond_signal<recv>");

				if (rep_seqno != req_signal.seqno) {

					if (Tmk_debug)
						Tmk_err("Tmk_lock_cond_signal<bad seqno: %d>: %d %d seqno == %d (received: %d)\n", 
							manager, lock_id, cond_id, req_signal.seqno, rep_seqno);

					goto retry;
				}
				sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

				Tmk_stat.messages++;
				Tmk_stat.bytes += size;
			}
			sigio_mutex(SIG_SETMASK, &mask, NULL);
		}
		else
			Tmk_errexit("Tmk_lock_cond_signal: cond id == %d\n", cond_id);
	}
	else
		Tmk_errexit("Tmk_lock_cond_signal: lock id == %d\n", lock_id);
#if defined(PTHREADS)
	pthread_mutex_unlock(&Tmk_monitor_lock);
#endif
}

/*
 *
 */
void
Tmk_cond_signal_sigio_handler(
	const struct req_cond *req)
{
	struct	cond   *cond = &lock_array_[req->id].cond_[req->id2];

	waitq_dequeue(cond);

	update_time(cond, req->vector_time);

	while (0 > send(rep_fd_[req->from], (char *) req, sizeof(req->seqno), 0))
		Tmk_errno_check("Tmk_cond_signal_sigio_handler<send>");
}

/*
 *
 */
void    Tmk_lock_cond_wait(lock_id, cond_id)
	unsigned        lock_id;
	unsigned        cond_id;
{   
#if defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
	if (lock_id < NLOCKS) {

		if (cond_id < NCONDS) {

			struct	lock   *lock;

			unsigned	manager;

			sigset_t	mask;

			sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

			Tmk_stat.waits++;

			lock = &lock_array_[lock_id];

			if ( ! lock->held)
				Tmk_errexit("Tmk_lock_cond_wait: lock not held id == %d\n", lock_id);

			while (lock->next == Tmk_proc_id) {

				sigset_t	empty_mask;

				sigemptyset(&empty_mask);

				sigsuspend(&empty_mask);
			}
			Tmk_lock_release(lock_id);

			if ((manager = lock->manager) == Tmk_proc_id) {

				struct	cond   *cond = &lock->cond_[cond_id];

				waitq_entry_t	we = &waitq_[Tmk_proc_id];

				we->next = NULL;

				cond->waitq_tail = cond->waitq_tail->next = we;

				we->queued = 1;

				/*
				 *	we->req.from = Tmk_proc_id;
				 */
				while (we->queued) {

					sigset_t	empty_mask;

					sigemptyset(&empty_mask);

					sigsuspend(&empty_mask);
				}
				Tmk_lock_acquire(lock_id);
			}
			else {
				struct	rep_typ	rep;
				int		fd, j, size;
				fd_set		readfds;
				unsigned short	vector_time_[NPROCS];

				lock->held = 1;
				lock->next = Tmk_proc_id;

				req_wait.seqno = req_seqno += SEQNO_INCR;
				req_wait.id = lock_id;
				req_wait_id2 = cond_id;
			rexmit:
				while (0 > sendmsg(req_fd_[manager], &req_wait_hdr, 0))
					Tmk_errno_check("Tmk_condition_wait<sendmsg>");

				Tmk_tout_flag = 0;

				setitimer(ITIMER_REAL, &Tmk_tout, NULL);

				memcpy(vector_time_, proc_vector_time_, sizeof(proc_vector_time_));

				sigio_mutex(SIG_UNBLOCK, &ALRM_and_IO_mask, NULL);

				if (Tmk_debug)
					Tmk_err("wait: %d %d (to: %d) ", lock_id, cond_id, manager);
			retry:
				readfds = req_fds;

				if (0 > select(req_maxfdp1, (fd_set_t)&readfds, NULL, NULL, NULL))
					if (Tmk_tout_flag) {

						if (Tmk_debug)
							Tmk_err("Tmk_condition_wait<timeout: %d>: %d %d seqno == %d\n",
								manager, lock_id, cond_id, req_wait.seqno);

						sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

						goto rexmit;
					}
					else if (errno == EINTR)
						goto retry;
#if defined(__sun) && defined(__SVR4)
					else if (errno == 0)
						goto retry;
#endif
					else
						Tmk_perrexit("Tmk_condition_wait<select>");

				for (j = 0; j < Tmk_nprocs; j++) {

					if (j == Tmk_proc_id)
						continue;

					fd = req_fd_[j];

					if (FD_ISSET(fd, &readfds))
						goto receive;
				}
#if defined(_AIX) || defined(__hpux)
				goto retry;
#else
				Tmk_errexit("Tmk_lock_cond_wait<req_fds>");
#endif
		receive:
				if ((size = recv(fd, (char *)&rep, MTU, 0)) < 0)
					if (Tmk_tout_flag) {

						if (Tmk_debug)
							Tmk_err("Tmk_lock_cond_wait<timeout: %d>: %d %d seqno == %d\n", 
								manager, lock_id, cond_id, req_wait.seqno);

						sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

						goto rexmit;
					}
					else if (errno == EINTR)
						goto retry;
					else
						Tmk_perrexit("Tmk_lock_cond_wait<recv>");

				if (rep.seqno != req_wait.seqno) {

					if (Tmk_debug)
						Tmk_err("Tmk_lock_cond_wait<bad seqno: %d>: %d %d seqno == %d (received: %d)\n",
							j, lock_id, cond_id, req_wait.seqno, rep.seqno);

					goto retry;
				}
				sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, NULL);

				Tmk_stat.messages++;
				Tmk_stat.bytes += size;

				Tmk_interval_incorporate(rep.data, size - sizeof(rep.seqno), vector_time_);
			}
			sigio_mutex(SIG_SETMASK, &mask, NULL);
		}
		else
			Tmk_errexit("Tmk_lock_cond_wait: cond id == %d\n", cond_id);
	}
	else
		Tmk_errexit("Tmk_lock_cond_wait: lock id == %d\n", lock_id);
#if defined(PTHREADS)
	pthread_mutex_unlock(&Tmk_monitor_lock);
#endif
}

/*
 *
 */
void	Tmk_cond_wait_sigio_handler(req)
	struct req_cond *req;
{
	struct	cond   *cond = &lock_array_[req->id].cond_[req->id2];

	unsigned	from = req->from;

	if (req->vector_time[from] <= cond->signal_vector_time_[from]) {

		/*
		 * The wait request is older than the last signal.
		 */
		req->type = REQ_LOCK;

		Tmk_lock_sigio_handler((struct req_syn *) req);
	}
	else {
		waitq_entry_t	we = &waitq_[from];

		memcpy(&we->req, req, sizeof(*req));

		we->next = NULL;

		cond->waitq_tail = cond->waitq_tail->next = we;

		we->queued = 1;
	}
}

/*
 *
 */
void    Tmk_cond_wait_sigio_duplicate_handler(req)
	struct req_cond *req;
{
	waitq_entry_t	we = &waitq_[req->from];

	if (we->queued && (we->req.seqno == req->seqno)) {
		/*
		 * This wait request is still queued.  Ignore the duplicate.
		 */
		return;
	}

	/*
	 *
	 */
	req->type = REQ_LOCK;

	Tmk_lock_sigio_duplicate_handler((struct req_syn *) req);
}

/*
 *
 */
void
Tmk_lock_initialize( void )
{
	int	i, j;

	for (i = 0; i < NLOCKS; i++) {

		struct	lock   *lock = &lock_array_[i];

		lock->manager = i % Tmk_nprocs;
		lock->tail = 0;
		lock->held = 0;
		lock->next = 0;

		for (j = 0; j < NCONDS; j++) {

			struct	cond   *cond = &lock->cond_[j];

			cond->waitq_head = NULL;
			cond->waitq_tail = (waitq_entry_t)&cond->waitq_head;

			memset(cond->signal_vector_time_, 0, sizeof(cond->signal_vector_time_));
		}
	}

	/*
	 * Required by waitq_dequeue.
	 */
	waitq_[Tmk_proc_id].req.from = Tmk_proc_id;

	req_acquire.from =
	req_broadcast.from =
	req_signal.from =
	req_wait.from = Tmk_proc_id;
}
