#include "worldmap.h"
#include "../../utils/malloc.h"
#include "../server.h"
#include "../syn_server_traces.h"

worldmap_t wm;
int* next_ent_id;


void worldmap_init( coord_t szx, coord_t szy, int tdepth )
{
	int i;

	vect_init( &wm.size, szx, szy );

	rect_init4( &wm.map_r, 0, 0, wm.size.x, wm.size.y );
	rect_init4( &wm.map_walls[DIR_UP],            0, wm.size.y, wm.size.x, wm.size.y );
	rect_init4( &wm.map_walls[DIR_RIGHT], wm.size.x,         0, wm.size.x, wm.size.y );
	rect_init4( &wm.map_walls[DIR_DOWN],          0,         0, wm.size.x,         0 );
	rect_init4( &wm.map_walls[DIR_LEFT],          0,         0,         0, wm.size.y );

	wm.area = rect_area( &wm.map_r );
	wm.depth  = tdepth;
	
	wm.area_tree = area_node_create( wm.map_r, DIR_UP, wm.depth );

	wm.n_entities = calloc_wrapper( n_entity_types, sizeof( int ) ); assert( wm.n_entities );
	wm.et_ratios  = calloc_wrapper( n_entity_types, sizeof( int ) ); assert( wm.et_ratios );
	wm.entities   = malloc_wrapper( n_entity_types * sizeof( entity_t** ) ); assert( wm.entities );

	next_ent_id = calloc_wrapper( n_entity_types, sizeof( int ) ); assert( next_ent_id );

	for( i = 0; i < n_entity_types; i++ )
	{
		wm.entities[i] = calloc_wrapper( MAX_ENTITIES, sizeof( entity_t* ) ); assert( wm.entities[i] );
		next_ent_id[i] = 0;
	}

	log_info3( "Worldmap init done - Size: %d x %d - Tree depth: %d\n", wm.size.x, wm.size.y, wm.depth );
}


int worldmap_add( entity_t* ent )
{
	assert( ent && entity_is_valid( ent, &wm.size) );
	int et_id = ent->ent_type;

	if( wm.et_ratios[et_id] >= ( ((double)wm.area) * entity_types[et_id]->ratio)/MAX_RATIO )
	{
		log_info1("[I] entity_type %s: ratio limit reached\n", entity_types[et_id]->name);
		return 0;
	}
	if( wm.n_entities[et_id] == MAX_ENTITIES )
	{
		log_info1("[W] entity_type %s: max entities limit reached\n", entity_types[et_id]->name);
		return 0;
	}

	wm.et_ratios[et_id] += rect_area( &ent->r );

	#ifdef GENERATE_NEAR_QUESTS
	if( et_id != ET_PLAYER )
	{
		int i, j, dist, min_dist = wm.size.x + wm.size.y;
		for( i = 0; i < sv.wl_quest_count; i++ )
		{
			for( j = 0; j < sv.wl_quest_spread; j++ )
			{
				dist = rect_distance( &sv.quest_locations[i][j], &ent->r );
				if( dist < min_dist )	min_dist = dist;
			}
		}
		if( min_dist > sv.n_multiple_actions * entity_types[ ET_PLAYER ]->attr_types[ PL_SPEED ].max )
		{
			entity_destroy( ent );
			return 1;
		}
	}
	#endif

	if( ent->ent_id == -1 )
	{
		int base = next_ent_id[ et_id ];
		int i;
		for( i = 0; i < MAX_ENTITIES; i++ )
			if( !wm.entities[et_id][ (base+i) % MAX_ENTITIES ] )		break;

		assert( i < MAX_ENTITIES );
		ent->ent_id = (base+i) % MAX_ENTITIES;
		next_ent_id[ et_id ] = (base+i+1) % MAX_ENTITIES;
	}

	wm.entities[et_id][ent->ent_id] = ent;
	wm.n_entities[et_id]++;
	
	area_node_t* an = area_node_get_node_containing_ent( wm.area_tree, ent );
	area_node_add( an, ent );

	server_register_object( ent );

	return 1;
}

void worldmap_del( entity_t* ent )
{
	assert( ent );
	int et_id = ent->ent_type;

	wm.et_ratios[et_id] -= rect_area( &ent->r );
	wm.n_entities[et_id]--;
	wm.entities[et_id][ent->ent_id] = NULL;

	area_node_t* an = area_node_get_node_containing_ent( wm.area_tree, ent );
	area_node_del( an, ent );
}

void worldmap_move( entity_t* ent, vect_t* move_v )
{
	#ifndef ALWAYS_MOVE
	if( move_v->x == 0 && move_v->y == 0 )	return;
	#endif

	area_node_t* an_1 = area_node_get_node_containing_rect( wm.area_tree, &ent->r );
	rect_t aux_r;
	vect_add( &ent->r.v1, move_v, &aux_r.v1 );
	vect_add( &ent->r.v2, move_v, &aux_r.v2 );
	area_node_t* an_2 = area_node_get_node_containing_rect( wm.area_tree, &aux_r );

	#ifndef ALWAYS_MOVE
	if( an_1 != an_2 )
	#endif
	{
		//mike: !!!!! get rid of the lock stuff
#ifndef INTEL_TM
		if( sv.lock_type == LOCK_LEAVES && !area_node_is_leaf(an_1) )	mutex_lock( an_1->ex_mutex );
#endif
		area_node_del( an_1, ent );

#ifndef INTEL_TM
		if( sv.lock_type == LOCK_LEAVES && !area_node_is_leaf(an_1) )	mutex_unlock( an_1->ex_mutex );
#endif

	}

	vect_add( &ent->r.v1, move_v, &ent->r.v1 );
	vect_add( &ent->r.v2, move_v, &ent->r.v2 );

	#ifndef ALWAYS_MOVE
	if( an_1 != an_2 )
	#endif
	{	

#ifndef INTEL_TM
		if( sv.lock_type == LOCK_LEAVES && !area_node_is_leaf(an_2) )	mutex_lock( an_2->ex_mutex );
#endif

		area_node_add( an_2, ent );

#ifndef INTEL_TM
		if( sv.lock_type == LOCK_LEAVES && !area_node_is_leaf(an_2) )	mutex_unlock( an_2->ex_mutex );
#endif

	}


}

int worldmap_is_vacant_from_fixed( rect_t* r )
{
	int i, etypes = 0;
	for( i = 0; i < n_entity_types; i++ )
		if( entity_types[i]->fixed )
			etypes |= ( 1 << i );
	return area_node_is_vacant( wm.area_tree, r, etypes );
}
int worldmap_is_vacant_from_mobile( rect_t* r )
{
	int i, etypes = 0;
	for( i = 0; i < n_entity_types; i++ )
		if( !entity_types[i]->fixed )
			etypes |= ( 1 << i );
	return area_node_is_vacant( wm.area_tree, r, etypes );
}

int worldmap_is_vacant( rect_t* r )
{
	return area_node_is_vacant( wm.area_tree, r, 0xffffffff );
}

entity_t* worldmap_generate_ent(int ent_type, rect_t* where )
{
	entity_t* ent = entity_create( ent_type );
	assert( ent );
	entity_generate_attrs( ent );
	game_entity_generate_size( ent );

	int trials = 0;
	do{
		entity_generate_position( ent, where );
		trials++;
	}while( !worldmap_is_vacant( &ent->r ) && trials < 100 );

	if( trials == 100 )
	{
		//mike: can't do printf inside atomic transactions
#ifndef INTEL_TM
		printf("[W] Map too crowded to add entities.\n");
#endif
		entity_destroy( ent ); ent = NULL;
	}
	return ent;
}


#ifndef RUN_TRACES

void worldmap_generate()
{
	log_info("Worldmap generate ...\n");
	entity_t* ent;

	int i;
	for( i = 0; i < n_entity_types; i++ )
	{
		if( !entity_types[i]->fixed )	continue;

		do{
			ent = worldmap_generate_ent( i, &wm.map_r );
		}while(	ent && worldmap_add(ent) );

		if( ent != NULL )     entity_destroy( ent );
	}

	#ifdef REGISTER_TRACES
	fprintf(sv.f_trace_objects, "-1"); // invalid object type to determine EOF
	#endif

	log_info("Worldmap generate done.\n");
}

#else



entity_t* worldmap_generate_ent_from_trace(int ent_type)
{
	int i, check;
	value_t atr;
	short v1x, v1y, v2x, v2y;
	FILE* f_in;
	entity_t* ent = NULL;

	if( ent_type != ET_PLAYER )
	{
		f_in = sv.f_trace_objects;
		fscanf( f_in, "%d", &ent_type);

		if( ent_type == -1 ) 		return NULL;
		assert( ent_type == ET_APPLE || ent_type == ET_WALL );
	}
	else	f_in = sv.f_trace_players;


	entity_type_t* et = entity_types[ent_type];
	ent = entity_create( ent_type ); assert( ent );
	fscanf( f_in, "%hd %hd %hd %hd", &v1x, &v1y, &v2x, &v2y);
	rect_init4( &ent->r, v1x, v1y, v2x, v2y );

	#ifdef TRACE_DEBUG
	printf( "%s [%hd;%hd - %hd;%hd]: \n", et->name, v1x, v1y, v2x, v2y );
	#endif

	for (i = 0; i < et->n_attr; ++i)
	{
		fscanf( f_in, "%d %d", &check, &atr);	assert( check == i );
		ent->attrs[i] = atr;

		#ifdef TRACE_DEBUG
		printf("\t Attr[%d]=%d\n", i, atr);
		#endif
	}

	if(ent_type == ET_PLAYER)
	{
		ent->size.x = ent->attrs[ PL_SIZE ];
		ent->size.y = ent->size.x;
	}
	if( ent_type == ET_APPLE )
	{
		ent->size.x = ent->attrs[ AP_SIZE ];
		ent->size.y = ent->size.x;
	}
	if( ent_type == ET_WALL )
	{
		ent->size.x = ent->attrs[ WL_SIZE_X ];
		ent->size.y = ent->attrs[ WL_SIZE_Y ];
	}

	return ent;
}

void worldmap_generate()
{
	log_info( "Worldmap generate ...\n" );
	entity_t* ent;

	do{
		ent = worldmap_generate_ent_from_trace(ET_APPLE); // or ET_WALL, just different than ET_PLAYER
	}while(	ent && worldmap_add(ent) );

	#ifdef REGISTER_TRACES
	fprintf(sv.f_trace_objects, "-1"); // invalid object type to determine EOF
	#endif

	log_info( "Worldmap generate done.\n" );
}



#endif


void worldmap_is_valid()
{
	int i, j;
	int* n_ents = (int*) malloc_wrapper( n_entity_types * sizeof(int) );

	for( i = 0; i < n_entity_types; i++ )
	{
		n_ents[i] = 0;
		for( j = 0; j < wm.n_entities[i]; j++ )
		{
			entity_t* ent = wm.entities[i][j];
			assert( entity_is_valid( ent, &wm.size ) );

			/*   EXTRA VALIDATION     */
			/*
			pentity_set_t* pent_set = worldmap_get_entities( &ent->r, 0xffffffff );
			assert( pentity_set_size( pent_set ) == 1 );

			elem_t * pos;
			pentity_set_for_each( pos, pent_set )
			{
				entity_t* ent2 = ((pentity_t*) pos)->ent;
				assert( ent2 == ent );
			}
			pentity_set_destroy( pent_set );

			int found = 0;
			area_node_t* an = area_node_get_node_containing_ent( wm.area_tree, ent );
			entity_set_for_each( pos, &an->ent_sets[i] )
			{
				entity_t* ent2 = ((entity_t*) pos);
				if( ent == ent2 )	found++;
			}
			assert( found == 1 );
			*/
		}
	}

	area_node_is_valid( wm.area_tree, n_ents );

	for( i = 0; i < n_entity_types; i++ )
		assert( n_ents[i] == wm.n_entities[i] );
}

void worldmap_print( int verbose )
{
	int i, j;
	level_stats_t* level_stats = malloc_wrapper( (wm.depth+1) * sizeof( level_stats_t ) );
	for( i = 0; i < wm.depth+1; i++ )
	{
		level_stats[i].n_ents   = malloc_wrapper( n_entity_types * sizeof( int ) );
		level_stats[i].min_ents = malloc_wrapper( n_entity_types * sizeof( int ) );
		level_stats[i].max_ents = malloc_wrapper( n_entity_types * sizeof( int ) );
		for( j = 0; j < n_entity_types; j++ )
		{
			level_stats[i].n_ents[j] = 0;
			level_stats[i].min_ents[j] = -1;
			level_stats[i].max_ents[j] = -1;
		}
	}

	area_node_print( wm.area_tree, 0, verbose, level_stats );

	printf( "EntityTree: for each level : n_ents(avg_ents)(min_ents : max_ents) \n" );
	for( i = 0; i < n_entity_types; i++ )
	{
		printf( "%6s: ", entity_types[i]->name );
		for( j = 0; j < wm.depth+1; j++ )
 			printf( "%3d(%3d)(%3d:%3d) ", level_stats[j].n_ents[i], level_stats[j].n_ents[i] / (1<<j),
										level_stats[j].min_ents[i], level_stats[j].max_ents[i] );
		printf( "\n" );
	}
	printf( "\n" );
}


parea_node_set_t* worldmap_get_nodes( rect_t* range )
{
	parea_node_set_t* pan_set = parea_node_set_create();

	area_node_get_nodes( wm.area_tree, range, pan_set );
	return pan_set;
}

parea_node_set_t* worldmap_get_leaves( rect_t* range )
{
	parea_node_set_t* pan_set = parea_node_set_create();
	area_node_get_leaves( wm.area_tree, range, pan_set );
	return pan_set;
}

parea_node_set_t* worldmap_get_parents( rect_t* range )
{
	parea_node_set_t* pan_set = parea_node_set_create();
	area_node_get_parents( wm.area_tree, range, pan_set );
	return pan_set;
}


void worldmap_pack( streamer_t* st, rect_t* range )
{
	int i;
	for( i = 0; i < n_entity_types; i++ )
	{
		if( i == ET_WALL )		continue;
		
		pentity_set_t* pe_set = worldmap_get_entities( range, 1 << i );

		assert( i != ET_PLAYER || pentity_set_size( pe_set ) > 0 );
		streamer_wrint( st, pentity_set_size( pe_set ) );

		elem_t* pos;
		pentity_set_for_each( pos, pe_set )
			game_entity_pack( ((pentity_t*)pos)->ent, st );

		pentity_set_destroy( pe_set );
	}
}


void worldmap_pack_fixed( streamer_t* st )
{
	int i, j;

	for( i = 0; i < n_entity_types; i++ )
	{
		if( !entity_types[i]->fixed )	continue;

		streamer_wrint( st, wm.n_entities[i] );
		for( j = 0; j < wm.n_entities[i]; j++ )
			entity_pack_fixed( wm.entities[i][j], st );
	}
}


/*       OBSOLETE            */
pentity_set_t* worldmap_get_entities( rect_t* range, int etypes )
{
	pentity_set_t* pe_set = pentity_set_create();
	area_node_get_entities( wm.area_tree, range, etypes, pe_set );
	return pe_set;
}


