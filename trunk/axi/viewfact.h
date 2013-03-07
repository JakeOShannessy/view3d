/*  viewfact.h: calculation of axisymmetric view factors
	Copyright (C) 1995 Juha Katajamäki

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

// #define abs(x) (((x) >= 0) ? (x) : (-(x)))
#define min(x, y) ( ((x) < (y)) ? (x) : (y) )
#define max(x, y) ( ((x) > (y)) ? (x) : (y) )
#define sgn(x) ( ((x) < 0.) ? (-1.) : (((x) > 0.) ? (1.) : 0.) )
#define TRUE 1
#define FALSE 0


typedef int BOOL;
typedef double Real;

void Viewfactor(const int **surfEltop, const Real *coord,
				Real **vf, int div);
BOOL InitialInterval(Real *c1, Real *c2);
Real ViewIntegral (Real c1, Real c2, int k);
BOOL IntervalIsect(Real x1, Real x2, Real y1, Real y2, Real *z1, Real *z2);
void ExaminePoint (Real x, Real *mi, Real *ma);
Real Integrate(Real c1, Real c2);
Real Area(Real r1, Real r2, Real z1, Real z2);

