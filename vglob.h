/*subfile:  vglob.h  *********************************************************/
/*  VGLOB:  global variables for VIEWCHK  */

FILE *_ulog=stderr; /* log file */
int _echo=0;       /* true = echo input file */
I1 _vdrive[_MAX_DRIVE];   /* drive letter for program ViewGrX.exe */
I1 _vdir[_MAX_DIR];       /* directory path for program ViewGrX.exe */
I1 _file_name[_MAX_PATH]; /* temporary file name */
I1 _string[LINELEN];  /* buffer for a character string */

int _encl;    /* true = surfaces form an enclosure */
int _list;    /* output control, higher value = more output */

int _nAllSrf;       /* total number of surfaces */
int _nRadSrf;       /* number of radiating surfaces;
                      initially includes mask & null surfaces */
int _nMaskSrf;      /* number of mask & null surfaces */
int _nObstrSrf;     /* number of obstruction surfaces */
int _nVertices;     /* number of vertices */
int _format;        /* geometry format: 3 or 4 */
int _outFormat;     /* output file format */
int _doRow;           /* row to solve; 0 = all rows */
int _doCol;           /* column to solve; 0 = all columns */
int _enclosure;     /* 1 = surfaces form an enclosure */
int _emittances;    /* 1 = process emittances */
int _nPossObstr;    /* number of possible view obstructing surfaces */
int _nProbObstr;    /* number of probable view obstructing surfaces */
int _prjReverse;    /* projection control; 0 = normal, 1 = reverse */
double _epsAdap;       /* convergence for adaptive integration */
double _rcRatio;       /* rRatio of surface radii */
double _relSep;        /* surface separation / sum of radii */
int _method;        /* 0 = 2AI, 1 = 1AI, 2 = 2LI, 3 = 1LI, 4 = ALI */
int _nEdgeDiv;      /* number of edge divisions */
int _maxRecursALI;  /* max number of ALI recursion levels */
unsigned long _usedV1LIadapt; /* number of V1LIadapt() calculations used */
int _failViewALI;   /* 1 = unobstructed view factor did not converge */
int _maxRecursion;  /* maximum number of recursion levels */
int _minRecursion;  /* minimum number of recursion levels */
int _failRecursion; /* 1 = obstructed view factor did not converge */
double _epsAF;         /* convergence for current AF calculation */
unsigned long _wastedVObs;    /* number of ViewObstructed() calculations wasted */
unsigned long _usedVObs;      /* number of ViewObstructed() calculations used */
unsigned long _totPoly;       /* total number of polygon view factors */
unsigned long _totVpt;        /* total number of view points */
int _failConverge;  /* 1 if any calculation failed to converge */
SRFDAT3X _srf1T;   /* participating surface; transformed coordinates */
SRFDAT3X _srf2T;   /* participating surface; transformed coordinates;
                      view from srf1T toward srf2T. */
SRFDAT3X *_srfOT;  /* pointer to array of view obstrucing surfaces;
                       dimensioned from 0 to maxSrfT in View3d();
                       coordinates transformed relative to srf2T. */
