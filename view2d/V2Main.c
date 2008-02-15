/*subfile:  v2main.c  ********************************************************/

/*  Main program for batch processing of 2-D view factors.  */

#ifdef _DEBUG
# define DEBUG 1
#else
# define DEBUG 0
#endif

#include <stdio.h>
#include <string.h> /* prototype: strcpy */
#include <stdlib.h> /* prototype: exit */
#include <math.h>   /* prototype: sqrt */
#include <time.h>   /* prototypes: time, localtime, asctime;
                       define: tm, time_t */
#include "types.h" 
#include "view2d.h"
#include "prtyp.h" 

/*  Main program for batch processing of 2-D view factors.  */

FILE *_unxt; /* input file */
FILE *_uout; /* output file */
FILE *_ulog; /* log file */
IX _echo=0;  /* true = echo input file */
IX _list;    /* output control, higher value = more output;
                0 = summary;
                1 = symmetric view factors + unconverged iterations;
                2 = view factor iterations;
                3 = echo input, etc.  */
I1 _string[LINELEN];  /* buffer for a character string */

void FindFile( I1 *msg, I1 *name, I1 *type );

main( IX argc, I1 **argv )
  {
  I1 program[]="View2D";   /* program name */
  I1 version[]="3.3";      /* program version */
  I1 inFile[_MAX_PATH]=""; /* input file name */
  I1 outFile[_MAX_PATH]="";/* output file name */
  I1 title[LINELEN];  /* project title */
  I1 **name;       /* surface names [1:nSrf][0:NAMELEN] */
  SRFDAT2D *srf;   /* vector of surface data structures [1:nSrf] */
  VFCTRL vfCtrl;   /* VF calculation control parameters */
  R8 **AF;         /* triangular array of area*view factor values [1:nSrf][] */
  R4 *area;        /* vector of surface areas [1:nSrf0] */
  R4 *emit;        /* vector of surface emittances [1:nSrf0] */
  IX *base;        /* vector of base surface numbers [1:nSrf0] */
  IX *cmbn;        /* vector of combine surface numbers [1:nSrf0] */
  R4 *vtmp;        /* temporary vector [1:nSrf0] */
  struct tm *curtime; /* time structure */
  time_t bintime;  /* seconds since 00:00:00 GMT, 1/1/70 */
  R4 time0, time1; /* elapsed time values */
  IX nSrf;         /* current number of surfaces */
  IX nSrf0;        /* initial number of surfaces */
  IX encl;         /* 1 = surfaces form enclosure */
  R4 eMax=0.0;     /* maximum row error, if enclosure */
  R4 eRMS=0.0;     /* RMS row error, if enclosure */
  IX n, flag;

  if( argv[1][0] == '?' )
    {
    fputs("\n\
    VIEW2D - compute view factors for a 2D geometry.\n\n\
      VIEW2D  input_file  output_file \n\n",
      stderr );
    exit( 1 );
    }
                /* open log file */
  _ulog = fopen( "View2D.log", "w" );

  fprintf( _ulog, "Program: %s\n", argv[0] );

  if( argc > 1 )
    strcpy( inFile, argv[1] );
  FindFile( "Enter name of V/S data file", inFile, "r" );
  fprintf( _ulog, "Data file:  %s\n", inFile );

  if( argc > 2 )
    strcpy( outFile, argv[2] );
  FindFile( "Enter name of VF output file", outFile, "w" );
  fprintf( _ulog, "Output file:  %s\n", outFile );

  time(&bintime);
  curtime = localtime(&bintime);
  fprintf( _ulog, "Time:  %s", asctime(curtime) );

  fputs("\n\
  View2D - calculation of view factors between 2-D planar surfaces.\n\
     Provided for review only.\n\
  This program is furnished by the government and is accepted by\n\
  any recipient with the express understanding that the United\n\
  States Government makes no warranty, expressed or implied,\n\
  concerning the accuracy, completeness, reliability, usability,\n\
  or suitability for any particular purpose of the information\n\
  and data contained in this program or furnished in connection\n\
  therewith, and the United States shall be under no liability\n\
  whatsoever to any person by reason of any use made thereof.\n\
  This program belongs to the government.  Therefore, the\n\
  recipient further agrees not to assert any proprietary rights\n\
  therein or to represent this program to anyone as other than\n\
  a government program.\n", stderr );

  time0 = CPUtime( 0.0 );

                 /* initialize control data */
  memset( &vfCtrl, 0, sizeof(VFCTRL) );

                 /* read Vertex/Surface data file */
  NxtOpen( inFile, __FILE__, __LINE__ );
  CountVS2D( title, &vfCtrl );
  fprintf( _ulog, "Title: %s\n", title );
  fprintf( _ulog, "Control values for 2-D view factor calculations:\n" );
  fprintf( _ulog, "     enclosure designator: %3d \n", vfCtrl.enclosure );
  fprintf( _ulog, " output control parameter: %3d \n", _list );
  fprintf( _ulog, "   obstructed convergence: %g \n", vfCtrl.epsObstr );
  fprintf( _ulog, "       maximum recursions: %3d \n", vfCtrl.maxRecursion );
  fprintf( _ulog, "       minimum recursions: %3d \n", vfCtrl.minRecursion );
  fprintf( _ulog, "       process emittances: %3d \n", vfCtrl.emittances );

  fprintf( _ulog, "\n" );
  fprintf( _ulog, " total number of surfaces: %3d \n", vfCtrl.nAllSrf );
  fprintf( _ulog, "   heat transfer surfaces: %3d \n", vfCtrl.nRadSrf );

  nSrf = nSrf0 = vfCtrl.nRadSrf;
  encl = vfCtrl.enclosure;
  name = Alc_MC( 1, nSrf0, 0, NAMELEN, sizeof(I1), __FILE__, __LINE__ );
  AF = Alc_MSC( 1, nSrf0, sizeof(R8), __FILE__, __LINE__ );
  area = Alc_V( 1, nSrf0, sizeof(R4), __FILE__, __LINE__ );
  emit = Alc_V( 1, nSrf0, sizeof(R4), __FILE__, __LINE__ );
  vtmp = Alc_V( 1, nSrf0, sizeof(R4), __FILE__, __LINE__ );
  base = Alc_V( 1, nSrf0, sizeof(IX), __FILE__, __LINE__ );
  cmbn = Alc_V( 1, nSrf0, sizeof(IX), __FILE__, __LINE__ );
  srf = Alc_V( 1, vfCtrl.nAllSrf, sizeof(SRFDAT2D), __FILE__, __LINE__ );

               /* read v/s data file */
  if( _echo )
    _echo = 0;
  else if( _list>3 )
    _echo = 1;
  GetVS2D( name, emit, base, cmbn, srf, &vfCtrl );
  for( n=1; n<=nSrf; n++ )
    area[n] = (R4)srf[n].area;
  NxtClose();

  if( encl )    /* determine volume of enclosure */
    {
    R8 volume=0.0;
    for( n=vfCtrl.nAllSrf; n; n-- )
      if( srf[n].type == 0 )
        volume += srf[n].v1.x * srf[n].v2.y - srf[n].v2.x * srf[n].v1.y;
    volume *= 0.5;
    fprintf( _ulog, "      volume of enclosure: %.3f\n", volume );
    }

  if( _list>2 )
    {
    fprintf( _ulog, "Surfaces:\n" );
    fprintf( _ulog, "   #     emit   base  cmbn   name\n" );
    for( n=1; n<=nSrf; n++ )
      fprintf( _ulog, "%4d    %5.3f %5d %5d    %s\n", n,
        emit[n], base[n], cmbn[n], name[n] );
    fprintf( _ulog, "Area, direction cosines:\n" );
    fprintf( _ulog, "   #     area        x          y          w\n" );
    for( n=1; n<=vfCtrl.nAllSrf; n++ )
      fprintf( _ulog, "%4d %10.5f %10.5f %10.5f %10.5f\n",
        n, srf[n].area, srf[n].dc.x, srf[n].dc.y, srf[n].dc.w );
    fprintf( _ulog, "Vertices:\n" );
    fprintf( _ulog, "   #      v1.x       v1.y       v2.x       v2.y\n" );
    for( n=1; n<=vfCtrl.nAllSrf; n++ )
      {
      fprintf( _ulog, "%4d %10.3f %10.3f %10.3f %10.3f\n",
        n, srf[n].v1.x, srf[n].v1.y, srf[n].v2.x, srf[n].v2.y );
      }
    }

  fprintf( stderr, "\nComputing view factors from surface N to surfaces 1 through N-1\n" );

  time1 = CPUtime( 0.0 );
  View2D( srf, AF, &vfCtrl );
  fprintf( _ulog, "\n%7.2f seconds to compute view factors.\n", CPUtime(time1) );
  Fre_V( srf, 1, vfCtrl.nAllSrf, sizeof(SRFDAT2D), __FILE__, __LINE__ );
  for( n=1; n<=nSrf; n++ )
    vtmp[n] = 1.0;
  if( _list>2 )
    ReportAF( nSrf, encl, "Initial view factors:",
      name, area, vtmp, base, AF, &eMax );

  fprintf( stderr, "\nAdjusting view factors\n" );
  time1 = CPUtime( 0.0 );

  for( flag=0, n=1; n<=nSrf; n++ )   /* separate subsurfaces */
    if(base[n]>0) flag = 1;
  if( flag )
    {
    Separate( nSrf, base, area, AF );
    for( n=1; n<=nSrf; n++ )
      base[n] = 0;
    if( _list>2 )
      ReportAF( nSrf, encl, "View factors after separating included surfaces:", 
        name, area, vtmp, base, AF, &eMax );
    }

  for( flag=0, n=1; n<=nSrf; n++ )   /* combine surfaces */
    if(cmbn[n]>0) flag = 1;
  if( flag )
    {
    nSrf = Combine( nSrf, cmbn, area, name, AF );
    if( _list>2 )
      {
      fprintf(_ulog,"Surfaces:\n");
      fprintf(_ulog,"  n   base  cmbn   area\n");
      for( n=1; n<=nSrf; n++ )
        fprintf(_ulog,"%3d%5d%6d%12.4e\n", n, base[n], cmbn[n], area[n] );
      ReportAF( nSrf, encl, "View factors after combining surfaces:",
        name, area, vtmp, base, AF, &eMax );
      }
    }

  eRMS = ReportAF( nSrf, encl, title, name, area, vtmp, base, AF, &eMax );
  if( encl )
    {
    fprintf( stderr, "\nMax enclosure error:  %.2e\n", eMax );
    fprintf( stderr, "RMS enclosure error:  %.2e\n", eRMS );
    }

  if( encl )         /* normalize view factors */
    {
    NormAF( nSrf, vtmp, area, AF, 1.0e-7, 30 );
    if( _list>2 )
      ReportAF( nSrf, encl, "View factors after normalization:",
        name, area, vtmp, base, AF, &eMax );
    }
  fprintf( _ulog, "%7.2f seconds to adjust view factors.\n", CPUtime(time1) );

#ifdef MEMTEST
  fprintf( _ulog, "Current %s\n", MemRem(_string) );
#endif
  if( vfCtrl.emittances )
    {
    fprintf( stderr, "\nProcessing surface emissivites\n" );
    time1 = CPUtime( 0.0 );
    for( n=1; n<=nSrf; n++ )
      vtmp[n] = area[n];
    IntFac( nSrf, emit, area, AF );
    fprintf( _ulog, "%7.2f seconds to include emissivities.\n",
      CPUtime(time1) );
    if( _list>2 )
      ReportAF( nSrf, encl, "View factors including emissivities:",
        name, area, emit, base, AF, &eMax );
    if( encl )
      NormAF( nSrf, emit, area, AF, 1.0e-7, 30 );   /* fix rounding errors */
    }

  if( vfCtrl.emittances )
    ReportAF( nSrf, encl, "Final view factors:",
      name, area, emit, base, AF, &eMax );
  else
    ReportAF( nSrf, encl, "Final view factors:",
      name, area, vtmp, base, AF, &eMax );

  time1 = CPUtime( 0.0 );
  SaveVF( outFile, program, version, vfCtrl.outFormat, vfCtrl.enclosure,
          vfCtrl.emittances, nSrf, area, emit, AF, vtmp );
  sprintf( _string, "%7.2f seconds to write view factors.\n", CPUtime(time1) );
  fputs( _string, stderr );
  fputs( _string, _ulog );


  Fre_V( cmbn, 1, nSrf0, sizeof(IX), __FILE__, __LINE__ );
  Fre_V( base, 1, nSrf0, sizeof(IX), __FILE__, __LINE__ );
  Fre_V( vtmp, 1, nSrf0, sizeof(R4), __FILE__, __LINE__ );
  Fre_V( emit, 1, nSrf0, sizeof(R4), __FILE__, __LINE__ );
  Fre_V( area, 1, nSrf0, sizeof(R4), __FILE__, __LINE__ );
  Fre_MSC( (void **)AF, 1, nSrf0, sizeof(R8), __FILE__, __LINE__ );
  Fre_MC( (void **)name, 1, nSrf0, 0, NAMELEN, sizeof(I1), __FILE__, __LINE__ );

  fprintf( _ulog, "%7.2f seconds for all calculations.\n\n", CPUtime(time0) );
  fclose( _ulog );

  fprintf( stderr, "\nDone!\n" );

  return 0;

  }  /* end of main */

#include <ctype.h>  /* prototype: toupper */

/***  CountVS2D.c  ***********************************************************/

/*  Determine number of vertices, number of surfaces,
 *  control parameters, and format of the input file.  */

void CountVS2D( char *title, VFCTRL *vfCtrl )
  {
  IX c;     /* first character in line */
  IX ctrl=0;
  IX flag=0;  /* NxtWord flag: 0 for first word of first line */

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

void GetVS2D( I1 **name, R4 *emit, IX *base, IX *cmbn,
  SRFDAT2D *srf, VFCTRL *vfCtrl )
  {
  I1 c;       /* first character in line */
  IX nv=0;    /* number of vertices */
  IX ns=0;    /* number of surfaces */
  IX flag=0;  /* NxtWord flag: 0 for first word of first line */
  VERTEX2D *xy;  /* vector of vertces [1:nVertices] */
  IX n;

  error( -2, __FILE__, __LINE__, "" );  /* clear error count */
  rewind( _unxt );
  xy = Alc_V( 1, vfCtrl->nVertices, sizeof(VERTEX2D), __FILE__, __LINE__ );

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

        SetSrf( &srf[ns] );          /* compute area and direction cosines */

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
            IX i;
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
  Fre_V( xy, 1, vfCtrl->nVertices, sizeof(VERTEX2D), __FILE__, __LINE__ );
  if( error( -1, __FILE__, __LINE__, "" )>0 )
    error( 3, __FILE__, __LINE__, "Fix errors in input data", "" );

  }  /*  end of GetVS2D  */

/***  ReportAF.c  ************************************************************/

R4 ReportAF( const IX nSrf, const IX encl, const I1 *title, const I1 **name,
  const R4 *area, const R4 *emit, const IX *base, R8 **AF, R4 *eMax )
  {
  IX n;    /* row */
  IX m;    /* column */
  R4 err;  /* error values assuming enclosure */
  R8 F, sumF;  /* view factor, sum of F for row */
/*  R4 eMax;   /* maximum row error, if enclosure */
  R8 eRMS=0.;  /* sum for RMS error */
#define MAXEL 10
  struct
    {
    R4 err;   /* enclosure error */
    IX n;     /* row number */
    } elist[MAXEL+1];
  IX i;

  fprintf( _ulog, "\n%s\n", title );
  if( encl && _list>0 )
    fprintf( _ulog, "          #        name   SUMj Fij (encl err)\n" );
  memset( elist, 0, sizeof(elist) );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    {
    for( sumF=0.0,m=1; m<=n; m++ )  /* compute sum of view factors */
      if( base[m] == 0 )
        sumF += AF[n][m];
    for( ; m<=nSrf; m++ )
      if( base[m] == 0 )
        sumF += AF[m][n];
    sumF /= area[n];
    if( _list>0 )
      {
      fprintf( _ulog, " Row:  %4d %12s %9.6f", n, name[n], sumF );
      if( encl )
        fprintf( _ulog, " (%.6f)", fabs( sumF - emit[n] ) );
      fputc( '\n', _ulog );
      }

    if( encl )                /* compute enclosure error value */
      {
      err = (R4)fabs( sumF - emit[n]);
      eRMS += err * err;
      for( i=MAXEL; i>0; i-- )
        {
        if( err<=elist[i-1].err ) break;
        elist[i].err = elist[i-1].err;
        elist[i].n = elist[i-1].n;
        }
      elist[i].err = err;
      elist[i].n = n;
      }

    if( _list>1 )   /* print row n values */
      {
      R8 invArea = 1.0 / area[n];
      for( m=1; m<=nSrf; m++ )
        {
        if( m>=n )
          F = AF[m][n] * invArea;
        else
          F = AF[n][m] * invArea;
        sprintf( _string, "%8.6f ", F );
        if( m%10==0 )  _string[8] = '\n';
        fprintf( _ulog, "%s", _string+1 );
        }
      if( m%10!=1 ) fputc( '\n', _ulog );
      }
    }  /* end of row n */

  if( encl )     /* print enclosure error summary */
    {
    fprintf( _ulog, "Summary:\n" );
    *eMax = elist[0].err;
    fprintf( _ulog, "Max enclosure error:  %.2e\n", *eMax );
    eRMS = sqrt( eRMS/nSrf );
    fprintf( _ulog, "RMS enclosure error:  %.2e\n", eRMS );
    if( elist[0].err>0.5e-6 )
      {
      fprintf( _ulog, "Largest errors [row, error]:\n" );
      for( i=0; i<MAXEL; i++ )
        {
        if( elist[i].err<0.5e-6 ) break;
        fprintf( _ulog, "%8d%10.6f\n", elist[i].n, elist[i].err );
        }
      }
    fprintf( _ulog, "\n" );
    }

  return (R4)eRMS;

  }  /* end of ReportAF */

/***  FindFile.c  ************************************************************/

/*  Find user designated file.  REPLACE WITH FILEOPEN.TXT ???
 *  First character of type string must be 'r', 'w', or 'a'.  */

void FindFile( I1 *msg, I1 *fileName, I1 *type )
/*  msg;    message to user
 *  name;   file name (string long enough for any file name)
 *  type;   type of file, see fopen() */
  {
  FILE  *pfile=NULL;

  while( !pfile )
    {
    if( fileName[0] )   /* try to open file */
      {
      pfile = fopen( fileName, type );
      if( pfile == NULL )
        fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
      }
    if( !pfile )        /* ask for file name */
      {
      fprintf( stderr, "%s: ", msg );
      scanf( "%s", fileName );
      }
    }

  fclose( pfile );

  }  /*  end of FindFile  */

#ifdef XXX
/***  OpenFile.c  ************************************************************/

/*  Open user designated file.
 *  First character of type string must be 'r', 'w', or 'a'.  */

FILE *OpenFile( I1 *msg, I1 *name, I1 *type )
/*  msg;    message to user
 *  name;   file name (string long enough for any file name)
 *  type;   type of file, see fopen() */
  {
  FILE  *pfile=NULL;

tryagain:
  fputs( msg, stderr );
  fputs( ": ", stderr );
  scanf( "%s", name );
  pfile = fopen( name, type );
  if( pfile == NULL )
    {
    fputs( "Error! Failed to open file: ", stderr );
    fputs( name, stderr );
    fputs( "\nTry again.\n", stderr );
    goto tryagain;
    }

  return (pfile);

  }  /*  end of OpenFile  */

/***  setlpos2.c  ************************************************************/

/*  Determine list of possible view obstructing surfaces  */

IX setlpos2( SRFDATD *srfv, IX *stype, IX *lpos )
/*
 * stype;  surface type data:  1 = obstruction only surface,
 *         0 = normal surface, -1 = included surface,
 *        -2 = part of an obstruction surface
 */
  {
  IX npos;     /* number of possible view obstructing surfaces */
  IX m, n;     /* surface numbers */
  IX infront;  /* true if a vertex is in front of surface n */
  IX behind;   /* true if a vertex is behind surface n */
  R4 t;

  for( npos=0,n=_ntot; n; n-- )
    {
    if( stype[n] < 0 ) continue;
    infront = behind = 0;
    for( m=1; m<=_ntot; m++ )
      {
      if( stype[m] < 0 ) continue;
      t = srfv[n].hca * srfv[m].v.x1 + srfv[n].hcb * srfv[m].v.y1 + srfv[n].hcc;
      if(t > 0.0) infront = 1;
      if(t < 0.0) behind = 1;
      t = srfv[n].hca * srfv[m].v.x2 + srfv[n].hcb * srfv[m].v.y2 + srfv[n].hcc;
      if(t > 0.0) infront = 1;
      if(t < 0.0) behind = 1;
      if(infront && behind) break;
      }  /* end m loop */
    if(infront && behind)
      lpos[++npos] = n;
    }  /* end n loop */

  return npos;

  }  /*  end of setlpos2  */
#endif

