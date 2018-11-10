INCLUDE (CheckLibraryExists)
cmake_minimum_required (VERSION 2.6)
project (View3D)

set (V3D_VERSION 3.5)

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

# produce config.h
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_SOURCE_DIR}/config.h"
  )

add_executable (view2d v2main.c)
target_link_libraries (view2d View3DLib)

add_executable (view3d v3main.c)
add_definitions(-DMAIN)
target_link_libraries (view3d View3DLib)

find_library(MATH_LIB m)
if(MATH_LIB)
  target_link_libraries (view2d m)
  target_link_libraries (view3d m)
endif()
