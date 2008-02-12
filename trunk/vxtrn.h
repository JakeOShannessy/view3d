/*subfile:  vxtrn.h  *********************************************************/
/*  global variables for VIEWCHK  */

extern FILE *_ulog; /* log file */
extern FILE *_unxt; /* input file */
//extern int _echo;    /* true = echo input file */
extern I1 _vdrive[_MAX_DRIVE];   /* drive letter for program ViewGrX.exe */
extern I1 _vdir[_MAX_DIR];       /* directory path for program ViewGrX.exe */
extern I1 _file_name[_MAX_PATH]; /* temporary file name */
extern I1 _string[LINELEN];  /* buffer for a character string */

extern int _ntot;    /* total number of surfaces */
extern int _nsrf;    /* number of radiating surfaces */
extern int _nobs;    /* number of obstruction surfaces */
extern int _nvrt;    /* number of vertices */

extern int _encl;    /* true = surfaces form an enclosure */
extern int _list;    /* output control, higher value = more output */

extern I1 *_pb;     /* memory block for polygon descriptions */
extern Polygon *_nfpd; /* pointer to next free polygon descripton */
extern Polygon *_mrdp; /* most recently defined polygon */
extern PolygonVertexEdge *_nfve; /* pointer to next free vertex */
extern float _epsarea; /* minimum surface area */

extern float _xmin, _xmax, _ymin, _ymax;  /* limits of transformed vertices */

extern int _nAllSrf;       /* total number of surfaces */
extern int _nRadSrf;       /* number of radiating surfaces; 
                             initially includes mask & null surfaces */
extern int _nMaskSrf;      /* number of mask & null surfaces */
extern int _nObstrSrf;     /* number of obstruction surfaces */
extern int _nVertices;     /* number of vertices */
extern int _format;        /* geometry format: 3 or 4 */
extern int _outFormat;     /* output file format */
extern int _doRow;         /* row to solve; 0 = all rows */
extern int _doCol;         /* column to solve; 0 = all columns */
extern int _enclosure;     /* 1 = surfaces form an enclosure */
extern int _emittances;    /* 1 = process emittances */
extern int _nPossObstr;    /* number of possible view obstructing surfaces */
extern int _nProbObstr;    /* number of probable view obstructing surfaces */
extern int _prjReverse;    /* projection control; 0 = normal, 1 = reverse */
extern double _epsAdap;       /* convergence for adaptive integration */
extern double _rcRatio;       /* rRatio of surface radii */
extern double _relSep;        /* surface separation / sum of radii */
extern int _method;        /* 0 = 2AI, 1 = 1AI, 2 = 2LI, 3 = 1LI, 4 = ALI */
extern int _nEdgeDiv;      /* number of edge divisions */
extern int _maxRecursALI;  /* max number of ALI recursion levels */
extern unsigned long _usedV1LIadapt; /* number of V1LIadapt() calculations used */
extern int _failViewALI;   /* 1 = unobstructed view factor did not converge */
extern int _maxRecursion;  /* maximum number of recursion levels */
extern int _minRecursion;  /* minimum number of recursion levels */
extern int _failRecursion; /* 1 = obstructed view factor did not converge */
extern double _epsAF;         /* convergence for current AF calculation */
extern unsigned long _wastedVObs;    /* number of ViewObstructed() calculations wasted */
extern unsigned long _usedVObs;      /* number of ViewObstructed() calculations used */
extern unsigned long _totPoly;       /* total number of polygon view factors */
extern unsigned long _totVpt;        /* total number of view points */
extern int _failConverge;  /* 1 if any calculation failed to converge */
extern SRFDAT3X _srf1T;   /* participating surface; transformed coordinates */
extern SRFDAT3X _srf2T;   /* participating surface; transformed coordinates;
                             view from srf1T toward srf2T. */ 
extern SRFDAT3X *_srfOT;  /* pointer to array of view obstrucing surfaces;
                             dimensioned from 0 to maxSrfT in View3d();
                             coordinates transformed relative to srf2T. */
