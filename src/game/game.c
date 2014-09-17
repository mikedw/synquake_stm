#include "game.h"

void game_entity_generate_size( entity_t* ent )
{
	assert( ent != NULL );

	if( ent->ent_type == ET_PLAYER )
	{
		ent->size.x = ent->attrs[ PL_SIZE ];
		ent->size.y = ent->size.x;
	}
	if( ent->ent_type == ET_APPLE )
	{
		ent->size.x = ent->attrs[ AP_SIZE ];
		ent->size.y = ent->size.x;
	}
	if( ent->ent_type == ET_WALL )
	{
		if( rand_range(0,1) == 0 )
		{
			ent->size.x = ent->attrs[ WL_SIZE_X ];
			ent->size.y = entity_types[ent->ent_type]->attr_types[ WL_SIZE_Y ].min;
		}
		else
		{
			ent->size.y = ent->attrs[ WL_SIZE_Y ];
			ent->size.x = entity_types[ent->ent_type]->attr_types[ WL_SIZE_X ].min;
		}
	}
}

void game_entity_pack( entity_t* ent, streamer_t* st )
{
	streamer_wrint( st, ent->ent_id );

	// if the entity is mobile pack its location
	if( !entity_types[ent->ent_type]->fixed )
		rect_pack( &ent->r, st );

	// pack only the relevant attributes based on entity type
	int i, n_attr = entity_types[ ent->ent_type ]->n_attr;
	switch( ent->ent_type )
	{
		case ET_PLAYER:
			i = PL_DIR;
			break;
		case ET_APPLE:
			i = AP_FOOD;
			break;
		default:
			i = n_attr;
	}
	assert( sizeof(value_t) == sizeof(int) );
	for(; i < n_attr; i++ )
		streamer_wrint( st, ent->attrs[i] );
}

void game_entity_unpack( entity_t* ent, streamer_t* st )
{
	if( !entity_types[ent->ent_type]->fixed )
	{
		rect_unpack( &ent->r, st );
		vect_substract( &ent->r.v2, &ent->r.v1, &ent->size );
	}

	int i, n_attr = entity_types[ ent->ent_type ]->n_attr;
	switch( ent->ent_type )
	{
		case ET_PLAYER:
			i = PL_DIR;
			break;
		case ET_APPLE:
			i = AP_FOOD;
			break;
		default:
			i = n_attr;
	}
	assert( sizeof(value_t) == sizeof(int) );
	for(; i < n_attr; i++ )
		ent->attrs[i] = streamer_rdint( st );
}



rect_t game_action_range2( int a_id, rect_t* loc, value_t speed, value_t dir, rect_t* map_r )
{
	assert( a_id >= 0 && a_id < n_actions );
	action_range_t ar = action_ranges[ (int)a_id ];
	rect_t r;

	if( a_id == AC_MOVE )	ar.front *= speed;

	if( dir == DIR_UP )		rect_init4(&r, -ar.left, -ar.behind, ar.right, ar.front);
	if( dir == DIR_RIGHT )	rect_init4(&r, -ar.behind, -ar.right, ar.front, ar.left);
	if( dir == DIR_DOWN )	rect_init4(&r, -ar.right, -ar.front, ar.left, ar.behind);
	if( dir == DIR_LEFT )	rect_init4(&r, -ar.front, -ar.left, ar.behind, ar.right);

	vect_add( &r.v1, &loc->v1, &r.v1 );
	vect_add( &r.v2, &loc->v2, &r.v2 );
	rect_crop(&r, map_r, &r);
	return r;
}

rect_t game_action_range( int a_id, entity_t* pl, rect_t* map_r )
{
	assert( a_id >= 0 && a_id < n_actions && pl && pl->ent_type == ET_PLAYER );
	return game_action_range2( a_id, &pl->r, pl->attrs[PL_SPEED], pl->attrs[PL_DIR], map_r );
}


int game_action_etypes( int a_id )
{
	assert( a_id >= 0 && a_id < n_actions );
	int etypes = 0;
	if( a_id == AC_MOVE )
	{
		etypes += 1 << ET_PLAYER;
		etypes += 1 << ET_APPLE;
		etypes += 1 << ET_WALL;
	}
	if( a_id == AC_EAT )
		etypes = 1 << ET_APPLE;
	if( a_id == AC_ATTACK )
		etypes = 1 << ET_PLAYER;

	return etypes;
}











