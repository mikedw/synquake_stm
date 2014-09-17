#include "action.h"
#include "../utils/malloc.h"

int n_actions = 0;
action_range_t* action_ranges = NULL;
char** action_names = NULL;
int max_action_id = 0;

void actions_init( conf_t* c )
{
	char nm[MAX_FILE_READ_BUFFER];

	n_actions = conf_get_int( c, "n_actions" );
	assert( n_actions > 0 && n_actions <= MAX_ACTIONS );
	action_ranges = (action_range_t *) malloc_wrapper( n_actions * sizeof( action_range_t ) );
	action_names = (char**) malloc_wrapper( n_actions * sizeof(char*) );

	int i;
	for( i = 0; i < n_actions; i++ )
	{
		action_names[i] = strdup( conf_get_string( c, spf1( nm, "action[%d].name", i ) ) );
		action_ranges[i].front  = conf_get_int( c, spf1( nm, "action_range[%d].front", i ) );
		action_ranges[i].behind = conf_get_int( c, spf1( nm, "action_range[%d].behind", i ) );
		action_ranges[i].left   = conf_get_int( c, spf1( nm, "action_range[%d].left", i ) );
		action_ranges[i].right  = conf_get_int( c, spf1( nm, "action_range[%d].right", i ) );
		assert( action_ranges[i].front >=0 && action_ranges[i].behind >=0 &&
				action_ranges[i].left >=0 && action_ranges[i].right >= 0 );

		if( action_ranges[i].front > action_ranges[max_action_id].front )	max_action_id = i;
	}
}

