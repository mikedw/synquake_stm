#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "../../utils/malloc.h"
#include "LOS_graph.h"
#include "AOI_graph.h"
#include "../sgame/worldmap.h"
#include "../server.h"
#include "grid.h"
#include "quad_tree.h"
#include "atree.h"
#include "load_balancer.h"

grid_unit_t** grid = NULL;
int n_grid_units;
int m_grid_units;
int grid_unit_sz_x;
int grid_unit_sz_y;

grid_border_t** borders = NULL;
int total_conflicts;

int* thread_stats;
quad_node_t* qtree = NULL;
tree_t* atree = NULL;

//===== Weighted union-find with path compression =====
static int _Root_(int i, int* parent) 
{
	while (i != parent[i])
	{
		parent[i] = parent[parent[i]];
		i = parent[i];
	}
	return i;
}

static int _Find_(int p, int q, int* parent)
{
	return (_Root_(p, parent) == _Root_(q, parent));
}

static void _Union_(int p, int q, int* parent, int* size)
{
	int i = _Root_(p, parent);
	int j = _Root_(q, parent);

	// Merge smaller tree into larger tree
	if (size[i] < size[j])
	{
		parent[i] = j;
		size[j] += size[i];
		return;
	}

	parent[j] = i;
	size[i] += size[j];
}

static int** a = NULL;
static int *parent, *size, *visited, *mappings, *compsize, *marked;
static int nrcomp=0, n_nodes;

static void allocate_memory(void)
{
	int i;
	if(!a)
	{
		a = (int**) malloc_wrapper( n_nodes * sizeof(int*) );     assert( a != NULL );
		for(i=0; i<n_nodes; i++) {
			a[i] = (int*) malloc_wrapper( n_nodes * sizeof(int) );     assert( a[i] != NULL );
		}
		parent = (int*) malloc_wrapper( n_nodes * sizeof(int) );         assert( parent != NULL );	
		size = (int*) malloc_wrapper( n_nodes * sizeof(int) );           assert( size != NULL );
		mappings = (int*) malloc_wrapper( n_nodes * sizeof(int) );       assert( mappings != NULL );
		visited = (int*) malloc_wrapper( n_nodes * sizeof(int) );        assert( visited != NULL );	
		marked = (int*) malloc_wrapper( n_nodes * sizeof(int) );         assert( marked != NULL );	
		compsize = (int*) malloc_wrapper( n_nodes * sizeof(int) );       assert( compsize != NULL );
	}
}

static int build_adjacency_matrix(int cycle)
{
	int summ=0, sum_marked=0;
	int g1, g2, i1, j1, i2, j2;

	int Conflict_Limit = 2 * sv.n_clients / (pow(2, wm.depth)); //sv.n_clients / (4 * sv.wl_quest_spread) / 2;

	#ifdef DEBUG_GRAPH
	if((cycle+1) % 100 == 0)
		printf("Conflict limit= %d\n", Conflict_Limit);
	#endif

	for(g1=0; g1<n_nodes; g1++) 
	{
		i1 = g1%n_grid_units; 
		j1 = g1/n_grid_units;
		for(g2=g1+1; g2<n_nodes; g2++) 
		{
			i2 = g2%n_grid_units; 
			j2 = g2/n_grid_units;
			
			if( i2 == i1 && j2 == j1+1 )
			{				
				//a[g1][g2] = borders[i1][j1].north;
				a[g1][g2] = grid[i1][j1].n_players + grid[i2][j2].n_players;
				a[g2][g1] = a[g1][g2];
			}
			else if( i2 == i1 && j2 == j1-1 )
			{				
				//a[g1][g2] = borders[i1][j1].south;
				a[g1][g2] = grid[i1][j1].n_players + grid[i2][j2].n_players;
				a[g2][g1] = a[g1][g2];
			}
			else if( i2 == i1-1 && j2 == j1 )
			{				
				//a[g1][g2] = borders[i1][j1].west;
				a[g1][g2] = grid[i1][j1].n_players + grid[i2][j2].n_players;
				a[g2][g1] = a[g1][g2];
			}
			else if( i2 == i1+1 && j2 == j1 )
			{				
				//a[g1][g2] = borders[i1][j1].east;
				a[g1][g2] = grid[i1][j1].n_players + grid[i2][j2].n_players;
				a[g2][g1] = a[g1][g2];
			}
			else
			{
				a[g1][g2] = a[g2][g1] = 0;
				continue;
			}
			
			// only make edge if enough players
			// !!!!! try changing to nplayers1+nplayers2 <= LIMIT instead of conflicts
			if(grid[i1][j1].n_players <= Conflict_Limit || grid[i2][j2].n_players <= Conflict_Limit)
			{
				a[g1][g2] = a[g2][g1] = 0;
			}

		}
		mappings[g1] = -1;
		parent[g1] = g1;
		size[g1] = 1;
		visited[g1] = 0;
		compsize[g1] = 0;

		summ = 0;
		for(g2=0; g2<n_nodes; g2++) 
			if(g2 != g1)   summ += a[g1][g2];

		if(summ == 0) 	{ marked[g1] = 0;  sum_marked++;  mappings[g1]=grid[i1][j1].tid; }
		else		{ marked[g1] = 1; }
		
		#ifdef DEBUG_GRAPH
		if((cycle+1) % 100 == 0)
			if(marked[g1]) {
				printf("%d  ", g1);
				if(g1 % 8 == 0) printf("\n");
			}
		#endif
	}
	
	#ifdef DEBUG_GRAPH
	if((cycle+1) % 100 == 0)
		printf("\n\n");
	#endif

	/* if adjacency matrix is (0n) leave assignment to static */
	if(sum_marked == n_nodes) 
	{
		#ifdef DEBUG_GRAPH
		if((cycle+1)%100 == 0)
			printf("No Components, fall back to static assignment\n");
		#endif		
		return 0;
	}
	return 1;
}

static void build_connected_components(int cycle)
{
	int u, v;

	#ifdef DEBUG_GRAPH
	if((cycle+1) % 100 == 0)
		printf("Build connected components... \n");
	#endif

	for(u=0; u<n_nodes; u++)
	{
		if(!(marked[u])) continue;

		for(v=0; v<n_nodes; v++)
		{
			if(!(marked[v])) continue;

			if( a[u][v] != 0)
				if(!_Find_(u, v, parent))
					_Union_(u, v, parent, size);
		}
	}	

	#ifdef DEBUG_GRAPH
	if((cycle+1) % 100 == 0)
	{
		int i;
		printf("\nPARENTS:\n");
	        for(i=0; i<n_nodes;i++)
		{
			if(i % n_grid_units == 0) printf("\n");
			printf("%5d[%d]", i, parent[i]);
		}
		printf("Build mappings... \n");	
	}
	#endif
}

static void build_mappings(int cycle)
{
	int i, j;
	nrcomp = 0;
	for(i=0; i<n_nodes; i++)
	{
		if(!(marked[i])) continue;
		if(visited[i] == 1) continue;

		mappings[i] = nrcomp;
		compsize[nrcomp] = grid[i%n_grid_units][i/n_grid_units].n_players;
		//compsize[nrcomp]++;	
		for(j=i+1; j<n_nodes; j++)
		{
			if(!(marked[j])) continue;			

			if(visited[j] == 0 && parent[i] == parent[j])
			{
				mappings[j] = nrcomp;
				visited[j] = 1;
				compsize[nrcomp] += grid[j%n_grid_units][j/n_grid_units].n_players;
				//compsize[nrcomp]++;
			}
		}
		nrcomp++;
	}
	//-------------------------------------------
	
	#ifdef DEBUG_GRAPH
	if((cycle+1) % 100 == 0)
	{
		printf("\n\t");
		for(i=0;i<nrcomp;i++)
		{
			printf("Compsize[%d]=%d players    ", i, compsize[i]);
			if(i % 8 == 0) printf("\n\t");
		}
		printf("\n");
	}
	#endif

	#ifdef DEBUG_GRAPH
	if((cycle+1) % 100 == 0)
	{
		printf("\nMAPPINGS:\n");
		for(j=m_grid_units-1; j>=0; j--)
		{
			for(i=0;i<n_grid_units;i++)
			{
				int i1 = j*n_grid_units + i;

				if( !(marked[i1]) ) { printf("  ---"); continue; }
				
				printf("%5d", mappings[i1]);
			}
			printf("\n");
		}
		printf("\n");
	}
	#endif
}

static int balance_static1(void);

static void balance_graph(int cycle)
{
	int i, j;

	//------------------------------------------------
 	/* do static first */	
	balance_static1();

	/* if no quests, no clustering */
	if( sv.wl_quest_count == 0 || sv.wl_quest_spread == 0 ) return;

	//--------------------------------------------------
	/* get border conflicts */
	if( !borders ) 	{
		borders = (grid_border_t**) malloc_wrapper (n_grid_units*sizeof(grid_border_t*)); assert(borders);
		for(i=0;i<n_grid_units;i++) {
			borders[i] = (grid_border_t*) malloc_wrapper (m_grid_units*sizeof(grid_border_t)); assert(borders[i]);
		}
	}
	total_conflicts = grid_border_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, borders);

	//--------------------------------------------------
	/* do graph assignment */
	n_nodes = n_grid_units*m_grid_units;
	allocate_memory();
	if( !(build_adjacency_matrix(cycle)) ) return;

	//-------------------------------------------	
	/* build connected components */
	build_connected_components(cycle);

	//-------------------------------------------
	/* build mappings */
	build_mappings(cycle);

	//----- ANALYZE COMPONENTS ------------------
	// Ncomp <= Nthreads,  each thread gets one component [some may get no players, for now]
	// Ncomp >  Nthreads,  distribute extra components in descending order of weight (using less loaded thread criterion)
	
	if(sv.num_threads <= nrcomp)
	{
		#ifdef DEBUG_GRAPH
		if((cycle+1) % 100 == 0)
			printf("Ncomp=%d  >=  %d=Nthreads\n", nrcomp, sv.num_threads);
		#endif

		for(i=0; i< sv.num_threads; i++)
		{
			thread_stats[i] = 0;
			for(j=0; j<n_nodes; j++)
			{
				if(!(marked[j])) continue;
				if(mappings[j] == i)   
					thread_stats[i] += grid[j%n_grid_units][j/n_grid_units].n_players;
			}
		}

		// distribute the extra components
		for(i=sv.num_threads; i<nrcomp; i++)
		{
			int least_loaded = grid_least_loaded_thread(thread_stats);

			//compsize[least_loaded] += compsize[i];
			for(j=0; j<n_nodes; j++)
			{
				if(!(marked[j])) continue;
				if(mappings[j] == i)   {
					mappings[j] = least_loaded;
					thread_stats[least_loaded] += grid[j%n_grid_units][j/n_grid_units].n_players;					
				}
			}
		}

		for(i=0;i<n_nodes;i++)
		{
			if(marked[i]) continue;			
			thread_stats[mappings[i]] += grid[i%n_grid_units][i/n_grid_units].n_players;
		}
	}
#ifndef NO_SPLIT_COMPONENTS
	else // nrcomp < sv.num_threads
	{
		int ii, ij, max, root_node, rni, rnj, node_dist_min, dist_min_i, dist_min_j;
		double min_dist, r_dist;

		// do
		//     select largest component (largest by number of players)
		//
		//     split in two components comp1 and comp2 as follows:
		//		select a root node
		//		assign root to comp1
		//		do
		//			select node X closest to root
		//			assign X to comp1
		//		until  ||comp1| - |comp2|| < eps, (eps = precision threshold)
		// until(nrcomp == num_threads)

		// do bisections
		while(sv.num_threads != nrcomp)
		{
			root_node = -1;
			max = -1;

			// select largest component
			for(i=0;i<nrcomp;i++)
				if( (max == -1) || (compsize[i] > compsize[max]) )
					max = i;
			assert( max != -1);
			assert( compsize[max] != 1 );
			compsize[nrcomp] = 0;

			// select root node as node with largest grid index sum (i+j)
			for(i=0;i<n_nodes;i++)
			{
				if(!(marked[i])) continue;				
				if(mappings[i] != max) continue;
				
				if( root_node == -1 ) {root_node = i; continue;}

				ii = i%n_grid_units;
				ij = i/n_grid_units;				
				rni = root_node%n_grid_units;
				rnj = root_node/n_grid_units;

				if( (ii + ij) > (rni + rnj) )	root_node = i;
				//if( ii > rni )	root_node = i;
			}				
			assert( root_node != -1 );

			#ifdef DEBUG_GRAPH
			if((cycle+1) % 100 == 0)
			{
				printf("root_node = %d (%d,%d)\n", root_node, root_node%n_grid_units, root_node/n_grid_units);
			}
			#endif

			#ifdef DEBUG_GRAPH
			if((cycle+1) % 100 == 0)
			{
				printf("Maxcompsize[%d] = %d   Before assigning root\n", max, compsize[max]);
			}
			#endif

			// start from root_node
			// assign N closest nodes to root_node, (where N ~= half the size of the maximum component)
			rni = root_node%n_grid_units;
			rnj = root_node/n_grid_units;
			mappings[root_node] = nrcomp;
			compsize[max]    -= grid[rni][rnj].n_players;
			compsize[nrcomp] += grid[rni][rnj].n_players;

			#ifdef DEBUG_GRAPH
			if((cycle+1) % 100 == 0)
			{
				printf("Maxcompsize[%d] = %d   After assigning root\n", max, compsize[max]);
			}
			#endif

			while( (abs(compsize[nrcomp] - compsize[max]) * 100 / (sv.n_clients) > 5) && 
//			       (abs(compsize[nrcomp] - compsize[max]) * 100 / (compsize[max] + compsize[nrcomp]) > 10) && 
			       (compsize[nrcomp] < compsize[max]) )
			{
				node_dist_min = -1;
				min_dist = 0;
				for(i=0; i<n_nodes; i++)
				{
					if(!(marked[i])) continue;
					if(mappings[i] != max) continue;
					if(node_dist_min == -1) {
						node_dist_min = i; 
						dist_min_i = node_dist_min%n_grid_units;
						dist_min_j = node_dist_min/n_grid_units;					
						min_dist = sqrt((rni-dist_min_i)*(rni-dist_min_i) + (rnj-dist_min_j)*(rnj-dist_min_j));
						continue;
					}	

					ii = i%n_grid_units;
					ij = i/n_grid_units;
					dist_min_i = node_dist_min%n_grid_units;
					dist_min_j = node_dist_min/n_grid_units;					
					r_dist = sqrt((rni-ii)*(rni-ii) + (rnj-ij)*(rnj-ij));
					
					if(r_dist < min_dist)
					{
						node_dist_min = i;
						min_dist = r_dist;
					}
				}
				assert(node_dist_min != -1);

				dist_min_i = node_dist_min%n_grid_units;
				dist_min_j = node_dist_min/n_grid_units;					
				mappings[node_dist_min] = nrcomp;
				compsize[max]    -= grid[dist_min_i][dist_min_j].n_players;
				compsize[nrcomp] += grid[dist_min_i][dist_min_j].n_players;
			}			

			nrcomp++;

			#ifdef DEBUG_GRAPH
			if((cycle+1) % 100 == 0)
			{
				printf("-----Split-----\n");
				for(i=0;i<nrcomp;i++)
				{
					printf("\tCompsize[%d]=%d\n", i, compsize[i]);
				}
				printf("\nMap after split:\n");
				for(j=m_grid_units-1; j>=0; j--)
				{
					for(i=0;i<n_grid_units;i++)
					{
						int i1 = j*n_grid_units + i;
	
						if( !(marked[i1]) ) { printf("  ---"); continue; }
				
						printf("%5d", mappings[i1]);
					}
					printf("\n");
				}
				printf("\n");
			}
			#endif
		}	
		
		// update thread_stats
		for(i = 0; i < sv.num_threads; i++)	thread_stats[i] = 0;

		for(i = 0; i < n_nodes; i++)
		{
			ii = i%n_grid_units;
			ij = i/n_grid_units;

			thread_stats[mappings[i]]      += grid[ii][ij].n_players;
		}
	}
#endif //NO_SPLIT_COMPONENTS

	//-------------------------------------------
	/* set grid unit mappings */
	for(i=0; i<n_nodes; i++)
	{
		if( !(marked[i]) ) continue;
		grid[i%n_grid_units][i/n_grid_units].tid = mappings[i];
	}
	
	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );
}

//---- Load balancing algorithms ------------------------------------
static void balance_init(void)
{
	int i;

	n_grid_units = 1<<((wm.depth+1)/2);
	m_grid_units = 1<<((wm.depth)/2);

	grid_unit_sz_x =  (wm.size.x % n_grid_units) == 0 ? (wm.size.x / n_grid_units) : ((wm.size.x / n_grid_units) + 1);
	grid_unit_sz_y =  (wm.size.y % m_grid_units) == 0 ? (wm.size.y / m_grid_units) : ((wm.size.y / m_grid_units) + 1);


	grid = (grid_unit_t**)malloc_wrapper(n_grid_units * sizeof(grid_unit_t*));    assert( grid );
	for(i=0; i<n_grid_units; i++)
	{
		grid[i] = (grid_unit_t*)malloc_wrapper(m_grid_units * sizeof(grid_unit_t)); assert( grid[i] );
	}

	thread_stats = (int*)malloc_wrapper(sv.num_threads * sizeof(int)); assert( thread_stats );
	grid_init(grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y );
}

static int balance_none(void)
{
	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);
	return 1;
}

static int balance_static1(void)
{
	int i, j;

	//balance
	int stripesz = (int)rint((double)n_grid_units / (double)sv.num_threads); // round to closest int
	for( i = 0; i < sv.num_threads * stripesz; i++ )
	{
		if( i >= n_grid_units )	break;

		for( j = 0; j < m_grid_units; j++ )
			grid[i][j].tid = i / stripesz;
	}
	for( i = sv.num_threads * stripesz; i <n_grid_units; i++ )
		for( j = 0; j < m_grid_units; j++ )
			grid[i][j].tid = sv.num_threads - 1;

	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);
	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );

	return 1;
}

static int balance_static2(void)
{
	int i, j;

	//balance
	int stripesz = (int)rint((double)n_grid_units / (double)sv.num_threads); // round to closest int
	for( i = 0; i < n_grid_units; i++ )
	{
		for( j = 0; j < sv.num_threads * stripesz; j++ )
		{
			if( j >= m_grid_units )	break;
			grid[i][j].tid = j / stripesz;
		}
		for( j = sv.num_threads * stripesz; j < m_grid_units; j++ )
			grid[i][j].tid = sv.num_threads - 1;
	}

	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);
	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );
	return 1;
}


//Static3 load balancing helper function 
static void static3_helper(int startTid, int endTid, int split, int x_grid_units_start, int y_grid_units_start, int x_grid_units_end, int y_grid_units_end)
{
	int i,j, midTid;
	assert( 0 <= startTid && startTid < endTid && (split == 0 || split == 1) );

	if( startTid == endTid - 1 )
	{
		//Do thread assignment and return
		for(i = x_grid_units_start; i < x_grid_units_end; i++)
			for(j = y_grid_units_start; j < y_grid_units_end; j++)
				grid[i][j].tid = startTid;
		return;
	}

	midTid = startTid + (endTid - startTid)/2;
	
	//If we split vertically
	if( split == 0 )
	{
		int x_grid_units_mid = x_grid_units_start + (x_grid_units_end - x_grid_units_start)/2;

		/* Recursive horizontal split */
		static3_helper( startTid, midTid, 1-split, x_grid_units_start, y_grid_units_start, x_grid_units_mid, y_grid_units_end) ;
		static3_helper( midTid, endTid, 1-split, x_grid_units_mid, y_grid_units_start, x_grid_units_end, y_grid_units_end );
	}
	else
	{
		//Splitting horizontally
		int y_grid_units_mid = y_grid_units_start + (y_grid_units_end - y_grid_units_start)/2;

		/* Recursive vertical split */
		static3_helper( startTid, midTid, 1-split, x_grid_units_start, y_grid_units_start, x_grid_units_end, y_grid_units_mid );
		static3_helper( midTid, endTid, 1-split, x_grid_units_start, y_grid_units_mid, x_grid_units_end, y_grid_units_end );
	}
}

static int balance_static3(void)
{
	//Call recursive helper with required parameters - First split vertically
	static3_helper(0, sv.num_threads, 0, 0, 0, n_grid_units, m_grid_units);

	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);
	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );
	return 1;
}

static int balance_lightest(void)
{
	int i, j;

	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);

	// distribute each grid unit to least loaded thread
	int most_loaded, least_loaded, best_i, best_j;
	int count = 0;


	//balance
	while(1)
	{
		most_loaded = grid_most_loaded_thread(thread_stats);
		//printf("---\n\tMost loaded=%d  npl[T%d]=%d\n", most_loaded, most_loaded, thread_stats[most_loaded]);

		if( !grid_is_overloaded_thread( thread_stats, most_loaded ) )
			break; // no one overloaded anymore

		least_loaded = grid_least_loaded_thread(thread_stats);
		//printf("\tLeast loaded=%d  npl[T%d]=%d\n", least_loaded, least_loaded, thread_stats[least_loaded]);

		if( grid_is_overloaded_thread( thread_stats, least_loaded ) ) break;
		if( most_loaded == least_loaded ) break;

		// find best region to move
		best_i = -1; best_j = -1;
		for(i=0; i<n_grid_units; i++)
		{
			for(j=0; j<m_grid_units; j++)
			{
				if(grid[i][j].tid != most_loaded)  continue;
				if(grid[i][j].n_players == 0)  continue;

				if(grid[i][j].tid == most_loaded &&
				     (best_i == -1 ||
				       (grid[i][j].n_players + thread_stats[least_loaded] < OVERLOAD_THRESHOLD && //doesn't overload least loaded thread
				        grid[i][j].n_players > grid[best_i][best_j].n_players) // is best region
				     )
				   )
				{
					best_i = i;
					best_j = j;
				}
			}
		}

		// check if any region found
		if(best_i == -1)   break;
		//printf("\tmoving region(%d;%d) players:%d\n", best_i, best_j, grid[best_i][best_j].n_players);

		// move region
		grid[best_i][best_j].tid = least_loaded;
		thread_stats[most_loaded]  -= grid[best_i][best_j].n_players;
		thread_stats[least_loaded] += grid[best_i][best_j].n_players;

		if(count++ > 1000){	printf("WARNING: Lightest count overflow\n");break;	}
	}

	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );


	return 1;
}

static int balance_spread(void)
{
	int i, j;

	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);
	for(i = 0; i < sv.num_threads; i++)	thread_stats[i] = 0;

	//balance
	for(i=0; i<n_grid_units; i++)
	{
		for(j=0; j<m_grid_units; j++)
		{
			if(grid[i][j].n_players == 0)  {
				grid[i][j].tid = (i+j) % (sv.num_threads);
				continue;
			}

			grid[i][j].tid = grid_least_loaded_thread(thread_stats);
			thread_stats[grid[i][j].tid] += grid[i][j].n_players;
		}
	}

	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );


	return 1;
}

static int balance_quad(int cycle)
{
	int i;

	if( cycle%50000 == 0)	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 1, thread_stats);
	else			grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);


	if( !borders ) 	{
		borders = (grid_border_t**) malloc_wrapper (n_grid_units*sizeof(grid_border_t*)); assert(borders);
		for(i=0;i<n_grid_units;i++) {
			borders[i] = (grid_border_t*) malloc_wrapper (m_grid_units*sizeof(grid_border_t)); assert(borders[i]);
		}
	}

	total_conflicts = grid_border_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, borders);

	//balance
	if( !qtree )
	{
		qtree = (quad_node_t*)malloc_wrapper(sizeof(quad_node_t)); if (qtree == NULL) return 0;
		quad_node_init(qtree, NULL, wm.map_r, (wm.depth+1)/2);
	}


	quad_node_balance(qtree);

	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );

	//quad_node_deinit(qtree);

	return 1;
}


static int balance_areatree(void)
{
	grid_update( grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, 0, thread_stats);

	//balance
	if( !atree )
	{
		atree = (tree_t*)malloc_wrapper(sizeof(tree_t)); if (atree == NULL) return 0;
		tree_init(atree, NULL, wm.map_r, DIR_UP, wm.depth);
	}

	grid_set_owner_no_tid( grid, n_grid_units, m_grid_units, thread_stats );
	tree_balance(atree);

	grid_assign_players( grid, grid_unit_sz_x, grid_unit_sz_y );


	return 1;
}


static int balanceLOS(void)
{
	if (!LOS_graph_build())
		return 0;

	return 1;
}

static int balanceAOI(void)
{
	if (!AOI_graph_build())
		return 0;

	return 1;
}

//-------------------------------------------------------------------

#ifdef __LOAD_BALANCING__

void loadb_init( char* type )
{
	sv.balance_name = type;
	
	if(!strcmp(type, BALANCE_NONE))			sv.balance_type = BALANCE_NONE_T;
	else if(!strcmp(type, BALANCE_STATIC1))		sv.balance_type = BALANCE_STATIC1_T;
	else if(!strcmp(type, BALANCE_STATIC2))		sv.balance_type = BALANCE_STATIC2_T;
	else if(!strcmp(type, BALANCE_STATIC3))		sv.balance_type = BALANCE_STATIC3_T;
	else if(!strcmp(type, BALANCE_LIGHTEST))	sv.balance_type = BALANCE_LIGHTEST_T;
	else if(!strcmp(type, BALANCE_SPREAD))		sv.balance_type = BALANCE_SPREAD_T;

	else if(!strcmp(type, BALANCE_QUADTREE))	sv.balance_type = BALANCE_QUADTREE_T;
	else if(!strcmp(type, BALANCE_AREATREE))	sv.balance_type = BALANCE_AREATREE_T;

	else if(!strcmp(type, BALANCE_GRAPH))		sv.balance_type = BALANCE_GRAPH_T;
	
	else if(!strcmp(type, BALANCE_AOIGRAPH))	sv.balance_type = BALANCE_AOIGRAPH_T;
	else if(!strcmp(type, BALANCE_LOSGRAPH))	sv.balance_type = BALANCE_LOSGRAPH_T;
	
	else
	{
		printf("Error: Unrecognized balance policy: %s\n", type );fflush(stdout);
		assert(0);
	}
	
	balance_init();
}

int loadb_balance( int cycle )
{
	if( sv.balance_type == BALANCE_NONE_T )			balance_none();
	else if( sv.balance_type == BALANCE_STATIC1_T )		balance_static1();
	else if( sv.balance_type == BALANCE_STATIC2_T )		balance_static2();
	else if( sv.balance_type == BALANCE_STATIC3_T )		balance_static3();
	else if( sv.balance_type == BALANCE_LIGHTEST_T )	balance_lightest();
	else if( sv.balance_type == BALANCE_SPREAD_T )		balance_spread();

	else if( sv.balance_type == BALANCE_QUADTREE_T )	balance_quad(cycle);
	else if( sv.balance_type == BALANCE_AREATREE_T )	balance_areatree();

	else if( sv.balance_type == BALANCE_GRAPH_T )		balance_graph(cycle);

	else if( sv.balance_type == BALANCE_AOIGRAPH_T )	balanceAOI();
	else if( sv.balance_type == BALANCE_LOSGRAPH_T )	balanceLOS();

	else	assert(0);

	loadb_print_results( cycle );
	return 1;
}

void loadb_print_results( int cycle )
{
	if( sv.print_grid && (((cycle+1) % sv.print_grid) == 0) )
	{
		grid_print_players(grid, n_grid_units, m_grid_units, grid_unit_sz_x, grid_unit_sz_y, cycle, thread_stats);
		worldmap_print( 0 );
	}
}

#else

void loadb_init( char* type ){}
int loadb_balance( int cycle ){ 			return 1;	}
void loadb_print_results( int cycle ){}

#endif

