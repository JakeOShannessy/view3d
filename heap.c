/*subfile:  heap.c  **********************************************************/

/*
	The custom heat management stuff in here is being switched off because
	tools exist these days like Valgrind and DMALLOC which do a better job
	on such tasks, and save us from carrying around all this custom code.

	-- JP, 7 Nov 2012
*/

#define DEBUG 0

#define V3D_BUILD
#include "heap.h"

#include <stdio.h>
#include <string.h> /* prototypes: memset, ... */
#include <stdlib.h> /* prototype: malloc, free */
#include <limits.h> /* define UINT_MAX */
#include "types.h"  /* define unsigned char, short, etc. */
#include "misc.h"  /* miscellaneous function prototypes */

long _bytesAllocated=0L;  /* through Alc_E() */
long _bytesFreed=0L;      /* through Fre_E() */

/*------------------------------------------------------------------------------
  ELEMENTS ('E')
*/

/**  Allocate memory for a single element, i.e. contiguous portion of the heap.
 *  This may be a single structure or an array. All allocated bytes set to 0.
 *  All memory allocations and de-allocations should go through Alc_E() 
 *  and Fre_E() to allow some useful heap checking options.
 *  @param length length of element (bytes).
 *  @param file   name of file for originating call.
 *  @param line   line in file.
*/
void *Alc_E( long length, const char *file, int line ){
  unsigned char *p;     /* pointer to allocated memory */
  char heapmsg[256];
  if( length < 1 ){
    sprintf( heapmsg, "Element too small to allocate: %ld bytes\n", length );
    error( 3, file, line, heapmsg, "" );
  }

  /*TODO use 'calloc' instead? */

  p = (unsigned char *)malloc( length );
  _bytesAllocated += length;

  if( p == NULL ){
    MemNet( "Alc_E error" );
    sprintf( heapmsg, "Memory allocation failed for %ld bytes\n", length );
    error( 3, file, line, heapmsg, "" );
  }

  memset( p, 0, length );  /* zero allocated memory */

  return (void *)p;
}  /*  end of Alc_E  */

/***  Chk_E.c  ***************************************************************/

/**  Check guard bytes around memory allocated by Alc_E().
 *  @return non-zero if heap is in error.
 *  @param  pm    pointer to allocated memory.
 *  @param length length of element (bytes).
 *  @param file   name of file for originating call.
 *  @param line   line in file.
*/
int Chk_E( void *pm, unsigned length, const char *file, int line ){
 return 0;
} /*  end of Chk_E  */

/**
	Free pointer to previously memory allocated by Alc_E().
	Includes a memory check.
	@param pm;    pointer to allocated memory.
	@param length; length of element (bytes).
	@param file;   name of file for originating call.
	@param line;   line in file.  
*/
void *Fre_E( void *pm, unsigned length, const char *file, int line ){
  unsigned char *p=pm;     /* pointer to allocated memory */
  _bytesFreed += length;
  free( p );
  return (NULL);
}  /*  end of Fre_E  */

/***  MemNet.c  **************************************************************/

/*  Report memory allocated and freed.  */

long MemNet( char *msg ){
  long netBytes=_bytesAllocated-_bytesFreed;
#ifdef XXX
  fprintf( _ulog, "%s: %ld bytes allocated, %ld freed, %ld net\n",
    msg, _bytesAllocated, _bytesFreed, netBytes );
  fflush( _ulog );
#endif
  return netBytes;
}  /* end of MemNet */

/***  MemList.c  *************************************************************/

/*  Report current allocated memory.  */

void MemList( void ){
	/* nothing here */
} /* end of MemList */

/***  MemRem.c  **************************************************************/

/*  Report memory allocated and freed. Tubro C's coreleft() reports the amount
 *  of memory between the highest allocated block and the top of the heap.
 *  Freed lower blocks are not counted as available memory.
 *  heapwalk() shows the details.  */

void MemRem( char *msg ){
  MemNet( msg );  /* for non-TurboC code */
} /* end of MemRem */

/***  Alc_EC.c  **************************************************************/

/*  Allocate small structures within a larger allocated block.
 *  This saves quite a bit of memory allocation overhead and is faster than
 *  calling Alc_E() for each small structure. Especially fast for deallocation
 *  because the entire block is freed. Cannot free the individual structures.
 *  Based on idea and code by Steve Weller, "The C Users Journal",
 *  April 1990, pp 103 - 107.
 *  Must begin with Alc_ECI for initialization; free with Fre_EC.  */

typedef struct memblock   /* block of memory for Alc_EC() allocation */
  {
  struct memblock *prevBlock;  /* pointer to previous block */
  struct memblock *nextBlock;  /* pointer to next block */
  long blockSize;   /* number of bytes in block */
  long dataOffset;  /* offset to free space */
} MEMBLOCK;

void *Alc_EC( char **block, long size, const char *file, int line ){
/*  block;  pointer to current memory block.
 *  size;   size (bytes) of structure being allocated.
 *  file;   name of file for originating call.
 *  line;   line in file. */
  
  char *p;  /* pointer to the structure */
  MEMBLOCK *mb, /* current memory block */
           *nb; /* next memory block */
  char heapmsg[256];

  if( size < 1 ){
    sprintf( heapmsg, "Element too small to allocate: %ld bytes\n", size );
    error( 3, file, line, heapmsg, "" );
  }

  /* Set memory alignment. */
  size = (size+7) & 0xFFFFFFF8;   /* 32 bit; multiple of 8 */

  mb = (void *)*block;
  if( size > mb->blockSize - (long)sizeof(MEMBLOCK) )
    {
    sprintf( heapmsg, "Requested size (%ld) larger than block (%ld)",
      size, mb->blockSize - (long)sizeof(MEMBLOCK) );
    error( 3, file, line, heapmsg, "" );
    }
  if( mb->dataOffset + size > mb->blockSize )
    {
    if( mb->nextBlock )
      nb = mb->nextBlock;     /* next block already exists */
    else
      {                       /* else create next block */
      nb = (MEMBLOCK *)Alc_E( mb->blockSize, file, line );
      nb->prevBlock = mb;     /* back linked list */
      mb->nextBlock = nb;     /* forward linked list */
      nb->nextBlock = NULL;
      nb->blockSize = mb->blockSize;
      nb->dataOffset = sizeof(MEMBLOCK);
      }
    mb = nb;
    *block = (void *)nb;
    }
  p = *block + mb->dataOffset;
  mb->dataOffset += size;

  return (void *)p;

}  /*  end of Alc_EC  */

/***  Alc_ECI.c  *************************************************************/

/*  Block initialization for Alc_EC.  Use:
 *    char *_ms;     memory block for small structures
 *    _ms = (char *)alc_eci( 2000, "ms-block" );  */

void *Alc_ECI( long size, const char *file, int line )
/*  size;   size (bytes) of block being allocated.
 *  file;   name of file for originating call.
 *  line;   line in file. */
  {
  MEMBLOCK *mb;
  char heapmsg[256];

  if( size > UINT_MAX )
    {
    sprintf( heapmsg, "Requested size (%ld) larger than unsigned int", size );
    error( 3, file, line, heapmsg, "" );
    }

  mb = Alc_E( size, file, line );
  mb->prevBlock = NULL;
  mb->nextBlock = NULL;
  mb->blockSize = size;
  mb->dataOffset = sizeof(MEMBLOCK);

  return mb;

  }  /*  end of Alc_ECI  */


/***  Clr_EC.c  **************************************************************/

/*  Clear (but do not free) blocks allocated by Alc_EC.
 *  Return pointer to first block in linked list.  */

void *Clr_EC( void *block, const char *file, int line )
/*  block;  pointer to current (last in list) memory block. */
  {
  unsigned char *p;      /* pointer to the block */
  MEMBLOCK *mb=block;

  for( ; mb->nextBlock; mb=mb->nextBlock )
    ;  /* guarantee mb at end of list */

  for( p=(void *)mb; mb; mb=mb->prevBlock )  /* loop to start of list */
    {
    p = (void *)mb;
    memset( p + sizeof(MEMBLOCK), 0, mb->blockSize - sizeof(MEMBLOCK) );
    mb->dataOffset = sizeof(MEMBLOCK);
    }

  return (void *)p;  /* return start of linked list */

  }  /*  end of Clr_EC  */

/***  Fre_EC.c  **************************************************************/

/*  Free blocks allocated by Alc_EC.  */

void *Fre_EC( void *block, const char *file, int line )
/*  block;  pointer to current memory block.
 *  file;   name of file for originating call.
 *  line;   line in file. */
  {
  MEMBLOCK *mb, *nb=block;

  for( mb=nb; mb->nextBlock; mb=mb->nextBlock )
    ;  /* guarantee mb at end of list */

  for( ; mb; mb=nb )  /* loop to start of list */
    {
    nb=mb->prevBlock;
    Fre_E( mb, mb->blockSize, file, line );
    }

  return (NULL);

  }  /*  end of Fre_EC  */

#define ANSIOFFSET 0 /* 1 = include V[0] in vector allocation */

/***  Alc_V.c  ***************************************************************/

/*  Allocate pointer for a vector with optional debugging data.
 *  This vector is accessed and stored as:
 *     V[J], V[J+1], V[J+2], ..., V[N-1], V[N]
 *  where J is the minimum vector index and N is the maximum index --
 *  J is typically 0 or 1. A pointer to V[0] is returned even if V[0]
 *  was not physically allocated. This is not strictly ANSI compliant.
 *  The index offset idea is from "Numeric Recipes in C" by Press, et.al,
 *  1988. The 2nd edition (1992) includes another offset to guarantee ANSI
 *  compliance for min_index = 1; a more general method is used here which
 *  is activated by #define ANSIOFFSET 1  (applies when min_index > 0)  */

void *Alc_V( int min_index, long max_index, int size, const char *file, int line )
/*  min_index;  minimum vector index:  vector[min_index] valid.
 *  max_index;  maximum vector index:  vector[max_index] valid.
 *  size;   size (bytes) of one data element.
 *  file;   name of file for originating call.
 *  line;   line in file. */
  {
  char *p;      /* pointer to the vector */
  long length = /* length of vector (bytes) */
    (max_index - min_index + 1) * size;
  char heapmsg[256];

  if( length < 1 )
    {
    sprintf( heapmsg, "Max index (%ld) < min index (%d)",
      max_index, min_index );
    error( 3, file, line, heapmsg, "" );
    }

#if( ANSIOFFSET > 0 )
  if( min_index > 0 )
    length += min_index * size;
  p = (char *)Alc_E( length, file, line );
  if( min_index < 0 )
    p -= min_index * size;
#else
  p = (char *)Alc_E( length, file, line );
  p -= min_index * size;
#endif

  return ((void *)p);

  }  /*  end of Alc_V  */

/***  Clr_V.c  ***************************************************************/

/*  Clear (zero all elements of) a vector created by Alc_V( ).  */

void Clr_V( void *v, int min_index, int max_index, int size, const char *file, int line )
/*  min_index;  minimum vector index:  vector[min_index] valid.
 *  max_index;  maximum vector index:  vector[max_index] valid.
 *  size;   size (bytes) of one data element.
 *  file;   name of file for originating call.
 *  line;   line in file. */
  {
  char *pdata = (char *)v + min_index * size;
  unsigned length = (unsigned)(max_index - min_index + 1) * size;
  memset( pdata, 0, length );

  }  /*  end of Clr_V  */

/***  Fre_V.c  ***************************************************************/

/*  Free pointer to a vector allocated by Alc_V().  */

void *Fre_V( void *v, int min_index, int max_index, int size, const char *file, int line )
/*  v;      pointer to allocated vector.
 *  min_index;  minimum vector index.
 *  max_index;  maximum vector index.
 *  size;   size (bytes) of one data element.
 *  file;   name of file for originating call.
 *  line;   line in file. */
  {
  char *p=(char *)v;  /* pointer to the vector data */
  unsigned length =     /* number of bytes in vector data */
    (unsigned)(max_index - min_index + 1) * size;

#if( ANSIOFFSET > 0 )
  if( min_index > 0 )
    length += min_index * size;
  if( min_index < 0 )
    p += min_index * size;
#else
  p += min_index * size;
#endif

  Fre_E( (void *)p, length, file, line );

  return (NULL);

  }  /*  end of Fre_V  */


/*------------------------------------------------------------------------------
  CONTIGUOUS MATRICES
*/

/** Allocate (contiguously) a matrix, M[i][j].
	This matrix is accessed (and stored) in a rectangular form:
	+------------+------------+------------+------------+-------
	| [r  ][c  ] | [r  ][c+1] | [r  ][c+2] | [r  ][c+3] |   ...
	+------------+------------+------------+------------+-------
	| [r+1][c  ] | [r+1][c+1] | [r+1][c+2] | [r+1][c+3] |   ...
	+------------+------------+------------+------------+-------
	| [r+2][c  ] | [r+2][c+1] | [r+2][c+2] | [r+2][c+3] |   ...
	+------------+------------+------------+------------+-------
	| [r+3][c  ] | [r+3][c+1] | [r+3][c+2] | [r+3][c+3] |   ...
	+------------+------------+------------+------------+-------
	|    ...     |    ...     |    ...     |    ...     |   ...
	where r is the minimum row index and
	c is the minimum column index (both usually 0 or 1).  

	@param min_row_index  minimum row index.
	@param max_row_index  maximum row index.
	@param min_col_index  minimum column index.
	@param max_col_index  maximum column index.
	@param size           size (bytes) of one data element.
	@param file           name of file for originating call.
	@param line           line in file.
*/
void *Alc_MC( int min_row_index, int max_row_index, int min_col_index,
              int max_col_index, int size, const char *file, int line
){
  /* prow - vector of pointers to rows of M */
  int nrow = max_row_index - min_row_index + 1;    /* number of rows [i] */
  long tot_row_index = min_row_index + nrow - 1;    /* max prow index */
  char **prow = (char **)Alc_V( min_row_index, tot_row_index, sizeof(int *), file, line );

  /* pdata - vector of contiguous A[i][j] data values */
  int ncol = max_col_index - min_col_index + 1;    /* number of columns [j] */
  long tot_col_index = min_col_index + nrow*ncol - 1;  /* max pdata index */
  char *pdata = (char *)Alc_V( min_col_index, tot_col_index, size, file, line );
  /* Note: must have nrow >= 1 and ncol >= 1. */
  /* If nrow < 1, allocation of prow will fail with fatal error message.  */
  /* If ncol < 1, allocation of pdata will fail since nrow*ncol <= 0. */

  int n, m;
  for( m=0,n=min_row_index; n<=tot_row_index; n++){
	/* set row pointers */
    prow[n] = pdata + ncol*size*m++;  
  }

  return ((void *)prow);

}  /*  end of Alc_MC  */


/**
	Clear (zero all elements of) a matrix created by Alc_MC( ).
*/
void Clr_MC( void *m, int min_row_index, int max_row_index, int min_col_index,
             int max_col_index, int size, const char *file, int line 
){
  int nrow = max_row_index - min_row_index + 1;
  int ncol = max_col_index - min_col_index + 1;
  int tot_col_index = min_col_index + nrow*ncol - 1;
  char **prow = (char **)m;
  char *pdata = prow[min_row_index];

  Clr_V( pdata, min_col_index, tot_col_index, size, file, line );

} /*  end of Clr_MC  */


/**
	Free pointer to a matrix allocated by Alc_MC().
*/
void *Fre_MC( void *m, int min_row_index, int max_row_index, int min_col_index,
             int max_col_index, int size, const char *file, int line 
){
  int nrow = max_row_index - min_row_index + 1;
  int tot_row_index = min_row_index + nrow - 1;
  char **prow = (char **)m;
  int ncol = max_col_index - min_col_index + 1;
  int tot_col_index = min_col_index + nrow*ncol - 1;
  char *pdata = prow[min_row_index];

  Fre_V( pdata, min_col_index, tot_col_index, size, file, line );
  Fre_V( prow, min_row_index, tot_row_index, sizeof(char *), file, line );

  return (NULL);
}  /*  end of Fre_MC  */


/*------------------------------------------------------------------------------
  SYMMETRIC MATRICES ('MSC')
*/

/** Allocate (contiguously) a symmetric matrix, M[i][j].
 *  This matrix is accessed (and stored) in a triangular form:
 *  +------------+------------+------------+------------+-------
 *  | [i  ][i  ] |     -      |     -      |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+1][i  ] | [i+1][i+1] |     -      |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+2][i  ] | [i+2][i+1] | [i+2][i+2] |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+3][i  ] | [i+3][i+1] | [i+3][i+2] | [i+3][i+3] |    - 
 *  +------------+------------+------------+------------+-------
 *  |    ...     |    ...     |    ...     |    ...     |   ...
 *  where i is the minimum array index (usually 0 or 1).
 *  NOTE!  M[i][j] is valid when j <= i.
 *  @param min_index;  minimum matrix index.
 *  @param max_index;  maximum matrix index.
 *  @param size;   size (bytes) of one data element.
 *  @param file;   name of file for originating call.
 *  @param line;   line in file.
*/
void *Alc_MSC( int min_index, int max_index, int size, const char *file, int line){
  /* prow - vector of pointers to rows of M */
  char **prow = (char **)Alc_V( min_index, max_index, sizeof(char *), file, line );

     /* pdata - vector of contiguous M[i][j] data values */
  int nrow = max_index - min_index + 1;    /* number of rows */
  int nval = nrow * (nrow + 1) / 2;        /* number of values */
  int tot_index = min_index + nval - 1;    /* max pdata index */
  char *pdata = (char *)Alc_V( min_index, tot_index, size, file, line );
     /* If nrow < 1, allocation of prow will fail with fatal error message.  */

  int n, m;
  prow[min_index] = pdata;
  for( m=1,n=min_index+1; n<=max_index; n++ ) 
    prow[n] = prow[n-1] + size*m++;  /* set row pointers */
  
  return ((void *)prow);
}  /*  end of Alc_MSC  */


/**
	Clear (zero all elements of) a symmetric matrix created by Alc_MSC( ).
*/
void Clr_MSC( void *m, int min_index, int max_index, int size, const char *file, int line){
  int nrow = max_index - min_index + 1;
  int nval = nrow * (nrow + 1) / 2;
  int tot_index = min_index + nval - 1;
  char **prow = (char **)m;
  char *pdata = prow[min_index];

  Clr_V( pdata, min_index, tot_index, size, file, line );
} /*  end of Clr_MSC  */


/*
	Free a symmetric matrix allocated by Alc_MSC.
	param v      pointer to allocated vector. (???)
*/
void *Fre_MSC( void *m, int min_index, int max_index, int size, const char *file, int line){
  int nrow = max_index - min_index + 1;
  int nval = nrow * (nrow + 1) / 2;
  int tot_index = min_index + nval - 1;
  char **prow = (char **)m;
  char *pdata = prow[min_index];

  Fre_V( pdata, min_index, tot_index, size, file, line );
  Fre_V( prow, min_index, max_index, sizeof(char *), file, line );

  return (NULL);
} /*  end of Fre_MSC  */


/*------------------------------------------------------------------------------
  SYMMETRIC MATRICES ('MSR')
*/

/**  Allocate (by rows) a symmertic matrix.
 *  This matrix is accessed (and stored) in a triangular form:
 *  +------------+------------+------------+------------+-------
 *  | [i  ][i  ] |     -      |     -      |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+1][i  ] | [i+1][i+1] |     -      |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+2][i  ] | [i+2][i+1] | [i+2][i+2] |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+3][i  ] | [i+3][i+1] | [i+3][i+2] | [i+3][i+3] |    - 
 *  +------------+------------+------------+------------+-------
 *  |    ...     |    ...     |    ...     |    ...     |   ...
 *
 *  where i is the minimum array index (usually 0 or 1).
 *  min_index;  minimum vector index:  matrix[min_index][min_index] valid.
 *  max_index;  maximum vector index:  matrix[max_index][max_index] valid.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.
 */
void *Alc_MSR( int min_index, int max_index, int size, const char *file, int line ){
  char **p;  /*  pointer to the array of row pointers  */
  int i;     /* row number */
  int j;     /* column number */

  /* Allocate vector of row pointers; then allocate individual rows. */ 

  p = (char **)Alc_V( min_index, max_index, sizeof(char *), file, line );
  for( j=i=max_index; i>=min_index; i--,j-- )
    p[i] = (char *)Alc_V( min_index, j, size, file, line );

  /* Vectors allocated from largest to smallest to allow
     maximum filling of holes in the heap. */

  return ((void *)p);

} /*  end of Alc_MSR  */


/*  Clear (zero all elements of) a symmetric matrix created by Alc_MSR( ).  */
void Clr_MSR(void *m, int min_index, int max_index, int size, const char *file, int line){
  char **p;  /*  pointer to the array of row pointers  */
  int i;     /* row number */
  int j;     /* column number */

  p = (char **)m;
  for( j=i=min_index; i<=max_index; i++,j++ )
    Clr_V( p[i], min_index, j, size, file, line );

} /*  end of Clr_MSR  */


/**
 *	Free a symmertic matrix allocated by Alc_MSR.
 *  @param v         pointer to allocated vector.
 *  @param min_index minimum vector index:  matrix[min_index][min_index] valid.
 *  @param max_index maximum vector index:  matrix[max_index][max_index] valid.
 *  @param size      size (bytes) of one data element.
 *  @param name      name of variable being freed.
*/
void *Fre_MSR( void *v, int min_index, int max_index, int size, const char *file, int line ){
  char **p;  /*  pointer to the array of row pointers  */
  int i;     /* row number */
  int j;     /* column number */

  p = (char **)v;
  for( j=i=min_index; i<=max_index; i++,j++ )
    Fre_V( p[i], min_index, j, size, file, line );
  Fre_V( p, min_index, max_index, sizeof(char *), file, line );

  return (NULL);
}  /*  end of Fre_MSR  */
