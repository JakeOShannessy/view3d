#ifndef V3D_VIEWPP_H
#define V3D_VIEWPP_H

/* post processing */

#include "common.h"
#include "view3d.h"

V3D_API int DelNull( const int nSrf, SRFDAT3D *srf, int *base, int *cmbn
	, float *emit, float *area, char **name, double **AF
);

V3D_API void NormAF( const int nSrf, const float *emit, const float *area
	, double **AF, const double eMax, const int itMax
);

V3D_API int Combine( const int nSrf, const int *cmbn, float *area, char **name, double **AF );
V3D_API void Separate( const int nSrf, const int *base, float *area, double **AF );
V3D_API void IntFac( const int nSrf, const float *emit, const float *area, double **AF );
void LUFactorSymm( const int neq, double **a );
void LUSolveSymm( const int neq, double **a, double *b );
void DAXpY( const int n, const double a, const double *x, double *y );
double DotProd( const int n, const double *x, const double *y );

#endif

