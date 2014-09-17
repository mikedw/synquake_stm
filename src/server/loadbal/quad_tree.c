#include "../server.h"
#include "../sgame/worldmap.h"
#include "../../game/entity.h"
#include "../../utils/malloc.h"
#include "grid.h"
#include "quad_tree.h"

extern grid_unit_t** grid;
extern int n_grid_units;
extern int m_grid_units;
extern int* thread_stats;
extern grid_border_t** borders;

void quad_node_print( quad_node_t* qt, int level )
{
	int i;

	if( !qt ) return;

	for( i = 0; i < level; i++ ) printf("\t");
	printf(" (%d,%d) (%d,%d) \n", qt->loc.v1.x, qt->loc.v1.y, qt->loc.v2.x, qt->loc.v2.y );

	
	assert( (qt->NW && qt->NE && qt->SW && qt->SE) || (!qt->NW && !qt->NE && !qt->SW && !qt->SE) );

	quad_node_print( qt->NW, level+1 );
	quad_node_print( qt->NE, level+1 );
	quad_node_print( qt->SW, level+1 );
	quad_node_print( qt->SE, level+1 );
}


void quad_node_init(quad_node_t* qt, quad_node_t* parent, rect_t _loc, int level)
{
	int i,j;
	assert(grid && thread_stats && qt && (_loc.v1.x <= _loc.v2.x && _loc.v1.y <= _loc.v2.y) && level >= 0 );


	qt->loc = _loc;
	qt->NE = NULL;  
	qt->NW = NULL;  
	qt->SE = NULL;  
	qt->SW = NULL;
	qt->parent = parent;

	if( level )
	{
		rect_t r_NE, r_NW, r_SE, r_SW;
		rect_t r_left, r_right, r_split;
		rect_split( &_loc, DIR_UP, &r_left, &r_right, &r_split );
		rect_split( &r_left,  DIR_RIGHT, &r_SW, &r_NW, &r_split );
		rect_split( &r_right, DIR_RIGHT, &r_SE, &r_NE, &r_split );
				
		qt->NE = quad_node_create( qt, r_NE, level-1 );
		qt->NW = quad_node_create( qt, r_NW, level-1 );
		qt->SE = quad_node_create( qt, r_SE, level-1 );
		qt->SW = quad_node_create( qt, r_SW, level-1 );
		return;
	}

	// is leaf

	qt->cost = (int*)calloc_wrapper(sv.num_threads, sizeof(int));  assert(qt->cost != NULL);
	qt->visited = (int*)calloc_wrapper(sv.num_threads, sizeof(int));  assert(qt->visited != NULL);

	for(i=0; i<n_grid_units; i++)
		for(j=0; j<m_grid_units; j++)
			if( vect_is_eq(&(grid[i][j].r.v1), &(_loc.v1)) && 
			    vect_is_eq(&(grid[i][j].r.v2), &(_loc.v2)) )
			{
				vect_init(&(qt->pos), (coord_t)i, (coord_t)j);
				qt->nplayers = grid[i][j].n_players;
				qt->tid      = grid[i][j].tid;
				break;
			}
}

void quad_node_deinit( quad_node_t* qt )
{
	assert( qt );
	
	if( !quad_node_is_leaf( qt ) )
	{
		quad_node_destroy( qt->NW );
		quad_node_destroy( qt->NE );
		quad_node_destroy( qt->SW );
		quad_node_destroy( qt->SE );
	}
}

//----- qt_* are helper functions for quad_node_balance ------------//
void qt_build_cost(quad_node_t* node, char direction)
{
	vect_t neighbour;
	
	vect_init(&neighbour, node->pos.x, node->pos.y);
	if(direction == 'N')  { 
		neighbour.y++;
		if ( neighbour.y == m_grid_units )             return ;  
		
		node->cost[ grid[neighbour.x][neighbour.y].tid ]  +=  borders[neighbour.x][neighbour.y].north; 
		return;
	}
	if(direction == 'S')  { 
		neighbour.y--;
		if ( neighbour.y < 0 ) return ;  
	
		node->cost[ grid[neighbour.x][neighbour.y].tid ]  +=  borders[neighbour.x][neighbour.y].south; 
		return;
	}
	if(direction == 'E')  { 
		neighbour.x++;  
		if ( neighbour.x == n_grid_units ) return ;  
	
		node->cost[ grid[neighbour.x][neighbour.y].tid ]  +=  borders[neighbour.x][neighbour.y].east; 
		return;
	}
	if(direction == 'W')  { 
		neighbour.x--;  
		if ( neighbour.x < 0 )             return ;  
	
		node->cost[ grid[neighbour.x][neighbour.y].tid ]  +=  borders[neighbour.x][neighbour.y].west; 
		return;
	}

	assert(0);
}


int qt_get_conflicts(quad_node_t* node1, quad_node_t* node2)
{
	int vertical   = node2->pos.y - node1->pos.y; // -1(n2 south of n1) +1 (n2 north of n1)
	int horizontal = node2->pos.x - node1->pos.x; // 
	
//	printf("Vertical=%d Horizontal=%d\n", vertical, horizontal);
	// neighbouring regions, no diagonals
	assert( (abs(vertical)!=abs(horizontal)) && (abs(vertical)==1 || abs(horizontal)==1) ); 
	
	if(vertical == -1)
		return borders[node1->pos.x][node1->pos.y].south;
	if(vertical == 1)
		return borders[node1->pos.x][node1->pos.y].north;
	if(horizontal == -1)
		return borders[node1->pos.x][node1->pos.y].west;
	if(horizontal == 1)
		return borders[node1->pos.x][node1->pos.y].east;

	// each pair of ifs can be merged into one with [node1->pos.x/y + hor/vert]

	assert(0);
	return 0; //mike: should never be here
}


int qt_assign_node(quad_node_t* qt)
{
	int i, max = -1, found = 1;
	for( i = 0; i < sv.num_threads; i++ )	{ qt->cost[i] = 0; qt->visited[i] = 0; }
	
	// build cost vector
	qt_build_cost(qt, 'N');
	qt_build_cost(qt, 'S');
	qt_build_cost(qt, 'E');
	qt_build_cost(qt, 'W');
	

	while (found)
	{
		found = 0;
		// estimate best thread
		for(i=0; i<sv.num_threads; i++)
		{
			if(!(qt->visited[i]) && (max == -1  ||  qt->cost[max] < qt->cost[i]))   { max = i; found = 1;}
		}

		if( found )
		{
			if(qt->cost[max] == 0) // no conflicts whatsoever
			{
				thread_stats[grid[qt->pos.x][qt->pos.y].tid] -= grid[qt->pos.x][qt->pos.y].n_players;
				grid[qt->pos.x][qt->pos.y].tid = grid_least_loaded_thread(thread_stats);
				qt->tid = grid[qt->pos.x][qt->pos.y].tid;
				thread_stats[grid[qt->pos.x][qt->pos.y].tid] += grid[qt->pos.x][qt->pos.y].n_players;

				return 0;
			}

			thread_stats[max] += grid[qt->pos.x][qt->pos.y].n_players;
			if( !grid_is_overloaded_thread(thread_stats, max) )
			{
				thread_stats[grid[qt->pos.x][qt->pos.y].tid] -= grid[qt->pos.x][qt->pos.y].n_players;
				grid[qt->pos.x][qt->pos.y].tid = max;
				qt->tid = grid[qt->pos.x][qt->pos.y].tid;
				thread_stats[grid[qt->pos.x][qt->pos.y].tid] += grid[qt->pos.x][qt->pos.y].n_players;
			
				break;
			}
			thread_stats[max] -= grid[qt->pos.x][qt->pos.y].n_players;
			qt->visited[max] = 1;
		}
	}

	return 1;
}


void qt_adjust(quad_node_t* node1, quad_node_t* node2)
{
	int i, max;
	int old_owner1 = node1->tid;
	int old_owner2 = node2->tid;
	int max1 = node1->cost[node1->tid];
	int max2 = node2->cost[node2->tid];
	int conflicts = qt_get_conflicts(node1, node2);
	
	if(conflicts == 0)  return;

	if(max1 + max2 > 2*conflicts)
	{	
		thread_stats[old_owner1] -= grid[node1->pos.x][node1->pos.y].n_players;
		thread_stats[old_owner2] -= grid[node2->pos.x][node2->pos.y].n_players;

		// assign to same thread
		// choose Max = max(node1->cost[i] + node2->cost[i]), i=1,NUM_THREADS
		max = -1;
		for(i=0; i<sv.num_threads; i++)
			if(max == -1 || (node1->cost[i]+node2->cost[i]) >= (node1->cost[max]+node2->cost[max]))
				max = i;

		node1->tid = max;  grid[node1->pos.x][node1->pos.y].tid = max;
		node2->tid = max;  grid[node2->pos.x][node2->pos.y].tid = max;

		thread_stats[max] += grid[node1->pos.x][node1->pos.y].n_players;
		thread_stats[max] += grid[node2->pos.x][node2->pos.y].n_players;

		// check if thread becomes overloaded after adjustment
      		if(grid_is_overloaded_thread(thread_stats, max))
		{
			// revert to initial assignment
			thread_stats[grid[node1->pos.x][node1->pos.y].tid] -= grid[node1->pos.x][node1->pos.y].n_players;
			thread_stats[grid[node2->pos.x][node2->pos.y].tid] -= grid[node2->pos.x][node2->pos.y].n_players;
			node1->tid  = old_owner1;
			node2->tid  = old_owner2;
			grid[node1->pos.x][node1->pos.y].tid = old_owner1;
			grid[node2->pos.x][node2->pos.y].tid = old_owner2;
			thread_stats[old_owner1] += grid[node1->pos.x][node1->pos.y].n_players;
			thread_stats[old_owner2] += grid[node2->pos.x][node2->pos.y].n_players;
		}
	}	
}


void qt_assign_threads(quad_node_t* qt)
{
	int confl1, confl2, confl3, confl4;

	assert( qt->NW );
	assert( qt->NE );
	assert( qt->SW );
	assert( qt->SE );

	if(!quad_node_is_leaf( qt->NW ))
	{
		qt_assign_threads( qt->NW );
		qt_assign_threads( qt->NE );
		qt_assign_threads( qt->SW );
		qt_assign_threads( qt->SE );
		return;
	}

	// children are leafs
	confl1 = qt_assign_node(qt->NW);
	confl2 = qt_assign_node(qt->NE);
	confl3 = qt_assign_node(qt->SW);
	confl4 = qt_assign_node(qt->SE);	

	//printf("conflNW=%d   conflNE=%d   conflSW=%d   conflSE=%d\n", confl1, confl2, confl3, confl4);
	if(confl1 && confl2) 	{

		//printf("NW->pos=(%d,%d)  NE->pos=(%d,%d)\n", qt->NW->pos.x, qt->NW->pos.y, qt->NE->pos.x, qt->NE->pos.y);
		qt_adjust(qt->NW, qt->NE);
			
	}
	if(confl1 && confl3) 	{

		//printf("NW->pos=(%d,%d)  SW->pos=(%d,%d)\n", qt->NW->pos.x, qt->NW->pos.y, qt->SW->pos.x, qt->SW->pos.y);		
		qt_adjust(qt->NW, qt->SW);
	}
	
	//OBS: no diagonal adjustments - diagonal conflicts not computed in **borders
	//if(confl1 && confl4) 	qt_adjust(qt->NW, qt->SE);
	//if(confl2 && confl3) 	qt_adjust(qt->NE, qt->SW);
	
	if(confl2 && confl4) 	{

		//printf("NE->pos=(%d,%d)  SE->pos=(%d,%d)\n", qt->NE->pos.x, qt->NE->pos.y, qt->SE->pos.x, qt->SE->pos.y);
		qt_adjust(qt->NE, qt->SE);
	}
	if(confl3 && confl4) 	{

		//printf("SW->pos=(%d,%d)  SE->pos=(%d,%d)\n", qt->SW->pos.x, qt->SW->pos.y, qt->SE->pos.x, qt->SE->pos.y);
		qt_adjust(qt->SW, qt->SE);
	}
	
	//grid_assign_players(qt->NW->loc, grid[qt->pos.x][qt->pos.y].tid);
	//grid_assign_players(qt->NE->loc, grid[qt->pos.x][qt->pos.y].tid);
	//grid_assign_players(qt->SW->loc, grid[qt->pos.x][qt->pos.y].tid);
	//grid_assign_players(qt->SE->loc, grid[qt->pos.x][qt->pos.y].tid);
}
//----------------------------------------------------------------

// Determine owner thread based on conflicts with neighbor regions
void quad_node_balance(quad_node_t* qt)
{
//	printf("\nbefore\n");
//	quad_node_print( qt, 0 );

	qt_assign_threads(qt);

//	printf("\nafter\n");
//	quad_node_print( qt, 0 );
}
