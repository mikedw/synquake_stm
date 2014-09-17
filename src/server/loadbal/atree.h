#ifndef A_TREE_H_
#define A_TREE_H_

#include "../../utils/geometry.h"

#include "../../game/game.h"


typedef struct _tree_node_t
{
	rect_t loc;		/* coordinates of the area */
	rect_t split;		/* segment of line along which this area node is split */
	
	struct _tree_node_t* left;
	struct _tree_node_t* right;
	struct _tree_node_t* parent;
	
	int level;
}tree_t;

#define tree_create( parent, _loc, _split_dir, level )	generic_create4( tree_t, tree_init, parent, _loc, _split_dir, level )
#define tree_destroy( at )				generic_destroy( at, tree_deinit )

void tree_init( tree_t* at,  tree_t* parent, rect_t _loc, int _split_dir, int level );
void tree_deinit( tree_t* at );

#define tree_is_leaf( at )			( (at)->left == NULL )

void tree_balance(tree_t* at);

#endif /*A_TREE_H_*/
