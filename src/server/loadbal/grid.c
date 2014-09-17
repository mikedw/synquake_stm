#include "../../utils/malloc.h"
#include "grid.h"
#include "load_balancer.h"
#include "../server.h"
#include "../sgame/worldmap.h"

void grid_init(grid_unit_t** grid, int n_grid_units, int m_grid_units, int size_x, int size_y )
{
	int i, j ;

	for(i=0; i<n_grid_units; i++)
	{
		for(j=0; j<m_grid_units; j++)
		{
			vect_t gv1, gv2; 
			vect_init(&gv1, i*size_x, j*size_y);
			vect_init(&gv2, (i+1)*size_x, (j+1)*size_y);
			rect_init(&(grid[i][j].r), gv1, gv2);
			
			grid[i][j].tid = 0;
			grid[i][j].n_players = 0;
		}
	}
}

void grid_deinit(grid_unit_t** grid, int n_grid_units)
{
	int i;
	if(grid == NULL) return;
	
	for(i=0; i<n_grid_units; i++)
		if(grid[i] != NULL)   free_wrapper(grid[i]);
	free_wrapper(grid);
}


 
void grid_update(grid_unit_t** grid, int n_grid_units, int m_grid_units, int size_x, int size_y, char init_rand, int* thread_stats)
{
	int i, j, pl;

	for(i=0; i<n_grid_units; i++)
		for(j=0; j<m_grid_units; j++)
		{
			grid[i][j].n_players = 0;

			if( init_rand == 1 )
				grid[i][j].tid = (i+j)%(sv.num_threads); //rand()%(sv.num_threads);
		}

	for(i=0; i < sv.num_threads; i++ )	thread_stats[i] = 0;

	for(pl=0; pl < sv.n_clients; pl++)
	{
		rect_t rr = sv.clients[pl]->player->r;
		int mid_x = (rr.v1.x + rr.v2.x) / 2;
		int mid_y = (rr.v1.y + rr.v2.y) / 2;
		int ind_x = mid_x / size_x + ( ( (mid_x % size_x == 0) && (pl % 2) ) ? -1 : 0 );
		int ind_y = mid_y / size_y + ( ( (mid_y % size_y == 0) && (pl % 2) ) ? -1 : 0 );

		grid[ind_x][ind_y].n_players++;

		if( sv.balance_type != BALANCE_NONE_T )
			thread_stats[ grid[ind_x][ind_y].tid ]++;
		else
			thread_stats[ sv.clients[pl]->tid ]++;
	}
}


void grid_set_owner_no_tid(grid_unit_t** grid, int n_grid_units, int m_grid_units, int* thread_stats)
{
	int i,j;

	for(i=0; i<n_grid_units; i++)
		for(j=0; j<m_grid_units; j++)
		{
			grid[i][j].tid = -1;
			grid[i][j].visited = 0;
		}

	for(i=0;i<sv.num_threads;i++)
		thread_stats[i] = 0;
}


void grid_assign_players( grid_unit_t** grid, int size_x, int size_y )
{
	int pl;
	for(pl=0; pl < sv.n_clients; pl++)
	{
		rect_t rr = sv.clients[pl]->player->r;
		int mid_x = (rr.v1.x + rr.v2.x) / 2;
		int mid_y = (rr.v1.y + rr.v2.y) / 2;
		int ind_x = mid_x / size_x + ( ( (mid_x % size_x == 0) && (pl % 2) ) ? -1 : 0 );
		int ind_y = mid_y / size_y + ( ( (mid_y % size_y == 0) && (pl % 2) ) ? -1 : 0 );
		
		sv.clients[pl]->tid = grid[ind_x][ind_y].tid;
	}
}

void grid_print_players( grid_unit_t** grid, int n_gridunits, int m_gridunits, int size_x, int size_y, int cycle, int* thread_stats )
{
	int i,j;

	printf( "Grid: %dx%d  %dx%d ; PlayersPerThread [ ", n_gridunits, m_gridunits, size_x, size_y );
	for( i = 0; i < sv.num_threads; i++ )	printf("%d ", thread_stats[i] );
	printf( "]\n" );

	if( wm.depth > 8 )			printf("Cycle: %d  Players Distribution \n", cycle);
	else	printf("Cycle: %d  Players Distribution and Thread Assignment \n", cycle);
	
	for( j = m_gridunits-1; j >= 0; j-- )
	{
		for(i=0; i<n_gridunits; i++)
			printf("%4d ", grid[i][j].n_players );
		printf("\t");
		
		if( wm.depth <= 8 )
			for(i=0; i<n_gridunits; i++)
				printf("%1d ", grid[i][j].tid);
		
		printf("\n");
	}

	if( wm.depth > 8 )
	{
		printf("Cycle: %d  THREAD ASSIGNMENT \n", cycle);
		for( j = m_gridunits-1; j >= 0; j-- )
		{
			for(i=0; i<n_gridunits; i++)
				printf("%1d ", grid[i][j].tid);
			printf("\n");
		}
	}
	printf("\n");

	if( sv.print_pls )
	{
		int quest_spread = sv.wl_quest_spread ? sv.wl_quest_spread : 1;
		for(i=0; i < sv.n_clients; i++)
		{
			rect_t* rr = &sv.clients[i]->player->r;
			fprintf( sv.f_players, "%d %d %d %d %d\n", cycle, (rr->v1.x+rr->v2.x)/2, (rr->v1.y+rr->v2.y)/2,
						sv.clients[i]->tid, i % quest_spread );
		}
	}
}

int grid_border_update(grid_unit_t** grid, int n_grid_units, int m_grid_units, int size_x, int size_y, grid_border_t** borders)
{
	int i,j;
	int total=0;
	
	for(i=0; i < n_grid_units; i++)
		for(j=0; j < m_grid_units; j++)
		{
			borders[i][j].north = 0;
			borders[i][j].south = 0;
			borders[i][j].east = 0;
			borders[i][j].west = 0;
		}
	
	for(i=0; i < sv.n_clients; i++)
	{
		rect_t rr = sv.clients[i]->player->r;
		int ind_x = ((rr.v1.x+rr.v2.x) / 2) / size_x;
		int ind_y = ((rr.v1.y+rr.v2.y) / 2) / size_y;

		assert( ind_x >= 0 && ind_x < n_grid_units );
		assert( ind_y >= 0 && ind_y < m_grid_units );
		
		/* check if player overlaps any border */
		if(ind_x > 0 && rect_is_overlapping( &rr, &(grid[ind_x-1][ind_y].r) ) )
		{
			borders[ind_x][ind_y].west   += OVERLAP_WEIGHT;
			borders[ind_x-1][ind_y].east += OVERLAP_WEIGHT;
			total += OVERLAP_WEIGHT;
		}
		if(ind_x < (n_grid_units-1) && rect_is_overlapping( &rr, &(grid[ind_x+1][ind_y].r) ) )
		{
			borders[ind_x][ind_y].east   += OVERLAP_WEIGHT;
			borders[ind_x+1][ind_y].west += OVERLAP_WEIGHT;
			total += OVERLAP_WEIGHT;
		}
		if(ind_y > 0 && rect_is_overlapping( &rr, &(grid[ind_x][ind_y-1].r) ) )
		{
			borders[ind_x][ind_y].south   += OVERLAP_WEIGHT;
			borders[ind_x][ind_y-1].north += OVERLAP_WEIGHT;
			total += OVERLAP_WEIGHT;
		}
		if(ind_y < (m_grid_units-1) && rect_is_overlapping( &rr, &(grid[ind_x][ind_y+1].r) ) )
		{
			borders[ind_x][ind_y].north   += OVERLAP_WEIGHT;
			borders[ind_x][ind_y+1].south += OVERLAP_WEIGHT;
			total += OVERLAP_WEIGHT;
		}
		
		/* Check if the player's max action range crosses a border */
		#define   pl   sv.clients[i]->player
		
		int max_action_range = (entity_types[ET_PLAYER]->attr_types[PL_SPEED].max) * 
		                       (action_ranges[AC_MOVE].front);
		rect_t pos_down, pos_up, pos_left, pos_right;
		rect_init4( &pos_left,  pl->r.v1.x - max_action_range, pl->r.v1.y, 
                                        pl->r.v2.x - max_action_range, pl->r.v2.y);
		rect_init4( &pos_right, pl->r.v1.x + max_action_range, pl->r.v1.y, 
                                        pl->r.v2.x + max_action_range, pl->r.v2.y);
		rect_init4( &pos_down, pl->r.v1.x, pl->r.v1.y - max_action_range, 
                                       pl->r.v2.x, pl->r.v2.y - max_action_range);
		rect_init4( &pos_up,   pl->r.v1.x, pl->r.v1.y + max_action_range, 
                                       pl->r.v2.x, pl->r.v2.y + max_action_range);		
		
		// OBS: !!! pos_* is a real coordinate not an index in the grid 
		int ind_x_left  = ((pos_left.v1.x  + pos_left.v2.x) / 2) / size_x;
		int ind_x_right = ((pos_right.v1.x + pos_right.v2.x) / 2) / size_x;
		int ind_y_down  = ((pos_down.v1.y + pos_down.v2.y) / 2) / size_y;
		int ind_y_up    = ((pos_up.v1.y   + pos_up.v2.y) / 2) / size_y;
		
		if(ind_x > 0 && ind_x_left != ind_x)  
		{
			borders[ind_x][ind_y].west   += BORDER_CROSS_WEIGHT;
			borders[ind_x-1][ind_y].east += BORDER_CROSS_WEIGHT;
			total += BORDER_CROSS_WEIGHT;
		}
		if(ind_x < (n_grid_units-1) && ind_x_right != ind_x)  
		{
			borders[ind_x][ind_y].east   += BORDER_CROSS_WEIGHT;
			borders[ind_x+1][ind_y].west += BORDER_CROSS_WEIGHT;
			total += BORDER_CROSS_WEIGHT;
		}
		if( ind_y > 0 && ind_y_down != ind_y)  
		{
			borders[ind_x][ind_y].south   += BORDER_CROSS_WEIGHT;
			borders[ind_x][ind_y-1].north += BORDER_CROSS_WEIGHT;
			total += BORDER_CROSS_WEIGHT;
		}
		if( ind_y < (m_grid_units-1) && ind_y_up != ind_y)  
		{
			borders[ind_x][ind_y].north   += BORDER_CROSS_WEIGHT;
			borders[ind_x][ind_y+1].south += BORDER_CROSS_WEIGHT;
			total += BORDER_CROSS_WEIGHT;
		}		
	}
	
	return total;
}

int grid_is_overloaded_thread(int* t_stats, int tid)
{
	if(sv.num_threads == 1) return 0;

	return ( t_stats[tid] >= OVERLOAD_THRESHOLD );
}

int grid_is_underloaded_thread(int* t_stats, int tid)
{
	if(sv.num_threads == 1) return 0;

	return ( t_stats[tid] <= UNDERLOAD_THRESHOLD );
}

int grid_most_overloaded_thread(int* t_stats)
{
	int i, max=-1;
	for(i=0; i<sv.num_threads; i++)
		if(grid_is_overloaded_thread(t_stats, i) && (max == -1 || t_stats[max] < t_stats[i]))   
			max = i;
		
	return max;
}

int grid_load_is_balanced(int* t_stats)
{
	int most = grid_most_loaded_thread(t_stats);
	int least = grid_least_loaded_thread(t_stats);
	
	if( grid_is_overloaded_thread( t_stats, most ) ) return 0;
	if( grid_is_underloaded_thread( t_stats, least ) ) return 0;
	
	return 1;
}

int grid_most_loaded_thread(int* t_stats)
{
	int i, max=-1;
	for(i=0; i<sv.num_threads; i++)
		if(max == -1  ||  t_stats[max] < t_stats[i])   
			max = i;
		
	return max;
}

int grid_least_loaded_thread(int* t_stats)
{
	int i, min=-1;
	for(i=0; i<sv.num_threads; i++)
		if(min == -1  ||  t_stats[min] > t_stats[i])   
			min = i;
		
	return min;
}
