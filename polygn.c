/*subfile:  Polygn.c  ********************************************************/

/* The functions in this file maintain:
 *   a stack of free vertex/edge structures,
 *   a stack of free polygon structures, and
 *   one or more stacks of defined polygon structures.
 * Only one defined polygon stack may be created at a time.
 * However, multiple stacks may be saved by using external pointers. */

#ifdef _DEBUG
# define DEBUG 1
#else
# define DEBUG 1
#endif

#define V3D_BUILD
#include "polygn.h"

#include <stdio.h>
#include <string.h> /* prototype: memset, memcpy */
#include <math.h>   /* prototype: fabs */
#include <limits.h> /* define UINT_MAX */
#include "types.h" 
#include "view3d.h"
#include "misc.h"
#include "heap.h"

static int TransferVrt(Vec2 *toVrt, const Vec2 *fromVrt, int nFromVrt);

char *_memPoly=NULL; /* memory block for polygon descriptions; must start NULL */
PolygonVertexEdge *_nextFreeVE; /* pointer to next free vertex/edge */
Polygon *_nextFreePD; /* pointer to next free polygon descripton */
Polygon *_nextUsedPD; /* pointer to top-of-stack used polygon */
double _epsDist;   /* minimum distance between vertices */
double _epsArea;   /* minimum surface area */

/*  Extensive use is made of 'homogeneous coordinates' (HC) which are not 
 *  familiar to most engineers.  The important properties of HC are 
 *  summarized below:
 *  A 2-D point (X,Y) is represented by a 3-element vector (w*X,w*Y,w), 
 *  where w may be any real number except 0.  A line is also represented 
 *  by a 3-element vector (a,b,c).  The directed line from point 
 *  (w*X1,w*Y1,w) to point (v*X2,v*Y2,v) is given by 
 *    (a,b,c) = (w*X1,w*Y1,w) cross (v*X2,v*Y2,v). 
 *  The sequence of the cross product is a convention to determine sign. 
 *  A point lies on a line if (a,b,c) dot (w*X,w*Y,w) = 0. 
 *  'Normalize' the representation of a point by setting w to 1.  Then 
 *  if (a,b,c) dot (X,Y,1) > 0.0, the point is to the left of the line. 
 *  If it is less than zero, the point is to the right of the line. 
 *  The point where two lines intersect is given by
 *    (w*X,w*Y,w) = (a1,b1,c1) cross (a2,b2,c2).
 *  Reference:  W.M. Newman and R.F. Sproull, "Principles of Interactive
 *  Computer Graphics", Appendix II, McGraw-Hill, 1973.  */

/***  PolygonOverlap.c  ******************************************************/

/*  Process two convex polygons.  Vertices are in clockwise sequence!!
 *
 *  There are three processing options -- savePD:
 *  (1)  Determine the portion of polygon P2 which is within polygon P1; 
 *       this may create one convex polygon.
 *  (3)  Determine the portion of polygon P2 which is outside polygon P1; 
 *       this may create one or more convex polygons.
 *  (2)  Do both (1) and (3).
 *
 *  If freeP2 is true, free P2 from its stack before exiting.
 *  Return 0 if P2 outside P1, 1 if P2 inside P1, 2 for partial overlap.
 */

int PolygonOverlap(const Polygon *p1, Polygon *p2, const int savePD, int freeP2){
  Polygon *pp;     /* pointer to polygon */
  Polygon *initUsedPD;  /* initial top-of-stack pointer */
  PolygonVertexEdge *pv1;    /* pointer to edge of P1 */
  int nLeftVrt;  /* number of vertices to left of edge */
  int nRightVrt; /* number of vertices to right of edge */
  int nTempVrt;  /* number of vertices of temporary polygon */
  Vec2 *leftVrt;  /* coordinates of vertices to left of edge */
  Vec2 *rightVrt; /* coordinates of vertices to right of edge */
  Vec2 *tempVrt;  /* coordinates of temporary polygon */
  int *u=NULL;  /* +1 = vertex left of edge; -1 = vertex right of edge */
  int overlap=0; /* 0: P2 outside P1; 1: P2 inside P1; 2: part overlap */
  int j, jm1;    /* vertex indices;  jm1 = j - 1 */

  /* Allocate some memory for the vertex coordinates */
  leftVrt = Alc_V( 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );
  rightVrt = Alc_V( 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );
  tempVrt = Alc_V( 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );
  u = Alc_V( 0, _maxNVT, sizeof(int), __FILE__, __LINE__ );

#if( DEBUG > 1 )
  fprintf( _ulog, "PolygonOverlap:  P1 [%p]  P2 [%p]  flag %d\n",
    p1, p2, savePD );
#endif

  initUsedPD = _nextUsedPD;
  nTempVrt = GetPolygonVrt2D( p2, tempVrt );

#if( DEBUG > 1 )
  DumpP2D( "P2:", nTempVrt, tempVrt );
#endif

  pv1 = p1->firstVE;
  do{  /*  process tempVrt against each edge of P1 (long loop) */
       /*  transfer tempVrt into leftVrt and/or rightVrt  */
    double a1, b1, c1; /* HC for current edge of P1 */
/*    int u[MAXNVT]; */ /* +1 = vertex left of edge; -1 = vertex right of edge */
    int left=1;     /* true if all vertices left of edge */
    int right=1;    /* true if all vertices right of edge */
#if( DEBUG > 1 )
    fprintf(_ulog, "Test against edge of P1\nU:" );
#endif
 
        /* compute and save u[j] - relations of vertices to edge */
    a1 = pv1->a; b1 = pv1->b; c1 = pv1->c;
    pv1 = pv1->next;
    for( j=0; j<nTempVrt; j++ )
      {
      double dot = tempVrt[j].x * a1 + tempVrt[j].y * b1 + c1;
      if( dot > _epsArea )
        { u[j] = 1; right = 0; }
      else if( dot < -_epsArea )
        { u[j] = -1; left = 0; }
      else
        u[j] = 0;
#if( DEBUG > 1 )
      fprintf( _ulog, " %d", u[j] );
#endif
      }
#if( DEBUG > 1 )
    fprintf( _ulog, "\nQuick test:  right %d; left %d;\n", right, left );
#endif
 
        /* use quick tests to skip unnecessary calculations */
    if( right ) continue;
    if( left ) goto p2_outside_p1;

        /* check each vertex of tempVrt against current edge of P1 */
    jm1 = nTempVrt - 1;
    for( nLeftVrt=nRightVrt=j=0; j<nTempVrt; jm1=j++ )    /* short loop */
      {
      if( u[jm1]*u[j] < 0 )  /* vertices j-1 & j on opposite sides of edge */
        {                             /* compute intercept of edges */
        double a, b, c, w; /* HC intersection components */
        a = tempVrt[jm1].y - tempVrt[j].y;
        b = tempVrt[j].x - tempVrt[jm1].x;
        c = tempVrt[j].y * tempVrt[jm1].x - tempVrt[jm1].y * tempVrt[j].x;
        w = b * a1 - a * b1;
#if( DEBUG > 1 )
        if( fabs(w) < _epsArea*(a+b+c) )
          {
          errorf( 1, __FILE__, __LINE__, "small W", "" );
          DumpHC( "P1:", p1, p1 );
          DumpHC( "P2:", p2, p2 );
          fprintf( _ulog, "a, b, c, w: %g %g %g %g\n", a, b, c, w );
          fflush( _ulog );
          fprintf( _ulog, "x, y: %g %g\n", (c*b1-b*c1)/w, (a*c1-c*a1)/w );
          }
#endif
#if( DEBUG > 0 )
        if( w == 0.0 ) errorf( 3, __FILE__, __LINE__,
          " Would divide by zero (w=0)", "" );
#endif
        rightVrt[nRightVrt].x = leftVrt[nLeftVrt].x = ( c*b1 - b*c1 ) / w;
        rightVrt[nRightVrt++].y = leftVrt[nLeftVrt++].y = ( a*c1 - c*a1 ) / w;
        }
      if( u[j] >= 0 )        /* vertex j is on or left of edge */
        {
        leftVrt[nLeftVrt].x = tempVrt[j].x;
        leftVrt[nLeftVrt++].y = tempVrt[j].y;
        }
      if( u[j] <= 0 )        /* vertex j is on or right of edge */
        {
        rightVrt[nRightVrt].x = tempVrt[j].x;
        rightVrt[nRightVrt++].y = tempVrt[j].y;
        }
      }  /* end of short loop */

#if( DEBUG > 1 )
    DumpP2D( "Left polygon:", nLeftVrt, leftVrt );
    DumpP2D( "Right polygon:", nRightVrt, rightVrt );
#endif
/*    if( nLeftVrt >= _maxNVT || nRightVrt >= _maxNVT ) */
/*      errorf( 3, __FILE__, __LINE__, "Parameter _maxNVT too small", "" ); */
    if( nLeftVrt >= _maxNVT )
      {
      errorf( 2, __FILE__, __LINE__,
        "Parameter maxV (", IntStr(_maxNVT), ") too small", "" );
      DumpP2D( "Offending Polygon:", nLeftVrt, leftVrt );
      overlap = -999;
      goto finish;
      }
    if( nRightVrt >= _maxNVT )
      {
      errorf( 2, __FILE__, __LINE__,
        "Parameter maxV (", IntStr(_maxNVT), ") too small", "" );
      DumpP2D( "Offending Polygon:", nRightVrt, rightVrt );
      overlap = -999;
      goto finish;
      }

    if( savePD > 1 )  /* transfer left vertices to outside polygon */
      {
      nTempVrt = TransferVrt( tempVrt, leftVrt, nLeftVrt );
#if( DEBUG > 1 )
      DumpP2D( "Outside polygon:", nTempVrt, tempVrt );
#endif
      if( nTempVrt > 2 )
        {
        SetPolygonHC( nTempVrt, tempVrt, p2->trns );
        overlap = 1;
        }
      }

                      /* transfer right side vertices to tempVrt */
    nTempVrt = TransferVrt( tempVrt, rightVrt, nRightVrt );
#if( DEBUG > 1 )
    DumpP2D( "Inside polygon:", nTempVrt, tempVrt );
#endif
    if( nTempVrt < 2 ) /* 2 instead of 3 allows degenerate P2; espArea = 0 */
      goto p2_outside_p1;

    }  while( pv1 != p1->firstVE );    /* end of P1 long loop */

  /* At this point tempVrt contains the overlap of P1 and P2. */

  if( savePD < 3 )    /* save the overlap polygon */
    {
#if( DEBUG > 1 )
    DumpP2D( "Overlap polygon:", nTempVrt, tempVrt );
#endif
    pp = SetPolygonHC( nTempVrt, tempVrt, p2->trns * p1->trns );
    if( pp==NULL && savePD==2 )   /* overlap area too small */
      goto p2_outside_p1;
    }
  overlap += 1;
  goto finish;

p2_outside_p1:     /* no overlap between P1 and P2 */
  overlap = 0;
#if( DEBUG > 1 )
  fprintf( _ulog, "P2 outside P1\n" );
#endif
  if( savePD > 1 )    /* save outside polygon - P2 */
    {
    if( initUsedPD != _nextUsedPD )  /* remove previous outside polygons */
      FreePolygons( _nextUsedPD, initUsedPD );

    if( freeP2 )         /* transfer P2 to new stack */
      {
      pp = p2;
      freeP2 = 0;               /* P2 already freed */
      }
    else                 /* copy P2 to new stack */
      {
      PolygonVertexEdge *pv, *pv2;
      pp = GetPolygonHC();      /* get cleared polygon data area */
      pp->area = p2->area;      /* copy P2 data */
      pp->trns = p2->trns;
      pv2 = p2->firstVE;
      do{
        if( pp->firstVE )
          pv = pv->next = GetVrtEdgeHC();
        else
          pv = pp->firstVE = GetVrtEdgeHC();
        memcpy( pv, pv2, sizeof(PolygonVertexEdge) );   /* copy vertex/edge data */
        pv2 = pv2->next;
        } while( pv2 != p2->firstVE );
      pv->next = pp->firstVE;   /* complete circular list */
#if( DEBUG > 1 )
    DumpHC( "COPIED SURFACE:", pp, pp );
#endif
      }
    pp->next = initUsedPD;   /* link PP to stack */
    _nextUsedPD = pp;
    }

finish:
  if( freeP2 )   /* transfer P2 to free space */
    FreePolygons( p2, p2->next );

  Fre_V( u, 0, _maxNVT, sizeof(int), __FILE__, __LINE__ );
  Fre_V( tempVrt, 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );
  Fre_V( rightVrt, 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );
  Fre_V( leftVrt, 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );

  return overlap;

  }  /* end of PolygonOverlap */

/***  TransferVrt.c  *********************************************************/

/*  Transfer vertices from polygon fromVrt to polygon toVrt eliminating nearly
 *  duplicate vertices.  Closeness of vertices determined by _epsDist.  
 *  Return number of vertices in polygon toVrt.  */

int TransferVrt(Vec2 *toVrt, const Vec2 *fromVrt, int nFromVrt){
  int j,  /* index to vertex in polygon fromVrt */
    jm1, /* = j - 1 */
     n;  /* index to vertex in polygon toVrt */

  jm1 = nFromVrt - 1;
  for( n=j=0; j<nFromVrt; jm1=j++ )
    if( fabs(fromVrt[j].x - fromVrt[jm1].x) > _epsDist ||
        fabs(fromVrt[j].y - fromVrt[jm1].y) > _epsDist )
      {               /* transfer to toVrt */
      toVrt[n].x = fromVrt[j].x;
      toVrt[n++].y = fromVrt[j].y;
      }
    else if( n>0 )    /* close: average with prior toVrt vertex */
      {
      toVrt[n-1].x = 0.5f * (toVrt[n-1].x + fromVrt[j].x);
      toVrt[n-1].y = 0.5f * (toVrt[n-1].y + fromVrt[j].y);
      }
    else              /* (n=0) average with prior fromVrt vertex */
      {
      toVrt[n].x = 0.5f * (fromVrt[jm1].x + fromVrt[j].x);
      toVrt[n++].y = 0.5f * (fromVrt[jm1].y + fromVrt[j].y);
      nFromVrt -= 1;  /* do not examine last vertex again */
      }

  return n;

  }  /* end TransferVrt */

/***  SetPolygonHC.c  ********************************************************/

/*  Set up polygon including homogeneous coordinates of edges.
 *  Return NULL if polygon area too small; otherwise return pointer to polygon.
 */

Polygon *SetPolygonHC( const int nVrt, const Vec2 *polyVrt, const double trns )
/* nVrt    - number of vertices (vertices in clockwise sequence);
 * polyVrt - X,Y coordinates of vertices (1st vertex not repeated at end),
             index from 0 to nVrt-1. */
  {
  Polygon *pp;    /* pointer to polygon */
  PolygonVertexEdge *pv;    /* pointer to HC vertices/edges */
  double area=0.0; /* polygon area */
  int j, jm1;   /* vertex indices;  jm1 = j - 1 */

  pp = GetPolygonHC();      /* get cleared polygon data area */
#if( DEBUG > 1 )
  fprintf( _ulog, " SetPolygonHC:  pp [%p]  nv %d\n", pp, nVrt );
#endif

  jm1 = nVrt - 1;
  for( j=0; j<nVrt; jm1=j++ )  /* loop through vertices */
    {
    if( pp->firstVE )
      pv = pv->next = GetVrtEdgeHC();
    else
      pv = pp->firstVE = GetVrtEdgeHC();
    pv->x = polyVrt[j].x;
    pv->y = polyVrt[j].y;
    pv->a = polyVrt[jm1].y - polyVrt[j].y; /* compute HC values */
    pv->b = polyVrt[j].x - polyVrt[jm1].x;
    pv->c = polyVrt[j].y * polyVrt[jm1].x - polyVrt[j].x * polyVrt[jm1].y;
    area -= pv->c;
    }
  pv->next = pp->firstVE;    /* complete circular list */

  pp->area = 0.5 * area;
  pp->trns = trns;
#if( DEBUG > 1 )
  fprintf( _ulog, "  areas:  %f  %f,  trns:  %f\n",
             pp->area, _epsArea, pp->trns );
  fflush( _ulog );
#endif

  if( pp->area < _epsArea )  /* polygon too small to save */
    {
    FreePolygons( pp, NULL );
    pp = NULL;
    }
  else
    {
    pp->next = _nextUsedPD;     /* link polygon to current list */
    _nextUsedPD = pp;           /* prepare for next linked polygon */
    }

  return pp;

  }  /*  end of SetPolygonHC  */

/***  GetPolygonHC.c  ********************************************************/

/*  Return pointer to a cleared polygon structure.  
 *  This is taken from the list of unused structures if possible.  
 *  Otherwise, a new structure will be allocated.  */

Polygon *GetPolygonHC( void )
  {
  Polygon *pp;  /* pointer to polygon structure */

  if( _nextFreePD )
    {
    pp = _nextFreePD;
    _nextFreePD = _nextFreePD->next;
    memset( pp, 0, sizeof(Polygon) );  /* clear pointers */
    }
  else
    pp = Alc_EC( &_memPoly, sizeof(Polygon), __FILE__, __LINE__ );

  return pp;

  }  /* end GetPolygonHC */

/***  GetVrtEdgeHC.c  ********************************************************/

/*  Return pointer to an uncleared vertex/edge structure.  
 *  This is taken from the list of unused structures if possible.  
 *  Otherwise, a new structure will be allocated.  */

PolygonVertexEdge *GetVrtEdgeHC( void )
  {
  PolygonVertexEdge *pv;  /* pointer to vertex/edge structure */

  if( _nextFreeVE )
    {
    pv = _nextFreeVE;
    _nextFreeVE = _nextFreeVE->next;
    }
  else
    pv = Alc_EC( &_memPoly, sizeof(PolygonVertexEdge), __FILE__, __LINE__ );

  return pv;

  }  /* end GetVrtEdgeHC */

/***  FreePolygons.c  ********************************************************/

/*  Restore list of polygon descriptions to free space.  */

void FreePolygons( Polygon *first, Polygon *last )
/* first;  - pointer to first polygon in linked list (not NULL).
 * last;   - pointer to polygon AFTER last one freed (NULL = complete list). */
  {
  Polygon *pp; /* pointer to polygon */
  PolygonVertexEdge *pv; /* pointer to edge/vertex */

  for( pp=first; ; pp=pp->next )
    {
#if( DEBUG > 0 )
    if( !pp ) error( 3, __FILE__, __LINE__, "Polygon PP not defined", "" );
    if( !pp->firstVE ) error( 3, __FILE__, __LINE__, "FirstVE not defined", "" );
#endif
    pv = pp->firstVE->next;           /* free vertices (circular list) */
    while( pv->next != pp->firstVE )  /* find "end" of vertex list */
      pv = pv->next;
    pv->next = _nextFreeVE;           /* reset vertex links */
    _nextFreeVE = pp->firstVE;
    if( pp->next == last ) break;
    }
  pp->next = _nextFreePD;       /* reset polygon links */
  _nextFreePD = first;

  }  /*  end of FreePolygons  */

/***  NewPolygonStack.c  *****************************************************/

/*  Start a new stack (linked list) of polygons.  */

void NewPolygonStack( void )
  {
  _nextUsedPD = NULL;  /* define bottom of stack */

  }  /* end NewPolygonStack */

/***  TopOfPolygonStack.c  ***************************************************/

/*  Return pointer to top of active polygon stack.  */

Polygon *TopOfPolygonStack( void )
  {
  return _nextUsedPD;

  }  /* end TopOfPolygonStack */

/***  GetPolygonVrt2D.c  *****************************************************/

/*  Get polygon vertices.  Return number of vertices.
 *  Be sure polyVrt is sufficiently long.  */

int GetPolygonVrt2D( const Polygon *pp, Vec2 *polyVrt )
  {
  PolygonVertexEdge *pv;    /* pointer to HC vertices/edges */
  int j=0;      /* vertex counter */

  pv = pp->firstVE;
  do{
    polyVrt[j].x = pv->x;
    polyVrt[j++].y = pv->y;
    pv = pv->next;
    } while( pv != pp->firstVE );

  return j;

  }  /*  end of GetPolygonVrt2D  */

/***  GetPolygonVrt3D.c  *****************************************************/

/*  Get polygon vertices.  Return number of vertices.
 *  Be sure polyVrt is sufficiently long.  */

int GetPolygonVrt3D( const Polygon *pp, Vec3 *polyVrt )
  {
  PolygonVertexEdge *pv;    /* pointer to HC vertices/edges */
  int j=0;      /* vertex counter */

  pv = pp->firstVE;
  do{
    polyVrt[j].x = pv->x;
    polyVrt[j].y = pv->y;
    polyVrt[j++].z = 0.0;
    pv = pv->next;
    } while( pv != pp->firstVE );

  return j;

  }  /*  end of GetPolygonVrt3D  */

/***  InitPolygonMem.c  ******************************************************/

/*  Initialize polygon processing memory and globals.  */

void InitPolygonMem( const double epsdist, const double epsarea )
  {
  if( _memPoly )  /* clear existing polygon structures data */
    _memPoly = Clr_EC( _memPoly, __FILE__, __LINE__ );
  else            /* allocate polygon structures heap pointer */
    _memPoly = Alc_ECI( 8000, __FILE__, __LINE__ );

  _epsDist = epsdist;
  _epsArea = epsarea;
  _nextFreeVE = NULL;
  _nextFreePD = NULL;
  _nextUsedPD = NULL;
#if( DEBUG > 1 )
  fprintf( _ulog, "InitPolygonMem: epsDist %g epsArea %g at [%p]\n",
    _epsDist, _epsArea, _memPoly );
#endif

  }  /* end InitPolygonMem */

/***  FreePolygonMem.c  ******************************************************/

/*  Free polygon processing memory.  */

void FreePolygonMem( void )
  {
  if( _memPoly )
    _memPoly = (char *)Fre_EC( _memPoly, __FILE__, __LINE__ );

  }  /* end FreePolygonMem */

/***  LimitPolygon.c  ********************************************************/

/*  This function limits the polygon coordinates to a rectangle which encloses
 *  the base surface. Vertices may be in clockwise or counter-clockwise order.
 *  The polygon is checked against each edge of the window, with the portion
 *  inside the edge saved in the tempVrt or polyVrt array in turn.
 *  The code is long, but the loops are short.  
 *  Return the number of number of vertices in the clipped polygon
 *  and the clipped vertices in polyVrt.
 */

int LimitPolygon( int nVrt, Vec2 polyVrt[],
  const double maxX, const double minX, const double maxY, const double minY )
  {
/*  Vec2 tempVrt[MAXNV2]; */ /* temporary vertices */
  int n, m;  /* vertex index */

                         /* test vertices against maxX */
  Vec2 *tempVrt = Alc_V( 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );

  polyVrt[nVrt].x = polyVrt[0].x;
  polyVrt[nVrt].y = polyVrt[0].y; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( polyVrt[n].x < maxX)
      {
      tempVrt[m].x = polyVrt[n].x;
      tempVrt[m++].y = polyVrt[n].y;
      if( polyVrt[n+1].x > maxX)
        {
        tempVrt[m].x = maxX;
        tempVrt[m++].y = polyVrt[n].y + (maxX - polyVrt[n].x)
          * (polyVrt[n+1].y - polyVrt[n].y) / (polyVrt[n+1].x - polyVrt[n].x);
        }
      }
    else if( polyVrt[n].x > maxX )
      {
      if ( polyVrt[n+1].x < maxX )
        {
        tempVrt[m].x = maxX;
        tempVrt[m++].y = polyVrt[n].y + (maxX - polyVrt[n].x)
          * (polyVrt[n+1].y - polyVrt[n].y) / (polyVrt[n+1].x - polyVrt[n].x);
        }
      }
    else
      {
      tempVrt[m].x = polyVrt[n].x;
      tempVrt[m++].y = polyVrt[n].y;
      }
    }  /* end of maxX test */
  nVrt = m;
  if( nVrt < 3 )
    return 0;
                         /* test vertices against minX */
  tempVrt[nVrt].x = tempVrt[0].x;
  tempVrt[nVrt].y = tempVrt[0].y; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( tempVrt[n].x > minX)
      {
      polyVrt[m].x = tempVrt[n].x;
      polyVrt[m++].y = tempVrt[n].y;
      if( tempVrt[n+1].x < minX)
        {
        polyVrt[m].x = minX;
        polyVrt[m++].y = tempVrt[n].y + (minX - tempVrt[n].x)
          * (tempVrt[n+1].y - tempVrt[n].y) / (tempVrt[n+1].x - tempVrt[n].x);
        }
      }
    else if( tempVrt[n].x < minX )
      {
      if ( tempVrt[n+1].x > minX )
        {
        polyVrt[m].x = minX;
        polyVrt[m++].y = tempVrt[n].y + (minX - tempVrt[n].x)
          * (tempVrt[n+1].y - tempVrt[n].y) / (tempVrt[n+1].x - tempVrt[n].x);
        }
      }
    else
      {
      polyVrt[m].x = tempVrt[n].x;
      polyVrt[m++].y = tempVrt[n].y;
      }
    }  /* end of minX test */
  nVrt = m;
  if( nVrt < 3 )
    return 0;
                         /* test vertices against maxY */
  polyVrt[nVrt].y = polyVrt[0].y;
  polyVrt[nVrt].x = polyVrt[0].x; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( polyVrt[n].y < maxY)
      {
      tempVrt[m].y = polyVrt[n].y;
      tempVrt[m++].x = polyVrt[n].x;
      if( polyVrt[n+1].y > maxY)
        {
        tempVrt[m].y = maxY;
        tempVrt[m++].x = polyVrt[n].x + (maxY - polyVrt[n].y)
          * (polyVrt[n+1].x - polyVrt[n].x) / (polyVrt[n+1].y - polyVrt[n].y);
        }
      }
    else if( polyVrt[n].y > maxY )
      {
      if ( polyVrt[n+1].y < maxY )
        {
        tempVrt[m].y = maxY;
        tempVrt[m++].x = polyVrt[n].x + (maxY - polyVrt[n].y)
          * (polyVrt[n+1].x - polyVrt[n].x) / (polyVrt[n+1].y - polyVrt[n].y);
        }
      }
    else
      {
      tempVrt[m].y = polyVrt[n].y;
      tempVrt[m++].x = polyVrt[n].x;
      }
    }  /* end of maxY test */
  nVrt = m;
  if( nVrt < 3 )
    return 0;
                         /* test vertices against minY */
  tempVrt[nVrt].y = tempVrt[0].y;
  tempVrt[nVrt].x = tempVrt[0].x; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( tempVrt[n].y > minY)
      {
      polyVrt[m].y = tempVrt[n].y;
      polyVrt[m++].x = tempVrt[n].x;
      if( tempVrt[n+1].y < minY)
        {
        polyVrt[m].y = minY;
        polyVrt[m++].x = tempVrt[n].x + (minY - tempVrt[n].y)
          * (tempVrt[n+1].x - tempVrt[n].x) / (tempVrt[n+1].y - tempVrt[n].y);
        }
      }
    else if( tempVrt[n].y < minY )
      {
      if ( tempVrt[n+1].y > minY )
        {
        polyVrt[m].y = minY;
        polyVrt[m++].x = tempVrt[n].x + (minY - tempVrt[n].y)
          * (tempVrt[n+1].x - tempVrt[n].x) / (tempVrt[n+1].y - tempVrt[n].y);
        }
      }
    else
      {
      polyVrt[m].y = tempVrt[n].y;
      polyVrt[m++].x = tempVrt[n].x;
      }
    }  /* end of minY test */
  nVrt = m;
  tempVrt = Fre_V( tempVrt, 0, _maxNVT, sizeof(Vec2), __FILE__, __LINE__ );
  if( nVrt < 3 )
    return 0;
  return nVrt;    /* note: final results are in polyVrt */

  }  /*  end of LimitPolygon  */

#if( DEBUG > 0 )
/***  DumpHC.c  **************************************************************/

/*  Print the descriptions of sequential polygons.  */

void DumpHC( char *title, const Polygon *pfp, const Polygon *plp ){
/*  pfp, plp; pointers to first and last (NULL acceptable) polygons  */
  const Polygon *pp;
  PolygonVertexEdge *pv;
  int i, j;

  fprintf( _ulog, "%s\n", title );
  for( i=0,pp=pfp; pp; pp=pp->next )  /* polygon loop */
    {
    fprintf( _ulog, " pd [%p]", pp );
    fprintf( _ulog, "  area %.4g", pp->area );
    fprintf( _ulog, "  trns %.3g", pp->trns );
    fprintf( _ulog, "  next [%p]", pp->next );
    fprintf( _ulog, "  fve [%p]\n", pp->firstVE );
    if( ++i >= 100 ) error( 3, __FILE__, __LINE__, "Too many surfaces", "" );

    j = 0;
    pv = pp->firstVE;
    do{                  /* vertex/edge loop */
      fprintf( _ulog, "  ve [%p] %10.7f %10.7f %10.7f %10.7f %13.8f\n",
               pv, pv->x, pv->y, pv->a, pv->b, pv->c );
      pv = pv->next;
      if( ++j >= _maxNVT ) error( 3, __FILE__, __LINE__, "Too many vertices", "" );
      } while( pv != pp->firstVE );

    if( pp==plp ) break;
    }
  fflush( _ulog );
}  /* end of DumpHC */


void DumpFreePolygons( void ){
  Polygon *pp;

  fprintf( _ulog, "FREE POLYGONS:" );
  for( pp=_nextFreePD; pp; pp=pp->next )
    fprintf( _ulog, " [%p]", pp );
  fprintf( _ulog, "\n" );
}  /* end DumpFreePolygons */


void DumpFreeVertices( void ){
  PolygonVertexEdge *pv;

  fprintf( _ulog, "FREE VERTICES:" );
  for( pv=_nextFreeVE; pv; pv=pv->next )
    fprintf( _ulog, " [%p]", pv );
  fprintf( _ulog, "\n" );
}  /* end DumpFreeVertices */
#endif  /* end DEBUG > 0 */

/***  DumpP3D.c  *************************************************************/

/*  Dump 3-D polygon vertex data. */

void DumpP3D( char *title, const int nvs, Vec3 *vs )
  {
  int n;

  fprintf( _ulog, "%s\n", title );
  fprintf( _ulog, " nvs: %d\tX\tY\tZ\n", nvs );
  for( n=0; n<nvs; n++)
    fprintf( _ulog, "%4d\t%12.7f\t%12.7f\t%12.7f\n",
             n, vs[n].x, vs[n].y, vs[n].z );
  fflush( _ulog );

  }  /* end of DumpP3D */

/***  DumpP2D.c  *************************************************************/

/*  Dump 2-D polygon vertex data.  */

void DumpP2D( char *title, const int nvs, Vec2 *vs )
  {
  int n;

  fprintf( _ulog, "%s\n", title );
  fprintf( _ulog, " nvs: %d\tX\tY\n", nvs );
  for( n=0; n<nvs; n++)
    fprintf( _ulog, "%4d\t%12.7f\t%12.7f\n", n, vs[n].x, vs[n].y );
  fflush( _ulog );

  }  /* end of DumpP2D */
