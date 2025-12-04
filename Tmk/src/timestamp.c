/*
 * $Id: timestamp.c,v 10.1.1.3 1999/08/03 05:29:31 alc Exp $
 *
 * Calls gettimeofday and prints the seconds field.
 *
 * The standard date (1) command doesn't provide this option.
 *
 * History:
 *	16-Jan-1995	R. Fowler	Created
 */
#if defined(PTHREADS)
#if defined(__sun) && defined(__SVR4)
#define __EXTENSIONS__
#endif
#endif
#include <stdio.h>
#include <sys/time.h>

int
main(void)
{
	struct timeval now;

	if (0 > gettimeofday(&now, NULL)) {
		perror("gettimeofday");
		exit(1);
	}
	printf("%d", now.tv_sec);
	exit(0);
}
