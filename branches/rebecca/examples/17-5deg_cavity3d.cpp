/*
	Program to create the ANU Little Dish cavity geometry for export to View3D.
	
	We use the SbVec2d data type from the COIN3D library to perform the simple
	vector algebra in this case.
*/
#include <Inventor/SbVec3d.h>
#include <Inventor/SbDPRotation.h>
#include <Inventor/SbDPMatrix.h>

const double PI = 3.14159265358;

#include <unistd.h>
#include <stdlib.h>

#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;


class Surface3D{
public:
	Surface3D() : v1(0), v2(0), v3(0), v4(0), bas(0), cmb(0), emit(0){}
	Surface3D(const unsigned &v1, const unsigned &v2, const unsigned &v3, const unsigned &v4, const double &emit)
		 : v1(v1), v2(v2), v3(v3), v4(v4), bas(0), cmb(0), emit(emit){}
	unsigned v1, v2, v3, v4;
	int bas;
	int cmb;
	double emit;
};

void write_vec(SbVec3d &v){
	cerr << v[0] << " " << v[1] << " " << v[2];
}
#define WRITE_VEC(VEC) \
	cerr << #VEC << " = "; \
	write_vec(VEC); \
	cerr << endl;

/**
	Add a reactor tube cylinder to the current scene
	(Reactor tube surfaces face "outwards" towards the rest of the cavity interior.)
	
	In case of B==P, assume a cone, and collapse vertices accordingly.
*/
int add_cylinder(map<unsigned,SbVec3d> &vertices
		, map<string,Surface3D> &surfaces
		, SbVec3d O, SbVec3d P, SbVec3d A, SbVec3d B
		, unsigned r, unsigned c
		, unsigned &n_start
		, string namestem
		, double emit
){
	// SbVec3d B = A + (P - O);
	cerr << "starting index " << n_start << endl;
	unsigned n = n_start;

	/* work out a transform, tf2, that puts O at origin, P at (0,0,|OP|) and
	A at (|OA|,0,0) */
	SbVec3d Z(0,0,1);
	SbVec3d X(1,0,0);

	SbDPMatrix tr; tr.setTranslate(-O);
	SbVec3d O1; tr.multVecMatrix(O,O1);

	SbVec3d OP = P - O;

	bool is_cone = (B == P);

	WRITE_VEC(O);
	WRITE_VEC(P);
	WRITE_VEC(OP);
	SbVec3d P1; tr.multVecMatrix(P,P1);
	WRITE_VEC(O1);
	WRITE_VEC(P1);
	WRITE_VEC(Z);
	SbDPMatrix tf1;
	if(P1.cross(Z).length() > 1e-6){
		/* FIXME SOMETHING wrong in here... */
		cerr << "rotation rot1 required" << endl;
		SbDPMatrix rot1;
		SbDPRotation rot1r(P1, Z);
		rot1.setRotate(rot1r);

		SbVec3d AX1; double rad;
		rot1r.getValue(AX1, rad);
		cerr << "rotation is " << rad << " around ";
		WRITE_VEC(AX1);

		tf1 = tr * rot1;
	}else{
		tf1 = tr;
	}

	WRITE_VEC(A);

	SbVec3d A1;
	tf1.multVecMatrix(A, A1);
	WRITE_VEC(A1);

	SbVec3d P11;
	tf1.multVecMatrix(P, P11);
	WRITE_VEC(P11);

	SbDPMatrix tf2;
	if(A1.cross(X).length() > 1e-6){
		SbDPMatrix rot2; rot2.setRotate(SbDPRotation(A1, X));
		tf2 = tf1 * rot2;
	}else{
		tf2 = tf1;
	}

	SbVec3d A2; tf2.multVecMatrix(A, A2);
	WRITE_VEC(A2);

	SbVec3d B2; tf2.multVecMatrix(B, B2);
	WRITE_VEC(B2);

	/* inverse transform to place points back at correct spot */
	SbDPMatrix itf2;
	if(tf2.det4() != 0){
		itf2 = tf2.inverse();
	}else{
		itf2 = SbDPMatrix::identity();
	}

	WRITE_VEC(B);
	SbVec3d B0; itf2.multVecMatrix(B2, B0);
	WRITE_VEC(B0);

	/* vector pointing along AB with length of one grid element */
	SbVec3d ABr2 = (B2 - A2) * (1./(r - 1));

	cerr << "Vertices..." << endl;
	for(unsigned i = 0; i < c; ++i){
		/* calculation rotation matrix to rotate A2 around Z-axis */
		SbDPRotation rot(Z, i*2*PI/c);
		SbDPMatrix mat; mat.setRotate(rot);
		SbVec3d A2dash;
		mat.multVecMatrix(A2, A2dash);
		WRITE_VEC(A2dash);
		/* displacement vector for subsequent rows */
		SbVec3d ABr2dash;
		mat.multVecMatrix(ABr2, ABr2dash);
		for(unsigned j = 0; j < r; ++j){
			SbVec3d D2 = A2dash + j * ABr2dash;
			SbVec3d D0; itf2.multVecMatrix(D2, D0);
			vertices[n++] = D0;
		}
	}

	cerr << "Surfaces..." << endl;
	/* output sides of the cone */
	for(unsigned i = 0; i < c; ++i){
		for(unsigned j = 0; j < r - 1; ++j){
			unsigned v1, v2, v3, v4;
			v1 = n_start + r*i + j;
			v2 = n_start + r*(i+1) + j;
			v3 = v2 + 1;
			v4 = v1 + 1;
			cerr << "vertices:" << v1 << " " << v2 << " " << v3 << " " << v4 << endl;

			if(i == c - 1){
				v2 = n_start + j;
				v3 = v2 + 1;
			}

			/* This string gives the surface name that will appear at the end of 
			each line in the .vs3 file. 
			--It also serves as the INDEX for each surface in array surfaces[]--
			So in the final lines of code with title /* output to View3D format 
			the surfaces are written in alpha-numeric order according to this 
			string. Previously c came before r in the string, but having r first 
			means that all of the first row are printed together, e.g. r0c0, 
			r0c1, r0c2 etc, which simplifies the temperature input file.
			*/
			stringstream ss;
			ss << namestem  << "r" << j << "c" << i;

			surfaces[ss.str()] = Surface3D(v1,v2,v3,v4, emit);
		}
	}

	/* FIXME optionally add end-faces for cylinder? */

	/* update the starting index for subsequent vertices */
	n_start = n;

	return 0;
}

/**
	Add a ring of cavity wall (cylinder) to the current scene
	This is very similar to the add_cylinder function above that creates the 
	reactor tubes, except that the cavity walls need to point into the cavity, 
	whereas the reactor tubes need to point "outwards".
	
	In case of B==P, assume a cone, and collapse vertices accordingly.
*/

int add_cylinder_walls(map<unsigned,SbVec3d> &vertices
		, map<string,Surface3D> &surfaces
		, SbVec3d O, SbVec3d P, SbVec3d A, SbVec3d B
		, unsigned r, unsigned c
		, unsigned &n_start
		, string namestem
		, double emit
){
	// SbVec3d B = A + (P - O);
	cerr << "starting index " << n_start << endl;
	unsigned n = n_start;

	/* work out a transform, tf2, that puts O at origin, P at (0,0,|OP|) and
	A at (|OA|,0,0) */
	SbVec3d Z(0,0,1);
	SbVec3d X(1,0,0);

	SbDPMatrix tr; tr.setTranslate(-O);
	SbVec3d O1; tr.multVecMatrix(O,O1);

	SbVec3d OP = P - O;

	bool is_cone = (B == P);

	WRITE_VEC(O);
	WRITE_VEC(P);
	WRITE_VEC(OP);
	SbVec3d P1; tr.multVecMatrix(P,P1);
	WRITE_VEC(O1);
	WRITE_VEC(P1);
	WRITE_VEC(Z);
	SbDPMatrix tf1;
	if(P1.cross(Z).length() > 1e-6){
		/* FIXME SOMETHING wrong in here... */
		cerr << "rotation rot1 required" << endl;
		SbDPMatrix rot1;
		SbDPRotation rot1r(P1, Z);
		rot1.setRotate(rot1r);

		SbVec3d AX1; double rad;
		rot1r.getValue(AX1, rad);
		cerr << "rotation is " << rad << " around ";
		WRITE_VEC(AX1);

		tf1 = tr * rot1;
	}else{
		tf1 = tr;
	}

	WRITE_VEC(A);

	SbVec3d A1;
	tf1.multVecMatrix(A, A1);
	WRITE_VEC(A1);

	SbVec3d P11;
	tf1.multVecMatrix(P, P11);
	WRITE_VEC(P11);

	SbDPMatrix tf2;
	if(A1.cross(X).length() > 1e-6){
		SbDPMatrix rot2; rot2.setRotate(SbDPRotation(A1, X));
		tf2 = tf1 * rot2;
	}else{
		tf2 = tf1;
	}

	SbVec3d A2; tf2.multVecMatrix(A, A2);
	WRITE_VEC(A2);

	SbVec3d B2; tf2.multVecMatrix(B, B2);
	WRITE_VEC(B2);

	/* inverse transform to place points back at correct spot */
	SbDPMatrix itf2;
	if(tf2.det4() != 0){
		itf2 = tf2.inverse();
	}else{
		itf2 = SbDPMatrix::identity();
	}

	WRITE_VEC(B);
	SbVec3d B0; itf2.multVecMatrix(B2, B0);
	WRITE_VEC(B0);

	/* vector pointing along AB with length of one grid element */
	SbVec3d ABr2 = (B2 - A2) * (1./(r - 1));

	cerr << "Vertices..." << endl;
	for(unsigned i = 0; i < c; ++i){
		/* calculation rotation matrix to rotate A2 around Z-axis */
		SbDPRotation rot(Z, i*2*PI/c);
		SbDPMatrix mat; mat.setRotate(rot);
		SbVec3d A2dash;
		mat.multVecMatrix(A2, A2dash);
		WRITE_VEC(A2dash);
		/* displacement vector for subsequent rows */
		SbVec3d ABr2dash;
		mat.multVecMatrix(ABr2, ABr2dash);
		for(unsigned j = 0; j < r; ++j){
			SbVec3d D2 = A2dash + j * ABr2dash;
			SbVec3d D0; itf2.multVecMatrix(D2, D0);
			vertices[n++] = D0;
		}
	}

	cerr << "Surfaces..." << endl;
	/* output sides of the cone */
	for(unsigned i = 0; i < c; ++i){
		for(unsigned j = 0; j < r - 1; ++j){
			unsigned v1, v2, v3, v4;
			v1 = n_start + r*i + j;
			v2 = v1 + 1;
			v4 = n_start + r*(i+1) + j;
			v3 = v4 + 1;
			
			cerr << "vertices:" << v1 << " " << v2 << " " << v3 << " " << v4 << endl;

			if(i == c - 1){
				v4 = n_start + j;
				v3 = v4 + 1;
			}
			
			/* This string gives the surface name that will appear at the end of each 
			line in the .vs3 file. 
			--It also serves as the INDEX for each surface in the array "surfaces[]".--
			So in the final lines of code with "title" /* output to View3D format... hopefully 
			the surfaces are written in alpha-numeric order according to this string.
			Previously c came before r in the string, but having r first means that all of the 
			first row are printed together, e.g. r0c0, r0c1, r0c2 etc, which simplifies the 
			temperature input file.
			*/
			stringstream ss;
			ss << namestem  << "r" << j << "c" << i;

			surfaces[ss.str()] = Surface3D(v1,v2,v3,v4, emit);
		}
	}

	/* FIXME optionally add end-faces for cylinder? */

	/* update the starting index for subsequent vertices */
	n_start = n;

	return 0;
}

/** Create the aperture.
	In general this creates a pyramid, tetrahedron, etc (or flat one in this case)
	by creating a 'star' of faces that take form from the edges of a polygon (vertices 
	already added, numbered per 'perim') and a new vertex 'A', added here.

	'Pavilion' is a term from diamond-cutting, it's the bottom tapered part
	of a cut diamond.
*/
int add_pavilion(map<unsigned,SbVec3d> &vertices
		, map<string,Surface3D> &surfaces
		, SbVec3d A
		, vector<unsigned> &perim /* already assumed to be added */
		, unsigned &n_start
		, string namestem
		, double emit
){
	int n = n_start;
	vertices[n++] = A;

	for(unsigned i = 0; i < perim.size(); ++i){
		unsigned v1, v2, v3;
		if(i == 0){
			v1 = perim[perim.size()-1];
		}else{
			v1 = perim[i - 1];
		}
		v2 = perim[i];
		v3 = n_start;

		stringstream ss;
		ss << namestem << "_" << i;
		surfaces[ss.str()] = Surface3D(v1,v2,v3,0, emit);
	}

	n_start = n;
	return 0;
}

/** Create the top of the cavity.
	This is very similar to the "add_pavilion" function except that top 
	surfaces now face into the cavity in the right-hand rule sense.
*/

int add_pavilion_top(map<unsigned,SbVec3d> &vertices
		, map<string,Surface3D> &surfaces
		, SbVec3d A
		, vector<unsigned> &perim /* already assumed to be added */
		, unsigned &n_start
		, string namestem
		, double emit
){
	int n = n_start;
	vertices[n++] = A;

	for(unsigned i = 0; i < perim.size(); ++i){
		unsigned v1, v2, v3;
		if(i == 0){
			v2 = perim[perim.size()-1];
		}else{
			v2 = perim[i - 1];
		}
		v1 = perim[i];
		v3 = n_start;

		stringstream ss;
		ss << namestem << "_" << i;
		surfaces[ss.str()] = Surface3D(v1,v2,v3,0, emit);
	}

	n_start = n;
	return 0;
}

/** add a row of quadrilateral elements to a solid of rotation.
	@param O origin of the solid
	@param OP axis of the solid around which rotation occurs
	@param A coordinates of the first new vertex to be added
	@param perim vertex IDs of the previous row of the solid, these are allowed
		to be non-contiguous
	@param n_start starting ID for new vertices being added.
	@param newperim vertix IDs of the vertices for the newly-created row will be
		added to this vector, which is assumed to be empty as-given.
	Added quadrilaterals will be 
	p0 p1 A'   A
	p1 p2 A''  A'
	p3 p4 A''' A''
	:  :  :    :
    pn p0 A(n) A(n-1)

	where A, A', A'',... A(n) are the rotated locations of A obtained by
	divided 2*pi by perim.size(). We assume (but do not check) that the
	vertices indicated by perim are distributed in the same way.
*/
int add_rotrow(map<unsigned,SbVec3d> &vertices
		, map<string,Surface3D> &surfaces
		, SbVec3d O, SbVec3d OP, SbVec3d A
		, vector<unsigned> &perim /* already assumed to be added */
		, vector<unsigned> &newperim
		, unsigned &n_start
		, string namestem
		, double emit
){
	unsigned n = n_start;
	SbDPMatrix tr1; tr1.setTranslate(-O);
	SbDPMatrix tr1i = tr1.inverse();
	unsigned n_sides = perim.size();

	WRITE_VEC(A);
	SbVec3d A1; tr1.multVecMatrix(A, A1);
	WRITE_VEC(A1);

	for(unsigned i=0; i<n_sides; ++i){
		double theta = 2*PI / n_sides * i;
		SbDPMatrix rot1; rot1.setRotate(SbDPRotation(OP,theta));
		SbDPMatrix tr = tr1*rot1*tr1i;
		SbVec3d Adash; tr.multVecMatrix(A, Adash); /* as in A' in quadrilaterals listed above in comments */
		WRITE_VEC(Adash);

		vertices[n] = Adash;
		newperim.push_back(n++);
	}

	for(unsigned i=0; i<n_sides; ++i){
		unsigned v1, v2, v3, v4;
		if(i < n_sides - 1){
			v1 = perim[i];
			v2 = perim[i+1];
			v3 = n_start + i + 1;
			v4 = n_start + i;
		}else{
			v1 = perim[i]; /* accounts for getting back around to start of circle/octagon etc. */
			v2 = perim[0];
			v3 = n_start;
			v4 = n_start + i;
		}
		stringstream ss;
		ss << namestem << i;
		cerr << "Adding surface " << ss.str() << endl;
		cerr << "v1 = " << v1 << ": ";
		WRITE_VEC(vertices[v1]);
		cerr << "v2 = " << v2 << ": ";
		WRITE_VEC(vertices[v2]);
		cerr << "v3 = " << v3 << ": ";
		WRITE_VEC(vertices[v3]);
		cerr << "v4 = " << v4 << ": ";
		WRITE_VEC(vertices[v4]);
		surfaces[ss.str()] = Surface3D(v1,v2,v3,v4, emit);
	}

	n_start = n;
	return 0;
}

int main(int argc, char **argv){

	const char outfilename[] = "17-5deg_--g_is_38mm.vs3";
	// const char outfilename[] = "17-5deg_two-rows_FEperp.vs3";

	cerr << "Creating cone..." << endl;

	map<unsigned,SbVec3d> vertices;
	map<string,Surface3D> surfaces;

	double emit_tubes = 0.8; /* emissivity*/
	double emit_walls = 0.4; /* emissivity */
	double emit_apert = 0.999999999999999; /* emissivity */
	double emit_top = 0.58; /* emissivity - accounts for inconel manifold & "white" insulation. */
	double emit_bot = 0.4; /* emissivity */

	double h =  0.570; /* cavity height in m*/
	double d1 = 0.480; /* frustrum base dia in m */
	double d2 = 0.300; /* frustrum apex dia in m*/
	double a =  0.195; /* aperture dia in m */

	unsigned n_tubes = 20;
	double t = 0.013; /* tube diameter in m*/

	// See "Diagram of fguv_ BAperp etc.pdf" in C:\Users\rebecca\Desktop\rebecca\View3d files
	// See also definition of SbVec3d F and SbVec3d E below.
	double f = 0.030; /* distance along cavity wall BA to base end of tube E in m. */
	double g = 0.038; /* distance normal from cavity wall BA to base end centreline of tube E in m. */
	double u = 0.033; /* distance along cavity wall BA to apex end of tube F in m. */
	double v = 0.112; /* distance normal from cavity wall BA to apex end centreline of tube F in m. */


	SbVec3d O(0,0,0);
	SbVec3d P(0,0,h);
	SbVec3d A(d1/2., 0, 0);
	SbVec3d B(d2/2., 0, h);
	SbVec3d C(a/2, 0, 0);

	// gridding parameters
	unsigned r = 5; /* number of rows in cone grid for walls (4 segments +1)*/
	unsigned c = 20; /* number of columns (circumf) in cone grid for walls*/
	
	unsigned tube_rows = 5; /* number of rows in cone grid for reactor tubes (4 segments +1) */
	unsigned tube_cols = 6; /* number of columns (circumf) in cone grid for reactor tubes */

	unsigned n_cyl_start = 1;
	unsigned n = n_cyl_start;

	/* main frustum */
	add_cylinder_walls(vertices, surfaces, O, P, A, B, r, c, n, "wall", emit_walls);

	/* top end of cavity */
	vector<unsigned> topperim;
	for(unsigned i=0; i < c; ++i){
		unsigned p = n_cyl_start + r*i + (r - 1);
		cerr << "p = " << p << endl;
		topperim.push_back(p);
	}
	add_pavilion_top(vertices, surfaces, P, topperim, n, "top", emit_top);

	/* bottom end of cavity */
	vector<unsigned> botperim, newperim;
	for(unsigned i=0; i < c; ++i){
		unsigned p = n_cyl_start + r*i;
		cerr << "p = " << p << endl;
		botperim.push_back(p);
	}
	add_rotrow(vertices, surfaces, O, P - O, C, botperim, newperim, n, "bot", emit_bot);

	cerr << "New perim contains " << newperim.size() << " faces" << endl;

	add_pavilion(vertices, surfaces, O, newperim, n, "apert", emit_apert);

#if 1
	/* inner pipes */
	SbVec3d BAhat = (A - B); BAhat.normalize();
	SbVec3d PBhat = B - P; PBhat.normalize();
	SbVec3d OPhat = P - O; OPhat.normalize();
	SbVec3d PBtan = PBhat.cross(OPhat);
	SbVec3d BAperp = BAhat.cross(PBtan);
	SbVec3d F = B + BAhat * u + BAperp * v;
	WRITE_VEC(F);
	SbVec3d E = A - BAhat * f + BAperp * g;
	SbVec3d FEhat = (E - F); FEhat.normalize();
	/* unit vector normal to FE */
	SbVec3d FEperp = PBtan.cross(FEhat);
	SbVec3d E1 = E + t/2. * FEperp; // first edge to "sweep out" tube suface.
	SbVec3d F1 = F + t/2. * FEperp;
	/* The sweep using BAperp only worked when FE was parallel to BA. */
	//SbVec3d E1 = E - t/2. * BAperp;
	//SbVec3d F1 = F - t/2. * BAperp;
	WRITE_VEC(E);
	WRITE_VEC(E1);
	WRITE_VEC(F1);

	for(unsigned i = 0; i < n_tubes; ++i){
		double theta = i * 2. * PI / n_tubes;
		SbVec3d Z(0,0,1);
		SbDPRotation rot(Z, theta);
		SbDPMatrix mat; mat.setRotate(rot);
		
		SbVec3d E2, F2, E12, F12;
		mat.multVecMatrix(E,E2);
		mat.multVecMatrix(F,F2);
		mat.multVecMatrix(E1,E12);
		mat.multVecMatrix(F1,F12);

		stringstream ss;
		ss << "cyl" << i << "_";
		cerr << "Adding tube " << i << endl;
		add_cylinder(vertices, surfaces, E2, F2, E12, F12, tube_rows, tube_cols, n, ss.str(), emit_tubes);
	}
#endif

	/* output to View3D format... hopefully */
	cerr << "Writing '" << outfilename << "'..." << endl;
	ofstream fs(outfilename);
	fs << "T 17.5 deg frustum and cavity for ANU little dish study" << endl;
	fs << "C  list=0 out=0 emit=0 encl=0 eps=1.e-6" << endl;
	fs << "F 3" << endl;
	fs.precision(15);

	for(map<unsigned,SbVec3d>::const_iterator i=vertices.begin(); i!=vertices.end(); ++i){
		fs << "V " << (i->first) << " " << i->second[0] << " " << i->second[1] << " " << i->second[2] << endl;
	}

	fs << "!   #   v1  v2  v3  v4 base cmb emit  name" << endl;
	unsigned surfnum=1;
	for(map<string,Surface3D>::const_iterator i=surfaces.begin(); i!=surfaces.end(); ++i){
		fs << "S " << surfnum++ << " " 
			<< (i->second.v1) << " " << (i->second.v2) << " " << (i->second.v3) << " " << (i->second.v4) << " "
			<< i->second.bas << " " << i->second.cmb << " " 
			<< i->second.emit <<  " " << i->first /* = name of srf */
			<< endl;
	}
	fs << "End of data" << endl;
	fs.close();
}












