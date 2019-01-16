#define V3D_BUILD
#include "readvs.h"

#include "getdat.h"
#include "heap.h"
#include "test2d.h"

#include <ctype.h>
#include <string.h>
#include <float.h>

extern  int _echo;      /* if true, echo NXT input file */

int LongCon( char *str, long *i);

V3D_API float CPUtime(float t1);

int streqli( char *s1, char *s2 );
int streql( char *s1, char *s2 );

int FltCon( char *str, float *f );
int IntCon( char *str, int *i );

int DblCon( char *str, double *f );

#define ERRORX(SEV,FIL,LIN,FMT,...) (fprintf(stderr,"%s:%d: ",FIL,LIN),fprintf(stderr,FMT "\n",##__VA_ARGS__))
#define ERROR2(FMT,...) ERRORX(2,__FILE__,__LINE__,FMT,##__VA_ARGS__)
#define ERROR1(FMT,...) ERRORX(1,__FILE__,__LINE__,FMT,##__VA_ARGS__)

#define BAD_INTEGER_VALUE(STR) ERRORX(2, __FILE__, __LINE__, "Bad integer value: '%s'", STR)

#include <time.h>   /* prototype: clock;  define CLOCKS_PER_SEC */
#include <math.h>   /* prototype: fabs */

/*  Get characters to end of line (\n --> \0); used by NxtWord().  */

char *NxtLine(FILE *f, char *str, int maxlen ) {
  int c=0;       /* character read from f */
  int i=0;       /* current position in str */

  while( c!='\n' ){
    c = getc(f);
    if( c==EOF ) return NULL;
    if( _echo ) putc( c, _ulog );
    if( maxlen < 1 ) continue;   /* do not fill buffer */
    if( c == '\r' ) continue;    /* 2007/10/07 Linux EOL = \n\r */
    str[i++] = (char)c;
    if( i == maxlen ){
      str[i-1] = '\0';
      ERRORX( 3, __FILE__, __LINE__, "Buffer overflow: '%s'", str);
    }
  }
  if( i )
    str[i-1] = '\0';
  else
    str[0] = '\0';

  return str;

}  /* end NxtLine */


/*  Get the next word from file inHandle.  Return NULL at end-of-file.
 *  Assuming standard word separators (blank, comma, tab),
 *  comment identifiers (! to end-of-line), and
 *  end of data (* or end-of-file). */
/*  Major change October 2007:  ContamX uses NxtWord to read multiple files
 *  which are open simultaneously. The old static variable "newl" may cause
 *  an error. It has been replaced by using ungetc() to note end-of-word (EOW)
 *  which may also be end-of-line (EOL) character.
 *  Initialization with flag = -1 in now invalid - debug checked. */

char *NxtWord(FILE *inHandle, char *str, int flag, int maxlen )
/* str;   buffer where word is stored; return pointer.
 * flag:  0:  get next word from current position in inHandle;
          1:  get 1st word from next line of inHandle;
          2:  get remainder of current line from inHandle (\n --> \0);
          3:  get next data line from inHandle (\n --> \0);
          4:  get next line (even if comment) (\n --> \0).
 * maxlen: length of buffer to test for overflow. */
  {
  int c;         /* character read from inHandle */
  int i=0;       /* current position in str */
  int done=0;    /* true when start of word is found or word is complete */
  c = getc(inHandle);
  if( flag > 0 )
    {
    if( c != '\n' )  /* last call did not end at EOL; ready to read next char. */
      {                /* would miss first char if reading first line of file. */
      if( flag == 2 )
        {
        if( ftell( inHandle) == 1 ) /* 2008/01/16 */
          ungetc( c, inHandle );  /* restore first char of first line */
        NxtLine(inHandle, str, maxlen );  /* read to EOL filling buffer */
        }
      else
        NxtLine(inHandle, str, 0 );       /* skip to EOL; fix size 2008/01/16 */
        /* if flag = 1; continue to read first word on next line */
      }
    if( flag > 1 )
      {
        /* if flag = 2; return (partial?) line just read */
      if( flag > 2 )
        {
        /* if flag > 2; return all of next line (must fit in buffer) */
        NxtLine(inHandle, str, maxlen );
        if( flag == 3 )  /* skip comment lines */
          while( str[0] == '!' )
            NxtLine(inHandle, str, maxlen );
#ifdef _DEBUG
        if( flag > 4 )
          ERRORX( 3, __FILE__, __LINE__, "Invalid flag: %d", flag);
#endif
        }
      ungetc( '\n', inHandle);  /* restore EOL character for next call */
      return str;
      }
    }
  else  /* flag == 0 */
    {
#ifdef _DEBUG
    if( flag < 0 )
      ERRORX( 3, __FILE__, __LINE__,
        "Invalid flag: %d", flag);
#endif
    if( c == ' ' || c == ',' || c == '\n' || c == '\t' )
      ; /* skip EOW char saved at last call */
    else
      ungetc( c, inHandle);  /* restore first char of first line */
    }

  while( !done )   /* search for start of next word */
    {
    c = getc(inHandle);
    if( c==EOF ) return NULL;
    if( _echo ) putc( c, _ulog );
    switch( c )
      {
      case ' ':          /* skip word separators */
      case ',':
      case '\n':
      case '\r':
      case '\t':
      case '\0':
        break;
      case '!':          /* begin comment; skip to EOL */
        NxtLine(inHandle, str, 0 );
        break;
      case '*':          /* end-of-file indicator */
        return NULL;
      default:           /* first character of word found */
        str[i++] = (char)c;
        done = 1;
        break;
      }
    }  /* end start-of-word search */

  done = 0;
  while( !done )   /* search for end-of-word (EOW) */
    {
    c = getc(inHandle);
    if( c==EOF ) return NULL;
    if( _echo ) putc( c, _ulog );
    switch( c )
      {
      case '\n':   /* EOW characters */
      case ' ':
      case ',':
      case '\t':
        str[i] = '\0';
        done = 1;
        break;
      case '\r':   /* 2004/01/14 here for Linux: EOL = \n\r */
      case '\0':
        break;
      default:     /* accumulate word in buffer */
        str[i++] = (char)c;
        if( i == maxlen )  /* with overflow test */
          {
          str[i-1] = '\0';
          ERRORX( 3, __FILE__, __LINE__, "Buffer overflow: %s", str);
          }
        break;
      }
    }  /* end EOW search */
  ungetc( c, inHandle); /* save EOW character for next call */

  return str;

}  /* end NxtWord */



static int ReadIX(FILE *inHandle){
  long value;
  char tmpstr[LINELEN];

  NxtWord(inHandle, tmpstr, 0, sizeof(tmpstr) );
  if(LongCon(tmpstr, &value ) || value > INT_MAX || value < INT_MIN ){ /* max/min depends on compiler */
   	ERROR2("Bad integer: '%s'", tmpstr);
  }

  return (int)value;

}  /* end ReadIX */


static float ReadR4(FILE *inHandle){
  double value;
  char tmpstr[LINELEN];

  NxtWord(inHandle,tmpstr, 0, sizeof(tmpstr) );
  if( DblCon(tmpstr, &value ) || value > FLT_MAX || value < -FLT_MAX ){
	ERROR2("Bad float value '%s'", tmpstr);
  }
  return (float)value;

}  /* end ReadR4 */

/*----------------------------------------------------------------------------*/


/*
	Process simulation control values.
	This is currently an exact copy of the code in getdat.c. But we plan to
	reimplement it perhaps using a flex/bison approach.
*/
static void GetCtrl( char *str, View3DControlData *vfCtrl){
  char *p;
  int i;
  float r;
  char *context; /* parsing state pointer, needed for thread-safety */

  p = strtok_r( str, "= ,", &context);
  while( p ){
    if( streqli( p, "eps" ) ){
      p = strtok_r( NULL, "= ,", &context );
      if( FltCon( p, &r )){
        ERROR2("Bad float value: '%s'", p);
      }else{
        vfCtrl->epsAdap = r;
        if( r < 0.99e-6 ){
          ERROR1("Convergence limit < 1.0e-6");
		}
        if( r > 1.0e-2 ){
          ERROR1("Convergence limit > 1.0e-2");
		}
      }
    }
    else if( streqli( p, "list" )){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) ){
        BAD_INTEGER_VALUE(p);
      }else
        _list = i;
    }else if( streqli( p, "out" ) ){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) ){
        BAD_INTEGER_VALUE(p);
      }else{
        if( i < 0 || i > 2 ){
          ERROR2("Invalid output file format");
        }else{
          vfCtrl->outFormat = i;
		}
	  }
    }else if( streqli( p, "encl" )){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) ){
        BAD_INTEGER_VALUE(p);
      }else{
        if( i ) vfCtrl->enclosure = 1;
	  }
    }else if( streqli( p, "emit" )){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) ){
        BAD_INTEGER_VALUE(p);
      }else{
        if( i ) vfCtrl->emittances = 1;
	  }
    }else if( streqli( p, "maxU" ) ){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) ){
        BAD_INTEGER_VALUE(p);
      }else{
        if( i < 4 ){
          ERROR1("Maximum unobstructed recursions reset to 4");
          i = 4;
        }
        if( i > 12 )
          ERROR1("Maximum unobstructed recursions may be too large");
        vfCtrl->maxRecursALI = i;
      }
    }else if( streqli( p, "maxO" )){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) ){
        BAD_INTEGER_VALUE(p);
      }else{
        if( i < 4 ){
          ERROR1("Maximum obstructed recursions reset to 4");
          i = 4;
        }
        if( i > 12 )
          ERROR1("Maximum obstructed recursions may be too large");
        vfCtrl->maxRecursion = i;
      }
    }else if( streqli( p, "minO" )){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) )
        BAD_INTEGER_VALUE(p);
      else{
        if( i < 0 )
         i = 0;
        if( i > 2 )
          ERROR1("Minimum obstructed recursions may be too large");
        vfCtrl->minRecursion = i;
      }
    }else if( streqli( p, "row" ) ){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) )
        BAD_INTEGER_VALUE(p);
      else{
        if( i < 0 )
          i = 0;
        vfCtrl->row = i;
      }
    }else if( streqli( p, "col" ) ){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) )
        BAD_INTEGER_VALUE(p);
      else{
        if( i < 0 )
          i = 0;
        vfCtrl->col = i;
      }
    }else if( streqli( p, "prjD" ) ){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) )
        BAD_INTEGER_VALUE(p);
      else
        if( i ) vfCtrl->prjReverse = 1;
    }else if( streqli( p, "maxV" ) ){
      p = strtok_r( NULL, "= ,", &context );
      if( IntCon( p, &i ) )
        BAD_INTEGER_VALUE(p);
      else
        _maxNVT = i;
    }else{
      ERROR1("Invalid control word: '%s'", p);
      p = strtok_r( NULL, "= ,", &context );
    }

    p = strtok_r( NULL, "= ,", &context );    /* get next word */
  }

  if( vfCtrl->col && !vfCtrl->row )
    ERROR2("Must set row before setting column");

}  /* end GetCtrl */

/*------------------------------------------------------------------------------
  unified file reading routine
*/

/**
	Determine number of vertices, number of surfaces,
	control parameters, and format of the input file.
*/
static int CountVS(FILE *inHandle, VertexSurfaceData *V){
  int c;     /* first character in line */
  int flag=0;  /* NxtWord flag: 0 for first word of first line */

  V->nrad = 0;
  V->nobst = 0;
  V->format = 0;
  char tmpstr[LINELEN];

  fprintf(stderr,"Reading vertex/surface count...\n");

  while( NxtWord(inHandle, tmpstr, flag, sizeof(tmpstr) ) != NULL ){
    c = toupper( tmpstr[0] );
    switch(c){
      case 'S':               /* surface */
        V->nrad += 1;
        break;
      case 'V':               /* vertex */
        V->nvert += 1;
        break;
      case 'O':               /* obstruction */
        V->nobst += 1;
        break;
      case 'T':               /* title */
        NxtWord(inHandle, V->title, 2, sizeof(tmpstr) );
        break;
      case 'F':               /* input file format: geometry */
	  case 'G':
        NxtWord(inHandle, tmpstr, 0, sizeof(tmpstr) );
        V->format = 0;
        if( streql( tmpstr, "2"  )){
			fprintf(stderr,"FILE FORMAT = 2\n");
			V->format = 2;
		}
        if( streql( tmpstr, "3"  ) ) V->format = 3;
        if( streqli( tmpstr, "3a" ) ) V->format = 4;
        break;
      case 'C':               /* C run control data */
        NxtWord(inHandle, tmpstr, 2, sizeof(tmpstr) );
		GetCtrl(tmpstr, &(V->CD));
        break;
      case 'E':
        goto finish;
      default:
        break;
    }
    flag = 1;
  }

finish:

  V->nall = V->nrad + V->nobst;
  if(V->row > V->nrad){
	ERROR2("\"row\" value too large");
	return 1;
  }
  if(V->col > V->nrad){
	ERROR2("\"row\" value too large");
	return 1;
  }

  return 0;
}  /*  end of CountVS  */


static int GetVS(FILE *inHandle, VertexSurfaceData *V){
  char c;       /* first character in line */
  int nv=0;    /* number of vertices */
  int ns=0;    /* number of surfaces */
  int flag=0;  /* NxtWord flag: 0 for first word of first line */
  Vec2 *xy;  /* vector of vertces [1:nVertices] */
  int n;
  char tmpstr[LINELEN];

  rewind(inHandle);

  xy = Alc_V( 1, V->nvert, sizeof(Vec2), __FILE__, __LINE__ );

  while( NxtWord(inHandle, tmpstr, flag, sizeof(tmpstr) ) != NULL ){
    c = toupper( tmpstr[0] );
    switch( c )
    {
      case 'V':
        n = ReadIX(inHandle);
        nv += 1;
        if( n!= nv )ERROR2("Vertex out of sequence: '%d'", n);
        xy[nv].x = ReadR4(inHandle);
        xy[nv].y = ReadR4(inHandle);
        break;
      case 'S':
      case 'O':
        n = ReadIX(inHandle);
        ns += 1;
        if( n!= ns ) ERROR2("Surface out of sequence: %d", n);
        if( c=='O' && n <= V->nrad) ERROR2("Obstruction surface out of sequence: '%d'", n);
        if( c=='S' && n > V->nrad) ERROR2("Radiating surface out of sequence: '%d'", n);
        V->d2.srf[ns].nr = ns;
        V->d2.srf[ns].type = 1;

        n = ReadIX(inHandle);
        if(n <= 0 || n > V->nvert) ERROR2("Improper first vertex; surface '%d'",ns);
        else{
          V->d2.srf[ns].v1.x = xy[n].x;
          V->d2.srf[ns].v1.y = xy[n].y;
        }

        n = ReadIX(inHandle);
        if(n <= 0 || n > V->nvert){
			ERROR2("Improper second vertex; surface '%d'",ns);
		}else{
          V->d2.srf[ns].v2.x = xy[n].x;
          V->d2.srf[ns].v2.y = xy[n].y;
        }

        SetSrf2D( &V->d2.srf[ns] );          /* compute area and direction cosines */

        if( c=='O' ) break;

        V->d2.srf[ns].type = 0;            /* non-obstruction surface data */
        n = ReadIX(inHandle);              /* obstruction surface number */
        if( n>0 )
          V->d2.srf[ns].type = -2;
        if( n<0 || (n <= V->nrad && n>0) || n > V->nall)
          ERROR2("Improper obstruction surface number: '%d'",n);

        n = ReadIX(inHandle);              /* base surface number */
        V->base[ns] = n;
        if( n<0 || n > V->nrad) ERROR2("Improper base surface number: %d",n);
        else if( n>0 ){
          V->d2.srf[ns].type = -1;
          if( n>=ns ) ERROR2("Subsurface %d must be after base surface %d.",ns,n);
          else{
            int i;
            for( i=ns-1; i>n; i-- ){
              if(V->base[i] == 0) break;
            }
            if( i != n ) ERROR2("Subsurface %d must be immediately after base surface %d",ns,n);
          }
        }

        n = ReadIX(inHandle);              /* combine surface number */
        V->cmbn[ns] = n;
        if( n>=ns ) ERROR2("Must combine surface with previous surface: '%d'",n);
        if(V->cmbn[ns])
          if(V->cmbn[n]) ERROR2("May not chain combined surfaces: %d", n);
        if( n<0 || n > V->nrad) ERROR2("Improper combine surface number: %d", n);

        V->emissivity[ns] = ReadR4(inHandle);       /* surface emittance */
        if(V->emissivity[ns] > 0.99901){
          ERROR1("Replacing surface %d emittance ('%s') with 0.999",ns,tmpstr);
          V->emissivity[ns] = 0.999f;
        }
        if(V->emissivity[ns] < 0.00099 ){
          ERROR1(" Replacing surface %d emittance ('%s') with 0.001", ns,tmpstr);
          V->emissivity[ns] = 0.001f;
        }

        NxtWord(inHandle, tmpstr, 0, sizeof(tmpstr) );        /* surface name */
        strncpy(V->name[ns], tmpstr, NAMELEN );
        V->name[ns][NAMELEN-1] = '\0';  /* guarantee termination */
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
        ERROR1("Undefined input identifier");
        break;
    }
    flag = 1;
  }

finish:
  Fre_V( xy, 1, V->nvert, sizeof(Vec2), __FILE__, __LINE__ );
  return 0;
}  /*  end of GetVS2D  */



VertexSurfaceData *read_vertex_surface_data(const char *filename){
	FILE *f;
	VertexSurfaceData *V;
	int res;

	/* read Vertex/Surface data file */
	f = fopen(filename,"r");
	if(!f){
		fprintf(stderr,"Unable to open vertex/surface data file '%s'\n",filename);
		return NULL;
	}

	fprintf(stderr,"Openend vertex/surface file OK...");

	/* allocate and zero space for the data */
	V = V3D_NEW(VertexSurfaceData);
	memset(V, 0, sizeof(VertexSurfaceData));

	/* read the file header and count the amount of required data allocation */
	res = CountVS(f, V);
	if(res){
		fprintf(stderr,"Unable to count vertices/surfaces from data file '%s'\n",filename);
		return NULL;
	}

	fprintf(stderr,"File (format %d) has  %d radiation and %d obstruction surfaces, and %d total vertices\n",V->format, V->nrad, V->nobst, V->nvert);

	/* allocate the data in the VertexSurfaceData struct */
	V->name = Alc_MC( 1, V->nrad, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
	V->emissivity = V3D_NEW_ARRAY(float,V->nrad);
	V->base = V3D_NEW_ARRAY(int,V->nrad);
	V->cmbn = V3D_NEW_ARRAY(int,V->nrad);
	if(V3D_IS_2D(V)){
		V->d2.srf = V3D_NEW_ARRAY(SRFDAT2D,V->nall);
	}else{
		V->d3.srf = V3D_NEW_ARRAY(SRFDAT3D,V->nall);
	}

	/* do we need to repoen the file pointer back to zero? */

    /* Read the actual data into memory */
	res = GetVS(f, V);
	if(res){
		fprintf(stderr,"Error reading data file '%s' (res=%d)\n",filename,res);
		return NULL;
	}

	fclose(f);

	return V;
}

void vertex_surface_data_destroy(VertexSurfaceData *V){
	Fre_MC((void **)V->name, 1, V->nrad, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
	V3D_FREE_ARRAY(float,V->nrad,V->emissivity);
	V3D_FREE_ARRAY(int,V->nrad,V->base);
	V3D_FREE_ARRAY(int,V->nrad,V->cmbn);
	if(V3D_IS_2D(V)){
		V3D_FREE_ARRAY(SRFDAT2D,V->nall,V->d2.srf);
	}else{
		V3D_FREE_ARRAY(SRFDAT3D,V->nall,V->d3.srf);
	}
	V3D_FREE(VertexSurfaceData,V);
}
