#include "stdinc.h"
#include <assert.h>

#include <sys/time.h>

/*
#ifndef SIMULATOR
#include <ulocks.h>
#endif
*/

#include "vectmath.h"

#define MAX_PROC 128
#ifdef USE_ORB
/* Arb. number of splits to eval */
#define NUMSPLITS  10
/* Work distribution percentile  */
#define HALF 0.5
/* range around p in which X falls */
#define WORK_TOL 0.003
#endif

/*
 * BODY and CELL data structures are used to represent the tree:
 *
 *         +-----------------------------------------------------------+
 * root--> | CELL: mass, pos, cost, quad, /, o, /, /, /, /, o, /, done |
 *         +---------------------------------|--------------|----------+
 *                                           |              |
 *    +--------------------------------------+              |
 *    |                                                     |
 *    |    +--------------------------------------+         |
 *    +--> | BODY: mass, pos, cost, vel, acc, phi |         |
 *         +--------------------------------------+         |
 *                                                          |
 *    +-----------------------------------------------------+
 *    |
 *    |    +-----------------------------------------------------------+
 *    +--> | CELL: mass, pos, cost, quad, o, /, /, o, /, /, o, /, done |
 *         +------------------------------|--------|--------|----------+
 *                                       etc      etc      etc
 */

/*
 * NODE: data common to BODY and CELL structures.
 */

typedef struct {
    short type;                 /* code for node type: body or cell */
    real mass;                  /* total mass of node */
    vector pos;                 /* position of node */
    int cost;                   /* number of interactions computed */
} node, *nodeptr;

#define Type(x) (((nodeptr) (x))->type)
#define Mass(x) (((nodeptr) (x))->mass)
#define Pos(x)  (((nodeptr) (x))->pos)
#define Cost(x) (((nodeptr) (x))->cost)

/*
 * BODY: data structure used to represent particles.
 */

#define BODY 01                 /* type code for bodies */

typedef struct {
    short type;
    real mass;                  /* mass of body */
    vector pos;                 /* position of body */
    int cost;                   /* number of interactions computed */
    vector vel;                 /* velocity of body */
    vector acc;                 /* acceleration of body */
    real phi;                   /* potential at body */
} body, *bodyptr;

#define Vel(x)  (((bodyptr) (x))->vel)
#define Acc(x)  (((bodyptr) (x))->acc)
#define Phi(x)  (((bodyptr) (x))->phi)

/*
 * CELL: structure used to represent internal nodes of tree.
 */

#define CELL 02                 /* type code for cells */

#define NSUB (1 << NDIM)        /* subcells per cell */

typedef struct {
    short type;
    real mass;                  /* total mass of cell */
    vector pos;                 /* cm. position of cell */
    int cost;                   /* number of interactions computed */
#ifdef QUADPOLE
    matrix quad;                /* quad. moment of cell */
#endif
    nodeptr subp[NSUB];         /* descendents of cell */
    volatile short int done;    /* flag to tell when the c.of.m is ready */
} cell, *cellptr;

#ifdef USE_ORB
struct orbtype {
	int currprocs[MAX_PROC];
	int channel;
	real currmin[NDIM];       /* min coords of curr proc subset */
	real currmax[NDIM];       /* min coords of curr proc subset */
	real X;
	real rx0,rx1;
    int CartDir;              /* 0 for x, 1 for y, 2 for z    */
	int GWork[NUMSPLITS];
    BARDEC(BarFindSplit)	/* barrier in Find_Split routine	   */
    BARDEC(BarSplitDir)	/* barrier in Find_Split routine	   */
	int TotalWork;
        real work_tol;
	struct orbtype *left;
	struct orbtype *right;
    int currsplit;
};

typedef struct orbtype  ORBNode, *ORBNodeptr;
#endif

#ifdef QUADPOLE
#define Quad(x) (((cellptr) (x))->quad)
#endif

#define Subp(x) (((cellptr) (x))->subp)
#define Done(x) (((cellptr) (x))->done)

/*
 * Integerized coordinates: used to mantain body-tree.
 */

#ifndef CRAY
#  define IMAX  (1 << (8 * sizeof(int) - 2))    /* highest bit of int coord */
#else
#  define IMAX  (1 << 22)                        /* Cray uses 24-bit ints    */
#endif

/*
 * Parameters and results for gravitational calculation.  Some others 
 * can be found in code.h
 */

global real fcells;             /* ratio of cells/bodies allocated          */

global real tol;                /* accuracy parameter: 0.0 => exact         */

global real tolsq;              /* square of previous                       */

global real eps;                /* potential softening parameter            */

global real epssq;              /* square of previous                       */

global int myn2bterm;     /* count body-body terms evaluated for this body  */

global int mynbcterm;     /* count body-cell terms evaluated for this body  */

global bool skipself;           /* true if self-interaction skipped OK      */
 
global int debug;               /* control debugging messages               */

global real dthf;               /* half time step                           */

global int workMin,workMax;     /* interval of cost to be treated by a proc */

global int maxcell,maxmybody;   /* maxcell = max number of cells  allocated */
		/* maxmybody = max no. of bodies allocated per processor */

/*********
#ifdef OWNEDCELLS
global int maxmycell;
#endif
********/

global int mywork;


/* 
 * Data structures for parallel version
 */
 
global vector min,max;           /* min and max of coordinates for each Proc. */
     
global cellptr ctab; /* array of cell used for the tree. size is maxcell cells*/
global cellptr allocctab;
                     /* array of cell used for the tree. size is maxcell cells*/

#ifdef USE_ORB
ORBNodeptr ORBtab; /* array of ORB nodes used in ORB tree, size is 2*NPROC */
#endif

global bodyptr bodytab;         /* array of bod. size is exactly nbody bodies */
global bodyptr allocbodytab;    /* array of bod. size is exactly nbody bodies */

#define MAXLOCK 500            /* maximum number of locks on Kittyhawk */

global struct CellLockType {
    int CL[MAXLOCK];            /* locks on the cells*/
    } *CellLock;
    

/* Struct defining global space that is written to only by one proc -rrk */
struct pgm {
    int n2bcalc;                /* total number of body/cell interactions  */
    int nbccalc;                /* total number of body/body interactions  */
    int selfint;                /* number of self interactions             */
    real mtot;                  /* total mass of N-body system             */
    real etot[3];               /* binding, kinetic, potential energy      */
    matrix keten;               /* kinetic energy tensor                   */
    matrix peten;               /* potential energy tensor                 */
    vector cmphase[2];          /* center of mass coordinates and velocity */
    vector amvec;               /* angular momentum vector                 */
    vector min;                 /* temporary lower-left corner of the box  */
    vector max;                 /* temporary upper right corner of the box */
};

typedef struct pgm privGlobalMemory;

struct GlobalMemory  {	/* all this info is for the whole system */
    int n2bcalc;                /* total number of body/cell interactions  */
    int nbccalc;                /* total number of body/body interactions  */
    int selfint;                /* number of self interactions             */
    real mtot;                  /* total mass of N-body system             */
    real etot[3];               /* binding, kinetic, potential energy      */
    matrix keten;               /* kinetic energy tensor                   */
    matrix peten;               /* potential energy tensor                 */
    vector cmphase[2];          /* center of mass coordinates and velocity */
    vector amvec;               /* angular momentum vector                 */
    cellptr G_root;             /* root of the whole tree                  */
    int ncell;                  /* number of cell actually used in tree    */
#ifdef USE_ORB
    ORBNodeptr ORB_root;	/* root of the ORB tree			   */
    int nORBcell;		/* number of ORB cells actually used       */
    float work_tolerance;
    int ORBNcellLock;
    bodyptr *Mybodystart[MAX_PROC];
    int Mynumbodies[MAX_PROC];
    int Gworklock;		/* lock to sum Gwork			   */
    int BarExchange;		/* barrier in Find_Split routine	   */
#endif
    vector rmin;                /* lower-left corner of coordinate box     */
    vector min;                 /* temporary lower-left corner of the box  */
    vector max;                 /* temporary upper right corner of the box */
    real rsize;                 /* side-length of integer coordinate box   */
    int body_loopLock;          /* distributed loop on the bodies for load */
    int Barbody_loop;
    int Subbody_loop;           /* Index of sub loop                       */
    int Countbody_loop;         /* Number of procs having finished the loop*/
    int CellLoopLock;           /* distributed loop on cells for hackcofm  */
    int BarCellLoop;
    int SubCellLoop;
    int CountCellLoop;
    int Barstart;               /* barrier at the beginning of stepsystem  */
    int Bartree;                /* barrier after loading the tree          */
    int Barcom;                 /* barrier after computing the c. of m.    */
    int Barload;                /*   */
    int Baraccel;               /* barrier after accel and before output   */
    int Barpos;                 /* barrier after computing the new pos     */
    int CountLock;              /* Lock on the shared variables            */
    int NcellLock;              /* Lock on the counter of array of cells for loadtree */
    int io_lock;
#ifdef TRACK 
    int Mywork[MAX_PROC+1];
    int Mynbody[MAX_PROC+1];
    int maxbodywork[MAX_PROC+1];
    int minbodywork[MAX_PROC+1];
    int sqbodywork[MAX_PROC+1];
#endif
    volatile int k; /*for memory allocation in code.C */
    };

extern struct GlobalMemory *Global;
extern privGlobalMemory *privGlobal[MAX_PROC];

