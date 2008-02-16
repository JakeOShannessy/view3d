#ifndef V3D_MISC_H
#define V3D_MISC_H

#include "common.h"

V3D_API int error(int severity, const char *file, const int line,...);

void PathMerge(char *fullpath, int szfp, char *drv, char *path, char *name, char *ext);

void PathSplit(char *fullpath, char *drv, int szd, char *path, int szp
		, char *name, int szn, char *ext, int sze
);

char *IntStr( long i );
char *FltStr( double f, int n );

V3D_API float CPUtime(float t1);

int streqli( char *s1, char *s2 );
int streql( char *s1, char *s2 );

int FltCon( char *str, float *f );

double ReadR8( int flag );
int ReadIX( int flag );

int IntCon( char *str, int *i );

V3D_API int NxtOpen(const char *file_name, const char *file, int line);
V3D_API void NxtClose( void );
char *NxtWord( char *str, int flag, int maxlen );

float ReadR4( int flag );

const char *sfname(const char* longfilename);

#endif

