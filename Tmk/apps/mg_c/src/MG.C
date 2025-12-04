/*
 * $Id: mg.c,v 10.5 1997/12/20 23:54:48 alc Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#if	defined(__sun)
#if   ! defined(__GNUC__)
#include <sunmath.h>
#else
double	aint(double);
#endif
#else
double	aint(double x)
{
	return (x > 0.0) ? floor(x) : ceil(x);
}
#endif

#include <sys/time.h>

#include <Tmk.h>

static double half23, half46, two23, two46;

#define M 10
static int    ind1[M][2], ind2[M][2], ind3[M][2];

/* undefined by rrk */
#define LM  6
#define NM  ((1 << LM) + 2)  /* 2**LM + 2 */  
#define NIT 20

double *u[LM], *r[LM];
int     local[LM] = {0};

static void init_rand()
{
  int i;

  half23 = half46 = two23 = two46 = 1.0;
  for (i = 23; i > 0; i--)
    half23 *= 0.5, half46 *= 0.5 * 0.5, two23 *= 2.0, two46 *= 2.0 * 2.0;
}  

static double  randlc(double *x, double a)
{
  double a1, a2, b1, b2, t1, t2, t3, t4, t5;

  a1 = aint(half23 * a);
  a2 = a - two23 * a1;
  b1 = aint(half23 * *x);
  b2 = *x - two23 * b1;
  t1 = a1 * b2 + a2 * b1;
  t2 = aint(half23 * t1); 
  t3 = t1 - two23 * t2;
  t4 = two23 * t3 + a2 * b2;
  t5 = aint(half46 * t4);
  *x = t4 - two46 * t5;

  return (half46 * *x);
}

/*
 * setup   must be called before mg3P 
 * computes the sizes of smaller grids and stores them in mm
 */
static void setup(lm, mm)
     int lm, *mm;
{
  int k;

  mm[lm-1] = 2 + (1 << lm);
  for (k = lm-1; k > 0; k--)
    mm[k-1] = 1 + mm[k]/2;
}


static void zero3(z, n)
     double *z;
     int     n;
{
  memset(z, 0, n*n*n*sizeof(double));
}


/*
 * bubble	does a bubble sort in direction dir
 */
static void bubble(ten, i1, i2, i3, m, ind)
     double ten[][2];
     int    i1[][2], i2[][2], i3[][2];
     int    m, ind;
{
  double dir = (ind == 1 ? 1. : -1.), temp;
  int    i, i_temp;

  for (i = 0; i < m-1; i++)
    {
      if (dir * ten[i][ind] > dir * ten[i+1][ind])
	{
	  temp = ten[i+1][ind];
	  ten[i+1][ind] = ten[i][ind];
	  ten[i][ind] = temp;

	  i_temp = i1[i+1][ind];
	  i1[i+1][ind] = i1[i][ind];
	  i1[i][ind] = i_temp;

	  i_temp = i2[i+1][ind];
	  i2[i+1][ind] = i2[i][ind];
	  i2[i][ind] = i_temp;

	  i_temp = i3[i+1][ind];
	  i3[i+1][ind] = i3[i][ind];
	  i3[i][ind] = i_temp;
	}
      else
	return;
    }
}

/* zran3  
 *      loads +1 at ten randomly chosen points,
 *      loads -1 at a different ten random points,
 *      and zero elsewhere.
 */
static void zran3(n)
     int     n;
{
#define A 1220703125.0

  int     i, i1, i2, i3; 
  double  ten[M][2];
  double  x = 314159265., tmp; 

  init_rand();

  for (i = 0; i < M; i++)
    {
      ten[i][1] = 0.;
      ten[i][0] = 1.;
    }

  for (i3 = 1; i3 < n-1; i3++)
    {
      for (i2 = 1; i2 < n-1; i2++)
	{
	  for (i1 = 1; i1 < n-1; i1++)
	    {
	      tmp = randlc(&x, A);
	      if (tmp > ten[0][1])
		{
		  ten[0][1] = tmp;
		  ind1[0][1] = i1;
		  ind2[0][1] = i2;
		  ind3[0][1] = i3;
		  bubble(ten, ind1, ind2, ind3, M, 1);
		}
	      if (tmp < ten[0][0])
		{
		  ten[0][0] = tmp;
		  ind1[0][0] = i1;
		  ind2[0][0] = i2;
		  ind3[0][0] = i3;
		  bubble(ten, ind1, ind2, ind3, M, 0);
		}
	    }
	}
    }
  /*
  printf("\nnegative charges at\n");
  for (i = 0; i < M; i++)
    printf("(%3d,%3d,%3d)\n", 1+ind1[i][0], 1+ind2[i][0], 1+ind3[i][0]);
  
  printf("\npositive charges at\n");
  for (i = 0; i < M; i++)
    printf("(%3d,%3d,%3d)\n", 1+ind1[i][1], 1+ind2[i][1], 1+ind3[i][1]);
  */
}


/*
 * global_comm3 communicates on all borders of an array
 */
static void  global_comm3(u, n)
     double  *u;
     int      n;
{
  double *u1, *u2, *u3;
  int     i1, i2, i3;
  int     nsq  = n*n;
  int     off1, off2;
  int     start, end;

  start = Tmk_proc_id * ((n-2) / Tmk_nprocs) + 1;
  if (Tmk_proc_id == 0) start = 0;
  end = (Tmk_proc_id + 1) * ((n-2) / Tmk_nprocs) + 1;
  if (Tmk_proc_id == Tmk_nprocs-1) end = n;
  
  if (Tmk_proc_id == 0)
    {
      off2 = (n-2) * nsq;
      for (i2 = 1, u2 = u + n; i2 < n-1; i2++, u2 += n)
	{
	  for (i3 = 1, u3 = u2 + 1; i3 < n-1; i3++, u3++)
	    {
	      *(u3       ) = *(u3 + off2);
	    }
	}
    }

  if (Tmk_proc_id == Tmk_nprocs-1)
    {
      off1 = (n-1) * nsq;
      for (i2 = 1, u2 = u + n; i2 < n-1; i2++, u2 += n)
	{
	  for (i3 = 1, u3 = u2 + 1; i3 < n-1; i3++, u3++)
	    {
	      *(u3 + off1) = *(u3 + nsq );
	    }
	}
    }

  /* Tmk_barrier(0); */

  off1 = nsq  - n;		/* (n-1) * n */
  off2 = off1 - n; 
  for (i1 = start, u1 = u + start * nsq; i1 < end; i1++, u1 += nsq)
    {
      for (i3 = 1, u3 = u1 + 1; i3 < n-1; i3++, u3++)
	{
	  *(u3       ) = *(u3 + off2);
	  *(u3 + off1) = *(u3 + n   );
	}
    }

  /* Tmk_barrier(0); */

  off1 = n    - 1;
  off2 = off1 - 1;
  for (i1 = start, u1 = u + start * nsq; i1 < end; i1++, u1 += nsq)
    {
      for (i2 = 0, u2 = u1; i2 < n; i2++, u2 += n)
	{
	  *(u2       ) = *(u2 + off2);
	  *(u2 + off1) = *(u2 + 1   );
	}
    }

  Tmk_barrier(0);
}


/*
 * comm3 communicates on all borders of an array
 */
static void  comm3(u, n)
     double  *u;
     int      n;
{
  double *u1, *u2, *u3;
  int     i1, i2, i3;
  int     nsq  = n*n;
  int     off1, off2;;

  off1 = (n-1) * nsq;
  off2 = off1  - nsq;
  for (i2 = 1, u2 = u + n; i2 < n-1; i2++, u2 += n)
    {
      for (i3 = 1, u3 = u2 + 1; i3 < n-1; i3++, u3++)
	{
	  *(u3       ) = *(u3 + off2);
	  *(u3 + off1) = *(u3 + nsq );
	}
    }

  off1 = nsq  - n; /* (n-1) * n */
  off2 = off1 - n; 
  for (i1 = 0, u1 = u; i1 < n; i1++, u1 += nsq)
    {
      for (i3 = 1, u3 = u1 + 1; i3 < n-1; i3++, u3++)
	{
	  *(u3       ) = *(u3 + off2);
	  *(u3 + off1) = *(u3 + n   );
	}
    }

  off1 = n    - 1;
  off2 = off1 - 1;
  for (i1 = 0, u1 = u; i1 < n; i1++, u1 += nsq)
    {
      for (i2 = 0, u2 = u1; i2 < n; i2++, u2 += n)
	{
	  *(u2       ) = *(u2 + off2);
	  *(u2 + off1) = *(u2 + 1   );
	}
    }
}


static void print_big_array(u, n, name)
     double *u;
     int     n;
     char   *name;
{
  double *u1, *u2, *u3;
  int     i1, i2, i3, nsq = n * n;
  
  for (i1 = 0, u1 = u; i1 < n; i1++, u1 += nsq)
    {
      for (i2 = 0, u2 = u1; i2 < n; i2++, u2 += n)
	{
	  for (i3 = 0, u3 = u2; i3 < n; i3++, u3++)
	    {
	      fprintf(stderr, "%s(%d, %d, %d) = %f\n", name, i1, i2, i3, *u3);
	    }
	}
    }
}

static void print_small_array(u, n, name)
     double *u;
     int     n;
     char   *name;
{
  double *u1, *u2, *u3;
  int     i1, i2, i3, nsq = n * n;
  
  for (i1 = 1, u1 = u + nsq; i1 < n-1; i1++, u1 += nsq)
    {
      for (i2 = 1, u2 = u1 + n; i2 < n-1; i2++, u2 += n)
	{
	  for (i3 = 1, u3 = u2 + 1; i3 < n-1; i3++, u3++)
	    {
	      fprintf(stderr, "%s(%d, %d, %d) = %f\n", 
		      name, i1-1, i2-1, i3-1, *u3);
	    }
	}
    }
}

/*
 * resid computes the residual:  r = v - Au
 */
static void resid(u, r, n, a)
     double *u, *r;
     int     n;
     double *a;
{
  double *u1, *u2, *u3;
  int     i, i1, i2, i3, nsq = n * n;
  int     r_offset = r - u;
  int     start, end;

  start = Tmk_proc_id * ((n-2) / Tmk_nprocs) + 1;
  end = (Tmk_proc_id + 1) * ((n-2) / Tmk_nprocs) + 1;
  
  for (i1 = start, u1 = u + start * nsq; i1 < end; i1++, u1 += nsq)
    {
      for (i2 = 1, u2 = u1 + n; i2 < n-1; i2++, u2 += n)
	{
	  for (i3 = 1, u3 = u2 + 1; i3 < n-1; i3++, u3++)
	    {
	      *(u3 + r_offset) =
		- a[0] * *u3
		- a[1] * (  *(u3 - nsq) + *(u3 + nsq)
			  + *(u3 - n  ) + *(u3 + n  )
			  + *(u3 - 1  ) + *(u3 + 1  ))
		- a[2] * (  *(u3 - nsq - n) + *(u3 + nsq - n)
			  + *(u3 - nsq + n) + *(u3 + nsq + n)
			  + *(u3 - n   - 1) + *(u3 + n   - 1)
			  + *(u3 - n   + 1) + *(u3 + n   + 1)
			  + *(u3 - nsq - 1) + *(u3 - nsq + 1)
			  + *(u3 + nsq - 1) + *(u3 + nsq + 1))
		- a[3] * (  *(u3 - nsq - n - 1) + *(u3 + nsq - n - 1)
			  + *(u3 - nsq + n - 1) + *(u3 + nsq + n - 1)
			  + *(u3 - nsq - n + 1) + *(u3 + nsq - n + 1)
			  + *(u3 - nsq + n + 1) + *(u3 + nsq + n + 1));
	    }
	}
    }

  for (i = 0; i < M; i++)
    {
      if (start <= ind1[i][0] && ind1[i][0] < end)
	{
	  *(r + ind1[i][0]*nsq + ind2[i][0]*n + ind3[i][0]) += -1.;
	}
      if (start <= ind1[i][1] && ind1[i][1] < end)
	{
	  *(r + ind1[i][1]*nsq + ind2[i][1]*n + ind3[i][1]) +=  1.;
	}
    }

  Tmk_barrier(0);

  global_comm3(r, n);
}


/*
 * self_resid computes the residual:  r = r - Au
 */
static void self_resid(u, r, n, a, local)
     double *u, *r;
     int     n;
     double *a;
     int     local;
{
  double *u1, *u2, *u3;
  int     i1, i2, i3, nsq = n * n;
  int     r_offset = r - u;
  int     start, end;

  if (local)
    {
      start = 1; 
      end = n - 1;
    }
  else
    {
      start = Tmk_proc_id * ((n-2) / Tmk_nprocs) + 1;
      end = (Tmk_proc_id + 1) * ((n-2) / Tmk_nprocs) + 1;
    }

  for (i1 = start, u1 = u + start * nsq; i1 < end; i1++, u1 += nsq)
    {
      for (i2 = 1, u2 = u1 + n; i2 < n-1; i2++, u2 += n)
	{
	  for (i3 = 1, u3 = u2 + 1; i3 < n-1; i3++, u3++)
	    {
	      *(u3 + r_offset) -=
		+ a[0] * *u3
		+ a[1] * (  *(u3 - nsq) + *(u3 + nsq)
			  + *(u3 - n  ) + *(u3 + n  )
			  + *(u3 - 1  ) + *(u3 + 1  ))
		+ a[2] * (  *(u3 - nsq - n) + *(u3 + nsq - n)
			  + *(u3 - nsq + n) + *(u3 + nsq + n)
			  + *(u3 - n   - 1) + *(u3 + n   - 1)
			  + *(u3 - n   + 1) + *(u3 + n   + 1)
			  + *(u3 - nsq - 1) + *(u3 - nsq + 1)
			  + *(u3 + nsq - 1) + *(u3 + nsq + 1))
		+ a[3] * (  *(u3 - nsq - n - 1) + *(u3 + nsq - n - 1)
			  + *(u3 - nsq + n - 1) + *(u3 + nsq + n - 1)
			  + *(u3 - nsq - n + 1) + *(u3 + nsq - n + 1)
			  + *(u3 - nsq + n + 1) + *(u3 + nsq + n + 1));
	    }
	}
    }

  if (local)
    {
      comm3(r, n);
    }
  else
    {
      Tmk_barrier(0);
      global_comm3(r, n);
    }
}


/*
 * init_resid computes the initial residual:  r = v
 * This function is called only once and always globally!
 */
static void init_resid(r, n)
     double *r;
     int     n;
{
  int i, nsq = n*n;
  int start, end;

  start = Tmk_proc_id * ((n-2) / Tmk_nprocs) + 1;
  if (Tmk_proc_id == 0) start = 0;
  end = (Tmk_proc_id + 1) * ((n-2) / Tmk_nprocs) + 1;
  if (Tmk_proc_id == Tmk_nprocs-1) end = n;
  
  memset(r + start*nsq, 0, (end-start)*nsq*sizeof(double));

  for (i = 0; i < M; i++)
    {
      if (start <= ind1[i][0] && ind1[i][0] < end)
	{
	  *(r + ind1[i][0]*nsq + ind2[i][0]*n + ind3[i][0]) = -1.;
	}
      if (start <= ind1[i][1] && ind1[i][1] < end)
	{
	  *(r + ind1[i][1]*nsq + ind2[i][1]*n + ind3[i][1]) =  1.;
	}
    }

  Tmk_barrier(0);

  global_comm3(r, n);
}


/*
 ****** psinv applies an approximate inverse as smoother:  u = u + Cr
 *
 *      This simple implementation costs  27A + 4M per result, where
 *      A and M denote the costs of Addition and Multiplication.  
 *      By using several two-dimensional buffers one can reduce this
 *      cost to  13A + 4M in the general case, or  11A + 3M when the
 *      coefficient c(3) is zero.
 */
static void psinv(r, u, n, c, local)
     double *r, *u;
     int     n;
     double *c;
     int     local;
{
  double *r1, *r2, *r3;
  int     i1, i2, i3, nsq = n * n;
  int     u_offset = u - r;
  int     start, end;

  if (local)
    {
      start = 1; 
      end = n - 1;
    }
  else
    {
      start = Tmk_proc_id * ((n-2) / Tmk_nprocs) + 1;
      end = (Tmk_proc_id + 1) * ((n-2) / Tmk_nprocs) + 1;
    }

  for (i1 = start, r1 = r + start * nsq; i1 < end; i1++, r1 += nsq)
    {
      for (i2 = 1, r2 = r1 + n; i2 < n-1; i2++, r2 += n)
	{
	  for (i3 = 1, r3 = r2 + 1; i3 < n-1; i3++, r3++)
	    {
	      *(r3 + u_offset) += 
		  c[0] * *r3
		+ c[1] * (  *(r3 - nsq) + *(r3 + nsq)
			  + *(r3 - n  ) + *(r3 + n  )
			  + *(r3 - 1  ) + *(r3 + 1  ))
		+ c[2] * (  *(r3 - nsq - n) + *(r3 + nsq - n)
			  + *(r3 - nsq + n) + *(r3 + nsq + n)
			  + *(r3 - n   - 1) + *(r3 + n   - 1)
			  + *(r3 - n   + 1) + *(r3 + n   + 1)
			  + *(r3 - nsq - 1) + *(r3 - nsq + 1)
			  + *(r3 + nsq - 1) + *(r3 + nsq + 1))
		+ c[3] * (  *(r3 - nsq - n - 1) + *(r3 + nsq - n - 1)
			  + *(r3 - nsq + n - 1) + *(r3 + nsq + n - 1)
			  + *(r3 - nsq - n + 1) + *(r3 + nsq - n + 1)
			  + *(r3 - nsq + n + 1) + *(r3 + nsq + n + 1));
	    }
	}
    }

  if (local)
    {
      comm3(u, n);
    }
  else
    {
      Tmk_barrier(0);
      global_comm3(u, n);
    }
}


/*
 * rprj3 projects onto the next coarser grid, 
 * using a trilinear Finite Element projection:  s = r' = P r
 */
static void rprj3(r, mk, s, mj, local)
     double *r;
     int     mk;
     double *s;
     int     mj;
     int     local;
{
  double *s1, *s2, *s3, *r1, *r2, *r3;
  int     j1,  j2,  j3;
  int     mjsq    = mj * mj;
  int     mksq    = mk * mk;
  int     twomk   = mk + mk;
  int     twomksq = mksq + mksq;
  int     start, end;

  if (local)
    {
      start = 1; 
      end = mj - 1;
    }
  else
    {
      start = Tmk_proc_id * ((mj-2) / Tmk_nprocs) + 1;
      end = (Tmk_proc_id + 1) * ((mj-2) / Tmk_nprocs) + 1;
    }

  for (j1 = start, s1 = s + start * mjsq, r1 = r + start * twomksq; 
       j1 < end; 
       j1++, s1 += mjsq, r1 += twomksq)
    {
      for (j2 = 1, s2 = s1 + mj, r2 = r1 + twomk; 
	   j2 < mj - 1; 
	   j2++, s2 += mj, r2 += twomk)
	{
	  for (j3 = 1, s3 = s2 + 1, r3 = r2 + 2; 
	       j3 < mj - 1; 
	       j3++, s3++, r3 += 2)
	    {
	      *s3 = 
		  0.5    *    *r3
		+ 0.25   * (  *(r3 - mksq) + *(r3 + mksq)
			    + *(r3 - mk  ) + *(r3 + mk  )
			    + *(r3 - 1  ) + *(r3 + 1  ))
		+ 0.125  * (  *(r3 - mksq - mk) + *(r3 + mksq - mk)
			    + *(r3 - mksq + mk) + *(r3 + mksq + mk)
			    + *(r3 - mk   - 1) + *(r3 + mk   - 1)
			    + *(r3 - mk   + 1) + *(r3 + mk   + 1)
			    + *(r3 - mksq - 1) + *(r3 - mksq + 1)
			    + *(r3 + mksq - 1) + *(r3 + mksq + 1))
		+ 0.0625 * (  *(r3 - mksq - mk - 1) + *(r3 + mksq - mk - 1)
			    + *(r3 - mksq + mk - 1) + *(r3 + mksq + mk - 1)
			    + *(r3 - mksq - mk + 1) + *(r3 + mksq - mk + 1)
			    + *(r3 - mksq + mk + 1) + *(r3 + mksq + mk + 1));
	    }
	}
    }

  if (local)
    {
      comm3(s, mj);
    }
  else
    {
      Tmk_barrier(0);
      global_comm3(s, mj);
    }
}
       
/*
 * interp adds the trilinear interpolation of the correction
 * from the coarser grid to the current approximation:  u = u + Qu'
 */
static void interp(z, m, u, n, local)
     double *z;
     int     m;
     double *u;
     int     n;
     int     local;
{
  double *u1, *u2, *u3, *z1, *z2, *z3;
  int     i1,  i2,  i3;
  int     msq    = m * m;
  int     nsq    = n * n;
  int     twon   = n + n;
  int     twonsq = nsq + nsq;
  int     start, end;

  if (local)
    {
      start = 1; 
      end = m - 1;
    }
  else
    {
      start = Tmk_proc_id * ((m-2) / Tmk_nprocs) + 1;
      end = (Tmk_proc_id + 1) * ((m-2) / Tmk_nprocs) + 1;
    }

  for (i1 = start, z1 = z + start * msq, u1 = u + start * twonsq; 
       i1 < end; 
       i1++, z1 += msq, u1 += twonsq)
    {
      for (i2 = 1, z2 = z1 + m, u2 = u1 + twon; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2; 
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 += 0.5 *(*z3 + *(z3 - 1));
	      *++u3 += *z3;
	    }
	}

      for (i2 = 1, z2 = z1 + m, u2 = u1 + n; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2;
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 += 0.25 * (*z3 + *(z3 - 1) + *(z3 - m) + *(z3 - m - 1));
	      *++u3 += 0.5 *  (*z3 + *(z3 - m));
	    }
	}
    }

  for (i1 = start, z1 = z + start * msq, u1 = u + start * twonsq - nsq; 
       i1 < end; 
       i1++, z1 += msq, u1 += twonsq)
    {
      for (i2 = 1, z2 = z1 + m, u2 = u1 + twon; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2;
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 += 0.25 * (*z3 + *(z3 - 1) + *(z3 - msq) + *(z3 - msq - 1));
	      *++u3 += 0.5 *  (*z3 + *(z3 - msq));
	    }
	}

      for (i2 = 1, z2 = z1 + m, u2 = u1 + n; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2; 
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 += 0.125 * (*z3 + *(z3 - 1) + *(z3 - m) + *(z3 - m - 1)
			      + *(z3 - msq) + *(z3 - msq - m) + *(z3 - msq - 1)
			      + *(z3 - msq - m - 1));
	      *++u3 += 0.25 * (*z3 + *(z3 - m) + *(z3 - msq) + *(z3 - msq - m));
	    }
	}
    }

  if (local)
    {
      comm3(u, n);
    }
  else
    {
      Tmk_barrier(0);
      global_comm3(u, n);
    }
}


/*
 * direct_interp adds the trilinear interpolation of the correction
 * from the coarser grid to the current approximation:  u = Qu'
 */
static void direct_interp(z, m, u, n, local)
     double *z;
     int     m;
     double *u;
     int     n;
     int     local;
{
  double *u1, *u2, *u3, *z1, *z2, *z3;
  int     i1,  i2,  i3;
  int     msq    = m * m;
  int     nsq    = n * n;
  int     twon   = n + n;
  int     twonsq = nsq + nsq;
  int     start, end;

  if (local)
    {
      start = 1; 
      end = m - 1;
    }
  else
    {
      start = Tmk_proc_id * ((m-2) / Tmk_nprocs) + 1;
      end = (Tmk_proc_id + 1) * ((m-2) / Tmk_nprocs) + 1;
    }

  for (i1 = start, z1 = z + start * msq, u1 = u + start * twonsq; 
       i1 < end; 
       i1++, z1 += msq, u1 += twonsq)
    {
      for (i2 = 1, z2 = z1 + m, u2 = u1 + twon; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2; 
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 = 0.5 *(*z3 + *(z3 - 1));
	      *++u3 = *z3;
	    }
	}

      for (i2 = 1, z2 = z1 + m, u2 = u1 + n; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2;
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 = 0.25 * (*z3 + *(z3 - 1) + *(z3 - m) + *(z3 - m - 1));
	      *++u3 = 0.5  * (*z3 + *(z3 - m));
	    }
	}
    }

  for (i1 = start, z1 = z + start * msq, u1 = u + start * twonsq - nsq; 
       i1 < end; 
       i1++, z1 += msq, u1 += twonsq)
    {
      for (i2 = 1, z2 = z1 + m, u2 = u1 + twon; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2;
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 = 0.25 * (*z3 + *(z3 - 1) + *(z3 - msq) + *(z3 - msq - 1));
	      *++u3 = 0.5 *  (*z3 + *(z3 - msq));
	    }
	}

      for (i2 = 1, z2 = z1 + m, u2 = u1 + n; 
	   i2 < m - 1; 
	   i2++, z2 += m, u2 += twon)
	{
	  for (i3 = 1, z3 = z2 + 1, u3 = u2; 
	       i3 < m - 1; 
	       i3++, z3++)
	    {
	      *++u3 = 0.125 * ( *z3 + *(z3 - 1) + *(z3 - m) + *(z3 - m - 1)
			      + *(z3 - msq) + *(z3 - msq - m) + *(z3 - msq - 1)
			      + *(z3 - msq - m - 1));
	      *++u3 = 0.25 * (*z3 + *(z3 - m) + *(z3 - msq) + *(z3 - msq - m));
	    }
	}
    }

  if (local)
    {
      comm3(u, n);
    }
  else
    {
      Tmk_barrier(0);
      global_comm3(u, n);
    }
}


/*
 * norm2u3 evaluates approximations to the L2 norm and the
 * uniform (or L-infinity or Chebyshev) norm, under the
 * assumption that the boundaries are periodic or zero.  Add the
 * boundaries in with half weight (quarter weight on the edges
 * and eigth weight at the corners) for inhomogeneous boundaries.
 */
static void norm2u3(r, rnm2, rnmu, n)
     double *r;
     double *rnm2;
     double *rnmu;
     int     n;
{
  double  a, sum = 0.0;
  double *r1, *r2, *r3;
  int     i1,  i2,  i3;
  int     nsq = n * n;

  *rnmu = 0.0;
  for (i1 = 1, r1 = r + nsq; i1 < n-1; i1++, r1 += nsq)
    {
      for (i2 = 1, r2 = r1 + n; i2 < n-1; i2++, r2 += n)
	{
	  for (i3 = 1, r3 = r2 + 1; i3 < n-1; i3++, r3++)
	    {
	      sum += (*r3 * *r3);
	      if ((a = fabs(*r3)) > *rnmu)
		*rnmu = a;
	    }
	}
    }
  *rnm2 = sqrt(sum / (double)((n-2)*(n-2)*(n-2)));
}

/*
 *  mg3P   implements a simple, constant coefficient version of the
 *         MG algorithm FAPIN (fast approximate inverse)
 */
static void mg3p(u, r, n, a, c, mm, lm)
     double *u[], *r[];
     int     n;
     double *a, *c;
     int    *mm;
     int     lm;
{
  int k;

  for (k = lm-1; k > 0; k--)
    {
      rprj3(r[k], mm[k], r[k-1], mm[k-1], local[k-1]);
    }

  zero3(u[0], mm[0]);
  psinv(r[0], u[0], mm[0], c, 1); /* local */

  for (k = 1; k < lm-1; k++)
    {
      direct_interp(u[k-1], mm[k-1], u[k], mm[k], local[k]);
      self_resid(u[k], r[k], mm[k], a, local[k]);
      psinv(r[k], u[k], mm[k], c, local[k]);
    }

  interp(u[lm-2], mm[lm-2], u[lm-1], n, 0); /* global */
  resid(u[lm-1], r[lm-1], n, a);     /* always global */
  psinv(r[lm-1], u[lm-1], n, c, 0);         /* global */
}


/*
 * main program for benchmark B
 */
main(argc, argv)
     int    argc;
     char **argv;
{
  double a[4] = {-8./3., 0., 1./6., 1./12.};
  double c[4] = {-3./8., 1./32., -1./64., 0.};
  double rnm2, rnmu, old2, oldu, elapsed;

  int    mm[LM], i, n = NM;

  struct timeval time1, time2;

  while ((i = getopt(argc, argv, "")) != -1);
  Tmk_startup(argc, argv);

  if (Tmk_proc_id == 0)
    {
      printf("Kernel B:  Solving a Poisson problem on a %3d by %3d by %3d grid, using %1d multigrid iterations\n",
	     n-2, n-2, n-2, NIT);
    }

  zran3(n);
  setup(LM, mm);
  local[0] = 1;
  local[1] = 1;
  local[2] = 1;

  if (Tmk_proc_id == 0)
    {
      for (i = LM-1; i >= 0; i--)
	{
	  if (local[i])
	    {
	      if (!(u[i]=(double *)malloc(mm[i]*mm[i]*mm[i] * sizeof(double))))
		Tmk_errexit("NOT ENOUGH MEMORY!");
	      if (!(r[i]=(double *)malloc(mm[i]*mm[i]*mm[i] * sizeof(double))))
		Tmk_errexit("NOT ENOUGH MEMORY!");
	    }
	  else
	    {
	      if (!(u[i]=(double *)Tmk_sbrk(mm[i]*mm[i]*mm[i]*sizeof(double))))
		Tmk_errexit("NOT ENOUGH SHARED MEMORY!\n");
              /* 
	      if (i == LM-1) memset(u[i], 0, mm[i]*mm[i]*mm[i]*sizeof(double));
	      */
	      Tmk_distribute((char *)&u[i], sizeof(u[i]));

	      if (!(r[i]=(double *)Tmk_sbrk(mm[i]*mm[i]* mm[i]*sizeof(double))))
		Tmk_errexit("NOT ENOUGH SHARED MEMORY!");

	      Tmk_distribute((char *)&r[i], sizeof(r[i]));
	    }
	}
    }

  else
    { 
      freopen("/tmp/nn", "w", stderr); setbuf(stderr, NULL);
      for (i = LM-1; i >= 0; i--)
	{
	  if (local[i])
	    {
	      if (!(u[i]=(double *)malloc(mm[i]*mm[i]*mm[i] * sizeof(double))))
		Tmk_errexit("NOT ENOUGH MEMORY!");
	      if (!(r[i]=(double *)malloc(mm[i]*mm[i]*mm[i] * sizeof(double))))
		Tmk_errexit("NOT ENOUGH MEMORY!");
	    }
	}     
    }

  Tmk_barrier(0);

  gettimeofday(&time1, 0);

  init_resid(r[LM-1], n); /* always global */
  
  for (i = 0; i < NIT; i++)
    {
      mg3p(u, r, n, a, c, mm, LM);
      resid(u[LM-1], r[LM-1], n, a);
    }

  Tmk_barrier(0);

  gettimeofday(&time2, 0);

  if (Tmk_proc_id == 0)
    {
      norm2u3(r[LM-1], &rnm2, &rnmu, n);
    }

  elapsed = (time2.tv_sec + time2.tv_usec*1e-6) - 
	    (time1.tv_sec + time1.tv_usec*1e-6);

  printf("\niter\tL2-norm(R)\t\tunif-norm(R)\n");
  printf("%4d\t%.7g\t\t%.7g\n", NIT, rnm2, rnmu);
  printf("Time for solve = %10.2f\ngiving MFLOP %10.2f\n",
	 elapsed, (33 + NIT * 111.14)*(n-2)*(n-2)*(n-2)*1e-6/elapsed);

  Tmk_exit(0);
}
