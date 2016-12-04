View3D - calculation of view factors between simple polygons.
------------------------------------------------------------
Author: George Walton
Owner: US Government.

Full details of this program and the algorithm that it implements are 
available from:

Walton, G. N.: "Calculation of obstructed view factors by adaptive integration",
Technical Report NISTIR–6925, National Institute of Standards and Technology,
Gaithersburg, MD, 2002.

http://www.bfrl.nist.gov/IAQanalysis/docs/NISTIR-6925.pdf

Abstract from the above report:

This report describes the use of adaptive integration for the calculation of 
view factors between simple convex polygons with obstructions. The accuracy of 
the view factor calculation is controlled by a convergence factor. The adaptive 
integration method is compared with two other common methods implemented in a 
modern computer program and found to have significant advantages in accuracy 
and even advantages in computational speed in some cases.

Details of the input file format and postprocessing steps are included
in the file MANUAL.doc included with this source code tarball.

Compiling View3D from source code
---------------------------------

Currently (Feb 2009) View3D is distributed as source code only. It has been
tested on Windows and Linux only, and works on both of these platforms.

In order to use the 2D viewer, you must have the GTK+ GUI library
and header files installed. On Ubuntu Linux this can be achieved using
"sudo apt-get install libgtk2.0-dev". On Windows, you should download
and unpack the Gtk+ 'bundle' from http://www.gtk.org/download-windows.html
then add the path to the 'bin' subdirectory therein to your PATH.

In order to use the 3D viewer, you must have the Coin3D and SoQt and Qt
librariexs and header files installed:
 * On Ubuntu 16.04, use "sudo apt install libsoqt4-dev"
 * On Ubuntu Linux 8.10, use "sudo apt-get install libsoqt3-dev".
 * On Ubuntu Linux 8.04, use "sudo apt-get install libsoqt-dev".
 * On Windows, download and install Qt 4.3.3, SoQt and Coin3D from the following 
   web page:
 http://ascendwiki.cheme.cmu.edu/Binary_installers_for_Coin3d_and_SoQt_on_MinGW

To build on Windows, you need to have MinGW, MSYS, Python and SCons installed.
Then, using MSYS, 'cd' to the source code directory, type "scons" and the
software should compile successfully. SCons version 1.2 or later is recommended,
because of the need for handling the '-mms-bitfields' option from pkg-config
for GTK when building the Viewer2D program.

To build on Linux, make sure you first install gcc and g++ and scons, then
run "scons" to build the software.

Building on Mac should be possible in principle, although it may require
that you build your own copy of SoQt from source first, if you want to use
the 3D viewer. Let me know if you have any success on this platform.

To summarise, the steps required to get up and running with View3D are:
1. install dependencies (SoQt, Coin3D, GTK) including header files
2. install development tools (gcc compiler, scons, python)
3. check out the source code from subversion, or download a tarball
4. unpack the tarball if applicable
5. enter the 'view3d' subdirectory and type 'scons'.
6. you should have the executables 'view3d', 'viewer', 'viewht' etc now
   present in your current directory. To run them you will need to modify
   your PATH (Windows) or LD_LIBRARY_PATH (Linux) to include the location
   of view3d.dll (Windows) or libview3d.so (Linux).

Any problems with the build on any of these platforms, please let me know.

-- 
John Pye
Feb 2009.

