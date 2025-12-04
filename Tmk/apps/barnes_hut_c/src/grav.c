#include <Tmk.h>

#define global extern
  
#include "code.h"

local bodyptr pskip;                  /* body to skip in force evaluation */
local vector pos0;                    /* point at which to evaluate field */
local real phi0;                      /* computed potential at pos0       */
local vector acc0;                    /* computed acceleration at pos0    */

local void gravsub(register nodeptr p);
local bool subdivp(register nodeptr p, real dsq);

void hackwalk(proced sub);
void walksub(register nodeptr p, real dsq);

/*
 * HACKGRAV: evaluate grav field at a given particle.
 */
  
void hackgrav(p)
  bodyptr p;
{
    pskip = p;                           /* exclude from force calc.         */
    SETV(pos0, Pos(p));                  /* eval force on this point         */
    phi0 = 0.0;                          /* init grav. potential and         */
    CLRV(acc0);                          /*  force accumulators              */
    myn2bterm = mynbcterm = 0;           /* zero the term counters           */
    skipself = FALSE;                    /* reset flag to look for p         */
    hackwalk(gravsub);                   /* recursively scan tree            */
    Phi(p) = phi0;                       /* stash resulting pot. and         */
    SETV(Acc(p), acc0);                  /* acceleration in body p           */
    Cost(p) = myn2bterm + mynbcterm;     /* update cost of particle          */
}



/*
 * GRAVSUB: compute a single body-body or body-cell interaction.
 */

local vector dr;                       /* data to be shared                 */
local real drsq;                       /* between gravsub and subdivp       */
local nodeptr pmem;

local void gravsub(p)
  register nodeptr p;               /* body or cell to interact with     */
{
    double sqrt();
    real drabs, phii, mor3;
    vector ai, quaddr;
    real dr5inv, phiquad, drquaddr;
    
    if (p != pmem) {                   /* cant use memorized data?          */
        SUBV(dr, Pos(p), pos0);        /*   then find seperation            */
        DOTVP(drsq, dr, dr);           /*   and square of distance          */
    }
    
    drsq += epssq;                             /* use standard softening   */

    drabs = sqrt((double) drsq);               /* find norm of distance    */
    phii = Mass(p) / drabs;                    /* and contribution to phi  */
    phi0 -= phii;                              /* add to total potential   */
    mor3 = phii / drsq;                        /* form mass / radius cubed */
    MULVS(ai, dr, mor3);                       /* and contribution to acc. */
    ADDV(acc0, acc0, ai);                      /* add to net acceleration  */
    if(Type(p) == CELL) {                      /* a body-cell interaction? */
        mynbcterm++;                           /*   count body-cell term   */
#ifdef QUADPOLE                                /*   add qpole correction   */
        dr5inv = 1.0/(drsq * drsq * drabs);    /*   form dr ** (-5)        */
        MULMV(quaddr, Quad(p), dr);            /*   form Q * dr            */
        DOTVP(drquaddr, dr, quaddr);           /*   form dr * Q * dr       */
        phiquad = -0.5 * dr5inv * drquaddr;    /*   quad. part of poten.   */
        phi0 = phi0 + phiquad;                 /*   increment potential    */
        phiquad = 5.0 * phiquad / drsq;        /*   save for acceleration  */
        MULVS(ai, dr, phiquad);                /*   components of acc.     */
        SUBV(acc0, acc0, ai);                  /*   increment              */
        MULVS(quaddr, quaddr, dr5inv);   
        SUBV(acc0, acc0, quaddr);              /*   acceleration           */
#endif
    }
    else                                       /* a body-body interaction  */
        myn2bterm++;                           /*   count body-body term   */
}



/*
 * HACKWALK: walk the tree opening cells too close to a given point.
 */

local proced hacksub;

void hackwalk(sub)
  proced sub;                                /* routine to do calculation */
{
    hacksub = sub;
    walksub((nodeptr) Global->G_root, Global->rsize * Global->rsize);
}

/*
 * WALKSUB: recursive routine to do hackwalk operation.
 */

void walksub(p, dsq)
  register nodeptr p;                        /* pointer into body-tree    */
  real dsq;                                  /* size of box squared       */
{
    register nodeptr *pp;
    register int k;
    
    if (subdivp(p, dsq))                     /* should p be opened?       */
        for (pp=Subp(p);pp<Subp(p)+NSUB;pp++) { /* loop over the subcells  */
            if (*pp != NULL)                    /*   does this one exist?  */
                walksub(*pp, dsq / 4.0);        /*     examine it in turn  */
        }
    else if (p != (nodeptr) pskip)             /* should p be included?    */
        (*hacksub)(p);                         /*   then use interaction   */
    else                                       /* self-interact'n skipped? */
        skipself = TRUE;                       /* note calculation is OK   */
}



/*
 * SUBDIVP: decide if a node should be opened.
 * Side effects: sets  pmem,dr, and drsq.
 */

local bool subdivp(p, dsq)
  register nodeptr p;                      /* body/cell to be tested    */
  real dsq;                                /* size of cell squared      */
{
    if (Type(p) == BODY)                     /* at tip of tree?           */
        return (FALSE);                      /*   then cant subdivide     */
    SUBV(dr, Pos(p), pos0);                  /* compute displacement      */
    DOTVP(drsq, dr, dr);                     /* and find dist squared     */
    pmem=p;                                  /* remember we know them     */
    return (tolsq * drsq < dsq);             /* use geometrical rule      */
}

