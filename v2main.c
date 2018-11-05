/*subfile:  v2main.c  ********************************************************/

/*  Main program for batch processing of 2-D view factors.  */

#ifdef _DEBUG
# define DEBUG 1
#else
# define DEBUG 0
#endif

#include "config.h"
#include "view2d.h"

#include <stdio.h>
#include <string.h> /* prototype: strcpy */
#include <stdlib.h> /* prototype: exit */
#include <math.h>   /* prototype: sqrt */
#include <time.h>   /* prototypes: time, localtime, asctime; define: tm, time_t */

#ifndef LIBONLY
#include <unistd.h>
#endif
#include <stdlib.h>

#include "types.h"
#include "misc.h"
#include "heap.h"
#include "test2d.h"
#include "viewpp.h"
#include "getdat.h"
#include "savevf.h"

/*  Main program for batch processing of 2-D view factors.  */

FILE *_uout; /* output file */

/* forward decls */

void FindFile( char *msg, char *name, char *type );

static float ReportAF(const int nSrf, const int encl, const char *title
	, char * const *const name
	, const float *area, const float *emit, const int *base, double **AF, float *eMax
);

int processPaths2d(char *inFile, char *outFile);
int processHandles2d(FILE *inHandle, FILE *outHandle);

/*------------------------------------------------------------------------------
  VIEW2D driver
*/

void usage(const char *progname){
	fprintf(stderr,
		"VIEW2D - compute view factors for a 2D geometry. Version %s.\n"
		"Usage: %s INFILE.vs2 OUTFILE.txt\n"
			, V3D_VERSION, progname
	);
}

#ifndef LIBONLY
int main( int argc, char **argv ){
  char inFile[_MAX_PATH]=""; /* input file name */
  char outFile[_MAX_PATH]="";/* output file name */
  struct tm *curtime; /* time structure */
  time_t bintime;  /* seconds since 00:00:00 GMT, 1/1/70 */


	char c;
	while((c=getopt(argc,argv,"?"))!=-1){
		switch(c){
			case '?':
				usage(argv[0]);
				exit(1);
		}
	}

	if(optind != argc - 2){
		fprintf(stderr,"ERROR: missing command-line arguments\n");
		usage(argv[0]);
		exit(1);
	}

	strcpy( inFile, argv[optind++] );
	strcpy( outFile, argv[optind++] );

	/* check that files exist */
	FILE *f;
	f = fopen(inFile,"r");
	if(f==NULL){
		fprintf(stderr,"ERROR: failed to open input file '%s' for reading!\n",inFile);
		exit(1);
	}
	fclose(f);

	/* open log file */
	_ulog = fopen( "View2D.log", "w" );
	if(_ulog==NULL){
		fprintf(stderr,"Unable to open logfile for writing\n");
		exit(1);
	}

	fprintf( _ulog, "Program: %s\n", argv[0] );
	fprintf( _ulog, "Version: %s\n", V3D_VERSION );
	fprintf( _ulog, "Data file:   %s\n", inFile );
	fprintf( _ulog, "Output file: %s\n", outFile );

	time(&bintime);
	curtime = localtime(&bintime);
	fprintf( _ulog, "Time:  %s", asctime(curtime) );

#if 0
	/* this text has been moved to LICENSE.txt */
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
#endif

  return processPaths2d(inFile, outFile);

}  /* end of main */
#endif

int processPaths2d(char *inFile, char *outFile) {
  FILE *inHandle = NxtOpenHndl(inFile, __FILE__, __LINE__ );
  _unxt = inHandle;
  /* Write the results to the output file. */
  /* TODO: if saving to binary format, open for binary write */
  FILE *outHandle;
  if(strlen(outFile) == 0 || outFile == NULL) {
    outHandle = stdout;
  } else {
    outHandle = fopen(outFile, "w");
  }
  return processHandles2d(inHandle, outHandle);
}

int processHandles2d(FILE *inHandle, FILE *outHandle) {
  char program[]="View2D";   /* program name */
  char version[]=V3D_VERSION;      /* program version */
  char title[LINELEN];  /* project title */
  char **name;       /* surface names [1:nSrf][0:NAMELEN] */
  SRFDAT2D *srf;   /* vector of surface data structures [1:nSrf] */
  View2DControlData vfCtrl;   /* VF calculation control parameters */
  double **AF;         /* triangular array of area*view factor values [1:nSrf][] */
  float *area;        /* vector of surface areas [1:nSrf0] */
  float *emit;        /* vector of surface emittances [1:nSrf0] */
  int *base;        /* vector of base surface numbers [1:nSrf0] */
  int *cmbn;        /* vector of combine surface numbers [1:nSrf0] */
  float *vtmp;        /* temporary vector [1:nSrf0] */
  struct tm *curtime; /* time structure */
  time_t bintime;  /* seconds since 00:00:00 GMT, 1/1/70 */
  float time0, time1; /* elapsed time values */
  int nSrf;         /* current number of surfaces */
  int nSrf0;        /* initial number of surfaces */
  int encl;         /* 1 = surfaces form enclosure */
  float eMax=0.0;     /* maximum row error, if enclosure */
  float eRMS=0.0;     /* RMS row error, if enclosure */
  int n, flag;


	/* open log file */
	_ulog = fopen( "View2D.log", "w" );
	if(_ulog==NULL){
		fprintf(stderr,"Unable to open logfile for writing\n");
		exit(1);
	}

	time(&bintime);
	curtime = localtime(&bintime);
	fprintf( _ulog, "Time:  %s", asctime(curtime) );

#if 0
	/* this text has been moved to LICENSE.txt */
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
#endif

  time0 = CPUtime( 0.0 );

                 /* initialize control data */
  memset( &vfCtrl, 0, sizeof(View2DControlData) );

                 /* read Vertex/Surface data file */
  CountVS2D( _unxt, title, &vfCtrl );
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
  name = Alc_MC( 1, nSrf0, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
  AF = Alc_MSC( 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
  area = Alc_V( 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  emit = Alc_V( 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  vtmp = Alc_V( 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  base = Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  cmbn = Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  srf = Alc_V( 1, vfCtrl.nAllSrf, sizeof(SRFDAT2D), __FILE__, __LINE__ );

               /* read v/s data file */
  if( _echo )
    _echo = 0;
  else if( _list>3 )
    _echo = 1;
  GetVS2D( _unxt, name, emit, base, cmbn, srf, &vfCtrl );
  for( n=1; n<=nSrf; n++ )
    area[n] = (float)srf[n].area;
  NxtClose();

  if( encl )    /* determine volume of enclosure */
    {
    double volume=0.0;
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
  /* This is the routine that computes the view factors */
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

  SaveVF( outHandle, program, version, vfCtrl.outFormat, vfCtrl.enclosure,
          vfCtrl.emittances, nSrf, area, emit, AF, vtmp );
  sprintf( _string, "%7.2f seconds to write view factors.\n", CPUtime(time1) );
  fputs( _string, stderr );
  fputs( _string, _ulog );


  Fre_V( cmbn, 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  Fre_V( base, 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  Fre_V( vtmp, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_V( emit, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_V( area, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_MSC( (void **)AF, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
  Fre_MC( (void **)name, 1, nSrf0, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );

  fprintf( _ulog, "%7.2f seconds for all calculations.\n\n", CPUtime(time0) );
  fclose( _ulog );

  fprintf( stderr, "\nDone!\n" );

  return 0;

}

#include <ctype.h>  /* prototype: toupper */

/***  ReportAF.c  ************************************************************/

float ReportAF(const int nSrf, const int encl, const char *title
	, char *const *const name, const float *area, const float *emit, const int *base
	, double **AF, float *eMax
){
  int n;    /* row */
  int m;    /* column */
  float err;  /* error values assuming enclosure */
  double F, sumF;  /* view factor, sum of F for row */
/*  float eMax;   * maximum row error, if enclosure * */
  double eRMS=0.;  /* sum for RMS error */
#define MAXEL 10
  struct
    {
    float err;   /* enclosure error */
    int n;     /* row number */
    } elist[MAXEL+1];
  int i;

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
      err = (float)fabs( sumF - emit[n]);
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
      double invArea = 1.0 / area[n];
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

  return (float)eRMS;

  }  /* end of ReportAF */

/***  FindFile.c  ************************************************************/

/*  Find user designated file.  REPLACE WITH FILEOPEN.TXT ???
 *  First character of type string must be 'r', 'w', or 'a'.  */

void FindFile( char *msg, char *fileName, char *type )
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
        fprintf( stderr, "ERROR: Failed to open '%s'. Try again...\n", fileName );
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

FILE *OpenFile( char *msg, char *name, char *type )
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

int setlpos2( SRFDATD *srfv, int *stype, int *lpos )
/*
 * stype;  surface type data:  1 = obstruction only surface,
 *         0 = normal surface, -1 = included surface,
 *        -2 = part of an obstruction surface
 */
  {
  int npos;     /* number of possible view obstructing surfaces */
  int m, n;     /* surface numbers */
  int infront;  /* true if a vertex is in front of surface n */
  int behind;   /* true if a vertex is behind surface n */
  float t;

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
