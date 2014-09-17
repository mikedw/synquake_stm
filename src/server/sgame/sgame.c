#include "../../utils/malloc.h"
#include "sgame.h"


rect_t* game_action_compute_compound_range( sv_client_t* cl )
{
	int i_act, a_id;
	rect_t* compound_r = cl->compound_r;
	entity_t* pl = cl->player;
	
	rect_t pl_loc = pl->r;
	value_t speed = pl->attrs[PL_SPEED];
	value_t dir = pl->attrs[PL_DIR];
	for( i_act = 0; i_act < sv.n_multiple_actions; i_act++ )
	{
		a_id = cl-> m_actions[i_act][M_ACT_ID];
		assert( a_id >= 0 && a_id < n_actions );
		
		if( a_id == AC_MOVE )
		{
			cl-> m_actions[i_act][M_ACT_SPD] = attribute_type_adjust_value( ET_PLAYER, PL_SPEED, cl-> m_actions[i_act][M_ACT_SPD] );
			cl-> m_actions[i_act][M_ACT_DIR] = attribute_type_adjust_value( ET_PLAYER, PL_DIR, cl-> m_actions[i_act][M_ACT_DIR] );
			
			speed = action_ranges[AC_MOVE].front * cl-> m_actions[i_act][M_ACT_SPD];
			dir   = cl-> m_actions[i_act][M_ACT_DIR];
		}
		
		compound_r[ i_act ] = game_action_range2( a_id, &pl_loc, speed, dir, &wm.map_r );
		if( a_id == AC_MOVE )
			pl_loc = compound_r[ i_act ];
	}
	
	return compound_r;
}

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int game_action_colision_detection( entity_t* pl, entity_t* ent, int move_dist )
{
	if( entity_is_eq( pl, ent ) )				return move_dist;

	int dist = rect_distance( &pl->r, &ent->r );		assert( dist != -1 );
	if( dist < move_dist )			move_dist = dist;
	return move_dist;
}

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int game_action_eat_entity( entity_t* ent )
{
	int eat_rez = ent->attrs[AP_FOOD] > entity_types[ET_APPLE]->attr_types[AP_FOOD].min;
	assert( eat_rez == 0 || eat_rez == 1 );
	entity_set_attr( ent, AP_FOOD, ent->attrs[AP_FOOD] - eat_rez );

	return eat_rez;
}

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int game_action_attack_entity( entity_t* pl, entity_t* ent )
{
	if( entity_is_eq( pl, ent ) )			return 0;

	int attack_rez = pl->attrs[PL_LIFE] - ent->attrs[PL_LIFE];
	if( attack_rez > 0 )			attack_rez = 1;
	if( attack_rez < 0 )			attack_rez = -1;

	entity_set_attr( ent, PL_LIFE, ent->attrs[PL_LIFE] - attack_rez );
	// signals that the player has been attacked, used for rendering
	if( attack_rez > 0 )	entity_set_attr( ent, PL_HIT, 1 );

	return attack_rez;
}

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int game_action_node( int a_id, entity_t* pl, rect_t* range, int etypes, area_node_t* an, int acc )
{
	int i;
	elem_t* pos;

	for( i = 0; i < n_entity_types; i++ )
	{
		if( ((1 << i) & etypes) == 0 )	continue;

		entity_set_for_each( pos, &an->ent_sets[i] )
		{
			entity_t* ent = (entity_t*) pos;

			time_event( CHECK_PL, 0 );
			
			#ifdef DELAY_OPS
			int n;
			for( n = 0; n < DELAY_OPS; n++ )    asm volatile("nop");
			#endif
			
			assert( rect_is_contained( &ent->r, &an->loc ) );
			assert( area_node_is_leaf( an ) || rect_is_overlapping( &an->split, &ent->r ) );

			if( !rect_is_overlapping( range, &ent->r ) )	continue;

			if( a_id == AC_MOVE )	acc = game_action_colision_detection( pl, ent, acc );
			if( a_id == AC_EAT )	acc += game_action_eat_entity( ent );
			if( a_id == AC_ATTACK )	acc += game_action_attack_entity( pl, ent );
			
			#ifndef CHECK_ALL
			if( a_id == AC_MOVE && acc == 0 )	break;
			#endif
		}
		#ifndef CHECK_ALL
		if( a_id == AC_MOVE && acc == 0 )	break;
		#endif
	}

	return acc;
}

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int game_action_finalize( entity_t* pl, int a_id, int acc )
{
	if( a_id == AC_MOVE && acc != 0 )
	{
		value_t dir = pl->attrs[PL_DIR];
		int dist = rect_distance( &pl->r, &wm.map_walls[dir] );	assert( dist != -1 );
		if( dist < acc )		acc = dist;
	}

	if( a_id == AC_MOVE )
	{
		vect_t move_v;
		vect_scale( &dirs[pl->attrs[PL_DIR]], acc, &move_v );
		worldmap_move( pl, &move_v );
	}
	if( a_id == AC_EAT && acc != 0 )
		entity_set_attr( pl, PL_LIFE, pl->attrs[PL_LIFE] + acc );
	if( a_id == AC_ATTACK && acc != 0 )
	{
		entity_set_attr( pl, PL_LIFE, pl->attrs[PL_LIFE] + acc );
		if( acc < 0 )		entity_set_attr( pl, PL_HIT, 1 );
	}
	
	return acc;
}



void ANT_lock( area_node_t* at, rect_t* ranges, int n_ranges )
{
	if( sv.lock_type == LOCK_TREE )			area_node_lock_tree( at, ranges, n_ranges );
	else if( sv.lock_type == LOCK_LEAVES )	area_node_lock_leaves( at, ranges, n_ranges );
	else assert(0);
}

void ANT_unlock( area_node_t* at, rect_t* ranges, int n_ranges )
{
	if( sv.lock_type == LOCK_TREE )			area_node_unlock_tree( at, ranges, n_ranges );
	else if( sv.lock_type == LOCK_LEAVES )	area_node_unlock_leaves( at, ranges, n_ranges );
	else assert(0);
}



void game_multiple_action( sv_client_t* cl )
{
	assert( cl && cl->player && cl->player->ent_type == ET_PLAYER );
	
	elem_t * pos;
	int i_act, acc, a_id, etypes;
	rect_t act_r;
	entity_t* pl = cl->player;
	cl->last_dist = 0;

	assert( sv.n_multiple_actions );
	parea_node_set_t** pan_set_leaves = (parea_node_set_t**)calloc_wrapper( sv.n_multiple_actions, sizeof(parea_node_set_t*) );
	parea_node_set_t** pan_set_parents = (parea_node_set_t**)calloc_wrapper( sv.n_multiple_actions, sizeof(parea_node_set_t*) );
	assert( pan_set_leaves && pan_set_parents );

	// Timer variables
	unsigned long long now_0, now_1, now_2, now_3, now_4, now_tmp = 0;
	unsigned long long time_lock_parents = 0, time_unlock_parents = 0;
	unsigned long long time_get_nodes = 0;

	// Compute the compund range for all the actions
	rect_t* compound_r = game_action_compute_compound_range( cl );

        // For Intel TM, lets move the game_action_range call outside the processing forloops to allow memory allocation.
        for (i_act = 0; i_act < sv.n_multiple_actions; ++i_act)
        {
                a_id = cl-> m_actions[i_act][M_ACT_ID];
                assert( a_id >= 0 && a_id < n_actions );

                acc = 0;
                if( a_id == AC_MOVE )
                {
                        entity_set_attr( pl, PL_SPEED, cl-> m_actions[i_act][M_ACT_SPD] );
                        entity_set_attr( pl, PL_DIR, cl-> m_actions[i_act][M_ACT_DIR] );
                        acc = action_ranges[AC_MOVE].front * pl->attrs[PL_SPEED];
                }

                etypes = game_action_etypes( a_id );
                act_r = game_action_range( a_id, pl, &wm.map_r );

                // Create leaves
                now_tmp = get_c();
                pan_set_leaves[i_act] = worldmap_get_leaves( &act_r );
		time_get_nodes += (get_c() - now_tmp);

		// Create parent leaves
                now_tmp = get_c();
                pan_set_parents[i_act] = worldmap_get_parents( &act_r );
		time_get_nodes += (get_c() - now_tmp);
        }

        now_0 = get_c();
 
#ifdef INTEL_TM
	__transaction [[TRANSATION_TYPE]] {
#else
	ANT_lock( wm.area_tree, compound_r, sv.n_multiple_actions );
#endif
        now_1 = get_c();

	for (i_act = 0; i_act < sv.n_multiple_actions; ++i_act)
	{
		// Process leaves
		parea_node_set_for_each( pos, pan_set_leaves[i_act] )
		{
			acc = game_action_node( a_id, pl, &act_r, etypes, ((parea_node_t*) pos)->an, acc );
			#ifndef CHECK_ALL
			if( a_id == AC_MOVE && acc == 0 )	break;
			#endif
		}

		// Process parents
		#ifndef CHECK_ALL
		if( !(a_id == AC_MOVE && acc == 0) )
		#endif
		{
			parea_node_set_for_each( pos, pan_set_parents[i_act] )
			{
				area_node_t* an = ((parea_node_t*) pos)->an;

				// TM doesn't need the lock timers
#ifndef INTEL_TM
				if( sv.lock_type == LOCK_LEAVES )	time_lock_parents += time_function( mutex_lock( an->ex_mutex ), now_tmp );
#endif

				acc = game_action_node( a_id, pl, &act_r, etypes, an, acc );

#ifndef INTEL_TM
				if( sv.lock_type == LOCK_LEAVES )	time_unlock_parents += time_function( mutex_unlock( an->ex_mutex ), now_tmp );
#endif

				#ifndef CHECK_ALL
				if( a_id == AC_MOVE && acc == 0 )	break;
				#endif
			}
		}

		acc = game_action_finalize( pl, a_id, acc );
		if( a_id == AC_MOVE )	cl->last_dist += acc;
		if( a_id == AC_MOVE && acc == 0 && sv.move_fail_stop )	break;
	}

	now_2 = get_c();

#ifdef INTEL_TM
	}
#else
	ANT_unlock( wm.area_tree, compound_r, sv.n_multiple_actions );
#endif

	now_3 = get_c();
	
	for( i_act = 0; i_act < sv.n_multiple_actions; i_act++ )
	{
		if( pan_set_leaves[i_act] )	parea_node_set_destroy( pan_set_leaves[i_act] );
		if( pan_set_parents[i_act] )	parea_node_set_destroy( pan_set_parents[i_act] );
	}

	free_wrapper( pan_set_leaves );
	free_wrapper( pan_set_parents );
	now_4 = get_c();

	time_event( GET_NODES, time_get_nodes );
	time_event( LOCK, (now_1 - now_0) );
	time_event( LOCK_P, time_lock_parents );
	time_event( ACTION, (now_2 - now_1 - time_get_nodes - time_lock_parents - time_unlock_parents) );
	time_event( UNLOCK, (now_3 - now_2) );
	time_event( UNLOCK_P, time_unlock_parents );
	time_event( DEST_NODES, (now_4 - now_3) );
}

void game_action( int a_id, entity_t* pl, value_t pl_speed, value_t pl_dir )
{
	volatile unsigned long long now_0=0, now_1=0, now_2=0, now_3=0, now_4=0, now_5=0, now_tmp=0;
	volatile unsigned long long time_lock_parents=0, time_unlock_parents=0, time_get_nodes=0;
	assert( a_id >= 0 && a_id < n_actions && pl && pl->ent_type == ET_PLAYER );

	int acc = 0;
	elem_t * pos;
	parea_node_set_t* pan_set_leaves = NULL;
	parea_node_set_t* pan_set_parents = NULL;

	rect_t ar = game_action_range( a_id, pl, &wm.map_r );
	int etypes = game_action_etypes( a_id );

	if( a_id == AC_MOVE )
	{
		entity_set_attr( pl, PL_SPEED, pl_speed );
		entity_set_attr( pl, PL_DIR, pl_dir );
		acc = action_ranges[AC_MOVE].front * pl->attrs[PL_SPEED];
	}

	now_0 = get_c();

	//mike: !!!!! comment out for now
	pan_set_leaves = worldmap_get_leaves( &ar );

	now_1 = get_c();
	ANT_lock( wm.area_tree, &ar, 1 );
	now_2 = get_c();

	//       Process leaves
	parea_node_set_for_each( pos, pan_set_leaves )
	{
		acc = game_action_node( a_id, pl, &ar, etypes, ((parea_node_t*) pos)->an, acc );
		#ifndef CHECK_ALL
		if( a_id == AC_MOVE && acc == 0 )	break;
		#endif
	}
	
	//       Process parents
	#ifndef CHECK_ALL
	if( !(a_id == AC_MOVE && acc == 0) )
	#endif
	{
		now_tmp = get_c();
		pan_set_parents = worldmap_get_parents( &ar );	time_get_nodes += (get_c() - now_tmp);

		parea_node_set_for_each( pos, pan_set_parents )
		{
			area_node_t* an = ((parea_node_t*) pos)->an;
			
			if( sv.lock_type == LOCK_LEAVES )	time_lock_parents += time_function( mutex_lock( an->ex_mutex ), now_tmp );
			acc = game_action_node( a_id, pl, &ar, etypes, an, acc );
			if( sv.lock_type == LOCK_LEAVES )	time_unlock_parents += time_function( mutex_unlock( an->ex_mutex ), now_tmp );
			
			#ifndef CHECK_ALL
			if( a_id == AC_MOVE && acc == 0 )	break;
			#endif
		}
	}

	acc = game_action_finalize( pl, a_id, acc );
	
	now_3 = get_c();
	ANT_unlock( wm.area_tree, &ar, 1 );

	now_4 = get_c();
	parea_node_set_destroy( pan_set_leaves );
	if( pan_set_parents )	parea_node_set_destroy( pan_set_parents );
	now_5 = get_c();

	time_event( GET_NODES, (now_1 - now_0 + time_get_nodes) );
	time_event( LOCK, (now_2 - now_1) );
	time_event( LOCK_P, time_lock_parents );
	time_event( ACTION, (now_3 - now_2 - time_lock_parents - time_unlock_parents - time_get_nodes) );
	time_event( UNLOCK, (now_4 - now_3) );
	time_event( UNLOCK_P, time_unlock_parents );
	time_event( DEST_NODES, (now_5 - now_4) );
}
