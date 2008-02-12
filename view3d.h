#ifndef V3D_VIEW3D_H
#define V3D_VIEW3D_H

#include "common.h"
#include "types.h"

/*subfile:  view3d.h  ********************************************************/
/*  include file for the VIEW3D program */

#include <string.h> /* prototype: memcpy */
#include <stdio.h>

#define MAXNV 4     /* max number of vertices for an initial surface */
#define MAXNV1 5    /* max number of vertices after 1 clip */
#define MAXNV2 12   /* max number of vertices clipped projection */
//#define NAMELEN 12  /* length of a name */
#ifdef XXX
#define PI       3.141592653589793238
#define PIt2     6.283185307179586477   /* 2 * pi */
#define PIt4    12.566370614359172954   /* 4 * pi */
#define PId2     1.570796326794896619   /* pi / 2 */
#define PId4     0.785398163397448310   /* pi / 4 */
#define PIinv    0.318309886183790672   /* 1 / pi */
#define PIt2inv  0.159154943091895346   /* 1 / (2 * pi) */
#define PIt4inv  0.079577471545947673   /* 1 / (4 * pi) */
#endif
#define RTD 57.2957795  /* convert radians to degrees */
#define EPS 1.0e-6
#define EPS2 1.0e-12

typedef struct v2d{       /* structure for 2D vertex */
  double  x;  /* X-coordinate */
  double  y;  /* Y-coordinate */
} Vec2;

typedef struct v3d{       /* structure for a 3D vertex or vector */
  double  x;  /* X-coordinate */
  double  y;  /* Y-coordinate */
  double  z;  /* Z-coordinate */
} Vec3;

/* vector macros using pointer to 3-element structures */

/*  VECTOR:  define vector C from vertex A to vextex B.  */
#define VECTOR(a,b,c)   \
    c->x = b->x - a->x; \
    c->y = b->y - a->y; \
    c->z = b->z - a->z; 

/*  VCOPY:  copy elements from vertex/vector A to B.  */
#define VCOPY(a,b)  { b->x = a->x; b->y = a->y; b->z = a->z; }

/*  VMID:  define vertex C midway between vertices A and B.  */
#define VMID(a,b,c)   \
    c->x = 0.5 * (a->x + b->x); \
    c->y = 0.5 * (a->y + b->y); \
    c->z = 0.5 * (a->z + b->z); 

/*  VDOT:  compute dot product of vectors A and B.  */
#define VDOT(a,b)   (a->x * b->x + a->y * b->y + a->z * b->z)

/*  VDOTW:  dot product of a vertex, V, and direction cosines, C.  */
#define VDOTW(v,c)  (c->w + VDOT(v,c))

/*  VLEN:  compute length of vector A.  */
#define VLEN(a)     sqrt( VDOT(a,a) )

/*  VCROSS:  compute vector C as cross product of A and B.  */
#define VCROSS(a,b,c)   \
    c->x = a->y * b->z - a->z * b->y; \
    c->y = a->z * b->x - a->x * b->z; \
    c->z = a->x * b->y - a->y * b->x; 

/*  VSCALE:  vector B = scalar C times vector A.  */
#define VSCALE(c,a,b)  \
    b->x = c * a->x; \
    b->y = c * a->y; \
    b->z = c * a->z; 

typedef struct dircos         /* structure for direction cosines */
  {
  double  x;  /* X-direction cosine */
  double  y;  /* Y-direction cosine */
  double  z;  /* Z-direction cosine */
  double  w;  /* distance from surface to origin (signed) */
  } DirCos;

typedef struct srfdat3d       /* structure for 3D surface data */
  {
  int nr;              /* surface number */
  int nv;              /* number of vertices */
  int shape;           /* 3 = triangle; 4 = parallelogram; 0 = other */
  int type;            /* surface type data - defined below */
  double area;            /* area of surface */
  double rc;              /* radius enclosing the surface */
  DirCos dc;          /* direction cosines of surface normal */
  Vec3 ctd;       /* coordinates of centroid */
  Vec3 *v[MAXNV]; /* pointers to coordinates of up to MAXNV vertices */
  int NrelS;           /* orientation of srf N relative to S:
                         -1: N behind S; +1: N in front of S;
                          0: part of N behind S, part in front */
  int MrelS;           /* orientation of srf M relative to S */
  } SRFDAT3D;

#define RSRF 0  /* normal surface */
#define SUBS 1  /* subsurface */
#define MASK 2  /* mask surface */
#define NULS 3  /* null surface */
#define OBSO 4  /* obstruction only surface */

typedef struct srfdatnm{       /* structure for 3D surface data */
  int nr;              /* surface number */
  int nv;              /* number of vertices */
  int shape;           /* 3 = triangle; 4 = rectangle; 0 = other */
  int buffer;          /* for structure alignment */
  double area;            /* area of surface */
  double rc;              /* radius enclosing the surface */
  DirCos dc;          /* direction cosines of surface normal */
  Vec3 ctd;       /* coordinates of centroid */
  Vec3 v[MAXNV1]; /* coordinates of vertices */
  double dist[MAXNV1];    /* distances of vertices above plane of other surface */
} SRFDATNM;

typedef struct srfdat3x{       /* structure for 3D surface data */
  int nr;              /* surface number */
  int nv;              /* number of vertices */
  int shape;           /* 3 = triangle; 4 = rectangle; 0 = other */
  int buffer;          /* for structure alignment */
  double area;            /* surface area  */
  double ztmax;           /* maximum Z-coordinate of surface */
  DirCos dc;          /* direction cosines of surface normal */
  Vec3 ctd;       /* coordinates of centroid */
  Vec3 v[MAXNV1]; /* coordinates of vertices */
} SRFDAT3X;

typedef struct{    /* structure for direction cosines of polygon edge */
  double  x;  /* X-direction cosine */
  double  y;  /* Y-direction cosine */
  double  z;  /* Z-direction cosine */
  double  s;  /* length of edge */
  } EdgeDir;

typedef struct{    /* structure for Gaussian division of polygon edge */
  double  x;  /* X-coordinate of element */
  double  y;  /* Y-coordinate of element */
  double  z;  /* Z-coordinate of element */
  double  s;  /* length of element */
} EdgeDivision;

typedef struct{          /* view factor calculation control values */
  int nAllSrf;       /* total number of surfaces */
  int nRadSrf;       /* number of radiating surfaces; 
                         initially includes mask & null surfaces */
  int nMaskSrf;      /* number of mask & null surfaces */
  int nObstrSrf;     /* number of obstruction surfaces */
  int nVertices;     /* number of vertices */
  int format;        /* geometry format: 3 or 4 */
  int outFormat;     /* output file format */
  int row;           /* row to solve; 0 = all rows */
  int col;           /* column to solve; 0 = all columns */
  int enclosure;     /* 1 = surfaces form an enclosure */
  int emittances;    /* 1 = process emittances */
  int nPossObstr;    /* number of possible view obstructing surfaces */
  int nProbObstr;    /* number of probable view obstructing surfaces */
  int prjReverse;    /* projection control; 0 = normal, 1 = reverse */
  double epsAdap;       /* convergence for adaptive integration */
  double rcRatio;       /* rRatio of surface radii */
  double relSep;        /* surface separation / sum of radii */
  int method;        /* 0 = 2AI, 1 = 1AI, 2 = 2LI, 3 = 1LI, 4 = ALI */
  int nEdgeDiv;      /* number of edge divisions */
  int maxRecursALI;  /* max number of ALI recursion levels */
  unsigned long usedV1LIadapt; /* number of V1LIadapt() calculations used */
  int failViewALI;   /* 1 = unobstructed view factor did not converge */
  int maxRecursion;  /* maximum number of recursion levels */
  int minRecursion;  /* minimum number of recursion levels */
  int failRecursion; /* 1 = obstructed view factor did not converge */
  double epsAF;         /* convergence for current AF calculation */
  unsigned long wastedVObs;    /* number of ViewObstructed() calculations wasted */
  unsigned long usedVObs;      /* number of ViewObstructed() calculations used */
  unsigned long totPoly;       /* total number of polygon view factors */
  unsigned long totVpt;        /* total number of view points */
  int failConverge;  /* 1 if any calculation failed to converge */
  SRFDAT3X srf1T;   /* participating surface; transformed coordinates */
  SRFDAT3X srf2T;   /* participating surface; transformed coordinates;
                       view from srf1T toward srf2T. */ 
  SRFDAT3X *srfOT;  /* pointer to array of view obstrucing surfaces;
                       dimensioned from 0 to maxSrfT in View3d();
                       coordinates transformed relative to srf2T. */
} View3DControlData;

#define UNK -1  /* unknown integration method */
#define DAI 0   /* double area integration */
#define SAI 1   /* single area integration */
#define DLI 2   /* double line integration */
#define SLI 3   /* single line integration */
#define ALI 4   /* adaptive line integration */

typedef struct PolygonVertexEdge_struct{   /* homogeneous coordinate description of vertex/edge */
  struct PolygonVertexEdge_struct *next;  /* pointer to next vertex/edge */
  int buffer;          /* for structure alignment */
  double x, y;    /* X and Y coordinates of the vertex */
  double a, b;    /* A, B */
  double c;       /*  & C homogeneous coordinates of the edge */
} PolygonVertexEdge;

typedef struct Polygon_struct{   /* description of a polygon */
  struct Polygon_struct *next;  /* pointer to next polygon */
  PolygonVertexEdge *firstVE;      /* pointer to first vertex of polygon */
  double trns;            /* (0.0 <= transparency <= 1.0) */
  double area;            /* area of the polygon */
} Polygon;

/* macros for simple mathematical operations */
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))   /* max of 2 values */
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))   /* min of 2 values */

/*  function prototypes.  */

/* 3-D view factor functions */
V3D_API void View3D( SRFDAT3D *srf, const int *base, int *possibleObstr
	, double **AF, View3DControlData *vfCtrl
);

int ProjectionDirection( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm,
  int *los, View3DControlData *vfCtrl );
int errorf( int severity, char *file, int line, ... );

#endif

