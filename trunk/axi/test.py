from math import pi, tan
import numpy as np
import viewax

if 1:
	# four-parameter simple cavity

	# specified geometry
	d = 0.6;
	h = 1;
	theta = 45 * pi/180.;
	phi = 60 * pi/180.;
	
	# calculated geometry
	h1 = (tan(phi) - d/2) / (tan(phi) + tan(theta));
	r2 = (h - (h*tan(phi) - d/2)/(tan(phi)+ tan(theta)))*tan(phi);

	# data structures to pass to viewfactorsaxi
	ns = 3;

	crd = np.array([
		[0, 0]
		,[0, h]
		,[r2, h1]
		,[d/2, 0]
	], dtype=np.float64)

	srf = np.array([
		[3,0] 
		, [1,2]
		, [2,3]
	], dtype=np.int32)
else:
	# cylindrical cavity
	n = 20 # total number of surfaces
	h = 1
	d = 0.6
	s = n-2
	crd = np.zeros((n+1,2),dtype=np.float64)
	srf = np.zeros((n,2),dtype=np.int32)

	crd[0] = [0,0]
	crd[1] = [0,h]
	crd[2:n+1,0] = d/2
	crd[2:n+1,1] = h - np.arange(0.,s+1,dtype=np.float64)*h/s

	srf[1:n,0] = np.arange(1,n)
	srf[1:n,1] = np.arange(2,n+1)
	srf[0] = [n,0]
	srf[1] = [1,2]

# in the current wrapping we need to allocate the returned array, and then 
# the values are returned by in-place modification of the array by our function.
# TODO wrap this function with an extra layer that also checks the return code
# for errors.
vf = np.zeros((srf.shape[0],srf.shape[0]),dtype=np.float64)
viewax.viewfactorsaxi_np(srf,crd,vf,50,0)
np.set_printoptions(linewidth=150)
print vf

# plot the geometry
import pylab as pl
pl.figure()
for i in range(srf.shape[0]):
	s = srf[i]
	r = (crd[s[0],0], crd[s[1],0])
	z = (crd[s[0],1], crd[s[1],1])
	pl.plot(z,r,'-bo')
	pl.text(np.mean(z),np.mean(r),'%d'%i)
pl.axes().set_aspect('equal','datalim')
pl.show()

