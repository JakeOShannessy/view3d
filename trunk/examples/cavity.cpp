/*
	Program to create the CLFR cavity receiver geometry for export to View2D.
	
	We use the SbVec2d data type from the COIN3D library to perform the simple
	vector algebra in this case.
*/
#include <Inventor/SbVec2d.h>

const double PI = 3.14159265358;

#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
using namespace std;

class Surface2D{
public:
	Surface2D() : from(0), to(0), epsilon(1.0), obs(0),bas(0),cmb(0){}
	Surface2D(const unsigned &from, const unsigned &to, const double &epsilon)
		 : from(from), to(to), epsilon(epsilon), obs(0),bas(0),cmb(0){}
	unsigned from;
	unsigned to;
	double epsilon;
	int obs;
	int bas;
	int cmb;
};

/*
	Add a slices of surface S to the scene, including the additional 
	vertices that are required. Note that S itself is not added to the scene.
*/
void addSliced(const unsigned n, const Surface2D &S
	, map<unsigned,SbVec2d> &vertices
	, map<string,Surface2D> &surfaces
	, const string &prefix
	, unsigned &currentindex
){
	const unsigned &from = S.from;
	const unsigned &to = S.to;
	SbVec2d delta = (vertices[to] - vertices[from]) / n;
	SbVec2d A = vertices[from];
	for(unsigned i=0; i < n; ++i){
		SbVec2d B = A + delta;
		unsigned from1 = from;
		unsigned to1 = to;
		if(i>0){
			from1 = currentindex - 1;
			// we will already have added it at the last step
		}
		if(i<n-1){
			to1 = currentindex++;
			vertices[to1] = B;
		}	
		Surface2D S1(S);
		S1.from = from1;
		S1.to = to1;
		stringstream ss;
		ss << prefix << i;
		surfaces[ss.str()] = S1;
		A = B;
	}
}

#define EDG(A,B) pair<unsigned,unsigned>((A),(B))

int main(){
	double W_abs = 0.575;
	double D_cav = 0.200;
	double theta = PI/180. * 32;

	unsigned ntubes = 12, nsegs = 8;
	double d = 0.0422;
	double wbank = 0.520;
	double vsep = 0.7*d;
	double eps_pipe = 0.4;

	double B_cav = W_abs + 2 * D_cav/tan(theta);

	map<unsigned,SbVec2d> vertices;
	map<string,Surface2D> surfaces;

	// create the outer vertices
	vertices[1] = SbVec2d(+W_abs/2.,0); /* A */
	vertices[2] = SbVec2d(-W_abs/2.,0); /* B */
	vertices[3] = SbVec2d(-B_cav/2.,-D_cav); /* C */
	vertices[4] = SbVec2d(+B_cav/2.,-D_cav); /* D */

	unsigned vcurrent = 5;
	addSliced(ntubes, Surface2D(1,2, 0.4), vertices, surfaces, "AB", vcurrent);
	addSliced(4, Surface2D(2,3, 0.7), vertices, surfaces, "BC", vcurrent);
	addSliced(ntubes, Surface2D(3,4, 0.92), vertices, surfaces, "CD", vcurrent);
	addSliced(4, Surface2D(4,1, 0.7), vertices,surfaces, "DA", vcurrent);

	// create the cavity outer surfaces
	//surfaces["AB"] = Surface2D(1, 2, 0.4);
	//surfaces["BC"] = Surface2D(2, 3, 0.7);
	//surfaces["CD"] = Surface2D(3, 4, 0.92);
	//surfaces["DA"] = Surface2D(4, 1, 0.7);

	// create each of the pipes on the interior
	{
		double sep = wbank / (ntubes - 1);
		SbVec2d L(-wbank/2., -vsep);
		SbVec2d delta(sep, 0);
		double dtheta = 2. * PI / nsegs;
		double r = d / 2.;
		for(unsigned i=0; i<ntubes; ++i){
			cerr << "tube = " << i << endl;
			SbVec2d C = L + i * delta;
			unsigned vfirst = vcurrent;
			for(unsigned j=0; j<nsegs; ++j){
				double phi = -dtheta * j;
				SbVec2d R = C + r * SbVec2d(cos(phi), sin(phi));
				stringstream ss;
				ss << "p" << i << "s" << j;
				Surface2D S1;
				if(j==nsegs-1){
					S1 = Surface2D(vcurrent, vfirst, eps_pipe);
				}else{
					S1 = Surface2D(vcurrent, vcurrent+1, eps_pipe);
				}
				cerr << "S1.epsilon = " << S1.epsilon << endl;
				surfaces[ss.str()] = S1;
				cerr << "adding vcurrent = " << vcurrent << endl;
				vertices[vcurrent++] = R;
			}
		}
	}

	//vertices[0] = SbVec2d(-1,-1);

	for(map<string,Surface2D>::const_iterator i = surfaces.begin(); i!=surfaces.end(); ++i){
		cerr << i->first << " (" << i->second.from << "--" << i->second.to << ") = (" 
			<< vertices[i->second.from][0] << "," << vertices[i->second.from][1] << ") to ("
			<< vertices[i->second.to][0] << "," << vertices[i->second.to][1] << ")" 
			<< ", eps = " << i->second.epsilon << endl;
	}

	// ok, so if it looks OK let's output to the View2D format:
	ofstream fs("cavity.vs2");
	fs << "T CLFR Stage 2 cavity cross-section" << endl;
	fs << "! encl list eps  maxr minr emit" << endl;
	fs << "C  1    2  2.e-6   10    1    1" << endl;
	fs << "G 2" << endl;
	for(map<unsigned,SbVec2d>::const_iterator i=vertices.begin(); i!=vertices.end(); ++i){
		fs << "V " << (i->first) << " " << i->second[0] << " " << i->second[1] << endl;
	}
	unsigned surfnum=1;
	for(map<string,Surface2D>::const_iterator i=surfaces.begin(); i!=surfaces.end(); ++i){
		fs << "S " << surfnum++ << " " << (i->second.from) << " " << (i->second.to) << " "
			<< i->second.obs << " " << i->second.bas << " " << i->second.cmb << " " 
			<< i->second.epsilon <<  " " << i->first << endl;
	}
	fs << "E" << endl;
	fs.close();
	
	return 0;
}


