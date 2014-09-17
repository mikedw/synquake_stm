#include "client.h"
#include "client_action.h"

extern "C" {
#include "../comm/comm.h"
#include "../game/game.h"

#include "cgame/worldmap.h"
}



/***************************************************************************************************
*
* Main loop
*
***************************************************************************************************/

int client_action_main( void* arg )
{
	pack_t* pck;
	Uint32 tolm = SDL_GetTicks();		// time_of_last_message
	Uint32 recv_until;

	printf("ClientActionModule started\n");

	while( cl.state == PLAYING )
	{
		recv_until = SDL_GetTicks() + cl.think_time;

		client_action_play();

		while( 1 )
		{
			pck = sock_receive( &cl.s, recv_until - SDL_GetTicks(), NULL );
			if( pck == NULL)	break;

			tolm = SDL_GetTicks();
			cl.server_addr = *pack_getaddr( pck );
			switch ( pck->type )
			{
				case SV_UPDATE:	client_action_update( pck->st ); break;

				//case MESSAGE_SC_NEW_QUEST: handle_NEW_QUEST(m); break;
				//case MESSAGE_SC_QUEST_OVER: handle_QUEST_OVER(); break;

				case SV_LEAVE:	cl.state = GONE; return 0;
				default:	printf("Received unknown message (%d) from server\n", pck->type);
			}
			pack_destroy( pck );
		}

		/* check if no message has been received for a long time */
		if( SDL_GetTicks() - tolm > cl.disconnect_timeout )
		{	cl.state = GONE;printf("Server timeout\n");	return 0;	}
	}

	pack_t* pck_r = pack_create( &cl.server_addr, CL_LEAVE );
	streamer_wrint( pck_r->st, cl.client_id );
	sock_send( &cl.s , pck_r );
	return 0;
}


int client_action_join()
{
	sock_send( &cl.s , pack_create( &cl.server_addr, CL_JOIN ) );

	pack_t* pck_r = sock_receive( &cl.s, cl.disconnect_timeout, NULL );
	if( !pck_r ){	printf("Cannot connect to the server.\n");				return 0;	}

	if( pck_r->type != SV_JOIN_OK && pck_r->type != SV_JOIN_NOK )
	{	printf("Received unknown message (%d) from server\n", pck_r->type);	return 0;	}

	if( pck_r->type == SV_JOIN_NOK )
	{	printf("Client not allowed to join.\n");							return 0;	}

	//	pck_r->type == SV_JOIN_OK
	int ent_type = streamer_rdint( pck_r->st );	assert( ent_type == ET_PLAYER );
	int ent_id   = streamer_rdint( pck_r->st );

	cl.pl = entity_create( ent_type );
	cl.pl->ent_id = ent_id;

	entity_unpack( cl.pl, pck_r->st );
	worldmap_unpack_fixed( pck_r->st );

	pack_destroy( pck_r );

	assert( entity_is_valid( cl.pl, &wm.size ) );
	cl.view_r = game_action_range( AC_VIEW, cl.pl, &wm.map_r );
	cl.client_id = ent_id;
	cl.state = PLAYING;

	printf("ClientActionModule joined server.\n");
	return 1;
}


void client_action_update( streamer_t* st )
{
	int la_sv = streamer_rdint( st );
	if( la_sv > cl.last_action_sv ) cl.last_action_sv = la_sv;

	int et = streamer_rdint( st );
	int e_id = streamer_rdint( st );
	assert( et == cl.pl->ent_type && e_id == cl.pl->ent_id );

	SDL_LockMutex(cl.game_mutex);

	entity_unpack( cl.pl, st );
	worldmap_unpack( st );

	assert( entity_is_valid( cl.pl, &wm.size ) );
	cl.view_r = game_action_range( AC_VIEW, cl.pl, &wm.map_r );

	char q_state = streamer_rdchar( st );
	if( q_state == QT_NEW )
	{
		rect_unpack( &cl.quest_r, st );
		cl.quest_active = true;
	}
	else if( q_state == QT_INACTIVE )
	{
		cl.quest_active = false;
	}

	SDL_UnlockMutex(cl.game_mutex);


	/* update average */
	Uint32 current_time = SDL_GetTicks();
	cl.last_update_interval = current_time - cl.time_of_last_update;
	if ( cl.average_update_interval < 0 )		cl.average_update_interval = cl.last_update_interval;
	else	cl.average_update_interval = cl.average_update_interval * 0.95 + (double)cl.last_update_interval * 0.05;
	cl.time_of_last_update = current_time;
	cl.updated = true;
}








void client_action_play()
{
	PlayerAI_Action action = cl.ai->takeAction();
	if ( cl.debug_AI) printf("Action: %s\n", AI_actions[(int)action]);
	if( action == NO_ACTION )	return;

	cl.last_action++;

	pack_t* pck = pack_create( &cl.server_addr, CL_ACTION );
	streamer_wrint( pck->st, cl.client_id );
	streamer_wrint( pck->st, cl.last_action );

	// no of actions
	streamer_wrchar( pck->st, cl.pl->attrs[PL_HIT] ? 4 : 3 );

	assert( sizeof(value_t) == sizeof(int));

	// set direction
	streamer_wrint( pck->st, -PL_DIR-1 );
	streamer_wrint( pck->st, cl.pl->attrs[PL_DIR] );

	// set speed
	streamer_wrint( pck->st, -PL_SPEED-1 );
	streamer_wrint( pck->st, cl.pl->attrs[PL_SPEED] );

	if( cl.pl->attrs[PL_HIT] )
	{
		streamer_wrint( pck->st, -PL_HIT-1 );
		streamer_wrint( pck->st, 0 );
	}

	// set action
	switch ( action )
	{
		case MOVE:	streamer_wrchar( pck->st, AC_MOVE+1 );break;
		case EAT:	streamer_wrchar( pck->st, AC_EAT+1 );break;
		case ATTACK:streamer_wrchar( pck->st, AC_ATTACK+1 );break;
		default:	assert(!"This should not happen.");
	};

	sock_send( &cl.s, pck );
}

