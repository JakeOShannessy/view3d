#ifndef V3D_TEST3D_H
#define V3D_TEST3D_H

#include "common.h"
#include "view3d.h"

/* 3-D view test functions */
int AddMaskSrf( SRFDAT3D *srf, const SRFDATNM *srfN, const SRFDATNM *srfM,
  const int *maskSrf, const int *baseSrf, View3DControlData *vfCtrl, int *los, int nPoss );
int BoxTest( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm, View3DControlData *vfCtrl,
  int *los, int nProb );
int ClipPolygon( const int flag, const int nv, Vec3 *v,
  double *dot, Vec3 *vc );
int ConeRadiusTest( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm,
  View3DControlData *vfCtrl, int *los, int nProb, double distNM );
int CylinderRadiusTest( SRFDAT3D *srf, SRFDATNM *srfN, SRFDATNM *srfM,
  int *los, double distNM, int nProb );
int OrientationTest( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm, 
  View3DControlData *vfCtrl, int *los, int nProb );
int OrientationTestN( SRFDAT3D *srf, int N, View3DControlData *vfCtrl,
  int *possibleObstr, int nPossObstr );
void SelfObstructionClip( SRFDATNM *srfn );
int SetShape( const int nv, Vec3 *v, double *area );
int SelfObstructionTest3D( SRFDAT3D *srf1, SRFDAT3D *srf2, SRFDATNM *srfn );
void IntersectionTest( SRFDATNM *srfn, SRFDATNM *srfm );

V3D_API void DumpOS( char *title, const int nos, int *los );

V3D_API int SetPosObstr3D( int nSrf, SRFDAT3D *srf, int *lpos );

#endif

