#ifndef HSET_H_
#define HSET_H_

#include "hashtable.h"


#define	hset_t											hashtable_t


#define hset_init( hset )								hashtable_init( hset, 16, 4 )
#define hset_init2( hset, _n_bkts, _fx )				hashtable_init( hset, _n_bkts, _fx )
#define hset_create()									hashtable_create()
#define hset_create2( _n_bkts, _fx )					hashtable_create2( _n_bkts, _fx )

#define hset_deinit( hset, elem_destroy )				hashtable_deinit( hset, elem_destroy )
#define hset_destroy( hset, elem_destroy )				hashtable_destroy( hset, elem_destroy )

#define hset_add( hset, elem )							hashtable_add( hset, &(elem)->e )

#define hset_del( hset, elem )							hashtable_del( hset, &(elem)->e )

#define hset_find( hset, jig_elem, elem_eq )			hashtable_find( hset, &(jig_elem)->e, elem_eq )
#define hset_remove( hset, jig_elem, elem_eq )			hashtable_remove( hset, &(jig_elem)->e, elem_eq )

#define hset_clear( hset, elem_destroy )				hashtable_clear( hset, elem_destroy )
#define hset_empty( hset )								hashtable_empty( hset )

#define hset_for_each( pos, cnt, hset )					hashtable_for_each( pos, cnt, hset )
#define hset_for_each_safe( pos, nxt, cnt, hset )		hashtable_for_each_safe( pos, nxt, cnt, hset )

#define hset_key_for_each( pos, hset, key )				bucket_for_each( pos, hset, key )
#define hset_key_for_each_safe( pos, nxt, hset, key )	bucket_for_each_safe( pos, nxt, hset, key )

#define hset_size( hset )								((hset)->n)

#endif /*HSET_H_*/


