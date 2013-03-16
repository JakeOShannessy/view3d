from math import pi, tan
import numpy as np
import viewax

aaa = [1,10,100];

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
], dtype=np.float)

srf = np.array([
	[3,0] 
	, [1,2]
	, [2,3]
], dtype=np.int)

# in the current wrapping we need to allocate the returned array, and then 
# the values are returned by in-place modification of the array by our function.
# TODO wrap this function with an extra layer that also checks the return code
# for errors.
vf = np.zeros((srf.shape[0],srf.shape[0]))
viewax.viewfactorsaxi_np(srf,crd,vf,20,0)

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

