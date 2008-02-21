#define V3D_BUILD
#include "common.h"

/* nasty global variables used in a range of places... */

const char *methods[7]={"2AI","1AI","2LI","1LI","ALI","Adapt","Blocked"}; /* abbreviations */
char _string[LINELEN];  /* buffer for a character string */
int _maxNVT=12;   /* maximum number of temporary polygon overlap vertices */
FILE *_unxt; /* input file */
FILE *_ulog; /* log file */
int _echo=0;  /* true = echo input file */
int _list=0;  /* output control, higher value = more output:
                0 = summary;
                1 = list view factors;
                2 = echo input, note calculations;
                3 = note obstructions. */

