#ifndef ACTION_H_
#define ACTION_H_

#include "../utils/geometry.h"
#include "../utils/conf.h"

#define MAX_ACTIONS		32

// multiple actions per frame

#define N_ACTIONS_ATTRIBUTES 3
#define M_ACT_ID 0
#define M_ACT_DIR 1
#define M_ACT_SPD  2

typedef struct
{
	coord_t front, behind;		// defines the area around the player
	coord_t left, right;		// that is going to be affected by this action
} action_range_t;

extern int n_actions;
extern action_range_t* action_ranges;
extern char** action_names;
extern int max_action_id;

void actions_init( conf_t* c );

#endif /*ACTION_H_*/
