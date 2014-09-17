#include "worldmap.h"

#include "../../game/game.h"
#include "../../utils/malloc.h"

worldmap_t wm;


void worldmap_init( conf_t* c )
{
	int i;

	vect_init( &wm.size, conf_get_int(c, "map.size_x"), conf_get_int(c, "map.size_y") );

	rect_init4( &wm.map_r, 0, 0, wm.size.x, wm.size.y );
	rect_init4( &wm.map_walls[DIR_UP],            0, wm.size.y, wm.size.x, wm.size.y );
	rect_init4( &wm.map_walls[DIR_RIGHT], wm.size.x,         0, wm.size.x, wm.size.y );
	rect_init4( &wm.map_walls[DIR_DOWN],          0,         0, wm.size.x,         0 );
	rect_init4( &wm.map_walls[DIR_LEFT],          0,         0,         0, wm.size.y );
	wm.area = rect_area( &wm.map_r );

	int depth  = conf_get_int( c, "map.tree_depth" );
	wm.area_tree = area_node_create( wm.map_r, DIR_UP, depth );

	wm.n_entities = (int *)calloc_wrapper( n_entity_types, sizeof( int ) );		assert( wm.n_entities );
	wm.et_ratios  = (int *)calloc_wrapper( n_entity_types, sizeof( int ) );		assert( wm.et_ratios );
	wm.entities   = (entity_t ***)malloc_wrapper( n_entity_types * sizeof( entity_t** ) );assert( wm.entities );

	for( i = 0; i < n_entity_types; i++ )
	{	wm.entities[i] = (entity_t **)calloc_wrapper( MAX_ENTITIES, sizeof( entity_t* ) );assert( wm.entities[i] );	}
}


int worldmap_add( entity_t* ent )
{
	assert( ent && entity_is_valid( ent, &wm.size) );
	int et_id = ent->ent_type;

	if( wm.et_ratios[et_id] >= (wm.area * entity_types[et_id]->ratio)/MAX_RATIO )
	{
		printf("[I] entity_type %s: ratio limit reached\n", entity_types[et_id]->name);
		return 0;
	}
	if( wm.n_entities[et_id] == MAX_ENTITIES )
	{
		printf("[W] entity_type %s: max entities limit reached\n", entity_types[et_id]->name);
		return 0;
	}

	if( ent->ent_id == -1 )
	{
		int i;
		for( i = 0; i < MAX_ENTITIES; i++ )		if( !wm.entities[et_id][i] )		break;
		assert( i < MAX_ENTITIES );
		ent->ent_id = i;
	}

	wm.entities[et_id][ent->ent_id] = ent;
	wm.n_entities[et_id]++;
	wm.et_ratios[et_id] += rect_area( &ent->r );

	area_node_insert( wm.area_tree, ent );
	return 1;
}

void worldmap_del( entity_t* ent )
{
	assert( ent );
	int et_id = ent->ent_type;

	wm.et_ratios[et_id] -= rect_area( &ent->r );
	wm.n_entities[et_id]--;
	wm.entities[et_id][ent->ent_id] = NULL;

	area_node_remove( wm.area_tree, ent );
}

void worldmap_move( entity_t* ent, vect_t* move_v )
{
	area_node_remove( wm.area_tree, ent );

	vect_add( &ent->r.v1, move_v, &ent->r.v1 );
	vect_add( &ent->r.v2, move_v, &ent->r.v2 );

	area_node_insert( wm.area_tree, ent );
}

int worldmap_is_vacant_from_fixed( rect_t* r )
{
	int i, etypes = 0;
	for( i = 0; i < n_entity_types; i++ )
		if( entity_types[i]->fixed )	etypes |= ( 1 << i );
	return area_node_is_vacant( wm.area_tree, r, etypes );
}
int worldmap_is_vacant_from_mobile( rect_t* r )
{
	int i, etypes = 0;
	for( i = 0; i < n_entity_types; i++ )
		if( !entity_types[i]->fixed )	etypes |= ( 1 << i );
	return area_node_is_vacant( wm.area_tree, r, etypes );
}

int worldmap_is_vacant( rect_t* r )
{
	return area_node_is_vacant( wm.area_tree, r, 0xffffffff );
}


pentity_set_t* worldmap_get_entities( rect_t* range, int etypes )
{
	pentity_set_t* pe_set = pentity_set_create();
	area_node_get_entities( wm.area_tree, range, etypes, pe_set );
	return pe_set;
}

void worldmap_unpack( streamer_t* st )
{
	int i, j, n_entities;
	for( i = 0; i < n_entity_types; i++ )
	{
		if( i == ET_WALL )		continue;

		// remove mobile entities
		if( !entity_types[i]->fixed )
			for( j = 0; j < MAX_ENTITIES; j++ )
			{
				entity_t* ent = wm.entities[i][j];
				if( ent )
				{
					worldmap_del( ent );
					entity_destroy( ent );
				}
			}

		n_entities = streamer_rdint( st );
		for( j = 0; j < n_entities; j++ )
		{
			int e_id = streamer_rdint( st );

			if( !entity_types[i]->fixed )
			{
				entity_t* ent = entity_create( i );
				ent->ent_id = e_id;
				entity_generate_attrs( ent );
				game_entity_unpack( ent, st );
				assert( worldmap_add( ent ) );
			}
			else
			{
				game_entity_unpack( wm.entities[i][e_id], st );
				assert( entity_is_valid( wm.entities[i][e_id], &wm.size ) );
			}
		}
	}
}





void worldmap_unpack_fixed( streamer_t* st )
{
	int i, j, n_entities;

	for( i = 0; i < n_entity_types; i++ )
	{
		if( !entity_types[i]->fixed )	continue;

		n_entities = streamer_rdint( st );
		for( j = 0; j < n_entities; j++ )
		{
			entity_t* ent = entity_create( i );
			ent->ent_id = j;
			entity_generate_attrs( ent );
			entity_unpack_fixed( ent, st );

			assert( worldmap_add( ent ) );
		}
	}
}



