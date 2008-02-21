#ifndef V3D_GETDAT_H
#define V3D_GETDAT_H

#include "common.h"
#include "view3d.h"

V3D_API void CountVS3D(char *title, View3DControlData *vfCtrl );

V3D_API void GetVS3D(char **name, float *emit, int *base, int *cmbn
	, SRFDAT3D *srf, Vec3 *xyz, View3DControlData *vfCtrl
);

V3D_API void GetVS3Da( char **name, float *emit, int *base, int *cmbn
	, SRFDAT3D *srf, Vec3 *xyz, View3DControlData *vfCtrl
);

V3D_API void CountVS2D( char *title, View2DControlData *vfCtrl );

V3D_API void GetVS2D( char **name, float *emit, int *base, int *cmbn,
  SRFDAT2D *srf, View2DControlData *vfCtrl );

typedef enum{
	V3D_2D = 0
	,V3D_3D = 1
) geometry_type;

struct VertexSurfaceData_2D_struct{
	SRFDAT2D *srf;
	View2dControlData CD;
};
struct VertexSurfaceData_3D_struct{
	SRFDAT3D *srf;
	View3DControlData CD;
};

struct VertexSurfaceData_struct{
	char title[V3D_MAXTITLE];
	char **name;
	float *emissivity;
	int *base;
	int *cmbn;
	unsigned nsrf; /* radiation surfaces */
	unsigned nallsurf; /* all surfaces (including obstructions, null surfaces) */ 
	int echo;
	int listmode;
	geometry_type type;
	union{
		/* depending on the value of 'type', either... */
		VertexSurfaceData_2D_struct d2;
		VertexSurfaceData_3D_struct d3;
	}
};

VertexSurfaceData *read_vertex_surface_data(const char *filename);

void vertex_surface_data_destroy(VertexSurfaceData *V);

#endif

