#ifndef SYN_SERVER_CL_H_
#define SYN_SERVER_CL_H_

#include "../general.h"
#include "../utils/streamer.h"
#include "../game/entity.h"
#include "../game/action.h"


typedef value_t m_action_t[N_ACTIONS_ATTRIBUTES];

typedef struct
{
	int cl_id;
	char name[MAX_NAME_LEN];
	int tid;					// thread id of the server handling requests/updates for this client
	unsigned long long tolm;	// time_of_last_message
	int last_action;			// the counter of the last action received from this client
	int last_dist;

	entity_t* player;
	
	m_action_t* m_actions;
	rect_t*     compound_r;
} sv_client_t;

#ifdef INTEL_TM
//mike: tm_safe strcpy
[[transaction_pure]]
char * stm_strcpy( char *strDest, const char *strSrc );

[[transaction_pure]]
size_t stm_strlen(char* src);

[[transaction_callable]]
#endif
void sv_client_init( sv_client_t* cl, entity_t* _pl, char* _name );

#define sv_client_create( _pl, _name )		generic_create2( sv_client_t, sv_client_init, _pl, _name )

entity_t* server_add_cl( char* pname );
void server_act_cl_synthetic( int cl_id );

#endif /*SYN_SERVER_CL_H_*/
