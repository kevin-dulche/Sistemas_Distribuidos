#include <stdlib.h>
#include <stdio.h>

/*
 * RAND_MAX isn't defined by SunOS 4.1.x's <stdlib.h>.
 */

#if   ! defined(RAND_MAX)
#	define	RAND_MAX 32767
#endif

#ifndef HZ
#define HZ 60.0
#endif

/*
 * XRAND: generate floating-point random number.
 */

double xrand(xl, xh)
  double xl, xh;			/* lower, upper bounds on number */
{
    return (xl + (xh - xl)*rand()/RAND_MAX);
}

/*
 * ERROR: scream and die quickly.
 */

void barnes_error(msg, a1, a2, a3, a4)
  char *msg, *a1, *a2, *a3, *a4;
{
    extern int errno;
    
    fprintf(stderr, msg, a1, a2, a3, a4);
    if (errno != 0)
        perror("Error");
    exit(0);
}

/*
 * CPUTIME: compute CPU time in min.
 */

#include <sys/types.h>
#include <sys/times.h>

double cputime()
{
    struct tms buffer;
    
    if (times(&buffer) == -1)
        barnes_error("times() call failed\n");
    return (buffer.tms_utime / (60.0 * HZ));
}
