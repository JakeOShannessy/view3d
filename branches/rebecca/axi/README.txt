
Axisymmetric radiation view factor calculation
==============================================

This directory contains code sourced from the ELMER FEM project, a
computational fluid dynamics / multiphysics software.

The code here implements calculation of view factors for axi-symmetriic
geometries (only).

The code is not yet integrated with the rest of View3D, but we have
placed it here because of the potential interest to the View3D user
community, as well as in order to have a place where we can log changes
to the code.

The function prototype has been modified from the original ELMER code due to
inappropriate use of pointers to pass in read-only configuration parameters
and also the integer number of surfaces. The number of surfaces has been moved
after the srf array for convenience in wrapping with SWIG.

-- 
John Pye
14 Mar 2013
