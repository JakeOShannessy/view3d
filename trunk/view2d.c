/*subfile:  view2d.c  ********************************************************/

#define V3D_BUILD
#include "view2d.h"

#include <stdio.h>
#include <stdarg.h> /* variable argument list macro definitions */
#include <string.h> /* prototype: memcpy */
#include <math.h>   /* prototypes: fabs, sqrt */
#include <float.h>  /* define: FLT_EPSILON */
#include "types.h"
#include "heap.h"
#include "misc.h"
#include "test2d.h"
#include "test3d.h" /* for DumpOS */

#include <stdlib.h>

#ifdef STKTEST
# ifdef __WATCOMC__
#  include <malloc.h> /* prototype: stackavail */
# endif
#endif

#if 0
extern IX _list;    /* output control, higher value = more output */
extern FILE *_ulog; /* log file */
extern I1 _string[]; /* buffer for a character string */
#endif

/*  redefinition of C variable types and path parameters.  */
#define I1 char
#define I2 short
#define I4 long
#define IX int

#define U1 usigned char
#define U2 unsigned short
#define U4 unsigned long
#define UX unsigned

#define R4 float
#define double double
#define RX long double

extern int _dmem2;   /* list every heap allocation or free */

static int _row=0;   /* row number; save for errorf() */
static int _col=0;   /* column number; " */

  /* Gaussian ordinates and weights */
const double gx3[3] = { 0.112701665, 0.500000000, 0.887298335 };
const double gw3[3] = { 0.277777778, 0.444444444, 0.277777778 };
const double gx4[4] = { 0.069431844, 0.330009478, 0.669990522, 0.930568156 };
const double gw4[4] = { 0.173927423, 0.326072577, 0.326072577, 0.173927423 };

/***  View2D.c  **************************************************************/

/*  Function to compute view factors for 2-dimensional surfaces.  */

/*  The AF array is stored in triangular form:
 *  +--------+--------+--------+--------+----
 *  | [1][1] |   -    |   -    |   -    |  - 
 *  +--------+--------+--------+--------+----
 *  | [2][1] | [2][2] |   -    |   -    |  - 
 *  +--------+--------+--------+--------+----
 *  | [3][1] | [3][2] | [3][3] |   -    |  - 
 *  +--------+--------+--------+--------+----
 *  | [4][1] | [4][2] | [4][3] | [4][4] |  - 
 *  +--------+--------+--------+--------+----
 *  |  ...   |  ...   |  ...   |  ...   | ...
 */

void View2D( SRFDAT2D *srf, double **AF, View2DControlData *vfCtrl )
/* srf    - surface / vertex data for all surfaces
 * AF     - array of Area * F values
 * vfCtrl - control values consolitated into struct
 */
  {
  IX n;  /* row */
  IX m;  /* column */
  IX *possibleObstr;   /* list of possible view obstructing surfaces */
  IX *probableObstr;   /* list of probable view obstructing surfaces */
  IX *orientObstr;     /* orientations of view obstructing surfaces */
  IX nProb;        /* number of probable view obstructing surfaces */
  IX mayView;      /* true if surfaces may view each other */
  SRFDAT2D srfN,   /* row N surface */
           srfM,   /* column M surface */
          *srf1,   /* view from srf1 to srf2 -- */
          *srf2;   /*   one is srfN, the other is srfM. */
  double calcAF;       /* computed AF value */
  UX nAF0=0,       /* number of AF which must equal 0 */
     nAFnO=0,      /* number of AF without obstructing surfaces */
     nAFwO=0,      /* number of AF with obstructing surfaces */
     nObstr=0;     /* total number of obstructions considered */

#ifdef MEMTEST
  if( _dmem2 )
    fprintf( _ulog, "at start of View2D - %s", MemRem( _string ) );
#endif

  possibleObstr = Alc_V( 1, vfCtrl->nAllSrf, sizeof(IX), __FILE__, __LINE__ );
  vfCtrl->nPossObstr = SetPosObstr2D( vfCtrl->nAllSrf, srf, possibleObstr );
  fprintf( _ulog, "    possible obstructions: %3d \n", vfCtrl->nPossObstr );
  if( vfCtrl->nPossObstr>0 )
    DumpOS( "Possible view obstructing surfaces:",
      vfCtrl->nPossObstr, possibleObstr );

  probableObstr = Alc_V( 1, vfCtrl->nPossObstr+1, sizeof(IX), __FILE__, __LINE__ );
  orientObstr = Alc_V( 1, vfCtrl->nPossObstr+1, sizeof(IX), __FILE__, __LINE__ );

  vfCtrl->srfOT = Alc_V( 0, vfCtrl->nPossObstr, sizeof(SRF2D), __FILE__, __LINE__ );
  vfCtrl->lines = Alc_V( 0, vfCtrl->nPossObstr, sizeof(LINE), __FILE__, __LINE__ );
  vfCtrl->failConverge = 0;
  
#ifdef MEMTEST
  if( _dmem2 )
    {
    fprintf( _ulog, "after View2D allocations - %s", MemRem( _string ) );
    MemWalk();
    }
#endif

  for( n=1; n<=vfCtrl->nRadSrf; n++ )  /* process AF values for row n */
    {
    _row = n;
    if( n%10==1 )
      fprintf( stderr, "\nsurface: " );
    fprintf( stderr, " %4d", _row );
    AF[n][n] = 0.0;

    for( m=1; m<n; m++ )   /* compute view factor: row n, column m */
      {
      _col = m;
      if( _list>2 )
        fprintf( _ulog, "*ROW %d, COL %d\n", _row, _col );

                   /* determine view obstructing surfaces */
      mayView = SelfObstructionTest2D( srf+n, srf+m, &srfM );
      if( mayView )
        mayView = SelfObstructionTest2D( srf+m, srf+n, &srfN );

      if( mayView )   /* check potential obstructing surfaces */
        {
        if( srfN.area * srfM.area == 0.0 )  /* must clip one or both surfces */
          {
          if( srfN.area + srfM.area == 0.0 )
            {
            errorf( 1, __FILE__, __LINE__, " Surfaces intersect", "" );
            ClipSrf2D( &srfN, 1 );
            ClipSrf2D( &srfM, 1 );
            }
          else if( srfN.area == 0.0 )
            ClipSrf2D( &srfN, 1 );
          else if( srfM.area == 0.0 )
            ClipSrf2D( &srfM, 1 );
          }

        nProb = vfCtrl->nPossObstr;
        if( nProb )
          {
          memcpy( probableObstr+1, possibleObstr+1, nProb*sizeof(IX) );
          nProb = BoxTest2D( &srfN, &srfM, srf,
            probableObstr, orientObstr, nProb );
          if( nProb < 0 )
            mayView = 0;
          }
        vfCtrl->nProbObstr = nProb;
        }

      if( mayView )           /* compute view factor */
        {
        if( vfCtrl->nProbObstr>0 )
          {
          Vec2 *v1, *v2;    /* vertices of surface 1 */
          double minArea;           /* area of smaller surface */
          if( ProjectionDirection2D( &srfN, &srfM, srf,
              probableObstr, orientObstr, vfCtrl ) > 0 )
            { srf1 = &srfN; srf2 = &srfM; }
          else                  /* set direction of projection */
            { srf1 = &srfM; srf2 = &srfN; }
          if( _list>3 )
            {
            DumpOS( " LOS:", vfCtrl->nProbObstr, probableObstr );
            DumpOS( " OOS:", vfCtrl->nProbObstr, orientObstr );
            }
          CoordTrans2D( srf1, srf2, srf, probableObstr, vfCtrl );
          vfCtrl->failRecursion = 0;
          minArea = MIN( srf1->area, srf2->area );
          vfCtrl->epsAF = (R4)(minArea * vfCtrl->epsObstr);
          v1 = &vfCtrl->srf1T.v1;
          v2 = &vfCtrl->srf1T.v2;
          calcAF = ViewAdapt2D( v1, v2, srf1->area, 0, vfCtrl );
          if( vfCtrl->failRecursion )
            {
            fprintf( _ulog, " row %d, col %d,  recursion did not converge\n",
              _row, _col );
            vfCtrl->failConverge = 1;
            }
          nObstr += vfCtrl->nProbObstr;
          nAFwO += 1;
          }
        else                    /* unobstructed view */
          {
          calcAF = FA1A2( &srfN.v1, &srfN.v2, &srfM.v1, &srfM.v2 );
          nAFnO += 1;
          }
        }
      else                      /* totally obstructed view */
        {
        calcAF = 0.0;
        nAF0 += 1;
        }
                              /* transfer calcAF to AF */
      AF[n][m] = calcAF;

      if( _list>2 )
        {
        fprintf( _ulog, " AF(%d,%d): %f %f %f\n", _row, _col, AF[n][m],
                 AF[n][m] / srf[n].area, AF[n][m] / srf[m].area);
        fflush( _ulog );
        }

      }  /* end of element M of row N */

    }  /* end of row N */
  fputc( '\n', stderr );

  fprintf( _ulog, "\nSurface pairs where F(i,j) must equal zero:%6u\n", nAF0 );
  fprintf( _ulog, "Surface pairs without obstructing surfaces:%6u\n", nAFnO );
  fprintf( _ulog, "Surface pairs with obstructing surfaces:   %6u\n", nAFwO );
  if( nAFwO>0 )
    fprintf( _ulog, "Average number of obstructions per view:   %6.2f\n",
             (double)nObstr / (double)nAFwO );
  fprintf( _ulog, "Adaptive integration evaluations used: %10lu\n",
    vfCtrl->usedVObs );
  fprintf( _ulog, "Adaptive integration evaluations lost: %10lu\n",
    vfCtrl->wastedVObs );
  if( vfCtrl->failConverge ) error( 2, __FILE__, __LINE__,
    " Some calculations did not converge, see VIEW2D.LOG", "" );

#ifdef MEMTEST
  if( _dmem2 )
    MemWalk();
#endif

  Fre_V( vfCtrl->lines, 0, vfCtrl->nPossObstr, sizeof(LINE), __FILE__, __LINE__ );
  Fre_V( vfCtrl->srfOT, 0, vfCtrl->nPossObstr, sizeof(SRF2D), __FILE__, __LINE__ );
  Fre_V( orientObstr, 1, vfCtrl->nPossObstr+1, sizeof(IX), __FILE__, __LINE__ );
  Fre_V( probableObstr, 1, vfCtrl->nPossObstr+1, sizeof(IX), __FILE__, __LINE__ );
  Fre_V( possibleObstr, 1, vfCtrl->nAllSrf, sizeof(IX), __FILE__, __LINE__ );
#ifdef MEMTEST
  if( _dmem2 )
    fprintf( _ulog, "at end of View2D - %s", MemRem( _string ) );
#endif

  }  /* end of View2D */

/***  FA1A2.c  ***************************************************************/

/*  Compute the radiation shape factor between flat surfaces A1 and A2 
 *  (2D geometry) using Hottel's "crossed-strings" method.
 *  Ref:  Hottel & Sarofim, "Radiative Transfer", 1967, p33.
 *  Using vector operations plus four SQRT functions per call.
 *  ( 4 sqrt, 9 *, 14 +- typical )
 */

double FA1A2( Vec2 *v1a, Vec2 *v1b, Vec2 *v2a, Vec2 *v2b )
/*  v1a  coordinates of first vertex of surface A1
 *  v1b  coordinates of second vertex of surface A1
 *  v2a  coordinates of first vertex of surface A2
 *  v2b  coordinates of second vertex of surface A2 */
  {
  Vec2 v;
  double AF;
                                        /* crossed strings */
  v.x = v2a->x - v1a->x;
  v.y = v2a->y - v1a->y;
  AF  = sqrt( v.x * v.x + v.y * v.y );
  v.x = v2b->x - v1b->x;
  v.y = v2b->y - v1b->y;
  AF += sqrt( v.x * v.x + v.y * v.y );
                                        /* uncrossed strings */
  v.x = v2a->x - v1b->x;
  v.y = v2a->y - v1b->y;
  AF -= sqrt( v.x * v.x + v.y * v.y );
  v.x = v2b->x - v1a->x;
  v.y = v2b->y - v1a->y;
  AF -= sqrt( v.x * v.x + v.y * v.y );

#ifdef DEBUG
  if( _list>4 )
    fprintf( _ulog, " AF %f  v1 (%.3f,%.3f) (%.3f,%.3f)  v1 (%.3f,%.3f) (%.3f,%.3f)\n",
      0.5*AF, v1a->x, v1a->y, v1b->x, v1b->y, v2a->x, v2a->y, v2b->x, v2b->y );
#endif

  return (0.5 * AF);

  }  /* end FA1A2 */

/***  ClipYC.c  **************************************************************/

/*  Clip line  v1 -- v2 so none of it is above y = yc  */

IX ClipYC( Vec2 *v1, Vec2 *v2, const double yc )
/*  v1,v2;  vertices of endpoints
 *  yc;  clipping plane */
  {
  IX flag=0;  /* result of clip (1 = line eliminated)  */

  if( v1->y > yc )
    if( v2->y > yc )
      flag = 1;
    else
      {
      v1->x = v1->x + (yc - v1->y) * (v2->x - v1->x) / (v2->y - v1->y);
      v1->y = yc;
      }
  else
    if( v2->y > yc )
      {
      v2->x = v1->x + (yc - v1->y) * (v2->x - v1->x) / (v2->y - v1->y);
      v2->y = yc;
      }
  return flag;

  }  /*  end of ClipYC  */

/***  ViewObstructed.c  ******************************************************/

/*  Compute the effect of obstructing surfaces on the view from a point on
 *  surface 1 to surface 2 (in vfCtrl).  Shadows of the intervening surfaces
 *  are projected from the point onto surface 2.  Overlapping shadows are
 *  combined.  
 */

double ViewObstructed2D( Vec2 *v1, View2DControlData *vfCtrl )
  {
  LINE base;  /* coordinates of base line */
  LINE new;   /* coordinates of new line */
  LINE used;  /* pointer to first used line */
  LINE free;  /* pointer to first free line */
  LINE *pl;   /* pointer to line data */
  double eps;     /* small value used to prevent accumulation of lines
                  separated by very small spaces */
  IX flag;    /* action taken:  FLAG = 1:  no action;  FLAG = 2:  new line
                  added to list;  FLAG = 3:  lines cover the base;
                  FLAG = 4:  insufficient array space */
  Vec2 vs1, vs2; /* vertices of shadin surface / shadow */
  SRFDAT2D *ps;  /* pointer to surface */
  double x1, x2;  /* ends of projected shadow */
  double yc;      /* height of clipping plane */
  double dF;      /* shape factor from point to shadow */
  IX shadow;  /* true if base is covered by a shadow */
  IX j;

#ifdef DEBUG
  if( _list>5 )
    fprintf( _ulog, " ViewObstructed from point (%f,%f)\n", v1->x, v1->y );
#endif

               /* set up surface 2 coordinates */
  ps = &vfCtrl->srf2T;
  base.xl = ps->v1.x;
  base.xr = ps->v2.x;
  dF = FdA1A2( v1, &vfCtrl->srf1T.dc, &ps->v1, &ps->v2 );  /* view to base */
#ifdef DEBUG
  if( _list>5 )
    fprintf( _ulog, "  base surface: x = %f to %f; dF %g\n",
      base.xl, base.xr, dF );
#endif
  if( v1->y < 0.5e-6 )   /* view point on Y=0 plane */
    return (0.5 * dF);   /* seems to be a problem; therefore cannot
                            use 3-5 Simpson adaptive integration */
  eps = 0.01 * vfCtrl->epsObstr * (base.xr - base.xl);
  yc = 0.999 * v1->y;
  pl = free.next = vfCtrl->lines;
  for( j=0; j<vfCtrl->nProbObstr; j++ )
    {
    pl->next = pl+1;
    pl = pl->next;
    }
  pl->next = 0;
  used.next = NULL;
               /* process all obstructing surfaces */
  for( shadow=0,j=0; j<vfCtrl->nProbObstr; j++ )
    {
    SRF2D *ps=vfCtrl->srfOT + j;
    vs1.x = ps->v1.x;
    vs1.y = ps->v1.y;
    vs2.x = ps->v2.x;
    vs2.y = ps->v2.y;
    if( ClipYC( &vs1, &vs2, yc ) )    /* clip surface */
      continue;
#ifdef DEBUG
    if( _list>5 )
      fprintf( _ulog, "  vs: %d (%f,%f) (%f,%f)\n",
        ps->nr, vs1.x, vs1.y, vs2.x, vs2.y );
#endif
    if( fabs(v1->y - vs2.y) < FLT_EPSILON )
      {
      if( v1->y * (v1->x - vs1.x) < FLT_EPSILON &&
          v1->y * (v1->x - vs2.x) < FLT_EPSILON ) continue;
      fprintf( _ulog, "  vs: (%.5g,%.5g) (%.5g,%.5g)  vp: (%.5g,%.5g)\n",
        vs1.x, vs1.y, vs2.x, vs2.y, v1->x, v1->y );
      errorf( 2, __FILE__, __LINE__, " Projection may fail", "" );
      }
               /* project the obstruction vertices to y=0 */
    x1 = v1->x - v1->y * (v1->x - vs1.x) / (v1->y - vs1.y);
    x2 = v1->x - v1->y * (v1->x - vs2.x) / (v1->y - vs2.y);
    if(x1<x2)
      { new.xl = x1; new.xr = x2; }
    else
      { new.xl = x2; new.xr = x1; }
#ifdef DEBUG
    if( _list>5 )
      fprintf( _ulog, "  line: %f %f\n", new.xl,new.xr );
#endif
    flag = LineOverlap( &base, &new, &used, &free, eps );
#ifdef DEBUG
    if( _list>5 )
      fprintf( _ulog, "  end overlap: %d\n", flag );
#endif
    if(flag>2)
      {
      shadow=1;
      break;
      }

#ifdef DEBUG
    if( _list>5 )
      fflush( _ulog );
#endif
    }  /* end of J (obstruction) loop */

  for( pl=used.next; pl; pl=pl->next )
    {
    double dFs;
    vs1.x = pl->xr;
    vs2.x = pl->xl;
    vs1.y = vs2.y = 0.0;
    dFs = FdA1A2( v1, &vfCtrl->srf1T.dc, &vs1, &vs2 );
    dF -= dFs;       /* subtract views to shadows from view to base */
#ifdef DEBUG
    if( _list>5 )
      fprintf( _ulog, "  dFs %f  (%f)\n", dFs, dF );
#endif
    }

  if( dF < 0.0 ) dF = 0.0;
  if( shadow ) dF = 0.0;

  return dF;

  }  /*  end of ViewObstructed  */

/***  FdA1A2.c  **************************************************************/

/*  Compute the radiation shape factor between infinitesimal 
 *  surface dA1 and flat surface A2 (2D geometry).
 *  Ref:  Hottel & Sarofim, "Radiative Transfer", 1967, p37.
 *  F = 0.5 * ( sin(Q1) - sin(Q2) )   see figure (2-17)
 *  Using vector operations plus two SQRT function calls.
 *  ( 2 sqrt, 2 /, 9 *, 12 +- typical )
 */

double FdA1A2( Vec2 *v1, DirCos2 *u1, Vec2 *v2a, Vec2 *v2b )
/*  v1   coordinates of surface (point) dA1
 *  u1   components of unit vector normal to surface dA1
 *  v2a  coordinates of first vertex of surface A2
 *  v2b  coordinates of second vertex of surface A2 */
  {
  Vec2 v; /* vector v1-->v2a or v1-->v2b */
  double vlen,    /* length of v */
   dF=0.0;    /* accumulated value of FdA1A2 */

  v.x = v2a->x - v1->x;
  v.y = v2a->y - v1->y;
  vlen = sqrt( v.x * v.x + v.y * v.y );
  if( vlen > 0.0 )
    dF += ( v.x * u1->y - v.y * u1->x ) / vlen;
  else
    dF += 1.0;

  v.x = v2b->x - v1->x;
  v.y = v2b->y - v1->y;
  vlen = sqrt( v.x * v.x + v.y * v.y );
  if( vlen > 0.0 )
    dF -= ( v.x * u1->y - v.y * u1->x ) / vlen;
  else
    dF -= 1.0;

  dF = 0.5 * fabs( dF );
  return dF;

  }  /* end FdA1A2 */

/***  LineOverlap.c  *********************************************************/

/*  Process overlapping lines.
 *  The left (XNL) and right (XNR) ends of the new line (XNL < XNR)
 *  are first compared against a base line (XBL, XBR) and possibly
 *  reduced to not be outside of it.  Coordinates (XOL, XOR) of old
 *  lines are stored in no special order.
 *  There are six cases representing the relationship of the new to
 *  the old line:
 *           Case 1:
 *                      --------XNR
 *                                       XOL----------
 *
 *           Case 2:
 *                                       XNL----------
 *                      --------XOR
 *
 *           Case 3:
 *                              XNL------XNR
 *                      XOL------------------------XOR
 *
 *           Case 4:
 *                              XNL----------------XNR
 *                      XOL--------------XOR
 *
 *           Case 5:
 *                      XNL--------------XNR
 *                              XOL----------------XOR
 *
 *           Case 6:
 *                      XNL------------------------XNR
 *                              XOL------XOR
 */

IX LineOverlap( LINE *base, LINE *new, LINE *used, LINE *free, const double eps )
/*  base;  coordinates of base line
 *  new;   coordinates of new line
 *  used;  pointer to list of old (used) lines
 *  free;  pointer to list of free (unused) lines
 *  eps;   small value used to prevent accumulation of lines
 *         separated by very small spaces
 */
  {
  IX flag=1; /* action taken:  FLAG = 1:  no action;  FLAG = 2:  new line
                added to list;  FLAG = 3:  lines cover the base;
                FLAG = 4:  insufficient array space */
  LINE *p0, *p1; /* pointers to prior and current lines */

                         /* Trim new line to limits of base line */
  if(new->xl < base->xl)
    new->xl = base->xl;
  if(new->xr > base->xr)
    new->xr = base->xr;
  if(new->xr < new->xl+eps)
    return flag;
                         /* Overlap new line with old lines in list */
  p0 = used;
  while( (p1=p0->next)!=NULL )
    {
    if(p1->xl >= new->xr+eps)
      p0 = p1;                              /* case 1 */
    else
      if(p1->xr <= new->xl-eps)
        p0 = p1;                            /* case 2 */
      else
        if(p1->xl < new->xl)
          if(p1->xr >= new->xr)
            goto finish;                    /* case 3 */
          else
            {
            new->xl = p1->xl;               /* case 4 */
            p0->next = p1->next;
            p1->next = free->next;
            free->next = p1;
            }
        else
          if(p1->xr > new->xr)
            {
            new->xr = p1->xr;               /* case 5 */
            p0->next = p1->next;
            p1->next = free->next;
            free->next = p1;
            }
          else
            {
            p0->next = p1->next;            /* case 6 */
            p1->next = free->next;
            free->next = p1;
            }
    }  /* end overlap loop */

    if( free->next )                    /* add new line to list */
      {
      p0 = free->next;
      p0->xl = new->xl;
      p0->xr = new->xr;
      free->next = p0->next;
      p0->next = used->next;
      used->next = p0;
      flag = 2;
      }
    else
      flag = 4;
                         /* Test for complete overlap */
  if(new->xl < base->xl+eps && base->xr < new->xr+eps)
    flag = 3;

finish:
  return flag;

  }  /*  end of LineOverlap  */

/***  ViewAdapt2D.c  *********************************************************/

/*  2-D adaptive view factor calculation.  Recursive calculation!  */

double ViewAdapt2D( Vec2 *v1, Vec2 *v2, double area, IX level, View2DControlData *vfCtrl )
/* v1,v2 - endpoints of surface 1;
 * area  - area of surface (distance between endpoints);
 * vfCtrl->maxRecursion - recursion limit. */
  {
  double AF3,      /* AF values for 3-  */
     AF4;      /* and 4-point integration */
  IX cnvg=0;   /* true if both AF.. sufficiently close */
  Vec2 vp; /* coordinates of view point */
  double dx, dy;
  IX n;        /* subsurface number */

  if( level >= vfCtrl->minRecursion )
    {
    dx = v2->x - v1->x;
    dy = v2->y - v1->y;
    for( AF3=0.0,n=0; n<3; n++ )
      {
      vp.x = v1->x + gx3[n] * dx;
      vp.y = v1->y + gx3[n] * dy;
      AF3 += gw3[n] * area * ViewObstructed2D( &vp, vfCtrl );
      }
    for( AF4=0.0,n=0; n<4; n++ )
      {
      vp.x = v1->x + gx4[n] * dx;
      vp.y = v1->y + gx4[n] * dy;
      AF4 += gw4[n] * area * ViewObstructed2D( &vp, vfCtrl );
      }
    }
  else
    {
    AF3 = -vfCtrl->epsAF;
    AF4 = vfCtrl->epsAF;
    }

#ifdef STKTEST
  if( _list>2 )
    {
# if defined __TURBOC__  
    fprintf( _ulog, "  ViewAdapt2D %d (%d) area %f AF3: %f  AF4: %f\n",
      level, _SP, area, AF3, AF4 );
# elif defined __WATCOMC__
    fprintf( _ulog, "  ViewAdapt2D %d (%d) area %f AF3: %f  AF4: %f\n",
      level, stackavail(), area, AF3, AF4 );
# else
    fprintf( _ulog, "  ViewAdapt2D %d area %f AF3: %f  AF4: %f\n",
      level, area, AF3, AF4 );
# endif
    fflush( _ulog );
    }
#endif

  if( fabs(AF4 - AF3) < vfCtrl->epsAF )
    cnvg = 1;
  else if( level++ >= vfCtrl->maxRecursion )
    vfCtrl->failRecursion = cnvg = 1;     /* limit maximum recursions */

  if( level >= vfCtrl->minRecursion )
    {
    vfCtrl->wastedVObs += 3;
    if( cnvg )
      vfCtrl->usedVObs += 4;
    else
      vfCtrl->wastedVObs += 4;
    }

  if( !cnvg )
    {                              /* Divide surface into two */
    vp.x = 0.5 * (v1->x + v2->x);  /* subsurfaces and compute AF */
    vp.y = 0.5 * (v1->y + v2->y);  /* for each subsurface and sum. */
    AF4 = ViewAdapt2D( v1, &vp, 0.5*area, level, vfCtrl )
        + ViewAdapt2D( &vp, v2, 0.5*area, level, vfCtrl );
    }

  return AF4;

  }  /* end ViewAdapt2D */

#ifdef XXX
/***  SimpAdapt.c  ***********************************************************/

/*  Compute integral by adaptive Simpson integration.
 *  This is a recursive calculation!  */

double SimpAdapt( Vec2 Vold[3], double dFold[3], R4 length6,
  IX level, View2DControlData *vfCtrl )
/* length6 = (length of line) / 6.0 */
  {
  Vec2 V[5];
  double dF[5];
  double F3,  /* F using 3-point Simpson integration */
     F5;  /* F using 5-point Simpson integration */
  IX j;

  for( j=0; j<3; j++ )
    {
    V[j+j].x = Vold[j].x;
    V[j+j].y = Vold[j].y;
    dF[j+j] = dFold[j];
    }
  F3 = (dF[0] + 4.0*dF[2] + dF[4]) * length6;

  V[1].x = V[0].x + 0.5 * (V[2].x - V[0].x);
  V[1].y = V[0].y + 0.5 * (V[2].y - V[0].y);
  dF[1] = ViewObstructed2D( V+1, vfCtrl );

  V[3].x = V[2].x + 0.5 * (V[4].x - V[2].x);
  V[3].y = V[2].y + 0.5 * (V[4].y - V[2].y);
  dF[3] = ViewObstructed2D( V+3, vfCtrl );
  length6 *= 0.5;
  F5 = (dF[0] + 4.0*dF[1] + 2.0*dF[2] + 4.0*dF[3] + dF[4]) * length6;
  vfCtrl->usedVObs += 2;
#ifdef DEBUG
  if( _list>5 )
    fprintf( _ulog, " Adapt: %d  F3 %g  F5 %g\n", level, F3, F5 );
#endif

  if( fabs( F5 - F3 ) > vfCtrl->epsAF )  /* test convergence */
    if( ++level > vfCtrl->maxRecursion )     /* limit maximum recursions */
      vfCtrl->failRecursion = 1;
    else             /* one more level of adaptive integration */
      F5 = SimpAdapt( V+0, dF+0, length6, level, vfCtrl )
         + SimpAdapt( V+2, dF+2, length6, level, vfCtrl );

#ifdef DEBUG
  if( vfCtrl->failRecursion )
    errorf( 3, __FILE__, __LINE__, " Recursion failure", "" );
#endif
  return F5;

  }  /* end SimpAdapt */

#endif  /* end XXX */

