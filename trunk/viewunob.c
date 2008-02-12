/*subfile:  ViewUnob.c  ******************************************************/

/*  Compute unobstructed view factors  */

#ifdef _DEBUG
# define DEBUG 1
#else
# define DEBUG 0
#endif

#include "viewunob.h"

#include <stdio.h>
#include <math.h>  /* prototypes: atan, cos, fabs, log, sqrt */
#include "types.h"
#include "view3d.h"
#include "misc.h"
#include "heap.h"
#include "viewobs.h"

#define PId2     1.570796326794896619   /* pi / 2 */
#define PIinv    0.318309886183790672   /* 1 / pi */
#define PIt4inv  0.079577471545947673   /* 1 / (4 * pi) */

/* The following variables are "global" to this file.  
 * They are allocated and freed in ViewsInit(). */
EdgeDir *_rc1; /* edge DirCos of surface 1 */
EdgeDir *_rc2; /* edge DirCos of surface 2 */
EdgeDivision **_dv1;  /* edge divisions of surface 1 */
EdgeDivision **_dv2;  /* edge divisions of surface 2 */
long _usedV1LIpart=0L;  /* number of calls to V1LIpart() */

/* forward decls */

static double View2AI( const int nss1, const DirCos *dc1, const Vec3 *pt1
	, const double *area1, const int nss2, const DirCos *dc2, const Vec3 *pt2, const double *area2
);

static double View2LI( const int nd1, const int nv1, const EdgeDir *rc1
	, EdgeDivision **dv1,const int nd2, const int nv2, const EdgeDir *rc2
	, EdgeDivision **dv2
);

static double View1LI( const int nd1, const int nv1, const EdgeDir *rc1
	, EdgeDivision **dv1, const Vec3 *v1, const int nv2, const Vec3 *v2
);
static double V1LIpart( const Vec3 *pp, const Vec3 *b0, const Vec3 *b1
	, const Vec3 *B, const double b2, int *flag
);

static double V1LIxact( const Vec3 *a0, const Vec3 *a1, const double a
	, const Vec3 *b0, const Vec3 *b1, const double b
);
static double V1LIadapt( Vec3 Pold[3], double dFold[3], double h
	, const Vec3 *b0, const Vec3 *b1, const Vec3 *B, const double b2
	, int level, View3DControlData *vfCtrl
);
static double ViewALI( const int nv1
	, const Vec3 *v1, const int nv2, const Vec3 *v2
	, View3DControlData *vfCtrl
);
static int DivideEdges( int nd, int nv, Vec3 *vs, EdgeDir *rc, EdgeDivision **dv );
static int GQParallelogram( const int nDiv, const Vec3 *vp, Vec3 *p, double *w );
static int GQTriangle( const int nDiv, const Vec3 *vt, Vec3 *p, double *w );

/***  ViewUnobstructed.c  ****************************************************/

/*  Compute view factor (AF) -- no view obstructions.  */

double ViewUnobstructed( View3DControlData *vfCtrl, int row, int col )
  {
  Vec3 pt1[16], pt2[16];
  double area1[16], area2[16];
  SRFDAT3X *srf1;  /* pointer to surface 1 */
  SRFDAT3X *srf2;  /* pointer to surface 2 */
  double AF0,  /* estimate of AF */
     AF1;  /* improved estimate; one more edge division */
  int nmax, mmax;
  int nDiv;

#if( DEBUG > 1 )
  fprintf( _ulog, " VU %.2e", vfCtrl->epsAF );
#endif

  srf1 = &vfCtrl->srf1T;
  srf2 = &vfCtrl->srf2T;
  if( vfCtrl->method < ALI )
    AF1 = 2.0 * srf1->area;
  if( vfCtrl->method == DAI )  /* double area integration */
    {
#if( DEBUG > 1 )
    fprintf( _ulog, " 2AI" );
#endif
    for( nDiv=1; nDiv<5; nDiv++ )
      {
      AF0 = AF1;
      nmax = SubSrf( nDiv, srf1->nv, srf1->v, srf1->area, pt1, area1 );
      mmax = SubSrf( nDiv, srf2->nv, srf2->v, srf2->area, pt2, area2 );
      AF1 = View2AI( nmax, &srf1->dc, pt1, area1, mmax, &srf2->dc, pt2, area2 );
#if( DEBUG > 1 )
      fprintf( _ulog, " %g", AF1 );
#endif
      if( fabs(AF1 - AF0) < vfCtrl->epsAF ) goto done;
      }
    }
  else if( vfCtrl->method == SAI )  /* single area integration */
    {
#if( DEBUG > 1 )
    fprintf( _ulog, " 1AI" );
#endif
    for( nDiv=1; nDiv<5; nDiv++ )
      {
      AF0 = AF1;
      nmax = SubSrf( nDiv, srf1->nv, srf1->v, srf1->area, pt1, area1 );
      AF1 = -View1AI( nmax, pt1, area1, &srf1->dc, srf2 );
#if( DEBUG > 1 )
      fprintf( _ulog, " %g", AF1 );
#endif
      if( fabs(AF1 - AF0) < vfCtrl->epsAF ) goto done;
      }
    }
  else if( vfCtrl->method == SLI )  /* single line integration */
    {
#if( DEBUG > 1 )
    fprintf( _ulog, " 1LI" );
#endif
    for( nDiv=1; nDiv<5; nDiv++ )
      {
      AF0 = AF1;
      DivideEdges( nDiv, srf1->nv, srf1->v, _rc1, _dv1 );
      AF1 = View1LI( nDiv, srf1->nv, _rc1, _dv1, srf1->v, srf2->nv, srf2->v );
#if( DEBUG > 1 )
      fprintf( _ulog, " %g", AF1 );
#endif
      if( fabs(AF1 - AF0) < vfCtrl->epsAF ) goto done;
      }
    }
  else if( vfCtrl->method == DLI )  /* double line integration */
    {
#if( DEBUG > 1 )
    fprintf( _ulog, " 2LI" );
#endif
    for( nDiv=1; nDiv<5; nDiv++ )
      {
      AF0 = AF1;
      DivideEdges( nDiv, srf1->nv, srf1->v, _rc1, _dv1 );
      DivideEdges( nDiv, srf2->nv, srf2->v, _rc2, _dv2 );
      AF1 = View2LI( nDiv, srf1->nv, _rc1, _dv1, nDiv, srf2->nv, _rc2, _dv2 );
#if( DEBUG > 1 )
      fprintf( _ulog, " %g", AF1 );
#endif
      if( fabs(AF1 - AF0) < vfCtrl->epsAF ) goto done;
      }
    }
    
  /* adaptive single line integration - method==ALI or simpler methods fail */
#if( DEBUG > 1 )
  if( vfCtrl->method < ALI )
    fprintf( _ulog, " Fixing view factor\n" );
  fprintf( _ulog, " ALI" );
#endif
#if( DEBUG == 1 )
  if( vfCtrl->method < ALI )
    fprintf( _ulog, " row %d, col %d,  Fix %s (r %.2f, s %.2f) AF0 %g Af1 %g\n",
      row, col, methods[vfCtrl->method],
      vfCtrl->rcRatio, vfCtrl->relSep, AF0, AF1 );
#endif

  AF1 = ViewALI( srf1->nv, srf1->v, srf2->nv, srf2->v, vfCtrl );

#if( DEBUG == 1 )
  if( vfCtrl->method < ALI )
    fprintf( _ulog, "AF %g\n", AF1 );
#endif
  if( vfCtrl->method == ALI )  /* adaptive line integration */
    nDiv = 2;                /* for bins[][] report */
#if( DEBUG > 1 )
  fprintf( _ulog, " %g", AF1 );
#endif

done:
#if( DEBUG > 1 )
  fprintf( _ulog, "\n" );
#endif
  vfCtrl->nEdgeDiv = nDiv;

  return AF1;

  }  /* end ViewUnobstructed */

/***  View2AI.c  *************************************************************/

/*  Compute direct interchange area by double area integration.
 *  Surfaces described by their direction cosines and NSS1|2 vertices 
 *  and associated areas for numberical integration.  */

double View2AI( const int nss1, const DirCos *dc1, const Vec3 *pt1, const double *area1,
            const int nss2, const DirCos *dc2, const Vec3 *pt2, const double *area2 )
  {
  Vec3 V;
  double r2, sumt, sum=0.0;
  int i, j;

  for( i=0; i<nss1; i++ )
    {
    for( j=0; j<nss2; j++ )
      {
      VECTOR( (pt1+i), (pt2+j), (&V) );
      sumt = VDOT( (&V), dc1 ) * VDOT( (&V), dc2 );
      r2 = VDOT( (&V), (&V) );
      sumt *= area1[i] * area2[j] / ( r2 * r2 );
      sum -= sumt;
      }
    }

  sum *= PIinv;             /* divide by pi */

  return sum;

  }  /* end View2AI */

/*  View1AI() is in viewobs.c  */

/***  View2LI.c  *************************************************************/

/*  Compute direct interchange area by double line integration. 
 *  Both surfaces described by directions cosines of edges and 
 *  subdivisions of those edges for numerical integration.  */

double View2LI( const int nd1, const int nv1, const EdgeDir *rc1, EdgeDivision **dv1,
            const int nd2, const int nv2, const EdgeDir *rc2, EdgeDivision **dv2 )
/* nd1 - number of edge divisions of polygon 1.
 * nv1 - number of vertices/edges of polygon 1.
 * rc1 - vector of direction cosines of edges of polygon 1.
 * dv1 - array of edge divisions of polygon 1.
 * nd2 - number of edge divisions of polygon 2.
 * nv2 - number of vertices/edges of polygon 2.
 * rc2 - vector of direction cosines of edges of polygon 2.
 * dv2 - array of edge divisions of polygon 2.
 */
  {
  Vec3 R;    /* vector between edge elements */
  double r2;         /* square of distance between edge elements */
  double dot;        /* dot product of edge direction cosines */
  double sum, sumt;  /* double because of large +/- operations */
  int i,   /* surface 1 edge index */
     j,   /* surface 2 edge index */
     n,   /* surface 1 edge element index */
     m;   /* surface 2 edge element index */

  for( sum=0.0,i=0; i<nv1; i++ )
    {
    for( j=0; j<nv2; j++ )
      {
      dot = VDOT( (rc1+i), (rc2+j) );
      if( fabs(dot) < EPS2 ) continue;
      for( sumt=0.0,n=0; n<nd1; n++ )
        for( m=0; m<nd2; m++ )
          {
          VECTOR( (dv1[i]+n), (dv2[j]+m), (&R) );
          r2 = VDOT( (&R), (&R) );
#if( DEBUG > 0 )
          if( r2 < EPS )
            errorf( 2, __FILE__, __LINE__, "log(r2) ", FltStr(r2,6), "" );
#endif
          sumt += dv1[i][n].s * dv2[j][m].s * log( r2 );
          }  /* end m & n loops */
      sum += dot * sumt;
      }  /* end j loop */
    }  /* end i loop */

  sum *= PIt4inv;          /* divide by 4*pi */

  return sum;

  }  /* end of View2LI */

/***  View1LI.c  *************************************************************/

/*  Compute direct interchange area by single line integral method.
 *  Surface 1 described by directions cosines of edges and 
 *  subdivisions of those edges for numerical integration. 
 *  Surface 2 described by its vertices.  */

double View1LI( const int nd1, const int nv1, const EdgeDir *rc1,
  EdgeDivision **dv1, const Vec3 *v1, const int nv2, const Vec3 *v2 )
/* nd1 - number of edge divisions of polygon 1.
 * nv1 - number of vertices/edges of polygon 1.
 * rc1 - vector of direction cosines of edges of polygon 1.
 * dv1 - array of edge divisions of polygon 1.
 * v1  - vector of vertices of polygon 1.
 * nv2 - number of vertices/edges of polygon 2.
 * v2  - vector of vertices of polygon 2.
 */
  {
  double sum, sumt;  /* double because of large +/- operations */
  int i, im1,  /* surface 1 edge index */
     j, jm1,  /* surface 2 edge index */
     n;       /* surface 1 edge element index */

     /* for all edges of polygon 2 */
  jm1 = nv2 - 1;
  for( sum=0.0,j=0; j<nv2; jm1=j++ )
    {
    Vec3 B;   /* edge of polygon 2 */
    double b2;        /* length of edge squared */
    double b, binv;   /* length of edge and its inverse */

    VECTOR( (v2+jm1), (v2+j), (&B) );
    b2 = VDOT( (&B), (&B) );
#if( DEBUG > 0 )
    if( b2 < EPS2 )
      errorf( 2, __FILE__, __LINE__, "small b2 ", FltStr(b2,6), "" );
#endif
    b = sqrt( b2 );
    binv = 1.0 / b;    /* b > 0.0 */
       /* for all edges of polygon 1 */
    im1 = nv1 - 1;
    for( i=0; i<nv1; im1=i++ )
      {
      int parallel=1;  /* true if edges i and j are parallel */
      double dot = VDOT( (&B), (rc1+i) ) * binv;
      if( fabs(dot) <= EPS * b ) continue;
      if( fabs(dot) < 1.0-EPS )
        parallel = 0;
      for( sumt=0.0,n=0; n<nd1; n++ )    /* numeric integration */
        {
        int close;
        sumt += V1LIpart( (void*)(dv1[i]+n), v2+jm1, v2+j, &B, b2, &close )
              * dv1[i][n].s;
        if( parallel && close ) break;   /* colinear edges */
        }  /* end numeric integration */

      if( n==nd1 )
        {
        sum += dot * sumt * binv;
        }
      else               /* colinear edges ==> analytic solution */
        {
        sumt = V1LIxact( v1+im1, v1+i, rc1[i].s, v2+jm1, v2+j, b );
        sum += dot * sumt;
        }
      }  /* end i loop */
    }  /* end j loop */

  sum *= PIt4inv;          /* divide by 4*pi */

  return sum;

  }  /* end of View1LI */

/***  V1LIpart.c  ************************************************************/

/*  Compute Mitalas & Stephenson dF value for a single point, PP,
 *  to an edge from B0 to B1.  Part of single line integral method.  */

double V1LIpart( const Vec3 *pp, const Vec3 *b0, const Vec3 *b1,
             const Vec3 *B, const double b2, int *flag )
/* pp     pointer to vertex on polygon 1;
 * b0     pointer to start of edge on polygon 2;
 * b1     pointer to end of edge on polygon 2;
 * B      vector from p0 to p1 (edge of polygon 2);
 * b2     length^2 of vector B;
 * *flag; return 1 if g=0; else return 0. */
  {
  Vec3 S, T, SxB;
  double s2, t2, sxb2;
  double sum=0.0;

  _usedV1LIpart += 1;  /* number of calls to V1LIpart() */
  VECTOR( b0, pp, (&S) );
  s2 = VDOT( (&S), (&S) );
  if( s2 > EPS2 )
    sum += VDOT( (&S), B ) * log( s2 );

  VECTOR( pp, b1, (&T) );
  t2 = VDOT( (&T), (&T) );
  if( t2 > EPS2 )
    sum += VDOT( (&T), B ) * log( t2 );

  VCROSS( (&S), B, (&SxB) );
  sxb2 = VDOT( (&SxB), (&SxB) );
  if( sxb2 > EPS2*b2 )
    {
    double h = s2 + t2 - b2;
    double g = sqrt( sxb2 );
    if( g > EPS2 )
      {
      double omega = PId2 - atan( 0.5 * h / g );
      sum += 2.0 * ( g * omega - b2 );
      }
    else
      errorf( 3, __FILE__, __LINE__, "View1LI failed, call George", "" );
    *flag = 0;
    }
  else
    {
    sum -= 2.0 * b2;
    *flag = 1;
    }

  return sum;

  }  /* end V1LIpart */

/***  V1LIadapt.c  ***********************************************************/

/*  Compute line integral by adaptive Simpson integration.
 *  This is a recursive calculation!  */

double V1LIadapt( Vec3 Pold[3], double dFold[3], double h, const Vec3 *b0,
  const Vec3 *b1, const Vec3 *B, const double b2, int level, View3DControlData *vfCtrl )
/* Pold   3 vertices on edge of polygon 1;
 * dFold  corresponding dF values;
 * h      |Pold[2] - Pold[0]| / 6.0;
 * b0     pointer to start of edge on polygon 2;
 * b1     pointer to end of edge on polygon 2;
 * B      vector from p0 to p1 (edge of polygon 2);
 * b2     length^2 of vector B;
 * *flag; return 1 if g=0; else return 0. */
  {
  Vec3 P[5];
  double dF[5];
  double F3,  /* F using 3-point Simpson integration */
     F5;  /* F using 5-point Simpson integration */
  int flag, j;

  for( j=0; j<3; j++ )
    {
    VCOPY( (Pold+j), (P+j+j) );
    dF[j+j] = dFold[j];
    }
  F3 = h * (dF[0] + 4.0*dF[2] + dF[4]);

  vfCtrl->usedV1LIadapt += 2;
  VMID( (P+0), (P+2), (P+1) );
  dF[1] = V1LIpart( P+1, b0, b1, B, b2, &flag );

  VMID( (P+2), (P+4), (P+3) );
  dF[3] = V1LIpart( P+3, b0, b1, B, b2, &flag );
  h *= 0.5;
  F5 = h * (dF[0] + 4.0*dF[1] + 2.0*dF[2] + 4.0*dF[3] + dF[4]);

  if( fabs( F5 - F3 ) > vfCtrl->epsAF )    /* test convergence */
    {
    if( ++level > vfCtrl->maxRecursALI )   /* limit maximum recursions */
      vfCtrl->failViewALI = 1;
    else             /* one more level of adaptive integration */
      F5 = V1LIadapt( P+0, dF+0, h, b0, b1, B, b2, level, vfCtrl )
         + V1LIadapt( P+2, dF+2, h, b0, b1, B, b2, level, vfCtrl );
    }

  return F5;

  }  /* end V1LIadapt */

/***  V1LIxact.c  ************************************************************/

/*  Analytic integration of colinear edges. */

double V1LIxact( const Vec3 *a0, const Vec3 *a1, const double a, 
             const Vec3 *b0, const Vec3 *b1, const double b )
/* a0 - point for start of edge of polygon 1.
 * a1 - point for end of edge of polygon 1.
 * a  - length of edge of polygon 1.
 * b0 - point for start of edge of polygon 2.
 * b1 - point for end of edge of polygon 2.
 * b -  length of edge of polygon 2.
 */
  {
  Vec3 V;   /* temporary vector */
  double e2, d2;
  double sum=0.0;

  VECTOR( b0, a1, (&V) );
  e2 = VDOT( (&V), (&V) );

  VECTOR( b1, a0, (&V) );
  d2 = VDOT( (&V), (&V) );

  if( e2 < EPS2 && d2 < EPS2 )   /* identical edges */
    {
    sum = b * b * (log( b * b ) - 3.0);
    }
  else
    {                          /* non-identical edges */
    double c2, f2;
    if( e2 > EPS2 )
      sum += e2 - e2 * log( e2 );
    if( d2 > EPS2 )
      sum += d2 - d2 * log( d2 );
    
    VECTOR( b0, a0, (&V) );
    c2 = VDOT( (&V), (&V) );
    if( c2 > EPS2 )
      sum += c2 * log( c2 ) - c2;
    
    VECTOR( b1, a1, (&V) );
    f2 = VDOT( (&V), (&V) );
    if( f2 > EPS2 )
      sum += f2 * log( f2 ) - f2;
    sum = 0.5 * sum - 2.0 * a * b;
    }

  return sum;

  }  /* end V1LIxact */

/***  ViewALI.c  *************************************************************/

/*  Compute direct interchange area by adaptive single line integral 
 *  (Mitalas-Stephensen) method */

double ViewALI( const int nv1, const Vec3 *v1,
            const int nv2, const Vec3 *v2, View3DControlData *vfCtrl )
/* nv1 - number of vertices/edges of polygon 1.
 * v1  - vector of vertices of polygon 1.
 * nv2 - number of vertices/edges of polygon 2.
 * v2  - vector of vertices of polygon 2.
 */
  {
  Vec3 A[MAXNV1]; /* edges of polygon 1 */
  double a[MAXNV1]; /* lengths of polygon 1 edges */
  double sum, sumt; /* double because of large +/- operations */
  int i, im1,    /* surface 1 edge index */
     j, jm1;    /* surface 2 edge index */

#if( DEBUG > 0 )
  if( nv1>MAXNV1 )
    errorf( 2, __FILE__, __LINE__, "MAXNV1 too small ", "" );
#endif
#if( DEBUG > 1 )
  fprintf( _ulog, "Begin ViewALI():\n" );
#endif
  im1 = nv1 - 1;
  for( i=0; i<nv1; im1=i++ )     /* for all edges of polygon 1 */
    {
    VECTOR( (v1+im1), (v1+i), (&A[i]) );
    a[i] = VLEN( (&A[i]) );
#if( DEBUG > 0 )
    if( a[i] < EPS )
      errorf( 2, __FILE__, __LINE__, "small edge (a) ", FltStr(a[i],6), "" );
#endif
    }

  jm1 = nv2 - 1;
  for( sum=0.0,j=0; j<nv2; jm1=j++ )   /* for all edges of polygon 2 */
    {
    Vec3 B;  /* edge of polygon 2 */
    double b, b2;    /* length  and length^2 of edge */
    double dot;      /* dot product of edges i and j */

    VECTOR( (v2+jm1), (v2+j), (&B) );
    b2 = VDOT( (&B), (&B) );
    b = sqrt( b2 );
#if( DEBUG > 0 )
    if( b < EPS )
      errorf( 2, __FILE__, __LINE__, "small edge (b) ", FltStr(b,6), "" );
#endif

    im1 = nv1 - 1;
    for( i=0; i<nv1; im1=i++ )     /* for all edges of polygon 1 */
      {
      Vec3 V[3];  /* vertices of edge i */
      double dF[3];
      int flag1, flag2;

#if( DEBUG > 2 )
      fprintf( _ulog, "Srf1 %d-%d (%f %f %f) to (%f %f %f)\n", i, ip1,
        v1[i].x, v1[i].y, v1[i].z, v1[ip1].x, v1[ip1].y, v1[ip1].z );
      fprintf( _ulog, "Srf2 %d-%d (%f %f %f) to (%f %f %f)\n", j, jp1,
        v2[j].x, v2[j].y, v2[j].z, v2[jp1].x, v2[jp1].y, v2[jp1].z );
#endif
      dot = VDOT( (&B), (A+i) ) / ( b * a[i] );
      if( fabs(dot) <= EPS ) continue;
#if( DEBUG > 1 )
      fprintf( _ulog, " ViewALI: j=%d i=%d b %f a %f dot %f\n",
        j, i, b, a[i], dot );
#endif
      VCOPY( (v1+im1), (V+0) );
      dF[0] = V1LIpart( V+0, v2+jm1, v2+j, &B, b2, &flag1 );

      VCOPY( (v1+i), (V+2) );
      dF[2] = V1LIpart( V+2, v2+jm1, v2+j, &B, b2, &flag2 );
      vfCtrl->usedV1LIadapt += 2;

      if( flag1 + flag2 == 2 )    /* analytic integration */
        {
        sumt = V1LIxact( v1+im1, v1+i, a[i], v2+jm1, v2+j, b );
        }
      else                        /* adaptive integration */
        {
        VMID( (V+0), (V+2), (V+1) );
        dF[1] = V1LIpart( V+1, v2+jm1, v2+j, &B, b2, &flag1 );
        vfCtrl->usedV1LIadapt += 1;
        sumt = V1LIadapt( V, dF, a[i]/6.0, v2+jm1, v2+j,
                          &B, b2, 0, vfCtrl ) / b;
        }
      sum += dot * sumt;
#if( DEBUG > 1 )
      fprintf( _ulog, "ALI: i %d j %d dot %f t %f sum %f\n",
        j, i, dot, sumt, sum );
#endif
      }  /* end i loop */
    }  /* end j loop */

  sum *= PIt4inv;          /* divide by 4*pi */
#if( DEBUG > 1 )
  fprintf( _ulog, "ViewALI AF: %g\n", sum );
#endif

  return sum;

  }  /* end of ViewALI */

/***  ViewsInit.c  ***********************************************************/

/*  Allocate / free arrays local to this file based on INIT.
 *  Initialize Gaussian integration coefficients.  
 *  Store G coefficients in vectors emulating triangular arrays.  */

void ViewsInit( int maxDiv, int init )
  {
  static int maxRC1;    /* max number of values in RC1 */
  static int maxRC2;    /* max number of values in RC2 */
  static int maxDV1;    /* max number of values in DV1 */
  static int maxDV2;    /* max number of values in DV2 */

  if( init )
    {
    maxRC1 = MAXNV1;
    _rc1 = Alc_V( 0, maxRC1, sizeof(EdgeDir), __FILE__, __LINE__ );
    maxDV1 = maxDiv - 1;
    _dv1 = Alc_MC( 0, maxRC1, 0, maxDV1, sizeof(EdgeDivision), __FILE__, __LINE__ );
    maxRC2 = _maxNVT;  // MAXNVT @@@ needs work; 2005/11/02;
    _rc2 = Alc_V( 0, maxRC2, sizeof(EdgeDir), __FILE__, __LINE__ );
    maxDV2 = maxDiv - 1;
    _dv2 = Alc_MC( 0, maxRC2, 0, maxDV2, sizeof(EdgeDivision), __FILE__, __LINE__ );
    }

  else
    {
    Fre_MC( _dv2, 0, maxRC2, 0, maxDV2, sizeof(EdgeDivision), __FILE__, __LINE__ );
    Fre_V( _rc2, 0, maxRC2, sizeof(EdgeDir), __FILE__, __LINE__ );
    Fre_MC( _dv1, 0, maxRC1, 0, maxDV1, sizeof(EdgeDivision), __FILE__, __LINE__ );
    Fre_V( _rc1, 0, maxRC1, sizeof(EdgeDir), __FILE__, __LINE__ );
    fprintf( _ulog, "Total line integral points evaluated:    %8lu\n",
      _usedV1LIpart );
    }

  }  /* end ViewsInit */

  const double _gqx[10] = {    /* Gaussian ordinates */
     0.500000000, 0.211324865, 0.788675135, 0.112701665, 0.500000000, 
     0.887298335, 0.069431844, 0.330009478, 0.669990522, 0.930568156 };
  const double _gqw[10] = {    /* Gaussian weights */
     1.000000000, 0.500000000, 0.500000000, 0.277777778, 0.444444444,
     0.277777778, 0.173927423, 0.326072577, 0.326072577, 0.173927423 };
  const int _offset[4] = { 0, 1, 3, 6 };

/***  DivideEdges.c  *********************************************************/

/*  Divide edges of a polygon for Gaussian quadrature, 1 <= nDiv <= 4.  */

int DivideEdges( int nDiv, int nVrt, Vec3 *Vrt, EdgeDir *rc, EdgeDivision **dv )
/* nDiv  - number of divisions per edge.
 * nVrt  - number of vertices/edges.
 * Vrt  - coordinates of vertices.
 * rc  - direction cosines of each edge.
 * dv  - edge divisions for Gaussian quadrature.
 */
  {
  Vec3 V; /* vector betweeen successive vertices */
  double s;    /* distance between vertices */
  int i, im1, j, n;

#if( DEBUG > 1 )
  fprintf( _ulog, "DIVEDGE:  nd=%d\n", nDiv );
  DumpP3D( "Polygon:", nVrt, Vrt );
#endif
#if( DEBUG > 0 )
  if( nDiv > 4 )
    {
    errorf( 2, __FILE__, __LINE__, "nDiv > 4", "" );
    nDiv = 4;
    }
#endif

  n = _offset[nDiv-1];

  im1 = nVrt - 1;
  for( i=0; i<nVrt; im1=i++ )  /* for all edges */
    {
    VECTOR( (Vrt+im1), (Vrt+i), (&V) );
    rc[i].s = s = VLEN( (&V) );
    rc[i].x = V.x / s;
    rc[i].y = V.y / s;
    rc[i].z = V.z / s;
    for( j=0; j<nDiv; j++ )  /* divide each edge */
      {
      dv[i][j].x = Vrt[im1].x + _gqx[n+j] * V.x;
      dv[i][j].y = Vrt[im1].y + _gqx[n+j] * V.y;
      dv[i][j].z = Vrt[im1].z + _gqx[n+j] * V.z;
      dv[i][j].s = _gqw[n+j] * s;
      }
    }

#if( DEBUG > 1 )
  fprintf( _ulog, "rc:       x            y            z            s\n" );
  for( i=0; i<nVrt; i++ )
    fprintf( _ulog, "%2d %12.5f %12.5f %12.5f %12.5f\n", i,
      rc[i].x, rc[i].y, rc[i].z, rc[i].s );
  fprintf( _ulog, "dv:       x            y            z            s\n" );
  for( i=0; i<nVrt; i++ )
    for( j=0; j<nDiv; j++ )
      fprintf( _ulog, "%2d %12.5f %12.5f %12.5f %12.5f\n", j,
        dv[i][j].x, dv[i][j].y, dv[i][j].z, dv[i][j].s );
  fflush( _ulog );
#endif
  return nDiv;

  }  /* end of DivideEdges */

/***  GQParallelogram.c  *****************************************************/

/*  Compute Gaussian integration values for a parallelogram.  
 *
 *           v[3] *----------*----------*-----*----------* v[2]
 *               /   ...    /   ...    / ... /  p[nn-1] /
 *              *----------*----------*-----*----------*
 *             /   ...    /   ...    / ... /   ...    /
 *            *----------*----------*-----*----------*
 *           /   p[n]   /  p[n+1]  / ... /   ...    /
 *          *----------*----------*-----*----------*
 *         /   p[0]   /   p[1]   / ... /  p[n-1]  /
 *   v[0] *----------*----------*-----*----------* v[1]
 */

int GQParallelogram( const int nDiv, const Vec3 *vp, Vec3 *p, double *w )
/* nDiv - division factor
 * vp   - vertices of parallelogram
 * p    - coordinates of Gaussian points
 * w    - Gaussian weights */
  {
  Vec3 v0,  /* vector from v[0] to v[3] */
           v1,  /* vector from v[1] to v[2] */
           v2;  /* vector from pt0 to pt1 */
  Vec3 pt0, pt1; /* Gaussian points on v0 and v1 */
  static const int nss[4] = { 1, 4, 9, 16 };
  int nSubSrf;   /* number of subsurfaces */
  int i, j, n;

  n = _offset[nDiv-1];
  nSubSrf = nss[nDiv-1];
  VECTOR( (vp+0), (vp+3), (&v0) );
  VECTOR( (vp+1), (vp+2), (&v1) );

  for( i=0; i<nDiv; i++ )
    {
    pt0.x = vp[0].x + v0.x * _gqx[n+i];
    pt0.y = vp[0].y + v0.y * _gqx[n+i];
    pt0.z = vp[0].z + v0.z * _gqx[n+i];
    pt1.x = vp[1].x + v1.x * _gqx[n+i];
    pt1.y = vp[1].y + v1.y * _gqx[n+i];
    pt1.z = vp[1].z + v1.z * _gqx[n+i];
    VECTOR( (&pt0), (&pt1), (&v2) );
    for( j=0; j<nDiv; j++,p++,w++ )
      {
      p->x = pt0.x + v2.x * _gqx[n+j];
      p->y = pt0.y + v2.y * _gqx[n+j];
      p->z = pt0.z + v2.z * _gqx[n+j];
      *w = _gqw[n+i] * _gqw[n+j];     /* not correct for a */
      }                               /* general quadrilateral */
    }

  return nSubSrf;

  }  /* end GQParallelogram */

/***  SubSrf.c  **************************************************************/

/*  Set Gaussian integration values for triangle or rectangle.  */

int SubSrf( const int nDiv, const int nv, const Vec3 *Sv, const double area,
  Vec3 *Gpt, double *wt )
/* nDiv - division factor
 * nv   - number of vertices, 3 or 4
 * Sv   - coordinates of vertices
 * area - of triangle or rectangle
 * Gpt  - coordinates of Gaussian points
 * wt   - Gaussian weights */
  {
  int nSubSrf;       /* number of subsurfaces */
  int n;

  if( nv == 3 )
    nSubSrf = GQTriangle( nDiv, Sv, Gpt, wt );
  else
    nSubSrf = GQParallelogram( nDiv, Sv, Gpt, wt );

  for( n=0; n<nSubSrf; n++ )
    wt[n] *= area;

#ifdef XXX
  fprintf( _ulog, "SubSrf: polygon vertices\n" );
  fprintf( _ulog, "   #     x         y         z\n" );
  for( n=0; n<nv; n++ )
    fprintf( _ulog, "%3d, %9.6f %9.6f %9.6f\n",
      n, Sv[n].x, Sv[n].y, Sv[n].z );
  fprintf( _ulog, " integration points\n" );
  fprintf( _ulog, "   #     x         y         z         w\n" );
  for( n=0; n<nSubSrf; n++ )
    fprintf( _ulog, "%3d, %9.6f %9.6f %9.6f %9.6f\n",
      n, Gpt[n].x, Gpt[n].y, Gpt[n].z, wt[n] );
  error( 3, __FILE__, __LINE__, "end test", "" );
#endif

  return nSubSrf;

  }  /* end SubSrf */

/***  GQTriangle.c  **********************************************************/

/*  Gaussian integration values for a triangle, 1 <= nDiv <= 4.
 *  Return the coordinates of Point N and its associated weighting.  */

int GQTriangle( const int nDiv, const Vec3 *vt, Vec3 *p, double *w )
  {
/* nDiv - division factor
 * vt   - vertices of triangle
 * p    - coordinates of Gaussian points
 * w    - Gaussian weights */
  static const int offset[4] = { 0, 1, 5, 12 };
  static const double gx[25][4] = {  /* Gaussian ordinates & weights */
     {0.33333333, 0.33333333, 0.33333333, 1.00000000 },  /* 1-point */

     {0.33333333, 0.33333333, 0.33333333, -0.5625000 },  /* 4-point */
     {0.60000000, 0.20000000, 0.20000000, 0.52083333 },
     {0.20000000, 0.60000000, 0.20000000, 0.52083333 },
     {0.20000000, 0.20000000, 0.60000000, 0.52083333 },

     {0.33333333, 0.33333333, 0.33333333, 0.22500000 },  /* 7-point */
     {0.05971587, 0.47014206, 0.47014206, 0.13239415 },
     {0.47014206, 0.05971587, 0.47014206, 0.13239415 },
     {0.47014206, 0.47014206, 0.05971587, 0.13239415 },
     {0.79742699, 0.10128651, 0.10128651, 0.12593918 },
     {0.10128651, 0.79742699, 0.10128651, 0.12593918 },
     {0.10128651, 0.10128651, 0.79742699, 0.12593918 },
     
     {0.33333333, 0.33333333, 0.33333333, -0.14957004 }, /* 13-point */
     {0.47930807, 0.26034597, 0.26034597, 0.17561526 },
     {0.26034597, 0.47930807, 0.26034597, 0.17561526 },
     {0.26034597, 0.26034597, 0.47930807, 0.17561526 },
     {0.86973979, 0.06513010, 0.06513010, 0.05334724 },
     {0.06513010, 0.86973979, 0.06513010, 0.05334724 },
     {0.06513010, 0.06513010, 0.86973979, 0.05334724 },
     {0.63844419, 0.31286550, 0.04869031, 0.07711376 },
     {0.63844419, 0.04869031, 0.31286550, 0.07711376 },
     {0.31286550, 0.63844419, 0.04869031, 0.07711376 },
     {0.31286550, 0.04869031, 0.63844419, 0.07711376 },
     {0.04869031, 0.63844419, 0.31286550, 0.07711376 },
     {0.04869031, 0.31286550, 0.63844419, 0.07711376 }
                             };  /* Gaussian ordinates & weights */
  static const int nss[4] = { 1, 4, 7, 13 };  /* number of subsurfaces */
  int nSubSrf;       /* number of subsurfaces */
  int j, n;

  n = offset[nDiv-1];
  nSubSrf = nss[nDiv-1];
  for( j=0; j<nSubSrf; j++,p++,w++ )
    {
    p->x = gx[j+n][0] * vt[0].x + gx[j+n][1] * vt[1].x + gx[j+n][2] * vt[2].x;
    p->y = gx[j+n][0] * vt[0].y + gx[j+n][1] * vt[1].y + gx[j+n][2] * vt[2].y;
    p->z = gx[j+n][0] * vt[0].z + gx[j+n][1] * vt[1].z + gx[j+n][2] * vt[2].z;
    *w = gx[j+n][3];
    }

  return nSubSrf;

  }  /* end GQTriangle */

