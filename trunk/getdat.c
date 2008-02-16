/*subfile:  Getdat.c  ********************************************************/

#ifdef _DEBUG
# define DEBUG 1
#else
# define DEBUG 1
#endif

#define V3D_BUILD
#include "getdat.h"

#include <stdio.h>
#include <string.h> /* prototype: memcpy, strcpy, strncpy */
#include <stdlib.h> /* prototype: atoi, atof */
#include <math.h>   /* prototype: sqrt */
#include <ctype.h>  /* prototype: toupper */
#include "types.h"
#include "misc.h"
#include "test3d.h"
#include "polygn.h"
#include "viewobs.h"
#include "ctrans.h"
#include "heap.h"
#include "test2d.h"

#define PI 3.141592653589793238
#define deg2rad(x)  ((x)*PI/180.) /* angle: degrees -> radians */
#define rad2deg(x)  ((x)*180./PI)  /* angle: radians -> degrees */

/* forward decls */

double VolPrism( Vec3 *a, Vec3 *b, Vec3 *c );
void SetPlane( SRFDAT3D *srf );

static void TestSubSrf( SRFDAT3D *srf, const int *baseSrf, View3DControlData *vfCtrl );

/***  GetCtrl.c  *************************************************************/

/*  Process simulation control values.  */

void GetCtrl( char *str, View3DControlData *vfCtrl )
  {
  char *p;
  int i;
  float r;

  p = strtok( str, "= ," );
  while( p )
    {
    if( streqli( p, "eps" ) )
      {
      p = strtok( NULL, "= ," );
      if( FltCon( p, &r ) )
        error( 2, __FILE__, __LINE__, "Bad float value: ", p, "" );
      else
        {
        vfCtrl->epsAdap = r;
        if( r < 0.99e-6 )
          error( 1, __FILE__, __LINE__, "Convergence limit < 1.0e-6", "" );
        if( r > 1.0e-2 )
          error( 1, __FILE__, __LINE__, "Convergence limit > 1.0e-2", "" );
        }
      }
    else if( streqli( p, "list" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        _list = i;
      }
    else if( streqli( p, "out" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        if( i < 0 || i > 2 )
          error( 2, __FILE__, __LINE__, "Invalid output file format", "" );
        else
          vfCtrl->outFormat = i;
      }
    else if( streqli( p, "encl" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        if( i ) vfCtrl->enclosure = 1;
      }
    else if( streqli( p, "emit" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        if( i ) vfCtrl->emittances = 1;
      }
    else if( streqli( p, "maxU" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        {
        if( i < 4 )
          {
          error( 1, __FILE__, __LINE__,
            "Maximum unobstructed recursions reset to 4", "" );
          i = 4;
          }
        if( i > 12 )
          error( 1, __FILE__, __LINE__,
            "Maximum unobstructed recursions may be too large", "" );
        vfCtrl->maxRecursALI = i;
        }
      }
    else if( streqli( p, "maxO" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        {
        if( i < 4 )
          {
          error( 1, __FILE__, __LINE__,
            "Maximum obstructed recursions reset to 4", "" );
          i = 4;
          }
        if( i > 12 )
          error( 1, __FILE__, __LINE__,
            "Maximum obstructed recursions may be too large", "" );
        vfCtrl->maxRecursion = i;
        }
      }
    else if( streqli( p, "minO" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        {
        if( i < 0 )
         i = 0;
        if( i > 2 )
          error( 1, __FILE__, __LINE__,
            "Minimum obstructed recursions may be too large", "" );
        vfCtrl->minRecursion = i;
        }
      }
    else if( streqli( p, "row" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        {
        if( i < 0 )
          i = 0;
        vfCtrl->row = i;
        }
      }
    else if( streqli( p, "col" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        {
        if( i < 0 )
          i = 0;
        vfCtrl->col = i;
        }
      }
    else if( streqli( p, "prjD" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        if( i ) vfCtrl->prjReverse = 1;
      }
    else if( streqli( p, "maxV" ) )
      {
      p = strtok( NULL, "= ," );
      if( IntCon( p, &i ) )
        error( 2, __FILE__, __LINE__, "Bad integer value: ", p, "" );
      else
        _maxNVT = i;
      }

    else
      {
      error( 1, __FILE__, __LINE__, "Invalid control word: ", p, "" );
      p = strtok( NULL, "= ," );
      }

    p = strtok( NULL, "= ," );    // get next word
    }

  if( vfCtrl->col && !vfCtrl->row )
    error( 2, __FILE__, __LINE__, "Must set row before setting column", "" );

  }  /* end GetCtrl */

/***  CountVS3D.c  ***********************************************************/

/*  Determine number of vertices, number of surfaces,
 *  control parameters, and format of the input file.  */

void CountVS3D( char *title, View3DControlData *vfCtrl )
  {
  int c;     /* first character in line */
  int flag=0;  /* NxtWord flag: 0 for first word of first line */

  error( -2, __FILE__, __LINE__, "" );  /* clear error count */
  vfCtrl->nRadSrf = vfCtrl->nObstrSrf = 0;

  while( NxtWord( _string, flag, sizeof(_string) ) != NULL )
    {
    c = toupper( _string[0] );
    switch( c )
      {
      case 'S':               /* surface */
        vfCtrl->nRadSrf += 1;
        break;
      case 'V':               /* vertex */
        vfCtrl->nVertices += 1;
        break;
      case 'M':               /* masking surface */
      case 'N':               /* "null" surface */
        vfCtrl->nMaskSrf += 1;
        vfCtrl->nRadSrf += 1;
        break;
      case 'O':               /* obstruction */
        vfCtrl->nObstrSrf += 1;
        break;
      case 'T':               /* title */
        NxtWord( title, 2, sizeof(_string) );
        break;
      case 'F':               /* input file format: geometry */
        NxtWord( _string, 0, sizeof(_string) );
        vfCtrl->format = 0;
        if( streql( _string, "3"  ) ) vfCtrl->format = 3;
        if( streqli( _string, "3a" ) ) vfCtrl->format = 4;
        if( vfCtrl->format < 3 ) error( 2, __FILE__, __LINE__,
          "Invalid input geometry: ", _string, "" );
        break;
      case 'C':               /* C run control data */
        NxtWord( _string, 2, sizeof(_string) );
        GetCtrl( _string, vfCtrl );
        break;
      case 'E':
        goto finish;
      default:
        break;
      }
    flag = 1;
    }
finish:
  vfCtrl->nAllSrf = vfCtrl->nRadSrf + vfCtrl->nObstrSrf;
  if( vfCtrl->row > vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
     "\"row\" value too large", "" );
  if( vfCtrl->col > vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
     "\"row\" value too large", "" );

  if( error( -1, __FILE__, __LINE__, "" )>0 )
    error( 3, __FILE__, __LINE__, "Fix errors in input data", "" );

  }  /*  end of CountVS3D  */

/***  GetSrfD.c  *************************************************************/

/*  Function to read common surface data */

void GetSrfD( char **name, float *emit, int *base, int *cmbn,
  SRFDAT3D *srf, View3DControlData *vfCtrl, int ns )
  {
  int n;

  n = ReadIX( 0 );              /* base surface number */
  base[ns] = n;
  if( n<0 || n>vfCtrl->nAllSrf ) error( 2, __FILE__, __LINE__,
    "Improper base surface number:", IntStr(n), "" );

  if( n>0 )
    {
    if( srf[ns].type == OBSO )
      error( 2, __FILE__, __LINE__,
        "Base surface not permitted for surface ", IntStr(ns), "" );
    if( srf[ns].type == MASK || srf[ns].type == NULS )
      if( n<=0 || n>=ns ) error( 2, __FILE__, __LINE__,
        "A valid base surface number is required for surface ", IntStr(ns), "" );
    if( srf[ns].type == RSRF && n<=vfCtrl->nRadSrf )
      if( n>=ns ) error( 2, __FILE__, __LINE__, "Subsurface ", IntStr(ns),
        "Must be after base surface ", IntStr(n), "" );
    if( srf[ns].type != MASK && srf[ns].type != NULS )
      srf[ns].type = SUBS;
    }

  n = ReadIX( 0 );              /* combine surface number */
  cmbn[ns] = n;
  if( n<0 || n>vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
     "Improper combine surface number:", IntStr(n), "" );
  else if( n > 0 )
    {
    if( srf[ns].type == MASK || srf[ns].type == NULS || srf[ns].type == OBSO )
      error( 2, __FILE__, __LINE__,
        "Combination not permitted for surface ", IntStr(ns), "" );
    if( n>=ns ) error( 2, __FILE__, __LINE__,
       "Must combine surface with previous surface:", IntStr(n), "" );
    if( cmbn[ns] )
      if( cmbn[n] ) error( 2, __FILE__, __LINE__,
       "May not chain combined surfaces:", IntStr(n), "" );
    }

  emit[ns] = ReadR4( 0 );       /* surface emittance */
  if( emit[ns] > 0.99901 )
    {
    error( 1, __FILE__, __LINE__,  "Replacing surface ", IntStr(ns),
       " emittance (", _string, ") with 0.999", "" );
    emit[ns] = 0.999f;
    }
  if( emit[ns] < 0.00099f )
    {
    error( 1, __FILE__, __LINE__,  "Replacing surface ", IntStr(ns),
       " emittance (", _string, ") with 0.001", "" );
    emit[ns] = 0.001f;
    }

  NxtWord( _string, 0, sizeof(_string) );  /* surface name */
  _string[NAMELEN-1] = '\0';    /* guarantee termination */
  strncpy( name[ns], _string, NAMELEN );

  }  /* end GetSrfD */

/***  GetVS3D.c  *************************************************************/

/*  Function to read the 3-D input file:  Vertex & Surface format */

void GetVS3D( char **name, float *emit, int *base, int *cmbn,
  SRFDAT3D *srf, Vec3 *xyz, View3DControlData *vfCtrl )
  {
  int c;       /* first character in line */
  int nv=0;    /* number of vertices */
  int ns=0;    /* number of surfaces */
  int n;
  int flag=0;  /* NxtWord flag: 0 for first word of first line */

  error( -2, __FILE__, __LINE__, "" );  /* clear error count */
  rewind( _unxt );

  while( NxtWord( _string, flag, sizeof(_string) ) != NULL )
    {
    c = toupper( _string[0] );
    switch( c )
      {
      case 'V':
        n = ReadIX( 0 );
        nv += 1;
        if( n!= nv ) error( 2, __FILE__, __LINE__,
          "Vertex: ", IntStr(n), " out of sequence", "" );
        xyz[nv].x = ReadR8( 0 );
        xyz[nv].y = ReadR8( 0 );
        xyz[nv].z = ReadR8( 0 );
        break;
      case 'M':               /* masking surface */
      case 'N':               /* "null" surface */
      case 'S':
      case 'O':
        n = ReadIX( 0 );
        ns += 1;
        if( n!= ns ) error( 2, __FILE__, __LINE__,
          "Surface: ", IntStr(n), " out of sequence", "" );
        if( c=='O' && n<=vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Obstruction surface: ", IntStr(n), " out of sequence", "" );
        if( c=='S' && n>vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Radiating surface: ", IntStr(n), " out of sequence", "" );
        srf[ns].nr = ns;
        if( c == 'S' )
          srf[ns].type = RSRF;
        else if( c == 'O' )
          srf[ns].type = OBSO;
        else if( c == 'N' )
          srf[ns].type = NULS;
        else if( c == 'M' )
          srf[ns].type = MASK;

        n = ReadIX( 0 );
        if( n<=0 || n>vfCtrl->nVertices ) error( 2, __FILE__, __LINE__,
          "Surface ", IntStr(ns), "- improper first vertex:", IntStr(n), "" );
        else
          srf[ns].v[0] = xyz + n;

        n = ReadIX( 0 );
        if( n<=0 || n>vfCtrl->nVertices ) error( 2, __FILE__, __LINE__,
          "Surface ", IntStr(ns), "- improper second vertex:", IntStr(n), "" );
        else
          srf[ns].v[1] = xyz + n;

        n = ReadIX( 0 );
        if( n<=0 || n>vfCtrl->nVertices ) error( 2, __FILE__, __LINE__,
          "Surface ", IntStr(ns), "- improper third vertex:", IntStr(n), "" );
        else
          srf[ns].v[2] = xyz + n;

        n = ReadIX( 0 );
        if( n<0 || n>vfCtrl->nVertices ) error( 2, __FILE__, __LINE__,
          "Surface ", IntStr(ns), "- improper fourth vertex:", IntStr(n), "" );
        if( n == 0 )
          srf[ns].nv = 3;
        else
          {
          srf[ns].nv = 4;
          srf[ns].v[3] = xyz + n;
          }

        SetPlane( srf+ns );          /* compute plane polygon values */

        if( c!='O' )
          GetSrfD( name, emit, base, cmbn, srf, vfCtrl, ns );

        break;

      case '/':               /* comment */
      case '!':
      case 'T':               /* title */
      case 'F':               /* geometry */
      case 'C':               /* control */
      case 'X':               /* coordinate transformation */
        break;

      case 'E':               /* end of data */
      case '*':
        goto finish;

      default:
        error( 1, __FILE__, __LINE__,
          "Undefined input identifier: ", _string, "" );
        break;
      }
    flag = 1;
    }

finish:
  TestSubSrf( srf, base, vfCtrl );

  if( error( -1, __FILE__, __LINE__, "" )>0 )
    error( 3, __FILE__, __LINE__, "Fix errors in input data", "" );

  }  /*  end of GetVS3D  */

/***  GetVS3Da.c  ************************************************************/

/*  Function to read the 3-D input file:  BLAST Surface format */

void GetVS3Da( char **name, float *emit, int *base, int *cmbn,
  SRFDAT3D *srf, Vec3 *xyz, View3DControlData *vfCtrl )
/*
 * stype;  surface type data:  1 = obstruction only surface,
 *         0 = normal surface, -1 = included surface,
 *        -2 = part of an obstruction surface
 */
  {
  Vec3 xyzLLC;  /* coordinates of lower left corner of surface plane */
  double azm, tilt;     /* azimuth and tilt angles of surface plane */
  double sa, ca, st, ct;
  Vec3 v[4];
  int s;       /* shape indicator: T=triangle, R=rectangle, Q=quadrilateral */
  int c;       /* first character in line */
  int nv=0;    /* number of vertices */
  int ns=0;    /* number of surfaces */
  int j, n;

  error( -2, __FILE__, __LINE__, "" );  /* clear error count */
  rewind( _unxt );
  NxtWord( _string, -1, sizeof(_string) );

  while( NxtWord( _string, 1, sizeof(_string) ) != NULL )
    {
    c = toupper( _string[0] );
    switch( c )
      {
      case 'S':
      case 'O':
      case 'M':
      case 'N':
        n = ReadIX( 0 );
        ns += 1;
        if( n!= ns ) error( 2, __FILE__, __LINE__,
           "Surface out of sequence:", IntStr(n), "" );
        if( c=='O' && n<=vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Obstruction surface out of sequence:", IntStr(n), "" );
        if( c=='S' && n>vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Radiating surface out of sequence:", IntStr(n), "" );
        srf[ns].nr = ns;
        if( c == 'S' )
          srf[ns].type = RSRF;
        else if( c == 'O' )
          srf[ns].type = OBSO;
        else if( c == 'N' )
          srf[ns].type = NULS;
        else if( c == 'M' )
          srf[ns].type = MASK;

        NxtWord( _string, 0, sizeof(_string) );
        s = toupper( _string[0] );

        if( c!='O' )
          GetSrfD( name, emit, base, cmbn, srf, vfCtrl, ns );

        xyzLLC.x = ReadR8( 1 );      /* surface geometry data */
        xyzLLC.y = ReadR8( 0 );
        xyzLLC.z = ReadR8( 0 );
        azm = ReadR8( 0 );
        tilt = ReadR8( 0 );
        ca = -cos( deg2rad(azm) );
        sa = sin( deg2rad(azm) );
        ct = cos( deg2rad(tilt) );
        st = sin( deg2rad(tilt) );

        if( s == 'R' )               /* rectangle */
          {
          srf[ns].nv = 4;
          v[0].x = v[1].x = 0.0;
          v[2].x = v[3].x = ReadR8( 0 );   /* width */
          v[0].y = v[3].y = ReadR8( 0 );   /* height */
          v[1].y = v[2].y = 0.0;
          }
        else if( s == 'T' )
          {
          srf[ns].nv = 3;
          for( j=0; j<3; j++ )
            {
            v[j].x = ReadR8( 0 );
            v[j].y = ReadR8( 0 );
            }
          }
        else if( s == 'Q' )
          {
          srf[ns].nv = 4;
          for( j=0; j<4; j++ )
            {
            v[j].x = ReadR8( 0 );
            v[j].y = ReadR8( 0 );
            }
          }
        else
          error( 2, __FILE__, __LINE__,
            "Invalid shape identifier: surface ", IntStr(ns), "" );

        v[0].z = v[1].z = v[2].z = v[3].z = 0.0;
        for( j=0; j<4; j++ )
          {
          nv++;
          srf[ns].v[j] = xyz+nv;
          srf[ns].v[j]->x = (xyzLLC.x + v[j].x * ca 
                          - (v[j].y * ct - v[j].z * st ) * sa);
          srf[ns].v[j]->y = (xyzLLC.y + v[j].x * sa 
                          + (v[j].y * ct - v[j].z * st ) * ca);
          srf[ns].v[j]->z = (xyzLLC.z + v[j].y * st + v[j].z * ct);
          }

        SetPlane( srf+ns );          /* compute plane polygon values */
        break;


      case '/':               /* comment */
      case '!':
      case 'T':               /* title */
      case 'F':               /* geometry */
      case 'C':               /* control */
      case 'X':               /* coordinate transformation */
        break;

      case 'E':               /* end of data */
      case '*':
        goto finish;

      default:
        error( 1, __FILE__, __LINE__,
          "Undefined input identifier: ", _string, "" );
        break;
      }
    }

finish:
  TestSubSrf( srf, base, vfCtrl );

  if( error( -1, __FILE__, __LINE__, "" )>0 )
    error( 3, __FILE__, __LINE__, "Fix errors in input data", "" );

  }  /*  end of GetVS3Da  */

/***  SetPlane.c  ************************************************************/

/*  Set values for a plane convex polygon, up to 4 vertices;
 *  Return 1 if error encountered.  */

void SetPlane( SRFDAT3D *srf )
  {
  Vec3 v={0.0, 0.0, 0.0};
  double r2, rr=0.0;
  int j;

  for( j=0; j<srf->nv; j++ )     /* approximate centroid */
    {
    if( fabs(srf->v[j]->x) < 1.0e-14 ) srf->v[j]->x = 0.0;
    if( fabs(srf->v[j]->y) < 1.0e-14 ) srf->v[j]->y = 0.0;
    if( fabs(srf->v[j]->z) < 1.0e-14 ) srf->v[j]->z = 0.0;
    v.x += srf->v[j]->x;
    v.y += srf->v[j]->y;
    v.z += srf->v[j]->z;
    }
  rr = 1.0 / srf->nv;
  srf->ctd.x = rr * v.x;
  srf->ctd.y = rr * v.y;
  srf->ctd.z = rr * v.z;

  VECTOR( (&srf->ctd), srf->v[0], (&v) );
  rr = VDOT( (&v), (&v) );
  for( j=1; j<srf->nv; j++ )     /* maximum radius */
    {
    VECTOR( (&srf->ctd), srf->v[j], (&v) );
    r2 = VDOT( (&v), (&v) );
    if( r2 > rr ) rr = r2;
    }
  srf->rc = sqrt( rr );

  srf->shape = 0;
  if( srf->nv==4 )               /* quadrilateral */
    {
    int err=0;
    Vec3 p[4]; /* coordinates of polygoc vertices */
    DirCos dct[4]; /* temporary direction cosines */
    double a[5],z[4];  /* temporary values: areas, elevations */
    for( j=0; j<4; j++ )
      VCOPY( (srf->v[j]), (p+j) );
    a[0] = Triangle( p+0, p+1, p+2, dct+0, srf->nr );
    a[1] = Triangle( p+1, p+2, p+3, dct+1, srf->nr );
    a[2] = Triangle( p+2, p+3, p+0, dct+2, srf->nr );
    a[3] = Triangle( p+3, p+0, p+1, dct+3, srf->nr );
    srf->area = 0.5f * (a[0] + a[1] + a[2] + a[3]);
    srf->dc.x = 0.25f * ( dct[0].x + dct[1].x + dct[2].x + dct[3].x );
    srf->dc.y = 0.25f * ( dct[0].y + dct[1].y + dct[2].y + dct[3].y );
    srf->dc.z = 0.25f * ( dct[0].z + dct[1].z + dct[2].z + dct[3].z );
    srf->dc.w = -VDOT( (&srf->dc), (&srf->ctd) );
    srf->shape = SetShape( 4, p, a+4 );
    if( fabs(a[4] - srf->area) > 1.0e-6 * (a[4] + srf->area) )
      error( 2, __FILE__, __LINE__, "Bad area calculation", "" );
    for( j=0; j<4; j++ )    /* flatness test */
      z[j] = VDOTW( (srf->v[j]), (&srf->dc) ) / srf->rc;
    for( j=0; j<4; j++ )
      if( fabs( z[j] ) > 3.0e-6 )
        err = 1;
    for( j=0; j<4; j++ )
      if( fabs( z[j] ) > 3.0e-5 )
        err = 2;
    if( err )
      {
      error( err, __FILE__,  __LINE__,
        "Vertices not in common plane: surface ", IntStr(srf->nr), "" );
      for( j=0; j<4; j++ )
        if( fabs( a[j] ) > 3.0e-6 )
          fprintf( _ulog, "  vertex %d: relative error %.2e\n", j, a[j] );
      }
    else
      {
      for( j=0; j<4; j++ )       /* test for convex polygon */
        if( VDOT( (&srf->dc), (dct+j) ) < 0.999 ) break;
      if( j<4 ) error( 2, __FILE__,  __LINE__,
        "Non-convex polygon: surface ", IntStr(srf->nr), "" );
      }
    }
  else if( srf->nv==3 )          /* is a triangle */
    {
    srf->area = Triangle( srf->v[0], srf->v[1], srf->v[2], &srf->dc, srf->nr );
    srf->dc.w = -VDOT( (&srf->dc), (&srf->ctd) );
    srf->shape = 3;
    }
  else
    error( 2, __FILE__,  __LINE__,  " Incorrect number of vertices", "" );

  }  /* end of SetPlane */

/***  TestSubSrf.c  **********************************************************/

/*  Test that subsurfaces lie in plane of their base surfaces.  */

void TestSubSrf( SRFDAT3D *srf, const int *baseSrf, View3DControlData *vfCtrl )
  {
  int size=(3*sizeof(int)+2*sizeof(double)+sizeof(DirCos)+sizeof(Vec3));
  int n;        /* surface number */
  int m;        /* base surface number */
  int j;        /* vertex number */
  int infront;  /* true if a vertex is in front of surface n */
  int behind;   /* true if a vertex is behind surface n */
  double dot, eps; /* dot product and test value */
  SRFDATNM srfN, srfM;
  SRFDAT3X *srfT;  /* pointer to surface */
  Vec2 vN[MAXNV1], vM[MAXNV1]; /* 2D vertices */
  Polygon *base;  /* pointer to base polygon */
  Polygon *subs;  /* pointer to subsurface polygon */

  for( n=1; n<=vfCtrl->nRadSrf; n++ )
    {
    if( !baseSrf[n] ) continue;
    m = baseSrf[n];
                                /* test for co-planar surfaces */
    eps = 1.0e-5f * srf[n].rc;
    infront = behind = 0;
    for( j=0; j<srf[n].nv; j++ )
      {
      dot = VDOTW( (srf[n].v[j]), (&srf[m].dc) );
      if( dot > eps ) infront = 1;
      if( dot < -eps ) behind = 1;
      }
    if( infront || behind )
      {
      error( 2, __FILE__, __LINE__, " Subsurface ",
        IntStr(n), " not in plane of base ", IntStr(m), "" );
      continue;
      }
                                /* test surface orintations */
    dot = VDOT( (&srf[n].dc), (&srf[m].dc) );
    if( srf[n].type == SUBS && dot < 0.999 )
      {
      error( 2, __FILE__, __LINE__, " Subsurface ",
        IntStr(n), " should face same direction as base", "" );
      continue;
      }
    if( (srf[n].type == MASK || srf[n].type == NULS ) && dot > -0.999 )
      {
      error( 2, __FILE__, __LINE__, "Mask/null surfacee ",
        IntStr(n), " should face opposite direction as base", "" );
      continue;
      }
                                /* test surface enclosure */
    vfCtrl->nProbObstr = 0;
    memcpy( &srfN, &srf[n], size );
    for( j=0; j<srf[n].nv; j++ )
        memcpy( srfN.v+j, srf[n].v[j], sizeof(Vec3) );
    memcpy( &srfM, &srf[m], size );
    for( j=0; j<srf[m].nv; j++ )
        memcpy( srfM.v+j, srf[m].v[j], sizeof(Vec3) );
    CoordTrans3D( srf, &srfN, &srfM, &j, vfCtrl );
    srfT = &vfCtrl->srf1T;
    if( dot < 0.0 )
      for( j=0; j<srfT->nv; j++ )      /* polygon N vertices are clockwise */
        {
        vN[j].x = srfT->v[j].x;
        vN[j].y = srfT->v[j].y;
        }
    else
      for( j=0; j<srfT->nv; j++ )      /* reverse polygon N to clockwise */
        {
        vN[j].x = srfT->v[srfT->nv-1-j].x;
        vN[j].y = srfT->v[srfT->nv-1-j].y;
        }
    eps = 1.0e-6 * sqrt(srfT->area);
    srfT = &vfCtrl->srf2T;
    for( j=0; j<srfT->nv; j++ )        /* reverse polygon M to clockwise */
      {
      vM[j].x = srfT->v[srfT->nv-1-j].x;
      vM[j].y = srfT->v[srfT->nv-1-j].y;
      }
                                /* begin with cleared small structures area */
    InitPolygonMem( eps, eps );
    base = SetPolygonHC( srfM.nv, vM, 1.0 );  /* convert to HC */
    subs = SetPolygonHC( srfN.nv, vN, 1.0 );
    if( subs && base )
      {
      if( PolygonOverlap( base, subs, 3, 0 ) != 1 )  /* 1 = enclosed */
        error( 2, __FILE__, __LINE__, " Subsurface ",
          IntStr(n), " is not (entirely?) within its base", "" );
      }
    else
      {
      fprintf( _ulog, "Enclosure test: base = %d, subs = %d, dot = %g\n", n, m, dot );
      error( 3, __FILE__, __LINE__, " Enclosure test failure", "" );
      }
    }  /* end surface loop */

  }  /* end of TestSubSrf */

/***  CountVS2D.c  ***********************************************************/

/*  Determine number of vertices, number of surfaces,
 *  control parameters, and format of the input file.  */

void CountVS2D( char *title, View2DControlData *vfCtrl )
  {
  int c;     /* first character in line */
  int ctrl=0;
  int flag=0;  /* NxtWord flag: 0 for first word of first line */

  vfCtrl->nRadSrf = vfCtrl->nObstrSrf = 0;

  while( NxtWord( _string, flag, sizeof(_string) ) != NULL )
    {
    c = toupper( _string[0] );
    switch( c )
      {
      case 'S':               /* surface */
        vfCtrl->nRadSrf += 1;
        break;
      case 'O':               /* obstruction */
        vfCtrl->nObstrSrf += 1;
        break;
      case 'V':               /* vertex */
        vfCtrl->nVertices += 1;
        break;
      case 'T':               /* title */
        NxtWord( title, 2, sizeof(_string) );
        break;
      case 'G':               /* geometry */
        c = ReadIX( 0 );
        if( c != 2 )
          error( 2, __FILE__, __LINE__, "Invalid geometry value", "" );
        break;
      case 'C':               /* C run control data */
        ctrl = 1;
        vfCtrl->enclosure = ReadIX( 0 );
          if( vfCtrl->enclosure ) vfCtrl->enclosure = 1;

        _list = ReadIX( 0 );
          if( _list<0 ) _list = 0;

        vfCtrl->epsObstr = ReadR4( 0 );
        if( vfCtrl->epsObstr < 1.0e-7 )
          {
          error( 1, __FILE__, __LINE__,
            " obstructed convergence limited to 1e-07", "" );
          vfCtrl->epsObstr = 1.0e-7f;
          }

        vfCtrl->maxRecursion = ReadIX( 0 );
        if( vfCtrl->maxRecursion < 4 )
          {
          error( 1, __FILE__, __LINE__,
            " maximum recursions reset to 4", "" );
          vfCtrl->maxRecursion = 4;
          }
        if( vfCtrl->maxRecursion > 12 )
          error( 1, __FILE__, __LINE__,
            " maximum recursions may be too large", "" );

        vfCtrl->minRecursion = ReadIX( 0 );
        if( vfCtrl->minRecursion < 0 )
         vfCtrl->minRecursion = 0;
        if( vfCtrl->minRecursion > 2 )
          error( 1, __FILE__, __LINE__,
            " minimum recursions may be too large", "" );

        vfCtrl->emittances = ReadIX( 0 );
          if( vfCtrl->emittances ) vfCtrl->emittances = 1;
        break;
      case 'E':               /* end of data */
        goto finish;
      default:
        break;
      }
    flag = 1;
    }
finish:
  vfCtrl->nAllSrf = vfCtrl->nRadSrf + vfCtrl->nObstrSrf;
  if( !ctrl ) error( 3, __FILE__, __LINE__,
     "No control values in data file", "" );

  }  /*  end of CountVS2D  */

/***  GetVS2D.c  *************************************************************/

/*  Read the 2-D surface data file */

void GetVS2D( char **name, float *emit, int *base, int *cmbn,
  SRFDAT2D *srf, View2DControlData *vfCtrl )
  {
  char c;       /* first character in line */
  int nv=0;    /* number of vertices */
  int ns=0;    /* number of surfaces */
  int flag=0;  /* NxtWord flag: 0 for first word of first line */
  Vec2 *xy;  /* vector of vertces [1:nVertices] */
  int n;

  error( -2, __FILE__, __LINE__, "" );  /* clear error count */
  rewind( _unxt );
  xy = Alc_V( 1, vfCtrl->nVertices, sizeof(Vec2), __FILE__, __LINE__ );

  while( NxtWord( _string, flag, sizeof(_string) ) != NULL )
    {
    c = toupper( _string[0] );
    switch( c )
      {
      case 'V':
        n = ReadIX( 0 );
        nv += 1;
        if( n!= nv ) error( 2, __FILE__, __LINE__,
           "Vertex out of sequence:", IntStr(n), "" );
        xy[nv].x = ReadR4( 0 );
        xy[nv].y = ReadR4( 0 );
        break;
      case 'S':
      case 'O':
        n = ReadIX( 0 );
        ns += 1;
        if( n!= ns ) error( 2, __FILE__, __LINE__,
           "Surface out of sequence:", IntStr(n), "" );
        if( c=='O' && n<=vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Obstruction surface out of sequence:", IntStr(n), "" );
        if( c=='S' && n>vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Radiating surface out of sequence:", IntStr(n), "" );
        srf[ns].nr = ns;
        srf[ns].type = 1;

        n = ReadIX( 0 );
        if( n<=0 || n>vfCtrl->nVertices ) error( 2, __FILE__, __LINE__,
           "Improper first vertex; surface ", IntStr(ns), "" );
        else
          {
          srf[ns].v1.x = xy[n].x;
          srf[ns].v1.y = xy[n].y;
          }

        n = ReadIX( 0 );
        if( n<=0 || n>vfCtrl->nVertices ) error( 2, __FILE__, __LINE__,
           "Improper second vertex; surface ", IntStr(ns), "" );
        else
          {
          srf[ns].v2.x = xy[n].x;
          srf[ns].v2.y = xy[n].y;
          }

        SetSrf2D( &srf[ns] );          /* compute area and direction cosines */

        if( c=='O' ) break;

        srf[ns].type = 0;            /* non-obstruction surface data */
        n = ReadIX( 0 );              /* obstruction surface number */
        if( n>0 )
          srf[ns].type = -2;
        if( n<0 || (n<=vfCtrl->nRadSrf && n>0) || n>vfCtrl->nAllSrf ) 
          error( 2, __FILE__, __LINE__,
           "Improper obstruction surface number:", IntStr(n), "" );

        n = ReadIX( 0 );              /* base surface number */
        base[ns] = n;
        if( n<0 || n>vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Improper base surface number:", IntStr(n), "" );
        else if( n>0 )
          {
          srf[ns].type = -1;
          if( n>=ns ) error( 2, __FILE__, __LINE__, "Subsurface ", IntStr(ns),
             " must be after base surface ", IntStr(n), "" );
          else
            {
            int i;
            for( i=ns-1; i>n; i-- )
              if( base[i] == 0 ) break;
            if( i != n ) error( 2, __FILE__, __LINE__, "Subsurface ", IntStr(ns),
               " must be immediately after base surface ", IntStr(n), "" );
            }
          }

        n = ReadIX( 0 );              /* combine surface number */
        cmbn[ns] = n;
        if( n>=ns ) error( 2, __FILE__, __LINE__,
           "Must combine surface with previous surface:", IntStr(n), "" );
        if( cmbn[ns] )
          if( cmbn[n] ) error( 2, __FILE__, __LINE__,
           "May not chain combined surfaces:", IntStr(n), "" );
        if( n<0 || n>vfCtrl->nRadSrf ) error( 2, __FILE__, __LINE__,
           "Improper combine surface number:", IntStr(n), "" );

        emit[ns] = ReadR4( 0 );       /* surface emittance */
        if( emit[ns] > 0.99901 )
          {
          error( 1, __FILE__, __LINE__,  " Replacing surface ", IntStr(ns),
             " emittance (", _string, ") with 0.999", "" );
          emit[ns] = 0.999f;
          }
        if( emit[ns] < 0.00099 )
          {
          error( 1, __FILE__, __LINE__,  " Replacing surface ", IntStr(ns),
             " emittance (", _string, ") with 0.001", "" );
          emit[ns] = 0.001f;
          }

        NxtWord( _string, 0, sizeof(_string) );        /* surface name */
        strncpy( name[ns], _string, NAMELEN );
        name[ns][NAMELEN-1] = '\0';  /* guarantee termination */
        break;

      case '/':               /* comment */
      case '!':
      case 'T':               /* title */
      case 'G':               /* geometry */
      case 'C':               /* control */
      case 'X':               /* coordinate transformation */
        break;

      case 'E':               /* end of data */
      case '*':
        goto finish;

      default:
        error(1,__FILE__, __LINE__, "Undefined input identifier", "" );
        break;
      }
    flag = 1;
    }

finish:
  Fre_V( xy, 1, vfCtrl->nVertices, sizeof(Vec2), __FILE__, __LINE__ );
  if( error( -1, __FILE__, __LINE__, "" )>0 )
    error( 3, __FILE__, __LINE__, "Fix errors in input data", "" );

  }  /*  end of GetVS2D  */


