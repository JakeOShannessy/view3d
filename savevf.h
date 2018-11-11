#ifndef V3D_SAVEVF_H
#define V3D_SAVEVF_H

#include "common.h"
#include "view3d.h"

V3D_API void SaveVF( FILE *file, char *program, char *version,
		int format, int encl, int didemit, int nSrf,
		float *area, float *emit, double **AF, float *vtmp
);

V3D_API void SaveVFNew( FILE *file, VFResultsC res);

#endif
