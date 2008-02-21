#ifndef V3D_HEAP_H
#define V3D_HEAP_H

#include "common.h"

/*
	Allocation of single elements
*/
void *Alc_E( long length, const char *file, int line );
int Chk_E( void *pm, unsigned, const char *, int );
void *Fre_E( void *pm, unsigned, const char *, int );

#define V3D_NEW(TYPE) Alc_E(sizeof(TYPE),__FILE__,__LINE__)
#define V3D_FREE(TYPE,VAR) Fre_E((void *)(VAR),sizeof(TYPE),__FILE__,__LINE__);

V3D_API long MemNet( char *msg );
V3D_API void MemList( void );
void MemRem( char *msg );

/*
	Allocation of space within a large preallocated block
*/
void *Alc_EC( char **block, long size, const char *file, int line );
void *Alc_ECI( long size, const char *, int );
void Chk_EC( void *block, const char *, int );
void *Clr_EC( void *block, const char *, int );
void *Fre_EC( void *block, const char *, int );

/*
	Allocation of arrays of elements
*/
V3D_API void *Alc_V( int min_index, long max_index, int size, const char *file, int line );
void Chk_V( void *, int, int, int, const char *, int );
void Clr_V( void *, int, int, int, const char *, int );
V3D_API void *Fre_V( void *, int, int, int, const char *, int );

#define V3D_NEW_ARRAY(TYPE,SIZE) Alc_V(1,SIZE,sizeof(TYPE),__FILE__,__LINE__)
#define V3D_FREE_ARRAY(TYPE,SIZE,VAR) Fre_V((void *)(VAR),1,SIZE,sizeof(TYPE),__FILE__,__LINE__);

/*
	Contiguous matrices
*/
V3D_API void *Alc_MC( int min_row, int max_row, int min_col, int max_col,
              int size, const char *file, int line );
void Chk_MC( void *, int, int, int, int, int, const char *, int );
void Clr_MC( void *, int, int, int, int, int, const char *, int );
V3D_API void *Fre_MC( void *, int, int, int, int, int, const char *, int );

#define V3D_NEW_MATRIX(TYPE,ROWS,COLS) Alc_MC(1,(ROWS),1,(COLS),sizeof(TYPE),__FILE__,__LINE__)
#define V3D_FREE_MATRIX(TYPE,ROWS,COLS,VAR) Fre_MC((void *)(VAR),1,(ROWS),1,(COLS),sizeof(TYPE),__FILE__,__LINE__)

void *Alc_MR( int min_row, int max_row, int min_col, int max_col,
              int size, const char *file, int line );
void Chk_MR( void *, int, int, int, int, int, const char *, int );
void *Fre_MR( void *, int, int, int, int, int, const char *, int );

void *Alc_AC( int min_plane, int max_plane, int min_row, int max_row,
              int min_col, int max_col, int size, const char *file, int line );
void Chk_AC( void *, int, int, int, int, int, int, int, const char *, int );
void Clr_AC( void *, int, int, int, int, int, int, int, const char *, int );
void *Fre_AC( void *, int, int, int, int, int, int, int, const char *, int );

/*
	Symmetric matrices
*/
V3D_API void *Alc_MSC( int min_index, int max_index, int size, const char *file, int line );
void Chk_MSC( void *m, int, int, int, const char *, int );
void Clr_MSC( void *m, int, int, int, const char *, int );
V3D_API void *Fre_MSC( void *m, int, int, int, const char *, int );

#define V3D_NEW_MATRIX_SYMM(TYPE,SIZE) Alc_MSC(1,(SIZE),sizeof(TYPE),__FILE__,__LINE__)
#define V3D_FREE_MATRIX_SYMM(TYPE,SIZE,VAR) Fre_MSC((void *)(VAR),1,(SIZE),sizeof(TYPE),__FILE__,__LINE__)

void *Alc_MSR( int min_index, int max_index, int size, const char *file, int line );
void Chk_MSR( void *m, int, int, int, const char *, int );
void Clr_MSR( void *m, int, int, int, const char *, int );
void *Fre_MSR( void *m, int, int, int, const char *, int );

void *Alc_MVC( int min_index, int max_index, int *elements, int size,
               int *tne, const char *file, int line );
void *Chk_MVC( void *m, int, int, int *, int, char *f, int );
void *Fre_MVC( void *m, int, int, int *, int, char *f, int );

#endif

