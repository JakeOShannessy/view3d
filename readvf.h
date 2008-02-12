#ifndef V3D_READVF_H
#define V3D_READVF_H

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
void ReadVF( char *fileName, char *program, char *version,
             int *format, int *encl, int *didemit, int *nSrf,
             float *area, float *emit, double **AF, float **F, int init, int shape );

#endif

