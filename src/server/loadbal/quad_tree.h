#ifndef QUAD_TREE_H_
#define QUAD TREE_H_

#include "../../utils/geometry.h"

typedef struct _quad_node_t
{
	vect_t pos;
	rect_t loc;
	
	struct _quad_node_t* NE;
	struct _quad_node_t* NW;
	struct _quad_node_t* SE;
	struct _quad_node_t* SW;
	struct _quad_node_t* parent;

	int nplayers;
	int tid;
	
	int* cost;
	int* visited;
}quad_node_t;

#define quad_node_create( parent, _loc, level )		generic_create3( quad_node_t, quad_node_init, parent, _loc, level )
#define quad_node_destroy( qt )				generic_destroy( qt, quad_node_deinit )

void quad_node_set_grid(grid_unit_t** _grid, int _n_grid_units, int _m_grid_units, int* t_stats);
void quad_node_init( quad_node_t* qt,  quad_node_t* parent, rect_t _loc, int level );
void quad_node_deinit( quad_node_t* qt );

#define quad_node_is_leaf( qt )				( (qt)->NW == NULL )

void quad_node_balance(quad_node_t* qt);

#endif /*QUAD_TREE_H_*/
