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

#endif

