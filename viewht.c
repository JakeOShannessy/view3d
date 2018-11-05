/*  ViewWHT  Program to compute heat transfer rates.  */

/* Usage: 
	ViewHT <view factor file> <surface temperature file>  <heat flux output file>
*/

#include <stdio.h>
#include <string.h> /* prototype: strcpy */
#include <stdlib.h> /* prototype: exit */
#include <ctype.h>  /* prototype: toupper */
#include <math.h>   /* prototype: sqrt */
#include "types.h" 
#include "heap.h"
#include "misc.h"

/* Temporary hack */
#define _MAX_PATH 256
#define _MAX_FNAME 256

/* type definitions */

/** structure for common surface data */
typedef struct surface{
  float area;   /* surface area */
  float emit;   /* surface emittance */
  float Tabs;   /* temperature [K] */
  float T4;     /* temperature^4 */
} SURFACE;

/* forward declarations */

void ReadVF( char *fileName, char *program, char *version,
             int *format, int *encl, int *didemit, int *nSrf,
             float *area, float *emit, double **AF, float **F, int init, int shape );
void ReadTK( char *fileName, int nSrf, float *TK );

void PgmInit(char *program);
void MemRem(char *msg );
FILE *FileOpen(char *prompt, char *fileName, char *mode, int flag);
float CPUtime(float t1);

/* global vars */

FILE *_ulog = NULL; /**< log file */
FILE *_uout; /**< output file */

/***  main.c  ****************************************************************/

int main( int argc, char **argv ){
  char program[16];     /* program name */
  char version[8];      /* program version */
  int format;          /* view factor file format; 0=text, 1=binary, ... */
  char inFile[_MAX_PATH + 1]=""; /* input file name */
  char outFile[_MAX_PATH + 1]=""; /* output file name */
  char tkFile[_MAX_PATH + 1]=""; /* temperature file name */
  float **VF=NULL;    /* square array of view factors */
  float *TK=NULL;     /* vector of surface temperatures [K] */
  float *T4=NULL;     /* vector of surface temperatures ^ 4 */
  float *area=NULL;   /* vector of surface areas [1:nSrf] */
  float *emit=NULL;   /* vector of surface emittances [1:nSrf] */
  float *sumq=NULL;   /* vector of surface heat fluxes [1:nSrf] */
  double sumQ;
  double SB_const = 5.6697e-8;
  double **atmp=NULL;
  int nSrf;    /* number of radiating surfaces */
  int encl;    /* true = surfaces should form enclosure, not used */
  int didemit; /* 1 = emittences included in view factors */
  float time0;
  int n, m;

  if( argc > 1 && argv[1][0] == '?' ){  /* arcg = 1 -> enter file names */
    
    fputs("\n"
    	"VIEWHT - radiant heat transfer calculation.\n\n"
		"ViewHT  VF_file  TK_file  output_file \n\n"
		"VF_file of view factors is created by VIEW3D or VIEW2D. \n"
		"TK_file sets the surface temperatures. \n"
		,stderr
	);
    exit(1);
  }

  /* open log file */
  PgmInit( argv[0] );
  MemRem( "Initial" );

  if( argc > 1 )strcpy( inFile, argv[1] );

  FileOpen( "Enter name of gray View Factors file", inFile, "r", 0 );
  fprintf( _ulog, "Data:  %s\n", inFile );

  if( argc > 2 )strcpy( tkFile, argv[2] );

  FileOpen( "Enter name of Temperatures file", tkFile, "r", 0 );
  fprintf( _ulog, "T[K]:  %s\n", tkFile );

  if( argc > 3 )strcpy( outFile, argv[3] );

  FileOpen( "Enter name of Heat Flux output file", outFile, "w", 0 );
  fprintf( _ulog, "Out:  %s\n", outFile );

  fprintf( stderr, "\n ViewHt running...\n\n" );

  /* read view factor file */
  time0 = CPUtime( 0.0 );
  ReadVF( inFile, program, version, &format, &encl, &didemit, &nSrf,
          area, emit, atmp, VF, 1, 1 );

  fprintf( _ulog, " total radiating surfaces: %3d\n", nSrf );
  fprintf( _ulog, "     enclosure designator: %3d\n", encl );
  fprintf( _ulog, "     emittance designator: %3d\n\n", didemit );

  VF = Alc_MC( 0, nSrf, 0, nSrf, sizeof(double), __FILE__, __LINE__ );
  TK = Alc_V( 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  T4 = Alc_V( 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  area = Alc_V( 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  emit = Alc_V( 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  sumq = Alc_V( 1, nSrf, sizeof(float), __FILE__, __LINE__ );

  ReadVF( inFile, program, version, &format, &encl, &didemit, &nSrf,
          area, emit, atmp, VF, 0, 1 );

  ReadTK( tkFile, nSrf, TK );   /* read temperatures file */

  for( n=1; n<=nSrf; n++ ){
    T4[n] = TK[n];
    T4[n] *= T4[n];
    T4[n] *= T4[n];
  }

  for( n=1; n<=nSrf; n++ ){    /* compute heat fluxes */
    double sum=0.0;
    for( m=1; m<=nSrf; m++ )
      sum += SB_const * VF[n][m] * (T4[m] - T4[n]);
    sumq[n] = (float)sum;
  }

  _uout = fopen( outFile, "w" );
  if( !didemit ){
    for( n=1; n<=nSrf; n++ ){
      emit[n] = 1.0;
	}
  }

  fprintf( _uout, "\nSrf #     area    emit    T [K]    q [W/m^2]      Q [W]\n" );
  for( sumQ=0.0,n=1; n<=nSrf; n++ ){
    fprintf( _uout, " %4d %8.3f %7.3f  %7.2f  %11.3e  %12.4e\n",
             n, area[n], emit[n], TK[n], sumq[n], sumq[n] * area[n] );
    sumQ += sumq[n] * area[n];
  }

  fprintf( _uout, "                                                 =========\n" );
  fprintf( _uout, "                                     balance:  %11.3e\n", sumQ );
  fclose( _uout );

  fprintf( _ulog, "\n%7.2f seconds for all operations.\n\n", CPUtime(time0) );

  Fre_V( sumq, 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  Fre_V( area, 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  Fre_V( emit, 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  Fre_V( T4, 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  Fre_V( TK, 1, nSrf, sizeof(float), __FILE__, __LINE__ );
  Fre_MC( VF, 0, nSrf, 0, nSrf, sizeof(double), __FILE__, __LINE__ );
  MemRem( "Final" );

  fprintf( stderr, "\nDone!\n" );

  return 0;

}  /* end main - ViewHT */

/***  ReadTK.c  **************************************************************/

/*  Read surface temperatures [K].  */

void ReadTK( char *fileName, int nSrf, float *TK ){
  int n, n1, n2;
  float Tabs;
  FILE *tfile;

  tfile = fopen( fileName, "r" );
  for(;;){
    fgets( _string, LINELEN, tfile );
    sscanf( _string, "%d %d %f", &n1, &n2, &Tabs );
    if( n1 < 1 ) break;
    if( n2 < 1 ) break;
    if( n1>nSrf )
      n1 = nSrf;
    if( n2>nSrf )
      n2 = nSrf;
    if( n2<n1 )
      { n = n1; n1 = n2; n2 = n; }

    for( n=n1; n<=n2; n++ )
      TK[n] = Tabs;
  }

}  /* end of ReadTK */


#include <time.h>   /* prototypes: time, localtime;
                       define: tm, time_t */

/***  PgmInit.c  *************************************************************/

/*  Functions at program initiation  */

void PgmInit( char *program ){
  char fileName[_MAX_PATH + 1]; /* name of file; _MAX_... in <stdlib.h> */
  char pgmDrv[_MAX_DRIVE + 1];  /* drive letter for current program */
  char pgmDir[_MAX_DIR + 1];    /* directory path for current program */
  char pgmName[_MAX_FNAME + 1]; /* file name for current program */
  struct tm *curtime; /* time structure */
  time_t bintime;     /* seconds since 00:00:00 GMT, 1/1/70 */

  /* open log file */
  PathSplit( program, pgmDrv, sizeof(pgmDrv), pgmDir, sizeof(pgmDir),
    pgmName, sizeof(pgmName), NULL, 0 );
  PathMerge( fileName, sizeof(fileName), pgmDrv, pgmDir, pgmName, ".log" );
  _ulog = fopen( fileName, "w" );
  if( _ulog == NULL )
    exit( 1 );

  fprintf( _ulog, "Program: %s\n", program );
  time(&bintime);
  curtime = localtime(&bintime);
  fprintf( _ulog, "Time:  %s", asctime(curtime) );

}  /* end PgmInit */

#ifdef __TURBOC__   /* using old TURBO C compiler */
# include <alloc.h> /* prototype: heapcheck, heapwalk */
#endif

/***  MemRem.c  **************************************************************/

/*  Report memory allocated and freed. Tubro C's coreleft() reports the amount
 *  of memory between the highest allocated block and the top of the heap.
 *  Freed lower blocks are not counted as available memory.
 *  heapwalk() shows the details.  */

void MemRem(char *msg)
  {
#if( __TURBOC__ >= 0x295 )
  {
  struct heapinfo hp;   /* heap information */
  unsigned long bytes = coreleft();
  fprintf( _ulog, "%s:\n", msg );
  fprintf( _ulog, "  Unallocated heap memory:  %ld bytes\n", bytes );

# if( MEMTEST > 1 )
  switch( heapcheck( ) )
    {
    case _HEAPEMPTY:
      fprintf( _ulog, "The heap is empty.\n" );
      break;
    case _HEAPOK:
      fprintf( _ulog, "The heap is O.K.\n" );
      break;
    case _HEAPCORRUPT:
      fprintf( _ulog, "The heap is corrupted.\n" );
      break;
    }  /* end switch */

  fprintf( _ulog, "Heap: loc, size, used?\n" );
  hp.ptr = NULL;
  while( heapwalk( &hp ) == _HEAPOK )
    {
    fprintf( _ulog, "[%p]%8lu %s\n",
             hp.ptr, hp.size, hp.in_use ? "used" : "free" );
    }
# endif
  }
#else
  MemNet( msg );  /* for non-TurboC code */
#endif

  }  /* end of MemRem */


/***  FileOpen.c  ************************************************************/

/*  Check/open fileName. If fileName is empty or or cannot be opened, 
 *  request a name from the user.  */

FILE *FileOpen( char *prompt, char *fileName, char *mode, int flag )
/*  prompt;   message to user.
 *  fileName; name of file (string _FILE_PATH long).
 *  mode;     access mode - see fopen() arguments -
 *              text files: "r" read, "w" write, "a" append;
 *            binary files: "rb" read, "wb" write, "ab" append.
 *  flag;     1 = return pointer to file; 0 = close pfile, return NULL */
  {
  FILE *pfile=NULL;
  char modr[4];
  int open=1;

#ifdef _DEBUG
  char c=mode[0];
  int err=0;
  if( !( c == 'r' || c == 'w' || c == 'a' ) )
    err = 1;
  c = mode[1];
  if( !( c == '\0' || c == 'b' ) )
    err = 1;
  if( ( c == 'b' && mode[2] != '\0' ) )
    err = 1;
  if( err )
    {
    error( 1, __FILE__, __LINE__, "For: ", prompt, "" );
    error( 3, __FILE__, __LINE__, "Invalid access mode: ", mode, "" );
    }
#endif

  if( mode[0] != 'r' )
    {
    strcpy( modr, mode );
    modr[0] = 'r';
    if( mode[0] == 'a' )
      fputs( "  Opening to append: ", stderr );
    else
      fputs( "  Opening to create: ", stderr );
    if( fileName[0] )
      fputs( fileName, stderr );
    fputc( '\n', stderr );
    }

  while( !pfile )
    {
    if( fileName[0] )   /* try to open file */
      {
      if( mode[0] != 'r' )  /* test write & append modes */
        {
        FILE *test = fopen( fileName, modr );
        if( mode[0] == 'w' )
          {
          if( test )
            open = NoYes( "File exists; replace" );
          }
        else
          {
          if( !test )
            open = NoYes( "File does not exist; create" );
          }
        if( test )
          fclose( test );
        }
      if( open )
        {
        pfile = fopen( fileName, mode );
        if( !pfile ) error( 2, __FILE__, __LINE__,
          "Failed to open: ", fileName, "\nTry again.\n", "" );
        }
      }
    if( !pfile )        /* ask for file name */
      GetStr( prompt, fileName, _MAX_PATH);
    }

  if( flag )
    return pfile;
  else
    fclose( pfile );
  return NULL;

  }  /*  end of FileOpen  */



#include <time.h>   /* prototype: clock;  define CLOCKS_PER_SEC */
#include <math.h>   /* prototype: fabs */

/***  CPUtime.c  *************************************************************/

/*  Determine elapsed time.  Call once to determine t1;
    call later to get elapsed time. */

float CPUtime( float t1 ){
  float t2;

  t2 = (float)clock() / (float)CLOCKS_PER_SEC;
  t2 = (float)(fabs(t2-t1));  /* clear -0.0 */

  return t2;

}  /* end CPUtime */
