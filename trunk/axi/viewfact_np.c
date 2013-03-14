#include "viewfact_np.h"
#include "viewfact.h"

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

