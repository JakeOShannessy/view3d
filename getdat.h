#ifndef V3D_GETDAT_H
#define V3D_GETDAT_H

#include "common.h"
#include "view3d.h"


V3D_API void CountVS3D( FILE *inHandle, char *title, View3DControlData *vfCtrl );

V3D_API void GetVS3D( FILE *inHandle, char **name, float *emit, int *base, int *cmbn
	, SRFDAT3D *srf, Vec3 *xyz, View3DControlData *vfCtrl
);

V3D_API void GetVS3Da( FILE *inHandle, char **name, float *emit, int *base, int *cmbn
	, SRFDAT3D *srf, Vec3 *xyz, View3DControlData *vfCtrl
);

V3D_API void CountVS2D( FILE *inHandle, char *title, View2DControlData *vfCtrl );

V3D_API void GetVS2D( FILE *inHandle, char **name, float *emit, int *base, int *cmbn,
  SRFDAT2D *srf, View2DControlData *vfCtrl );

#endif

