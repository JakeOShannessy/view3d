cmake_minimum_required (VERSION 2.6)
project (View3D)

add_library (View3DLib v3lib.c
                       ctrans.c
                       heap.c
                       polygn.c
                       savevf.c
                       viewobs.c
                       viewunob.c
                       getdat.c
                       readvf.c
                       readvs.c
                       test3d.c
                       view3d.c
                       viewpp.c
                       common.c
                       view2d.c
                       test2d.c
                       misc.c
                       v2lib.c
                       v3lib.c
                       )
add_executable (view3d v3main.c)
target_link_libraries (view3d View3DLib)
