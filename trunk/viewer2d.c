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
#include <assert.h>

#include "types.h" 
#include "misc.h"
#include "heap.h"
#include "test2d.h"
#include "viewpp.h"
#include "readvs.h"
#include "savevf.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <cairo-pdf.h>

#define DEFAULT_WIDTH  600
#define DEFAULT_HEIGHT 400

void usage(const char *progname){
	fprintf(stderr,
			"Usage: %s [-o OUTFILE] [-h] [-t [-m ID]] INFILE\n"
			"Load and parse a View2D file and render using Cairo/GTK+\n"
			"  INFILE      View2D .vs2 file to render (eg 'facet.vs2')\n"
			"  -t          Include text in the rendered output.\n"
			"  -o OUTFILE  Name of a PDF file to which output should be\n"
			"              directed (instead of on-screen rendering)\n"
			"  -m ID       Don't draw text labels for surfaces with ID\n"
			"              exceeding this value\n"
		, progname
	);	
}

/* data structures */

/* information about the current mouse selection (for zooming) */
typedef struct
{
  gboolean active;   /* whether the selection is active or not */
  gdouble  x, y;
  gdouble  w, h;
}
SelectionInfo;

/* all scene data to be used in the drawing */
typedef struct{
	VertexSurfaceData *V;
	char infotext;
	SelectionInfo sel;
	const char *outfile; /* set non-null if output to a PDF file is desired (this will be the filename) */
	int maxlabel; /* maximum ID for surface labelling (or -1 if all should be labelled) */
	float tx, ty, s; /* translation and scaling, used to work out where we will be zooming to */
} Scene2D;

/* forward decls */
static void paint (GtkWidget *widget, GdkEventExpose *eev
	, gpointer data
);

static void paint_selection(cairo_t *cr, SelectionInfo *sel);

static gboolean event_press(
	GtkWidget *widget, GdkEventButton *bev, SelectionInfo  *sel
);

static gboolean event_motion(
	GtkWidget *widget, GdkEventMotion *mev, SelectionInfo  *sel
);

static gboolean event_release(
	GtkWidget *widget, GdkEventButton *bev, SelectionInfo  *sel
);
/*------------------------------------------------------------------------------
	Main routine, sets up the window and links drawing methods to it
*/
int main (int argc, char **argv){
	short infotext = 0;
	const char *outfile = NULL;
	int maxlabel = 0;

	char c;	
	while((c=getopt(argc,argv,"o:tm:"))!=-1){
		switch(c){
			case 'o':
				outfile = optarg;
				break;
			case 't':
				infotext = 1;
				break;
			case 'm':
				maxlabel = atoi(optarg);
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
	S.infotext = infotext;
	S.outfile = outfile;
	S.maxlabel = maxlabel;

	S.V = read_vertex_surface_data(filename);
	if(S.V==NULL){
		fprintf(stderr,"Failed to read vertex/surface data from '%s'",filename);
		exit(3);
	}

	if(!S.outfile){
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

		/* connect callbacks for zooming the display */

		gtk_widget_add_events (canvas,
			GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
		);

		g_signal_connect (G_OBJECT (canvas), "button_press_event",
			G_CALLBACK (event_press), &(S.sel)
		);
		g_signal_connect (G_OBJECT (canvas), "button_release_event",
			G_CALLBACK (event_release), &(S.sel)
		);
		g_signal_connect (G_OBJECT (canvas), "motion_notify_event",
			G_CALLBACK (event_motion), &(S.sel)
		);

		/* pack canvas widget into window */
		gtk_container_add(GTK_CONTAINER (window), canvas);

		/* show window and all it's children (just the canvas widget) */
		gtk_widget_show_all(window);

		/* enter main loop */ 
		gtk_main();

		vertex_surface_data_destroy(S.V);
	}else{
		paint(NULL,NULL,&S);
	}
	return 0;
}


/*------------------------------------------------------------------------------
	The actual function invoked to paint the canvas widget, this is where most 
	Cairo painting functions will go
*/
void paint(GtkWidget *widget, GdkEventExpose *eev, gpointer data){
	Scene2D *S = (Scene2D *)data;
	VertexSurfaceData *V = S->V;

	gint width, height;
	gint i;
	cairo_t *cr;
	float d, ox, oy;
	cairo_text_extents_t ex;
	float mx, my, dx, dy;

	assert(V3D_IS_2D(V));

	float minx = FLT_MAX, miny = FLT_MAX, maxx = -FLT_MAX, maxy = -FLT_MAX;
	for(i=1; i<=V->nall; ++i){
		float x,y;
		x = V->d2.srf[i].v1.x; y = V->d2.srf[i].v1.y;
		if(x<minx)minx=x;
		if(y<miny)miny=y;
		if(x>maxx)maxx=x;
		if(y>maxy)maxy=y;
		x = V->d2.srf[i].v2.x; y = V->d2.srf[i].v2.y;
		if(x<minx)minx=x;
		if(y<miny)miny=y;
		if(x>maxx)maxx=x;
		if(y>maxy)maxy=y;
	}

	dx = maxx-minx;
	dy = maxy-miny;
		
	//fprintf(stderr,"minx = %f, maxx = %f\n", minx, maxx);
	//fprintf(stderr,"miny = %f, maxy = %f\n", miny, maxy);
	//fprintf(stderr,"s = %f\n",s);

	cairo_surface_t *sfc;
	if(S->outfile){
		mx = 25, my = 25;
		width = 1000;
		height = 2*my + (width - 2*mx) * dy / dx;
		sfc = cairo_pdf_surface_create(S->outfile,width, height);
		cr = cairo_create(sfc);
	}else{
		// render via GTK
		cr = gdk_cairo_create(widget->window);
		width  = widget->allocation.width;
		height = widget->allocation.height;
		// margins
		mx = 25, my = 25;
	}

	float sx = dx/(width - 2*mx); /* true-length per pixel */
	float sy = dy/(height - 2*my);
	float s = sx;
	if(sy > sx)s = sy;

	/* clear background */
	cairo_set_source_rgb(cr, 1,1,1);
	cairo_paint(cr);

	cairo_save(cr);

	float cx = 0.5*(maxx+minx);
	float cy = 0.5*(maxy+miny);
	//fprintf(stderr,"cx = %f, cy = %f",cx,cy);

	S->tx = width/2. - cx/s;
	S->ty = height/2. + cy/s;
	S->s = s;
	cairo_translate(cr, S->tx, S->ty);

	/* 
		note that we don't scale y, so when plotting, 'y' values must be negated
		to make +y be up the screen
	*/
	cairo_scale (cr, 1./s, 1./s);
	cairo_set_font_size (cr, 10.*s);
	cairo_set_line_width (cr, 2*s);
	cairo_set_source_rgb (cr, 0, 0, 0);


	for(i=1; i<=V->nall; ++i){
		cairo_move_to(cr, V->d2.srf[i].v1.x, -V->d2.srf[i].v1.y);
		cairo_line_to(cr, V->d2.srf[i].v2.x, -V->d2.srf[i].v2.y);
	};
	cairo_stroke(cr);

	/* draw a dot at the start of each edge */
	for(i=1; i<=V->nall; ++i){
		cairo_arc(cr, V->d2.srf[i].v1.x, -V->d2.srf[i].v1.y, 2*s, 0, 2*M_PI);
		cairo_fill(cr);
	}

	if(S->infotext){
		//fprintf(stderr,"WRITING INFOTEXT...\n");

		cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

		/* enclosing in a save/restore pair since we alter the
		* font size */
		for(i=1; i<=V->nrad; ++i){
			if(S->maxlabel && i > S->maxlabel){	
				fprintf(stderr,"Stopping with maxlabel = %d\n",i);
				break;
			}
			/* reuse the vars cx,cy for the midpoint of the edge */
			cx = 0.5 * (V->d2.srf[i].v1.x + V->d2.srf[i].v2.x);
			cy = 0.5 * (V->d2.srf[i].v1.y + V->d2.srf[i].v2.y);
			/* reuse the vars dx,dy for the gradient of the line */
			dx = (V->d2.srf[i].v2.x - V->d2.srf[i].v1.x);
			dy = (V->d2.srf[i].v2.y - V->d2.srf[i].v1.y);
			d = sqrt(dx*dx + dy*dy);
			//fprintf(stderr,"dx = %f, dy = %f, d = %f\n",dx,dy,d);
			dx *= 10.*s / d;
			dy *= 10.*s / d;
			/* calculate the width of the text so that we can right-justify if required */
			cairo_text_extents(cr,V->name[i],&ex);
			ox = 0; oy = 0;
			if(dy < 0){
				ox = -ex.width;
			}else if(dy==0){
				ox = -0.5*ex.width;
			}
			if(dx > 0){
				oy = -ex.height;
			}

			/* draw a line to the midpoint label point */
			cairo_save(cr);
			cairo_set_line_width (cr, 1*s);
			cairo_set_source_rgb (cr, 1, 0.7, 0.7);
			cairo_move_to(cr, cx, -cy);
			cairo_line_to(cr, cx + dy, -(cy - dx));
			cairo_stroke(cr);
			cairo_restore(cr);

			/* we will offset the text normal to the edge */
			cairo_save(cr);
			cairo_set_source_rgb (cr, 1.0, 0, 0);
			cairo_move_to (cr, cx + dy + ox, -(cy - dx + oy));
			//fprintf(stderr,"text at (%f, %f) = %s\n",cx-dy,cy+dx,S->name[i]);
			cairo_show_text (cr, V->name[i]);
			cairo_restore(cr);
		}
	}

	if(!S->outfile){
		cairo_restore(cr);
		paint_selection(cr, &(S->sel));
	}

	cairo_destroy(cr);
	if(S->outfile){
		fprintf(stderr,"Finalising surface...\n");
		cairo_surface_destroy(sfc);
	}
}

/* drag-to-zoom stuff */

/* function to draw the rectangular selection */
void paint_selection(cairo_t *cr, SelectionInfo *sel){
	if(!sel->active)
	return;

	cairo_save (cr);
	cairo_rectangle (cr, sel->x, sel->y, sel->w, sel->h);
	cairo_set_source_rgba (cr, 0, 0, 1, 0.2);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
	cairo_stroke (cr);
	cairo_restore (cr);
}

gboolean event_press(
	GtkWidget *widget, GdkEventButton *bev, SelectionInfo  *sel
){
	sel->active = TRUE;

	sel->x = bev->x;
	sel->y = bev->y;
	sel->w = 0;
	sel->h = 0;

	//fprintf(stderr,"PRESS at %f,%f\n",sel->x, sel->y);

	/* tell the canvas widget that it needs to redraw itself */
	gtk_widget_queue_draw (widget);

	return TRUE;
}


gboolean event_motion(
	GtkWidget *widget, GdkEventMotion *mev, SelectionInfo  *sel
){
  sel->w = mev->x - sel->x;
  sel->h = mev->y - sel->y;

  /* tell the canvas widget that it needs to redraw itself */
  gtk_widget_queue_draw (widget);
  return TRUE;
}

gboolean event_release(
	GtkWidget *widget, GdkEventButton *bev, SelectionInfo  *sel
){
  sel->active = FALSE;

  /* tell the canvas widget that it needs to redraw itself */
  gtk_widget_queue_draw (widget);
  return TRUE;
}

