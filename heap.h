#ifndef V3D_HEAP_H
#define V3D_HEAP_H

#include "common.h"

/* functions in heap.c */
void *Alc_E( long length, char *file, int line );
int Chk_E( void *pm, unsigned, char *, int );
void *Fre_E( void *pm, unsigned, char *, int );

V3D_API long MemNet( char *msg );
V3D_API void MemList( void );
void MemRem( char *msg );

void *Alc_EC( char **block, long size, char *file, int line );
void *Alc_ECI( long size, char *, int );
void Chk_EC( void *block, char *, int );
void *Clr_EC( void *block, char *, int );
void *Fre_EC( void *block, char *, int );

V3D_API void *Alc_V( int min_index, long max_index, int size, char *file, int line );
void Chk_V( void *, int, int, int, char *, int );
void Clr_V( void *, int, int, int, char *, int );
V3D_API void *Fre_V( void *, int, int, int, char *, int );

V3D_API void *Alc_MC( int min_row, int max_row, int min_col, int max_col,
              int size, char *file, int line );
void Chk_MC( void *, int, int, int, int, int, char *, int );
void Clr_MC( void *, int, int, int, int, int, char *, int );
V3D_API void *Fre_MC( void *, int, int, int, int, int, char *, int );

void *Alc_MR( int min_row, int max_row, int min_col, int max_col,
              int size, char *file, int line );
void Chk_MR( void *, int, int, int, int, int, char *, int );
void *Fre_MR( void *, int, int, int, int, int, char *, int );

void *Alc_AC( int min_plane, int max_plane, int min_row, int max_row,
              int min_col, int max_col, int size, char *file, int line );
void Chk_AC( void *, int, int, int, int, int, int, int, char *, int );
void Clr_AC( void *, int, int, int, int, int, int, int, char *, int );
void *Fre_AC( void *, int, int, int, int, int, int, int, char *, int );

V3D_API void *Alc_MSC( int min_index, int max_index, int size, char *file, int line );
void Chk_MSC( void *m, int, int, int, char *, int );
void Clr_MSC( void *m, int, int, int, char *, int );
V3D_API void *Fre_MSC( void *m, int, int, int, char *, int );

void *Alc_MSR( int min_index, int max_index, int size, char *file, int line );
void Chk_MSR( void *m, int, int, int, char *, int );
void Clr_MSR( void *m, int, int, int, char *, int );
void *Fre_MSR( void *m, int, int, int, char *, int );

void *Alc_MVC( int min_index, int max_index, int *elements, int size,
               int *tne, char *file, int line );
void *Chk_MVC( void *m, int, int, int *, int, char *f, int );
void *Fre_MVC( void *m, int, int, int *, int, char *f, int );

#endif

