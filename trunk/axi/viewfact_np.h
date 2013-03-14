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

/**
	A refactored call to Juha Katajamäki's axisymmetric view factor calculation
	routine. This function is given parameters suitable for wrapping via SWIG
	using the 'numpy.i' conventions.

	@param surf Array of surface coordinate indices (nrows = nsurf, ncols = 2)
	@param nsurf Number of surfaces
	@param nvert Number of vertices per surface. Must be 2. numpy.i will want to give us this, even though its value can't be changed.
	@param coord Array of coordinates for vertices (nrows = npoint, ncols = 2)
	@param npoint Number of points specified in coord.
	@param ndim Number of dimensions in the specified points. Must be 2. numpy will want to give us this, even though its value must be 2.
	@param vf View factors, values modified in-place for return. Square array of size nsurf x nsurf (We will check nsurf1==nsurf and nsurf2==nsurf)
	@param idiv Number of divisions to make in shading calculations, TODO document this.
	@param fast Something about checking and merging, TODO document this.

	@return 0 on success, nonzero if there was an error in input dimensions
*/
int viewfactorsaxi_np(
	int *surf, int nsurf, int nvert
	, double *coord, int npoint, int ndim
	, double *vf, int nsurf1, int nsurf2
	, int idiv, int fast
);

