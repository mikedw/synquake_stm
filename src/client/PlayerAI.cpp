
/***************************************************************************************************
*
* SUBJECT:
*    A Benckmark for Massive Multiplayer Online Games
*    Game Server and Client
*
* AUTHOR:
*    Mihai Paslariu
*    Politehnica University of Bucharest, Bucharest, Romania
*    mihplaesu@yahoo.com
*
* TIME AND PLACE:
*    University of Toronto, Toronto, Canada
*    March - August 2007
*
***************************************************************************************************/
#include <stdlib.h>

#include "PlayerAI.h"

extern "C"	{
#include "../game/game.h"

#include "cgame/worldmap.h"
}

/***************************************************************************************************
*
* Constructor
*
***************************************************************************************************/

PlayerAI::PlayerAI()
{
	current_path = new StepStack();
	new_path	 = new StepStack();

	debug_AI = cl.debug_AI;
	wait_count = 0;
}

/***************************************************************************************************
*
* Condition check methods
*
***************************************************************************************************/

rect_t* PlayerAI::foodIsVisible()
{
	value_t food_min = entity_types[ET_APPLE]->attr_types[AP_FOOD].min;
	return findNearObject( ET_APPLE, AP_FOOD, food_min+1, 0x7F );
}

rect_t* PlayerAI::weakPlayerNear()
{
	return findNearObject( ET_PLAYER, PL_LIFE, 10, cl.pl->attrs[PL_LIFE]-1 );
}

rect_t* PlayerAI::strongPlayerNear()
{
	return findNearObject( ET_PLAYER, PL_LIFE, cl.pl->attrs[PL_LIFE]+1, 0x7F );
}

rect_t* PlayerAI::findNearObject( int ent_type, int attr_id, value_t min_value, value_t max_value)
{
	assert( ent_type < n_entity_types && attr_id < entity_types[ ent_type ]->n_attr );

	rect_t* closest_r = NULL;
	int min_dist = 1000000;

	elem_t* pos;
	pentity_set_t* pe_set = worldmap_get_entities( &cl.view_r, 1 << ent_type );
	pentity_set_for_each( pos, pe_set )
	{
		entity_t* ent = ((pentity_t*)pos)->ent;
		if( entity_is_eq( ent, cl.pl ) )		continue;
		if( ent->attrs[attr_id] < min_value || max_value < ent->attrs[attr_id] )
			continue;

		int dist = rect_distance( &cl.pl->r, &ent->r );
		if( dist < min_dist ){	min_dist = dist; closest_r = &ent->r;	}
	}
	pentity_set_destroy( pe_set );

	return closest_r;
}

bool PlayerAI::isPurposeValid( int p, rect_t* purpose_r )
{
	rect_t* purp_r = NULL;
	switch( p )
	{
		case SEEKING_QUEST:
			if( cl.quest_active )	purp_r = &cl.quest_r;
			break;

		case SEEKING_FOOD:
			{
				value_t max_life = entity_types[ET_PLAYER]->attr_types[PL_LIFE].max;
				if( cl.pl->attrs[PL_LIFE] < max_life )		purp_r = foodIsVisible();
			}
			break;

		case RUNNING_AWAY:		purp_r = strongPlayerNear();	break;
		case CHASING_PLAYER:	purp_r = weakPlayerNear();		break;
		case EXPLORING:			purp_r = &cl.pl->r;				break;
		default: assert(0);
	};

	if( !purp_r )	return false;
	*purpose_r = *purp_r;
	return true;
}

bool PlayerAI::isDestinationValid( rect_t* dest_r, int p, rect_t* purpose_r )
{
	if( !worldmap_is_vacant_from_fixed( dest_r ) )							return false;
	switch( p )
	{
		case SEEKING_QUEST:
			if( rect_is_overlapping( dest_r, purpose_r ) &&
				!vect_is_eq( &cl.pl->r.v1, &dest_r->v1 ) )					return true;
			break;
		case RUNNING_AWAY:
			if( rect_distance( dest_r, purpose_r ) < 2*action_ranges[AC_VIEW].front )
				return false;
			{
				vect_t aux_v, aux_v1, aux_v2;
				rect_t aux_r;
				vect_substract( &purpose_r->v1, &cl.pl->r.v1, &aux_v );
				double m1 = ( 2*action_ranges[AC_VIEW].front ) / ((double)(abs( aux_v.x ) + abs( aux_v.y )));
				double m2 = ( wm.size.x + wm.size.y ) / ((double)(abs( aux_v.x ) + abs( aux_v.y )));

				// compute area where we want to run away
				vect_scale( &aux_v, m1, &aux_v1 );	vect_substract( &purpose_r->v1, &aux_v1, &aux_v1 );
				vect_scale( &aux_v, m2, &aux_v2 );	vect_substract( &purpose_r->v1, &aux_v2, &aux_v2 );

				vect_init( &aux_r.v1, min(aux_v1.x,aux_v2.x), min(aux_v1.y,aux_v2.y) );
				vect_init( &aux_r.v2, max(aux_v1.x,aux_v2.x), max(aux_v1.y,aux_v2.y) );

				// check whether the run away area is valid
				if( !rect_crop( &aux_r, &wm.map_r, &aux_r ) ||
					!rect_can_fit_inside( &cl.pl->r, &aux_r ) )					return true;
				if( rect_is_contained( dest_r, &aux_r) )						return true;
			}
			break;
		case SEEKING_FOOD:
		case CHASING_PLAYER:
			if( rect_is_nextto( dest_r, purpose_r ) )						return true;
			break;
		case EXPLORING:
			if( vect_distance( &dest_r->v1, &purpose_r->v1 ) > 0 )			return true;
			break;
		default: assert(0);
	};
	return false;
}

void PlayerAI::generateDestination( int p, rect_t* purpose_r, rect_t* dest_r )
{
	switch( p )
	{
		case SEEKING_QUEST:		rect_generate_overlapping( dest_r, purpose_r, &cl.pl->size, &wm.size );	break;
		case RUNNING_AWAY:
			{
				vect_t aux_v, aux_v1, aux_v2;
				rect_t aux_r;
				vect_substract( &purpose_r->v1, &cl.pl->r.v1, &aux_v );
				double m = ( 2*action_ranges[AC_VIEW].front + cl.pl->size.x + cl.pl->size.y ) /
															((double)(abs( aux_v.x ) + abs( aux_v.y )));

				// compute area where we can run away
				vect_scale( &aux_v,   m, &aux_v1 );	vect_substract( &purpose_r->v1, &aux_v1, &aux_v1 );
				vect_scale( &aux_v, 2*m, &aux_v2 );	vect_substract( &purpose_r->v1, &aux_v2, &aux_v2 );

				vect_init( &aux_r.v1, min(aux_v1.x,aux_v2.x), min(aux_v1.y,aux_v2.y) );
				vect_init( &aux_r.v2, max(aux_v1.x,aux_v2.x), max(aux_v1.y,aux_v2.y) );

				// check whether the run away area is valid
				if( !rect_crop( &aux_r, &wm.map_r, &aux_r ) || !rect_can_fit_inside( &cl.pl->r, &aux_r ) )
					rect_generate_position( dest_r, &cl.pl->size, &wm.map_r );
				else
					rect_generate_position( dest_r, &cl.pl->size, &aux_r );
			}
			break;
		case SEEKING_FOOD:
		case CHASING_PLAYER:
			{
				rect_t closest;
				int i, dist, min_dist = 1000000;
				for( i = 0; i < 10; i++ )
				{
					rect_generate_nextto( dest_r, purpose_r, &cl.pl->size, &wm.size );
					if( !worldmap_is_vacant_from_fixed( dest_r ) )		continue;

					dist = rect_distance( &cl.pl->r, dest_r );
					if( dist < min_dist )
					{
						min_dist = dist;
						closest = *dest_r;
					}
				}
				if( min_dist < 1000000 )	*dest_r = closest;
			}
			break;
		case EXPLORING:			rect_generate_position( dest_r, &cl.pl->size, &wm.map_r );				break;
		default: assert(0);
	};
}


bool PlayerAI::isPathValid( StepStack* path, int p, rect_t* purpose_r )
{
	if( path->empty() )											return false;
	if( !isDestinationValid( &path->dest_r, p, purpose_r ) )	return false;
	return true;
}


void PlayerAI::computePath( StepStack* path )
{
	pathfind_alg.findPath( path, &cl.pl->r.v1, &cl.pl->size, 1024, true, true );
}

void PlayerAI::recomputePath( StepStack* path )
{
	pathfind_alg.findPath( path, &cl.pl->r.v1, &cl.pl->size, 128, true, false );
}

int PlayerAI::stepAlongPath( StepStack* path, rect_t** move_dest_out )
{
	// partial path ended
	if( path->empty() )		recomputePath( path );
	if( path->empty() )		return -1;

	rect_t* move_dest = path->top();
	assert( cl.pl->r.v1.x == move_dest->v1.x || cl.pl->r.v1.y == move_dest->v1.y );

	*move_dest_out = move_dest;
	return vect_distance( &cl.pl->r.v1, &move_dest->v1 );
}

int  PlayerAI::moveAlongPath( StepStack* path )
{
	// destination reached
	if( vect_is_eq( &cl.pl->r.v1, &path->dest_r.v1 ) )		return 0;

	rect_t* move_dest;
	value_t move_dist = stepAlongPath( path, &move_dest );
	if( move_dist  < 0 )		return -1;
	if( move_dist == 0 )
	{
		path->pop();
		move_dist = stepAlongPath( path, &move_dest );

		if( move_dist  < 0 )		return -1;
		assert( move_dist > 0 );
	}
	entity_set_attr( cl.pl, PL_SPEED, move_dist );
	entity_set_attr( cl.pl, PL_DIR, vect_direction( &cl.pl->r.v1, &move_dest->v1 ) );

	// perform collision detection
	elem_t* pos;
	rect_t* closest_r = NULL;
	int min_dist = 1000000;

	rect_t ar = game_action_range( AC_MOVE, cl.pl, &wm.map_r );		// get the area from the map affected by this action
	int etypes = game_action_etypes( AC_MOVE );						// get the bitset of entity_types affected by this action
	pentity_set_t* pe_set = worldmap_get_entities( &ar, etypes );	// get all the entities present in the "ar" area

	pentity_set_for_each( pos, pe_set )
	{
		entity_t* ent = ((pentity_t*)pos)->ent;
		if( entity_is_eq( ent, cl.pl ) )		continue;

		int dist = rect_distance( &cl.pl->r, &ent->r );
		if( dist < min_dist ){	min_dist = dist; closest_r = &ent->r;	}
	}
	pentity_set_destroy( pe_set );

	if( closest_r )		move_dist = min_dist;
	// cannot move, search new path
	if( move_dist == 0 )
	{
		path->clear();
		move_dist = stepAlongPath( path, &move_dest );

		if( move_dist  < 0 )		return -1;
		assert( move_dist > 0 );
	}

	entity_set_attr( cl.pl, PL_SPEED, move_dist );
	entity_set_attr( cl.pl, PL_DIR, vect_direction( &cl.pl->r.v1, &move_dest->v1 ) );


	if( debug_AI )
		printf( "%s - Moving from %d,%d to %d,%d (%d) towards %d,%d\n", cl.name,
							cl.pl->r.v1.x, cl.pl->r.v1.y, move_dest->v1.x, move_dest->v1.y,
											move_dist, path->dest_r.v1.x, path->dest_r.v1.y );
	return move_dist;
}

/***************************************************************************************************
*
* Main action taking method
*
***************************************************************************************************/

PlayerAI_Action PlayerAI::takeAction()
{
	assert( cl.last_action_sv == cl.last_action-1 || cl.last_action_sv == cl.last_action );
	if( cl.last_action_sv == cl.last_action-1 )
	{
		wait_count++;
		if( wait_count < AI_RETRY_COUNT )		return NO_ACTION;

		wait_count = 0;
		cl.last_action_sv++;
		printf("[W][AI]Wait limit for action confirmation reached.\n");
	}

	rect_t purp_r;
	int p;
	for( p = SEEKING_QUEST; p <= EXPLORING; p++ )
	{
		// validate "p" as our next purpose
		if( !isPurposeValid( p, &purp_r) )				continue;

		// see whether we can use the current path
		if( isPathValid( current_path, p, &purp_r ) )	break;

		int trials;
		for( trials = 0; trials < 10; trials++ )
		{
			generateDestination( p, &purp_r, &new_path->dest_r );
			if( !isDestinationValid( &new_path->dest_r, p, &purp_r ) )	continue;

			computePath( new_path );

			if( !new_path->empty() )		break;
		}
		if( trials < 10 )
		{
			StepStack* aux = current_path;
			current_path = new_path;
			new_path = aux;
			cl.purpose = BASIC;
			break;
		}
	}
	if( p > EXPLORING )
	{	printf("[W] %s failed to find any valid purpose.\n", cl.name );	return NO_ACTION;	}
	if( debug_AI && p != cl.purpose )
	{	printf( "[AI] %s - %s %d,%d\n", cl.name, AI_debug_msgs[p], purp_r.v1.x, purp_r.v1.y );fflush(stdout);	}
	cl.purpose = p;

	int move_dist = moveAlongPath( current_path );
	if( move_dist < 0 )
	{
		if( debug_AI) printf("[W] %s failed to find any valid path.\n", cl.name );
		return NO_ACTION;
	}

	if( move_dist > 0 )		return MOVE;


	if( p == SEEKING_FOOD || p == CHASING_PLAYER )
	{
		value_t d = -1;
		if( cl.pl->r.v1.y == purp_r.v2.y )	d = DIR_DOWN;
		if( cl.pl->r.v2.y == purp_r.v1.y )	d = DIR_UP;
		if( cl.pl->r.v1.x == purp_r.v2.x )	d = DIR_LEFT;
		if( cl.pl->r.v2.x == purp_r.v1.x )	d = DIR_RIGHT;
		assert( d != -1 );
		entity_set_attr( cl.pl, PL_DIR, d );
	}

	if( p == SEEKING_FOOD )		return EAT;
	if( p == CHASING_PLAYER )	return ATTACK;

	printf("This should not happen, purpose: %d\n", p);
	return NO_ACTION;
}
