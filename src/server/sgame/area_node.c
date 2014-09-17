#include "../../utils/malloc.h"
#include "area_node.h"

#include "worldmap.h"


char lock_type_names[N_LOCK_TYPES][16] = { LOCK_TREE_S, LOCK_LEAVES_S };


void area_node_init( area_node_t* an, rect_t _loc, int _split_dir, int level )
{
	int i;
	assert( an && _loc.v1.x <= _loc.v2.x && _loc.v1.y <= _loc.v2.y && level >= 0 );
	assert( _split_dir == DIR_UP || _split_dir == DIR_RIGHT );

	an->loc = _loc;
	rect_init4( &an->split, -1, -1, -1, -1 );
	an->left = NULL;an->right = NULL;

	an->ent_sets = malloc_wrapper( n_entity_types * sizeof( entity_set_t ) );
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

	an->im_mutex = mutex_create();
	an->ex_mutex = mutex_create();
	an->cond = cond_create();

	an->numEx = 0;
	an->excl_waiting = 0;
	
	an->n_takes = 0;
	an->lock_wait = 0;
}

void area_node_deinit( area_node_t* an )
{
	int i = 0;
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


area_node_t* area_node_get_node_containing_ent( area_node_t* an, entity_t* ent )
{
	if( area_node_is_leaf( an ) || rect_is_overlapping( &an->split, &ent->r ) )
		return an;
	
	if( rect_cmp( &ent->r, &an->split ) < 0 )
		return area_node_get_node_containing_ent( an->left, ent );
	else
		return area_node_get_node_containing_ent( an->right, ent );
}

area_node_t* area_node_get_node_containing_rect( area_node_t* an, rect_t* r )
{
	if( area_node_is_leaf( an ) || rect_is_overlapping( &an->split, r ) )
		return an;
	if( rect_cmp( r, &an->split ) < 0 )
		return area_node_get_node_containing_rect( an->left, r );
	else
		return area_node_get_node_containing_rect( an->right, r );
}

/* add "ent" into "an" */
/* assumes that "ent" doesn't overlap with any existing entities in "an" */
void area_node_add( area_node_t* an, entity_t* ent )
{
	entity_set_add( &an->ent_sets[ent->ent_type], ent );
}

/* delete "ent" from "an" */
/* assumes that "ent" is placed correctly in "an" */
void area_node_del( area_node_t* an, entity_t* ent )
{
	entity_set_del( &an->ent_sets[ent->ent_type], ent );
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



/* recursively collects into "pan_set" nodes */
/* that have some overlap with "range" */
void area_node_get_nodes( area_node_t* an, rect_t* range, parea_node_set_t* pan_set )
{
	if( !rect_is_overlapping( range, &an->loc ) )	return;

	parea_node_set_add( pan_set, parea_node_create( an ) );

	if( !area_node_is_leaf( an ) )
	{
		area_node_get_nodes( an->left, range, pan_set );
		area_node_get_nodes( an->right, range, pan_set );
	}
}

/* recursively collects into "pan_set" leaves */
/* that have some overlap with "range" */
void area_node_get_leaves( area_node_t* an, rect_t* range, parea_node_set_t* pan_set )
{
	if( !rect_is_overlapping( range, &an->loc ) )	return;

	if( !area_node_is_leaf( an ) )
	{
		area_node_get_leaves( an->left, range, pan_set );
		area_node_get_leaves( an->right, range, pan_set );
		return;
	}
	parea_node_set_add( pan_set, parea_node_create( an ) );
}

/* recursively collects into "pan_set" parents */
/* that have some overlap with "range" */
void area_node_get_parents( area_node_t* an, rect_t* range, parea_node_set_t* pan_set )
{
	if( area_node_is_leaf( an ) || !rect_is_overlapping( range, &an->loc ) )	return;
	parea_node_set_add( pan_set, parea_node_create( an ) );
	
	area_node_get_parents( an->left, range, pan_set );
	area_node_get_parents( an->right, range, pan_set );
}




void area_node_is_valid( area_node_t* an, int* n_ents )
{
	assert( an && an->loc.v1.x <= an->loc.v2.x && an->loc.v1.y <= an->loc.v2.y );

	elem_t * pos;

	int i;
	for( i = 0; i < n_entity_types; i++ )
	{
		entity_set_for_each( pos, &an->ent_sets[i] )
		{
			entity_t* ent;	ent = (entity_t*) pos;

			assert( ent->ent_type == i && ent == wm.entities[i][ent->ent_id] );
			assert( rect_is_contained( &ent->r, &an->loc ) );
			assert( area_node_is_leaf( an ) || rect_is_overlapping( &an->split, &ent->r ) );
			assert( entity_is_valid( ent, &wm.size ) );

			n_ents[i]++;
		}
	}

	if( !area_node_is_leaf( an ) )
	{
		area_node_is_valid( an->left, n_ents );
		area_node_is_valid( an->right, n_ents );
	}
}




//----- Sync code --------------------------

void incl_lock( area_node_t* an )
{
	mutex_lock ( an->ex_mutex );
	while( an->numEx > 0 || an->excl_waiting != 0 )
	{
		// Wait until numEx <= 0;
		cond_wait( an->cond, an->ex_mutex);
	}

	an->numEx--;
	mutex_unlock ( an->ex_mutex );
}

void incl_unlock( area_node_t* an )
{
	// Unlock it up
	mutex_lock ( an->ex_mutex );
	an->numEx++;
	if( an->numEx == 0 )
	{
		cond_broadcast( an->cond );
	}
	mutex_unlock ( an->ex_mutex );
}

extern __thread int __tid;

void excl_lock( area_node_t* an )
{
	int marked = 0;

	mutex_lock( an->ex_mutex );
	while( an->numEx != 0 )
	{
		if( marked == 0 ){
			an->excl_waiting++;
			marked++;
		}
		cond_wait( an->cond, an->ex_mutex );
	}
	an->numEx++;
	if( marked )	an->excl_waiting--;
	
	mutex_unlock( an->ex_mutex );
}

void excl_unlock( area_node_t* an )
{
	mutex_lock( an->ex_mutex );
	an->numEx--;
	if ( an->numEx == 0 )
	{
		cond_broadcast( an->cond );
	}
	mutex_unlock( an->ex_mutex );
}



void area_node_lock_tree( area_node_t* an, rect_t* ranges, int n_ranges )
{
	if( area_node_is_leaf( an ) || 
		rect_is_overlapping_any( &an->split, ranges, n_ranges ) )
	{
		excl_lock( an );
		return;
	}

	incl_lock( an );

	if( rect_cmp( &ranges[0], &an->split ) < 0 )
		area_node_lock_tree( an->left, ranges, n_ranges );
	else
		area_node_lock_tree( an->right, ranges, n_ranges );
}

void area_node_unlock_tree( area_node_t* an, rect_t* ranges, int n_ranges )
{
	if ( area_node_is_leaf( an ) || 
		rect_is_overlapping_any( &an->split, ranges, n_ranges ) )
	{
		excl_unlock( an );
		return;
	}

	if( rect_cmp( &ranges[0], &an->split ) < 0 )
		area_node_unlock_tree( an->left, ranges, n_ranges );
	else
		area_node_unlock_tree( an->right, ranges, n_ranges );

	incl_unlock( an );
}



void area_node_lock_leaves( area_node_t* an, rect_t* ranges, int n_ranges )
{
	if( !rect_is_overlapping_any( &an->loc, ranges, n_ranges ) )	return;

	if( area_node_is_leaf( an ) )
	{
		mutex_lock( an->ex_mutex );
		return;
	}

	area_node_lock_leaves( an->left, ranges, n_ranges );
	area_node_lock_leaves( an->right, ranges, n_ranges );
}

void area_node_unlock_leaves( area_node_t* an, rect_t* ranges, int n_ranges )
{
	if( !rect_is_overlapping_any( &an->loc, ranges, n_ranges ) )	return;

	if( area_node_is_leaf( an ) )
	{
		mutex_unlock( an->ex_mutex );
		return;
	}

	area_node_unlock_leaves( an->left, ranges, n_ranges );
	area_node_unlock_leaves( an->right, ranges, n_ranges );
}


//----- End of Sync code --------------------------



void area_node_print( area_node_t* an, int level, int verbose, level_stats_t* level_stats )
{
	int i;

	if( verbose )
	{
		for( i = 0; i < level; i++ )	printf("\t");
		printf( "%d - Loc (%d,%d) (%d,%d) - ", level, an->loc.v1.x, an->loc.v1.y, an->loc.v2.x, an->loc.v2.y );
		for( i = 0; i < n_entity_types; i++ )	
			printf( "%d ", entity_set_size( &an->ent_sets[i] ) );
		printf( "\n" );
	}

	int set_size;
	for( i = 0; i < n_entity_types; i++ )
	{
		set_size = entity_set_size( &an->ent_sets[i] );
		level_stats[level].n_ents[i] += set_size;
		if( level_stats[level].min_ents[i] == -1 || set_size < level_stats[level].min_ents[i] )
			level_stats[level].min_ents[i] = set_size;
		if( level_stats[level].max_ents[i] == -1 || set_size > level_stats[level].max_ents[i] )
			level_stats[level].max_ents[i] = set_size;
	}

    if( an->left )
    {
        area_node_print( an->left, level+1, verbose, level_stats );
        area_node_print( an->right, level+1, verbose, level_stats );
    }
}




/*          OBSOLETE            */
/* recursively collects into "pe_set" entities located within the */
/* area of "an" that have some overlap with range */
void area_node_get_entities( area_node_t* an, rect_t* range, int etypes, pentity_set_t* pe_set )
{
	int i;
	elem_t* pos;

	if( !rect_is_overlapping( range, &an->loc ) )		return;

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



