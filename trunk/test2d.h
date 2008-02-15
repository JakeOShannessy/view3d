#ifndef V3D_TEST2D_H
#define V3D_TEST2D_H

#include "common.h"
#include "view3d.h"

int SelfObstructionTest2D( SRFDAT2D *srf1, SRFDAT2D *srf2, SRFDAT2D *srfn );

int BoxTest2D( SRFDAT2D *srfN, SRFDAT2D *srfM, SRFDAT2D *srf, int *los
	, int *oos, int npos
);

void ClipSrf2D( SRFDAT2D *srf, const float flag );

int ProjectionDirection2D( SRFDAT2D *srfN, SRFDAT2D *srfM, SRFDAT2D *srf
	, int *probableObstr, int *orientObstr, View2DControlData *vfCtrl
);

void CoordTrans2D( SRFDAT2D *srf1, SRFDAT2D *srf2, SRFDAT2D *srf,
  int *probableObstr, View2DControlData *vfCtrl );
int ClipY0( Vec2 *v1, Vec2 *v2 );

void SetSrf2D( SRFDAT2D *srf );

int SetPosObstr2D( int nSrf, SRFDAT2D *srf, int *lpos );

void DumpSrf( SRFDAT2D *srf );

#endif

