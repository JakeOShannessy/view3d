#ifndef V3D_CTRANS_H
#define V3D_CTRANS_H

#include "view3d.h"

     /* vector functions */
void CoordTrans3D( SRFDAT3D *srfAll, SRFDATNM *srf1, SRFDATNM *srf2,
  int *probableObstr, View3DControlData *vfCtrl );
void DumpSrf3D( char *title, SRFDAT3D *srf );
void DumpSrfNM( char *title, SRFDATNM *srf );
void Dump3X( char *tittle, SRFDAT3X *srfT );
void DumpVA( char *title, const int rows, const int cols, double *a );

#endif
