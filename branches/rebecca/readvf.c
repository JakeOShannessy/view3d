/*subfile:  ReadVF.c  ********************************************************/

/*  Function to read View3D 3.2 view factor files.  */

#define V3D_BUILD
#include "readvf.h"
#include "misc.h"
#include "types.h"
#include "heap.h"

#include <stdio.h>

/***  ReadF0s.c  *************************************************************/

/*  Read view factors + area + emit; text format.  Save in square array.  */

static void ReadF0s(const char *fileName, const int nSrf, float *area, float *emit, float **F ){
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

static void ReadF0t(const char *fileName, const int nSrf, float *area, float *emit, double **AF ){
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

static void ReadF1s(const char *fileName, const int nSrf, float *area, float *emit, float **F){
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

static void ReadF1t(const char *fileName, const int nSrf, float *area, float *emit, double **AF ){
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

void ReadVF(const char *fileName, char *program, char *version,
		int *format, int *encl, int *didemit, int *nSrf,
		float *area, float *emit, double **AF, float **F
		, const int init, const int shape
){
  /* if(non-zero number) - it appears that (non-zero number = true) and therefore 
  the if statement will be executed. However
	 if(0) - (0 = false) and hence the if statement will NOT be executed.
	 http://www.cplusplus.com/forum/articles/3483/
  */
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

ViewFactors *read_view_factors(const char *filename){
	ViewFactors *V;
	V = V3D_NEW(ViewFactors);

	if(V==NULL){
		fprintf(stderr,"Unable to allocate ViewFactors\n");
		return NULL;
	}

	/* read the file header */
	ReadVF(filename, V->program, V->version
		, &(V->format), &(V->encl), &(V->didemit), &(V->nsrf)
		, NULL, NULL, NULL, NULL
		, 1/* to read headers */, 0/* shape doesn't matter for this */
	);

	/* allocate memory according to the size declared in the file */
	V->area = V3D_NEW_ARRAY(float, V->nsrf);
	V->emissivity = V3D_NEW_ARRAY(float, V->nsrf);
	V->AF = V3D_NEW_MATRIX_SYMM(float, V->nsrf);
	V->F = V3D_NEW_MATRIX(float,V->nsrf,V->nsrf);

	/* now read the actual data */
	ReadVF(filename, V->program, V->version
		, &(V->format), &(V->encl), &(V->didemit), &(V->nsrf)
		, V->area, V->emissivity, V->AF, V->F
		, 0/* to read data */, V3D_SHAPE_SQUARE
	);

	return V;
}


void view_factors_destroy(ViewFactors *V){
	V3D_FREE_ARRAY(float,V->nsrf,V->area);
	V3D_FREE_ARRAY(float,V->nsrf,V->emissivity);
	V3D_FREE_MATRIX_SYMM(float, V->nsrf, V->AF);
	V3D_FREE_MATRIX(float,V->nsrf,V->nsrf, V->F);
	V3D_FREE(ViewFactors, V);
}


