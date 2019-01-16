#ifndef V3D_VIEWUNOB_H
#define V3D_VIEWUNOB_H

#include "view3d.h"

typedef struct {
  EdgeDir *rc1; /* edge DirCos of surface 1 */
  EdgeDir *rc2; /* edge DirCos of surface 2 */
  EdgeDivision **dv1;  /* edge divisions of surface 1 */
  EdgeDivision **dv2;  /* edge divisions of surface 2 */
} EdgeData;

double ViewUnobstructed( View3DControlData *vfCtrl, int row, int col );
EdgeData ViewsInit( int maxDiv, int init, EdgeData edgeDataOld);

int SubSrf( const int nDiv, const int nv, const Vec3 *v, const double area,
  Vec3 *pt, double *wt );

#endif

