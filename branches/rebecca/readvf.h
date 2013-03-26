#ifndef V3D_READVF_H
#define V3D_READVF_H

#include "common.h"

typedef struct{
	char program[36];
	char version[36];
	int format;
	int encl;
	int didemit;
	int nsrf;
	float *area;
	float *emissivity;
	double **AF;
	float **F;
} ViewFactors;

/**
	Read view factors file
	@param fileName name of file to be read
	@param init whether to read file reader or values
	@param shape shape-description format to be used when reading values.

	If init is nonzero, ReadVF reads the file header into and returns values 
	program, version, format, encl, didemit.

	If init is zero, ReadVF reads the data from the file and results values
	area, emit, AF, F (how they are read depends on the setting of shape.
*/
V3D_API void ReadVF(const char *fileName, char *program, char *version
		, int *format, int *encl, int *didemit, int *nSrf
		, float *area, float *emit, double **AF, float **F
		, const int init, const int shape
);

typedef enum{
	V3D_SHAPE_TRIANGULAR=0
	, V3D_SHAPE_SQUARE=1
} shape_enum;

/**
	An all-in-one function for reading view factors from a file. This
	function allocates the necessary storage space for the returned
	object, or (should) return NULL if something went wrong.
*/
V3D_API ViewFactors *read_view_factors(
		const char *filename
);

V3D_API void view_factors_destroy(ViewFactors *V);

#endif

