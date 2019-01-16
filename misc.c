/*subfile:  misc.c  **********************************************************/

/*    utility functions   */

#define V3D_BUILD
#include "misc.h"

#ifdef _DEBUG
#  define DEBUG 1
#else
#  define DEBUG 0
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> /* variable argument list macro definitions */
#include "types.h"  /* define unsigned char, short, etc. */
#include "readvs.h"

#define NMAX 4      /* maximum number of calls to xxxStr at one time */
#define NXTBUF 1    /* buffer size (if > 1023) */
#define ANSIC 1     /* 1 to use only ANSI C code for Path functions */

int _emode=1;   /* error message mode: 0=logFile, 1=DOS console, 2=Windows */

/* forward decsl */

     /* functions in misc.c */
void LogNote( char *file, int line, ... );

void PathCWD( char *path, int szp );

void PgmInit( char *program );
char *NextArg( const int argc, char **argv );

int HexCon( char *s, unsigned long *i );
char *HexStr( unsigned long j );

int DblCon( char *str, double *f );

int TimeCon( char *string, long *time );
char *TimeStr( long time );
int DateCon( char *s, long *day_of_year );
char *DateStr( int day_of_year );
int DatXCon( char *s, long *day_of_year );
char *DatXStr( int day_of_year );

int GetKey( void );
void Pause( void );

void NxtInit( char *file, int line );

short ReadI2( int flag );
long ReadI4( int flag );
unsigned char ReadU1( int flag );
unsigned short ReadU2( int flag );
unsigned long ReadX4( int flag );
long ReadHMS( int flag );
int ReadMD( int flag );
int ReadMDx( int flag );

int FileCopy( char *from_file, char *to_file );
void FileNameFix( char *filename );
FILE *FileOpen( char *prompt, char *fileName, char *mode, int flag );
FILE *NewFile( const char *filename, const char *mode );
FILE *LogFile( char *filename );

void Delay( float seconds );
int max0( int n1, int n2 );
int min0( int n1, int n2 );
void PtrXchg( void **p1, void **p2 );
char *strctr( char *s, int n );
char *strcpys( char *s, const int mx, ...  );


/***  errora.c  **************************************************************/

/*  Minimal error message - written to .LOG file.  */

static void errora( const char *head, char *message, char *source ){
  if( _ulog ){
    fputs( head, _ulog );
    fputs( source, _ulog );
    fputs( message, _ulog );
    fflush( _ulog );
  }

}  /*  end of errora  */

/***  errorb.c  **************************************************************/

/*  Standard error message for console version.  */

static void errorb( const char *head, char *message, char *source ){
  fputs( head, stdout );
  fputs( message, stdout );
  errora( head, message, source );

}  /*  end of errorb  */

/***  error.c  ***************************************************************/

/*  Standard error message routine - select errora, errorb or errorw.

	@NOTE global file pointer _ulog MUST be defined. @ENDNOTE

	@param severity  values 0 - 3 defined below
	@param file      file name: __FILE__
	@param line      line number: __LINE__
	@param ...       string variables (up to LINELEN characters total)
*/
int error( int severity, const char *file, const int line, ... ){
  char message[LINELEN]; /* merged message */
  char source[64];       /* source code info. */
  va_list argp;        /* variable argument list */
  char start[]=" ";      /* leading blank in message */
  char *msg, *s;
  static int count=0;   /* count of severe errors */
  static const char *head[4] = { "NOTE", "WARNING", "ERROR", "FATAL" };
  int n=1;

  if( severity >= 0 ){
    if( severity>3 ) severity = 3;
    if( severity==2 ) count += 1;
    msg = start;   /* merge message strings */
    s = message;
    va_start( argp, line );
    while( *msg && n<LINELEN )
      {
      while( *msg && n++<LINELEN )
        *s++ = *msg++;
      msg = (char *)va_arg( argp, char * );
      }
    *s++ = '\n';
    *s = '\0';
    va_end( argp );

    sprintf( source, "  (file %s, line %d)\n",
      sfname( file ), line );

#if( _MSC_VERXXX ) /* for Windoows programs */
    if( _emode == 2 )
      errorw( head[severity], message, source );
    else
#endif
    if( _emode == 1 )
      errorb( head[severity], message, source );
    else
      errora( head[severity], message, source );
    if( severity>2 )
      {
      if( _ulog )
        fclose( _ulog );  /* exit() closes files */
      exit( 1 );
      }
  }else if( severity < -1 ){
	/* clear error count */
    count = 0;
  }

  return( count );
}  /*  end error  */

/***  sfname.c  ***************************************************************/

/*  Return pointer to file name from the full path name.  */

const char *sfname(const char* fullpath){
	const char *name=fullpath, *c;  /* allow for name == fullpath */

	for( c=fullpath; *c; c++ ){  /* find last dir char before name */
		if( *c == '/' ) name = c+1;
		if( *c == '\\' ) name = c+1;
		if( *c == ':' ) name = c+1;
	}
	return name;
}  /* end sfname */


#if( __GNUC__ )
#include <unistd.h> /* prototypes: getcwd */
char _dirchr='/';
#elif( _MSC_VER )
#include <direct.h> /* prototypes: _getcdw */
char _dirchr='\\';
#elif( __TURBOC__ )
#include <dir.h>    /* prototypes: getcwd */
char _dirchr='\\';
#else
char _dirchr='\0';    /* this will force a failure */
#endif

/***  errorc.c  **************************************************************/

/*  Minimal error message - avoids recursive call to PathSplit.
 *  An error there will usually result a subsequent error.  */

static void errorc( int severity, char *message ){
  static const char *head[4] = { "NOTE", "WARNING", "ERROR", "FATAL" };

  if( severity>3 ) severity = 3;
  if( _emode < 2 )
    fprintf( stdout, "%s %s\n", head[severity], message );
  if( _ulog ){
    fprintf( _ulog, "%s %s\n", head[severity], message );
    fflush( _ulog );
  }
  if( severity > 2 )
    exit( 1 );

}  /*  end of errorc  */

/***  PathMerge.c  ***********************************************************/

/*  Merge full path from its component parts.  */

void PathMerge( char *fullpath, int szfp, char *drv, char *path, char *name, char *ext ){
  /* string indexing [ 0 | 1 | ... | sz-2 | sz-1 ] for size sz string */
  int n=0; /* index to fullpath string */
  char *c;  /* pointer to source string */

#if( DEBUG > 0 )
  if( !fullpath ) errorc( 3, "PathMerge: NULL fullpath" );
  if( !_dirchr ) errorc( 3, "PathMerge: NULL _dirchr" );
#endif

  if( !fullpath ) return;

  if( drv && *drv )
    for( c=drv; *c && n<szfp; c++ )
      fullpath[n++] = *c;

  if( path && *path )
    {
    for( c=path; *c && n<szfp; c++ )
      fullpath[n++] = *c;
    if( fullpath[n-1] != _dirchr && n<szfp )
      fullpath[n++] = _dirchr;  /* ensure path ends with _dirchr */
    }

  if( name && *name )
    for( c=name; *c && n<szfp; c++ )
      fullpath[n++] = *c;

  if( ext && *ext )
    {
    if( ext[0] != '.' && n<szfp )
      fullpath[n++] = '.';  /* ensure period before extension */
    for( c=ext; *c && n<szfp; c++ )
      fullpath[n++] = *c;
    }

  if( n < szfp )
    fullpath[n] = '\0';
  else
    {
    errorc( 2, "PathMerge: fullpath overrun" );
    fullpath[szfp-1] = '\0';
    }

#if( DEBUG > 0 )
  if( _ulog )
    {
    fprintf( _ulog, "Merge path: %s\n", fullpath );
/*  fprintf( _ulog, "   drv: %s\n", drv ); */
/*  fprintf( _ulog, "  path: %s\n", path ); */
/*  fprintf( _ulog, "  name: %s\n", name ); */
/*  fprintf( _ulog, "   ext: %s\n", ext ); */
    fflush( _ulog );
    }
#endif
  return;

}  /* end PathMerge */

/***  PathSplit.c  ***********************************************************/

/*  Split full path into its component parts.  Using:
 *    Visual C's _splitpath()  defined in <direct.h>
 *    Turbo C's fnsplit()  defined in <dir.h>
 *    - or - ANSI C code to do equivalent.
 *  In the call pass each part string and sizeof(string).
 *  Use NULL pointers for any parts not wanted.
 *  A null drv will leave the drive as part of the path.  */

void PathSplit( char *fullpath, char *drv, int szd, char *path, int szp
		,char *name, int szn, char *ext, int sze
){
  char *c, /* position in fullpath */
     *p; /* pointer to special charactor */
  int n;  /* character count */

#if( DEBUG > 0 )
  if( !fullpath ) errorc( 3, "PathSplit: NULL fullpath" );
#endif

  c = fullpath;
  if( drv )   /* fill directory string */
    {
    n = 0;
    if( fullpath[1] == ':' )
      {
      if( szd < 3 )
        errorc( 2, "pathsplit: file drive overrun" );
      else
        {
        drv[n++] = *c++;  /* copy drive characters */
        drv[n++] = *c++;
        }
      }
    drv[n] =  '\0';  /* terminate drive string */
    }

  p = strrchr( c, _dirchr );
  if( p )  /* p = last directory charactor in fullpath */
    {
    if( path )
      {
      n = 0;
      while( c<=p && n<szp )
        path[n++] = *c++;
      if( n == szp )
        { errorc( 2, "pathsplit: file path overrun" ); n--; }
      path[n] = '\0';  /* terminate path string */
      }
    c = p + 1;  /* c = start of name in fullpath */
    }

  p = strrchr( c, '.' );
  if( name )
    {
    n = 0;
    if( p )  /* p = last period in fullpath */
      while( c<p && n<szn )
        name[n++] = *c++;
    else     /* no period in/after name */
      while( *c && n<szn )
        name[n++] = *c++;
    if( n == szn )
      { errorc( 2, "pathsplit: file name overrun" ); n--; }
    name[n] = '\0';  /* terminate name string */
    }

  if( ext )
    {
    n = 0;
    if( p )  /* p = last period in fullpath */
      {
      for( c=p; *c && n<sze; c++ )
        ext[n++] = *c;
      if( n == sze )
        { errorc( 2, "pathsplit: file extension overrun" ); n--; }
      }
    ext[n] = '\0';  /* terminate extension string */
    }

#if( DEBUG > 0 )
  if( _ulog )
    {
    fprintf( _ulog, "Split path: %s\n", fullpath );
    fprintf( _ulog, "   drv: %s\n", drv );
    fprintf( _ulog, "  path: %s\n", path );
    fprintf( _ulog, "  name: %s\n", name );
    fprintf( _ulog, "   ext: %s\n", ext );
    }
#endif

}  /* end PathSplit */

/***  PathCWD.c  *************************************************************/

/*  Determine component parts of Current Working Directory.
 *    Visual C's _getcwd()  defined in <direct.h>
 *    Turbo C's getcwd()  defined in <dir.h>
 *    These functions do not produce the trailing directory character.
 *    This function may be unique to Windows/DOS.
 *    The drive characters are not separated from the path string.
 *    This will not matter in a subsequent PathMerge( ).
 *  The getcwd functions allocate a vector at path.  */

void PathCWD( char *path, int szp ){
#if( DEBUG > 0 )
  if( !path ) errorc( 3, "PathCWD: NULL path" );
#endif
#if( ANSIC )
  if( szp < 2 ) errorc( 3, "PathCWD: szp < 2" );
  path[0] = '.';
  path[1] = '\0';
#elif( _MSC_VER )
  if( !_getcwd( path, szp ) )
    path[0] = '\0';
#elif( __TURBOC__ || __GNUC__ )
  if( !getcwd( path, szp ) )
    path[0] = '\0';
#else
  errorc( 3, "PathCWD: Compiler not defined" );
#endif

}  /* end PathCWD */

extern int _echo;      /* if true, echo NXT input file */

/*  Open file_name and return the handle.  */

FILE *NxtOpenHndl(const char *file_name, const char *file, int line ){
  /* file;  source code file name: __FILE__
  * line;  line number: __LINE__ */

  FILE *handle = fopen( file_name, "r" );  /* = NULL if no file */
  if( !handle ){
    error( 2, file, line, "Could not open file: ", file_name, "" );
  }

  return handle;

}  /* end NxtOpen */



/*  Close handle.  */

void NxtCloseHndl(FILE *handle){
    if( fclose(handle) )
      error( 2, __FILE__, __LINE__, "Problem while closing handle", "" );
}

#include <float.h>  /* define: FLT_MAX, FLT_MIN */



/*  Use ANSI functions to convert a \0 terminated string to a double value.
 *  Return 1 if string is invalid, 0 if valid.
 *  Global errno will indicate overflow.
 *  Used in place of atof() because of error processing.  */

int DblCon( char *str, double *f )
  {
  char *endptr;
  double value;
  int eflag=0;
#if( !__GNUC__)
  extern int errno;
  errno = 0;
#endif

  endptr = str;
  value = strtod( str, &endptr );
  if( *endptr != '\0' ) eflag = 1;
#if( !__GNUC__)
  if( errno ) eflag = 1;
#endif

  if( eflag )
    *f = 0.0;
  else
    *f = value;
  return eflag;

  }  /* end of DblCon */

/***  FltCon.c  **************************************************************/

/*  Use ANSI functions to convert a \0 terminated string to a float value.
 *  Return 1 if string is invalid, 0 if valid.
 *  Floats are in the range -3.4e38 to +3.4e38 (FLT_MIN to FLT_MAX).
 *  Used in place of atof() because of error processing.  */

int FltCon( char *str, float *f )
  {
  double value;    /* compute result in high precision, then chop */
  int eflag=0;

  if( DblCon( str, &value) ) eflag = 1;
  if( value > FLT_MAX ) eflag = 1;
  if( value < -FLT_MAX ) eflag = 1;

  if( eflag )
    *f = 0.0;
  else
    *f = (float)value;
  return eflag;

  }  /* end of FltCon */

/***  FltStr.c  **************************************************************/

/*  Convert a float number to a string of characters;
 *  n significant digits; uses ANSI sprintf().
 *  Static string required to retain results in calling function.
 *  NMAX allows up to NMAX calls to IntStr() in one statement.
 *  Replaces nonstandard GCVT function.  */

char *FltStr( double f, int n )
  {
  static char string[NMAX][32];  /* string long enough for any practical value */
  char format[8];
  static int index=0;

  if( ++index == NMAX )
    index = 0;

  sprintf( format, "%%%dg", n );
  sprintf( string[index], format, f );

  return ( string[index] );

  }  /* end of FltStr */

/***  ReadR8.c  **************************************************************/

double ReadR8( FILE *inHandle, int flag )
  {
  double value;
  char string[LINELEN + 1]; /* buffer for a character string */

  NxtWord( inHandle, string, flag, sizeof(string) );
  if( DblCon( string, &value ) )
    error( 2, __FILE__, __LINE__, string, " is not a (valid) number", "" );
  return value;

  }  /* end ReadR8 */

/***  ReadR4.c  **************************************************************/

/*  Convert next word from file inHandle to float real. */

float ReadR4( FILE *inHandle, int flag )
  {
  double value;
  char string[LINELEN + 1]; /* buffer for a character string */

  NxtWord( inHandle, string, flag, sizeof(string) );
  if( DblCon( string, &value ) || value > FLT_MAX || value < -FLT_MAX )
    error( 2, __FILE__, __LINE__, "Bad float value: ", string, "" );

  return (float)value;

  }  /* end ReadR4 */

#include <limits.h> /* define: SHRT_MAX, SHRT_MIN */

/***  LongCon.c  *************************************************************/

/*  Use ANSI functions to convert a \0 terminated string to a long integer.
 *  Return 1 if string is invalid, 0 if valid.
 *  Global errno will indicate overflow.
 *  Used in place of atol() because of error processing.  */

int LongCon( char *str, long *i )
  {
  char *endptr;
  long value;
  int eflag=0;
#if( !__GNUC__)
  extern int errno;
  errno = 0;
#endif

  endptr = str;
  value = strtol( str, &endptr, 0 );
  if( *endptr ) eflag = 1;
#if( !__GNUC__)
  if( errno ) eflag = 1;
#endif

  if( eflag )
    *i = 0L;
  else
    *i = value;
  return eflag;

  }  /* end of LongCon */

/***  IntCon.c  **************************************************************/

/*  Use ANSI functions to convert a \0 terminated string to a short integer.
 *  Return 1 if string is invalid, 0 if valid.
 *  Short integers are in the range -32767 to +32767 (INT_MIN to INT_MAX).
 *  Used in place of atoi() because of error processing.  */

int IntCon( char *str, int *i )
  {
  long value;    /* compute result in long integer, then chop */
  int eflag=0;

  if( LongCon( str, &value ) ) eflag = 1;
  if( value > SHRT_MAX ) eflag = 1;
  if( value < SHRT_MIN ) eflag = 1;

  if( eflag )
    *i = 0;
  else
    *i = (int)value;
  return eflag;

  }  /* end of IntCon */

/***  IntStr.c  **************************************************************/

/*  Convert an integer to a string of characters.
 *  Can handle short or long integers by conversion to long.
 *  Static variables required to retain results in calling function.
 *  NMAX allows up to NMAX calls to IntStr() in one statement.
 *  Replaces nonstandard ITOA & LTOA functions for radix 10.  */

char *IntStr( long i )
  {
  static char string[NMAX][12];  /* strings long enough for 32-bit integers */
  static int index=0;

  if( ++index == NMAX )
    index = 0;

  sprintf( string[index], "%ld", i );

  return string[index];

  }  /* end of IntStr */

/***  ReadIX.c  **************************************************************/

/*  Convert next word from file inHandle to int integer. */

int ReadIX( FILE *inHandle, int flag )
  {
  long value;
  char string[LINELEN + 1]; /* buffer for a character string */
  
  NxtWord( inHandle, string, flag, sizeof(string) );
  if( LongCon( string, &value ) ||
      value > INT_MAX || value < INT_MIN )  /* max/min depends on compiler */
    error( 2, __FILE__, __LINE__, "Bad integer: ", string, "" );

  return (int)value;

  }  /* end ReadIX */

#include <time.h>   /* prototype: clock;  define CLOCKS_PER_SEC */
#include <math.h>   /* prototype: fabs */

/***  CPUtime.c  *************************************************************/

/*  Determine elapsed time.  Call once to determine t1;
    call later to get elapsed time. */

float CPUtime(float t1){
  float t2;
  t2 = (float)clock() / (float)CLOCKS_PER_SEC;
  t2 = (float)(fabs(t2-t1));  /* clear -0.0 */
  return t2;
}  /* end CPUtime */

#include <ctype.h> /* prototype: toupper */

/***  streql.c  **************************************************************/

/*  Test for equality of two strings; return 1 if equal, 0 if not.  */

int streql( char *s1, char *s2 )
  {
  for( ; *s1 && *s2 && *s1 == *s2; ++s1, ++s2 )
    ;

  if( *s1 == *s2 )  /* both must equal '\0' */
    return 1;
  else
    return 0;

  }  /* end of streql */

/***  streqli.c  *************************************************************/

/*  Test for equality of two strings; ignore differences in case.
 *  Return 1 if equal, 0 if not.  */

int streqli( char *s1, char *s2 )
  {
  for( ; *s1 && *s2 && (toupper(*s1) == toupper(*s2)); ++s1, ++s2 )
    ;

  if( *s1 == *s2 )  /* both must equal '\0' */
    return 1;
  else
    return 0;

  }  /* end of streqli */


/***  GetStr.c  **************************************************************/

/*  Get a string from the keyboard (user).  */

char *GetStr( char *prompt, char *str, int maxchr )
  {
  if( prompt[0] )
    {
    fputs( prompt, stderr );
    fputs( ": ", stderr );
    }
  if( !fgets(str, maxchr, stdin) )
    error( 3, __FILE__, __LINE__, "Failed to process input", "" );

  /* replace newline at end with a null */
  if(str[strlen(str)-1] == '\n'){
	str[strlen(str)-1] = '\0';
  }

  return str;

  }  /* end GetStr */

/***  NoYes.c  ***************************************************************/

/*  Obtain n/y response to query.  NO returns 0; YES returns 1.  */

int NoYes( char *question )
  {
  int i = -1;

  while( i == -1 )
    {
    int response;
         /* print input prompt */
    fputs( question, stderr );
    fputs( "? (y/n)  ", stderr );

         /* get user response */
    response = GetKey();
#if( _MSC_VER || __TURBOC__ || __WATCOMC__ )
    fputc( response, stderr );
    fputc( '\n', stderr );
#endif

         /* process user response */
    switch( response )
      {
      case 'y':
      case 'Y':
      case '1':
        i = 1;
        break;
      case 'n':
      case 'N':
      case '0':
        i = 0;
        break;
      default:
        fputs( " Valid responses are:  y or Y for yes, n or N for no.\n", stderr);
      }
    }  /* end while loop */

  return i;

  }  /*  end NoYes  */

#if( _MSC_VER || __TURBOC__ || __WATCOMC__ )
#include <conio.h>  /* prototype: _getch(MSC), getch(TC) */
#endif

#if( _MSC_VER )   /* VISUAL C version */
/***  GetKey.c  **************************************************************/

int GetKey( void )
  {
  int c = _getch();       /* <conio.h> read keystroke */
  if( !c )               /* function & arrow characters above 127 */
    c = 128 + _getch();

  return c;

  }  /* end GetKey */

#elif( __TURBOC__ || __WATCOMC__ )
/***  GetKey.c  **************************************************************/

int GetKey( void )
  {
  int c = getch();        /* <conio.h> read keystroke */
  if( !c )               /* function & arrow characters above 127 */
    c = 128 + getch();

  return c;

  }  /* end GetKey */

#else
/***  GetKey.c  **************************************************************/

int GetKey( void )
  {
  int c = getchar();  /* ANSI, requies ENTER key also */

  return c;

  }  /* end GetKey */
#endif
