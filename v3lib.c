/* v3lib.c  ********************************************************/

/*
This file is licensed as per the original View3D license, as included below.

License:

The code is public domain paid for by US taxpayers.

This program is furnished by the government and is accepted by
any recipient with the express understanding that the United
States Government makes no warranty, expressed or implied,
concerning the accuracy, completeness, reliability, usability,
or suitability for any particular purpose of the information
and data contained in this program or furnished in connection
therewith, and the United States shall be under no liability
whatsoever to any person by reason of any use made thereof.
This program belongs to the government.  Therefore, the
recipient further agrees not to assert any proprietary rights
therein or to represent this program to anyone as other than
a government program.
*/

/*  External library, everything but main.  */

#ifdef _DEBUG
# define DEBUG 1
#else
# define DEBUG 0
#endif

// This is part of the library, so therefore it needs to #define V3D_BUILD to
// ensure things are exported, not imported.
#define V3D_BUILD

#include <stdio.h>
#include <string.h> /* prototype: strcpy */
#include <stdlib.h> /* prototype: exit */
#include <math.h>   /* prototype: sqrt */
#include <time.h>   /* prototypes: time, localtime, asctime;
                       define: tm, time_t */

#include "types.h"
#include "view3d.h"
#include "misc.h"
#include "readvf.h"
#include "savevf.h"
#include "heap.h"
#include "polygn.h"
#include "viewpp.h"
#include "getdat.h"
#include "test3d.h"
#include "common.h"

/* forward decls */

void FindFile(char *msg, char *name, char *type);
void CheckFileWritable(char *fileName);
void CheckFileReadable(char *fileName);
double VolPrism(Vec3 *a, Vec3 *b, Vec3 *c);
VFResultsC processPaths(char *inFile, char *outFile);

void ReportAF( const int nSrf, const int encl, const char *title, char **name,
  const float *area, const float *emit, const int *base, double **AF, int flag
);

InData readFileHandle(FILE *inHandle);
RawInData readFileHandleRaw(FILE *inHandle);

InData InDataFromRaw(RawInData *rawInData);

// API
extern InData parseIn(FILE *file);
extern VFResultsC calculateVFs(InData inData);
// TODO: need to work out some things about the results format before that
// can be completed.
// extern void printVFs(int format, FILE *file);


InData parseIn(FILE *file) {
    return readFileHandle(file);
}

// void printVFs(int format, FILE *file, InData inData, VFResultsC results) {
//   return SaveVFNew(file, inData.vfCtrl.outFormat, inData.vfCtrl.enclosure,
//           inData.vfCtrl.emittances, results.nSrf, inData.area, inData.emit,
//           results.AF, inData.vtmp );
// }
RawInData readFileHandleRaw(FILE *inHandle) {
  char **name;         /* surface names [1:nSrf][0:NAMELEN] */
  float *area;         /* vector of surface areas [1:nSrf] */
  float *emit;         /* vector of surface emittances [1:nSrf] */
  float *vtmp;         /* temporary vector [1:nSrf] */
  int *base;           /* vector of base surface numbers [1:nSrf] */
  int *cmbn;           /* vector of combine surface numbers [1:nSrf] */
  Vec3 *xyz;           /* vector of vertces [1:nVrt] - for ease in
  converting V3MAIN to a subroutine */
  SRFDAT3D *srf;       /* vector of surface data structures [1:nSrf] */
  // vfCtrl contains control information and information on the numbers of
  // vertices etc.
  View3DControlData vfCtrl;
  char title[LINELEN]; /* project title */

  RawInData rawInData;

  int nSrf;            /* current number of surfaces */
  int nSrf0;           /* initial number of surfaces */
  int encl;            /* 1 = surfaces form enclosure */
  int n, flag;

  /* initialize control data */
//   memset( &rawInData.opts, 0, sizeof(RawInOptions) );
  // non-zero control values:
  vfCtrl.epsAdap = 1.0e-4f; // convergence for adaptive integration
  vfCtrl.maxRecursALI = 12; // maximum number of recursion levels
  vfCtrl.maxRecursion = 8;  // maximum number of recursion levels

  /* read Vertex/Surface data file */
  // FILE *inHandle = NxtOpenHndl(inFile, __FILE__, __LINE__ );
  // _unxt = inHandle;
  // Read the file initially to determine the size number of components (so that
  // we can allocate memory). This double-read may not make sense in a
  // javascript context.
  CountVS3D(inHandle, title, &vfCtrl );
  // Copy vfCtrl data to opts
  fprintf(stderr, "##readFileHandleRaw##epsAdap: %f\n", vfCtrl.epsAdap);
  rawInData.opts.epsAdap = vfCtrl.epsAdap;
  rawInData.opts.enclosure = vfCtrl.enclosure;
  rawInData.opts.emittances = vfCtrl.emittances;
  rawInData.opts.maxRecursALI = vfCtrl.maxRecursALI;
  rawInData.opts.minRecursion = vfCtrl.minRecursion;
  rawInData.opts.maxRecursion = vfCtrl.maxRecursion;
  rawInData.opts.col = vfCtrl.col;
  rawInData.opts.row = vfCtrl.row;
  rawInData.opts.prjReverse = vfCtrl.prjReverse;
  // TODO: allocate memory and copy title string.
  // inData.title = title;
#ifdef LOGGING
  fprintf(_ulog, "\nTitle: %s\n", title );

  fprintf(_ulog, "Control values for 3-D view factor calculations:\n" );
  if(rawInData.opts.enclosure)fprintf( _ulog, "  Surfaces form enclosure.\n" );
  if(rawInData.opts.emittances)fprintf( _ulog, "  Will process emittances.\n" );

  fprintf(_ulog, "     Adaptive convergence: %g", rawInData.opts.epsAdap );
  if(rawInData.opts.epsAdap != 1.e-4f )fprintf( _ulog, " *" );

  fprintf(_ulog, "\n  Unobstructed recursions: %d", rawInData.opts.maxRecursALI );
  if(rawInData.opts.maxRecursALI != 12)fprintf( _ulog, " *" );

  fprintf(_ulog, "\nMax obstructed recursions: %d", rawInData.opts.maxRecursion );
  if(rawInData.opts.maxRecursion != 8)fprintf( _ulog, " *" );

  fprintf(_ulog, "\nMin obstructed recursions: %d", rawInData.opts.minRecursion );
  if(rawInData.opts.minRecursion)fprintf( _ulog, " *" );

  fprintf(_ulog, "\n              Solving row:" );
  if(rawInData.opts.row)fprintf( _ulog, " %d *", rawInData.opts.row );
  else fprintf(_ulog, " all" );

  fprintf(_ulog, "\n           Solving column:" );
  if(rawInData.opts.col)fprintf(_ulog, " %d *", rawInData.opts.col );
  else fprintf(_ulog, " all" );

  if(rawInData.opts.prjReverse)fprintf(_ulog, "\n      Reverse projections. **" );

  fprintf(_ulog, "\n Output control parameter: %d\n", _list );

  fprintf(_ulog, "\n" );
  fprintf(_ulog, " Total number of surfaces: %d \n", rawInData.nAllSrf );
  fprintf(_ulog, "   Heat transfer surfaces: %d \n", rawInData.nRadSrf );
#endif

  nSrf = nSrf0 = rawInData.nRadSrf;
  encl = rawInData.opts.enclosure;

  GetVS3DNew(inHandle, &rawInData);
  
  // NxtClose();
  return rawInData;
}
// Read the file into a data structure, heap allocate it and return a pointer to
// it. There is no point allocating it before this as we don't know the size of
// the data.
InData readFileHandle(FILE *inHandle) {
  char **name;         /* surface names [1:nSrf][0:NAMELEN] */
  float *area;         /* vector of surface areas [1:nSrf] */
  float *emit;         /* vector of surface emittances [1:nSrf] */
  float *vtmp;         /* temporary vector [1:nSrf] */
  int *base;           /* vector of base surface numbers [1:nSrf] */
  int *cmbn;           /* vector of combine surface numbers [1:nSrf] */
  Vec3 *xyz;           /* vector of vertces [1:nVrt] - for ease in
  converting V3MAIN to a subroutine */
  SRFDAT3D *srf;       /* vector of surface data structures [1:nSrf] */
  // vfCtrl contains control information and information on the numbers of
  // vertices etc.
  View3DControlData vfCtrl;
  char title[LINELEN]; /* project title */

  InData inData;

  int nSrf;            /* current number of surfaces */
  int nSrf0;           /* initial number of surfaces */
  int encl;            /* 1 = surfaces form enclosure */
  int n, flag;

  /* initialize control data */
  memset( &vfCtrl, 0, sizeof(View3DControlData) );
  // non-zero control values:
  vfCtrl.epsAdap = 1.0e-4f; // convergence for adaptive integration
  vfCtrl.maxRecursALI = 12; // maximum number of recursion levels
  vfCtrl.maxRecursion = 8;  // maximum number of recursion levels

  /* read Vertex/Surface data file */
  // FILE *inHandle = NxtOpenHndl(inFile, __FILE__, __LINE__ );
  // _unxt = inHandle;
  // Read the file initially to determine the size number of components (so that
  // we can allocate memory). This double-read may not make sense in a
  // javascript context.
  CountVS3D(inHandle, title, &vfCtrl );
  // TODO: allocate memory and copy title string.
  // inData.title = title;
#ifdef LOGGING
  fprintf(_ulog, "\nTitle: %s\n", title );

  fprintf(_ulog, "Control values for 3-D view factor calculations:\n" );
  if(vfCtrl.enclosure)fprintf( _ulog, "  Surfaces form enclosure.\n" );
  if(vfCtrl.emittances)fprintf( _ulog, "  Will process emittances.\n" );

  fprintf(_ulog, "     Adaptive convergence: %g", vfCtrl.epsAdap );
  if(vfCtrl.epsAdap != 1.e-4f )fprintf( _ulog, " *" );

  fprintf(_ulog, "\n  Unobstructed recursions: %d", vfCtrl.maxRecursALI );
  if(vfCtrl.maxRecursALI != 12)fprintf( _ulog, " *" );

  fprintf(_ulog, "\nMax obstructed recursions: %d", vfCtrl.maxRecursion );
  if(vfCtrl.maxRecursion != 8)fprintf( _ulog, " *" );

  fprintf(_ulog, "\nMin obstructed recursions: %d", vfCtrl.minRecursion );
  if(vfCtrl.minRecursion)fprintf( _ulog, " *" );

  fprintf(_ulog, "\n              Solving row:" );
  if(vfCtrl.row)fprintf( _ulog, " %d *", vfCtrl.row );
  else fprintf(_ulog, " all" );

  fprintf(_ulog, "\n           Solving column:" );
  if(vfCtrl.col)fprintf(_ulog, " %d *", vfCtrl.col );
  else fprintf(_ulog, " all" );

  if(vfCtrl.prjReverse)fprintf(_ulog, "\n      Reverse projections. **" );

  fprintf(_ulog, "\n Output control parameter: %d\n", _list );

  fprintf(_ulog, "\n" );
  fprintf(_ulog, " Total number of surfaces: %d \n", vfCtrl.nAllSrf );
  fprintf(_ulog, "   Heat transfer surfaces: %d \n", vfCtrl.nRadSrf );
#endif

  nSrf = nSrf0 = vfCtrl.nRadSrf;
  encl = vfCtrl.enclosure;

  if(vfCtrl.format == 4)vfCtrl.nVertices = 4 * vfCtrl.nAllSrf;
  // Allocate memory for all of the surfaces. These will need to be resized as
  // the data is read in.
  inData.name = Alc_MC(1, nSrf0, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
  inData.area = Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  inData.emit = Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  inData.vtmp = Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );

  for(n=nSrf0; n; n--)(inData.vtmp)[n] = 1.0;

  inData.base = Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  inData.cmbn = Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  inData.xyz = Alc_V( 1, vfCtrl.nVertices, sizeof(Vec3), __FILE__, __LINE__ );
  inData.srf = Alc_V( 1, vfCtrl.nAllSrf, sizeof(SRFDAT3D), __FILE__, __LINE__ );
  InitTmpVertMem();  /* polygon operations in GetDat() and View3D() */
  InitPolygonMem(0, 0);

  // reads the  file a second time
  /* read v/s data file */
  if(_list>2)
  _echo = 1;
  if(vfCtrl.format == 4){
    GetVS3Da(inHandle, inData.name, inData.emit, inData.base, inData.cmbn, inData.srf, inData.xyz, &vfCtrl );
  }
  else {
    GetVS3D(inHandle, inData.name, inData.emit, inData.base, inData.cmbn, inData.srf, inData.xyz, &vfCtrl );
  }
  for( n=nSrf; n; n-- )
  (inData.area)[n] = (float)(inData.srf)[n].area;
  NxtClose();
  inData.vfCtrl = vfCtrl;
  return inData;
}

void printRawInData(RawInData *rawInData) {
  // for (int i 0; i < 5; i++) 
  fprintf(stderr, "printing rawInData\n");
  fprintf(stderr, "s1 name: %s\n", rawInData->surfaces[1].name);
  fprintf(stderr, "s1 v2: %d\n", rawInData->surfaces[1].vertexIndices[1]);
  fprintf(stderr, "nVertices: %d\n", rawInData->nVertices);
}


void printInData(InData *inData) {
  // for (int i 0; i < 5; i++) 
  // View3DControlData vfCtrl;
  // char **name;
  // float *area;
  // float *emit;
  // float *vtmp;
  // char title[LINELEN]; /* project title */
  // int *base;
  // int *cmbn;
  // Vec3 *xyz;
  // SRFDAT3D *srf;


  fprintf(stderr, "printing InData\n");
  fprintf(stderr, "vftrl:\n");
  fprintf(stderr, "\tnAllSrf: %d\n", inData->vfCtrl.nAllSrf);
  fprintf(stderr, "\tnRadSrf: %d\n", inData->vfCtrl.nRadSrf);
  fprintf(stderr, "\tnMaskSrf: %d\n", inData->vfCtrl.nMaskSrf);
  fprintf(stderr, "\tnObstrSrf: %d\n", inData->vfCtrl.nObstrSrf);
  fprintf(stderr, "\tnVertices: %d\n", inData->vfCtrl.nVertices);
  fprintf(stderr, "\tformat: %d\n", inData->vfCtrl.format);
  fprintf(stderr, "\toutFormat: %d\n", inData->vfCtrl.outFormat);
  fprintf(stderr, "\trow: %d\n", inData->vfCtrl.row);
  fprintf(stderr, "\tcol: %d\n", inData->vfCtrl.col);
  fprintf(stderr, "\tenclosure: %d\n", inData->vfCtrl.enclosure);
  fprintf(stderr, "\temittances: %d\n", inData->vfCtrl.emittances);
  fprintf(stderr, "\tnPossObstr: %d\n", inData->vfCtrl.nPossObstr);
  fprintf(stderr, "\tnProbObstr: %d\n", inData->vfCtrl.nProbObstr);
  fprintf(stderr, "\tprjReverse: %d\n", inData->vfCtrl.prjReverse);
  fprintf(stderr, "\tepsAdap: %f\n", inData->vfCtrl.epsAdap);
  fprintf(stderr, "\trcRatio: %f\n", inData->vfCtrl.rcRatio);
  fprintf(stderr, "\trelSep: %f\n", inData->vfCtrl.relSep);
  fprintf(stderr, "\tmethod: %d\n", inData->vfCtrl.method);
  fprintf(stderr, "\tnEdgeDiv: %d\n", inData->vfCtrl.nEdgeDiv);
  fprintf(stderr, "\tmaxRecursALI: %d\n", inData->vfCtrl.maxRecursALI);
  // unsigned long usedV1LIadapt; /* number of V1LIadapt() calculations used */
  // int failViewALI;   /* 1 = unobstructed view factor did not converge */
  fprintf(stderr, "\tminRecursion: %d\n", inData->vfCtrl.minRecursion);
  fprintf(stderr, "\tmaxRecursion: %d\n", inData->vfCtrl.maxRecursion);
  // int failRecursion; /* 1 = obstructed view factor did not converge */
  // double epsAF;         /* convergence for current AF calculation */
  // unsigned long wastedVObs;    /* number of ViewObstructed() calculations wasted */
  // unsigned long usedVObs;      /* number of ViewObstructed() calculations used */
  // unsigned long totPoly;       /* total number of polygon view factors */
  // unsigned long totVpt;        /* total number of view points */
  // int failConverge;  /* 1 if any calculation failed to converge */
  // SRFDAT3X srf1T;   /* participating surface; transformed coordinates */
  // SRFDAT3X srf2T;   /* participating surface; transformed coordinates;
  //                      view from srf1T toward srf2T. */ 
  // SRFDAT3X *srfOT;  /* pointer to array of view obstrucing surfaces;
  //                      dimensioned from 0 to maxSrfT in View3d();
  //                      coordinates transformed relative to srf2T. */
  for (int i = 1; i <= inData->vfCtrl.nRadSrf; i++) {
    fprintf(stderr, "surface %d: %s\n", i, inData->name[i]);
    fprintf(stderr, "\tnr: %d\n", inData->srf[i].nr);
    fprintf(stderr, "\tnv: %d\n", inData->srf[i].nv);
    fprintf(stderr, "\tshape: %d\n", inData->srf[i].shape);
    fprintf(stderr, "\ttype: %d\n", inData->srf[i].type);
    fprintf(stderr, "\tarea: %f %f\n", inData->srf[i].area, inData->area[i]);
    fprintf(stderr, "\temit: %f\n", inData->emit[i]);
    fprintf(stderr, "\tbase: %d\n", inData->base[i]);
    fprintf(stderr, "\tcmbn: %d\n", inData->cmbn[i]);
    fprintf(stderr, "\trc: %f\n", inData->srf[i].rc);
    // fprintf(stderr, "\tdc: %d\n", i, inData->srf[i].dc);
    fprintf(stderr, "\tctd: (%f,%f,%f)\n", inData->srf[i].ctd.x, inData->srf[i].ctd.y, inData->srf[i].ctd.z);
    fprintf(stderr, "\tvertices:\n");
    for (int n = 0; n < inData->srf[i].nv; n++) {
      fprintf(stderr, "\t\tv[%d]: (%f,%f,%f)\n", n, inData->srf[i].v[n]->x, inData->srf[i].v[n]->y, inData->srf[i].v[n]->z);
    }
    // fprintf(stderr, "\tctd:(%f,%f,%f)\n", i, inData->srf[i].ctd.x, inData->srf[i].ctd.y, inData->srf[i].ctd.z);

  // DirCos dc;          /* direction cosines of surface normal */
  // int NrelS;           /* orientation of srf N relative to S:
  //                        -1: N behind S; +1: N in front of S;
  //                         0: part of N behind S, part in front */
  // int MrelS;           /* orientation of srf M relative to S */
  }
}

InData readFilePath(char *inFile) {
  FILE *inHandle = NxtOpenHndl(inFile, __FILE__, __LINE__ );
  _unxt = inHandle;
  return readFileHandle(inHandle);
}

double getEnclosureVolume(View3DControlData vfCtrl, SRFDAT3D *srf) {
    /* determine volume of enclosure */
    double volume=0.0;
    // Loop through each of the surfaces.
    for(int n=vfCtrl.nAllSrf; n; n-- ) {
      // If it is a subsurface (SUBS) we skip it.
      if( srf[n].type == SUBS ) continue;
      volume += VolPrism( srf[n].v[0], srf[n].v[1], srf[n].v[2] );
      if( srf[n].nv == 4 )
        volume += VolPrism( srf[n].v[2], srf[n].v[3], srf[n].v[0] );
    }
    volume /= -6.0;        /* see VolPrism() */
    return volume;
  }

VFResultsC processHandlesSimple(FILE *inHandle, FILE *outHandle) {
#define NEW_READ
#ifdef NEW_READ
  RawInData rawInData = readFileHandleRaw(inHandle);
  // printRawInData(&rawInData);
  InData inData = InDataFromRaw(&rawInData);
#else
  InData inData = parseIn(inHandle);
#endif
  printInData(&inData);
  return calculateVFs(inData);
}
/*----------------------------------------------------------------------------*/
/* This is modified to be the simplest possible (and with little logging). */
VFResultsC calculateVFs(InData inData){
  _ulog = stderr;
  double **AF;         /* triangular array of area*view factor values [1:nSrf][] */
  int *possibleObstr;  /* list of possible view obstructing surfaces */

  int encl = inData.vfCtrl.enclosure; /* 1 = surfaces form enclosure */
  int nSrf0 = inData.vfCtrl.nRadSrf; /* initial number of surfaces */
  int nSrf = nSrf0;            /* current number of surfaces */
  View3DControlData vfCtrl = inData.vfCtrl; /* VF calculation control parameters - avoid globals */
  char **name = inData.name;
  float *area = inData.area; /* the areas of each surface */
  float *emit = inData.emit;  /* vector of surface emittances [1:nSrf] */
  float *vtmp = inData.vtmp;/* temporary vector [1:nSrf] */
  int *base = inData.base;/* vector of base surface numbers [1:nSrf] */
  int *cmbn = inData.cmbn;/* vector of combine surface numbers [1:nSrf] */
  Vec3 *xyz = inData.xyz; /* vector of vertces [1:nVrt] */
  SRFDAT3D *srf = inData.srf; /* the surface data */

  // Allocate some space to store the list of possible obstructions. This array
  // will never be longer than all the surfaces, so make it that length.
  possibleObstr = Alc_V( 1, vfCtrl.nAllSrf, sizeof(int), __FILE__, __LINE__ );
  // Find the number of possible obstructing surfaces. The index list of these
  // surfaces is stored in possibleObstr.
  vfCtrl.nPossObstr = SetPosObstr3D( vfCtrl.nAllSrf, srf, possibleObstr );

  // If row is specified (i.e. we are only interested in the view factors of
  // one surface) then we allocate an array big enough for those values.
  if( vfCtrl.row ){  // may not work with some compilers. GNW 2008/01/22
    AF = Alc_MC( vfCtrl.row, vfCtrl.row, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
  // Otherwise we want every surface to every surface and must allocate a
  // sufficiently sized array.
  } else {
#ifdef __TURBOC__
    AF = Alc_MSR( 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#else
    AF = Alc_MSC( 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#endif
#ifdef LOGGING
    fprintf( _ulog, "\nComputing view factors for all %d surfaces\n\n", nSrf0 );
#endif
  }

  /*----- view factor calculation -----*/
  View3D( srf, base, possibleObstr, AF, &vfCtrl );
  // The view factors have now been calculated and stored in AF.

  // The possibly obstruction surface information is no longer needed after
  // this point.
  Fre_V( possibleObstr, 1, vfCtrl.nAllSrf, sizeof(int), __FILE__, __LINE__ );
  FreeTmpVertMem();  /* free polygon overlap vertices */
  FreePolygonMem();
  Fre_V( xyz, 1, vfCtrl.nVertices, sizeof(Vec3), __FILE__, __LINE__ );

  // TODO: what does this do
  for(int n = nSrf; n; n-- ){  /* clear base pointers to OBSO & MASK srfs */
	// FIXME should the following line say 'srf[n]'??
	if( srf[base[n]].type == OBSO )  /* Base is used for several things. */
      base[n] = 0;                   /* It must be progressively cleared */

	// FIXME should the following line say 'base[n]'??
    if( srf[n].type == MASK )        /* as each use is completed. */
      base[n] = 0;
  }

  // Here we being adusting the view factors

  // Determine if any of the surfaces are NULS
  for(int n = nSrf; n; n-- ) {
    // If any surface has the type NULS, run the DelNull procedure to remove
    // them.
    if( srf[n].type==NULS ) {
      // This will trigger once at least one such surface is found, DelNull
      // is then applied to the whole geometry.
      nSrf = DelNull( nSrf, srf, base, cmbn, emit, area, name, AF );
      // And we can break from the loop.
      break;
    }
  }

  // TODO: work out what this does
  for(int n = nSrf; n; n-- ) {
    if( base[n]>0 ) {       /* separate subsurfaces */
      Separate( nSrf, base, area, AF );
      for(int i = nSrf; i; i-- ) {
        base[i] = 0;
      }
      // Once we have found a single instance, the Separate procedure is applied
      // to the whole geometry, so we can stop looping.
      break;
    }
  }

  for(int n = nSrf; n; n-- ) {
    if (cmbn[n] > 0) {                         /* combine surfaces */
      nSrf = Combine( nSrf, cmbn, area, name, AF );
      // Once we have found a single instance, the Separate procedure is applied
      // to the whole geometry, so we can stop looping.
      break;
    }
  }

  // If the geometry is an enclosure, we know that the sum of the view factors
  // from a particular surface to all other surfaces equals 1. We can use this
  // fact to normalise and adjust the view factors.
  if( encl ) {                         /* normalize view factors */
    NormAF( nSrf, vtmp, area, AF, 1.0e-7f, 100 );
  }

  // This is where the emissivity values are applied, if that configuration
  // option is selected. Note that this produces a different value (not the
  // view factor) so it needs to be very clear.
  if( vfCtrl.emittances ){ // Process surface emissivities
    // Compute the total radiation interchange factors. This modifies the
    // values in AF by applying the emssivity values.
#ifdef LOGGING
    ReportAF( nSrf, encl, "Before IntFac", name, area, vtmp, base, AF, 0 );
#endif
    IntFac( nSrf, emit, area, AF );
#ifdef LOGGING
    ReportAF( nSrf, encl, "After IntFac", name, area, vtmp, base, AF, 0 );
#endif
    if( encl ) // If it is an enclosure we want to normalise again
      NormAF( nSrf, emit, area, AF, 1.0e-7f, 30 );   /* fix rounding errors */
  }

  // The calculations are done at this point, and everything after here is
  // just marshalling the output values into a different format and freeing
  // memory.

  // These are some conversions to make the external interface simpler
  // Copy the values into single contigious array
  int ret_len = nSrf0*nSrf0;
  double *ret = malloc(sizeof(double)*ret_len);

  for(int n = 1; n <= nSrf; n++) {
    /* process AF values for row n */
    double Ainv = 1.0 / area[n];
    for(int m = 1; m <= nSrf; m++) {
      /* process column values */
      if(m < n){
        ret[(n-1)*nSrf+(m-1)] = (float)(AF[n][m] * Ainv);
      }else{
        ret[(n-1)*nSrf+(m-1)] = (float)(AF[m][n] * Ainv);
      }
    }
  }

  // zero-based array for areas
  float *areas0 = malloc(sizeof(float)*nSrf0);
  for (int n = 1; n <= nSrf; n++) {
    areas0[n-1] = area[n];
  }

  //zero-based array for emissivities
  float *emit0 = malloc(sizeof(float)*nSrf0);
  for (int n = 1; n <= nSrf; n++) {
    emit0[n-1] = emit[n];
  }

  VFResultsC res_struct;
  res_struct.n_surfs = nSrf0;
  res_struct.encl = encl;
  res_struct.area = areas0;
  res_struct.emit = emit0;
  res_struct.values = ret;

//   SaveVF( outHandle, "View3D", "3.5", vfCtrl.outFormat, vfCtrl.enclosure,
//           vfCtrl.emittances, nSrf, area, emit, AF, vtmp );

  /* Begin: Free memory of data structures */
  if( vfCtrl.row ){
    Fre_MC( AF, vfCtrl.row, vfCtrl.row, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
  }else{
#ifdef __TURBOC__
    Fre_MSR( (void **)AF, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#else
    Fre_MSC( (void **)AF, 1, nSrf0, sizeof(double), __FILE__, __LINE__ );
#endif
  }
  Fre_V( srf, 1, vfCtrl.nAllSrf, sizeof(SRFDAT3D), __FILE__, __LINE__ );
  Fre_V( cmbn, 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  Fre_V( base, 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  Fre_V( vtmp, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_V( emit, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_V( area, 1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  Fre_MC( (void **)name, 1, nSrf0, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
  /* End: Free memory of data structures */

  return res_struct;

} /* end of processHandlesSimple() */

VFResultsC processPaths(char *inFile, char *outFile) {
  FILE *inHandle = NxtOpenHndl(inFile, __FILE__, __LINE__ );
  _unxt = inHandle;
  // Write the results to the output file.
  // TODO: if saving to binary format, open for binary write
  FILE *outHandle;
  if(strlen(outFile) == 0 || outFile == NULL) {
    outHandle = stdout;
  } else {
    outHandle = fopen(outFile, "w");
  }
  return processHandlesSimple(inHandle, outHandle);
}

int processStrings(char *inString, char *outFile) {
  // Windows does not support fmemopen, so write to a temporary file and open
  // that.
  char *tmpPath = "temp.txt";
  FILE *tmp = fopen(tmpPath, "w");
  fwrite(inString, 1, strlen(inString), tmp);
  fclose(tmp);

  // FILE *inHandle = fmemopen(inString, strlen(inString), "r");
  FILE *inHandle = NxtOpenHndl(tmpPath, __FILE__, __LINE__ );
  _unxt = inHandle;
  // Write the results to the output file.
  // TODO: if saving to binary format, open for binary write
  FILE *outHandle;
  if(strlen(outFile) == 0 || outFile == NULL) {
    outHandle = stdout;
  } else {
    outHandle = fopen(outFile, "w");
  }
  processHandlesSimple(inHandle, outHandle);
  return 0;
}


/**
	Compute 6 * volume of a prism defined by vertices a, b, c, and (0,0,0).

	Ref: E Kreyszig, _Advanced Engineering Mathematics_, 3rd ed, Wiley, 1972,
	pp 214,5.  Volume = A dot (B cross C) / 6; A = vector from 0 to a, ...;

	Uses the fact that Vec3 A = Vec3 a, ...; Sign of result depends
	on sequence (clockwise or counter-clockwise) of vertices.
*/
double VolPrism( Vec3 *a, Vec3 *b, Vec3 *c ){
  Vec3 bxc;

  VCROSS( b, c, (&bxc) );
  return VDOT( a, (&bxc) );
}


/**
	@TODO DOCUMENT THIS
*/
void ReportAF( const int nSrf, const int encl, const char *title, char **name,
  const float *area, const float *emit, const int *base, double **AF, int flag
){
  int n;           /* row */
  int m;           /* column */
  double err;      /* error values assuming enclosure */
  double F, sumF;  /* view factor, sum of F for row */
  double eMax=0.0; /* maximum row error, if enclosure */
  double eRMS=0.0; /* RMS row error, if enclosure */
#define MAXEL 10
  struct{
    double err;    /* row sumF error */
    int n;         /* row number */
  }elist[MAXEL+1];
  int i;

  fprintf( _ulog, "\n%s\n", title );
  if(encl && _list>0)fprintf( _ulog, "          #        name   SUMj Fij (encl err)\n" );
  memset( elist, 0, sizeof(elist) );

  for( n=1; n<=nSrf; n++ ){
	/* process AF values for row n */
    for(sumF=0.0,m=1; m<=n; m++){
      /* compute sum of view factors */
      if(base[m] == 0)sumF += AF[n][m];
    }
    for(; m<=nSrf; m++){
      if(base[m] == 0)sumF += AF[m][n];
    }
    sumF /= area[n];
    if(_list>0){
      fprintf( _ulog, " Row:  %4d %12s %9.6f", n, name[n], sumF );
      if(encl)fprintf( _ulog, " (%.6f)", fabs( sumF - emit[n] ) );
      fputc( '\n', _ulog );
    }

    if(encl){
      /* compute row sumF error value */
      err = fabs( sumF - emit[n]);
      eRMS += err * err;
      for( i=MAXEL; i>0; i-- ){
        if(err<=elist[i-1].err)break;
        elist[i].err = elist[i-1].err;
        elist[i].n = elist[i-1].n;
      }
      elist[i].err = err;
      elist[i].n = n;
    }

    if( _list>0 ){
      /* print row n values */
      double invArea = 1.0 / area[n];
      for( m=1; m<=nSrf; m++ ){
        char *s = _string;
        if( m>=n )
          F = AF[m][n] * invArea;
        else
          F = AF[n][m] * invArea;
        sprintf( _string, "%8.6f ", F );
        if( _string[0] == '0' ){
          s += 1;
          if( m%10==0 ) _string[8] = '\n';
        }else{
          sprintf( _string, "%7.5f ", F );  /* handle F = 1.0 */
          if( m%10==0 ) _string[7] = '\n';
        }
        fprintf( _ulog, "%s", s );
      }
      if( m%10!=1 ) fputc( '\n', _ulog );
    }
  }  /* end of row n */

  if( encl ){     /* print row sumF error summary */
    fprintf( _ulog, "Summary:\n" );
    eMax = elist[0].err;
    fprintf( _ulog, "Max row sumF error:  %.2e\n", eMax );
    eRMS = sqrt( eRMS/nSrf );
    fprintf( _ulog, "RMS row sumF error:  %.2e\n", eRMS );
    if( flag ){
      fprintf( stderr, "\nMax row sumF error:  %.2e\n", eMax );
      fprintf( stderr, "RMS row sumF error:  %.2e\n", eRMS );
    }
    if( elist[0].err>0.5e-6 ){
      fprintf( _ulog, "Largest errors [row, error]:\n" );
      for( i=0; i<MAXEL; i++ ){
        if( elist[i].err<0.5e-6 ) break;
        fprintf( _ulog, "%8d%10.6f\n", elist[i].n, elist[i].err );
      }
    }
    fprintf( _ulog, "\n" );
  }
} /* end of ReportAF() */


/**
	Find user designated file.  REPLACE WITH FILEOPEN.TXT ???
	First character of type string must be 'r', 'w', or 'a'.
	@param msg    message to user
	@param name   file name (string long enough for any file name)
	@param type   type of file, see fopen()
*/
void FindFile( char *msg, char *fileName, char *type ){

  FILE  *pfile=NULL;

  while(!pfile){
    if(fileName[0]){
      /* try to open file */
      pfile = fopen( fileName, type );
      if( pfile == NULL )
        fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
    }
    if(!pfile){        /* ask for file name */
      fprintf( stderr, "%s: ", msg );
      fflush(stderr);
      scanf( "%s", fileName );
    }
  }
  fclose( pfile );
} /* End of FindFile() */


// Currently does not open the file.
void CheckFileWritable(char *fileName){

  FILE  *pfile=NULL;

  if(fileName!=NULL && strlen(fileName)>0){
    /* try to open file */
    pfile = fopen( fileName, "w");
    if( pfile == NULL )
      fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
  }
  fclose( pfile );
}

// Currently does not open the file.
void CheckFileReadable(char *fileName){

  FILE  *pfile=NULL;

  if(fileName!=NULL && strlen(fileName)>0){
    /* try to open file */
    pfile = fopen( fileName, "r");
    if( pfile == NULL )
      fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
  }
  fclose( pfile );
}
