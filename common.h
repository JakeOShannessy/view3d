#ifndef V3D_COMMON_H
#define V3D_COMMON_H

#ifdef __WIN32__
# define V3D_EXPORT __declspec(dllexport)
# define V3D_IMPORT __declspec(dllimport)
#else
# ifdef HAVE_GCCVISIBILITY
#  define V3D_EXPORT __attribute__ ((visibility("default")))
#  define V3D_IMPORT
# else
#  define V3D_EXPORT
#  define V3D_IMPORT
# endif
#endif

/* strtok_r is called strtok_s when using Microsofts Visual C compiler,
   the funcitonality and arguments are identical */
#ifdef _WIN32
# define strtok_r strtok_s
#endif

/* V3D_BUILD is currently always set, as we always build the library 
   separately */
#define V3D_BUILD

#ifdef V3D_BUILD
# define V3D_API extern V3D_EXPORT
# define V3D_DLL V3D_EXPORT
#else
# define V3D_API extern V3D_IMPORT
# define V3D_DLL V3D_IMPORT
#endif

#define LINELEN 256

#include <stdio.h>
V3D_API const char *methods[7]; /* method abbreviations */
V3D_API int _maxNVT;   /* maximum number of temporary polygon overlap vertices */
V3D_API FILE *_ulog; /* log file */
V3D_API int _echo;  /* true = echo input file */
V3D_API int _list;  /* output control, higher value = more output:
                0 = summary;
                1 = list view factors;
                2 = echo input, note calculations;
                3 = note obstructions. */

#endif
