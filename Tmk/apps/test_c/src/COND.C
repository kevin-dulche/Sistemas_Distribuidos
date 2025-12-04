/*
 * Condition variable test program
 *
 * Created by Rob Fowler
 *
 * $Id: cond.c,v 1.5 1997/05/26 04:32:09 alc Exp $
 */
#include <sys/time.h>
#include <stdio.h>

#include "Tmk.h"

unsigned        monlock;
int             pshift;

extern char    *optarg;

int             debug = 0;
int             mysignals;
int             mywaits;
int             iterations = 5;

struct phonyq {
	int             count;
	int             value;
	int             visits[TMK_NPROCS];
	int             waits[TMK_NPROCS];
	int             signals[TMK_NPROCS];
	int             padding[2000];
};

struct phonyq  *mq;

main(argc, argv)
	int             argc;
	char           *argv[];
{
	struct timeval  start, end;
	int             mycount = 0;
	char            c;

	pshift = monlock = 0;

	while ((c = getopt(argc, argv, "di:l:s:")) != -1)
		switch (c) {
		case 'd':
			debug++;
			printf("Debugging on.\n");
			break;
		case 'i':
			iterations = atoi(optarg);
			Tmk_err("Run for %d iterations \n", iterations);
			break;
		case 'l':
			monlock = atoi(optarg);
			Tmk_err("Using lock %d\n", monlock);
			break;
		case 's':
			pshift = atoi(optarg);
			break;
		}

	Tmk_startup(argc, argv);

	if (Tmk_nprocs < 2) {
		Tmk_errexit("Producer/consumer test.\n"
			    "Must be run with at least two procs.\n");
	}

	Tmk_err("Producer =  %d\n", pshift);
	Tmk_err("Consumer =  %d\n", (pshift + 1) % Tmk_nprocs);

	if (Tmk_proc_id == 0) {

		int             i;

		if ((mq = (struct phonyq *) Tmk_malloc(sizeof(struct phonyq))) == NULL)
			Tmk_errexit("Tmk_malloc error");

		mq->value = 2000;
		mq->count = 0;
		for (i = 0; i < 2000; i++)
			mq->padding[i] = i;

		Tmk_distribute((char *)&mq, sizeof(mq));
	}
	Tmk_barrier(0);

	/*
	 * mq->value == 2000 means the buffer is empty.  == 3000 means full
	 * condition 1 is used to signal full. condition 0 is used to signal
	 * empty.
	 */

	gettimeofday(&start, NULL);

	if (Tmk_proc_id == pshift) {

		if (debug)
			Tmk_err("Producer (node %d) ready to start\n", Tmk_proc_id);

		mywaits = mysignals = 0;

		while (mycount < iterations) {

			Tmk_lock_acquire(monlock);

			if (debug)
				Tmk_err("... got lock ...");

			mq->visits[Tmk_proc_id]++;

			if (mq->value == 2000) {

				mq->value = 3000;
				mycount = mq->count += 1;

				mq->signals[Tmk_proc_id] = mysignals += 1;

				Tmk_lock_cond_signal(monlock, 1);
			}
			else if (mq->value == 3000) {

				mq->waits[Tmk_proc_id] = mywaits += 1;

				Tmk_lock_cond_wait(monlock, 0);

				if (debug)
					fprintf(stderr, "after %d awakenings\n", mywaits);
			}
			else {
				Tmk_err("Something really wrong\n");
			}
			Tmk_lock_release(monlock);
		}
	}
	else if (Tmk_proc_id == (pshift + 1) % Tmk_nprocs) {

		if (debug)
			Tmk_err("Consumer %d ready to start\n", Tmk_proc_id);

		mywaits = mysignals = 0;

		while (mycount < iterations) {

			Tmk_lock_acquire(monlock);

			if (debug)
				Tmk_err("... got lock ...");

			mq->count++;
			mq->visits[Tmk_proc_id]++;

			if (mq->value == 3000) {
				int             i;

				mycount = mq->count;
				mq->value = 2000;
				mq->signals[Tmk_proc_id] = mysignals += 1;

				Tmk_lock_cond_signal(monlock, 0);
			}
			else if (mq->value == 2000) {

				mq->waits[Tmk_proc_id] = mywaits += 1;

				Tmk_lock_cond_wait(monlock, 1);

				if (debug)
					fprintf(stderr, "consumer after %d iterations\n", mywaits);
			}
			else {
				Tmk_err("Something really wrong\n");
			}
			Tmk_lock_release(monlock);
		}
	}
	else {
		if (debug)
			Tmk_err("Manager (?)  %d ready to start\n", Tmk_proc_id);
	}
	Tmk_barrier(0);

	gettimeofday(&end, NULL);

	fprintf(stderr, "Elapsed time: %.3f milliseconds\n",
		(((end.tv_sec * 1000000.0 + end.tv_usec) -
		  (start.tv_sec * 1000000.0 + start.tv_usec)) / 1000.0));

	Tmk_exit(0);
}
