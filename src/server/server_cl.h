#ifndef SERVER_CL_H_
#define SERVER_CL_H_

#include "../general.h"
#include "../utils/streamer.h"
#include "sgame/sgame.h"



typedef struct
{
	int cl_id;
	char name[MAX_NAME_LEN];
	addr_t addr;
	int tid;					// thread id of the server handling requests/updates for this client
	unsigned long long tolm;	// time_of_last_message
	int last_action;			// the counter of the last action received from this client

	entity_t* player;
} sv_client_t;

void sv_client_init( sv_client_t* cl, entity_t* _pl, char* _name, addr_t* addr );

#define sv_client_create( _pl, _name, addr )		generic_create3( sv_client_t, sv_client_init, _pl, _name, addr )
#define sv_client_destroy( cl )						generic_free( cl )

entity_t* server_add_cl( char* pname, addr_t* addr );
void server_del_cl( int cl_id, addr_t* addr );
void server_act_cl( int cl_id, addr_t* addr, streamer_t* st );
void server_update_cl( sv_client_t* cl, streamer_t* st );

#endif /*SERVER_CL_H_*/
