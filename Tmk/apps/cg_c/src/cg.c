/*
 * $Id: cg.c,v 10.3.0.1 1998/09/05 20:45:27 alc Exp $
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <sys/time.h>

#include <Tmk.h>

#define LARGE
#define FULL

#define  niter 15
#define  nitcg 25

#ifdef SMALL

#define  nn 1400       
#define  nnp1 1401     
#define  nnz 100300    
#define  lenwrk 101701 
#define  ilnwrk 203401 
#define  NONZER 7       

#elif defined (MEDIUM)

#define  nn 3000
#define  nnp1 3001
#define  nnz 400000
#define  lenwrk 404001 
#define  ilnwrk 808001 
#define  NONZER 9      

#elif defined (LARGE)

#define  nn 14000
#define  nnp1 14001
#define  nnz 2030000
#define  lenwrk 2044001
#define  ilnwrk 4088001
#define  NONZER 11

#else
#error Must define one of SMALL,MEDIUM,LARGE
#endif

/******* Prototypes definitions */
void    vecset(int n, double v[], int iv[], int *nzv, int i, double val);
void    sprnvc(int n, int nz, double v[], int iv[], int nzloc[], int mark[]);
void    sparse(double a[], int rowidx[], int colstr[],  int n, 
	       int arow[], int acol[],   double aelt[], double x[],
	       int mark[], int nzloc[],  int nnza);
int     icnvrt (double x, int ipwr2);
double  myint (double x);
void    cgsol(int n,      double a[], int rowidx[], int colstr[],
	      double b[], double x[], double *resid, int Nitcg,
	      double r[], double p[], double q[]);
void    makea (int n,         int nz,       double a[], int rowidx[], 
	       int colstr[],  double rcond, int arow[], int acol[],
	       double aelt[], double v[],   int iv[]);
double  ddot(int n, double x[], double y[]);
void    daxpy(double y[], double x[], double a, int n);
void    daypx(double y[], double x[], double a, int n);
void    matvec(int n, double a[], int rowidx[], int colstr[], 
	       double x[], double y[]);
void    init_rand();
double  randlc(double *x, double a);
void    main (int argc, char **argv);

/******* Globals */
struct  timeval t0, t1, ti, td;
double  totdot=0.0, totmat;
double  amult, rcond = 0.1, tran;
double  r23, r46, t23, t46;
int     startn, endn;
double  *shared_ddotr; 
double  *shared_zeta;
double  z[nn+1], r[nn+1], p[nn+1], q[nn+1];
double  *shared_q;
double  work[lenwrk+1];
double  a[nnz+1], new_a[nnz+1], invidx[nn+1], x[nn+1];
int     colstr[nnp1+1], rowidx[nnz+1], new_row[nnp1+1], new_col[nnz+1], ptr_a;
int     iwork[ilnwrk+1];
int     my_colstr[nnp1+1+1], my_rowidx[nnz+1];
double  my_a[nnz+1];

/****** Formats converted to c */
char m10000[]="\n\n****** starting APP benchmark kernel C\n";
char m11000[]="\n%5d    %12.4e    %20.14f";
char m12000[]="\ntotal number of operations        = %16.4f\ntotal execution time              = %16.4f\nperformance in mflops             = %16.4f\nratio: this machine / ymp(1cpu)   = %16.4f\n";
char m13000[]="\n******* kernel failed accuracy check      ****\n******* number of nonzeros does not match ****\ncomputed number of nonzeros       = %16d\ncorrect  number of nonzeros       = %16d\n";
char m14000[]="\n******* comparison for accuracy check ****\ncomputed number of nonzeros       = %16d\ncorrect  number of nonzeros       = %16d\n";
char m15000[]="\ncomputed zeta                     = %14.5e\nreference value                   = %14.5e";
char m16000[]="\ncomputed residual                 = %14.5e\nreference value                   = %14.5e";
char m17000[]="\n\n****** finishing APP benchmark kernel C\n";



int
icnvrt (double x, int ipwr2)
{
    return (int) ipwr2 * x;
}


void
vecset (int n, double v[], int iv[], int *nzv, int i, double val)
{
    int   k, set;

    set = 0;
    for (k = 1; k <= *nzv; k++) {
	if (iv[k] == i) {
	    v[k] = val;
	    set  = 1;
	}
    }

    if (set == 0) {
	*nzv     = *nzv + 1;
	v[*nzv]  = val;
	iv[*nzv] = i;
    }
}


void
init_rand()
{
    int i;

    r23 = r46 = t23 = t46 = 1.0;
    for (i = 23; i > 0; i--)
    {
	r23 *= 0.5;
	t23 *= 2.0;
    }
    for (i = 46; i > 0; i--)
    {
	r46 *= 0.5;
	t46 *= 2.0;
    }
}  

double  myint (double x)
{
  if (x >= 0.0)
    return floor(x);
  else
    return ceil (x);
}

double  randlc(double *x, double a)
{
  double a1, a2, x1, x2, t1, t3, t4;

  a1 = myint(r23 * a);
  a2 = a - t23 * a1;

  x1 = myint(r23 * *x);
  x2 = *x - t23 * x1;
  t1 = a1 * x2 + a2 * x1;
  t3 = t23 * (t1 - t23 * myint(r23 * t1)) + a2 * x2;
  t4 = myint(r46 * t3);
  *x = t3 - t46 * t4;

  return (r46 * *x);
}


void    sprnvc(int n,     int nz,      double v[], 
	       int iv[],  int nzloc[], int mark[])
{
  int    nn1;
  int    nzcol, nzv, ii, i;
  double vecelt, vecloc;

  for (i = 1; i <= n; i++)
    mark[i] = 0;
  
  nzv = 0;
  nzcol = 0;
  nn1 = 1;
 L50:
  nn1 = 2 * nn1;
  if (nn1 < n) goto L50;

 L100:
  if (nzv >= nz) goto L110;
  vecelt = randlc(&tran, amult);

  vecloc = randlc(&tran, amult);
  i = icnvrt(vecloc, nn1) + 1;
  if (i > n) goto L100;

  if (mark[i] == 0) {
    mark[i] = 1;
    nzcol = nzcol + 1;
    nzloc[nzcol] = i;
    nzv = nzv + 1;
    v[nzv] = vecelt;
    iv[nzv] = i;
  }
  goto L100;
 L110:
  for (ii = 1; ii <= nzcol; ii++) {
    i = nzloc[ii];
    mark[i] = 0;
  }
}

void   sparse(double a[], int rowidx[], int colstr[],  int n, 
	      int arow[], int acol[],   double aelt[], double x[],
	      int mark[], int nzloc[],  int nnza)
{
  int     i, j, jajp1, nza, k, nzcol;
  double  xi;

  for (j = 1; j <= n; j++) {
    colstr[j] = 0;
    mark[j] = 0;
  }
  colstr[n+1] = 0;
  
  for (nza = 1; nza <= nnza; nza++) {
    j = acol[nza] + 1;
    colstr[j] = colstr[j] + 1;
  }
  
  colstr[1] = 1;
  for (j = 2; j <= n+1; j++)
    colstr[j] = colstr[j] + colstr[j-1];
  

  for (nza = 1; nza <= nnza; nza++) {
    if (nza % 1000 == 0)
      if (Tmk_proc_id == 0)
	printf (".");
    j = acol[nza];
    k = colstr[j];
    a[k] = aelt[nza];
    rowidx[k] = arow[nza];
    colstr[j] = colstr[j] + 1;
  }

  for (j = n; j >= 1; j--)
    colstr[j+1] = colstr[j];
  colstr[1] = 1;
  
  nza = 0;
  for (i = 1; i <= n; i++) {
    x[i]    = 0.0 ;
    mark[i] = 0;
  }
  
  jajp1 = colstr[1];
  for (j = 1; j <= n; j++) {
    nzcol = 0;

    if (j % 100 == 0)
      if (Tmk_proc_id == 0)
	printf ("@");
    for (k = jajp1; k <= colstr[j+1]-1; k++) {
      i = rowidx[k];
      x[i] = x[i] + a[k];
      if ( (mark[i] == 0) && (x[i] != 0.e0)) {
	mark[i] = 1;
	nzcol = nzcol + 1;
	nzloc[nzcol] = i;
      }
    }

    for (k = 1; k <= nzcol; k++) {
      i = nzloc[k];
      mark[i] = 0;
      xi = x[i];
      x[i] = 0.e0;
      if (xi != 0.e0) {
	nza = nza + 1;
	a[nza] = xi;
	rowidx[nza] = i;
      }
    }
    jajp1 = colstr[j+1];
    colstr[j+1] = nza + colstr[1];
  }
  if (Tmk_proc_id == 0)
    fprintf (stdout, "\n\n\nfinal nonzero count in sparse \nnumber of nonzeros       = %16d", nza);
}


void    makea (int n,         int nz,       double a[], int rowidx[], 
	       int colstr[],  double rcond, int arow[], int acol[],
	       double aelt[], double v[],   int iv[])
{
  int     i, nnza, iouter, ivelt, ivelt1, jcol, nzv;
  double  size, ratio, scale;

  size = 1.0e0;
  ratio = pow (rcond, (1.0e0/n));
  nnza = 0;
  
  for (iouter = 1; iouter <= n; iouter++) {
    if (iouter % 100 == 0)
      if (Tmk_proc_id == 0)
	printf ("%d ", iouter);
    nzv = NONZER;
    sprnvc(n, nzv, v, iv, &rowidx[1], &rowidx[n+1]);
    vecset(n, v, iv, &nzv, iouter, .5e0);
    for (ivelt = 1; ivelt <= nzv; ivelt++) {
      jcol = iv[ivelt];
      scale = size * v[ivelt];
      for (ivelt1 = 1; ivelt1 <= nzv; ivelt1++) {
        nnza = nnza + 1;
        arow[nnza] = iv[ivelt1];
        acol[nnza] = jcol;
        aelt[nnza] = v[ivelt1] * scale;
      }
    }
    size = size * ratio;
  }
  if (Tmk_proc_id == 0)
    printf ("\n");
  for (i = 1; i <= n; i++) {
    nnza = nnza + 1;
    arow[nnza] = i;
    acol[nnza] = i;
    aelt[nnza] = rcond;
  }
  sparse(a, rowidx, colstr, n, arow, acol, aelt, v, &iv[1], &iv[n+1], nnza);
  
}


double  ddot(int n, double x[], double y[])
{
  int i;
  double ddotr;

/* Initialize local and global results */
  ddotr = 0.e0;

/* Compute local(partial) results */
  for (i = startn; i <= endn; i++)
    ddotr = ddotr + x[i] * y[i];

/* Compute global result */

  shared_ddotr[Tmk_proc_id] = ddotr;

  Tmk_barrier(0);

  ddotr = 0;
  for (i = 0; i < Tmk_nprocs; i++)
      ddotr += shared_ddotr[i];

  return ddotr;
}


void    daxpy(double y[], double x[], double a, int n)
{ 
  int i;
  for (i = startn; i <= endn; i++) 
    y[i] = y[i] + a * x[i];
}


void    daypx(double y[], double x[], double a, int n)
{     
  int i;
  for (i = startn; i <= endn; i++)
    y[i] = a * y[i] + x[i];
}


void    matvec(int n, double a[], int rowidx[], 
	       int colstr[], double x[], double y[])
{    /* y = a * x.  a is a sparse matrix rep in a, rowidx, colstr form */
  int    j, k;

  memcpy (&shared_q[startn], &x[startn], (endn-startn+1) * sizeof(double));

  Tmk_barrier(0);

  memcpy (&x[1], &shared_q[1], n * sizeof(double));

  for (j = startn; j <= endn; j++)
    y[j] = 0.0;
  for (j = 1; j <= nn; j++) {
    for (k = my_colstr[j]; k < my_colstr[j+1]; k++)
    {
      y[my_rowidx[k]] += my_a[k] * x[j];
      /* printf ("y[%8d] = %21.17f\n", rowidx[k], y[rowidx[k]]); */
    }
  }
}


void    cgsol(int n,      double a[], int rowidx[], int colstr[],
	      double b[], double x[], double *resid, int Nitcg,
	      double r[], double p[], double q[])
{
  int     i, iter;
  double  alpha, beta, rho, rho0;
 
  for (i = startn; i <= endn; i++) {
    x[i] = 0.e0;
    r[i] = b[i];
    p[i] = b[i];
  }

  rho = ddot(n, r, r);

  for (iter = 1; iter <= Nitcg; iter++) {
    matvec(n, a, rowidx, colstr, p, q);
    alpha = rho / ddot(n, p, q);
    rho0 = rho;
    daxpy(x, p, alpha, n);
    daxpy(r, q, -alpha, n);
    rho = ddot(n, r, r);
    beta = rho / rho0;
    daypx(p, r, beta, n);
  }
  matvec(n, a, rowidx, colstr, x, r);

  for (i = startn; i <= endn; i++) 
    r[i] = b[i] - r[i];
  
  *resid =  sqrt(ddot(n, r, r));
}


void    main (int argc, char **argv)
{
  double    resid, zeta, zeta1, zeta2, zetapr;
  double    ops, opscg, time, flops, ratio;
  
  int       i, j, k, it, matops, nnzcom;
#ifndef LARGE
#ifndef SMALL
  int       nnzchk = 268754;             
  double    zetchk = 0.10188582986104e0; 
  double    zettol = 1.e-10;
  double    reschk = 0.6241e-5;           
  double    timchk = 1.0538;             
#else
  int       nnzchk = 78148;             
  double    zetchk = 0.10188582986104e0; 
  double    zettol = 1.e-10;
  double    reschk = 0.6241e-5;           
  double    timchk = 1.0538;             
#endif
#else
  int       nnzchk = 1853104;             
  double    zetchk = 1.01249586035172e-1; 
  double    zettol = 1.e-10;
  double    reschk = 3.5094e-4;           
  double    timchk = 21.77;              
#endif
               
  int             c;


/* Initialize seed for random number generator */
  while ((c = getopt(argc, argv, "")) != -1)
    switch (c) 
      {
      }
  Tmk_startup(argc, argv);
  init_rand ();
  tran    = 314159265.0e0;
  amult   = 1220703125.0e0;
  zeta    = randlc (&tran, amult);

/* Initialize distributed memory */
  if (Tmk_proc_id == 0)
    {
      shared_q = (double *) Tmk_sbrk((nn+1) * sizeof(double));
      Tmk_distribute((char *)&shared_q, sizeof(shared_q));
      shared_ddotr = (double *) Tmk_sbrk(Tmk_page_size);
      shared_ddotr = (double *) Tmk_sbrk(sizeof(double)*Tmk_nprocs);
      Tmk_distribute((char *)&shared_ddotr, sizeof(shared_ddotr));
      shared_zeta = (double *) Tmk_sbrk(Tmk_page_size);
      shared_zeta = (double *) Tmk_sbrk(Tmk_nprocs*sizeof(double));
      Tmk_distribute((char *)&shared_zeta, sizeof(shared_zeta));
    }
    else {
	/* Include these if you want stderr from other nodes to go to a file */
#if 0
	char buf[32];
	sprintf (buf, "err.%d", Tmk_proc_id);
	freopen(buf, "w", stderr); setbuf(stderr, NULL);
#endif
    }
  startn = Tmk_proc_id * (nn / Tmk_nprocs) + 1;
  endn   = (Tmk_proc_id + 1) * (nn / Tmk_nprocs);
  if (Tmk_proc_id == Tmk_nprocs-1) endn = nn;
  
  makea (nn, nnz, a, rowidx, colstr, rcond, &iwork[1], &iwork[nnz+1], 
	 &work[1], &work[nnz+1], &iwork[2*nnz+1]);

  {
    int    aidx = 1;

    my_colstr[1] = 1;
    for (j = 1; j <= nn; j++)
    {
        for (k = colstr[j]; k < colstr[j+1]; k++)
	{
	    if (rowidx[k] < startn || rowidx[k] > endn) continue;

	    my_a[aidx] = a[k];
	    my_rowidx[aidx] = rowidx[k];
	    aidx++;
	}
	my_colstr[j+1] = aidx;
    }
  }


  for (i= 1; i<= nn; i++)
    x[i] = 1.0e0;

  if (Tmk_proc_id == 0)
    fprintf (stdout, m10000);

  ops = 0.0;
  Tmk_barrier(0);

  gettimeofday(&t0, NULL);
  
  zeta2 = 0.e0;
  zeta1 = 0.e0;
  zeta  = 0.e0;    

  for (it = 1; it <= niter; it++) { 
    cgsol(nn, a, rowidx, colstr, x, z, &resid, nitcg, r, p, q);

    zeta2 = zeta1;
    zeta1 = zeta;
    zeta = 0.0e0;

    for (i = startn; i <= endn; i++)
      if (zeta < fabs(z[i]))
	zeta = fabs(z[i]);
    shared_zeta[Tmk_proc_id] = zeta;

    Tmk_barrier(0);

    for (i = 0; i < Tmk_nprocs; i++)
	if (zeta < shared_zeta[i]) zeta = shared_zeta[i];

    zeta = 1. / zeta;

    zetapr = zeta - ( (zeta - zeta1)*(zeta - zeta1) 
		     / (zeta - 2.e0*zeta1 + zeta2) );
    if (Tmk_proc_id == 0)
      fprintf(stdout, m11000, it, resid, zetapr);
    for (i = startn; i <= endn; i++)
      x[i] = z[i] * zeta;
  }

  gettimeofday(&t1, NULL);  
  
  nnzcom = colstr[nn+1] - colstr[1];
  matops = 2 * nnzcom ;
  opscg  = 5.0*nn + matops + nitcg*(matops + 10.0*nn + 2.0);
  ops    = niter  * (opscg + nn + 1.0); 
  time   = ((t1.tv_sec - t0.tv_sec)*1.0e6 + (t1.tv_usec-t0.tv_usec))/1.0e6;
  flops  =  1.0e-6 * ops / time;
  ratio  =  timchk / time;
  if (Tmk_proc_id == 0)
    fprintf (stdout, m12000, ops, time, flops, ratio);

  if ( nnzchk != nnzcom )
    if (Tmk_proc_id == 0)
      fprintf (stdout, m13000, nnzcom, nnzchk);
  
  if (Tmk_proc_id == 0) {
    fprintf (stdout, m14000, nnzcom, nnzchk);
    fprintf (stdout, m15000, zetapr, zetchk);
    fprintf (stdout, m16000, resid , reschk);
    fprintf (stdout, m17000);
  }
  Tmk_exit (0);
}
