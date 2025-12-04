#include <sys/time.h>

double	timer_()
{
	struct	timeval	time;

	gettimeofday(&time, 0);

	return time.tv_sec + time.tv_usec/1000000.0;
}
