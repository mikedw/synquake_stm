#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include "../general.h"
#include "list.h"
#include "malloc.h"

typedef struct
{
	list_t* bkts;
	unsigned int n_bkts;
	unsigned int n;
	unsigned int fx;			// if n > n_bkts * fx then hashtable_grow;
} hashtable_t;


#define hashtable_for_each( pos, cnt, h )			\
	for( cnt = 0; cnt < (h)->n_bkts; cnt++ )	list_for_each( pos, &(h)->bkts[cnt] )

#define hashtable_for_each_safe( pos, nxt, cnt, h )	\
	for( cnt = 0; cnt < (h)->n_bkts; cnt++ )	list_for_each_safe( pos, nxt, &(h)->bkts[cnt] )

#define bucket_for_each( pos, h, key )				\
	list_for_each( pos, &((h)->bkts[ key % ((h)->n_bkts) ]) )

#define bucket_for_each_safe( pos, nxt, h, key )	\
	list_for_each_safe( pos, nxt, &((h)->bkts[ key % ((h)->n_bkts) ]) )


static
#ifdef INTEL_TM
[[transaction_callable]]
#endif
inline void	hashtable_init( hashtable_t* h, unsigned int _n_bkts, unsigned int _fx )
{
	unsigned int i;
	assert( h && _n_bkts > 0 && _fx > 0 );

	h->n = 0;
	h->fx = _fx;
	h->n_bkts = _n_bkts;
	h->bkts	= (list_t*)malloc_wrapper( _n_bkts * sizeof(list_t) );	assert( h->bkts );
	for( i = 0; i < _n_bkts; i++ )	list_init( &h->bkts[i] );
}

static
#ifdef INTEL_TM
[[transaction_callable]]
#endif
inline void	hashtable_grow( hashtable_t* h )
{
	unsigned int i;
	list_t* old_bkts = h->bkts;
	unsigned int old_n_bkts = h->n_bkts;
	hashtable_init( h, h->n_bkts * 2, h->fx );

	elem_t *elem, *nxt;
	for( i = 0; i < old_n_bkts; i++ )
		list_for_each_safe( elem, nxt, &old_bkts[i] )
			list_move( &h->bkts[ elem->key % h->n_bkts ], elem );

	free_wrapper( old_bkts );
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void	hashtable_add( hashtable_t* h, elem_t* elem )
{
	assert( h && elem );
	list_add( &h->bkts[ elem->key % h->n_bkts ], elem );
	h->n++;
// remi: big hack
//	if( h->n > h->n_bkts*h->fx )	hashtable_grow( h );
}

static inline void	hashtable_rehash( hashtable_t* h, elem_t* elem )
{
	assert( h && elem );
	list_move( &h->bkts[ elem->key % h->n_bkts ], elem );
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void	hashtable_del( hashtable_t* h, elem_t* elem )
{
	assert( h && elem );
	list_del( elem );
	h->n--;
}

static inline int hashtable_empty( hashtable_t* h )
{
	assert( h );
	return h->n == 0;
}

#define hashtable_find( h, elem, elem_eq )							\
({																	\
	assert( h && elem );											\
	elem_t *pos, *rez = NULL;										\
	bucket_for_each( pos, h, (elem)->key )							\
		if( elem_eq( pos, elem) )									\
		{															\
			rez = pos;												\
			break;													\
		}															\
	rez;															\
})

#define	hashtable_remove( h, elem, elem_eq )						\
({																	\
	assert( h && elem );											\
	elem_t *rez = hashtable_find( h, elem, elem_eq );				\
	if( rez )	hashtable_del( h, rez );							\
}}

#define hashtable_clear( h, elem_destroy )							\
({																	\
	unsigned int __i = 0;													\
	list_t *pos, *nxt;												\
	assert( h );													\
																	\
	hashtable_for_each_safe( pos, nxt, __i, h )						\
	{																\
		list_del( pos );											\
		elem_destroy( pos );										\
	}																\
	(h)->n = 0;														\
})

#define hashtable_deinit( h, elem_destroy )							\
({																	\
	assert(h);														\
	hashtable_clear( h, elem_destroy );								\
	free_wrapper( (h)->bkts );												\
})


// Remi: changes the value to 32 as a hack to avoid growing a hashtable
#define hashtable_create()									generic_create2( hashtable_t, hashtable_init, 32, 4 )
#define hashtable_create2( _n_bkts, _fx )					generic_create2( hashtable_t, hashtable_init, _n_bkts, _fx )

#define hashtable_destroy( h, elem_destroy )				generic_destroy1( h, hashtable_deinit, elem_destroy )


#endif /*HASHTABLE_H_*/
