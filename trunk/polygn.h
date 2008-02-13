#ifndef V3D_POLYGN_H
#define V3D_POLYGN_H

#include "common.h"
#include "view3d.h"

/* polygon processing */
int PolygonOverlap( const Polygon *p1, Polygon *p2, const int flagOP, int freeP2 );
void FreePolygons( Polygon *first, Polygon *last );
Polygon *SetPolygonHC( const int nVrt, const Vec2 *polyVrt, const double trns );
int GetPolygonVrt2D( const Polygon *pp, Vec2 *polyVrt );
int GetPolygonVrt3D( const Polygon *pp, Vec3 *srfVrt );
Polygon *GetPolygonHC( void );
PolygonVertexEdge *GetVrtEdgeHC( void );
void NewPolygonStack( void );
Polygon *TopOfPolygonStack( void );
V3D_API void InitTmpVertMem( void );
V3D_API void FreeTmpVertMem( void );
V3D_API void InitPolygonMem( const double epsDist, const double epsArea );
V3D_API void FreePolygonMem( void );
int LimitPolygon( int nVrt, Vec2 polyVrt[],
  const double maxX, const double minX, const double maxY, const double minY );
void DumpHC( char *title, const Polygon *pfp, const Polygon *plp );
void DumpFreePolygons( void );
void DumpFreeVertices( void );
void DumpP2D( char *title, const int nvs, Vec2 *vs );
void DumpP3D( char *title, const int nvs, Vec3 *vs );

#endif

