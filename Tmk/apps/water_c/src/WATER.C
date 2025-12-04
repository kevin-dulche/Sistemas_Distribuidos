#include <Tmk.h>

#include "stdio.h"
#include <sys/time.h>
#include "comments.h"	/* moves leading comments to another file */
#include "split.h"

/*  include files for declarations  */
#define extern
#include "parameters.h"
#include "mdvar.h"
#include "water.h"
#include "wwpot.h"
#include "cnst.h"
#include "mddata.h"
#include "fileio.h"
#include "frcnst.h"
#include "randno.h"
#include "global.h"
#undef extern

struct GlobalMemory *gl;        /* pointer to the Global Memory
                                structure, which contains the lock,
                                barrier, and some scalar variables */
                                

int NSTEP, NSAVE, NRST, NPRINT,NFMC;
int NORD1;
int II;                         /*  variables explained in common.h */
int i;
int NDATA;
int   NFRST=11;
int  NFSV=10;
int  LKT=0;

int StartMol[MAXPROCS+1];       /* number of the first molecule
                                   to be handled by this process; used
                                   for static scheduling     */ 
int MolsPerProc;                /* number of mols per processor */ 

extern char	*optarg;

main(argc, argv)
char **argv;
{
    FILE *fp;
    char *input_file = "sample.in";
    int mol, pid, func, c, dir, atom, tsteps = 0;
    double XTT, MDMAIN();
    double VIR = 0.0;
    struct timeval start, finish;

    /* default values for the control parameters of the driver */
    /* are in parameters.h */

    six = stderr;
    nfmc = fopen("LWI12","r"); /* input file for particle
                                  displacements */

    TEMP  =298.0;
    RHO   =0.9980;
    CUTOFF=0.0;

    while ((c = getopt(argc, argv, "i:t:")) != -1)
	switch (c) {
	case 'i':
	    input_file = optarg;
	    break;
	case 't':
	    tsteps = atoi(optarg);
	    break;
	}

   /* READ INPUT */

    /*
     *   TSTEP = time interval between steps
     *   NMOL  = number of molecules to be simulated
     *   NSTEP = number of time-steps to be simulated
     *   NORDER = order of the predictor-corrector method. 6 by default
     *   NSAVE = frequency of data saving.  -1 by default
     *   NRST  = no longer used
     *   NPRINT = frequency (in time-steps) of computing pot. energy and
     *            writing output file.  setting it larger than NSTEP
     *            means not doing any I/O until the very end.
     *   NFMC = file number to read initial displacements from.  set to 
     *          0 if program should generate a regular lattice initially.
     *   Tmk_nprocs = number of processors to be used.
     */
    if (!(fp = fopen(input_file,"r"))) {
	fprintf(stderr, "Unable to open '%s'\n", input_file);
	exit(-1);
    }
    fscanf(fp, "%lf%d%d%d%d%d%d%d%d",&TSTEP, &NMOL, &NSTEP, &NORDER, 
	   &NSAVE, &NRST, &NPRINT, &NFMC,&Tmk_nprocs);
    if (tsteps) NSTEP = tsteps;
    if (NMOL > MAXMOLS) {
	fprintf(stderr, "Lock array in global.H has size %d < %d (NMOL)\n",
							MAXMOLS, NMOL);
	exit(-1);
    }
    Tmk_startup(argc, argv);

    printf("Using %d procs on %d steps of %d mols\n", Tmk_nprocs, NSTEP, NMOL);

    /* SET UP SCALING FACTORS AND CONSTANTS */

    NORD1=NORDER+1;

    CNSTNT(NORD1,TLC);  /* sub. call to set up constants */

    fprintf(six,"\nTEMPERATURE                = %8.2f K\n",TEMP);
    fprintf(six,"DENSITY                    = %8.5f G/C.C.\n",RHO);
    fprintf(six,"NUMBER OF MOLECULES        = %8d\n",NMOL);
    fprintf(six,"NUMBER OF PROCESSORS       = %8d\n",Tmk_nprocs);
    fprintf(six,"TIME STEP                  = %8.2e SEC\n",TSTEP);
    fprintf(six,"ORDER USED TO SOLVE F=MA   = %8d \n",NORDER);
    fprintf(six,"NO. OF TIME STEPS          = %8d \n",NSTEP);
    fprintf(six,"FREQUENCY OF DATA SAVING   = %8d \n",NSAVE);
    fprintf(six,"FREQUENCY TO WRITE RST FILE= %8d \n",NRST);

    if (Tmk_proc_id == 0) { /* Do memory initializations */
	int kk;
	unsigned mol_size = sizeof(molecule_type) * NMOL;
	unsigned gmem_size = sizeof(struct GlobalMemory);

      /* allocate space for main (VAR) data structure as well as
           synchronization variables */
	VAR = (molecule_type *) Tmk_malloc(mol_size);
	gl = (struct GlobalMemory *) Tmk_malloc(gmem_size);

	Tmk_distribute((char *)&VAR, sizeof(VAR));
	Tmk_distribute((char *)&gl, sizeof(gl));

             /* macro calls to initialize synch varibles  */

	gl->start = 0;
	gl->InterfBar = 1;
	gl->PotengBar = 2;
        gl->IntrafVirLock = 1;
        gl->InterfVirLock = 2;
        gl->FXLock = 3;
        gl->FYLock = 4;
        gl->FZLock = 5;

	for (kk = 0; kk < NMOL; kk++)
		gl->MolLock[kk] = kk + 8;

	gl->KinetiSumLock = 6;
	gl->PotengSumLock = 7;
      }
    Tmk_barrier(0);

    /* set up control for static scheduling */

    MolsPerProc = NMOL/Tmk_nprocs;
    StartMol[0] = 0;
    for (pid = 1; pid < Tmk_nprocs; pid += 1) {
      /*StartMol[pid] = StartMol[pid-1] + MolsPerProc;*/
	StartMol[pid] = (pid*NMOL)/Tmk_nprocs;
    }
    StartMol[Tmk_nprocs] = NMOL;

    SYSCNS();    /* sub. call to initialize system constants  */

    fprintf(six,"SPHERICAL CUTOFF RADIUS    = %8.4f ANGSTROM\n",CUTOFF);
    fflush(six);

    IRST=0;

            /* if there is no input displacement file, and we are to
               initialize to a regular lattice */
    if (NFMC == 0) {
	fclose(nfmc);
	nfmc = NULL;
    }

            /* initialization routine; also reads displacements and
             sets up random velocities*/
    if (Tmk_proc_id == 0)
      INITIA(nfmc);

    /*.......ESTIMATE ACCELERATION FROM F/M */
    if (Tmk_proc_id == 0) {       
            /* note that these initial calls to the force-computing 
               routines  (INTRAF and INTERF) use only 1 process since 
               others haven't been created yet */
    int tmp_procs = Tmk_nprocs;
    int tmp_smol = StartMol[1];
    StartMol[0] = 0;
    StartMol[1] = NMOL;
    Tmk_nprocs = 1;
    INTRAF(&VIR);
    INTERF(ACC,&VIR);
    StartMol[1] = tmp_smol;
    Tmk_nprocs = tmp_procs;
    }  /* Acclerations estimated */

    NFMC= -1;

    /*.....START MOLECULAR DYNAMIC LOOP */
    if (NFMC < 0) {
  	ELPST=0.00;
        TKIN=0.00;
        TVIR=0.00;
        TTMV=0.00;
    };
    if (NSAVE > 0)  /* not true for input decks provided */
      fprintf(six,"COLLECTING X AND V DATA AT EVERY %4d TIME STEPS \n",NSAVE);

            /* call routine to do the timesteps */
    gettimeofday(&start, NULL);
    XTT = MDMAIN(NFSV,NFRST,NSTEP,NRST,NPRINT,NSAVE,LKT,NORD1); 
    gettimeofday(&finish, NULL);

    fprintf(stderr, "\nExited Happily with XTT %g\n", XTT);
    fprintf(stderr, "Elapsed time: %.2f seconds\n",
	    (((finish.tv_sec * 1000000.0) + finish.tv_usec) -
	     ((start.tv_sec * 1000000.0) + start.tv_usec)) / 1000000.0);

    Tmk_exit(0);

} /* main.c */
