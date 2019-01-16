/*subfile:  View3D.c  ********************************************************/

#ifdef _DEBUG
# define DEBUG 1
#else
# define DEBUG 1
#endif

#define V3D_BUILD
#include "view3d.h"

#include <stdio.h>
#include <stdarg.h> /* variable argument list macro definitions */
#include <stdlib.h> /* prototype: exit */
#include <string.h> /* prototype: memcpy */
#include <math.h>   /* prototypes: fabs, sqrt */
#include <float.h>  /* define: FLT_EPSILON */
#include "types.h"
#include "misc.h"
#include "heap.h"
#include "viewunob.h"
#include "viewobs.h"
#include "test3d.h"
#include "ctrans.h"

void ViewMethod( SRFDATNM *srfN, SRFDATNM *srfM, double distNM, View3DControlData *vfCtrl );
void InitViewMethod( View3DControlData *vfCtrl );

extern int _list;    /* output control, higher value = more output */
extern FILE *_ulog; /* log file */

int _row=0;  /* row number; save for errorf() */
int _col=0;  /* column number; " */
double _sli4;   /* use SLI if rcRatio > 4 and relSep > _sli4 */
double _sai4;   /* use SAI if rcRatio > 4 and relSep > _sai4 */
double _sai10;  /* use SAI if rcRatio > 10 and relSep > _sai10 */
double _dai1;   /* use DAI if relSep > _dai1 */
double _sli1;   /* use SLI if relSep > _sli1 */

/***  View3D.c  **************************************************************/

/*  Driver function to compute view factors for 3-dimensional surfaces.  */

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

void View3D( SRFDAT3D *srf, const int *base, int *possibleObstr,
             double **AF, View3DControlData *vfCtrl )
/* srf    - surface / vertex data for all surfaces
 * base   - base surface numbers
 * possibleObstr - list of possible view obstructing surfaces
 * AF     - array of Area * F values
 * vfCtrl - control values consolitated into structure
 */
  {
  int n;  /* row */
  int m;  /* column */
  int n1=1, nn;     /* first and last rows */
  int m1=1, mm;     /* first and last columns */
  int *maskSrf;     /* list of mask and null surfaces */
  int *possibleObstrN;  /* list of possible obstructions rel. to N */
  int *probableObstr;   /* list of probable obstructions */
  int nPossN;       /* number of possible obstructions rel. to N */
  int nProb;        /* number of probable obstructions */
  int mayView;      /* true if surfaces may view each other */
  SRFDATNM srfN,   /* row N surface */
           srfM,   /* column M surface */
          *srf1,   /* view from srf1 to srf2 -- */
          *srf2;   /*   one is srfN, the other is srfM. */
  Vec3 vNM;    /* vector between centroids of srfN and srfM */
  double distNM;       /* distance between centroids of srfN and srfM */
  double minArea;      /* area of smaller surface */
  unsigned long nAF0=0,       /* number of AF which must equal 0 */
     nAFnO=0,      /* number of AF without obstructing surfaces */
     nAFwO=0,      /* number of AF with obstructing surfaces */
     nObstr=0;     /* total number of obstructions considered */
  unsigned **bins;       /* for statistical summary */
  double nAFtot=1;     /* total number of view factors to compute */
  int maxSrfT=4;    /* max number of participating (transformed) surfaces */

  maxSrfT = vfCtrl->nPossObstr + 1;
  nn = vfCtrl->nRadSrf;
  if( nn>1 )
    nAFtot = (double)((nn-1)*nn);
  if( vfCtrl->row > 0 )
    {
    n1 = nn = vfCtrl->row;   /* can process a single row of view factors, */
    if( vfCtrl->col > 0 )
      m1 = vfCtrl->col;      /* or a single view factor, */
    }

  EdgeData dummyEdgeData;
  EdgeData edgeData = ViewsInit( 4, 1, dummyEdgeData);  /* initialize Gaussian integration coefficients */
  InitViewMethod( vfCtrl );

  possibleObstrN = Alc_V( 1, vfCtrl->nAllSrf, sizeof(int), __FILE__, __LINE__ );
  probableObstr = Alc_V( 1, vfCtrl->nAllSrf, sizeof(int), __FILE__, __LINE__ );

  vfCtrl->srfOT = Alc_V( 0, maxSrfT, sizeof(SRFDAT3X), __FILE__, __LINE__ );
  bins = Alc_MC( 0, 4, 1, 5, sizeof(unsigned), __FILE__, __LINE__ );
  vfCtrl->failConverge = 0;

  if( vfCtrl->nMaskSrf ) /* pre-process view masking surfaces */
    {
    maskSrf = Alc_V( 1, vfCtrl->nMaskSrf, sizeof(int), __FILE__, __LINE__ );
    for( m=1,n=vfCtrl->nRadSrf; n; n-- )   /* set mask list */
      if( srf[n].type == MASK || srf[n].type == NULS )
        maskSrf[m++] = n;
    DumpOS( "Mask and Null surfaces:", vfCtrl->nMaskSrf, maskSrf );

    for( n=n1; n<=nn; n++ )
      {
      if( vfCtrl->col )
        mm = m1 + 1;
      else
        mm = n;
      for( m=m1; m<mm; m++ )   /* set all AF involving mask/null */
        if( srf[n].type == MASK || srf[n].type == NULS )
          if( base[n] == m )
            AF[n][m] = srf[n].area;
          else
            AF[n][m] = 0.0;
        else if( srf[m].type == MASK || srf[m].type == NULS )
          AF[n][m] = 0.0;
        else
          AF[n][m] = -1.0;     /* set AF flag values */
      }
    }

  for( n=n1; n<=nn; n++ )  /* process AF values for row N */
    {
    _row = n;
    if( vfCtrl->row == 0 )  /* progress display - all surfaces */
      {
      double pctDone = 100 * (double)((n-1)*n) / nAFtot;
#ifdef LOGGING
      fprintf( stderr, "\rSurface: %d; ~ %.1f %% complete", _row, pctDone );
#endif
      }
    AF[n][n] = 0.0;
    nPossN = vfCtrl->nPossObstr;  /* remove obstructions behind N */
    memcpy( possibleObstrN+1, possibleObstr+1, nPossN*sizeof(int) );
    nPossN = OrientationTestN( srf, n, vfCtrl, possibleObstrN, nPossN );
    if( vfCtrl->col )  /* set column limits */
      mm = m1 + 1;
    else if( vfCtrl->row > 0 )
      mm = vfCtrl->nRadSrf + 1;
    else
      mm = n;

    for( m=m1; m<mm; m++ )   /* compute view factor: row N, columns M */
      {
      if( vfCtrl->nMaskSrf && AF[n][m] >= 0.0 ) continue;
      if( m == n ) continue;
      _col = m;
      if( vfCtrl->row > 0 && vfCtrl->col == 0)  /* progress display - single surface */
        {
#ifdef LOGGING
        fprintf( stderr, "\rSurface %d to surface %d", _row, _col );
#endif
        }
      if( _list>0 && vfCtrl->row )
#ifdef LOGGING
        fprintf( _ulog, "*ROW %d, COL %d\n", _row, _col );
#endif
      if( vfCtrl->col )
        {
#ifdef LOGGING
        DumpSrf3D( "  srf", srf+_row );
        DumpSrf3D( "  srf", srf+_col );
        fflush( _ulog );
#endif
        }

      minArea = MIN( srf[n].area, srf[m].area );
      mayView = SelfObstructionTest3D( srf+n, srf+m, &srfM );
      if( mayView )
        mayView = SelfObstructionTest3D( srf+m, srf+n, &srfN );
      if( mayView )
        {
        if( srfN.area * srfM.area == 0.0 )  /* must clip one or both surfces */
          {
          if( srfN.area + srfM.area == 0.0 )
            {
            IntersectionTest( &srfN, &srfM );  /* check invalid geometry */
            SelfObstructionClip( &srfN );
            SelfObstructionClip( &srfM );
            }
          else if( srfN.area == 0.0 )
            SelfObstructionClip( &srfN );
          else if( srfM.area == 0.0 )
            SelfObstructionClip( &srfM );
          }
        if( vfCtrl->col )
          {
#ifdef LOGGING
          DumpSrfNM( "srfN", &srfN );
          DumpSrfNM( "srfM", &srfM );
          fflush( _ulog );
#endif
          }
        VECTOR( (&srfN.ctd), (&srfM.ctd), (&vNM) );
        distNM = VLEN( (&vNM) );
        if( distNM < 1.0e-5 * (srfN.rc + srfM.rc) )
          errorf( 3, __FILE__, __LINE__, "Surfaces have same centroids", "" );

        nProb = nPossN;
        memcpy( probableObstr+1, possibleObstrN+1, nProb*sizeof(int) );

        /* special test for extreme clipping; clipped surface amost
           in the plane of the other surface.  */
        if( srfN.area < 1.0e-4*srf[n].area ||
            srfM.area < 1.0e-4*srf[m].area )
          {
          nProb = 0;
          if( vfCtrl->col )
            fprintf( _ulog, "Extreme Clipping\n" );
          }

        if( nProb )
          nProb = ConeRadiusTest( srf, &srfN, &srfM,
            vfCtrl, probableObstr, nProb, distNM );
/*DumpOS( " Rad LOS:", nProb, probableObstr ); */

        if( nProb )
          nProb = BoxTest( srf, &srfN, &srfM, vfCtrl, probableObstr, nProb );
/*DumpOS( " Box LOS:", nProb, probableObstr ); */

        if( nProb )   /* test/set obstruction orientations */
          nProb = OrientationTest( srf, &srfN, &srfM,
            vfCtrl, probableObstr, nProb );
/*DumpOS( " Orn LOS:", nProb, probableObstr ); */

        if( vfCtrl->nMaskSrf ) /* add masking surfaces */
          nProb = AddMaskSrf( srf, &srfN, &srfM, maskSrf, base,
            vfCtrl, probableObstr, nProb );
        vfCtrl->nProbObstr = nProb;

        {
        int j, k=0;
        for( j=1; j<=nProb; j++ )
          if( probableObstr[j] != _row && probableObstr[j] != _col )
            probableObstr[++k] = probableObstr[j];
        nProb = k;
        }
/*DumpOS( " Msk LOS:", nProb, probableObstr ); */

        if( vfCtrl->nProbObstr )    /*** obstructed view factors ***/
          {
          SRFDAT3X subs[5];    /* subsurfaces of surface 1  */
          int j, nSubSrf;       /* count / number of subsurfaces */
          double calcAF = 0.0;
                               /* set direction of projection */
          if( ProjectionDirection( srf, &srfN, &srfM,
              probableObstr, vfCtrl ) > 0 )
            { srf1 = &srfN; srf2 = &srfM; }
          else
            { srf1 = &srfM; srf2 = &srfN; }

#ifdef LOGGING
          if( vfCtrl->col )
            {
            fprintf( _ulog, " Project rays from srf %d to srf %d\n",
              srf1->nr, srf2->nr );
/*          DumpSrfNM( "from srf", srf1 ); */
/*          DumpSrfNM( "  to srf", srf2 ); */
            fprintf( _ulog, " %d probable obtructions:\n", vfCtrl->nProbObstr );
            for( j=1; j<=vfCtrl->nProbObstr; j++ )
              DumpSrf3D( "   surface", srf+probableObstr[j] );
            }
          else if( _list>0 && vfCtrl->row )
            fprintf( _ulog, " %d probable obtructions\n", vfCtrl->nProbObstr );
#endif

          if( vfCtrl->nProbObstr > maxSrfT ) /* expand srfOT array */
            {
            Fre_V( vfCtrl->srfOT, 0, maxSrfT, sizeof(SRFDAT3X), __FILE__, __LINE__ );
            maxSrfT = vfCtrl->nProbObstr + 4;
            vfCtrl->srfOT = Alc_V( 0, maxSrfT, sizeof(SRFDAT3X), __FILE__, __LINE__ );
            }
          CoordTrans3D( srf, srf1, srf2, probableObstr, vfCtrl );

          nSubSrf = Subsurface( &vfCtrl->srf1T, subs );
          for( vfCtrl->failRecursion=j=0; j<nSubSrf; j++ )
            {
/*          minArea = MIN( subs[j].area, vfCtrl->srf2T.area ); */
            vfCtrl->epsAF = minArea * vfCtrl->epsAdap;
            if( subs[j].nv == 3 )
              calcAF += ViewTP( subs[j].v, subs[j].area, 0, vfCtrl );
            else
              calcAF += ViewRP( subs[j].v, subs[j].area, 0, vfCtrl );
            }
          AF[n][m] = calcAF * srf2->rc * srf2->rc;   /* area scaling factor */
          if( vfCtrl->failRecursion )
            {
            fprintf( _ulog, " row %d, col %d,  recursion did not converge, AF %g\n",
              _row, _col, AF[n][m] );
            vfCtrl->failConverge = 1;
            }
          nObstr += vfCtrl->nProbObstr;
          nAFwO += 1;
          vfCtrl->method = 5;
          }

        else                      /*** unobstructed view factors ***/
          {
          SRFDATNM *srf1;  /* pointer to surface 1 (smaller surface) */
          SRFDATNM *srf2;  /* pointer to surface 2 */
          vfCtrl->method = 5;
          vfCtrl->failViewALI = 0;
          if( srfN.rc >= srfM.rc )
            { srf1 = &srfM; srf2 = &srfN; }
          else
            { srf1 = &srfN; srf2 = &srfM; }
          ViewMethod( &srfN, &srfM, distNM, vfCtrl );
/*        minArea = MIN( srfN.area, srfM.area ); */
          vfCtrl->epsAF = minArea * vfCtrl->epsAdap;
          AF[n][m] = ViewUnobstructed( vfCtrl, _row, _col, edgeData );
          if( vfCtrl->failViewALI )
            {
            fprintf( _ulog, " row %d, col %d,  line integral did not converge, AF %g\n",
              _row, _col, AF[n][m] );
            vfCtrl->failConverge = 1;
            }
          if( vfCtrl->method<5 ) /* ??? */
            bins[vfCtrl->method][vfCtrl->nEdgeDiv] += 1;   /* count edge divisions */
          nAFnO += 1;
          }
        }
      else
        {                         /* view not possible */
        AF[n][m] = 0.0;
        nAF0 += 1;
        vfCtrl->method = 6;
        }

      if( srf[n].area > srf[m].area )  /* remove very small values */
        {
        if( AF[n][m] < 1.0e-12 * srf[n].area )
          AF[n][m] = 0.0;
        }
      else
        if( AF[n][m] < 1.0e-12 * srf[m].area )
          AF[n][m] = 0.0;
#ifdef XXX
#endif

#ifdef LOGGING
      if( _list>0 && vfCtrl->row )
        {
        fprintf( _ulog, " AF(%d,%d): %.7e %.7e %.7e %s\n", _row, _col,
          AF[n][m], AF[n][m] / srf[n].area, AF[n][m] / srf[m].area,
          methods[vfCtrl->method] );
        fflush( _ulog );
        }
#endif

      }  /* end of element M of row N */

    }  /* end of row N */
#ifdef LOGGING
  fputc( '\n', stderr );
#endif

#ifdef LOGGING
  fprintf( _ulog, "\nSurface pairs where F(i,j) must be zero: %8lu\n", nAF0 );
  fprintf( _ulog, "\nSurface pairs without obstructed views:  %8lu\n", nAFnO );
#endif
  bins[4][5] = bins[0][5] + bins[1][5] + bins[2][5] + bins[3][5];
#ifdef LOGGING
  fprintf( _ulog, "   nd %7s %7s %7s %7s %7s\n",
    methods[0], methods[1], methods[2], methods[3], methods[4] );
  fprintf( _ulog, "    2 %7u %7u %7u %7u %7u direct\n",
     bins[0][2], bins[1][2], bins[2][2], bins[3][2], bins[4][2] );
  fprintf( _ulog, "    3 %7u %7u %7u %7u\n",
     bins[0][3], bins[1][3], bins[2][3], bins[3][3] );
  fprintf( _ulog, "    4 %7u %7u %7u %7u\n",
     bins[0][4], bins[1][4], bins[2][4], bins[3][4] );
  fprintf( _ulog, "  fix %7u %7u %7u %7u %7u fixes\n",
     bins[0][5], bins[1][5], bins[2][5], bins[3][5], bins[4][5] );
#endif
  ViewsInit( 4, 0, edgeData );
#ifdef LOGGING
  fprintf( _ulog, "Adaptive line integral evaluations used: %8lu\n",
    vfCtrl->usedV1LIadapt );
  fprintf( _ulog, "\nSurface pairs with obstructed views:   %10lu\n", nAFwO );
  if( nAFwO > 0 )
    {
    fprintf( _ulog, "Average number of obstructions per pair:   %6.2f\n",
      (double)nObstr / (double)nAFwO );
    fprintf( _ulog, "Adaptive viewpoint evaluations used:   %10lu\n",
      vfCtrl->usedVObs );
    fprintf( _ulog, "Adaptive viewpoint evaluations lost:   %10lu\n",
      vfCtrl->wastedVObs );
    fprintf( _ulog, "Non-zero viewpoint evaluations:        %10lu\n",
      vfCtrl->totVpt );
/***fprintf( _ulog, "Number of 1AI point-polygon evaluations: %8u\n",
      vfCtrl->totPoly );***/
    fprintf( _ulog, "Average number of polygons per viewpoint:  %6.2f\n\n",
      (double)vfCtrl->totPoly / (double)vfCtrl->totVpt );
    }
#endif

  if( vfCtrl->failConverge ) error( 1, __FILE__, __LINE__,
    "Some calculations did not converge, see VIEW3D.LOG", "" );

#if( DEBUG > 1 )
  MemRem( "After View3D() calculations" );
#endif
  if( vfCtrl->nMaskSrf )
    Fre_V( maskSrf, 1, vfCtrl->nMaskSrf, sizeof(int), __FILE__, __LINE__ );
  Fre_MC( bins, 0, 4, 1, 5, sizeof(int), __FILE__, __LINE__ );
  Fre_V( vfCtrl->srfOT, 0, maxSrfT, sizeof(SRFDAT3X), __FILE__, __LINE__ );
  Fre_V( probableObstr, 1, vfCtrl->nAllSrf, sizeof(int), __FILE__, __LINE__ );
  Fre_V( possibleObstrN, 1, vfCtrl->nAllSrf, sizeof(int), __FILE__, __LINE__ );

  }  /* end of View3D */

/***  ProjectionDirection.c  *************************************************/

/*  Set direction of projection of obstruction shadows.
 *  Direction will be from surface 1 to surface 2.
 *  Want surface 2 large and distance from surface 2
 *  to obstructions to be small.
 *  Return direction indicator: 1 = N to M, -1 = M to N.
 */

int ProjectionDirection( SRFDAT3D *srf, SRFDATNM *srfN, SRFDATNM *srfM,
  int *probableObstr, View3DControlData *vfCtrl )
/* srf  - data for all surfaces.
 * srfN - data for surface N.
 * srfM - data for surface M.
 * probableObstr  - list of probable obstructing surfaces (input and revised for output).
 * vfCtrl - computation controls.
 */
  {
  int j, k;   /* surface number */
  Vec3 v;
  double sdtoN, sdtoM; /* minimum distances from obstruction to N and M */
  int direction=0;  /* 1 = N is surface 1; -1 = M is surface 1 */

#if( DEBUG > 1 )
  fprintf( _ulog, "ProjectionDirection:\n");
#endif

#ifdef XXX
  if( fabs(srfN->rc - srfM->rc) > 0.002*(srfN->rc + srfM->rc) )
    if( srfN->rc <= srfM->rc )    old code
      direction = 1;
    else
      direction = -1;
               this is worse for OVERHANG.VS3
#endif
#ifdef XXX  /* this great idea doesn't seem to work well */
  if( srfM->rc > 10.0 * srfN->rc )  /* 2005/11/06 */
    direction = 1;
  if( srfN->rc > 10.0 * srfM->rc )
    direction = -1;
#endif

  if( direction == 0 )
    {
      /* determine distances from centroid of K to centroids of N & M */
    sdtoM = sdtoN = 1.0e9;
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      {
      double dist;
      k = probableObstr[j];
      if( srf[k].NrelS >= 0 )
        {
        VECTOR( (&srfN->ctd), (&srf[k].ctd), (&v) );
        dist = VDOT( (&v), (&v) );
        if( dist<sdtoN ) sdtoN = dist;
        }
      if( srf[k].MrelS >= 0 )
        {
        VECTOR( (&srfM->ctd), (&srf[k].ctd), (&v) );
        dist = VDOT( (&v), (&v) );
        if( dist<sdtoM ) sdtoM = dist;
        }
      }
    sdtoN = sqrt( sdtoN );
    sdtoM = sqrt( sdtoM );
#if( DEBUG > 1 )
    fprintf( _ulog, " min dist to srf %d: %e\n", srfN->nr, sdtoN );
    fprintf( _ulog, " min dist to srf %d: %e\n", srfM->nr, sdtoM );
#endif

      /* direction based on distances and areas of N & M */
    if( fabs(sdtoN - sdtoM) > 0.002*(sdtoN + sdtoM) )
      {
      if( sdtoN < sdtoM ) direction = -1;
      if( sdtoN > sdtoM ) direction = 1;
      }

#if( DEBUG > 1 )
    fprintf( _ulog, " sdtoN %e, sdtoM %e, dir %d\n",
      sdtoN, sdtoM, direction );
#endif
    }

  if( !direction )   /* direction based on number of obstructions */
    {
    int nosN=0, nosM=0;   /* number of surfaces facing N or M */
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      {
      k = probableObstr[j];
      if( srf[k].NrelS >= 0 ) nosN++;
      if( srf[k].MrelS >= 0 ) nosM++;
      }
    if( nosN > nosM )
      direction = +1;
    else
      direction = -1;
    }

#if( DEBUG > 1 )
  fprintf( _ulog, " rcN %e, rcM %e, dir %d, pdir %d\n",
    srfN->rc, srfM->rc, direction, vfCtrl->prjReverse );
#endif

  if( vfCtrl->prjReverse )       /* direction reversal parameter */
    direction *= -1;

  k = 0;         /* eliminate probableObstr surfaces facing wrong direction */
  if( direction > 0 )  /* projections from N toward M */
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      if( srf[probableObstr[j]].MrelS >= 0 )
        probableObstr[++k] = probableObstr[j];
  if( direction < 0 )  /* projections from M toward N */
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      if( srf[probableObstr[j]].NrelS >= 0 )
        probableObstr[++k] = probableObstr[j];
  vfCtrl->nProbObstr = k;

  return direction;

  }  /*  end of ProjectionDirection  */

/***  ViewMethod.c  **********************************************************/

/*  Determine method to compute unobstructed view factor.  */

void ViewMethod( SRFDATNM *srfN, SRFDATNM *srfM, double distNM, View3DControlData *vfCtrl )
  {
  SRFDATNM *srf1;  /* pointer to surface 1 (smaller surface) */
  SRFDATNM *srf2;  /* pointer to surface 2 */

  if( srfN->rc >= srfM->rc )
    { srf1 = srfM; srf2 = srfN; }
  else
    { srf1 = srfN; srf2 = srfM; }
  memcpy( &vfCtrl->srf1T, srf1, sizeof(SRFDAT3X) );
  memcpy( &vfCtrl->srf2T, srf2, sizeof(SRFDAT3X) );

  vfCtrl->rcRatio = srf2->rc / srf1->rc;
  vfCtrl->relSep = distNM / (srf1->rc + srf2->rc);

  vfCtrl->method = UNK;
  if( vfCtrl->rcRatio > 4 )
    {
    if( srf1->shape )
      {
      double relDot = VDOTW( (&srf1->ctd), (&srf2->dc) );
      if( vfCtrl->rcRatio > 10.0f &&
        (vfCtrl->relSep > _sai10 || relDot > 2.0*srf1->rc ) )
        vfCtrl->method = SAI;
      else if( vfCtrl->relSep > _sai4 || relDot > 2.0*srf1->rc )
        vfCtrl->method = SAI;
      }
    if( vfCtrl->method==UNK && vfCtrl->relSep > _sli4 )
      vfCtrl->method = SLI;
    }
  if( vfCtrl->method==UNK && vfCtrl->relSep > _dai1
      && srf1->shape && srf2->shape )
    vfCtrl->method = DAI;
  if( vfCtrl->method==UNK && vfCtrl->relSep > _sli1 )
    vfCtrl->method = SLI;
  if( vfCtrl->method==SLI && vfCtrl->epsAdap < 0.5e-6 )
    vfCtrl->method = ALI;
  if( vfCtrl->method==UNK )
    vfCtrl->method = ALI;

  if( vfCtrl->col )
    fprintf( _ulog, "ViewMethod %s; R-ratio %.3f, A-ratio %.3f, relSep %.4f, vertices %d %d\n",
      methods[vfCtrl->method], vfCtrl->rcRatio, srf2->area / srf1->area,
      vfCtrl->relSep, srf1->nv, srf2->nv );

  }  /* end ViewMethod */

/***  InitViewMethod.c  ******************************************************/

/*  Initialize ViewMethod() coefficients.  */

void InitViewMethod( View3DControlData *vfCtrl )
  {
  if( vfCtrl->epsAdap < 0.99e-7 )
    {
    _sli4 = 0.7f;
    _sai4 = 1.5f;
    _sai10 = 1.8f;
    _dai1 = 3.0f;
    _sli1 = 3.0f;
    }
  else if( vfCtrl->epsAdap < 0.99e-6 )
    {
    _sli4 = 0.5f;
    _sai4 = 1.2f;
    _sai10 = 1.2f;
    _dai1 = 2.3f;
    _sli1 = 2.2f;
    }
  else if( vfCtrl->epsAdap < 0.99e-5 )
    {
    _sli4 = 0.45f;
    _sai4 = 1.1f;
    _sai10 = 1.0f;
    _dai1 = 1.7f;
    _sli1 = 1.5f;
    }
  else if( vfCtrl->epsAdap < 0.99e-4 )
    {
    _sli4 = 0.4f;
    _sai4 = 1.0f;
    _sai10 = 0.8f;
    _dai1 = 1.3f;
    _sli1 = 0.9f;
    }
  else
    {
    _sli4 = 0.3f;
    _sai4 = 0.9f;
    _sai10 = 0.6f;
    _dai1 = 1.0f;
    _sli1 = 0.6f;
    }

  }  /* end InitViewMethod */

/***  errorf.c  **************************************************************/

/*  error messages for view factor calculations  */

int errorf( int severity, char *file, int line, ... ){
/* severity;  values 0 - 3 defined below
 * file;      file name: __FILE__
 * line;      line number: __LINE__
 * ...;       string variables (up to 80 char total) */

  va_list argp;     /* variable argument list */
  char start[]=" ";
  char *msg, *s;
  static char *head[4] = { "  *** note *** ",
                         "*** warning ***",
                         " *** error *** ",
                         " *** fatal *** " };
  char string[LINELEN + 1]; /* buffer for a character string */
/*  char name[16]; */
  int n=1;

  if( severity >= 0 )
    {
    if( severity>3 ) severity = 3;
/*    PathSplit( file, string, string, name, string ); */
    sprintf( string, "%s  (file %s, line %d)\n", head[severity], sfname( file ), line );
    fputs( "\n", stderr );
    fputs( string, stderr );
    if( _ulog != NULL && _ulog != stderr )
      {
      fputs( string, _ulog );
      fflush( _ulog );
      }

    msg = start;   /* merge message strings */
    sprintf( string, "row %d, col %d; ", _row, _col );
    s = string;
    while( *s )
      s++;
    va_start( argp, line );
    while( *msg && n<80 )
      {
      while( *msg && n++<80 )
        *s++ = *msg++;
      msg = (char *)va_arg( argp, char * );
      }
    *s++ = '\n';
    *s = '\0';
    va_end( argp );

    fputs( string, stderr );
    if( _ulog != NULL && _ulog != stderr )
      fputs( string, _ulog );
    }

  if( severity>2 ) exit( 1 );

  return 0;

  }  /*  end of errorf  */
