/*	VIEW3D OpenGL examiner/viewer
	Copyright (C) 2008 John Pye

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**@FILE
	Load and parse View3D data file, then render the 3D geometry using OpenGL
	in the COIN3D Examiner-Viewer.
	See http://www.coin3d.org/ as well as http://view3d.sf.net (this project)
*/

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoText2.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* prototype: strcpy */
#include <math.h>   /* prototype: sqrt */

extern "C"{
#include "types.h"
#include "view3d.h"
#include "prtyp.h"
#include "readvf.h"
#include "savevf.h"
#include "heap.h"
#include "misc.h"
#include "getdat.h"
#include "polygn.h"
};

#include "render.h"

#include <sstream>
#include <stdexcept>
#include <vector>
#include <map>
using namespace std;

const char *defaultsceneoutfile = "view3d.iv";

void usage(const char *progname){
	fprintf(stderr,
			"Usage: %s [-o [OUTFILE]] [-h] [-t] INFILE\n"
			"Load and parse a View3D file and render using Coin3D/OpenGL\n"
			"  -h      Render using higher quality graphics (slower).\n"
			"  INFILE  View3D .vs3 file to render (eg 'sample.vs3')\n"
			"  -t      Include text in the rendered output.\n"
			"  OUTFILE Open Inventor file to output (defaults to '%s')\n"
			"  -v      Display vertex labels.\n\n"
		, progname, defaultsceneoutfile
	);
}

int main(int argc, char **argv){

	const char *sceneoutfile = NULL;
	bool highquality = false;
	bool infotext = false;
	bool vertextext = false;
	
	char c;
	while((c=getopt(argc,argv,"ho::tv"))!=-1){
		switch(c){
			case 'h':
				highquality = 1;
				break;
			case 'o':
				sceneoutfile = (optarg ? optarg : defaultsceneoutfile);
				break;
			case 't':
				infotext = true;
				break;
			case 'v':
				vertextext = true;
				break;
			case '?':
				usage(argv[0]);
				exit(1);
		}
	}

	if(optind != argc-1){
		fprintf(stderr,"%s: missing command-line argument (need a filename)\n",argv[0]);
		usage(argv[0]);
		exit(2);
	}
	const char *filename = argv[optind];

	/* read Vertex/Surface data file */
	fprintf(stderr,"Reading View3D file '%s'...\n",filename);
	View3DControlData CD; /* VF calculation control parameters - avoid globals */
	char title[LINELEN]; /* project title */
	memset(&CD, 0, sizeof(View3DControlData));
	CD.epsAdap = 1.0e-4f; // convergence for adaptive integration
	CD.maxRecursALI = 12; // maximum number of recursion levels
	CD.maxRecursion = 8;  // maximum number of recursion levels
	NxtOpen(filename, __FILE__, __LINE__ );
	CountVS3D(title, &CD );

	fprintf(stderr, "\nTitle: %s\n", title );
	fprintf(stderr, "Control values for 3-D view factor calculations:\n");
	if(CD.enclosure)fprintf(stderr, "  Surfaces form enclosure.\n");
	if(CD.emittances)fprintf(stderr, "  Will process emittances.\n");

	if(CD.prjReverse)fprintf(stderr, "\n      Reverse projections. **");

	fprintf(stderr, " Total number of surfaces: %d \n", CD.nAllSrf );
	fprintf(stderr, "   Heat transfer surfaces: %d \n", CD.nRadSrf );

	int nSrf, nSrf0;
	nSrf = CD.nRadSrf;
	nSrf0 = CD.nRadSrf;

  	if(CD.format == 4)CD.nVertices = 4 * CD.nAllSrf;

	/* surface names [1:nSrf][0:NAMELEN] */
	char **name = (char **)Alc_MC(1, nSrf0, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );

	/* vector of surface areas [1:nSrf] */
	float *area = (float *)Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );

	/* vector of surface emittances [1:nSrf] */
	float *emitt = (float *)Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );

	/* temporary vector [1:nSrf] */
	float *vtmp = (float *)Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );

	for(int n=nSrf0; n; n--)vtmp[n] = 1.0;

	int *base = (int *)Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
	int *cmbn = (int *)Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
	Vec3 *xyz = (Vec3 *)Alc_V( 1, CD.nVertices, sizeof(Vec3), __FILE__, __LINE__ );
	SRFDAT3D *srf = (SRFDAT3D *)Alc_V( 1, CD.nAllSrf, sizeof(SRFDAT3D), __FILE__, __LINE__ );
	InitTmpVertMem();  /* polygon operations in GetDat() and View3D() */
	InitPolygonMem(0, 0);

	/* read v/s data file */
	if(_list>2)_echo = 1;
	if(CD.format == 4){
		GetVS3Da(name, emitt, base, cmbn, srf, xyz, &CD);
	}else{
		GetVS3D(name, emitt, base, cmbn, srf, xyz, &CD);
	}
	for(int n=nSrf; n; n--)area[n] = (float)srf[n].area;
	NxtClose();

#if 0
	const char *types[]={"rsrf","subs","mask","nuls","obso"};

    fprintf(stderr, "Surfaces:\n" );
    fprintf( stderr, "   #        name     area   emit  type bsn csn (dir cos) (centroid)\n" );
	int n;
    for(n=1; n<=nSrf; n++ )
      fprintf( stderr, "%4d %12s %9.2e %5.3f %4s %3d %3d (%g %g %g %g) (%g %g %g)\n",
        n, name[n], area[n], emitt[n], types[srf[n].type], base[n], cmbn[n],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );
    for(; n<=CD.nAllSrf; n++ )
      fprintf( stderr, "%4d %12s %9.2e       %4s         (%g %g %g %g) (%g %g %g)\n",
        n, " ", area[n], types[srf[n].type],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );

    fprintf( stderr, "Vertices:\n" );
    for( n=1; n<=CD.nAllSrf; n++ ){
      int j;
      fprintf( stderr, "%4d ", n );
      for( j=0; j<srf[n].nv; j++ )
        fprintf( stderr, " (%g %g %g)",
          srf[n].v[j]->x, srf[n].v[j]->y, srf[n].v[j]->z );
      fprintf( stderr, "\n" );
    }
#endif

#define FAILED_TO_READ_MODEL 0
	/* FIXME how to determine if there was a problem reading the input file? */
	if(FAILED_TO_READ_MODEL){
		fprintf(stderr,"Failed to read View3D model file!\n");
		exit(3);
	}

	QWidget *mainwin = NULL;
	if(sceneoutfile){
		SoDB::init();
	}else{
		 mainwin = SoQt::init(argc, argv, argv[0]);
	}

	SoSeparator *root = new SoSeparator;
	root->ref();

	// FIXME: add materials info
	SoMaterial *mat = new SoMaterial;
	mat->diffuseColor.setValue(1,1,0);
	mat->transparency = 0.5;
	root->addChild(mat);

	// vertices
	SoCoordinate3 *coo = new SoCoordinate3;
	SbVec3f *vcoo = new SbVec3f[CD.nVertices];
	for(int i=0; i<CD.nVertices; ++i){
		SbVec3f P(xyz[i+1].x, xyz[i+1].y, xyz[i+1].z);
		vcoo[i] = P;
	}
	coo->point.setValues(0, CD.nVertices, vcoo);
	root->addChild(coo);

	fprintf(stderr, "Created %d vertices.\n",CD.nVertices);

#if 0
	// surface normals
	SoNormal *nrm = new SoNormal;
	SbVec3f *vnrm = new SbVec3f[CD.nRadSrf];
	for(int i=0; i<CD.nRadSrf; ++i){
		SbVec3f N(srf[i].dc.x, srf[i].dc.y, srf[i].dc.z);
		vnrm[i] = N;
	}
	nrm->vector.setValues(0,CD.nRadSrf,vnrm);

	root->addChild(nrm);
	SoNormalBinding *nbi = new SoNormalBinding;
	nbi->value = SoNormalBinding::PER_FACE;
	root->addChild(nbi);
#endif

	// indices of the vertices for each face
	vector<int32_t> ids;
	map<int,SbVec3f> sfclabels;
	long nvals = 0;
    for(int i=1; i<=CD.nAllSrf; i++){
		SbVec3f L(0,0,0);
		for(int j=0; j<srf[i].nv; j++){
			long id = (long)(srf[i].v[j] - xyz);
			ids.push_back(id-1);
			//fprintf(stderr,"srf %d: vert %ld = %f, %f, %f (=? %f, %f, %f)\n",i,id,xyz[id].x, xyz[id].y, xyz[id].z, vcoo[id][0], vcoo[id][1], vcoo[id][2]);
			SbVec3f P(xyz[id].x, xyz[id].y, xyz[id].z);
			L = L + P;
		}
		L = float(1./srf[i].nv) * L;
		sfclabels[i] = L;
		ids.push_back(SO_END_FACE_INDEX);
		nvals += srf[i].nv + 1;
    }
	fprintf(stderr,"Created %d faces.\n",CD.nAllSrf);

	SoIndexedFaceSet *fset = new SoIndexedFaceSet;
	fset->coordIndex.setValues(0, nvals, &(ids[0]));
	root->addChild(fset);

	if(infotext){
		// add the surface labels
		fprintf(stderr,"OUTPUT SURFACE LABELS\n");
		SoMaterial *textcolor = new SoMaterial;
		textcolor->diffuseColor.setValue(1,0,0);
		root->addChild(textcolor);

		// label the faces, and give their emissivities
		for(map<int,SbVec3f>::const_iterator mi=sfclabels.begin(); mi!=sfclabels.end(); ++mi){
			stringstream ss;
			ss << name[mi->first] << " (e=" << emitt[mi->first] << ")";
			root->addChild(text(mi->second,ss.str().c_str(),RED));
		}
	}
	
	if(vertextext){
		// label the vertices
		for(int i=0; i<CD.nVertices; ++i){
			SbVec3f P(xyz[i+1].x, xyz[i+1].y, xyz[i+1].z);
			stringstream ss;
			ss << (i+1) << endl;
			root->addChild(text(P,ss.str().c_str(),CYAN));
		}
	}
		
	// add axes
	//root->addChild(axes(0.1));

	if(sceneoutfile){
		// output the scene to a file that can be viewed later
		fprintf(stderr,"Writing scene to file '%s'...\n",sceneoutfile);
		SoWriteAction wr;
		FILE *f;
		f = fopen(sceneoutfile,"w");
		if(!f) throw runtime_error("Unable to write to scene output file");
		wr.getOutput()->setFilePointer(f);
		//fprintf(stderr,"Starting write...");
		wr.apply(root);
		fprintf(stderr,"done!\n");
	}else{
		// or else render the scene directly to the screen
		fprintf(stderr,"Rendering scene to OpenGL directly...\n");

		// Use one of the convenient SoQt viewer classes.
		SoQtExaminerViewer *eviewer = new SoQtExaminerViewer(mainwin);
		eviewer->getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
		eviewer->setSceneGraph(root);
		eviewer->show();

		// Pop up the main window.
		SoQt::show(mainwin);
		// Loop until exit.
		SoQt::mainLoop();

		// Clean up resources.
		delete eviewer;
	}
	root->unref();


}
