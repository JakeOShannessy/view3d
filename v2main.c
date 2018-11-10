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
void CheckFileWritable(char *fileName);
void CheckFileReadable(char *fileName);

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

int main( int argc, char **argv ){
  char inFile[_MAX_PATH]=""; /* input file name */
  char outFile[_MAX_PATH]="";/* output file name */
  struct tm *curtime; /* time structure */
  time_t bintime;  /* seconds since 00:00:00 GMT, 1/1/70 */

	/* If argc is less than 2, (i.e. 1) we have no arguments */
	if(argc < 2){
		fprintf(stderr,"ERROR: missing command-line arguments\n");
		usage(argv[0]);
		exit(1);
	}
	/* If "?" is the first argument, print usage and quit */
	if (argc > 1 && argv[1][0] == '?') {
		usage(argv[0]);
		exit(0);
	}

	/* Load the input file argument */
	if( argc > 1 ){
    strcpy( inFile, argv[1] );
    CheckFileReadable(inFile);
  }

	/* Load the output file argument, if it exists */
	if( argc > 2 ) {
    strcpy( outFile, argv[2] );
    CheckFileWritable(outFile);
  } else {
    /* outFile = NULL; */
  }

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

  return processPaths2d(inFile, outFile);

}  /* end of main */

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
#endif

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