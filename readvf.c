/*subfile:  ReadVF.c  ********************************************************/

/*  Function to read View3D 3.2 view factor files.  */

#include <stdio.h>
#include "types.h"

#define V3D_BUILD
#include "readvf.h"
#include "misc.h"

/***  ReadF0s.c  *************************************************************/

/*  Read view factors + area + emit; text format.  Save in square array.  */

static void ReadF0s( char *fileName, int nSrf, float *area, float *emit, float **F ){
  FILE *vfin;
  char header[36];
  int n;    /* row */
  int m;    /* column */

  vfin = fopen( fileName, "r" );
  fgets( header, 35, vfin );
  for( n=1; n<=nSrf; n++ )
    fscanf( vfin, "%f", &area[n] );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    for( m=1; m<=nSrf; m++ )      /* process column values */
      fscanf( vfin, "%f", &F[n][m] );

  for( n=1; n<=nSrf; n++ )
    fscanf( vfin, "%f", &emit[n] );
  fclose( vfin );

}  /* end of ReadF0s */

/***  ReadF0t.c  *************************************************************/

/*  Read view factors + area + emit; text format. Save in triangular array.  */

static void ReadF0t( char *fileName, int nSrf, float *area, float *emit, double **AF ){
  FILE *vfin;
  char header[36];
  int n;    /* row */
  int m;    /* column */

  vfin = fopen( fileName, "r" );
  fgets( header, 35, vfin );
  for( n=1; n<=nSrf; n++ )
    fscanf( vfin, "%f", &area[n] );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    {
    float F;
    for( m=1; m<=n; m++ )      /* process column values */
      {
      fscanf( vfin, "%f", &F );
      AF[n][m] = F * area[n];
      }
    for( ; m<=nSrf; m++ )
      fscanf( vfin, "%f", &F );
    }

  for( n=1; n<=nSrf; n++ )
    fscanf( vfin, "%f", &emit[n] );
  fclose( vfin );

}  /* end of ReadF0t */

/***  ReadF1s.c  *************************************************************/

/*  Read view factors + area + emit; binary format.  Save in square array.  */

static void ReadF1s( char *fileName, int nSrf, float *area, float *emit, float **F){
  FILE *vfin;
  char header[36];
  int n;    /* row */

  vfin = fopen( fileName, "rb" );
  fread( header, sizeof(char), 32, vfin );
  fread( area+1, sizeof(float), nSrf, vfin );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    fread( F[n]+1, sizeof(float), nSrf, vfin );

  fread( emit+1, sizeof(float), nSrf, vfin );
  fclose( vfin );

}  /* end of ReadF1s */

/***  ReadF1t.c  *************************************************************/

/* Read view factors + area + emit; binary format. Save in triangular array. */

static void ReadF1t( char *fileName, int nSrf, float *area, float *emit, double **AF ){
  FILE *vfin;
  char header[36];
  int n;    /* row */
  int m;    /* column */

  vfin = fopen( fileName, "rb" );
  fread( header, sizeof(char), 32, vfin );
  fread( area+1, sizeof(float), nSrf, vfin );

  for( n=1; n<=nSrf; n++ ){      /* process AF values for row n */
    fread( emit+1, sizeof(float), nSrf, vfin );  /* read F into emit */
    for( m=1; m<=n; m++ )      /* process column values */
      AF[n][m] = emit[m] * area[n];
  }

  fread( emit+1, sizeof(float), nSrf, vfin );
  fclose( vfin );
}  /* end of ReadF1t */

/***  ReadVF.c  **************************************************************/

/*  Read view factors file.  */

void ReadVF( char *fileName, char *program, char *version,
		int *format, int *encl, int *didemit, int *nSrf,
		float *area, float *emit, double **AF, float **F, int init, int shape
){
  if(init){
    char header[36];
    FILE *vfin = fopen( fileName, "r" );
    fgets( header, 35, vfin );
    sscanf( header, "%s %s %d %d %d %d"
		, program, version, format, encl, didemit, nSrf
	);
    fclose( vfin );
  }else{
    int ns = *nSrf;
    if( *format == 0 ){
      if( shape == 0 )
        ReadF0t( fileName, ns, area, emit, AF );
      else
        ReadF0s( fileName, ns, area, emit, F );
    }else if( *format == 1 ){
      if( shape == 0 )
        ReadF1t( fileName, ns, area, emit, AF );
      else
        ReadF1s( fileName, ns, area, emit, F );
    }else{
      error( 3, __FILE__, __LINE__, "Undefined format: ", IntStr(*format), "" );
	}
  }
}  /* end ReadVF */

