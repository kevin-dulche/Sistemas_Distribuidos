#include "defs.h"

global string headline;                     /* message describing calculation */

global string infile;                       /* file name for snapshot input   */

global string outfile;                      /* file name for snapshot output  */

global real dtime;                        /* timestep for leapfrog integrator */

global real dtout;                          /* time between data outputs      */

global real tstop;                          /* time to stop calculation       */

global int nbody;                           /* number of bodies in system     */

global int mynbody;            /* number of bodies allocated to the processor */

global bodyptr *mybodytab;    /* array of bodies allocated to each processor  */

#ifdef OWNEDCELLS
global int myncell;           /* number of cells allocated to the processor */
global cellptr *mycelltab;    /* array of cellptrs allocated to each processor */
#endif

global real tnow;                         /* current value of simulation time */

global real tout;                           /* time next output is due        */

global int nstep;                       /* number of integration steps so far */

global int myn2bcalc;      /* body-body force calculations for each processor */

global int mynbccalc;      /* body-cell force calculations for each processor */

global int myselfint;           /* count self-interactions for each processor */


