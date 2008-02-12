/*subfile:  SaveVF.c  ********************************************************/

/*  Main program for batch processing of 3-D view factors.  */

#define V3D_BUILD
#include "savevf.h"

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "view3d.h"
#include "misc.h"

/***  SaveF0.c  **************************************************************/

/*  Save view factors as square array; + area + emit; text format.  */

static void SaveF0( char *fileName, char *header, int nSrf,
             float *area, float *emit, double **AF, float *F 
){
  FILE *vfout;
  int n;    /* row */
  int m;    /* column */

  vfout = fopen( fileName, "w" );
  fprintf( vfout, "%s", header );
  fprintf( vfout, "%g", area[1] );
  for( n=2; n<=nSrf; n++ )
    fprintf( vfout, " %g", area[n] );
  fprintf( vfout, "\n" );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    {
    double Ainv = 1.0 / area[n];
    for( m=1; m<=nSrf; m++ )      /* process column values */
      {
      if( m < n )
        F[m] = (float)(AF[n][m] * Ainv);
      else
        F[m] = (float)(AF[m][n] * Ainv);
      }
    fprintf( vfout, "%.6f", F[1] );   /* write row */
    for( m=2; m<=nSrf; m++ )
      fprintf( vfout, " %.6f", F[m] );
    fprintf( vfout, "\n" );
    }

  fprintf( vfout, "%.3f", emit[1] );
  for( n=2; n<=nSrf; n++ )
    fprintf( vfout, " %.3f", emit[n] );
  fprintf( vfout, "\n" );
  fclose( vfout );

}  /* end of SaveF0 */

/***  SaveF1.c  **************************************************************/

/*  Save view factors as square array; binary format. */

static void SaveF1( char *fileName, char *header, int nSrf,
             float *area, float *emit, double **AF, float *F 
){
  FILE *vfout;
  int n;    /* row */

  vfout = fopen( fileName, "wb" );
  fwrite( header, sizeof(char), 32, vfout );
  fwrite( area+1, sizeof(float), nSrf, vfout );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    {
    int m;    /* column */
    double Ainv = 1.0 / area[n];
    for( m=1; m<=nSrf; m++ )      /* process column values */
      {
      if( m < n )
        F[m] = (float)(AF[n][m] * Ainv);
      else
        F[m] = (float)(AF[m][n] * Ainv);
      }
    fwrite( F+1, sizeof(float), nSrf, vfout );   /* write row */
    }

  fwrite( emit+1, sizeof(float), nSrf, vfout );
  fclose( vfout );

}  /* end of SaveF1 */

/***  SaveAF.c  **************************************************************/

/*  Save view factors from 3D calculations.  */

void SaveAF( char *fileName, char *header, int nSrf, char *title, char **name, 
		float *area, float *emit, double **AF 
){
  FILE *vfout;
  int n;    /* row */

  vfout = fopen( fileName, "w" );
  fprintf( vfout, "%s", header );
  fprintf( vfout, "T %s\n", title );
  fprintf( vfout, "!  #     area          emit    name\n" );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    {
    int m;    /* column */
    fprintf( vfout, "%4d %14.6e %7.3f   %s\n",
      n, area[n], emit[n], name[n] );

    for( m=1; m<=n; m++ )  /* process column values */
      {
      fprintf( vfout, "%14.6e", AF[n][m] );
      if( m%5 )
        fputc( ' ', vfout );
      else
        fputc( '\n', vfout );
      }
    if( m%5 != 1 ) fputc( '\n', vfout );
    }
  fputc( '\n', vfout );
  fclose( vfout );

  }  /* end of SaveAF */

/***  SaveVF.c  **************************************************************/

/*  Save computed view factors.  */

void SaveVF( char *fileName, char *program, char *version,
             int format, int encl, int didemit, int nSrf,
             float *area, float *emit, double **AF, float *vtmp )
  {
  char header[32];
  int j;

  /* fill output file header line */
  sprintf( header, "%s %s %d %d %d %d",
    program, version, format, encl, didemit, nSrf );
  for( j=strlen(header); j<30; j++ )
    header[j] = ' ';
  header[30] = '\n';
  header[31] = '\0';

  if( format == 0 )  /* simple text file */
    {
    SaveF0( fileName, header, nSrf, area, emit, AF, vtmp );
    }
  else if( format == 1 )  /* simple binary file */
    {
    header[30] = '\r';
    header[31] = '\n';
    SaveF1( fileName, header, nSrf, area, emit, AF, vtmp );
    }
  else
    {
    error( 3, __FILE__, __LINE__, "Undefined format: ", IntStr(format), "" );
//    SaveAF( fileName, header, nSrf, title, name, area, emit, AF );
    }

  }  /* end SaveVF */

