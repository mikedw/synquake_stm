#ifndef AREA_NODE_H_
#define AREA_NODE_H_

#include "../../general.h"
#include "../../utils/geometry.h"

#include "../../game/game.h"


#define		LOCK_TREE		0
#define		LOCK_LEAVES		1
#define		N_LOCK_TYPES	2

#define		LOCK_TREE_S			"lock_tree"
#define		LOCK_LEAVES_S		"lock_leaves"

extern char lock_type_names[N_LOCK_TYPES][16];



typedef struct _area_node_t
{
	rect_t	loc;			/* coordinates of the area */
	rect_t	split;			/* segment of line along which this area node is split */

	struct _area_node_t	*left, *right;	/* descendants of this area node */

	entity_set_t*	ent_sets;		/* the sets of entities assigned to this node, one entity set per entity type */

	// sync
	mutex_t* im_mutex;
	mutex_t* ex_mutex;
	cond_t* cond;
	int numEx;
	int excl_waiting;
	
	int n_takes;
	unsigned long long lock_wait;
} area_node_t;

//extern area_node_t	area_tree;


void area_node_init( area_node_t* an, rect_t _loc, int _split_dir, int level );
void area_node_deinit( area_node_t* an );

#define area_node_create( _loc, _split_dir, level )		generic_create3( area_node_t, area_node_init, _loc, _split_dir, level )
#define area_node_destroy( an )							generic_destroy( an, area_node_deinit )

#define area_node_is_leaf( an )							( an->left == NULL )

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
area_node_t* area_node_get_node_containing_ent( area_node_t* an, entity_t* ent );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
area_node_t* area_node_get_node_containing_rect( area_node_t* an, rect_t* r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void area_node_add( area_node_t* an, entity_t* ent );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void area_node_del( area_node_t* an, entity_t* ent );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int area_node_is_vacant( area_node_t* an, rect_t* r, int etypes );

void area_node_lock_tree( area_node_t* an, rect_t* ranges, int n_ranges );
void area_node_unlock_tree( area_node_t* an, rect_t* ranges, int n_ranges );

void area_node_lock_leaves( area_node_t* an, rect_t* ranges, int n_ranges );
void area_node_unlock_leaves( area_node_t* an, rect_t* ranges, int n_ranges );

void area_node_is_valid( area_node_t* an, int* n_ents );


//       OBSOLETE
void area_node_get_entities( area_node_t* an, rect_t* range, int etypes, pentity_set_t* pe_set );


/*		AREA_NODE POINTER ELEMENT		*/

typedef struct
{
	elem_t e;
	area_node_t* an;
} parea_node_t;

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void parea_node_init( parea_node_t* pan, area_node_t* an )
{
	assert( pan && an );
	elem_init( &pan->e, 0 );
	pan->an = an;
}

#define parea_node_create( an )				generic_create1( parea_node_t, parea_node_init, an )
#define parea_node_destroy( pan )			generic_free( pan )




/*		AREA_NODE POINTER SET		*/

#define parea_node_set_t							hset_t

#define parea_node_set_init( pan_s )				hset_init( pan_s )
#define parea_node_set_deinit( pan_s )				hset_deinit( pan_s, parea_node_destroy )
#define parea_node_set_create()						hset_create2( 1, 1000000000 )
#define parea_node_set_destroy( pan_s )				hset_destroy( pan_s, parea_node_destroy )
#define parea_node_set_add( pan_s, pan )			hset_add( pan_s, pan )

#define parea_node_set_del( pan_s, pan )			hset_del( pan_s, pan )

#define parea_node_set_for_each( pos, pan_s )		hset_key_for_each( pos, pan_s, 0 )
#define parea_node_set_size( pan_s )				hset_size( pan_s )


void area_node_get_nodes( area_node_t* an, rect_t* range, parea_node_set_t* pan_set );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
void area_node_get_leaves( area_node_t* an, rect_t* range, parea_node_set_t* pan_set );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
void area_node_get_parents( area_node_t* an, rect_t* range, parea_node_set_t* pan_set );




typedef struct _level_stats_t
{
	int* n_ents;
	int* min_ents;
	int* max_ents;
} level_stats_t;

void area_node_print( area_node_t* an, int level, int verbose, level_stats_t* level_stats );





#endif /*AREA_NODE_H_*/
