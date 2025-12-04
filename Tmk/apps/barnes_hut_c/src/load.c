#include <Tmk.h>

#define global extern

#include "code.h"

local bool intcoord();
local cellptr makecell();
local void loadtree(bodyptr p);
local int subindex(int x[NDIM], int l);
local void hackcofm(int nc);

local int k;

#if	DEBUG
extern FILE *junk;
#endif

/*
 * MAETREE: initialize tree structure for hack force calculation.
 */
void maketree(btab, nbody)
  bodyptr btab;			/* array of bodies to build into tree */
  int nbody;			    /* number of bodies in above array */
{
  int i;
  bodyptr p, *pp;
  struct timeval      start, end;  

  for (i = 0; i < nbody; i++) {/* for my bodies */
    p = &(bodytab[i]);
    if (Mass(p) != 0.0) 		/*   only load massive ones */
      loadtree(p);                /*     insert into tree     */
    else
      fprintf(stderr, "Process %d found body %d to have zero mass\n",Tmk_proc_id, (long) p);
  }

  hackcofm(Global->ncell);      /* find c-of-m and cost for all cells */

}


void printtree(p)
  cellptr p;
{
    int k;
    PRTV("cgpos",Pos(p));
    fprintf(stderr,", Cost: %d, mass:%f,%d :",Cost(p),Mass(p),(long)p);
    for (k=0;k<NSUB;k++) 
        if (Subp(p)[k] == NULL) fprintf(stderr,"  NULL  ");
        else fprintf(stderr," %d",(long)Subp(p)[k]);
    fprintf(stderr,"\n");
    for (k=0;k<NSUB;k++)
        if (Subp(p)[k] != NULL) if(Type(Subp(p)[k])== CELL) printtree((cellptr) Subp(p)[k]);
}

/*
 * LOADTREE: descend tree and insert particle.
 */

local void loadtree(p)
     bodyptr p;                        /* body to load into tree */
{
  int l, xq[NDIM], xp[NDIM], /* subindex(),*/ flag;
  volatile nodeptr *qptr, mynode;
  cellptr c;
    
  intcoord(xp, Pos(p));       /* form integer coords in range [0,IMAX] */
  mynode = (nodeptr) Global->G_root;       /* start with tree root     */
  qptr = &Subp(mynode)[subindex(xp, IMAX >>1)]; 
  /* uses integer position to determine which child the particle falls into */
  
  l = IMAX >> 2;        /* this is the tree level at which I still don't 
			   which child cell my body falls into         */
  flag=TRUE;
  while (flag) {                           /* loop descending tree     */
    if (l==0) barnes_error("not enough levels in tree\n");
    
    /* max. no. of levels in tree is 31 */
    if(*qptr==NULL) { 
      /* there's nothing where I'm trying to insert myself */
      if (*qptr == NULL) {               /*  retest: still no child    */
	*qptr = (nodeptr) p;             /*  new child               */
	flag=FALSE;                      /* job is finished          */
      }
    }

    if (flag && (*qptr != NULL) && (Type(*qptr) == BODY)) {        
      /*   reached a "leaf"?      */
      if (Type(*qptr) == BODY) {             /* still a "leaf"?      */
	c = makecell();                    /* alloc a new cell     */
	intcoord(xq, Pos(*qptr));          /* get integer coords   */
	Subp(c)[subindex(xq, l)] = *qptr;  /* put body in cell     */
	*qptr = (nodeptr) c;               /* link cell in tree    */
      }
    }
    if (flag) {
      mynode = *qptr;
      qptr = &Subp(*qptr)[subindex(xp, l)];  /* move down one level  */
      l = l >> 1;                            /* and test next bit    */
    }
  }
}


/*
 * INTCOORD: compute integerized coordinates.
 * Returns: TRUE unless rp was out of bounds.
 */

local bool intcoord(xp, rp)
  int xp[NDIM];                  /* integerized coordinate vector [0,IMAX) */
  vector rp;                     /* real coordinate vector (system coords) */
{
    int k;
    bool inb;
    double xsc, floor();
    
    inb = TRUE;                               /* use to check bounds      */
    for (k = 0; k < NDIM; k++) {              /* loop over dimensions     */
        xsc = (rp[k] - Global->rmin[k]) / Global->rsize; 
        /* scale to range [0,1)*/
        if (0.0 <= xsc && xsc < 1.0)          /*   within unit interval?  */
            xp[k] = floor(IMAX * xsc);        /*     then integerize      */
        else                                  /*   out of range           */
            inb = FALSE;                      /*     then remember that   */
    }
    return (inb);
}

/*
 * SUBINDEX: determine which subcell to select.
 */

local int subindex(x, l)
  int x[NDIM];                       /* integerized coordinates of particle */
  int l;                             /* current level of tree */
{
    int i, k;
    int yes;
    
    i = 0;                                   /* sum index in i           */
    yes = FALSE;
    if (x[0] & l) {
        i += NSUB >> 1;
        yes = TRUE;
    }
    for (k = 1; k < NDIM; k++) {              /* check each dimension     */
        if (((x[k] & l) && !yes) || (!(x[k] & l) && yes)) { 
            i += NSUB >> (k + 1);            /*     skip over subcells   */
            yes = TRUE;
        }
        else yes = FALSE;
    }

    return (i);
}


/*
 * HACKCOFM: descend tree finding center-of-mass coordinates.
 */

local void hackcofm(nc)
  int nc;
{
    int i,Myindex,k;
    nodeptr r;
    cellptr q;

    int cellcount = 0;
    static vector tmpv, dr;
    static real drsq;
    static matrix drdr, Idrsq, tmpm;

    /* get a cell using get*sub.  Cells are got in reverse of the order in */
    /* the cell array; i.e. reverse of the order in which they were created */
    /* this way, we look at child cells before parents			 */

    for (k = nc - 1; k>=0; k--) { /* For all the cells */
      q = &(ctab[k]);
      cellcount++;
      assert(Type(q)==CELL);
      Mass(q) = 0.0;                          /*   init total mass        */
      Cost(q) = 0;
      CLRV(Pos(q));
      for (i = 0; i < NSUB; i++) {            /*   loop over subcells     */
	r = Subp(q)[i];
	if (r != NULL) {                    /*     does subcell exist?  */
	  Mass(q) += Mass(r);             /*       sum total mass     */
	  Cost(q) += Cost(r);             /*       sum costs          */
	  MULVS(tmpv, Pos(r), Mass(r));   /*       find moment        */
	  ADDV(Pos(q), Pos(q), tmpv);     /*       sum tot. moment    */
	}
#if	DEBUG
	fprintf(junk, "Done with child %d\n", i);
	fflush(junk);
#endif
      }
      DIVVS(Pos(q), Pos(q), Mass(q));         /*   rescale cms position   */
#ifdef QUADPOLE
        CLRM(Quad(q));                          /*   init. quad. moment     */
        for (i = 0; i < NSUB; i++) {            /*   loop over subnodes     */
            r = Subp(q)[i];
            if (r != NULL) {                    /*     does subnode exist?  */
                SUBV(dr, Pos(r), Pos(q));       /*       displacement vect. */
                OUTVP(drdr, dr, dr);            /*       outer prod. of dr  */
                DOTVP(drsq, dr, dr);            /*       dot prod. dr * dr  */
                SETMI(Idrsq);                   /*       init unit matrix   */
                MULMS(Idrsq, Idrsq, drsq);      /*       scale by dr * dr   */
                MULMS(tmpm, drdr, 3.0);         /*       scale drdr by 3    */
                SUBM(tmpm, tmpm, Idrsq);        /*       form quad. moment  */
                MULMS(tmpm, tmpm, Mass(r));     /*       of cm of subnode,  */
                if (Type(r) == CELL)            /*       if subnode is cell */
                    ADDM(tmpm, tmpm, Quad(r));  /*         use its moment   */
                ADDM(Quad(q), Quad(q), tmpm);   /*       add to qm of cell  */
            }
        }
#endif
    } /* For k = nc -1 .. */
}


/*
 * MAKECELL: allocation routine for cells.
 */

local cellptr makecell()
{
    cellptr c;
    int i, Mycell;
    
    if (Global->ncell ==maxcell)               /* if too many cells       */
      barnes_error("makecell: Processor %d need more than %d cells; increase fcells\n",Tmk_proc_id,maxcell);
    Mycell=Global->ncell++;
    c = ctab + Mycell;          /* get the cell's address    */
    Type(c) = CELL;                          /* initialize the fields     */
    Done(c) = FALSE;
    Mass(c) = 0.0;
    for (i = 0; i < NSUB; i++)
        Subp(c)[i] = NULL;
    return (c);
}

#ifdef USE_ORB
ORBNodeptr makeORBcell(parent,side)
ORBNodeptr parent;
bool side;				/* 0 for before split, 1 for after */
{

  ORBNodeptr c;
  int i, Mycell, NumChildProcs;
  int intpow();

  Tmk_lock_acquire(Global->ORBNcellLock);
  if (Global->nORBcell == 2*Tmk_nprocs)
	barnes_error("Not enough space allocated for ORB tree\n");
  Mycell = Global->nORBcell++;
  Tmk_lock_release(Global->ORBNcellLock);
  c = ORBtab + Mycell;

 /*Set the number of processors in childs current proc subset */
  NumChildProcs = intpow(2,parent->channel);

 /* If side = 1 then move last half of Proc Id's to front of currprocs array */
      for (i=0; i<NumChildProcs; i++) 
	  c->currprocs[i] = (side == 1) ? parent->currprocs[NumChildProcs + i]: parent->currprocs[i];
	  
/* Write -1 as terminator of currprocs list */
      c->currprocs[NumChildProcs] = -1;

  c->channel = parent->channel- 1;
  for (i=0; i<NDIM; i++) {
      c->currmin[i] = parent->currmin[i];
      c->currmax[i] = parent->currmax[i];
  }
  if (side == 0)
     c->currmax[parent->CartDir] = parent->X;
  else
     c->currmin[parent->CartDir] = parent->X;
  c->X = 0;
  c->rx0 = c->rx1 = 0;
  c->CartDir = (parent->CartDir + 1) % NDIM;
  for (i = 0; i < NUMSPLITS; i++)
      c->GWork[i] = 0;
  c->TotalWork = 0;
  c->work_tol = parent->work_tol / HALF;
  c->left = NULL;
  c->right = NULL;
  return(c);
}
#endif
