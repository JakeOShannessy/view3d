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
VFResultsC processPaths(char *inFile);

void ReportAF( const int nSrf, const int encl, const char *title, char **name,
  const float *area, const float *emit, const int *base, double **AF, int flag
);

/*----------------------------------------------------------------------------*/
int main( int argc, char **argv ){
  char program[]="View3D";   /* program name */
  char version[]=V3D_VERSION;      /* program version */
  char inFile[_MAX_PATH]=""; /* input file name */
  char outFile[_MAX_PATH]="";/* output file name */
  struct tm *curtime;  /* time structure */
  time_t bintime;      /* seconds since 00:00:00 GMT, 1/1/70 */

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
  /* TODO: Make logging to a particular file an option, log to stderr by */
  /* default. */
#ifdef ANSI
  /* _ulog = fopen( "View3D.log", "w" ); */
  _ulog = stderr;
#else
  /* PathSplit( argv[0], vdrive, sizeof(vdrive), vdir, sizeof(vdir), NULL, 0, NULL, 0 ); */
  /* PathMerge( fileName, sizeof(fileName), vdrive, vdir, "View3D", ".log" ); */
  /* _ulog = fopen( fileName, "w" ); */
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
    /* TODO: specify a non-interactive mode. */
  /* FindFile("Enter name of input (vertex/surface) data file", inFile, "r" ); */
  fprintf(_ulog, "Data file:  %s\n", inFile );

  if( argc > 2 ) {
    strcpy( outFile, argv[2] );
    CheckFileWritable(outFile);
  } else {
    /* outFile = NULL; */
  }
    /* TODO: if there is not output file set, output to stdout. */
  /* FindFile("Enter name of output (view factor) file", outFile, "w" ); */
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
  VFResultsC res = processPaths(inFile);
  fflush(stderr);
  fflush(stdout);

  FILE *outHandle;
  if(strlen(outFile) == 0 || outFile == NULL) {
    outHandle = stdout;
  } else {
    outHandle = fopen(outFile, "w");
  }
  SaveVFNew(outHandle, res);
  return 0;
}

/**
	Find user designated file.  REPLACE WITH FILEOPEN.TXT ???
	First character of type string must be 'r', 'w', or 'a'.
	@param msg    message to user
	@param name   file name (string long enough for any file name)
	@param type   type of file, see fopen()
*/
void FindFile( char *msg, char *fileName, char *type ){

  FILE  *pfile=NULL;

  while(!pfile){
    if(fileName[0]){
      /* try to open file */
      pfile = fopen( fileName, type );
      if( pfile == NULL )
        fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
    }
    if(!pfile){        /* ask for file name */
      fprintf( stderr, "%s: ", msg );
      fflush(stderr);
      scanf( "%s", fileName );
    }
  }
  fclose( pfile );
} /* End of FindFile() */


/* Currently does not open the file. */
void CheckFileWritable(char *fileName){

  FILE  *pfile=NULL;

  if(fileName!=NULL && strlen(fileName)>0){
    /* try to open file */
    pfile = fopen( fileName, "w");
    if( pfile == NULL )
      fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
  }
  fclose( pfile );
}

/* Currently does not open the file. */
void CheckFileReadable(char *fileName){

  FILE  *pfile=NULL;

  if(fileName!=NULL && strlen(fileName)>0){
    /* try to open file */
    pfile = fopen( fileName, "r");
    if( pfile == NULL )
      fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
  }
  fclose( pfile );
}