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

vf = np.zeros((3,3))
viewax.viewfactorsaxi_np(srf,crd,vf,20,0)

print vf

