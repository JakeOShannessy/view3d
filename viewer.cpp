/**@FILE
	Load and parse View3D data file, then render the 3D geometry using OpenGL 
	in the COIN3D Examiner-Viewer.
*/

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/actions/SoGLRenderAction.h>

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
};

#include <sstream>
#include <stdexcept>
using namespace std;

const char *defaultsceneoutfile = "view3d.iv";

void usage(const char *progname){
	fprintf(stderr,"Usage: %s [-o [OUTFILE]] [-h] [-t] INFILE\n",progname);
	fprintf(stderr,"Load and parse a View3D file and render using Coin3D/OpenGL\n");
	fprintf(stderr,"  -h      Render using higher quality graphics (slower).\n");
	fprintf(stderr,"  INFILE  View3D .vs3 file to render (eg 'sample.vs3')\n");
	fprintf(stderr,"  -t      Include text in the rendered output.\n");
	fprintf(stderr,"  OUTFILE Open Inventor file to output (defaults to '%s')\n\n",defaultsceneoutfile);	
}

int main(int argc, char **argv){

	const char *sceneoutfile = NULL;
	bool highquality = false;
	bool infotext = false;
	
	char c;	
	while((c=getopt(argc,argv,"ho::t"))!=-1){
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
	View3DControlData vfCtrl; /* VF calculation control parameters - avoid globals */
	char title[LINELEN]; /* project title */
	vfCtrl.epsAdap = 1.0e-4f; // convergence for adaptive integration
	vfCtrl.maxRecursALI = 12; // maximum number of recursion levels
	vfCtrl.maxRecursion = 8;  // maximum number of recursion levels
	NxtOpen(filename, __FILE__, __LINE__ );
	CountVS3D(title, &vfCtrl );

	fprintf(stderr, "\nTitle: %s\n", title );
	fprintf(stderr, "Control values for 3-D view factor calculations:\n" );
	if(vfCtrl.enclosure)fprintf( stderr, "  Surfaces form enclosure.\n" );
	if(vfCtrl.emittances)fprintf( stderr, "  Will process emittances.\n" );

	if(vfCtrl.prjReverse)fprintf(stderr, "\n      Reverse projections. **" );

	fprintf(stderr, " Total number of surfaces: %d \n", vfCtrl.nAllSrf );
	fprintf(stderr, "   Heat transfer surfaces: %d \n", vfCtrl.nRadSrf );

#if 0
  char inFile[_MAX_PATH]=""; /* input file name */
  char outFile[_MAX_PATH]="";/* output file name */
  char **name;         /* surface names [1:nSrf][0:NAMELEN] */
  char *types[]={"rsrf","subs","mask","nuls","obso"};
  Vec3 *xyz;           /* vector of vertces [1:nVrt] - for ease in
                          converting V3MAIN to a subroutine */
  SRFDAT3D *srf;       /* vector of surface data structures [1:nSrf] */
  double **AF;         /* triangular array of area*view factor values [1:nSrf][] */
  float *area;         /* vector of surface areas [1:nSrf] */
  float *emit;         /* vector of surface emittances [1:nSrf] */
  int *base;           /* vector of base surface numbers [1:nSrf] */
  int *cmbn;           /* vector of combine surface numbers [1:nSrf] */
  float *vtmp;         /* temporary vector [1:nSrf] */
  int *possibleObstr;  /* list of possible view obstructing surfaces */
  struct tm *curtime;  /* time structure */
  time_t bintime;      /* seconds since 00:00:00 GMT, 1/1/70 */
  float time0, time1;  /* elapsed time values */
  int nSrf;            /* current number of surfaces */
  int nSrf0;           /* initial number of surfaces */
  int encl;            /* 1 = surfaces form enclosure */
  int n, flag;


  nSrf = nSrf0 = vfCtrl.nRadSrf;
  encl = vfCtrl.enclosure;

  if(vfCtrl.format == 4)vfCtrl.nVertices = 4 * vfCtrl.nAllSrf;

  name = Alc_MC(1, nSrf0, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
  area = Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  emit = Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );
  vtmp = Alc_V(1, nSrf0, sizeof(float), __FILE__, __LINE__ );

  for(n=nSrf0; n; n--)vtmp[n] = 1.0;

  base = Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  cmbn = Alc_V( 1, nSrf0, sizeof(int), __FILE__, __LINE__ );
  xyz = Alc_V( 1, vfCtrl.nVertices, sizeof(Vec3), __FILE__, __LINE__ );
  srf = Alc_V( 1, vfCtrl.nAllSrf, sizeof(SRFDAT3D), __FILE__, __LINE__ );
  InitTmpVertMem();  /* polygon operations in GetDat() and View3D() */
  InitPolygonMem(0, 0);

  /* read v/s data file */
  if(_list>2)_echo = 1;
  if(vfCtrl.format == 4)
    GetVS3Da(name, emit, base, cmbn, srf, xyz, &vfCtrl );
  else
    GetVS3D(name, emit, base, cmbn, srf, xyz, &vfCtrl );
  for(n=nSrf; n; n--)area[n] = (float)srf[n].area;
  NxtClose();

  if( _list>2 ){
    fprintf( _ulog, "Surfaces:\n" );
    fprintf( _ulog, "   #        name     area   emit  type bsn csn (dir cos) (centroid)\n" );
    for( n=1; n<=nSrf; n++ )
      fprintf( _ulog, "%4d %12s %9.2e %5.3f %4s %3d %3d (%g %g %g %g) (%g %g %g)\n",
        n, name[n], area[n], emit[n], types[srf[n].type], base[n], cmbn[n],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );
    for( ; n<=vfCtrl.nAllSrf; n++ )
      fprintf( _ulog, "%4d %12s %9.2e       %4s         (%g %g %g %g) (%g %g %g)\n",
        n, " ", area[n], types[srf[n].type],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );

    fprintf( _ulog, "Vertices:\n" );
    for( n=1; n<=vfCtrl.nAllSrf; n++ ){
      int j;
      fprintf( _ulog, "%4d ", n );
      for( j=0; j<srf[n].nv; j++ )
        fprintf( _ulog, " (%g %g %g)",
          srf[n].v[j]->x, srf[n].v[j]->y, srf[n].v[j]->z );
      fprintf( _ulog, "\n" );
    }
  }

#endif
#define FAILED_TO_READ_MODEL 1

	if(!FAILED_TO_READ_MODEL){
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
	
	// render axes?

#if 0
	// render all the nodes in the model
	Vector labeloffset(0.1,0.1,0.1);
	if(infotext){
		for(unsigned i=0; i<M->num_nodes; ++i){
			node_stmt *n;
			n = &(M->node[i]);
			stringstream ss;
			ss << n->id;
			Vector p = labeloffset + Vector(n->x,n->y,n->z);
			//fprintf(stderr,"Label = %s\n",ss.str().c_str());
			//cerr << "pos = " << p << endl;
			root->addChild(text(p,ss.str().c_str(),RED));
		}
	}

	// render all the members in the model
	for(unsigned i=0; i<M->num_membs; ++i){
		memb_stmt *m;
		m = &(M->memb[i]);
		node_stmt *A = &(M->node[m->fromnode]);
		node_stmt *B = &(M->node[m->tonode]);
		prop_stmt *p = model_find_prop(M, m->prop);

		Vector vA(A->x, A->y, A->z);
		Vector vB(B->x, B->y, B->z);

		stringstream ss;
		SbColor c;
		ss << m->id;
		if(p){
			const section *s = section_find(l, p->name);
			if(s){
				if(section_is_chs(s)){
					c = RED;
					double d = section_chs_outside_diameter(s) / 1000.; /* convert to metres */
					root->addChild(cylinder(vA,vB,d/2.,c));
				}
			}else{
				fprintf(stderr,"Warning: unknown section name '%s'\n",p->name);
				c = WHITE;
				root->addChild(cylinder(vA,vB,0.01,c));
			}
			ss << " (" << p->name << ")" << endl;
		}
		if(infotext){
			root->addChild(text(0.5*(vA+vB)+labeloffset, ss.str().c_str(), c));
		}
	}
#endif

	root->ref();

	if(sceneoutfile){
		// output the scene to a file that can be viewed later
		SoWriteAction wr;
		FILE *f;
		f = fopen(sceneoutfile,"w");
		if(!f) throw runtime_error("Unable to write to scene output file");
		wr.getOutput()->setFilePointer(f);
		fprintf(stderr,"Writing scene file to '%s'...", sceneoutfile);
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
