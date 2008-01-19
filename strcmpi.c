/*
	File to provide 'strcmpi' function, which is not a standard
	POSIX function.
*/
#include "strcmpi.h"

#ifndef _MSC_VER
# include <ctype.h>

int v3_strcmpi(char *s1, char *s2){
	for (; *s1 && *s2 && (toupper(*s1) == toupper(*s2)); ++s1, ++s2);
	return *s1 - *s2;
}

#endif

