/*
 * $Id: stdinc.h,v 1.3 1997/12/21 04:41:15 alc Exp $
 */

#include <stdlib.h>

/*
 *
 */

#include <string.h>

/*
 * If not already loaded, include stdio.h.
 */

#ifndef FILE
#  include <stdio.h>
#endif

/*
 * STREAM: a replacement for FILE *.
 */

typedef FILE *stream;

/*
 * NULL: denotes a pointer to no object.
 */

#ifndef NULL
#  define NULL 0
#endif

/*
 * BOOL, TRUE and FALSE: standard names for logical values.
 */

typedef short int bool;

#ifndef TRUE

#  define FALSE 0
#  define TRUE  1

#endif

/*
 * BYTE: a short name for a handy chunk of bits.
 */

typedef unsigned char byte;

/*
 * STRING: for null-terminated strings which are not taken apart.
 */

typedef char *string;

/*
 * REAL: default type is double; if single precision calculation is
 * supported and desired, compile with -DSINGLEPREC.
 */

#ifndef  SINGLEPREC
  typedef  double  real, *realptr;
#else
  typedef  float   real, *realptr;
#endif

/*
 * VOID: type specifier used to declare procedures called for side-effect
 * only.  Note: this slightly kinky substitution is used to so that one
 * need not declare something to be void before using it.
 */
#if 0
#define void int
#endif
/*
 * PROC, IPROC, RPROC: pointers to procedures, integer functions, and
 * real-valued functions, respectively.
 */

typedef void (*proced)();
typedef int (*iproc)();
typedef real (*rproc)();

/*
 * LOCAL: declare something to be local to a file.
 * PERMANENT: declare something to be permanent data within a function.
 */

#define local     static
#define permanent static

/*
 * STREQ: handy string-equality macro.
 */

#define streq(x,y) (strcmp((x), (y)) == 0)

/*
 *  PI, etc.  --  mathematical constants
 */
#if ! defined(PI)
#define   PI         3.14159265358979323846
#endif
#define   TWO_PI     6.28318530717958647693
#define   FOUR_PI   12.56637061435917295385
#define   HALF_PI    1.57079632679489661923
#define   FRTHRD_PI  4.18879020478639098462

/*
 *  ABS: returns the absolute value of its argument
 *  MAX: returns the argument with the highest value
 *  MIN: returns the argument with the lowest value
 */

#define   ABS(x)       (((x) < 0) ? -(x) : (x))

/*
 * Used by the barrier
 */
#if ! defined(ALL_PROCS)
#define ALL_PROCS 0
#endif
