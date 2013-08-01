/*  viewfact.cpp: calculation of axisymmetric view factors
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
*//** @file
	This subroutine computes the viewfactors for an axisymmetric
	geometry. The code is written by Juha Katajamäki while
	working for CSC.

	http://www.csc.fi/english/pages/elmer --- http://www.elmerfem.org/
*/

/* 'TR:' comments are translated from Finnish and need review -- JP */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// enable floating point errors to trip code, need to catch exceptions in Python!
#include <fenv.h>

// #define abs(x) (((x) >= 0) ? (x) : (-(x)))
#define SQ(x) ((x)*(x))
#define min(x, y) ( ((x) < (y)) ? (x) : (y) )
#define max(x, y) ( ((x) > (y)) ? (x) : (y) )
#define sgn(x) ( ((x) < 0.) ? (-1.) : (((x) > 0.) ? (1.) : 0.) )
#define TRUE 1
#define FALSE 0

// forward definitions

void Viewfactor(const int **surfEltop, const double *coord,double **vf, int div);
int InitialInterval(double *c1, double *c2);
double ViewIntegral (double c1, double c2, int k);
int IntervalIsect(double x1, double x2, double y1, double y2, double *z1, double *z2);
void ExaminePoint (double x, double *mi, double *ma);
double Integrate(double c1, double c2);
double Area(double r1, double r2, double z1, double z2);


// lots of nasty, naughty global variables

static double r1, r2, z1, z2;     /* TR: "Watching" surface coordinates */
static double r3, r4, z3, z4;     /* TR: viewing surface coordinates */
static double r12, r34, z12, z34; /* TR: Central Line coordinate */
static double zd1, zd3, zd;
static double rd1, rd3;

static double g1, g3, d1, d3, t, rratio;

static const double eps = 1e-7, eps2 = 1.0e-14; /* eps^2 */
static const double delta = 1e-6; /* Maximum cosine (TR?) difference which causes integration */

static int nsurf,nsurfShade,inode,jnode;
static double *coord;
static int *surfEltop, *surfEltopShade, *shadeParent;

static int compact = 1, verify = 0, selfshading = 1;

/*----------------------------------------------------------------------------*/
/*
        Main entry point... see header file for argument description.
*/
void viewfactorsaxi(int n, int surf[], double crd[], double *vf, int idiv, int fast){
  int i, j, ii, jj,div;
  double a, sum, viewint, viewint2, vf2, sumdvf;
  double c1, c2;    /*  cosine of the rotation angle upper and lower limit  */
  double _r1, _r2, _r3, _r4, _z1, _z2, _z3, _z4;
  double ds1,ds2,dp1,dz1,dz2,dr1,dr2,err,maxerr;
  double epsilon = 1.0e-5;

  feenableexcept(-1);

  /* store the geometry in some global variables for access from the other functions */
  nsurf = n;
  coord = crd;
  surfEltop = surf; 
  
  div = idiv;
  compact = fast;


  if(!compact) {
    printf("Using original set of boundary elements for shading\n");
    nsurfShade = nsurf;
    surfEltopShade = surfEltop;
  }

  if(compact) {
    int *nodehits,*nodetable;
    double _r0, _z0;
    int ind0,ind1,ind2;

    printf("Combining original boundary elements for shading\n");

    int maxind = 0;
    for (i=0; i<2*nsurf; i++) 
      if(maxind < surfEltop[i]) maxind = surfEltop[i];
    // printf("Maximum node index is %d\n",maxind);

    // nodehits = new int[maxind+1];
    nodehits = (int*) malloc((maxind+1)*sizeof(int));
    for(i=0;i<=maxind;i++)
      nodehits[i] = 0;
    for (i=0; i<2*nsurf; i++)  
      nodehits[surfEltop[i]] += 1; 

    int maxnodehits = 0;
    for (i=0; i<=maxind; i++) 
      if(nodehits[i] > maxnodehits) maxnodehits = nodehits[i];
    // printf("Maximum node hits is %d\n",maxnodehits);
    
    int tablesize = (maxind+1)*maxnodehits;
    // nodetable = new int[tablesize];
    nodetable = (int*) malloc(tablesize*sizeof(int));
    for (i=0; i< tablesize; i++) 
      nodetable[i] = 0;

    for(i=0;i<=maxind;i++)
      nodehits[i] = 0;
    for (i=0; i<nsurf; i++) {
      ind1 = surfEltop[2*i+1];
      ind2 = surfEltop[2*i+0];
      nodetable[maxnodehits*ind1 + nodehits[ind1]] = i;
      nodetable[maxnodehits*ind2 + nodehits[ind2]] = i;
      nodehits[ind1] += 1;
      nodehits[ind2] += 1;
    }
 
    // surfEltopShade = new int[2*nsurf];
    surfEltopShade = (int*) malloc(2*nsurf*sizeof(int));
    for(i=0;i<2*nsurf;i++)
      surfEltopShade[i] = surfEltop[i];
 
    for (i=0; i<=maxind; i++) {
      int elem1,elem2;
      ind0 = i;
      
      if( nodehits[ind0] != 2) continue;

      elem1 = nodetable[maxnodehits*ind0+0];
      if( surfEltopShade[2*elem1+1] == ind0 ) 
	ind1 = surfEltopShade[2*elem1];
      else 
	ind1 = surfEltopShade[2*elem1+1];

      elem2 = nodetable[maxnodehits*ind0+1];
      if( surfEltopShade[2*elem2+1] == ind0 ) 
	ind2 = surfEltopShade[2*elem2];
      else 
	ind2 = surfEltopShade[2*elem2+1];

      _r0 = coord[2 * ind0];
      _r1 = coord[2 * ind1];
      _r2 = coord[2 * ind2];
      
      _z0 = coord[2 * ind0 + 1];
      _z1 = coord[2 * ind1 + 1];
      _z2 = coord[2 * ind2 + 1];

      dr1 = _r1 - _r0;
      dr2 = _r2 - _r0;
      dz1 = _z1 - _z0;
      dz2 = _z2 - _z0;
      
      dp1 = dr1 * dr2 + dz1 * dz2;
      ds1 = sqrt(dr1*dr1+dz1*dz1);
      ds2 = sqrt(dr2*dr2+dz2*dz2);
      
      dp1 /= (ds1*ds2);

      // Boundary elements mush be aligned
      if( dp1 > epsilon - 1. ) continue;

      // printf("Eliminating node %d\n",ind0);
      
      // Make the 1st element bigger 
      if( surfEltopShade[2*elem1] == ind0 ) 
	surfEltopShade[2*elem1] = ind2;
      else 
	surfEltopShade[2*elem1+1] = ind2;
      
      // Destroy the 2nd element 
      surfEltopShade[2*elem2] = 0;
      surfEltopShade[2*elem2+1] = 0;

      // Update the node information 
      nodehits[ind0] = 0;
      if( nodetable[maxnodehits*ind2] == elem2) 
	nodetable[maxnodehits*ind2] = elem1;
      else 
	nodetable[maxnodehits*ind2+1] = elem1;
    }

    // Free, not needed anymore
    free((char*)(nodetable));

    // Cannibalism of already used vector which does not need to be used again!
    shadeParent = nodehits;
    for (i=0; i<nsurf; i++) 
      shadeParent[i] = -1;
    
    j = 0;
    for (i=0; i<nsurf; i++) {
      if(surfEltopShade[2*i+1] || surfEltopShade[2*i+0]) {
	surfEltopShade[2*j+1] = surfEltopShade[2*i+1];
	surfEltopShade[2*j+0] = surfEltopShade[2*i+0];	
	j++;
      }
    }
    nsurfShade = j;
    printf("The combined set includes %d line segments (vs. %d)\n",nsurfShade,nsurf);


    // This is a dummy N^2 algorithm of finding the parent superelements
    // The info could also be inhereted in time of creating the superelements...
    
    int k, hit, parents = 0;
    maxerr = 0.;
    for (i=0; i<nsurfShade; i++) {
      
      ind1 = surfEltopShade[2*i];
      ind2 = surfEltopShade[2*i+1];
      
      _r1 = coord[2 * ind1];
      _r2 = coord[2 * ind2];
      
      _z1 = coord[2 * ind1 + 1];
      _z2 = coord[2 * ind2 + 1];
      
      dz1 = _z2 - _z1;
      dr1 = _r2 - _r1;
      ds1 = sqrt(dz1*dz1 + dr1*dr1);
      
      // Unit vector in direction of superelement
      dz1 /= ds1;
      dr1 /= ds1;

       
      for (j=0; j<nsurf; j++) {

	if( shadeParent[j] >= 0) continue;
	hit = 1;
	
	for(k=0;k<2;k++) {
	  
	  ind0 = surfEltop[2*j+k];
	  
	  // if node is joined, it is still a good candidate
	  // this check avoids also singularity at division
	  if(ind0 == ind1 || ind0 == ind1) continue;
	  
	  _r3 = coord[2 * ind0];
	  _z3 = coord[2 * ind0 + 1];
	  
	  dz2 = _z3 - _z1;
	  dr2 = _r3 - _r1;
	  ds2 = sqrt(dz2*dz2 + dr2*dr2);
	  
	  // Dot product of the superelement and the candidate-node element
	  dp1 = dz1*dz2 + dr1*dr2;
	  
	  // check that the node is on the line defined by the superelement
	  if( dp1 / ds2 < 1-epsilon ) {
	    hit = 0;
	    break;
	  }
	  
	  // check that node is within the segment of the superelement
	  if( dp1 / ds1 > 1+epsilon  || dp1 / ds1 < -epsilon) {
	    hit = 0;
	    break;
	  }
	}

	if(hit) {
	  shadeParent[j] = i;
	  parents++;
	}
      }
    }
    if(parents != nsurf) printf("Inconsistent number of parents found %d (vs. %d)\n",parents,nsurf);
    
    // delete [] nodetable;
    // delete [] nodehits;
  }


  // ************************************************************
  // The main N^2*M loop where M is the size of the shading table  
  for (i=0; i<nsurf; i++) {

    inode = i;    
    sum = 0.;
    sumdvf = 0.;

    _r3 = coord[2 * surfEltop[2*i+1]];
    _r4 = coord[2 * surfEltop[2*i+0]];
    
    _z3 = coord[2 * surfEltop[2*i+1]+1];
    _z4 = coord[2 * surfEltop[2*i+0]+1];

    
    a = Area(_r3, _r4, _z3, _z4);

    for (j=0; j<nsurf; j++) {

      jnode = j;
      _r1 = coord[2 * surfEltop[2*j+1]];
      _r2 = coord[2 * surfEltop[2*j+0]];
      
      _z1 = coord[2 * surfEltop[2*j+1]+1];
      _z2 = coord[2 * surfEltop[2*j+0]+1];
      
      vf[i*nsurf+j] = 0.;
      vf2 = 0.;
 
      if (a < eps) continue;

      for (ii=0; ii<div; ii++) {
	r3 = _r3 * (div - ii)/div + _r4 * ii/div;
	r4 = _r3 * (div - ii - 1)/div + _r4 * (ii + 1)/div;
	z3 = _z3 * (div - ii)/div + _z4 * ii/div;
	z4 = _z3 * (div - ii - 1)/div + _z4 * (ii + 1)/div;

	r34 = .5*(r3+r4);
	z34 = .5*(z3+z4);
	zd3=z3-z4;
	rd3=r3-r4;
	
	for (jj=0; jj<div; jj++) {
	  r1 = _r1 * (div - jj)/div + _r2 * jj/div;
	  r2 = _r1 * (div - jj - 1)/div + _r2 * (jj + 1)/div;
	  z1 = _z1 * (div - jj)/div + _z2 * jj/div;
	  z2 = _z1 * (div - jj - 1)/div + _z2 * (jj + 1)/div;
	  r12 = .5*(r1+r2);
	  if ( r12 < eps || r34 < eps) continue;

	  if (r1 < eps) r1 = eps;
	  if (r2 < eps) r2 = eps;
	  if (r3 < eps) r3 = eps;
	  if (r4 < eps) r4 = eps;
	  
	  zd1=z1-z2;
	  rd1=r1-r2; 
	  z12 = .5*(z1+z2);
	  zd = z12-z34;
	  
	  if (!InitialInterval(&c1, &c2)) continue;	  
	  viewint = ViewIntegral(c1, c2, 0);

	  // Code for verification
	  if(verify) {
	    int *surfEltopTmp;
	    int nsurfTmp;
	    
	    surfEltopTmp = surfEltopShade;
	    nsurfTmp = nsurfShade;	    
	    
	    surfEltopShade = surfEltop;
	    nsurfShade = nsurf;
	    	    
	    if (!InitialInterval(&c1, &c2)) continue;	  
	    viewint2 = ViewIntegral(c1, c2, 0);	    
	    vf2 = vf2 + 4. * viewint2;

	    surfEltopShade = surfEltopTmp;
	    nsurfShade = nsurfTmp;
	  }	      	    

	  vf[i*nsurf+j] += 4. * viewint;
	  /* TR: The factor 4 is composed of two factors (reflective symmetry), 2pi (rotational) and 1/pi (the integral expression the constant) */
	}
      }

      vf[i*nsurf+j] /= a;
      sum += vf[i*nsurf+j];

      if(verify) {
	vf2 /= a;
	sumdvf += ( fabs(vf[i*nsurf+j]-vf2 ) / a);
      }
    }

    if(verify) {
      if(sumdvf > maxerr) maxerr = sumdvf;    
      printf("Line sum: %d %g %g %g\n", i, sum, sumdvf, maxerr);
    }
    else {
      //      printf("Line sum: %d %g\n", i, sum );      
    }
  }


  // Deallocate stuff
  if(compact) {
    free((char*)(surfEltopShade));
    free((char*)(shadeParent));
  }

}

/** 
        TR: The number of cross-watching point of the rotation angle cos the condition that the line connecting the surfaces and angles between the normal must be < pi/2.
        Function assumes that r12 and r34 are non-zero.

        @return FALSE if the solution set is empty or zero-length, otherwise TRUE. 
*/
int InitialInterval(double *c1, double *c2){

  double cc1, cc3; 
  
  *c1 = -1.; *c2 = 1.;
  if(fabs(zd1) > eps){
    cc1 = (- zd * rd1 + r12 * zd1) / (r34 * zd1);        
    if(fabs(zd3) > eps) {
      cc3 = (zd * rd3 + r34 * zd3) / (r12 * zd3);        
      if(zd1 > 0.) {
	if(zd3 > 0.)*c1 = max(cc1, cc3);
	else{*c1 = cc1; *c2 = cc3;}
      }else{
	if(zd3 < 0.)*c2 = min(cc1, cc3);
	else{*c1 = cc3; *c2 = cc1;}
      }
    }else{
      if(sgn(rd3) && sgn(rd3) == -sgn(zd)){
	if (zd1 > 0.)*c1 = cc1;
	else *c2 = cc1;
      }else{
        *c1 = 1.; *c2 = -1.; /* TR: Number of items */
      }
    }
  }else{
    if(fabs(zd3) > eps){
      cc3 = (zd * rd3 + r34 * zd3) / (r12 * zd3);
      if(sgn(rd1) && sgn(rd1) == sgn(zd)){
	if(zd3 > 0.)*c1 = cc3;
	else *c2 = cc3;
      }else{*c1 = 1.; *c2 = -1.;} /* TR: Number of items */
    }else{
      if(!sgn(rd1) || sgn(rd1) != sgn(zd) || sgn(rd1) != -sgn(rd3)){
        *c1 = 1.; *c2 = -1.; /* Otherwise number = [-1, 1] */
      }
    }
  }
  
  *c1 = max(-1.+eps, *c1); *c2 = min(1.-eps, *c2);
  /* TR: Epsilon prevent division by zero integration */
  if (*c2 - *c1 < eps) return FALSE;
  return TRUE;
}


double ViewIntegral (double c1, double c2, int k){
/**<
TR: This function calculates the view Factor one element pair.
The area of ​​integration is limited to the examination of the conical surfaces of the induced
shading. If the integration region is divided into two parts, performed a recursive
call on both parts. If the integration region shrinks void
or empty, is returned to zero.
Function assumes global variables r12 and r34 is non-zero.
*/

  static double r5, r6, z5, z6;    /* TR: to shade the edges of the surface coordinates */
  static double zd5, t1, tt1, t2, tt2, t0;
  double cc1,cc2;
  rratio = r34/r12;

  while (k < nsurfShade) {
    
    r5 = coord[2 * surfEltopShade[2*k+1]];
    r6 = coord[2 * surfEltopShade[2*k+0]];
    
    z5 = coord[2 * surfEltopShade[2*k+1]+1];
    z6 = coord[2 * surfEltopShade[2*k+0]+1];

    k++;

    // either element cannot shade one another
    if(selfshading){
      // Condition for superelements
      if(nsurf != nsurfShade){
	if(k-1 == shadeParent[inode] || k-1 == shadeParent[jnode]) continue;
      }
      else if(nsurf == nsurfShade){
	if(k-1 == inode || k-1 == jnode) continue;
      }
    }

    if(r5+r6 < eps) continue;


    zd5 = z5-z6;
    if(fabs(zd5) < eps){
      /*  TR: Shade the surface is flat ring */
      
      /* TR: Level Ring can not be overshadowed by itself  */
      /* TR: This appendix addresses the subroutine for a long time, the bug (PR 23/04/2004) */
      if(nsurf == nsurfShade && inode == k-1) continue;

      if(fabs(zd) < eps)continue;

      t1 = (z12-z5)/zd; 
      tt1 = 1.-t1;
      if(t1 < eps || tt1 < eps) continue;


      t = rratio * t1/tt1;
      cc1 = .5*(r5*r5/(r12*r34*t1*tt1) - t - 1./t);
      cc2 = .5*(r6*r6/(r12*r34*t1*tt1) - t - 1./t);

      if(cc1 > cc2){ t = cc1; cc1 = cc2; cc2 = t;}
    }else{
      /* TR: Shade the surface is a cone or a cylinder */
      /* TR: Calculate the line connecting the shadow of the intermediate remaining in the z-direction */

      if(fabs(zd) < eps ) {
	if((z12-z5 < eps && z12-z6 > eps) ||
	     (z12-z5 > eps && z12-z6 < eps) ){
          t1 = 0.; t2 = 1;
        }else continue;
      }else{
	t1 = (z12-z5)/zd; 
	t2 = t1 + zd5/zd;
	if(t1 > t2){
          t = t1; t1 = t2; t2 = t;
        }
      }

      if(!IntervalIsect(0., 1., t1, t2, &t1, &t2))continue;
      tt1 = 1.-t1; 
      tt2 = 1.-t2;
      
      /* TR: Calculate the values ​​of cosine of the angle of rotation may be in the interval [t1, t2] */
      cc1 = 1.; cc2 = -1.;
      g1 = (r5 * (z12-z6) - r6 * (z12-z5)) / (r12 * zd5);
      g3 = (r5 * (z34-z6) - r6 * (z34-z5)) / (r34 * zd5);
      d1 = g1*g1 - 1; 
      d3 = g3*g3 - 1;  
      /* TR: These indicate which of the two */
      /* TR: Side of the cone are watching and watch point */

      /* TR: Study interval endpoint */
      ExaminePoint (t1, &cc1, &cc2);
      ExaminePoint (t2, &cc1, &cc2);

      /* TR: If both point outside the cone, to investigate the derivative */
      /* TR: Zero, if it is in the interval [t1, t2] */
      if(d1 <= -eps && d3 <= -eps) {
	t0 = 1. / (1. + sqrt(rratio * d3/d1));
	if(t0 - t1 > eps && t2 - t0 > eps) {
	  ExaminePoint(t0, &cc1, &cc2);
	}
      }
      if(cc1 > cc2) {
	cc1 = cc2; /* TR: This can happen due to rounding */
      }
    }


    if (IntervalIsect(c1, c2, cc1, cc2, &cc1, &cc2)) {
      if (cc1 - c1 < delta) {
	if (c2 - cc2 < delta) {
	  return 0.;
	}else{
	  c1 = cc2;
	}
      }else if (c2 - cc2 < delta) {
	c2 = cc1;
      }else{
	return ViewIntegral(c1, cc1, k) + ViewIntegral(cc2, c2, k);
      }
    }
  }
  return Integrate(c1, c2);
}


int IntervalIsect(double x1, double x2, double y1, double y2, double *z1, double *z2){
  /* TR: Calculate the interval [x1, x2] and [y1, y2] section and return FALSE
  if this is blank or null and void. Order of the input parameters must be correct.
  */
  
  *z1 = x1; *z2 = x2;
  if (x2 - y1 < eps) return FALSE;
  if (y1 - x1 > eps) *z1 = y1;
  if (y2 - x1 < eps) return FALSE;
  if (x2 - y2 > eps) *z2 = y2;
  return (*z2 - *z1 >= eps);
}


void ExaminePoint (double x, double *mi, double *ma){
  double y;
  if(x > eps) {
    if(1.-x > eps) {
      t = rratio*x/(1.-x);
      y = .5*(d1/t + d3*t) + g1*g3;
    }else if(fabs(d3) < eps){
      y = g1*g3;
    }else{
      y = sgn(d3);
    }
  }else if(fabs(d1) < eps ){
    y = g1*g3;
  }else{
    y = sgn(d1);
  }

  if (y > *ma) *ma = y;
  if (y < *mi) *mi = y;
}


double Integrate(double c1, double c2){
  /* TR: C1 and c2 are the cosine of the angle of the integration interval boundaries. */ 
  /* TR: The integral is calculated without the denominator of pi-factor. */
  
  /* TR: The first and last point of integration can not be exactly 0 and 1, so that adjacent elements in the case of division by zero should not. */
  /*	static const double qp[] = { 1e-6, .25, .5, .75, 1.-1e-6 }, */
  /*    					w[] = { 1./12., 1./3., 1./6., 1./3., 1./12. }; */
  /*  static const double qp[] = { 0.211324865, 0.788675134 }, */
  /*	    w[] = { .5, .5 }; */
  static const double qp[] = { 0.112701665, 0.5, 0.887298334 },
			     w[] = { 0.277777777, 0.444444444, 0.277777777 };
  static const int nqp = 3;
    
  int i;
  double c = zd1*zd1 + rd1*rd1;
  if (c < eps2) return 0.; /* TR: Surface shrunk circular arcs, this test */
  /* TR: Needed to avoid division by zero */
  
  double z, r, h, hh1, hh2, g1, g2, gg1, gg2, value, integral;
  double d1, d2, e1, e2, f1, f2;
  double zrd = r2*z1-r1*z2;
  double a1 = rd3*r1, a2 = rd3*r2;
  double b1 = zd3*z1, b2 = zd3*z2; 
  double s1 = sqrt(1. - c1*c1), s2 = sqrt(1. - c2*c2);
  /* TR: machines and articles for the indices 1 and 2 of the other way around as the other variables! */
  double cs = (1.+c1)*(1.+c2), cd = (1.-c1)*(1.-c2);

  
  integral = 0.;
  for (i=0; i<nqp; i++) {
    z = z3 - qp[i] * zd3;  /* TR: qp is the integration of variable */
    r = r3 - qp[i] * rd3;
    e1 = (z1-z)*(z1-z) + r1*r1 + r*r;
    f1 = (z2-z)*(z2-z) + r2*r2 + r*r;
    hh1 = 2*r1*r;
    hh2 = 2*r2*r;
    g1 = - e1 / hh1;
    g2 = - f1 / hh2;
    e2 = e1 - c1*hh1;
    f2 = f1 - c1*hh2;
    e1 -= c2*hh1;
    f1 -= c2*hh2;
    h = zd3*z + rd3*r;
    gg1 = (g1+c2)*(g1+c1);
    gg2 = (g2+c2)*(g2+c1);
    
    /* TR: Curves share: */
    value  = (-.5 * (a1 + (h-b1)*g1) / sqrt(g1*g1-1) ) *
      acos( .5 * ( (1.-g1) * sqrt(cd/gg1) - 
		   (1.+g1) * sqrt(cs/gg1) ) );
    value -= (-.5 * (a2 + (h-b2)*g2) / sqrt(g2*g2-1) ) *
      acos( .5 * ( (1.-g2) * sqrt(cd/gg2) - 
		   (1.+g2) * sqrt(cs/gg2) ) );
    value += .25 * (b1-b2) * acos(c1*c2 + s1*s2);

    /* TR: Direct proportion pages: */
    gg1 = e1+f1-c; gg2 = e2+f2-c;
    hh1 = 4*e1*f1; hh2 = 4*e2*f2;
    d1 = hh1 - gg1*gg1; d2 = hh2 - gg2*gg2;
    h = r * (rd1*h + zrd*zd3);
    value -= h * (s1 / sqrt(d2)) * acos( gg2 / sqrt(hh2) );
    value += h * (s2 / sqrt(d1)) * acos( gg1 / sqrt(hh1) );

    integral += w[i] * value;
  }

  return integral;
}


double Area(double r1, double r2, double z1, double z2){
  if(z1==z2){
    if(r1==r2)return 0;
    if(r1==0) return M_PI*(double)r2*(double)r2;
    else if(r2==0)return M_PI*SQ(r1);
    else return M_PI * fabs(SQ(r2) - SQ(r1));
  }else{
    return M_PI * (r1+r2) * sqrt(SQ(z1-z2) + SQ(r1-r2));
  }
}

/* vim: ts=8 et */
