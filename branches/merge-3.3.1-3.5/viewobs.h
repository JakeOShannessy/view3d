#ifndef V3D_VIEWOBS_H
#define V3D_VIEWOBS_H

#include "view3d.h"

double ViewObstructed( View3DControlData *vfCtrl, int nv1, Vec3 v1[], double area, int nDiv );
double View1AI( int nss, Vec3 *p1, double *area1, DirCos *dc1, SRFDAT3X *srf2 );
int Subsurface( SRFDAT3X *srf, SRFDAT3X sub[] );
double SetCentroid( const int nv, Vec3 *vs, Vec3 *ctd );
double Triangle( Vec3 *p1, Vec3 *p2, Vec3 *p3, void *dc, int dcflag );
double ViewTP( Vec3 v1[], double area, int level, View3DControlData *vfCtrl );
double ViewRP( Vec3 v1[], double area, int level, View3DControlData *vfCtrl );

#endif

