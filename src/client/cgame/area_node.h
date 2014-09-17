#ifndef AREA_NODE_H_
#define AREA_NODE_H_

#include "../../general.h"
#include "../../utils/geometry.h"

#include "../../game/entity.h"


typedef struct _area_node_t
{
	rect_t	loc;			/* coordinates of the area */
	rect_t	split;			/* segment of line along which this area node is split */
		
	struct _area_node_t	*left, *right;	/* descendants of this area node */
	
	entity_set_t*	ent_sets;		/* the sets of entities assigned to this node */
} area_node_t;

extern area_node_t	area_tree;


void area_node_init( area_node_t* an, rect_t _loc, int _split_dir, int level );
void area_node_deinit( area_node_t* an );

#define area_node_create( _loc, _split_dir, level )		generic_create3( area_node_t, area_node_init, _loc, _split_dir, level )
#define area_node_destroy( an )							generic_destroy( an, area_node_deinit )

#define area_node_is_leaf( an )							( an->left == NULL )	

void area_node_insert( area_node_t* an, entity_t* ent );
void area_node_remove( area_node_t* an, entity_t* ent );

int area_node_is_vacant( area_node_t* an, rect_t* r, int etypes );
void area_node_get_entities( area_node_t* an, rect_t* range, int etypes, pentity_set_t* pe_set );


#endif /*AREA_NODE_H_*/
