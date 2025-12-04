/*
 * $Id: gauss.c,v 10.12 1997/12/23 07:53:22 alc Exp $
 */
#include <stdio.h>
#include <sys/time.h>
#include <Tmk.h>

#define SWAP(a,b)       { float tmp; tmp = (a); (a) = (b); (b) = tmp; }
#define ABS(a)		(((a) > 0) ? (a) : -(a))	/* much faster than fabs */


struct GlobalMemory {
    float	**a;
    int		odd_sweep_pivot, even_sweep_pivot;
} *Global;


#define LOWERVAL 0.0
#define UPPERVAL 2.0


int CheckRow(row, rownum, size)
float *row;
int rownum;
int size; {

int     i;
int     errorcount = 0;
 
    for (i = 0; i < rownum; i++)
	if (row[i] != LOWERVAL)
	    errorcount++;
    for (i = rownum; i < size; i++)
	if (row[i] != UPPERVAL)
	    errorcount++;
    return errorcount;
}
 

int CheckArray(a, size)	/* check_array & check_row used to check res */
float **a;
int size; {

int i;
int errorcount = 0;
 
    for (i = 0; i < size; i++)
	errorcount += CheckRow(a[i], i, size);
    return errorcount;
}


void TransposeAndZero(a, size) 
float **a;
int size; {

int     i, j;
 
    for (i = 0; i < size; i++)
        for (j = 0; j < i; j++) {
            SWAP(a[i][j], a[j][i]);
            a[i][j] = 0;
        }
}


void InitArray(a, size)
float **a;
int size; {

int i, j;

#define fILEREAD
#ifdef FILEREAD
    for (i = 0; i < size; i++)
	for (j = 0; j < size; j++)
	    scanf("%f", &a[i][j]);
#else
    for (i = Tmk_proc_id; i < size; i += Tmk_nprocs) {
	for (j = 0; j < i; j++)
	    a[i][j] = j * 2 + 2;
	for (j = i; j < size; j++)
	    a[i][j] = i * 2 + 2;
    }
#endif
}


void PrintArray(a, size)
float **a;
int size; {

int i, j;
FILE *fd;

	fd = fopen("res", "w");
	printf("\n");
	for (i = 0; i < size; i++) {
	    for (j = 0; j < size; j++)
		fprintf(fd, "%f   ", a[i][j]);
	    fprintf(fd, "\n");
	}
	printf("\n");
}


void Compute(a, size, iters)
float **a;
int size;
int iters; {

float	factor;

int	curr_pivot, pivot_col, j, k;
int	starting_row = Tmk_proc_id;

    for (curr_pivot = 0; curr_pivot < iters; curr_pivot++) {

	if (starting_row == curr_pivot) {	/* Find pivot element */
	    float max = ABS(a[curr_pivot][curr_pivot]);
	    pivot_col = curr_pivot;

	    for (j = curr_pivot+1; j < size; j++) {
		float abs_j = ABS(a[curr_pivot][j]);
		if (max < abs_j) {
		    pivot_col = j;
		    max = abs_j;
		}
	    }
		
	    /* Place the pivot element on the diagonal */
	    SWAP(a[curr_pivot][curr_pivot], a[curr_pivot][pivot_col]);
	    if (curr_pivot & 0x01)		/* Odd sweep */
		Global->odd_sweep_pivot = pivot_col;
	    else
		Global->even_sweep_pivot = pivot_col;
	    for (j = curr_pivot+1; j < size; j++)
		a[curr_pivot][j] /= a[curr_pivot][curr_pivot];
		
	    starting_row += Tmk_nprocs;
	}

	Tmk_barrier(0);

	pivot_col = (curr_pivot & 0x01) ? Global->odd_sweep_pivot : 
						Global->even_sweep_pivot;

	for (k = starting_row; k < size; k += Tmk_nprocs) {
	    SWAP(a[k][curr_pivot], a[k][pivot_col]);
	    for (j = curr_pivot + 1; j < size; j++) {
		a[k][j] -= a[k][curr_pivot] * a[curr_pivot][j];
	    }
	}
    }

    Tmk_barrier(0);
}


void main(argc, argv)
int argc;
char **argv; {

int c, i, iters = 0, j, size = 1024;
extern char *optarg;
struct timeval start, finish;

    while ((c = getopt(argc, argv, "i:r:")) != -1) {
	switch (c) {
	  case 'i':
	    iters = atoi(optarg);
	    break;
	  case 'r':
	    size = atoi(optarg);
	    break;
	}
    }

    Tmk_startup(argc, argv);

    printf("Gaussian elimination on %d by %d using %d processors\n",
	size, size, Tmk_nprocs);

    if (iters == 0)
	iters = size - 1;
    else {
	if ((iters < 1) || (iters >= size)) {
	    printf("\tIllegal value, %d, to \"-i\" option.\n",
		   iters);

	    iters = size - 1;
	}
	printf("\tHalting after %d elimination steps.\n",
	       iters);
    }

    if (!Tmk_proc_id) {
	Global = (struct GlobalMemory *) Tmk_malloc(sizeof(struct GlobalMemory));
	Global->a = (float **) Tmk_malloc(size*sizeof(float *));

	for (j = 0; j < Tmk_nprocs; j++) {	/* Wacky init for distinct cachelines*/
	    for (i = j; i < size; i += Tmk_nprocs) {
		Global->a[i] = (float *) Tmk_malloc(size*sizeof(float));
	    }
	}
	Tmk_distribute((char *)&Global, sizeof(Global));
    }
    Tmk_barrier(0);

    InitArray(Global->a, size);

    Tmk_barrier(0);

    gettimeofday(&start, NULL);

    Compute(Global->a, size, iters);

    gettimeofday(&finish, NULL);

    if (!Tmk_proc_id) {
	printf("Elapsed time: %.3f seconds\n\n",
               (((finish.tv_sec * 1000000.0) + finish.tv_usec) -
                ((start.tv_sec * 1000000.0) + start.tv_usec)) / 1000000.0);

	if (iters == size - 1) {
	    TransposeAndZero(Global->a, size);
	    if ((i = CheckArray(Global->a, size)) != 0)
		printf("ERRORs IN RESULT! %d errors found\n", i);
	}
	else
	    printf("\tSkipping verification step.\n");
    }
    Tmk_exit(0);
}
