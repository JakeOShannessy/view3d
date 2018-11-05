/*subfile:  V3Main.c  ********************************************************/

/*  Main program for batch processing of 3-D view factors.  */

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

#include "config.h"
#include "types.h"
#include "view3d.h"
#include "misc.h"
#include "readvf.h"
#include "savevf.h"
#include "heap.h"
#include "polygn.h"
#include "viewpp.h"
#include "getdat.h"
#include "test3d.h"

/* forward decls */

void FindFile(char *msg, char *name, char *type);
void CheckFileWritable(char *fileName);
void CheckFileReadable(char *fileName);
double VolPrism(Vec3 *a, Vec3 *b, Vec3 *c);
VFResultsC processPathsMain(char *inFile, char *outFile);
VFResultsC processHandlesMain(FILE *inHandle, FILE *outHandle);
InData readFileHandle(FILE *inHandle);

void ReportAF( const int nSrf, const int encl, const char *title, char **name,
  const float *area, const float *emit, const int *base, double **AF, int flag
);

/*----------------------------------------------------------------------------*/
int main( int argc, char **argv ){
  char program[]="View3D";   /* program name */
  char version[]=V3D_VERSION;      /* program version */
  char inFile[_MAX_PATH]=""; /* input file name */
  char outFile[_MAX_PATH]="";/* output file name */
#ifndef ANSI
  char fileName[_MAX_PATH];  /* name of file */
  char vdrive[_MAX_DRIVE];   /* drive letter for program View3D.exe */
  char vdir[_MAX_DIR];       /* directory path for program View3D.exe */
#endif
  char title[LINELEN]; /* project title */
  char **name;         /* surface names [1:nSrf][0:NAMELEN] */
  char *types[]={"rsrf","subs","mask","nuls","obso"};
  Vec3 *xyz;           /* vector of vertces [1:nVrt] - for ease in
                          converting V3MAIN to a subroutine */
  SRFDAT3D *srf;       /* vector of surface data structures [1:nSrf] */
  View3DControlData vfCtrl; /* VF calculation control parameters - avoid globals */
  double **AF;         /* triangular array of area*view factor values [1:nSrf][] */
  float *area;         /* vector of surface areas [1:nSrf] */
  float *emit;         /* vector of surface emittances [1:nSrf] */
  int *base;           /* vector of base surface numbers [1:nSrf] */
  int *cmbn;           /* vector of combine surface numbers [1:nSrf] */
  float *vtmp;         /* temporary vector [1:nSrf] */
  int *possibleObstr;  /* list of possible view obstructing surfaces */
  struct tm *curtime;  /* time structure */
  time_t bintime;      /* seconds since 00:00:00 GMT, 1/1/70 */
  float time0, time1;  /* elapsed time values */
  int nSrf;            /* current number of surfaces */
  int nSrf0;           /* initial number of surfaces */
  int encl;            /* 1 = surfaces form enclosure */
  int n, flag;

  if( argc == 1 || argv[1][0] == '?' ){
    fprintf(stderr,"\n"
		"VIEW3D - compute view factors for a 3D geometry. Version %s.\n\n"
		"Usage: %s INFILE.vs3 OUTFILE.txt\n\n"
		"You may also enter the file names interactively.\n\n"
		, V3D_VERSION, argv[0]
	);
    if( argc > 1 )
      exit( 1 );
  }

  /* open log file */
  // TODO: Make logging to a particular file an option, log to stderr by
  // default.
#ifdef ANSI
  // _ulog = fopen( "View3D.log", "w" );
  _ulog = stderr;
#else
  // PathSplit( argv[0], vdrive, sizeof(vdrive), vdir, sizeof(vdir), NULL, 0, NULL, 0 );
  // PathMerge( fileName, sizeof(fileName), vdrive, vdir, "View3D", ".log" );
  // _ulog = fopen( fileName, "w" );
  _ulog = stderr;
#endif
  if( !_ulog )
    error( 3, __FILE__, __LINE__, "Failed to open View3D.log", "" );

#if( DEBUG > 0 )
  _echo = 1;
#endif

  fprintf( _ulog, "Program: %s %s\n", program, version );
  fprintf( _ulog, "Executing: %s\n", argv[0] );

  if( argc > 1 ){
    strcpy( inFile, argv[1] );
    CheckFileReadable(inFile);
  }
    // TODO: specify a non-interactive mode.
  // FindFile("Enter name of input (vertex/surface) data file", inFile, "r" );
  fprintf(_ulog, "Data file:  %s\n", inFile );

  if( argc > 2 ) {
    strcpy( outFile, argv[2] );
    CheckFileWritable(outFile);
  } else {
    // outFile = NULL;
  }
    // TODO: if there is not output file set, output to stdout.
  // FindFile("Enter name of output (view factor) file", outFile, "w" );
  fprintf(_ulog, "Output file:  %s\n", outFile );

  time(&bintime);
  curtime = localtime(&bintime);
  fprintf(_ulog, "Time:  %s", asctime(curtime) );
  fputs("\n"
	"View3D - calculation of view factors between simple polygons.\n"
	"\n"
	"This program is furnished by the government and is accepted by\n"
	"any recipient with the express understanding that the United\n"
	"States Government makes no warranty, expressed or implied,\n"
	"concerning the accuracy, completeness, reliability, usability,\n"
	"or suitability for any particular purpose of the information\n"
	"and data contained in this program or furnished in connection\n"
	"therewith, and the United States shall be under no liability\n"
	"whatsoever to any person by reason of any use made thereof.\n"
	"This program belongs to the government.  Therefore, the\n"
	"recipient further agrees not to assert any proprietary rights\n"
	"therein or to represent this program to anyone as other than\n"
	"a government program.\n"
	, stderr
  );
  processPathsMain(inFile, outFile);
  return 0;
}

VFResultsC processPathsMain(char *inFile, char *outFile) {
  FILE *inHandle = NxtOpenHndl(inFile, __FILE__, __LINE__ );
  _unxt = inHandle;
  // Write the results to the output file.
  // TODO: if saving to binary format, open for binary write
  FILE *outHandle;
  if(strlen(outFile) == 0 || outFile == NULL) {
    outHandle = stdout;
  } else {
    outHandle = fopen(outFile, "w");
  }
  return processHandlesMain(inHandle, outHandle);
}

/*----------------------------------------------------------------------------*/
VFResultsC processHandlesMain(FILE *inHandle, FILE *outHandle) {
  if(_ulog==NULL) {
    _ulog = stderr;
  }
  char program[]="View3D";   /* program name */
  char version[]=V3D_VERSION;      /* program version */
  #ifndef ANSI
  char fileName[_MAX_PATH];  /* name of file */
  char vdrive[_MAX_DRIVE];   /* drive letter for program View3D.exe */
  char vdir[_MAX_DIR];       /* directory path for program View3D.exe */
  #endif
  char title[LINELEN]; /* project title */
  char *types[]={"rsrf","subs","mask","nuls","obso"};
  View3DControlData vfCtrl; /* VF calculation control parameters - avoid globals */
  double **AF;         /* triangular array of area*view factor values [1:nSrf][] */
  int *possibleObstr;  /* list of possible view obstructing surfaces */
  struct tm *curtime;  /* time structure */
  time_t bintime;      /* seconds since 00:00:00 GMT, 1/1/70 */
  float time0, time1;  /* elapsed time values */
  int nSrf;            /* current number of surfaces */
  int nSrf0;           /* initial number of surfaces */
  int encl;            /* 1 = surfaces form enclosure */
  int n, flag;
  time(&bintime);
  curtime = localtime(&bintime);
  fprintf(_ulog, "Time:  %s", asctime(curtime) );

  time0 = CPUtime( 0.0 );  /* start-of-run time */
  InData inData = readFileHandle(inHandle);
  fprintf(_ulog, "Done reading\n");
  encl = inData.vfCtrl.enclosure;
  nSrf = nSrf0 = inData.vfCtrl.nRadSrf;
  vfCtrl = inData.vfCtrl;
  char **name = inData.name;
  float *area = inData.area;
  float *emit = inData.emit;
  float *vtmp = inData.vtmp;
  int *base = inData.base;
  int *cmbn = inData.cmbn;
  Vec3 *xyz = inData.xyz;
  SRFDAT3D *srf = inData.srf; /* the surface data */

  // encl is a boolean from the input file and indicates if the surfaces are
  // supposed to form an enclosure. This is of no consequence to the
  // calculations and is simply printed.
  if(encl){
    /* determine volume of enclosure */
    double volume=0.0;
    // Loop through each of the surfaces.
    for( n=vfCtrl.nAllSrf; n; n-- ){
      // If it is a subsurface (SUBS) we skip it.
      if( srf[n].type == SUBS ) continue;
      volume += VolPrism( srf[n].v[0], srf[n].v[1], srf[n].v[2] );
      if( srf[n].nv == 4 )
        volume += VolPrism( srf[n].v[2], srf[n].v[3], srf[n].v[0] );
    }
    volume /= -6.0;        /* see VolPrism() */
    fprintf( _ulog, "      volume of enclosure: %.3f\n", volume );
  }

  // If there are 1,000 or more surfaces then print diagnostics, otherwise it's
  // not really worth it.
  if( nSrf0 >= 1000 ){
    sprintf( _string, "\n %.2f seconds to process input data\n", CPUtime(time0) );
    fputs( _string, stderr );
    fputs( _string, _ulog );
  }

  // If verbosity (_list) is greater than 2, print some information on the
  // surfaces.
  if( _list>2 ){
    fprintf( _ulog, "Surfaces:\n" );
    fprintf( _ulog, "   #        name     area   emit  type bsn csn (dir cos) (centroid)\n" );
    for( n=1; n<=nSrf; n++ )
      fprintf( _ulog, "%4d %12s %9.2e %5.3f %4s %3d %3d (%g %g %g %g) (%g %g %g)\n",
        n, name[n], area[n], emit[n], types[srf[n].type], base[n], cmbn[n],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );
    for( ; n<=vfCtrl.nAllSrf; n++ )
      fprintf( _ulog, "%4d %12s %9.2e       %4s         (%g %g %g %g) (%g %g %g)\n",
        n, " ", area[n], types[srf[n].type],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );

    fprintf( _ulog, "Vertices:\n" );
    for( n=1; n<=vfCtrl.nAllSrf; n++ ){
      int j;
      fprintf( _ulog, "%4d ", n );
      for( j=0; j<srf[n].nv; j++ )
        fprintf( _ulog, " (%g %g %g)",
          srf[n].v[j]->x, srf[n].v[j]->y, srf[n].v[j]->z );
      fprintf( _ulog, "\n" );
    }
  }

  // View factor calculations start from here.
  /* start-of-VF-calculation time */
  time1 = CPUtime( 0.0 );
  // Allocate some space to store the list of possible obstructions. This array
  // will never be longer than all the surfaces, so make it that length.
  possibleObstr = Alc_V( 1, vfCtrl.nAllSrf, sizeof(int), __FILE__, __LINE__ );
  // Find the number of possible obstructing surfaces. The index list of these
  // surfaces is stored in possibleObstr.
  vfCtrl.nPossObstr = SetPosObstr3D( vfCtrl.nAllSrf, srf, possibleObstr );
  // Print diagnostics on finding possibly obstructing surfaces
  sprintf( _string, "\n %.2f seconds to determine %d possible view obstructing surfaces",
           CPUtime(time1), vfCtrl.nPossObstr
  );
  fputs( _string, stderr );
  fputs( "\n\n", stderr );
  fputs( _string, _ulog );
  // Dump the list of possibly obstructing surfaces, but only if the verbosity
  // is greater than 1.
  if( vfCtrl.nPossObstr > 0 && _list > 1 )
    DumpOS( ":", vfCtrl.nPossObstr, possibleObstr );
  else
    fputs( "\n", _ulog );
  fflush( _ulog );

  // If row is specified (i.e. we are only interested in the view factors of
  // one surface) then we allocate an array big enough for those values.
  if( vfCtrl.row ){  // may not work with some compilers. GNW 2008/01/22
    AF = Alc_MC( vfCtrl.row, vfCtrl.row, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
    // If col is specified, we are only interested in the view factor between
    // two particular surfaces and don't even need to allocate
    if( vfCtrl.col ){
      fprintf( stderr, "\nComputing view factor for surface %d to surface %d\n\n",
        vfCtrl.row, vfCtrl.col );
    }else{
      fprintf( stderr, "\nComputing view factors for surface %d\n\n",
        vfCtrl.row );
    }
  // Otherwise we want every surface to every surface and must allocate a
  // sufficiently sized array.
  }else{
#ifdef __TURBOC__
    AF = Alc_MSR( 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#else
    AF = Alc_MSC( 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#endif
    fprintf( stderr, "\nComputing view factors for all %d surfaces\n\n", nSrf0 );
  }

  if( _list>0 ){
    MemNet( "At start of View3D()" );
  }
  time1 = CPUtime( 0.0 );

  /*----- view factor calculation -----*/
  View3D( srf, base, possibleObstr, AF, &vfCtrl );
  // ReportAF( nSrf, encl, "Calculated view factors:",
  //       name, area, emit, base, AF, 0 );
  // The view factors have now been calculated and stored in AF.
  fprintf( _ulog, "\n%7.2f seconds to compute view factors.\n", CPUtime(time1) );
  if( _list>0 )
    MemNet( "At end of View3D()" );

  // The possibly obstruction surface information is no longer needed after
  // this point.
  Fre_V( possibleObstr, 1, vfCtrl.nAllSrf, sizeof(int), __FILE__, __LINE__ );
  FreeTmpVertMem();  /* free polygon overlap vertices */
  FreePolygonMem();
  Fre_V( xyz, 1, vfCtrl.nVertices, sizeof(Vec3), __FILE__, __LINE__ );
  fprintf(stderr, "vfCtrl.row: %d\n", vfCtrl.row);
  fprintf(stderr, "vfCtrl.col: %d\n", vfCtrl.col);

  // If only the row is specified, we simply print the values for that row
  if( vfCtrl.row ){
    int n=vfCtrl.row,
       m=vfCtrl.col;
    double ai=1/area[n];
    double F, sum;
    fprintf( _ulog, "\n" );
    if( vfCtrl.col ){
      // If the column and row are both specified, we can simply print that
      // single value and end
      fprintf( _ulog, "F[%d][%d] = %.5e\n\n", n, m, AF[n][m]*ai );
      goto FreeMemory;
    }
    if( _list>0 ){
      fprintf( _ulog, "View factors from surface n (%d = %s) to surface m:\n", n, name[n] );
      fprintf( _ulog, "srf m   A(n)*F(n,m)    F(n,m)        F(m,n)      base   cmb  name\n" );
      for( sum=0,m=1; m<=nSrf; m++ ){
        F = AF[n][m]*ai;
        sum += F;
        fprintf( _ulog, "%5d %13.5e %13.5e %13.5e %5d %5d  %s\n",
          m, AF[n][m], F, AF[n][m]/area[m], base[m], cmbn[m], name[m] );
      }
      fprintf( _ulog, "    sum[F(n,m)] = %.8f\n\n", sum );
    }
    // separate subsurfaces
    // combine surfaces
    for( m=1; m<=nSrf; m++ ){
      if( cmbn[m] ){
        AF[n][cmbn[m]] += AF[n][m];
        area[cmbn[m]] += area[m];
      }
    }
    fprintf( _ulog, "View factors from surface n (%d = %s) to combined surface m:\n", n, name[n] );
    fprintf( _ulog, "srf m   A(n)*F(n,m)    F(n,m)        F(m,n)      name\n" );
    sum = 0;
    for( sum=0,m=1; m<=nSrf; m++ ){
      if( cmbn[m] == 0 ){
        F = AF[n][m]*ai;
        sum += F;
        fprintf( _ulog, "%5d %13.5e %13.5e %13.5e  %s\n",
          m, AF[n][m], F, AF[n][m]/area[m], name[m] );
      }
    }
    fprintf( _ulog, "\n    sum[F(n,m)] = %.8f\n\n", sum );
    fflush( _ulog );
    goto FreeMemory;
  }

  for( n=nSrf; n; n-- ){  /* clear base pointers to OBSO & MASK srfs */
	// FIXME should the following line say 'srf[n]'??
	if( srf[base[n]].type == OBSO )  /* Base is used for several things. */
      base[n] = 0;                   /* It must be progressively cleared */

	// FIXME should the following line say 'base[n]'??
    if( srf[n].type == MASK )        /* as each use is completed. */
      base[n] = 0;
  }

  if( _list>1 ){
    int *jtmp = Alc_V( 1, nSrf, sizeof(int), __FILE__, __LINE__ );
    for( n=nSrf; n; n-- )
     if( srf[n].type == NULS )
       jtmp[n] = 0;
     else
       jtmp[n] = base[n];
    ReportAF( nSrf, encl, "Initial view factors:",
      name, area, vtmp, jtmp, AF, 0 );
    Fre_V( jtmp, 1, nSrf, sizeof(int), __FILE__, __LINE__ );
  }

  fprintf( stderr, "\nAdjusting view factors\n" );
  time1 = CPUtime( 0.0 );

  // Determine if any of the surfaces are NULS
  for( flag=0,n=nSrf; n; n-- )
    if( srf[n].type==NULS ) flag = 1;
  // If there are, run the DelNull routine to remove them.
  if( flag ){                         /* remove null surfaces */
    nSrf = DelNull( nSrf, srf, base, cmbn, emit, area, name, AF );
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors after removing null surfaces:",
        name, area, vtmp, base, AF, 0 );
  }

  for( flag=0,n=nSrf; n; n-- )
    if( base[n]>0 ) flag = 1;
  if( flag ){                         /* separate subsurfaces */
    Separate( nSrf, base, area, AF );
    for( n=nSrf; n; n-- )
      base[n] = 0;
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors after separating included surfaces:",
        name, area, vtmp, base, AF, 0 );
  }

  for( flag=0,n=nSrf; n; n-- )
    if( cmbn[n]>0 ) flag = 1;
  if( flag ){                         /* combine surfaces */
    nSrf = Combine( nSrf, cmbn, area, name, AF );
    if( _list>1 ){
      fprintf(_ulog,"Surfaces:\n");
      fprintf(_ulog,"  n   base  cmbn   area\n");
      for( n=1; n<=nSrf; n++ )
        fprintf(_ulog,"%3d%5d%6d%12.4e\n", n, base[n], cmbn[n], area[n] );
      ReportAF( nSrf, encl, "View factors after combining surfaces:",
        name, area, vtmp, base, AF, 0 );
    }
  }

  if( encl || vfCtrl.emittances )    // intermediate report -
    if( _list < 2 )                  // unnormalized view factors
      ReportAF( nSrf, encl, title, name, area, vtmp, base, AF, 1 );

  if( encl ){                         /* normalize view factors */
    NormAF( nSrf, vtmp, area, AF, 1.0e-7f, 100 );
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors after normalization:",
        name, area, vtmp, base, AF, 0 );
  }
  sprintf( _string, "%7.2f seconds to adjust view factors.\n", CPUtime(time1) );
  fputs( _string, stderr );
  fputs( _string, _ulog );

  if( vfCtrl.emittances ){
    fprintf( stderr, "\nProcessing surface emissivites\n" );
    time1 = CPUtime( 0.0 );
    IntFac( nSrf, emit, area, AF );
    sprintf( _string, "%7.2f seconds to include emissivities.\n", CPUtime(time1) );
    fputs( _string, stderr );
    fputs( _string, _ulog );
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors including emissivities:",
        name, area, emit, base, AF, 0 );
    if( encl )
      NormAF( nSrf, emit, area, AF, 1.0e-7f, 30 );   /* fix rounding errors */
  }

  fprintf( _ulog, "\nFinal view factors:" );
  // Copy the values into single contigious array
  int ret_len = nSrf0*nSrf0;
  fprintf( stderr, "\nret_len: %d\n", ret_len);
  double *ret = malloc(sizeof(double)*ret_len);

  for(int n = 1; n <= nSrf; n++) {
    /* process AF values for row n */
    double Ainv = 1.0 / area[n];
    for(int m = 1; m <= nSrf; m++) {
      /* process column values */
      if(m < n){
        ret[(n-1)*nSrf+(m-1)] = (float)(AF[n][m] * Ainv);
      }else{
        ret[(n-1)*nSrf+(m-1)] = (float)(AF[m][n] * Ainv);
      }
    }
  }

  // zero-based array for areas
  float *areas0 = malloc(sizeof(float)*nSrf0);
  for (int n = 1; n <= nSrf; n++) {
    areas0[n-1] = area[n];
  }

  //zero-based array for emissivities
  float *emit0 = malloc(sizeof(float)*nSrf0);
  for (int n = 1; n <= nSrf; n++) {
    emit0[n-1] = emit[n];
  }

  VFResultsC res_struct;
  res_struct.n_surfs = nSrf0;
  res_struct.encl = encl;
  res_struct.area = areas0;
  res_struct.emit = emit0;
  res_struct.values = ret;

  if( vfCtrl.emittances )
    ReportAF( nSrf, encl, title, name, area, emit, base, AF, 0 );
  else
    ReportAF( nSrf, encl, title, name, area, vtmp, base, AF, 0 );

  time1 = CPUtime( 0.0 );

  SaveVF( outHandle, program, version, vfCtrl.outFormat, vfCtrl.enclosure,
          vfCtrl.emittances, nSrf, area, emit, AF, vtmp );
  sprintf( _string, "%7.2f seconds to write view factors.\n", CPUtime(time1) );
  fputs( _string, stderr );
  fputs( _string, _ulog );


#ifdef XXX
  fprintf( _ulog, "\nFinal list of surfaces:\n" );
  fprintf( _ulog, "   #        name     area  emit\n" );
  for( n=1; n<=nSrf; n++ )
    fprintf( _ulog, "%4d %12s %8.3f %5.3f\n", n, name[n], area[n], emit[n] );
#endif

FreeMemory:
  if( vfCtrl.row ){
    Fre_MC( AF, vfCtrl.row, vfCtrl.row, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
  }else{
#ifdef __TURBOC__
    Fre_MSR( (void **)AF, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#else
    Fre_MSC( (void **)AF, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#endif
  }
  Fre_V( srf, 1, vfCtrl.nAllSrf, sizeof(SRFDAT3D), __FILE__, __LINE__ );
  Fre_V( cmbn, 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  Fre_V( base, 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  Fre_V( vtmp, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_V( emit, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_V( area, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_MC( (void **)name, 1, nSrf0, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
  if( MemNet("\nAfter all calculations") )
    MemList();

  fprintf( _ulog, "\n%7.2f seconds for all calculations.\n", CPUtime(time0) );
  time(&bintime);
  curtime = localtime(&bintime);
  fprintf( _ulog, "Time:  %s", asctime(curtime) );
  fprintf( _ulog, "\n**********\n\n" );

// Closing _ulog will not work in library
#ifndef LIBONLY
  // Close log handle. TODO: move this to main.
  fclose( _ulog );
#endif

  fprintf( stderr, "\nDone!\n" );

  // return 0;

  return res_struct;

} /* end of processHandles() */
