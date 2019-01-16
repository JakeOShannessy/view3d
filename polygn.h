#ifndef V3D_POLYGN_H
#define V3D_POLYGN_H

#include "common.h"
#include "view3d.h"

typedef struct {
  char *memPoly; /* memory block for polygon descriptions; must start NULL */
  PolygonVertexEdge *nextFreeVE; /* pointer to next free vertex/edge */ /* this is a linked list */
  Polygon *nextFreePD; /* pointer to next free polygon descripton */
  Polygon *nextUsedPD; /* pointer to top-of-stack used polygon */
} PolyData;

/* polygon processing */
int PolygonOverlap( PolyData polyData, const Polygon *p1, Polygon *p2, const int flagOP, int freeP2 );
void FreePolygons( PolyData polyData, Polygon *first, Polygon *last );
Polygon *SetPolygonHC( PolyData polyData, const int nVrt, const Vec2 *polyVrt, const double trns );
int GetPolygonVrt2D( const Polygon *pp, Vec2 *polyVrt );
int GetPolygonVrt3D( const Polygon *pp, Vec3 *srfVrt );
Polygon *GetPolygonHC( PolyData polyData );
PolygonVertexEdge *GetVrtEdgeHC( PolyData polyData);
void NewPolygonStack( PolyData polyData );
Polygon *TopOfPolygonStack( PolyData polyData );
V3D_API PolyData InitPolygonMem( const double epsDist, const double epsArea );
V3D_API void FreePolygonMem(PolyData polyData);
int LimitPolygon( int nVrt, Vec2 polyVrt[],
  const double maxX, const double minX, const double maxY, const double minY );
void DumpHC( char *title, const Polygon *pfp, const Polygon *plp );
void DumpFreePolygons( PolyData polyData );
void DumpFreeVertices( void );
void DumpP2D( char *title, const int nvs, Vec2 *vs );
void DumpP3D( char *title, const int nvs, Vec3 *vs );

#endif

