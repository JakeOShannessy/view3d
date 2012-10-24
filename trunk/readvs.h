#ifndef V3D_READVS_H
#define V3D_READVS_H

#include "common.h"
#include "view3d.h"

/* unified data types for reading vertex/surface data both 2D and 3D */

struct VertexSurfaceData_2D_struct{
	SRFDAT2D *srf;
};

struct VertexSurfaceData_3D_struct{
	SRFDAT3D *srf;
};

#define V3D_MAXTITLE 256

typedef struct VertexSurfaceData_struct{
	char title[V3D_MAXTITLE];
	char **name;
	float *emissivity;
	int *base;
	int *cmbn;
	unsigned nvert; /* number of vertices */
	unsigned nrad; /* radiation surfaces */
	unsigned nobst;
	unsigned nall; /* all surfaces (including obstructions, null surfaces) */ 
	unsigned format;
	unsigned row, col; /* row and column of view factor matrix that is to be reported */
	int echo;
	int listmode;
	View3DControlData CD;
	union{
		/* depending on the value of 'format' ('2' or other)... */
		struct VertexSurfaceData_2D_struct d2;
		struct VertexSurfaceData_3D_struct d3;
	};
} VertexSurfaceData;

#define V3D_IS_2D(V) ((V)->format==2)

/* unified file reading routines */

V3D_API VertexSurfaceData *read_vertex_surface_data(const char *filename);

V3D_API void vertex_surface_data_destroy(VertexSurfaceData *V);

#endif

