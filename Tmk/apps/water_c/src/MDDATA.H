typedef double vm_type[3];

typedef struct mol_dummy {
    vm_type VM;
    double F[MXOD2][NDIR][NATOM];
} molecule_type;

extern molecule_type *VAR;

extern double  TLC[100],
      ELPST,TKIN,TVIR,TTMV,FPOT,FKIN;
extern int IX[3*MXOD2+1], IRST,NVAR,NXYZ,NXV,IXF,IYF,IZF,IMY,IMZ;

#define MAXPROCS 128

extern int StartMol[MAXPROCS+1];
extern int StartPos[MAXPROCS+1];
extern int MolsPerProc;
