/*subfile:  view2d.h *********************************************************/
/*  include file for the VIEW2D program */

typedef struct v2d       /* structure for a 2D vertex or vector */
  {
  R8  x;  /* X-coordinate */
  R8  y;  /* Y-coordinate */
  } V2D;

#define VERTEX2D V2D
#define VECTOR2D V2D

typedef struct dircos         /* structure for direction cosines */
  {
  R8  x;  /* X-direction cosine */
  R8  y;  /* Y-direction cosine */
  R8  w;  /* distance from surface to origin (signed) */
  } DIRCOS;

typedef struct srfdat2d       /* structure for 2D surface data */
  {
  VERTEX2D v1;  /* coordinates of first vertex */
  VERTEX2D v2;  /* coordinates of second vertex */
  DIRCOS dc;    /* direction cosines of surface normal */
  R8 area;      /* area (length) of surface */
  R8 dist[2];   /* distances of vertices above clipping plane */
  IX nr;        /* surface number */
  IX type;      /* surface type data:  1 = obstruction only surface,
                   0 = normal surface, -1 = included surface,
                  -2 = part of an obstruction surface */
  } SRFDAT2D;

typedef struct srf2d          /* structure for limited surface data */
  {
  VERTEX2D v1;  /* coordinates of first vertex */
  VERTEX2D v2;  /* coordinates of second vertex */
  IX nr;        /* surface number */
  } SRF2D;

typedef struct line           /* structure for 1-D lines */
  {
  struct line *next; /* pointer to next line */
  R8 xl;   /* minimum X-coordinate */
  R8 xr;   /* maximum X-coordinate */
  } LINE;

typedef struct          /* view factor calculation control values */
  {
  IX nAllSrf;       /* total number of surfaces */
  IX nRadSrf;       /* number of radiating surfaces */
  IX nObstrSrf;     /* number of obstruction surfaces */
  IX nVertices;     /* number of vertices */
  IX enclosure;     /* 1 = surfaces form an enclosure */
  IX emittances;    /* 1 = process emittances */
  IX outFormat;     /* output file format */
  IX nPossObstr;    /* number of possible view obstructing surfaces */
  IX nProbObstr;    /* number of probable view obstructing surfaces */
  IX prjReverse;    /* projection control; 0 = normal, 1 = reverse */
  R4 epsObstr;      /* convergence for obstrcted views */
  IX maxRecursion;  /* maximum number of recursion levels */
  IX minRecursion;  /* minimum number of recursion levels */
  IX failRecursion; /* 1 = obstructed view factor did not converge */
  R4 epsAF;         /* convergence for current AF calculation */
  U4 wastedVObs;    /* number of ViewObstructed() calculations wasted */
  U4 usedVObs;      /* number of ViewObstructed() calculations used */
  IX failConverge;  /* 1 if any calculation failed to converge */
  SRFDAT2D srf1T;   /* participating surface; transformed coordinates */
  SRFDAT2D srf2T;   /* participating surface; transformed coordinates;
                        view from srf1T toward srf2T. */ 
  SRF2D *srfOT;     /* pointer to array of view obstrucing surfaces;
                        dimensioned from 0 to maxSrfT in View3d();
                        coordinates transformed relative to srf2T. */
  LINE *lines;      /* coordinates of lines representing obstruction shadows */
  } VFCTRL;

/* macros for simple mathematical operations */
#define ABS(a)    (((a) >= 0) ? (a) : (-a))   /* absolute value */
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))   /* max of 2 values */
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))   /* min of 2 values */
#define SQR(a)    ((a)*(a))                   /* square of value */
#define CUBE(a)   ((a)*(a)*(a))               /* cube of value */

/* macros to avoid recomputing the argument; for complicated arguments.
   requires that 'z' and 'y' be declared. */
#define ABSZ(a)   (((z=a) >= 0) ? (z) : (-z))   /* absolute value */
#define MAXZ(a,b) (((z=a) > (y=b)) ? (z) : (y)) /* max of 2 values */
#define MINZ(a,b) (((z=a) < (y=b)) ? (z) : (y)) /* min of 2 values */
#define SQRZ(a)   (((z=a))*(z))                 /* square of value */
#define CUBEZ(a)  (((z=a))*(z)*(z))             /* cube of value */

#define scopy1(n,x,y) memcpy(y,x,n<<1)          /* copy 2-byte integers */
#define icopy1(n,x,y) memcpy(y,x,n*sizeof(IX))  /* copy current integers */
#define lcopy1(n,x,y) memcpy(y,x,n<<2)          /* copy 4-byte integers */
#define fcopy1(n,x,y) memcpy(y,x,n<<2)          /* copy 4-byte reals */
#define dcopy1(n,x,y) memcpy(y,x,n<<3)          /* copy 8-byte reals */

#define szero1(n,x) memset(x,0,n<<1)            /* zero 2-byte integers */
#define izero1(n,x) memset(x,0,n*sizeof(IX))    /* zero current integers */
#define lzero1(n,x) memset(x,0,n<<1)            /* zero 4-byte integers */
#define fzero1(n,x) memset(x,0,n<<2)            /* zero 4-byte reals */
#define dzero1(n,x) memset(x,0,n<<3)            /* zero 8-byte reals */

/*  View2D function prototypes.  */

     /* input / output */
void CountVS2D( char *title, VFCTRL *vfCtrl );
void GetVS2D( I1 **name, R4 *emit, IX *base, IX *cmbn,
  SRFDAT2D *srf, VFCTRL *vfCtrl );
R4 ReportAF( const IX nSrf, const IX encl, const I1 *title, const I1 **name,
  const R4 *area, const R4 *emit, const IX *base, R8 **AF, R4 *eMax );
void SaveVF( I1 *fileName, I1 *program, I1 *version,
             IX format, IX encl, IX didemit, IX nSrf,
             R4 *area, R4 *emit, R8 **AF, R4 *vtmp );

     /* 2-D view factor functions */
void View2D( SRFDAT2D *srf, R8 **AF, VFCTRL *vfCtrl );
R8 FA1A2( VERTEX2D *v1a, VERTEX2D *v1b, VERTEX2D *v2a, VERTEX2D *v2b );
R8 FdA1A2( VERTEX2D *v1, DIRCOS *u1, VERTEX2D *v2a, VERTEX2D *v2b );
IX ClipYC( VERTEX2D *v1, VERTEX2D *v2, const R8 yc );
R8 ViewObstructed( VERTEX2D *v1, VFCTRL *vfCtrl );
IX LineOverlap( LINE *base, LINE *new, LINE *used, LINE *free, const R8 eps );
R8 ViewAdapt2D( VERTEX2D *v1, VERTEX2D *v2, R8 area, IX level, VFCTRL *vfCtrl );
IX errorf( IX severity, I1 *file, IX line, ... );

IX SelfObstructionTest2D( SRFDAT2D *srf1, SRFDAT2D *srf2, SRFDAT2D *srfn );
IX BoxTest( SRFDAT2D *srfN, SRFDAT2D *srfM, SRFDAT2D *srf,
  IX *los, IX *oos, IX npos );
void ClipSrf2D( SRFDAT2D *srf, const R4 flag );
IX ProjectionDirection( SRFDAT2D *srfN, SRFDAT2D *srfM, SRFDAT2D *srf,
  IX *probableObstr, IX *orientObstr, VFCTRL *vfCtrl );
void CoordTrans2D( SRFDAT2D *srf1, SRFDAT2D *srf2, SRFDAT2D *srf,
  IX *probableObstr, VFCTRL *vfCtrl );
IX ClipY0( VERTEX2D *v1, VERTEX2D *v2 );
void SetSrf( SRFDAT2D *srf );
IX SetPosObstr2D( IX nSrf, SRFDAT2D *srf, IX *lpos );
void DumpOS( I1 *title, const IX nos, IX *los );
void DumpSrf( SRFDAT2D *srf );

     /* post processing */
void NormAF( const nSrf, const R4 *emit, const R4 *area, R8 **AF,
  const R8 eMax, const IX itMax );
IX Combine( const IX nSrf, const IX *cmbn, R4 *area, I1 **name, R8 **AF );
void Separate( const IX nSrf, const IX *base, R4 *area, R8 **AF );
void IntFac( const IX nSrf, const R4 *emit, const R4 *area, R8 **AF );

