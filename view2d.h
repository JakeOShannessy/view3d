/*subfile:  view2d.h *********************************************************/
/*  include file for the VIEW2D program */

#include "common.h"
#include "view3d.h"

/* 2-D view factor functions */
V3D_API void View2D( SRFDAT2D *srf, double **AF, View2DControlData *vfCtrl );

double FA1A2( Vec2 *v1a, Vec2 *v1b, Vec2 *v2a, Vec2 *v2b );
double FdA1A2( Vec2 *v1, DirCos2 *u1, Vec2 *v2a, Vec2 *v2b );
int ClipYC( Vec2 *v1, Vec2 *v2, const double yc );
double ViewObstructed( Vec2 *v1, View2DControlData *vfCtrl );
int LineOverlap( LINE *base, LINE *new, LINE *used, LINE *free, const double eps );
double ViewAdapt2D( Vec2 *v1, Vec2 *v2, double area, int level, View2DControlData *vfCtrl );
int errorf( int severity, char *file, int line, ... );

/* text here moved to test2d.h */

/* post processing functions the same as for View3D */

