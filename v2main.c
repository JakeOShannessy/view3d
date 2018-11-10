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
#include <getopt.h>
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

int main( int argc, char **argv ){
  char inFile[_MAX_PATH]=""; /* input file name */
  char outFile[_MAX_PATH]="";/* output file name */
  struct tm *curtime; /* time structure */
  time_t bintime;  /* seconds since 00:00:00 GMT, 1/1/70 */


	char c;
	/* TODO: getopt (and unistd.h) is not available on Windows without MinGW */
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
