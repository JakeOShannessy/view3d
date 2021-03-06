#ifndef V3D_MISC_H
#define V3D_MISC_H

#include "common.h"

V3D_API int error(int severity, const char *file, const int line,...);

V3D_API void PathMerge(char *fullpath, int szfp, char *drv, char *path, char *name, char *ext);

V3D_API void PathSplit(char *fullpath, char *drv, int szd, char *path, int szp
		, char *name, int szn, char *ext, int sze
);

char *IntStr( long i );
char *FltStr( double f, int n );

int LongCon( char *str, long *i);

V3D_API float CPUtime(float t1);

int streqli( char *s1, char *s2 );
int streql( char *s1, char *s2 );

int FltCon( char *str, float *f );

double ReadR8( FILE *inHandle, int flag );
int ReadIX( FILE *inHandle, int flag );

int IntCon( char *str, int *i );

V3D_API int NxtOpen(const char *file_name, const char *file, int line);
FILE *NxtOpenHndl(const char *file_name, const char *file, int line );
void NxtCloseHndl(FILE *file);
V3D_API void NxtClose( void );
char *NxtWord( FILE* inHandle, char *str, int flag, int maxlen );

float ReadR4( FILE *inHandle, int flag );

const char *sfname(const char* longfilename);

V3D_API char *GetStr( char *prompt, char *str, int maxchar);
V3D_API int NoYes( char *question );


#endif

