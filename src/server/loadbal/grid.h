#ifndef GRID_H_
#define GRID_H_

#include "../../utils/geometry.h"

typedef struct {
	rect_t r;
	int n_players;
	int tid;

	int visited;
}grid_unit_t;

typedef struct {
	int north;
	int south;
	int east;
	int west;
} grid_border_t;

#define OVERLOAD_THRESHOLD ( ((double)1.1) * ((double)sv.n_clients) / ((double)sv.num_threads) )
#define UNDERLOAD_THRESHOLD ( ((double)0.9) * ((double)sv.n_clients) / ((double)sv.num_threads) )

#define OVERLAP_WEIGHT 2
#define BORDER_CROSS_WEIGHT 1

void grid_init(grid_unit_t** grid, int n_grid_units, int m_grid_units, int size_x, int size_y );
void grid_deinit(grid_unit_t** grid, int n_grid_units);

void grid_update(grid_unit_t** grid, int n_grid_units, int m_grid_units, int size_x, int size_y, char init_rand, int* thread_stats);
void grid_assign_players(grid_unit_t** grid, int grid_unit_size_x, int grid_unit_size_y );

void grid_set_owner_no_tid(grid_unit_t** grid, int n_grid_units, int m_grid_units, int* thread_stats);

int grid_border_update(grid_unit_t** grid, int n_grid_units, int m_grid_units, int size_x, int size_y, grid_border_t** borders);

int grid_least_loaded_thread(int* thread_stats);
int grid_most_loaded_thread(int* thread_stats);
int grid_most_overloaded_thread(int* thread_stats);
int grid_is_overloaded_thread(int* thread_stats, int tid);
int grid_is_underloaded_thread(int* thread_stats, int tid);

int grid_load_is_balanced(int* t_stats);

void grid_print_players(grid_unit_t** grid, int n_grid_units, int m_grid_units, int size_x, int size_y, int cycle, int* thread_stats );

#endif /*GRID_H_*/
