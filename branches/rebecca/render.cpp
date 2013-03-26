/*	VIEW3D
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
*/
#include "render.h"

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoLOD.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoCoordinate3.h>

#include <iostream>
#include <cstdlib>
#include <sstream>
using namespace std;

const SbColor WHITE(1,1,1);
const SbColor RED(1,0,0);
const SbColor GREEN(0,1,0);
const SbColor YELLOW(1,1,0);
const SbColor BLUE(0.5,0.5,1);
const SbColor ORANGE(1,0.6,0);
const SbColor PURPLE(1,0,1);
const SbColor CYAN(0,1,1);

static float angle(const SbVec3f &a, const SbVec3f &b){
	SbVec3f c(a.cross(b));
	return atan2(c.length(),a.dot(b));
}


SoSeparator *text(const SbVec3f &left, const char *str, const SbColor &c){
	SoSeparator *s = new SoSeparator;
	
	SoBaseColor *col = new SoBaseColor;
	col->rgb = c;
	s->addChild(col);

	SoTranslation *tr = new SoTranslation;
	tr->translation = left;
	s->addChild(tr);

	SoText2 *txt = new SoText2;
	txt->string.setValue(str);
	s->addChild(txt);

	return s;
}

SoSeparator *sphere(const SbVec3f &C, const double &r, const SbColor &c){
	SoSeparator *s = new SoSeparator;

	SoBaseColor *col = new SoBaseColor;
	col->rgb = c;
	s->addChild(col);

	SoTranslation *tr = new SoTranslation;
	tr->translation = C;
	s->addChild(tr);

	SoSphere *sph = new SoSphere;
	sph->radius = r;
	s->addChild(sph);

	return s;
}	

SoSeparator *cylinder(const SbVec3f &A, const SbVec3f &B, const double &radius, const SbColor &c){

	SoSeparator *s = new SoSeparator;

	SbVec3f AB(B - A);
	AB.normalize();
	SbVec3f d(AB);
	SbVec3f m(float(0.5) * (A + B));

	SbVec3f z(0,1,0);
	SbVec3f a = z.cross(d);
	double theta = angle(z,d);
	if(a.length() < 1e-6){
		a = SbVec3f(0,0,1);
		theta = 0;
	}

	SoBaseColor *col = new SoBaseColor;
	col->rgb = c;
	s->addChild(col);

	//cerr << "translation = " << m << endl;
	SoTranslation *tr = new SoTranslation;
	tr->translation = m;
	s->addChild(tr);

	//cerr << "rotation = " << theta * 180./PI << " deg around " << a << endl;
	SoRotation *rot = new SoRotation;
	rot->rotation = SbRotation(a,theta);
	s->addChild(rot);

	//SoComplexity *cplx = new SoComplexity;
	//cplx->type = SoComplexity::SCREEN_SPACE;
	//s->addChild(cplx);

	SoCylinder *cyl = new SoCylinder;
	cyl->height = (A-B).length();
	cyl->radius = radius;
	s->addChild(cyl);

	return s;
}

/**
	Cone with its axis on AB, with its base at A and its apex at B. The base
	radius is r.
*/
SoSeparator *cone(const SbVec3f &A, const SbVec3f &B, const double &r, const SbColor &c){

	SoSeparator *s = new SoSeparator;

	SbVec3f AB(B - A);
	AB.normalize();
	SbVec3f d(AB);
	SbVec3f m(float(0.5) * (A + B));

	SbVec3f z(0,1,0);
	SbVec3f a = z.cross(d);
	double theta = angle(z,d);
	if(a.length() < 1e-6){
		a = SbVec3f(0,0,1);
		theta = 0;
	}

	SoBaseColor *col = new SoBaseColor;
	col->rgb = c;	
	s->addChild(col);

	//cerr << "translation = " << m << endl;
	SoTranslation *tr = new SoTranslation;
	tr->translation = m;
	s->addChild(tr);

	//cerr << "rotation = " << theta * 180./PI << " deg around " << a << endl;
	SoRotation *rot = new SoRotation;
	rot->rotation = SbRotation(a,theta);
	s->addChild(rot);

	SoCone *cyl = new SoCone;
	cyl->height = (B-A).length();
	cyl->bottomRadius = r;
	s->addChild(cyl);

	return s;
}

SoSeparator *axes(const double &size, double thickness,bool labelled){

	if(thickness==0){
		thickness = size/20.;
	}

 	SoSeparator *s = new SoSeparator;
	s->addChild(cylinder(SbVec3f(0,0,0), SbVec3f(size,0,0), thickness, RED));
	s->addChild(cone(SbVec3f(size,0,0), SbVec3f(size+3*thickness,0,0), 2*thickness, RED));

	s->addChild(cylinder(SbVec3f(0,0,0), SbVec3f(0,size,0), thickness, GREEN));
	s->addChild(cone(SbVec3f(0,size,0), SbVec3f(0,size+3*thickness,0), 2*thickness, GREEN));

	s->addChild(cylinder(SbVec3f(0,0,0), SbVec3f(0,0,size), thickness, BLUE));
	s->addChild(cone(SbVec3f(0,0,size), SbVec3f(0,0,size+3*thickness), 2*thickness, BLUE));

	if(labelled)s->addChild(text(SbVec3f(0,0,size+5*thickness),"Z"));
	if(labelled)s->addChild(text(SbVec3f(0,size+5*thickness,0),"Y"));
	if(labelled)s->addChild(text(SbVec3f(size+5*thickness,0,0),"X"));

	return s;
}

SoSeparator *arrow(const SbVec3f &A, const SbVec3f &B, const SbColor &c, const char *label, double thickness){
	SoSeparator *s = new SoSeparator;
	SbVec3f Bs = B + float(3. * thickness) * (A-B);
	s->addChild(cylinder(A,Bs,0.5*thickness, c));
	s->addChild(cone(Bs,B,thickness, c));
	if(label!=NULL){
		s->addChild(text(float(0.5)*(A+Bs)+SbVec3f(thickness,thickness,thickness), label, c));
	}
	return s;
}

/**
	Return a 'face set' containing just a single face.
	@param n direction of the surface normal for the face
	@param vertices the vertices lying on the face
	@param c color in which to render it.
*/
SoSeparator *face(const SbVec3f &n, const vector<SbVec3f> &vertices, const SbColor &c){
	SoSeparator *s = new SoSeparator;

	SoNormal *n1 = new SoNormal;
	n1->vector.setValues(0,1, &n);
	s->addChild(n1);	

	SoNormalBinding *nb = new SoNormalBinding;
	nb->value = SoNormalBinding::PER_FACE;
	s->addChild(nb);

	SoBaseColor *col = new SoBaseColor;
	col->rgb = c;
	s->addChild(col);

	SoCoordinate3 *cor = new SoCoordinate3;
	cor->point.setValues(0,vertices.size(),&(vertices[0]));
	s->addChild(cor);
	
	SoFaceSet *fs = new SoFaceSet;
	const int32_t nv = vertices.size();
	fs->numVertices.setValues(0,1,&nv);
	s->addChild(fs);

	return s;
}

