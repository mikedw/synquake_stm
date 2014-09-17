#include <assert.h>

#include "../../utils/malloc.h"
#include "../../utils/geometry.h"

#include "area_node.h"

void area_node_init( area_node_t* an, rect_t _loc, int _split_dir, int level )
{
	int i;
	assert( an && _loc.v1.x <= _loc.v2.x && _loc.v1.y <= _loc.v2.y && level >= 0 );
	assert( _split_dir == DIR_UP || _split_dir == DIR_RIGHT );

	an->loc = _loc;
	rect_init4( &an->split, -1, -1, -1, -1 );
	an->left = NULL;an->right = NULL;

	an->ent_sets = (entity_set_t *)malloc_wrapper( n_entity_types * sizeof( entity_set_t ) );
	assert( an->ent_sets );
	for( i = 0; i < n_entity_types; i++ )
		entity_set_init( &an->ent_sets[i] );

	if( level )
	{
		rect_t r_left, r_right;
		rect_split( &_loc, _split_dir, &r_left, &r_right, &an->split );

		an->left  = area_node_create( r_left,  1 - _split_dir, level-1 );
		an->right = area_node_create( r_right, 1 - _split_dir, level-1 );
	}
}

void area_node_deinit( area_node_t* an )
{
	int i;
	assert( an );

	if( !area_node_is_leaf( an ) )
	{
		area_node_destroy( an->left );
		area_node_destroy( an->right );
	}

	for( i = 0; i < n_entity_types; i++ )
		entity_set_deinit( &an->ent_sets[i] );
	free_wrapper( an->ent_sets );
}

/* recursively insert "ent" into the area tree represented by "an" */
/* assumes that "ent" doesn't overlap with any existing entities in "an" */
void area_node_insert( area_node_t* an, entity_t* ent )
{
	if( area_node_is_leaf( an ) || rect_is_overlapping( &an->split, &ent->r ) )
	{
		entity_set_add( &an->ent_sets[ent->ent_type], ent );
		return;
	}

	if( rect_cmp( &ent->r, &an->split ) < 0 )	area_node_insert( an->left, ent );
	else										area_node_insert( an->right, ent );
}

/* recursively remove "ent" from the area tree represented by "an" */
/* assumes that "ent" is placed correctly in "an" */
void area_node_remove( area_node_t* an, entity_t* ent )
{
	if( area_node_is_leaf( an ) || rect_is_overlapping( &an->split, &ent->r ) )
	{
		entity_set_del( &an->ent_sets[ent->ent_type], ent );
		return;
	}

	if( rect_cmp( &ent->r, &an->split ) < 0 )	area_node_remove( an->left, ent );
	else										area_node_remove( an->right, ent );
}

/* checks whether the area occupied by "r" is free within "an" */
int area_node_is_vacant( area_node_t* an, rect_t* r, int etypes )
{
	int i;
	elem_t* pos;
	entity_t* ent;

	if( !rect_is_overlapping( r, &an->loc ) )				return 1;

	for( i = 0; i < n_entity_types; i++ )
		if( etypes & (1<<i) )
			entity_set_for_each( pos, &an->ent_sets[i] )
			{
				ent = (entity_t*)pos;
				if( rect_is_overlapping( r, &ent->r ) )	return 0;
			}

	if( !area_node_is_leaf( an ) )
	{
		if( !area_node_is_vacant( an->left, r, etypes ) )	return 0;
		if( !area_node_is_vacant( an->right, r, etypes ) )	return 0;
	}

	return 1;
}

/* recursively collects into "pe_set" entities located within the */
/* area of "an" that have some overlap with range */
void area_node_get_entities( area_node_t* an, rect_t* range, int etypes, pentity_set_t* pe_set )
{
	int i;
	elem_t* pos;

	if( !rect_is_overlapping( range, &an->loc ) )				return;

	for( i = 0; i < n_entity_types; i++ )
		if( (1 << i) & etypes )
		{
			entity_set_for_each( pos, &an->ent_sets[i] )
			{
				entity_t* ent = (entity_t*)pos;

				assert( rect_is_contained( &ent->r, &an->loc ) );
				assert( area_node_is_leaf( an ) || rect_is_overlapping( &ent->r, &an->split ) );

				if( rect_is_overlapping( range, &ent->r ) )
					pentity_set_add( pe_set, pentity_create( ent ) );
			}
		}

	if( !area_node_is_leaf( an ) )
	{
		area_node_get_entities( an->left, range, etypes, pe_set );
		area_node_get_entities( an->right, range, etypes, pe_set );
	}
}

