#include "../utils/malloc.h"
#include "server.h"
#include "sgame/worldmap.h"
#include "sgame/sgame.h"

#ifdef __SYNTHETIC__

#ifdef INTEL_TM
/* Needs to be wrapping to support TM */
static char * stm_strcpy( char *strDest, const char *strSrc )
{
	return strcpy(strDest, strSrc);
}

static size_t stm_strlen(char* src)
{
	return strlen(src);
}
#endif

void sv_client_init( sv_client_t* cl, entity_t* _pl, char* _name )
{
#ifdef INTEL_TM
	assert( cl && _name && stm_strlen(_name) < MAX_NAME_LEN && _pl );
#else
	assert( cl && _name && strlen(_name) < MAX_NAME_LEN && _pl );
#endif

	cl->cl_id = _pl->ent_id;

#ifdef INTEL_TM
	stm_strcpy( cl->name, _name );
#else
	strcpy( cl->name, _name );
#endif

	int aux1 = (cl->cl_id % (sv.num_threads * sv.num_threads)) / sv.num_threads;
	int aux2 = ((cl->cl_id % sv.num_threads) + aux1) % sv.num_threads;
	cl->tid = aux2;//cl->cl_id % sv.num_threads ;//0;//rand_n( sv.num_threads );

#ifdef INTEL_TM
	//mike: need tm_safe implementation of this, it does file I/O, not sure what to do
	cl->tolm = get_tm();
#else
	cl->tolm = get_tm();
#endif

	cl->last_action = -1;
	cl->player = _pl;

	assert( sv.n_multiple_actions );

#ifdef INTEL_TM
	//mike: need tm_safe implementation of calloc_wrapper
	cl->m_actions = (m_action_t*) calloc_wrapper ( (size_t)sv.n_multiple_actions, sizeof(m_action_t) );
	assert( cl->m_actions );
	cl->compound_r = (rect_t*) calloc_wrapper( (size_t)sv.n_multiple_actions, sizeof(rect_t) );
	assert( cl->compound_r );
#else
	cl->m_actions = (m_action_t*) calloc_wrapper ( (size_t)sv.n_multiple_actions, sizeof(m_action_t) );
	assert( cl->m_actions );
	cl->compound_r = (rect_t*) calloc_wrapper( (size_t)sv.n_multiple_actions, sizeof(rect_t) );
	assert( cl->compound_r );
#endif
}


entity_t* server_add_cl( char* pname )
{
#if 0 //#ifdef INTEL_TM
	__transaction [[TRANSATION_TYPE]] {
#else
	mutex_lock( sv.mutex );
#endif
		#ifndef RUN_TRACES
			rect_t where = wm.map_r;

			#ifdef GENERATE_NEAR_QUESTS
			if( sv.wl_quest_count )
			{
				double aux = sqrt( sv.wl_quest_spread );
				double fx[ N_DIRS ] = { wm.size.y / aux / 2, wm.size.x / aux / 2,
										wm.size.y / aux / 2, wm.size.x / aux / 2 };
				rect_t qr = sv.quest_locations[ 0 ][ sv.n_clients % sv.wl_quest_spread ];
				rect_expand( &qr, fx, &where );
				rect_crop( &where, &wm.map_r, &where );
			}
			#endif

			entity_t *pl = worldmap_generate_ent( ET_PLAYER, &where );
		#else
			entity_t *pl = worldmap_generate_ent_from_trace( ET_PLAYER );
		#endif

#if 0 //#ifdef INTEL_TM

		if (pl)
		{
			int res = worldmap_add( pl );
			if (res)
			{
				sv.clients[ pl->ent_id ] = sv_client_create( pl, pname );
				sv.n_clients++;
			}
			else
			{
				entity_destroy( pl );
				pl = NULL;
			}
		}
	}

#else

		if( !pl )
			{ mutex_unlock( sv.mutex ); return NULL;}

			int res = worldmap_add( pl );
			if( !res )
			{ mutex_unlock( sv.mutex ); entity_destroy( pl ); return NULL;}

			sv.clients[ pl->ent_id ] = sv_client_create( pl, pname );
			sv.n_clients++;

			mutex_unlock( sv.mutex );
#endif

	return pl;
}

void server_act_cl_synthetic( int cl_id )
{
	sv_client_t* cl = sv.clients[cl_id];	assert(cl);
	entity_t* pl = cl->player;		assert(pl);

	game_multiple_action( cl );
}


#endif
