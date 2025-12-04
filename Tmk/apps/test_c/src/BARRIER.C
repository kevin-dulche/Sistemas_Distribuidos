/*
 * $Id: barrier.c,v 1.1 1997/04/12 08:55:40 alc Exp $
 */
#include <sys/time.h>
#include <stdio.h>

#include "Tmk.h"

main(argc, argv)
	int		argc;
	char	       *argv[];
{
	struct timeval	start, end;
	int		c, i;

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		}

	Tmk_startup(argc, argv);

	Tmk_barrier(0);

	if (Tmk_proc_id == 0) {

		gettimeofday(&start, NULL);

		for (i = 0; i < 1024; i++)
			Tmk_barrier(0);

		gettimeofday(&end, NULL);

		printf("Elapsed time: %.3f milliseconds\n",
		       (((1000000.0*end.tv_sec) + end.tv_usec) -
			((1000000.0*start.tv_sec) + start.tv_usec))/1024000.0);
	}
	else
		for (i = 0; i < 1024; i++)
			Tmk_barrier(0);

	Tmk_exit(0);
}
