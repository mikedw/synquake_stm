#include "server.h"
#include "sgame/worldmap.h"

#ifndef __SYNTHETIC__


void sv_client_init( sv_client_t* cl, entity_t* _pl, char* _name, addr_t* addr )
{
	assert( cl && _name && strlen(_name) < MAX_NAME_LEN && addr && _pl );
	cl->cl_id = _pl->ent_id;
	strcpy( cl->name, _name );
	cl->addr = *addr;
	cl->tid = rand_n( sv.num_threads );
	cl->tolm = get_tm();
	cl->last_action = -1;
	cl->player = _pl;
}



entity_t* server_add_cl( char* pname, addr_t* addr )
{
	mutex_lock( sv.mutex );
	entity_t* pl;

	pl = worldmap_generate_ent( ET_PLAYER );
	if( !pl )
	{	mutex_unlock( sv.mutex ); return NULL;}

	int res = worldmap_add( pl );
	if( !res )
	{	mutex_unlock( sv.mutex ); entity_destroy( pl );	return NULL;}

	sv.clients[ pl->ent_id ] = sv_client_create( pl, pname, addr );
	sv.n_clients++;

	mutex_unlock( sv.mutex );
	return pl;
}

void server_del_cl( int cl_id, addr_t* addr )
{
	mutex_lock( sv.mutex );

	sv_client_t* cl = sv.clients[cl_id];	assert( cl && addr_equal( addr, &cl->addr ) );

	sv.n_clients--;
	sv.clients[cl->cl_id] = NULL;
	worldmap_del( cl->player );

	mutex_unlock( sv.mutex );

	printf("[I] %s left.\n", cl->name );
	entity_destroy( cl->player );
	sv_client_destroy(cl);
}

void entity_attribute_set(entity_t* pl, char a_id, value_t new_v )
{

    ANT_lock( &pl->r, wm.area_tree );
    entity_set_attr( pl, (-a_id)-1, new_v );
    ANT_unlock( &pl->r, wm.area_tree );
}

void server_act_cl( int cl_id, addr_t* addr, streamer_t* st )
{
	//mutex_lock( sv.mutex );

	sv_client_t* cl = sv.clients[cl_id];	assert( cl && addr_equal( addr, &cl->addr ) );
	entity_t* pl = cl->player;

	// get the counter value for the received action
	int la = streamer_rdint( st );			assert( la >= 0 && la == cl->last_action+1 );
	cl->last_action = la;

	// get the number of actions to perform
	char n_acts = streamer_rdchar( st );	assert( n_acts >= 0 );
	char a_i;
	for( a_i = 0; a_i < n_acts; a_i++ )
	{
		// get the id of the action to be performed
		// a_id < 0 => change the value of some attribute (pseudo-action)
		// a_id > 0 => perform an actual action (eg. eat)
		// a_id represents the id of the attribute/action incremented to avoid the value "0"
		char a_id = streamer_rdchar( st );

		assert( sizeof(value_t) == sizeof(int));
		if( a_id < 0 )	entity_attribute_set( pl, a_id, streamer_rdint( st ) );//entity_set_attr( pl, (-a_id)-1, streamer_rdchar( st ) );
		if( a_id > 0 )	game_action( a_id-1, pl );
	}

	//mutex_unlock( sv.mutex );
}

void server_update_cl( sv_client_t* cl, streamer_t* st )
{
	assert( cl );
	streamer_wrint( st, cl->last_action );
	entity_pack( cl->player, st );

	rect_t vr = game_action_range( AC_VIEW, cl->player, &wm.map_r );
	worldmap_pack( st, &vr );
}
#endif
