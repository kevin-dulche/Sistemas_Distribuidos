c     COMMON blocks for ZELIG version WATERSHED 2.4.
*
* $Id: w2.Tmknew.i,v 10.3 1997/06/02 07:30:26 alc Exp $
*
c
c     Version WATERSHED 2.4 is draped over a terrain map and
c     simulates a small watershed, closing the water budget.
c     Climate varies across the grid (WEATHR has (kr,kc) arguments),
c     DLA is draped over elevation, SOLWAT is remapped to a flowpath
c     version of the planar grid, and the hooks are in for seed
c     dispersal.
c     
c
c     Parameters to define I/O logical units:
      integer RUNCON, INSITE, INSPP, PCHFILE, PRTFILE, TRACER,
     2  PROFILE, LOGFILE, INCFILE, DUMP
      PARAMETER (RUNCON=2,INSITE=3,INSPP=4,PCHFILE=7,PRTFILE=8,
     2  TRACER=9,PROFILE=10,LOGFILE=11,INCFILE=12,DUMP=13)
c *W* More infiles for version WATERSHED:
      Parameter (INSOIL=14,INDEM=15,INSLP=16,INASP=17)

c     Parameters to dimension ZELIG version WATERSHED 2.4:
c     MR=max rows, MC=max columns in grid;
c     MT=max trees per plot;
c     MS=max number of species;
c     MH=max height of foliage profiles;
c     MST=max number of soil types on grid;
c     ML=max number of soil layers;
c     MLY=max lag years before seedlings are planted.

*     Redimension the arrays so that the leading dimension is always a
*     multiple of 16 for better subpage alignment
      PARAMETER (MR=32,MC=32,MT=400,MS=16,MH=80,MST=10,ML=10,MLY=7)

c     Variables in COMMON ...
c     
c     The first set are constants (replicated to all nodes);
c     The second set are dynamic (partitioned to nodes);
c     The third set is dynamic (shared by all nodes);
c     The fourth set are work arrays (dynamic, local to each node):
c

      COMMON/Tmk_shared_common/
     2       elev(MR,MC), slope(MR,MC), aspect(MR,MC),
     3       biom(MR,MC), pwbi(MR,MC), dfla(MR,MC),
     4       sba(MS,MR,MC), cohorts(MLY,MS,MR,MC),
     5       phi(2,MR,MC), rrksun(12,MR,MC), rsun(12,MR,MC),
     6       ala(MH,MR,MC), al(0:MH,MR,MC), dbh(MT,MR,MC),
     7       snopak(MR,MC), sw(ML,MR,MC),
     F       ind(MR,MC), msol(MR,MC), maxht(MR,MC),
     G       isp(MT,MR,MC), ibc(MT,MR,MC), nogro(MT,MR,MC)
      character*4 msp

      COMMON/WORK/  ct, tx, bgs, egs, tgs, ddbase,
     2       sff(3), ddf(MS), smf(2,MS),
     E       smr, smbt, wsc, xc, drzs, days(12),
     3       tmin(12), tmax(12), t(12), rain(12), snow(12),
     8       sf(MST), fff(MST), dl(ML,MST), fc(ML,MST),
     9       wp(ML,MST), wk(ML,MST), frft(ML,MST), frfs(ML,MST),
     A       amax(MS), dmax(MS), hmax(MS), h2(MS), h3(MS), 
     B       s0(MS), s1(MS), t0(MS), t1(MS), t2(MS), slr(MS), aw0(MS),
     C       aw1(MS), g(MS), ddmin(MS), ddmax(MS), seed(MS), sdmax(MS),
     D       xtree, area, xk, theta, dlat, ebase, 
     a       xr(12), vr(12), vt(12),
     b       xtmin(12), xtmax(12), b(12),
     c       tlaps1(12), tlaps2(12), plapse(12),
     d       btmin(12), btmax(12), br(12),
     H       light(MS), mdrt(MS), nutri(MS), nsprt(MS), nsl(MST),
     I       msp(MS), lf(MS), ia(12),
     J       mode, indata, nrows, ncols, nspp, nyrs, kyr, iprt,
     K       ilog, ipch, ilai, itrx, nmax, nsoils, mxs,
     4       mbgs, megs, idum(3), nstmp(2,MS), iproc, inp

c     Definitions of variables in COMMON:
c
c     /RUN/  Run configuration and control, constant for run ...
c     MODE:  =0 if independent plots; =1 if interactive shading;
c     INDATA:  =0 to start from bare ground; 1, to read external initials;
c       =2 to write tree dump in final year (to be used as initials);
c     NROWS, NCOLS:  number of plots (grid rows, columns) to simulate;  
c     NSPP:  number of species in species driver file;
c     NYRS:  number of years to simulate (KYR, the counter);
c     IPRT:  ILOG, IPCH, ILAI, ITRX, interval for print, diagnostics log,
c       plot punch, leaf-area profile, and tracer file;
c     NMAX:  maximum number of trees per plot (set by AREA);
c     NSOILS:  number of soil types;
c     NSL:  number of soil layers per soil type (ML dimensions);
c     IDUM:  seed for random number generators; 
c     MXS:  height (m) of cross-section through leaf area profile (MODE=1)
c
c     /SPECIES/  Dimensioned by parameter MS, constant for run ...
c     MSP, AMAX, DMAX:  species mnemonic, max age (yr), and max dbh (cm);
c     HMAX, H2, H3:  max height (m) and coefficients of height allometry;
c     G, LF:  growth rate, lifeform (1=conifer, 2=deciduous);
c     T0, T1, T2:  taper coefficients;
c     S0, S1:  coefficients for sapwood thickness;
c     SLR:  sapwood:leaf area ratio;
c     AW0, AW1:  coefficients for aboveground woody biomass;
c     DDMIN, DDMAX:  min, max degree-days;
c     LIGHT, MDRT, NUTRI:  shade-, drought-, nutrient response class;
c     SEED, NSPRT, SDMAX:  seedling establishment rate, sprouts, and
c       max sproutable diameter;
c
c     /TREES/  Dimensioned by MT trees for plot (MR,MC) ...
c     ISP:  species label (A4);
c     DBH, IBC:  diameter (cm) and height (m) to base-of-crown;
c     NOGRO:  number of no-growth years.
c
c     /PLOTS/  Dimensioned (at least) by plot (MR,MC) ... 
c     IND:  number of individual trees; 
c     MSOL:  mapped soil type (integer label);
c     MAXHT:  maximum canopy height;
c     ELEV:  elevation (m), read from GIS DEM;
c     SLOPE:  slope (%), read from GIS file;
c     ASPECT:  slope aspect (degrees), from GIS file;
c     BIOM:  above-ground woody biomass;
c     PWBI:  potential woody biomass increment (used to compute SFF);
c     DFLA:  deciduous fraction of LAI (to estimate interception);
c     SBA:  basal area per species;
c     COHORTS:  seedling cohorts, by age class and species;
c     PHI:  relative radiation; 1=direct, 2=diffuse;
c     SUN:  total daily solar radiation (W/m**2), per month;
c     RSUN:  radiation relative to flat ground, per month;
c     ALA:  actual leaf area per height per plot;
c     AL:  available light at height m. 
c
c     /WATER/  Arrays serving the soil water balance ...
c     SNOPAK:  depth of snowpack (cm water), per plot;
c     SW:  soil water per plot and per soil layer;
c   
c     /SITE/  Site variables, mostly constants or scalars ...
c     XTREE:  mean canopy area (sq.m) of typical full-sized tree;
c     AREA:  plot area (sq.m); SF:  soil fertility (Mg/ha/yr);
c     THETA:  sun angle (rad);
c     SMR: snow melt rate (cm per degree day);
c     WSC:  water-to-snow conversion factor (cm->m);
c     XC:  interception constant (fraction per unit LAI);
c     DRZS:  depth of rooting zone for seedlings (=layer 1 depth);
c     FFF:  fast-flow fraction of infiltration in SOLWAT;
c     DDbase:  degree-day base temperature (C);
c     DBE:  donor plot's basal elevation (for deep percolation);
c
c     /MONTHS/  Constant arrays dimensioned for each month ...
c     XTMin, XTmax, VT; XR, VR:  mean temperature (C) and std. dev.,
c       mean total precip (cm) and std. dev.; these for base elevation;
c     IA, B:  parameters of the gamma distribution for precipitation;
c     TLAPS1, TLAPS2, PLAPSE:  elevation lapse rates for Tmin, Tmax, and P.
c
c     /SOIL/  Constant parameters per soil type ...
c     NSL:  number of soil layers per soil type;
c     SF:  soil fertility per soil type (Mg woody production /ha/year);
c     DL, FC, WP:  depth, field capacity, and wilting point (cm),
c       per soil layer (dimensioned by layer per soil type).
c     WK:  critical water content at which drawdown decelerates;
c     FRFt,s:  fine-root fraction for trees and seedlings, per soil layer.

c     /CLIME/  Base weather data, new each year and shared by all plots ...
c     BTmin, BTmax:  base min and max temperatures;
c     BR:  rain at the base elevation.

c     /WORK/  Arrays that are overwritten for each plot, each year ...
c     CT, TX:  coefficients for Priestley-Taylor PET estimate;
c     BGS, EGS; TGS:  day growing season begins and ends; total length;
c     MBGS, MEGS:  these as months (integers);
c     Tmin, Tmax, T:  monthly min, max, and mean temperature, per plot;
c     RAIN, SNOW:  monthly rain, and snow (each plot);
c     SFF:  soil fertility factors, per response class;
c     NSTMP:  number of new stumps from mortality (1=healthy, 2=stressed);
c     SMF:  soil-moisture factor (1=topsoil, 2=average of all soil layers);
c     DDF:  degree-day factor.

