/*
	Viewer for the .vs2 files containing 2D vertex/surface geometry.
	
	This viewer is implemented using the Cairo 2D graphics library.

	Based on cairo-demo/gtk/hello.c by Øvyind Kolås.

	Only so far developed/tested on Windows, but should build/run fine on 
	Windows provided you have a recent version GTK+ (developer version)
	installed.
*/

#include "view2d.h"

#include <stdio.h>
#include <string.h> /* prototype: strcpy */
#include <stdlib.h> /* prototype: exit */
#include <math.h>   /* prototype: sqrt */
#include <unistd.h>
#include <limits.h>

#include "types.h" 
#include "misc.h"
#include "heap.h"
#include "test2d.h"
#include "viewpp.h"
#include "getdat.h"
#include "savevf.h"

#include <gtk/gtk.h>
#include <cairo.h>

#define DEFAULT_WIDTH  400
#define DEFAULT_HEIGHT 200

/* forward decls */
static void paint (GtkWidget *widget, GdkEventExpose *eev
	, gpointer data
);

void usage(const char *progname){
	fprintf(stderr,
			"Usage: %s [-o [OUTFILE]] [-h] [-t] INFILE\n"
			"Load and parse a View2D file and render using Cairo/GTK+\n"
			"  INFILE  View2D .vs2 file to render (eg 'facet.vs2')\n"
			"  -t      Include text in the rendered output.\n"
		, progname
	);	
}

typedef struct{
	float *emit;        /* vector of surface emittances [1:nsrf] */
	int *base;        /* vector of base surface numbers [1:nsrf] */
	int *cmbn;        /* vector of combine surface numbers [1:nsrf] */
	SRFDAT2D *srf;   /* vector of surface data structures [1:nsrf] */
	int nsrf;
	char **name;       /* surface names [1:nsrf][0:NAMELEN] */
} Scene2D;

/*------------------------------------------------------------------------------
	Main routine, sets up the window and links drawing methods to it
*/
int main (int argc, char **argv){
	short infotext = 0;
	
	char c;	
	while((c=getopt(argc,argv,"t"))!=-1){
		switch(c){
			case 't':
				infotext = 1;
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

	Scene2D S;

	char title[LINELEN];  /* project title */
	View2DControlData CD;   /* VF calculation control parameters */
	float *vtmp;        /* temporary vector [1:nsrf] */
	int encl;         /* 1 = surfaces form enclosure */
	int n;

	/* initialize control data */
	memset(&CD, 0, sizeof(View2DControlData) );

	/* read Vertex/Surface data file */
	NxtOpen(filename, __FILE__, __LINE__ );
	CountVS2D( title, &CD );
	fprintf(stderr, "Title: %s\n", title );
	fprintf(stderr, "Control values for 2-D view factor calculations:\n" );
	fprintf(stderr, "     enclosure designator: %3d \n", CD.enclosure );
	fprintf(stderr, " output control parameter: %3d \n", _list );
	fprintf(stderr, "   obstructed convergence: %g \n", CD.epsObstr );
	fprintf(stderr, "       maximum recursions: %3d \n", CD.maxRecursion );
	fprintf(stderr, "       minimum recursions: %3d \n", CD.minRecursion );
	fprintf(stderr, "       process emittances: %3d \n", CD.emittances );

	fprintf(stderr, "\n" );
	fprintf(stderr, " total number of surfaces: %3d \n", CD.nAllSrf );
	fprintf(stderr, "   heat transfer surfaces: %3d \n", CD.nRadSrf );

	S.nsrf = CD.nRadSrf;
	encl = CD.enclosure;
	S.name = Alc_MC( 1, S.nsrf, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );
	S.emit = Alc_V( 1, S.nsrf, sizeof(float), __FILE__, __LINE__ );
	vtmp = Alc_V( 1, S.nsrf, sizeof(float), __FILE__, __LINE__ );
	S.base = Alc_V( 1, S.nsrf, sizeof(int), __FILE__, __LINE__ );
	S.cmbn = Alc_V( 1, S.nsrf, sizeof(int), __FILE__, __LINE__ );
	S.srf = Alc_V( 1, CD.nAllSrf, sizeof(SRFDAT2D), __FILE__, __LINE__ );

	/* read v/s data file */
	GetVS2D( S.name, S.emit, S.base, S.cmbn, S.srf, &CD );
	NxtClose();

	fprintf( stderr, "Surfaces:\n" );
	fprintf( stderr, "   #     emit   base  cmbn   name\n" );
	for( n=1; n<=S.nsrf; n++ ){
		fprintf( stderr, "%4d    %5.3f %5d %5d    %s\n", n
			, S.emit[n], S.base[n], S.cmbn[n], S.name[n] 
		);
	}

	fprintf( stderr, "Area, direction cosines:\n" );
	fprintf( stderr, "   #     area        x          y          w\n" );
	for( n=1; n<=CD.nAllSrf; n++ ){
		fprintf( stderr, "%4d %10.5f %10.5f %10.5f %10.5f\n"
			, n, S.srf[n].area, S.srf[n].dc.x, S.srf[n].dc.y, S.srf[n].dc.w
		);
	}

	fprintf( stderr, "Vertices:\n" );
	fprintf( stderr, "   #      v1.x       v1.y       v2.x       v2.y\n" );
	for(n=1; n<=CD.nAllSrf; n++){
		fprintf( stderr, "%4d %10.3f %10.3f %10.3f %10.3f\n"
			,n, S.srf[n].v1.x, S.srf[n].v1.y, S.srf[n].v2.x, S.srf[n].v2.y
		);
	}

	Fre_V(S.cmbn, 1, S.nsrf, sizeof(int), __FILE__, __LINE__ );
	Fre_V(S.base, 1, S.nsrf, sizeof(int), __FILE__, __LINE__ );
	Fre_V(vtmp, 1, S.nsrf, sizeof(float), __FILE__, __LINE__ );
	Fre_V(S.emit, 1, S.nsrf, sizeof(float), __FILE__, __LINE__ );
	Fre_MC((void **)S.name, 1, S.nsrf, 0, NAMELEN, sizeof(char), __FILE__, __LINE__ );

	/* render it to 2D window */
	GtkWidget *window;
	GtkWidget *canvas;

	gtk_init(&argc, &argv);

	/* create a new top level window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/* make the gtk terminate the process the close button is pressed */
	g_signal_connect(
		G_OBJECT (window), "delete-event",
		G_CALLBACK (gtk_main_quit), NULL
	);

	/* create a new drawing area widget */
	canvas = gtk_drawing_area_new();

	/* set a requested (minimum size) for the canvas */
	gtk_widget_set_size_request(canvas, DEFAULT_WIDTH, DEFAULT_HEIGHT);

	/* connect our drawing method to the "expose" signal */
	g_signal_connect(
		G_OBJECT (canvas), "expose-event",
		G_CALLBACK (paint),
		&S /* pointer to our scene data for rendering */
	);

	/* pack canvas widget into window */
	gtk_container_add(GTK_CONTAINER (window), canvas);

	/* show window and all it's children (just the canvas widget) */
	gtk_widget_show_all(window);

	/* enter main loop */ 
	gtk_main();
	return 0;
}


/*------------------------------------------------------------------------------
	The actual function invoked to paint the canvas widget, this is where most 
	Cairo painting functions will go
*/
void paint(GtkWidget *widget, GdkEventExpose *eev, gpointer data){
	Scene2D *S = (Scene2D *)data;

	gint width, height;
	gint i;
	cairo_t *cr;

	float minx = FLT_MAX, miny = FLT_MAX, maxx = -FLT_MAX, maxy = -FLT_MAX;
	for(i=1; i<=S->nsrf; ++i){
		float x,y;
		x = S->srf[i].v1.x; y = S->srf[i].v1.y;
		if(x<minx)minx=x;
		if(y<miny)miny=y;
		if(x>maxx)maxx=x;
		if(y>maxy)maxy=y;
		x = S->srf[i].v2.x; y = S->srf[i].v2.y;
		if(x<minx)minx=x;
		if(y<miny)miny=y;
		if(x>maxx)maxx=x;
		if(y>maxy)maxy=y;
	}

	float dx = maxx-minx;
	float dy = maxy-miny;
	
	width  = widget->allocation.width;
	height = widget->allocation.height;

	// margins
	float mx = 20, my = 20;

	float sx = dx/(width - 2*mx); /* true-length per pixel */
	float sy = dy/(height - 2*my);

	float s = sx;
	if(sy > sx)s = sy;
	
	//fprintf(stderr,"minx = %f, maxx = %f\n", minx, maxx);
	//fprintf(stderr,"miny = %f, maxy = %f\n", miny, maxy);
	//fprintf(stderr,"s = %f\n",s);

	cr = gdk_cairo_create (widget->window);

	/* clear background */
	cairo_set_source_rgb(cr, 1,1,1);
	cairo_paint(cr);

	float cx = 0.5*(maxx+minx);
	float cy = 0.5*(maxy+miny);
	//fprintf(stderr,"cx = %f, cy = %f",cx,cy);
	cairo_translate(cr, width/2. - cx/s, height/2. + cy/s);

	/* negate y scaling so that up is positive */
	cairo_scale (cr, 1./s, -1./s);

	cairo_set_line_width (cr, 2*s);

	cairo_set_source_rgb (cr, 0, 0, 0);

	for(i=1; i<=S->nsrf; ++i){
		cairo_move_to(cr, S->srf[i].v1.x, S->srf[i].v1.y);
		cairo_line_to(cr, S->srf[i].v2.x, S->srf[i].v2.y);
	};
	cairo_stroke(cr);

#if 0
	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
		                                CAIRO_FONT_WEIGHT_BOLD);
    /* enclosing in a save/restore pair since we alter the
     * font size */
    cairo_save (cr);
      cairo_set_font_size (cr, 40);
      cairo_move_to (cr, 40, 60);
      cairo_set_source_rgb (cr, 0,0,0);
      cairo_show_text (cr, "Hello World");
    cairo_restore (cr);

    cairo_set_source_rgb (cr, 1,0,0);
    cairo_set_font_size (cr, 20);
    cairo_move_to (cr, 50, 100);
    cairo_show_text (cr, "greetings from gtk and cairo");

    cairo_set_source_rgb (cr, 0,0,1);

    cairo_move_to (cr, 0, 150);
    for (i=0; i< width/10; i++){
		cairo_rel_line_to (cr, 5,  10);
		cairo_rel_line_to (cr, 5, -10);
    }
    cairo_stroke (cr);
#endif

	cairo_destroy (cr);
}

