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
*//* 
SWIG interface definition file. This defines a 'wrapping' of the
viewfactorsaxi function in a form that can be accessed from the Python
language, using NumPy arrays or Python lists or tuples as the data interchange
format.

Perhaps we will extend this to allow view factors to be calculated from Python
for other geometries, too, eventually!

See http://docs.scipy.org/doc/numpy/reference/swig.interface-file.html
for information on the implementation details.

John Pye, 14 Mar 2013
*/
%feature("autodoc", "1");
%module viewax

%{
#define SWIG_FILE_WITH_INIT
#include "viewfact.h"
%}

%include "numpy.i"

%init %{
import_array();
%}

%apply (int IN_ARRAY2[], int DIM1, int DIM2) {(int *surf, int nsurf, int nvert), (double *coord, int npoint, int ndim)};
%apply (double* INPLACE_ARRAY2, int DIM1, int DIM2 ) {(double *vf, int nsurf1, int nsurf2)};
%rename(viewax)viewfactorsaxi_np;

%inline %{
int viewfactorsaxi_np(
	int *surf, int nsurf, int nvert
	, double *coord, int npoint, int ndim
	, double *vf, int nsurf1, int nsurf2
	, int idiv, int fast
){
	if(nsurf<=1)return 3;
	if(nsurf1!=nsurf)return 1;
	if(nsurf2!=nsurf)return 2;
	if(nvert!=2)return 4;
	if(ndim!=2)return 5;
	if(idiv<1)return 6;

	/* TODO check that npoint agrees with surf contents? */
	
	viewfactorsaxi(nsurf, surf, coord, vf, idiv, fast);
	return 0;
}

%}
