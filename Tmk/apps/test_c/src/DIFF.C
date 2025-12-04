/*
 * $Id: diff.c,v 1.1 1997/04/12 08:58:27 alc Exp $
 */
#include <sys/time.h>
#include <stdio.h>

#include <assert.h>

#include "Tmk.h"

char   *char_array;

main(argc, argv)
	int		argc;
	char	       *argv[];
{
	struct timeval	start, end;
	int		c, i, total = 0;

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		}

	Tmk_startup(argc, argv);

	if (Tmk_proc_id == 0) {

		char_array = Tmk_malloc(1025*Tmk_page_size);

		Tmk_distribute((char *)&char_array, sizeof(char_array));
	}
	Tmk_barrier(0);

	if (Tmk_proc_id == 1)
		for (i = 0; i < 1024*Tmk_page_size; i += Tmk_page_size)
			char_array[i] = 0;

	Tmk_barrier(0);

	if (Tmk_proc_id == 0) {

		gettimeofday(&start, NULL);

		for (i = 0; i < 1024*Tmk_page_size; i += Tmk_page_size)
			total += char_array[i];

		gettimeofday(&end, NULL);

		assert(total == 0);
	
		printf("Elapsed time: %.3f milliseconds\n",
		       (((1000000.0*end.tv_sec) + end.tv_usec) -
			((1000000.0*start.tv_sec) + start.tv_usec))/1024000.0);
	}
	Tmk_barrier(0);

	if (Tmk_proc_id == 1)
		for (i = 0; i < 1024*Tmk_page_size; i++)
			char_array[i] = 127;

	Tmk_barrier(0);

	if (Tmk_proc_id == 0) {

		gettimeofday(&start, NULL);

		for (i = 0; i < 1024*Tmk_page_size; i += Tmk_page_size)
			total += char_array[i];

		gettimeofday(&end, NULL);

		assert(total == 1024*127);
	
		printf("Elapsed time: %.3f milliseconds\n",
		       (((1000000.0*end.tv_sec) + end.tv_usec) -
			((1000000.0*start.tv_sec) + start.tv_usec))/1024000.0);
	}
	Tmk_barrier(0);

	Tmk_exit(0);
}
