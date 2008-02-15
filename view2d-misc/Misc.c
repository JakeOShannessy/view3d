/*subfile:  misc.c  **********************************************************/

/*    utility functions   */

#ifdef _DEBUG
#  define DEBUG 1
#else
#  define DEBUG 0
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> // variable argument list macro definitions
#include "types.h"  // define U1, I2, etc.
#include "prtyp.h"  // miscellaneous function prototypes

extern FILE *_ulog;  // LOG output file
extern I1 _string[LINELEN];  // buffer for ReadXX(); helps debugging

#define NMAX 4      // maximum number of calls to xxxStr at one time
#define NXTBUF 1    // buffer size (if > 1023)
#define ANSIC 1     // 1 to use only ANSI C code for Path functions

IX _emode=1;   /* error message mode: 0=logFile, 1=DOS console, 2=Windows */

/***  errora.c  **************************************************************/

/*  Minimal error message - written to .LOG file.  */

void errora( const I1 *head, I1 *message, I1 *source )
  {
  if( _ulog )
    {
    fputs( head, _ulog );
    fputs( source, _ulog );
    fputs( message, _ulog );
    fflush( _ulog );
    }

  }  /*  end of errora  */

/***  errorb.c  **************************************************************/

/*  Standard error message for console version.  */

void errorb( const I1 *head, I1 *message, I1 *source )
  {
  fputs( head, stdout );
  fputs( message, stdout );
  errora( head, message, source );

  }  /*  end of errorb  */

/***  error.c  ***************************************************************/

/*  Standard error message routine - select errora, errorb or errorw.
 *  _ulog MUST be defined.  */

IX error( IX severity, I1 *file, IX line, ... )
/* severity;  values 0 - 3 defined below
 * file;      file name: __FILE__
 * line;      line number: __LINE__
 * ...;       string variables (up to LINELEN characters total) */
  {
  I1 message[LINELEN]; /* merged message */
  I1 source[64];       /* source code info. */
  va_list argp;        /* variable argument list */
  I1 start[]=" ";      /* leading blank in message */
  I1 *msg, *s;
  static IX count=0;   /* count of severe errors */
  static const I1 *head[4] = { "NOTE", "WARNING", "ERROR", "FATAL" };
  IX n=1;

  if( severity >= 0 )
    {
    if( severity>3 ) severity = 3;
    if( severity==2 ) count += 1;
    msg = start;   /* merge message strings */
    s = message;
    va_start( argp, line );
    while( *msg && n<LINELEN )
      {
      while( *msg && n++<LINELEN )
        *s++ = *msg++;
      msg = (I1 *)va_arg( argp, I1 * );
      }
    *s++ = '\n';
    *s = '\0';
    va_end( argp );

    sprintf( source, "  (file %s, line %d)\n",
      sfname( file ), line );

#if( _MSC_VERXXX ) // for Windoows programs
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
        fclose( _ulog );  // exit() closes files
      exit( 1 );
      }
    }
  else if( severity < -1 )   /* clear error count */
    count = 0;

  return( count );

  }  /*  end error  */

/***  sfname.c  ***************************************************************/

/*  Return pointer to file name from the full path name.  */

I1 *sfname( I1* fullpath )
  {
  I1 *name=fullpath, *c;  // allow for name == fullpath

  for( c=fullpath; *c; c++ )  // find last dir char before name
    {
    if( *c == '/' ) name = c+1;
    if( *c == '\\' ) name = c+1;
    if( *c == ':' ) name = c+1;
    }

  return name;

  }  /* end sfname */

#if( __GNUC__ )
#include <unistd.h> // prototypes: getcwd
I1 _dirchr='/';
#elif( _MSC_VER )
#include <direct.h> // prototypes: _getcdw
I1 _dirchr='\\';
#elif( __TURBOC__ )
#include <dir.h>    // prototypes: getcwd
I1 _dirchr='\\';
#else
I1 _dirchr='\0';    // this will force a failure
#endif

/***  errorc.c  **************************************************************/

/*  Minimal error message - avoids recursive call to PathSplit.
 *  An error there will usually result a subsequent error.  */

void errorc( IX severity, I1 *message )
  {
  static const I1 *head[4] = { "NOTE", "WARNING", "ERROR", "FATAL" };

  if( severity>3 ) severity = 3;
  if( _emode < 2 )
    fprintf( stdout, "%s %s\n", head[severity], message );
  if( _ulog )
    {
    fprintf( _ulog, "%s %s\n", head[severity], message );
    fflush( _ulog );
    }
  if( severity > 2 )
    exit( 1 );

  }  /*  end of errorc  */

/***  PathMerge.c  ***********************************************************/

/*  Merge full path from its component parts.  */

void PathMerge( I1 *fullpath, IX szfp, I1 *drv, I1 *path, I1 *name, I1 *ext )
  {
  // string indexing [ 0 | 1 | ... | sz-2 | sz-1 ] for size sz string
  IX n=0; // index to fullpath string
  I1 *c;  // pointer to source string

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
      fullpath[n++] = _dirchr;  // ensure path ends with _dirchr
    }

  if( name && *name )
    for( c=name; *c && n<szfp; c++ )
      fullpath[n++] = *c;

  if( ext && *ext )
    {
    if( ext[0] != '.' && n<szfp )
      fullpath[n++] = '.';  // ensure period before extension
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
//  fprintf( _ulog, "   drv: %s\n", drv );
//  fprintf( _ulog, "  path: %s\n", path );
//  fprintf( _ulog, "  name: %s\n", name );
//  fprintf( _ulog, "   ext: %s\n", ext );
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

void PathSplit( I1 *fullpath, I1 *drv, IX szd, I1 *path, IX szp,
                I1 *name, IX szn, I1 *ext, IX sze )
  {
  I1 *c, // position in fullpath
     *p; // pointer to special charactor
  IX n;  // character count

#if( DEBUG > 0 )
  if( !fullpath ) errorc( 3, "PathSplit: NULL fullpath" );
#endif

  c = fullpath;
  if( drv )   // fill directory string
    {
    n = 0;
    if( fullpath[1] == ':' )
      {
      if( szd < 3 )
        errorc( 2, "pathsplit: file drive overrun" );
      else
        {
        drv[n++] = *c++;  // copy drive characters
        drv[n++] = *c++;
        }
      }
    drv[n] =  '\0';  // terminate drive string
    }

  p = strrchr( c, _dirchr );
  if( p )  // p = last directory charactor in fullpath
    {
    if( path )
      {
      n = 0;
      while( c<=p && n<szp )
        path[n++] = *c++;
      if( n == szp )
        { errorc( 2, "pathsplit: file path overrun" ); n--; }
      path[n] = '\0';  // terminate path string
      }
    c = p + 1;  // c = start of name in fullpath
    }
  
  p = strrchr( c, '.' );
  if( name )
    {
    n = 0;
    if( p )  // p = last period in fullpath
      while( c<p && n<szn )
        name[n++] = *c++;
    else     // no period in/after name
      while( *c && n<szn )
        name[n++] = *c++;
    if( n == szn )
      { errorc( 2, "pathsplit: file name overrun" ); n--; }
    name[n] = '\0';  // terminate name string
    }

  if( ext )
    {
    n = 0;
    if( p )  // p = last period in fullpath
      {
      for( c=p; *c && n<sze; c++ )
        ext[n++] = *c;
      if( n == sze )
        { errorc( 2, "pathsplit: file extension overrun" ); n--; }
      }
    ext[n] = '\0';  // terminate extension string
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

void PathCWD( I1 *path, IX szp )
  {
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

extern FILE *_unxt;   /* NXT input file */
extern IX _echo;      /* if true, echo NXT input file */
I1 *_nxtbuf;   /* large buffer for NXT input file */

/***  NxtOpen.c  *************************************************************/

/*  Open file_name as UNXT file.  */

IX NxtOpen( I1 *file_name, I1 *file, IX line )
/* file;  source code file name: __FILE__
 * line;  line number: __LINE__ */
  {
  IX result=0;

  if( _unxt ) error( 3, file, line, "_UNXT already open", "" );
  _unxt = fopen( file_name, "r" );  /* = NULL if no file */
  if( !_unxt )
    {
    error( 2, file, line, "Could not open file: ", file_name, "" );
    result = 1;
    }

  return result;

  }  /* end NxtOpen */

/***  NxtClose.c  ************************************************************/

/*  Close _unxt.  */

void NxtClose( void )
  {
  if( _unxt )
    {
    if( fclose( _unxt ) )
      error( 2, __FILE__, __LINE__, "Problem while closing _UNXT", "" );
    _unxt = NULL;
    }

  }  /* end NxtClose */

/***  NxtLine.c  *************************************************************/

/*  Get characters to end of line (\n --> \0); used by NxtWord().  */

I1 *NxtLine( I1 *str, IX maxlen )
  {
  IX c=0;       /* character read from _unxt */
  IX i=0;       /* current position in str */

  while( c!='\n' )
    {
    c = getc( _unxt );
    if( c==EOF ) return NULL;
    if( _echo ) putc( c, _ulog );
    if( maxlen < 1 ) continue;   // do not fill buffer
    if( c == '\r' ) continue;    // 2007/10/07 Linux EOL = \n\r
    str[i++] = (I1)c;
    if( i == maxlen )
      {
      str[i-1] = '\0';
      error( 3, __FILE__, __LINE__, "Buffer overflow: ", str, "" );
      }
    }
  if( i )
    str[i-1] = '\0';
  else
    str[0] = '\0';

  return str;

  }  /* end NxtLine */

/***  NxtWord.c  *************************************************************/

/*  Get the next word from file _unxt.  Return NULL at end-of-file.
 *  Assuming standard word separators (blank, comma, tab),
 *  comment identifiers (! to end-of-line), and
 *  end of data (* or end-of-file). */
/*  Major change October 2007:  ContamX uses NxtWord to read multiple files
 *  which are open simultaneously. The old static variable "newl" may cause
 *  an error. It has been replaced by using ungetc() to note end-of-word (EOW)
 *  which may also be end-of-line (EOL) character.
 *  Initialization with flag = -1 in now invalid - debug checked. */

I1 *NxtWord( I1 *str, IX flag, IX maxlen )
/* str;   buffer where word is stored; return pointer.
 * flag:  0:  get next word from current position in _unxt;
          1:  get 1st word from next line of _unxt;
          2:  get remainder of current line from _unxt (\n --> \0);
          3:  get next data line from _unxt (\n --> \0);
          4:  get next line (even if comment) (\n --> \0).
 * maxlen: length of buffer to test for overflow. */
  {
  IX c;         // character read from _unxt
  IX i=0;       // current position in str
  IX done=0;    // true when start of word is found or word is complete

#ifdef _DEBUG
  if( !_unxt )
    error( 3, __FILE__, __LINE__, "_UNXT not open", "" );
  if( maxlen < 16 )
    error( 3, __FILE__, __LINE__, "Invalid maxlen: ", IntStr(maxlen), "" );
#endif
  c = getc( _unxt );
  if( flag > 0 )
    {
    if( c != '\n' )  // last call did not end at EOL; ready to read next char.
      {                // would miss first char if reading first line of file.
      if( flag == 2 )
        {
        if( ftell( _unxt) == 1 ) // 2008/01/16
          ungetc( c, _unxt );  // restore first char of first line
        NxtLine( str, maxlen );  // read to EOL filling buffer
        }
      else
        NxtLine( str, 0 );       // skip to EOL; fix size 2008/01/16
        // if flag = 1; continue to read first word on next line
      }
    if( flag > 1 )
      {
        // if flag = 2; return (partial?) line just read
      if( flag > 2 )
        {
        // if flag > 2; return all of next line (must fit in buffer)
        NxtLine( str, maxlen );
        if( flag == 3 )  // skip comment lines
          while( str[0] == '!' )
            NxtLine( str, maxlen );
#ifdef _DEBUG
        if( flag > 4 )
          error( 3, __FILE__, __LINE__,
            "Invalid flag: ", IntStr(flag), "" );
#endif
        }
      ungetc( '\n', _unxt );  // restore EOL character for next call
      return str;
      }
    }
  else  // flag == 0
    {
#ifdef _DEBUG
    if( flag < 0 )
      error( 3, __FILE__, __LINE__,
        "Invalid flag: ", IntStr(flag), "" );
#endif
    if( c == ' ' || c == ',' || c == '\n' || c == '\t' )
      ; // skip EOW char saved at last call
    else
      ungetc( c, _unxt );  // restore first char of first line
    }

  while( !done )   // search for start of next word
    {
    c = getc( _unxt );
    if( c==EOF ) return NULL;
    if( _echo ) putc( c, _ulog );
    switch( c )
      {
      case ' ':          // skip word separators
      case ',':
      case '\n':
      case '\r':
      case '\t':
      case '\0':
        break;
      case '!':          // begin comment; skip to EOL
        NxtLine( str, 0 );
        break;
      case '*':          // end-of-file indicator
        NxtClose();
        return NULL;
      default:           // first character of word found
        str[i++] = (I1)c;
        done = 1;
        break;
      }
    }  // end start-of-word search

  done = 0;
  while( !done )   // search for end-of-word (EOW)
    {
    c = getc( _unxt );
    if( c==EOF ) return NULL;
    if( _echo ) putc( c, _ulog );
    switch( c )
      {
      case '\n':   // EOW characters
      case ' ':
      case ',':
      case '\t':
        str[i] = '\0';
        done = 1;
        break;
      case '\r':   // 2004/01/14 here for Linux: EOL = \n\r
      case '\0':
        break;
      default:     // accumulate word in buffer
        str[i++] = (I1)c;
        if( i == maxlen )  // with overflow test
          {
          str[i-1] = '\0';
          error( 3, __FILE__, __LINE__, "Buffer overflow: ", str, "" );
          }
        break;
      }
    }  // end EOW search
  ungetc( c, _unxt ); // save EOW character for next call

  return str;

  }  /* end NxtWord */

#include <float.h>  /* define: FLT_MAX, FLT_MIN */

/***  DblCon.c  **************************************************************/

/*  Use ANSI functions to convert a \0 terminated string to a double value.
 *  Return 1 if string is invalid, 0 if valid.
 *  Global errno will indicate overflow.
 *  Used in place of atof() because of error processing.  */

IX DblCon( I1 *str, R8 *f )
  {
  I1 *endptr;
  R8 value;
  IX eflag=0;
#if( !__GNUC__)
  extern IX errno;
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

IX FltCon( I1 *str, R4 *f )
  {
  R8 value;    /* compute result in high precision, then chop */
  IX eflag=0;

  if( DblCon( str, &value) ) eflag = 1;
  if( value > FLT_MAX ) eflag = 1;
  if( value < -FLT_MAX ) eflag = 1;

  if( eflag )
    *f = 0.0;
  else
    *f = (R4)value;
  return eflag;
  
  }  /* end of FltCon */

/***  FltStr.c  **************************************************************/

/*  Convert a float number to a string of characters;
 *  n significant digits; uses ANSI sprintf().
 *  Static string required to retain results in calling function.
 *  NMAX allows up to NMAX calls to IntStr() in one statement. 
 *  Replaces nonstandard GCVT function.  */

I1 *FltStr( R8 f, IX n )
  {
  static I1 string[NMAX][32];  /* string long enough for any practical value */
  I1 format[8];
  static IX index=0;

  if( ++index == NMAX )
    index = 0;

  sprintf( format, "%%%dg", n );
  sprintf( string[index], format, f );

  return ( string[index] );

  }  /* end of FltStr */

/***  ReadR4.c  **************************************************************/

/*  Convert next word from file _unxt to R4 real. */

R4 ReadR4( IX flag )
  {
  R8 value;

  NxtWord( _string, flag, sizeof(_string) );
  if( DblCon( _string, &value ) || value > FLT_MAX || value < -FLT_MAX )
    error( 2, __FILE__, __LINE__, "Bad float value: ", _string, "" );

  return (R4)value;

  }  /* end ReadR4 */

#include <limits.h> /* define: SHRT_MAX, SHRT_MIN */

/***  LongCon.c  *************************************************************/

/*  Use ANSI functions to convert a \0 terminated string to a long integer.
 *  Return 1 if string is invalid, 0 if valid.
 *  Global errno will indicate overflow.
 *  Used in place of atol() because of error processing.  */

IX LongCon( I1 *str, I4 *i )
  {
  I1 *endptr;
  I4 value;
  IX eflag=0;
#if( !__GNUC__)
  extern IX errno;
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

IX IntCon( I1 *str, IX *i )
  {
  I4 value;    /* compute result in long integer, then chop */
  IX eflag=0;

  if( LongCon( str, &value ) ) eflag = 1;
  if( value > SHRT_MAX ) eflag = 1;
  if( value < SHRT_MIN ) eflag = 1;

  if( eflag )
    *i = 0;
  else
    *i = (IX)value;
  return eflag;
  
  }  /* end of IntCon */

/***  IntStr.c  **************************************************************/

/*  Convert an integer to a string of characters.
 *  Can handle short or long integers by conversion to long.
 *  Static variables required to retain results in calling function.
 *  NMAX allows up to NMAX calls to IntStr() in one statement. 
 *  Replaces nonstandard ITOA & LTOA functions for radix 10.  */

I1 *IntStr( I4 i )
  {
  static I1 string[NMAX][12];  // strings long enough for 32-bit integers
  static IX index=0;

  if( ++index == NMAX )
    index = 0;

  sprintf( string[index], "%ld", i );

  return string[index];

  }  /* end of IntStr */

/***  ReadIX.c  **************************************************************/

/*  Convert next word from file _unxt to IX integer. */

IX ReadIX( IX flag )
  {
  I4 value;

  NxtWord( _string, flag, sizeof(_string) );
  if( LongCon( _string, &value ) ||
      value > INT_MAX || value < INT_MIN )  // max/min depends on compiler
    error( 2, __FILE__, __LINE__, "Bad integer: ", _string, "" );

  return (IX)value;

  }  /* end ReadIX */

#include <time.h>   /* prototype: clock;  define CLOCKS_PER_SEC */
#include <math.h>   /* prototype: fabs */

/***  CPUtime.c  *************************************************************/

/*  Determine elapsed time.  Call once to determine t1;
    call later to get elapsed time. */

R4 CPUtime( R4 t1 )
  {
  R4 t2;

  t2 = (R4)clock() / (R4)CLOCKS_PER_SEC;
  t2 = (R4)(fabs(t2-t1));  // clear -0.0

  return t2;

  }  /* end CPUtime */

#include <ctype.h> /* prototype: toupper */

/***  streql.c  **************************************************************/

/*  Test for equality of two strings; return 1 if equal, 0 if not.  */

IX streql( I1 *s1, I1 *s2 )
  {
  for( ; *s1 && *s2 && *s1 == *s2; ++s1, ++s2 )
    ;

  if( *s1 == *s2 )  // both must equal '\0'
    return 1;
  else
    return 0;

  }  /* end of streql */

/***  streqli.c  *************************************************************/

/*  Test for equality of two strings; ignore differences in case.
 *  Return 1 if equal, 0 if not.  */

IX streqli( I1 *s1, I1 *s2 )
  {
  for( ; *s1 && *s2 && (toupper(*s1) == toupper(*s2)); ++s1, ++s2 )
    ;

  if( *s1 == *s2 )  // both must equal '\0'
    return 1;
  else
    return 0;

  }  /* end of streqli */
