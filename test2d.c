/*subfile:  test2d.c  ********************************************************/

#define V3D_BUILD
#include "test2d.h"
#include "view3d.h"

#include <stdio.h>
#include <string.h> /* prototype: memcpy */
#include <math.h>   /* prototype: fabs */
#include <float.h>  /* define: FLT_EPSILON */
#include "types.h" 

#define izero1(n,x) memset(x,0,n*sizeof(int))    /* zero current integers */


/***  BoxTest.c  *************************************************************/

/*  Determine which surfaces might obstruct the view from N to M.
 *  Tests are ordered to eliminate surfaces as quiclky as possible.  */

int BoxTest2D( SRFDAT2D *srfN, SRFDAT2D *srfM, SRFDAT2D *srf,
  int *los, int *oos, int npos )
  {
/* los  - list of probable obstructing surfaces (input and revised as output).
 * oos  - orientations of obstructing surfaces J, 
 *         +1 = J can see only N, -1 = J can see only M; 0 = J can see both.
 * npos - initial number of possible obstructions.  */
  int nos=0; /* number of view obstructing surfaces, -1 = blocked view */
  double dot;
  int u[12];
/*         u[0]  > 0 if vertex 1 of surface N is to left of surface J.
 *         u[1]  > 0 if vertex 2 of surface N is to left of surface J.
 *         u[2]  > 0 if vertex 1 of surface M is to left of surface J.
 *         u[3]  > 0 if vertex 2 of surface M is to left of surface J.
 *         u[4]  > 0 if vertex 1 of surface J is to left of side A.
 *         u[5]  > 0 if vertex 2 of surface J is to left of side A.
 *         u[6]  > 0 if vertex 1 of surface J is to left of side B.
 *         u[7]  > 0 if vertex 2 of surface J is to left of side B.
 *         u[8]  > 0 if vertex 1 of surface J is to left of surface N.
 *         u[9]  > 0 if vertex 2 of surface J is to left of surface N.
 *         u[10] > 0 if vertex 1 of surface J is to left of surface M.
 *         u[11] > 0 if vertex 2 of surface J is to left of surface M.
 *
 *  The basic geometry:
 *                                           surface N
 *                                 (v2) *-------------------* (v1)
 *                (v2) *               /                   /
 *                     |              /                   /
 *           surface J |     side A  /                   /  side B
 *                     |            /                   /
 *                (v1) *           /                   /
 *                           (v1) *-------------------* (v2)
 *                                     surface M
 *
 *  These surfaces must form a convex figure (quadrilateral or
 *  possibly triangle) for the tests to work properly.  Any self-
 *  obstruction which would prevent this is handled by removing part
 *  of surface N and/or M in SelfObstructionTest2D().
 */

#define UDOT(a,b) if(dot>b) a=1; else if(dot<-b) a=-1; else a=0

  SRFDAT2D sideA, sideB;
  int adegen=0;  /* true if side A is degenerate:  start of M = end of N */
  int bdegen=0;  /* true if side B is degenerate:  start of N = end of M */
  int block=0;   /* true is view N-M is totally blocked by surface J */
  int left;      /* true if all vertices of quadrilateral are left of J */
  int right;     /* true if all vertices of quadrilateral are right of J */
  int j, k, n;

                   /* Set up side A */
  sideA.nr = 0;
  sideA.v1.x = srfN->v2.x;
  sideA.v1.y = srfN->v2.y;
  sideA.v2.x = srfM->v1.x;
  sideA.v2.y = srfM->v1.y;
  SetSrf2D( &sideA );
  if( sideA.area < FLT_EPSILON ) adegen = 1;

                   /* Set up side B */
  sideB.nr = 0;
  sideB.v1.x = srfM->v2.x;
  sideB.v1.y = srfM->v2.y;
  sideB.v2.x = srfN->v1.x;
  sideB.v2.y = srfN->v1.y;
  SetSrf( &sideB );
  if( sideB.area < FLT_EPSILON ) bdegen = 1;

  izero1( 12, u );

#ifdef DEBUG
  if( _list>4 )
    {
    fprintf( _ulog, " BoxTest:\n");
    DumpSrf( srfN );
    DumpSrf( srfM );
    DumpSrf( &sideA );
    DumpSrf( &sideB );
    fflush( _ulog );
    }
#endif
                   /* Check every surface J: */
  for( n=1; n<=npos; n++)
    {
    j = los[n];
                     /* Check for N in front of J */
    dot = srf[j].dc.x * srfN->v1.x + srf[j].dc.y * srfN->v1.y + srf[j].dc.w;
    UDOT( u[0], FLT_EPSILON );
    dot = srf[j].dc.x * srfN->v2.x + srf[j].dc.y * srfN->v2.y + srf[j].dc.w;
    UDOT( u[1], FLT_EPSILON );

                     /* Check for M in front of J */
    dot = srf[j].dc.x * srfM->v1.x + srf[j].dc.y * srfM->v1.y + srf[j].dc.w;
    UDOT( u[2], FLT_EPSILON );
    dot = srf[j].dc.x * srfM->v2.x + srf[j].dc.y * srfM->v2.y + srf[j].dc.w;
    UDOT( u[3], FLT_EPSILON );
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "\n obstruction (%d): %2d %2d %2d %2d",
             j, u[0], u[1], u[2], u[3] );
#endif

                     /* Check for J to right of side A */
    if( !adegen )
      {
      dot = sideA.dc.x * srf[j].v1.x + sideA.dc.y * srf[j].v1.y + sideA.dc.w;
      UDOT( u[4], FLT_EPSILON );
      dot = sideA.dc.x * srf[j].v2.x + sideA.dc.y * srf[j].v2.y + sideA.dc.w;
      UDOT( u[5], FLT_EPSILON );
#ifdef DEBUG
    if( _list>4 )
      fprintf( _ulog, " %2d %2d", u[4], u[5] );
#endif
      if( u[4]<=0 && u[5]<=0 ) continue;
      }

                     /* Check for J to right of side B */
    if( !bdegen )
      {
      dot = sideB.dc.x * srf[j].v1.x + sideB.dc.y * srf[j].v1.y + sideB.dc.w;
      UDOT( u[6], FLT_EPSILON );
      dot = sideB.dc.x * srf[j].v2.x + sideB.dc.y * srf[j].v2.y + sideB.dc.w;
      UDOT( u[7], FLT_EPSILON );
#ifdef DEBUG
    if( _list>4 )
      fprintf( _ulog, " %2d %2d", u[6], u[7] );
#endif
      if( u[6]<=0 && u[7]<=0 ) continue;
      }

                     /* Check for J completely behind N */
    dot = srfN->dc.x * srf[j].v1.x + srfN->dc.y * srf[j].v1.y + srfN->dc.w;
    UDOT( u[8], FLT_EPSILON );
    dot = srfN->dc.x * srf[j].v2.x + srfN->dc.y * srf[j].v2.y + srfN->dc.w;
    UDOT( u[9], FLT_EPSILON );
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, " %2d %2d", u[8], u[9] );
#endif
    if( u[8]<=0 && u[9]<=0 ) continue;

                     /* Check for J completely behind M */
    dot = srfM->dc.x * srf[j].v1.x + srfM->dc.y * srf[j].v1.y + srfM->dc.w;
    UDOT( u[10], FLT_EPSILON );
    dot = srfM->dc.x * srf[j].v2.x + srfM->dc.y * srf[j].v2.y + srfM->dc.w;
    UDOT( u[11], FLT_EPSILON );
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, " %2d %2d", u[10], u[11] );
#endif
    if( u[10]<=0 && u[11]<=0 ) continue;

                     /* Check for figure left or right of J */
    right = left = 1;
    for( k=0; k<4; k++)
      {
      if( u[k]>0 ) right = 0;
      if( u[k]<0 ) left = 0;
      }
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "\n  right %d, left %d", right, left );
#endif
    if(right) continue;
    if(left) continue;

                     /* Check for J completely obstructing N-M */
    if(adegen)
      {
      if( u[0]<=0 && u[1]<=0 && u[2]>=0 && u[3]>=0 &&
         u[6]<=0 && u[8]<=0 && u[10]<=0 ) block = 1;
      }
    else if(bdegen)
      {
      if( u[0]<=0 && u[1]<=0 && u[2]>=0 && u[3]>=0 &&
         u[5]<=0 && u[9]<=0 && u[11]<=0 ) block = 1;
      }
    else
      if( u[0]<=0 && u[1]<=0 && u[2]>=0 && u[3]>=0 &&
         u[5]<=0 && u[7]<=0 ) block = 1;
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, ", block %d", block );
#endif
    if(block) break;
                     /* J is an obstructing surface */
    los[++nos] = j;
                     /* determine orientation of J */
    k = 0;
    if( u[0]>0 || u[1]>0 ) k += 1;
    if( u[2]>0 || u[3]>0 ) k -= 1;
    oos[nos] = k;
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "\n  nos %d, oos %d", nos, oos[nos] );
#endif

    } /* end of n (j) loop */

  if( block )
    nos = -1;
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "\n" );
  if( nos>0 && _list>4 )
    {
    DumpOS( " LOS: ", nos, los );
    DumpOS( " OOS: ", nos, oos );
    }
#endif

  return nos;

  }  /*  end of BoxTest  */

/***  ClipSrf2D.c  ***********************************************************/

/*  Clip line according to FLAG and DIST vector.
 *  Ordering of vertices is retained.  */

void ClipSrf2D( SRFDAT2D *srf, const float flag )
/* flag   - save vertices above or below clipping plane
 *          for flag equal to 1.0 or -1.0, respectively. */
  {
  double h;  /* linear interpolation factor */
  Vec2 v;

  h = srf->dist[0] / (srf->dist[1] - srf->dist[0]);

#ifdef XXX
  if( _list>4 )
    {
    fprintf( _ulog, " ClipSrf2D:  %f %f\n", srf->dist[0], srf->dist[1] );
    DumpSrf( srf );
    }
#endif

  if( flag * srf->dist[0] < 0.0 )   /* clip v1 */
    {
    srf->v1.x = srf->v1.x - h * (srf->v2.x - srf->v1.x );
    srf->v1.y = srf->v1.y - h * (srf->v2.y - srf->v1.y );
    }

  if( flag * srf->dist[1] < 0.0 )   /* clip v2 */
    {
    srf->v2.x = srf->v1.x - h * (srf->v2.x - srf->v1.x );
    srf->v2.y = srf->v1.y - h * (srf->v2.y - srf->v1.y );
    }

  v.x = srf->v2.x - srf->v1.x;
  v.y = srf->v2.y - srf->v1.y;
  srf->area = (float)sqrt( v.x * v.x + v.y * v.y );

#ifdef DEBUG
  if( _list>4 )
    DumpSrf( srf );
#endif

  }  /* end of ClipSrf2D */

/***  CoordTrans2D.c  ********************************************************/

/*  Coordinate transformation of all surfaces in the N1-N2 view.
 *  Translate and rotate N1, N2, and all obstructing surfaces by a
 *  transformation which puts the start of N2 at the origin (0,0)
 *  and the end at (A2,0).
 */

void CoordTrans2D( SRFDAT2D *srf1, SRFDAT2D *srf2, SRFDAT2D *srf,
  int *probableObstr, View2DControlData *vfCtrl )
  {
  SRFDAT2D *srf1T;   /* pointer to surface 1 */
  SRFDAT2D *srf2T;   /* pointer to surface 2 */
  SRF2D *srfOT;      /* pointer to obstrucing surface */
  double cosphi, sinphi; /* cosine & sine of rotation angle */
  double x, y;           /* coordinates after translation */
  int i, j;

            /* determine rotation angle */
  cosphi = (srf2->v2.x - srf2->v1.x) / srf2->area;
  sinphi = (srf2->v2.y - srf2->v1.y) / srf2->area;
  if( _list>4 )
    fprintf( _ulog, " CoordTrans2D: %f %f (%f,%f)\n",
      cosphi, sinphi, srf2->v1.x, srf2->v1.y);

            /*  transform surface 1 */
  srf1T = &vfCtrl->srf1T;
  srf1T->nr = srf1->nr;
  x = srf1->v1.x - srf2->v1.x;
  y = srf1->v1.y - srf2->v1.y;
  srf1T->v1.x = x * cosphi + y * sinphi;
  srf1T->v1.y = y * cosphi - x * sinphi;
  x = srf1->v2.x - srf2->v1.x;
  y = srf1->v2.y - srf2->v1.y;
  srf1T->v2.x = x * cosphi + y * sinphi;
  srf1T->v2.y = y * cosphi - x * sinphi;
  SetSrf2D( srf1T );

            /* transform surface 2 */
  srf2T = &vfCtrl->srf2T;
  srf2T->nr = srf2->nr;
  srf2T->v1.x = 0.0;
  srf2T->v1.y = 0.0;
/***
  x = srf2->v2.x - srf2->v1.x;
  y = srf2->v2.y - srf2->v1.y;
  srf2T->v2.x = x * cosphi + y * sinphi;
  srf2T->v2.y = y * cosphi - x * sinphi;
***/
  srf2T->v2.x = srf2T->area = srf2->area;
  srf2T->v2.y = 0.0;

            /* transform all obstructing surfaces */
  for( i=0,j=1; j<=vfCtrl->nProbObstr; j++ )
    {
    SRFDAT2D *ps=srf+probableObstr[j];
    srfOT = vfCtrl->srfOT + i++;
    srfOT->nr = ps->nr;
    x = ps->v1.x - srf2->v1.x;
    y = ps->v1.y - srf2->v1.y;
    srfOT->v1.x = x * cosphi + y * sinphi;
    srfOT->v1.y = y * cosphi - x * sinphi;
    x = ps->v2.x - srf2->v1.x;
    y = ps->v2.y - srf2->v1.y;
    srfOT->v2.x = x * cosphi + y * sinphi;
    srfOT->v2.y = y * cosphi - x * sinphi;
    if( srfOT->v1.y<0.0 || srfOT->v2.y<0.0 )
      if( ClipY0( &srfOT->v1, &srfOT->v2 ) )     /* Clip against Y=0 plane */
        i -= 1;
    }
  vfCtrl->nProbObstr = i;

#ifdef DEBUG
  if( _list>4 )
    {
    DumpSrf( &vfCtrl->srf1T );
    DumpSrf( &vfCtrl->srf2T );
    for( i=0; i<vfCtrl->nProbObstr; i++)
      {
      SRF2D *ps = vfCtrl->srfOT + i;
      fprintf( _ulog, "  obs %d; v1 (%9.4f,%9.4f), v2 (%9.4f,%9.4f)\n",
        ps->nr, ps->v1.x, ps->v1.y, ps->v2.x, ps->v2.y );
      }
    fflush( _ulog );
    }
#endif

  }  /*  end of CoordTrans2D  */

/***  ClipY0.c  **************************************************************/

/*  Clip line so none of it is below y = 0.  */

int ClipY0( Vec2 *v1, Vec2 *v2 )
/*  v;  vertices of endpoints
 *  y0;  clipping line */
  {
  int flag=0;  /* result of clip (1 = line eliminated)  */

  if( v1->y < 0.0 )
    if( v2->y < 0.0 )
      flag = 0;
    else
      {
      v1->x = v1->x - v1->y * (v2->x - v1->x) / (v2->y - v1->y);
      v1->y = 0.0;
      }
  else
    if( v2->y < 0.0 )
      {
      v2->x = v1->x - v1->y * (v2->x - v1->x) / (v2->y - v1->y);
      v2->y = 0.0;
      }
  return flag;

  }  /*  end of ClipY0  */

/***  ProjectionDirection.c  *************************************************/

/*  Set direction of projection of obstruction shadows.
 *  Direction will be from surface 1 to surface 2.  
 *  Want surface 2 large and distance from surface 2
 *  to obstructions to be small. 
 *  Return direction indicator: 1 = n to m, -1 = m to n.
 */

int ProjectionDirection2D( SRFDAT2D *srfN, SRFDAT2D *srfM, SRFDAT2D *srf,
  int *probableObstr, int *orientObstr, View2DControlData *vfCtrl )
/* srf  - data for all surfaces.
 * srfN - data for surface N.
 * srfM - data for surface M.
 * probableObstr  - list of probable obstructing surfaces (input and revised for output).
 * orientObstr  - orientations of obstructing surfaces K, 
 *         +1 = K can see N, -1 = K can see M; 0 = K can see both.
 * ctrl - computation controls.
 */
  {
  int n, k;   /* surface number */
  Vec2 center;
  double dx, dy, dist;  /* components of distance calculation */
  double sdtoN, sdtoM;  /* minimum distances from obstruction to N and M */
  int direction=-1;  /* 1 = N is surface 1; 0 = M is surface 1 */

#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, " Direction:\n");
#endif
    /* for all obstructing surfaces which can see N,
       determine distances from center of K to center of N */
  sdtoN = 1.0e9;
  center.x = 0.5 * (srfN->v1.x + srfN->v2.x);
  center.y = 0.5 * (srfN->v1.y + srfN->v2.y);
  for( n=1; n<=vfCtrl->nProbObstr; n++ )
    {
    if( orientObstr[n]<0 ) continue;
    k = probableObstr[n];
    dx = 0.5 * (srf[k].v1.x + srf[k].v2.x) - center.x;
    dy = 0.5 * (srf[k].v1.y + srf[k].v2.y) - center.y;
    dist = dx * dx + dy * dy;
    if( dist<sdtoN ) sdtoN = dist;
    }
  sdtoN = sqrt( sdtoN );
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "  min dist to srf %d: %e\n", srfN->nr, sdtoN );
#endif
  sdtoN *= srfM->area;

    /* for all obstructing surfaces which can see M,
       determine distances from center of K to center of M */
  sdtoM = 1.0e9;
  center.x = 0.5 * (srfM->v1.x + srfM->v2.x);
  center.y = 0.5 * (srfM->v1.y + srfM->v2.y);
  for( n=1; n<=vfCtrl->nProbObstr; n++ )
    {
    if( orientObstr[n]>0 ) continue;
    k = probableObstr[n];
    dx = 0.5 * (srf[k].v1.x + srf[k].v2.x) - center.x;
    dy = 0.5 * (srf[k].v1.y + srf[k].v2.y) - center.y;
    dist = dx * dx + dy * dy;
    if( dist<sdtoM ) sdtoM = dist;
    }
  sdtoM = sqrt( sdtoM );
#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "  min dist to srf %d: %e\n", srfM->nr, sdtoM );
#endif
  sdtoM *= srfN->area;

    /* direction based on distances and areas of N & M */
  if( sdtoN < sdtoM ) direction = 0;
  if( sdtoN > sdtoM ) direction = 1;

#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "  sdtoN %e, sdtoM %e, dir %d\n",
      sdtoN, sdtoM, direction );
#endif

  if( direction==-1 )   /* direction based only on areas of N & M */
    {
    if( srfN->area <= srfM->area )
      direction = 1;
    else
      direction = 0;
    }

#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, "  areaN %e, areaM %e, dir %d, pdir %d\n",
      srfN->area, srfM->area, direction, vfCtrl->prjReverse );
#endif

  if( vfCtrl->prjReverse )       /* direction reversal parameter */
    direction = !direction;

#ifdef DEBUG
  if( _list>3 )
    if( direction )
      fprintf( _ulog, "  project from srf %d to srf %d\n", srfN->nr, srfM->nr );
    else
      fprintf( _ulog, "  project from srf %d to srf %d\n", srfM->nr, srfN->nr );
#endif

  k = 0;         /* eliminate probableObstr surfaces facing wrong direction */
  if( direction )
    {
    for( n=1; n<=vfCtrl->nProbObstr; n++ )
      if( orientObstr[n]<=0 )
        {
        probableObstr[++k] = probableObstr[n];
        orientObstr[k] = orientObstr[n];
        }
    }
  else
    {
    for( n=1; n<=vfCtrl->nProbObstr; n++ )
      if( orientObstr[n]>=0 )
        {
        probableObstr[++k] = probableObstr[n];
        orientObstr[k] = orientObstr[n];
        }
    }
  vfCtrl->nProbObstr = k;

  return direction;

  }  /*  end of ProjectionDirection  */

/***  SelfObstructionTest2D.c  ***********************************************/

/*  Self-obstruction test -- determine the portion of SRF2 that is 
 *  in front of SRF1; transfer SRF2 data to srfN.
 *  Return 0 if the SRF2 is entirely behind SRF1; return 1 if SRF2 is fully
 *  or partially in front.  If partially in front, srfN->area set to 0.  */

int SelfObstructionTest2D( SRFDAT2D *srf1, SRFDAT2D *srf2, SRFDAT2D *srfN )
  {
  int infront=0; /* true if a vertex of surface 2 is in front of surface 1 */
  int behind=0;  /* true if a vertex of surface 2 is behind surface 1 */
  double eps=1.0e-5*srf2->area; /* test value for "close to plane of SRF1" */

                            /* check both vertices of srf2 against srf1 */
  srf2->dist[0] = srf2->v1.x * srf1->dc.x 
    + srf2->v1.y * srf1->dc.y + srf1->dc.w;
  if( srf2->dist[0] > eps )
    infront = 1;
  else if( srf2->dist[0] < -eps )
    behind = 1;
  else
    srf2->dist[0] = 0.0;

  srf2->dist[1] = srf2->v2.x * srf1->dc.x 
    + srf2->v2.y * srf1->dc.y + srf1->dc.w;
  if( srf2->dist[1] > eps )
    infront = 1;
  else if( srf2->dist[1] < -eps )
    behind = 1;
  else
    srf2->dist[1] = 0.0;

  if( infront )
    {
    memcpy( srfN, srf2, sizeof(SRFDAT2D) );     /* transfer data */
    if( behind )
      srfN->area = 0.0;   /* flag for later calculation of area */
#ifdef DEBUG
    if( _list>4 )
      DumpSrf( srfN );
#endif
    return 1;
    }
  else                    /* all vertices of srf2 on or behind srf1 plane */
    return 0;

  }  /*  end of SelfObstructionTest2D  */

/***  SetSrf.c  **************************************************************/

/*  Set surface area and direction cosine values from vertices.  */

void SetSrf2D( SRFDAT2D *srf ){
  Vec2 v; /* vector v1-->v2 */

  v.x = srf->v2.x - srf->v1.x;
  v.y = srf->v2.y - srf->v1.y;
  srf->area = sqrt( v.x * v.x + v.y * v.y );
  if( srf->area > FLT_EPSILON )
    {
    srf->dc.x = -v.y / srf->area;
    srf->dc.y = v.x / srf->area;
    v.x = 0.5 * (srf->v2.x + srf->v1.x);  /* (x,y) = midpoint */
    v.y = 0.5 * (srf->v2.y + srf->v1.y);
    srf->dc.w = -(v.x * srf->dc.x + v.y * srf->dc.y);
    }
  else
    srf->area = 0.0;

}  /* end SetSrf */

/***  SetPosObstr2D.c  *******************************************************/

/*  Set list of possible view obstructing surfaces.
 *  Return number of possible view obstructing surfaces.  */

int SetPosObstr2D( int nSrf, SRFDAT2D *srf, int *possibleObstr )
/* nSrf;  total number of surfaces ('S' and 'O')
 * srf;   vector of surface data [1:nSrf]
 * possibleObstr;  vector of possible view obtructions [1:nSrf]
 */
  {
  int ns;       /* surface number */
  int n;        /* surface number */
  int infront;  /* true if a vertex is in front of surface ns */
  int behind;   /* true if a vertex is behind surface ns */
  double dot, eps; /* dot product and test value */
  int npos=0;   /* number of possible view obstructing surfaces */

  for( ns=nSrf; ns; ns-- )  /* reverse order for obstruction surfaces first */
    {
    if( srf[ns].type < 0 ) continue;    /* skip subsurfaces */
    eps = 1.0e-5 * srf[ns].area;
    infront = behind = 0;
    for( n=nSrf; n; n-- )
      {
      if( srf[n].type < 0 ) continue;  /* surface included in another */
      dot = srf[n].v1.x * srf[ns].dc.x 
          + srf[n].v1.y * srf[ns].dc.y + srf[ns].dc.w;
      if( dot > eps ) infront = 1;
      if( dot < -eps ) behind = 1;
      dot = srf[n].v2.x * srf[ns].dc.x 
          + srf[n].v2.y * srf[ns].dc.y + srf[ns].dc.w;
      if( dot > eps ) infront = 1;
      if( dot < -eps ) behind = 1;
      if( infront && behind ) break;
      }  /* end n loop */

    if( n )                    /* some vertices in front and some behind */
      {                        /* surface ns ==> ns may obstruct a view. */
      npos += 1;
      for( n=npos; n>1; n-- )  /* insertion sort: largest surfaces first */
        if( srf[possibleObstr[n-1]].area < srf[ns].area )
          possibleObstr[n] = possibleObstr[n-1];
        else
          break;
      possibleObstr[n] = ns;
      }
    }  /* end ns loop */

  return npos;

  }  /*  end of SetPosObstr2D  */

#ifdef DEBUG
/***  DumpSrf.c  *************************************************************/

void DumpSrf( SRFDAT2D *srf )
  {
  fprintf( _ulog, "  srf %d; v1 (%9.4f,%9.4f), v2 (%9.4f,%9.4f), area %9.4f\n",
    srf->nr, srf->v1.x, srf->v1.y, srf->v2.x, srf->v2.y, srf->area );

  }  /* end DumpSrf */
#endif

#ifdef XXX
/***  ClipLine.c  ************************************************************/

/*  Clip line according to FLAG and DIST vector.
 *  Ordering of vertices is retained.  */

void ClipLine( Vec2 *v0, Vec2 *v1, const double dist[2], const float flag )
/* v0,v1  - end points of line.
 * dist   - distance of each vertex above clipping plane; found by dot
 *          product of vertex with clipping plane normal form coefficients.
 * flag   - save vertices above or below clipping plane
 *          for flag equal to 1.0 or -1.0, respectively. */
  {
  double h;  /* linear interpolation factor */

  h = dist[1] / (dist[1] - dist[0]);   /* no division by zero */

  if( flag * dist[0] < 0.0 )   /* clip v0 */
    {
    v0->x = v0->x + h * (v1->x - v0->x );
    v0->y = v0->y + h * (v1->y - v0->y );
    }

  if( flag * dist[1] < 0.0 )   /* clip v1 */
    {
    v1->x = v0->x + h * (v1->x - v0->x );
    v1->y = v0->y + h * (v1->y - v0->y );
    }

  }  /* end of ClipLine */

/***  CT2D.c  ****************************************************************/

/*  Coordinate transformation of 2D vertices.
 *  NV vertices/coordinates are stored contiguously:
 *     [x0][y0][x1][y1][x2] ...
 *  Transform:  {q} = [T]{p}.
 *     | q0 |   | t00 t01 t02 |   | p0 |  (x coordinate)
 *     | q1 | = | t10 t11 t12 | * | p1 |  (y coordinate)
 *     | 1. |   | 0.0 0.0 1.0 |   | 1. |  (assumed)
 *  (Constants in row 3 are assumed)
 */

void CT2D( const int nv, const float t[3][2], const Vec2 *p, Vec2 *q )
  {
  int n;

  for( n=nv; n; n--,p++,q++ )
    {
    q->x = t[0][0] * p->x + t[0][1] * p->y + t[0][2];
    q->y = t[1][0] * p->x + t[1][1] * p->y + t[1][2];
    }

  }  /* end of CT2D */

#endif  /* end XXX */

