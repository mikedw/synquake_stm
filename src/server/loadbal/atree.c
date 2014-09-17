#include "../server.h"
#include "grid.h"
#include "atree.h"

extern grid_unit_t** grid;
extern int n_grid_units;
extern int m_grid_units;
extern int grid_unit_sz_x;
extern int grid_unit_sz_y;

extern int* thread_stats;

void tree_print( tree_t* at, int level )
{
	int i;

	if( !at ) return;

	for( i = 0; i < level; i++ ) printf("\t");
	printf(" (%d,%d) (%d,%d) \n", at->loc.v1.x, at->loc.v1.y, at->loc.v2.x, at->loc.v2.y );


	assert( (at->left && at->right ) || (!at->left && !at->right ) );

	tree_print( at->left, level+1 );
	tree_print( at->right, level+1 );
}


void tree_init(tree_t* at, tree_t* parent, rect_t _loc, int _split_dir, int level)
{
	assert( grid && thread_stats && at );
	assert( (_loc.v1.x <= _loc.v2.x && _loc.v1.y <= _loc.v2.y) && level >= 0 );
	assert( _split_dir == DIR_UP || _split_dir == DIR_RIGHT );

	at->loc = _loc;
	rect_init4( &at->split, -1, -1, -1, -1 );
	at->parent = parent;
	at->left = NULL;
	at->right = NULL;
	at->level = level;

	if( level )
	{
		rect_t r_left, r_right;
		rect_split( &_loc, _split_dir, &r_left, &r_right, &at->split );

		at->left  = tree_create( at, r_left,  1 - _split_dir, level-1 );
		at->right = tree_create( at, r_right, 1 - _split_dir, level-1 );

		return;
	}
}

void tree_deinit( tree_t* at )
{
	assert( at );

	if( !tree_is_leaf( at ) )
	{
		tree_destroy( at->left );
		tree_destroy( at->right );
	}
}

void tree_assign_threads(tree_t* at)
{
	int i, j;
	if( !at ) return;

	if( tree_is_leaf(at) ) return;

	if( at->split.v1.x == at->split.v2.x )
	{
		/* vertical split */
//		for( i = 0; i < at->level; i++ ) printf("\t");
//		printf("Vertical split\n");

		i = at->split.v1.x;

		int least_tid = grid_least_loaded_thread(thread_stats);

		for(j=at->split.v1.y; j<at->split.v2.y; j+=grid_unit_sz_y)
		{
			int ind_x = i / grid_unit_sz_x;
			int ind_y = j / grid_unit_sz_y;

			if(grid[ind_x-1][ind_y].tid == -1)
			{
				grid[ind_x-1][ind_y].tid = least_tid;
				thread_stats[least_tid] += grid[ind_x-1][ind_y].n_players;

//				printf("G[%d;%d] to Tid_%d\n", ind_x-1, ind_y, least_tid);
			}

			if(grid[ind_x][ind_y].tid == -1)
			{
				grid[ind_x][ind_y].tid   = least_tid;
				thread_stats[least_tid] += grid[ind_x][ind_y].n_players;

//				printf("G[%d;%d] to Tid_%d\n", ind_x, ind_y, least_tid);
			}
		}
	}
	else if( at->split.v1.y == at->split.v2.y )
	{
		/* horizontal split */
//		for( i = 0; i < at->level; i++ ) printf("\t");
//		printf("Horizontal split\n");

		j = at->split.v1.y;

		int least_tid = grid_least_loaded_thread(thread_stats);

		for(i=at->split.v1.x; i<at->split.v2.x; i+=grid_unit_sz_x)
		{
			int ind_x = i / grid_unit_sz_x;
			int ind_y = j / grid_unit_sz_y;

			if(grid[ind_x][ind_y-1].tid == -1)
			{
				grid[ind_x][ind_y-1].tid = least_tid;
				thread_stats[least_tid] += grid[ind_x][ind_y-1].n_players;

//				printf("G[%d;%d] to Tid_%d\n", ind_x, ind_y-1, least_tid);
			}

			if(grid[ind_x][ind_y].tid == -1)
			{
				grid[ind_x][ind_y].tid   = least_tid;
				thread_stats[least_tid] += grid[ind_x][ind_y].n_players;

//				printf("G[%d;%d] to Tid_%d\n", ind_x, ind_y, least_tid);
			}
		}
	}
	else assert(0);

	assert( (at->left && at->right ) || (!at->left && !at->right ) );

	tree_assign_threads( at->left );
	tree_assign_threads( at->right );

}

int local_tids[32]={0};

void tree_localize_threads(tree_t* at)
{
	int i, j, ind_x, ind_y;

	if( !at ) return;
	if( tree_is_leaf(at) ) return;

	// must be done before setting visited grids around split
	for( i = 0; i < sv.num_threads; i++ )	local_tids[i] = 0;

	for( i = at->loc.v1.x; i < at->loc.v2.x; i+= grid_unit_sz_x )
		for( j = at->loc.v1.y; j < at->loc.v2.y; j+= grid_unit_sz_y )
		{
			ind_x = i / grid_unit_sz_x;
			ind_y = j / grid_unit_sz_y;

			if(  grid[ind_x][ind_y].visited )
				local_tids[ grid[ind_x][ind_y].tid ]++;
		}



	if( at->split.v1.x == at->split.v2.x )
	{
		i = at->split.v1.x;
		for(j=at->split.v1.y; j<at->split.v2.y; j+=grid_unit_sz_y)
		{
			ind_x = i / grid_unit_sz_x;
			ind_y = j / grid_unit_sz_y;

			grid[ind_x-1][ind_y].visited = 1;
			grid[ind_x][ind_y].visited = 1;
		}
	}
	else if( at->split.v1.y == at->split.v2.y )
	{
		j = at->split.v1.y;
		for(i=at->split.v1.x; i<at->split.v2.x; i+=grid_unit_sz_x)
		{
			ind_x = i / grid_unit_sz_x;
			ind_y = j / grid_unit_sz_y;

			grid[ind_x][ind_y-1].visited = 1;
			grid[ind_x][ind_y].visited = 1;
		}
	}
	else assert(0);



	if( at->parent == NULL )
	{
		tree_localize_threads( at->left );
		tree_localize_threads( at->right );

		return;
	}




	int new_tid;
	for( new_tid = 0; new_tid < sv.num_threads; new_tid++ )
	{
		if( !local_tids[ new_tid ] )	continue;


		int n_new_pls = 0;
		for( i = at->loc.v1.x; i < at->loc.v2.x; i+= grid_unit_sz_x )
			for( j = at->loc.v1.y; j < at->loc.v2.y; j+= grid_unit_sz_y )
			{
				ind_x = i / grid_unit_sz_x;
				ind_y = j / grid_unit_sz_y;
				assert( grid[ind_x][ind_y].tid >= 0 );

				if( grid[ind_x][ind_y].tid == new_tid )		continue;
				if( grid[ind_x][ind_y].visited )		continue;
				n_new_pls += grid[ind_x][ind_y].n_players;
			}

		thread_stats[ new_tid ] += n_new_pls;
		if( grid_is_overloaded_thread( thread_stats, new_tid ) )
		{
			thread_stats[ new_tid ] -= n_new_pls;
			continue;
		}
//		fprintf(stderr, "\tthread_stats[%d] = %d + %d\n", new_tid, thread_stats[new_tid]-n_new_pls, n_new_pls);

		for( i = at->loc.v1.x; i < at->loc.v2.x; i+= grid_unit_sz_x )
			for( j = at->loc.v1.y; j < at->loc.v2.y; j+= grid_unit_sz_y )
			{
				ind_x = i / grid_unit_sz_x;
				ind_y = j / grid_unit_sz_y;
				if( new_tid == grid[ind_x][ind_y].tid )		continue;
				if( grid[ind_x][ind_y].visited )		continue;

				thread_stats[ grid[ind_x][ind_y].tid ] -= grid[ind_x][ind_y].n_players;
				grid[ind_x][ind_y].tid = new_tid;
			}


		// localization succeeded => no need to go deeper
		return;
	}

	tree_localize_threads( at->left );
	tree_localize_threads( at->right );
}


/*
void tree_localize_threads(tree_t* at)
{
	int i, j;
	if( !at ) return;

	if( tree_is_leaf(at) ) return;

	tree_localize_threads( at->left );
	tree_localize_threads( at->right );

	if( at->parent == NULL ) return;

	int ind_x = at->parent->split.v1.x / grid_unit_sz_x;
	int ind_y = at->parent->split.v1.y / grid_unit_sz_y;

	int new_tid = grid[ind_x][ind_y].tid;


	int n_new_pls = 0;
	for( i = at->loc.v1.x; i < at->loc.v2.x; i+= grid_unit_sz_x )
		for( j = at->loc.v1.y; j < at->loc.v2.y; j+= grid_unit_sz_y )
		{
			ind_x = i / grid_unit_sz_x;
			ind_y = j / grid_unit_sz_y;
			assert( grid[ind_x][ind_y].tid >= 0 );
			if( grid[ind_x][ind_y].tid != new_tid )
				n_new_pls += grid[ind_x][ind_y].n_players;
		}

	thread_stats[ new_tid ] += n_new_pls;
	if( grid_is_overloaded_thread( thread_stats, new_tid ) )
	{
		thread_stats[ new_tid ] -= n_new_pls;
		return;
	}
	fprintf(stderr, "\tthread_stats[%d] = %d + %d\n", new_tid, thread_stats[new_tid]-n_new_pls, n_new_pls);

	for( i = at->loc.v1.x; i < at->loc.v2.x; i+= grid_unit_sz_x )
		for( j = at->loc.v1.y; j < at->loc.v2.y; j+= grid_unit_sz_y )
		{
			ind_x = i / grid_unit_sz_x;
			ind_y = j / grid_unit_sz_y;
			if( new_tid == grid[ind_x][ind_y].tid )		continue;

			thread_stats[ grid[ind_x][ind_y].tid ] -= grid[ind_x][ind_y].n_players;
			grid[ind_x][ind_y].tid = new_tid;
		}

}
*/


// Determine owner thread based on conflicts with neighbor regions
void tree_balance(tree_t* at)
{
	tree_assign_threads(at);
	tree_localize_threads(at);
}

