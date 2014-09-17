/*******************************************************************************
*
*  Authors:  Remi Dufour - remi.dufour@mail.utoronto.ca
*            Mike Dai Wang - dai.wang@mail.utoronto.ca
*
*  Date:     March 7th, 2013
*
*  Name:     Malloc Wrappers
*
*  Description: This API wraps an external memory allocator. 
*
*******************************************************************************/

#ifndef _MALLOC_H_
# define _MALLOC_H_

# include <stddef.h>
# include <stdlib.h>

# ifdef __cplusplus 
extern "C" {
# endif

/* Allocate memory block */
#define malloc_wrapper malloc
/*
static
#ifdef INTEL_TM
[[ALLOCATOR_TRANSACTION]]
#endif
inline void *malloc_wrapper(size_t size)
{
    return malloc(size);
}
*/

/* Deallocate memory block */
#define free_wrapper free
/*
static
#ifdef INTEL_TM
[[ALLOCATOR_TRANSACTION]]
#endif
inline void free_wrapper(void *ptr)
{
    free(ptr);
}
*/

/* Allocate and zero-initialize array */
#define calloc_wrapper calloc
/*
static
#ifdef INTEL_TM
[[ALLOCATOR_TRANSACTION]]
#endif
inline void *calloc_wrapper(size_t num, size_t size)
{
    return calloc(num, size);
}
*/

/* Reallocate memory block */
#define realloc_wrapper realloc
/*
static
#ifdef INTEL_TM
[[ALLOCATOR_TRANSACTION]]
#endif
inline void *realloc_wrapper(void *ptr, size_t size)
{
    return realloc(ptr, size);
}
*/

# ifdef __cplusplus
}
# endif

#endif
