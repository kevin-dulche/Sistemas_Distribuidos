/*
 * A Red-Black SOR
 *
 *	using separate red and block matrices
 *	to minimize false sharing
 *
 * Solves a M+2 by 2N+2 array
 *
 * $Id: sor.c,v 10.3 1997/07/20 05:00:59 alc Exp $
 */
#include <stdio.h>

#include <sys/time.h>

struct	timeval	start, finish;

#include <Tmk.h>

int	iterations = 10;

int	M = 2000;
int	N = 500;	/* N.B. There are 2N columns. */

float **red_;
float **black_;

/*
 * The first row's index odd.
 */
void	sor_first_row_odd(first_row, end)
	int	first_row;
	int	end;
{
	int	i, j, k;

	for (i = 0; i < iterations; i++) {

		for (j = first_row; j <= end; j++) {

			for (k = 0; k < N; k++) {

				black_[j][k] = (red_[j-1][k] + red_[j+1][k] + red_[j][k] + red_[j][k+1])/(float) 4.0;
			}
			if ((j += 1) > end)
				break;

			for (k = 1; k <= N; k++) {

				black_[j][k] = (red_[j-1][k] + red_[j+1][k] + red_[j][k-1] + red_[j][k])/(float) 4.0;
			}
		}
		Tmk_barrier(0);

		for (j = first_row; j <= end; j++) {

			for (k = 1; k <= N; k++) {

				red_[j][k] = (black_[j-1][k] + black_[j+1][k] + black_[j][k-1] + black_[j][k])/(float) 4.0;
			}
			if ((j += 1) > end)
				break;

			for (k = 0; k < N; k++) {

				red_[j][k] = (black_[j-1][k] + black_[j+1][k] + black_[j][k] + black_[j][k+1])/(float) 4.0;
			}
		}				
		Tmk_barrier(0);
	}
}

/*
 * The first row's index is even.
 */
void	sor_first_row_even(first_row, end)
	int	first_row;
	int	end;
{
	int	i, j, k;

	for (i = 0; i < iterations; i++) {

		for (j = first_row; j <= end; j++) {

			for (k = 1; k <= N; k++) {

				black_[j][k] = (red_[j-1][k] + red_[j+1][k] + red_[j][k-1] + red_[j][k])/(float) 4.0;
			}
			if ((j += 1) > end)
				break;

			for (k = 0; k < N; k++) {

				black_[j][k] = (red_[j-1][k] + red_[j+1][k] + red_[j][k] + red_[j][k+1])/(float) 4.0;
			}
		}
		Tmk_barrier(0);

		for (j = first_row; j <= end; j++) {

			for (k = 0; k < N; k++) {

				red_[j][k] = (black_[j-1][k] + black_[j+1][k] + black_[j][k] + black_[j][k+1])/(float) 4.0;
			}
			if ((j += 1) > end)
				break;

			for (k = 1; k <= N; k++) {

				red_[j][k] = (black_[j-1][k] + black_[j+1][k] + black_[j][k-1] + black_[j][k])/(float) 4.0;
			}
		}				
		Tmk_barrier(0);
	}
}

extern	char	       *optarg;

main(argc, argv)
	int		argc;
	char	       *argv[];
{
	int		c, i, j;
	int		first_row, last_row;

	while ((c = getopt(argc, argv, "i:m:n:")) != -1)
		switch (c) {
		case 'i':
			iterations = atoi(optarg);
			break;
		case 'm':
			M = atoi(optarg);
			break;
		case 'n':
			N = atoi(optarg);
			break;
		}

	Tmk_startup(argc, argv);

	printf("Red-Black SOR: %d processors running %d iterations over a %d by %d array\n",
	       Tmk_nprocs, iterations, M, 2*N);

	if (Tmk_proc_id == 0) {

		if ((red_ = (float **) Tmk_malloc((M + 2)*sizeof(float *))) == 0)
			Tmk_errexit("out of shared memory");

		if ((black_ = (float **) Tmk_malloc((M + 2)*sizeof(float *))) == 0)
			Tmk_errexit("out of shared memory");

		for (i = 0; i <= M + 1; i++) {

			if ((red_[i] = (float *) Tmk_malloc((N + 1)*sizeof(float))) == 0)
				Tmk_errexit("out of shared memory");

			if ((black_[i] = (float *) Tmk_malloc((N + 1)*sizeof(float))) == 0)
				Tmk_errexit("out of shared memory");
		}
		Tmk_distribute((char *)&red_, sizeof(red_));

		Tmk_distribute((char *)&black_, sizeof(black_));
	}
	Tmk_barrier(0);

	first_row = (M*Tmk_proc_id)/Tmk_nprocs + 1;
	last_row  = (M*(Tmk_proc_id + 1))/Tmk_nprocs;

	for (i = first_row; i <= last_row; i++) {
		/*
		 * Initialize the top edge.
		 */
		if (i == 1)
			for (j = 0; j <= N; j++)
				red_[0][j] = black_[0][j] = (float) 1.0;
		/*
		 * Initialize the left and right edges.
		 */
		if (i & 1) {
			red_[i][0] = (float) 1.0;
			black_[i][N] = (float) 1.0;
		}
		else {
			black_[i][0] = (float) 1.0;
			red_[i][N] = (float) 1.0;
		}
		/*
		 * Initialize the bottom edge.
		 */
		if (i == M)
			for (j = 0; j <= N; j++)
				red_[i+1][j] = black_[i+1][j] = (float) 1.0;
	}
	Tmk_barrier(0);

	gettimeofday(&start, NULL);

	if (first_row & 1)
		sor_first_row_odd(first_row, last_row);
	else
		sor_first_row_even(first_row, last_row);

	gettimeofday(&finish, NULL);

	printf("Elapsed time: %.2f seconds\n",
	       (((finish.tv_sec * 1000000.0) + finish.tv_usec) -
		((start.tv_sec * 1000000.0) + start.tv_usec)) / 1000000.0);

	Tmk_exit(0);
}
