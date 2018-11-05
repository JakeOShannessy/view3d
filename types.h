/*subfile:  types.h  *********************************************************/

/*  redefinition of C variable types and path parameters.  */

#ifndef STD_TYPES
# define STD_TYPES

/* removed the #defines for U1, R8 etc as they were making the code hard to read */

#define NAMELEN 16
#define LINELEN 256

#if( __GNUC__ )
#include <limits.h>  /* define PATH_MAX */
#else
#include <stdlib.h>  /* define _MAX_PATH, etc. */
#endif
#include <limits.h>  /* define _MAX_PATH, etc. */
#ifndef _MAX_PATH
# ifdef PATH_MAX     /* GNUC parameter defined in <limits.h> */
#  define _MAX_PATH  PATH_MAX
#  define _MAX_DIR   PATH_MAX
#  define _MAX_FNAME NAME_MAX
# else
/* _MAX_PATH, _MAX_DIR, _MAX_FNAME retain VisualC++ values */
#  define _MAX_PATH  4096
#  define _MAX_DIR   4096
#  define _MAX_FNAME 4096
# endif
#define _MAX_DRIVE 4    /* 3 minimum (3 VisualC++) */
#define _MAX_EXT   8    /* 5 minimum (256 VisualC++) */
/* replace _MAX_DRIVE, _MAX_EXT VisualC++ values */
#endif

#endif