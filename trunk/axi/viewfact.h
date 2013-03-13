/*  viewfact.h: calculation of axisymmetric view factors
	Copyright (C) 1995 Juha Katajamäki
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
// http://www.csc.fi/english/pages/elmer --- http://www.elmerfem.org/

/**
	Calculate view factors for axisymmetric geometry.

	The function uses global variables to store the geometry, and so is not
	thread-safe.

	@param n number of surface elements
	@param surf[2*n] facets (a,b,..), stored as pairs like i1_a, i2_a, i1_b, i2_b,... where i1 and i2 are indices into the list of pairs 'crd'.
	@param crd[???] coordinates, as pairs like r0, z0, r1, z1, r2, z2,...

	The number of elements in the 'crd' array can possibly be determined by
	looking at the maximum index contained in the srf array...? Need to examine
	the logic here to see how it works.

	Each facet is described by an index pair (i1,i2), which are indices into the
	crd array (the '0', '1' etc in the pairs example).

	TODO wouldn't it be easier to use some structs for this lot?

	@param fast boolean, if non-zero, combine original boundary elements for shading calculation. if zero, don't combine (check this??)
	@param idiv looks like a number of subdivision of the specified surfaces, not sure what it's being used for yet (check this??)

	@param vf pointer to an array of size n^2, such that vf[i*n+j] is the view factor from surface i to surface j (TODO check i vs j direction)

	@return Returns the values vf by writing to the vf array.
*/
void viewfactorsaxi(int surf[], int nsurf, double crd[], double *vf, int idiv, int fast);

