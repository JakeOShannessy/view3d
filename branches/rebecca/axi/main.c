/*  View3D: calculation of axisymmetric view factors
	Copyright (C) 2013 John Pye

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "viewfact.h"
#include <stdio.h>
#include <math.h>

int main(void){

	double d, h, theta, phi;

	// specified geometry
	d = 0.6;
	h = 1;
	theta = 45 * M_PI/180;
	phi = 60 * M_PI/180;
	
	// calculated geometry
	double h1 = (tan(phi) - d/2) / (tan(phi) + tan(theta));
	double r2 = (h - (h*tan(phi) - d/2)/(tan(phi)+ tan(theta)))*tan(phi);

	// data structures to pass to viewfactorsaxi
#define n 3 //number of surface elements
	int ns = n;

	double crd[] = {
		  0, 0   // point 0
        , 0, h   // point 1	
		, r2, h1 // point 2
		, d/2, 0 // point 3
	};

	int srf[2*n] = {
		  3,0 // srf 0
		, 1,2 // srf 1
		, 2,3 // srf 2
	};

	double vf[n*n];
	int idiv, fast;
	idiv = 10;
	fast = 0;
	
	viewfactorsaxi(ns, srf, crd, vf, idiv, fast);

	int i,j;
	fprintf(stderr,"%3s | ","");
	for(j=0;j<n;++j){
		fprintf(stderr,"%s%8d",(j?"  ":""),j);
	}
	fprintf(stderr,"\n");
	fprintf(stderr,"----|");
	for(j=0;j<n;++j){
		fprintf(stderr,"----------");
	}
	fprintf(stderr,"\n");


	for(i=0; i<n; ++i){
		fprintf(stderr,"%3d | ",i);
		for(j=0; j<n; ++j){
			fprintf(stderr,"%s%8f",(j?"  ":""),vf[i*n+j]);
		}
		fprintf(stderr,"\n");
	}


	fprintf(stderr,"Hello world!\n");
	return 0;
}

