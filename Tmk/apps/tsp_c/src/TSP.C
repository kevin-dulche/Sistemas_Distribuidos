#include <stdio.h>

#include <sys/time.h>

#include <Tmk.h>

#include "tsp.h"

#define SILENT

/* BEGIN SHARED DATA */
GlobalMemory	*global = NULL;
/* END SHARED DATA */


int 		StartNode, TspSize, NodesFromEnd;
int 		dump, debug, debugPrioQ;
int		performance;
extern int	visitNodes;

void
usage()
{
fprintf(stderr, "tsp:\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n",
		"-s: start node",
		"-f: size (filename)", "-r: size to recursively solve",
		"-d: debug mode", "-D: debug the priority queue",
		"-S: run silently");
}

extern char    *optarg;

main(argc,argv)
unsigned argc;
char **argv;
{
    int 		c, i, silent;
    char 	       *fname;
    struct timeval	start, end;

    TspSize = MAX_TOUR_SIZE;
    NodesFromEnd = 12;
    dump = 0;
    fname = "18";
#ifdef SILENT
    silent = 1;
#endif /*SILENT*/

    while ((c = getopt(argc, argv, "df:pr:s:DSh")) != -1)
	switch (c) {
	case 'd':
		debug++;
		printf("Debugging on.\n");
		break;
	case 'f':
		fname = optarg;
		break;
	case 'p':
		performance = 1;
		break;
	case 'r':
		sscanf(optarg, "%d", &NodesFromEnd);
		break;
	case 's':
		StartNode = atoi(optarg);
		printf("Starting at node %d\n",StartNode);
		break;
	case 'D':
		debugPrioQ++;
		printf("Debugging prioQ.\n");
		break;
	case 'S':
		silent++;
		printf("Running silent.\n");
		break;
	case '?':
	case 'h':
		usage();
		exit(0);
	}

    Tmk_startup(argc, argv);

    if (Tmk_proc_id == 0) {
	if ((global = (GlobalMemory *) Tmk_malloc(sizeof(GlobalMemory))) == 0) {
	    fprintf(stderr, "Unable to alloc shared memory\n");
	    exit(-1);
	}
	memset(global, 0, sizeof(*global));
	global->TourStackTop = -1;
	global->MinTourLen = BIGINT;
    	global->TourLock = 1;
	global->MinLock = 2;
	global->barrier = 0;

	/* Read in the data file with graph description. */
	TspSize = read_tsp(fname);

	Tmk_distribute((char *)&TspSize, sizeof(TspSize));

	/* Initialize the first tour. */
	global->Tours[0].prefix[0] = StartNode;
	global->Tours[0].conn = 1;
	global->Tours[0].last = 0;
	global->Tours[0].prefix_weight = 0;
	calc_bound(0);			/* Sets lower_bound. */

	/* Initialize the priority queue structures. */
	global->PrioQ[1].index = 0;	/* The first PrioQ entry is Tour 0. */
	global->PrioQ[1].priority = global->Tours[0].lower_bound;
	global->PrioQLast = 1;

	/* Put all the unused tours in the free tour stack. */
	for (i = MAX_NUM_TOURS - 1; i > 0; i--)
    	    global->TourStack[++global->TourStackTop] = i;

	Tmk_distribute((char *)&global, sizeof(global));
    }
    else {
	/*freopen("/tmp/err.1", "a", stderr); setbuf(stderr, NULL);*/
    }
    Tmk_barrier(0);

    gettimeofday(&start, NULL);

    Worker();

    gettimeofday(&end, NULL);

    fprintf(stderr, "Elapsed time: %.2f seconds\n",
	    (((end.tv_sec * 1000000.0) + end.tv_usec) -
	     ((start.tv_sec * 1000000.0) + start.tv_usec)) / 1000000.0);

    fprintf(stderr, "\n-----------------\n\n");

    fprintf(stderr, "\t[MINIMUM TOUR LENGTH: %d]\n\n", global->MinTourLen);
    fprintf(stderr, "MINIMUM TOUR:\n");
	for (i = 0; i < TspSize; i++) {
	    if (i % 10) fprintf(stderr, " - ");
	    else fprintf(stderr, "\n\t");
	    fprintf(stderr, "%2d", (int) global->MinTour[i]);
	}
    fprintf(stderr, " - %2d\n\n", StartNode);

    Tmk_exit(0);
}

void
Worker()
{
    int curr = -1;

    /* Wait for signal to start. */
    Tmk_barrier(global->barrier);

    for (;;) {
	/* Continuously get a tour and split. */
	curr = get_tour(curr);
	if (curr < 0) {
	    break;
	}
	recursive_solve(curr);
    }
    Tmk_barrier(global->barrier);	/* Signal completion. */
}
