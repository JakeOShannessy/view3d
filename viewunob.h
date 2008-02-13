#ifndef V3D_VIEWUNOB_H
#define V3D_VIEWUNOB_H

#include "view3d.h"

double ViewUnobstructed( View3DControlData *vfCtrl, int row, int col );
void ViewsInit( int maxDiv, int init );

int SubSrf( const int nDiv, const int nv, const Vec3 *v, const double area,
  Vec3 *pt, double *wt );

#endif

