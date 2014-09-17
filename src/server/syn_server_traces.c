#include "../utils/malloc.h"
#include "server.h"
#include "sgame/worldmap.h"
#include "syn_server_traces.h"


void server_traces_init()
{

#ifdef REGISTER_TRACES
	if(sv.num_threads != 1)
	{
		printf("Cannot print trace in multithread version !\n"); exit(1);
	}

	sv.f_trace_players = fopen("Trace_players.conf", "wt"); assert(sv.f_trace_players != NULL);
	sv.f_trace_objects = fopen("Trace_objects.conf", "wt"); assert(sv.f_trace_objects != NULL);
	sv.f_trace_quests = fopen( sv.wl_quest_file, "wt"); assert(sv.f_trace_quests != NULL);
	sv.f_trace_moves = fopen("Trace_moves.conf", "wt"); assert(sv.f_trace_moves != NULL);
#endif

#ifdef RUN_QUESTS
	sv.f_trace_quests = fopen( sv.wl_quest_file, "rt"); assert(sv.f_trace_quests != NULL);
#endif

#ifdef RUN_TRACES
	sv.f_trace_players = fopen("Trace_players.conf", "rt"); assert(sv.f_trace_players != NULL);
	sv.f_trace_objects = fopen("Trace_objects.conf", "rt"); assert(sv.f_trace_objects != NULL);

#ifdef RUN_TRACE_ALL_MOVES
	sv.f_trace_moves = fopen("Trace_moves.conf", "rt"); assert(sv.f_trace_moves != NULL);
	#ifdef TRACE_DEBUG
	sv.f_trace_check = fopen("Trace_check.conf", "wt"); assert(sv.f_trace_check != NULL);
	#endif
#endif

#endif

}

void server_traces_deinit()
{
#ifdef REGISTER_TRACES
	fclose(sv.f_trace_players);
	fclose(sv.f_trace_objects);
	fclose(sv.f_trace_quests);
	fclose(sv.f_trace_moves);
#endif

#ifdef RUN_QUESTS
	fclose(sv.f_trace_quests);
#endif

#ifdef RUN_TRACES
	fclose(sv.f_trace_players);
	fclose(sv.f_trace_objects);

#ifdef RUN_TRACE_ALL_MOVES

	fclose(sv.f_trace_moves);
	#ifdef TRACE_DEBUG
	fclose(sv.f_trace_check);
	#endif

	for( i = 0; i < wl_cycles; i++)
		free_wrapper(recorded_moves[i]);
	free_wrapper(recorded_moves);
#endif

#endif
}


#ifdef REGISTER_TRACES


void server_register_quest( int i, int j )
{
	fprintf(sv.f_trace_quests, "%d %d %hd %hd\n", i, j,
				sv.quest_locations[i][j].v1.x, sv.quest_locations[i][j].v1.y);
}


void server_register_player( entity_t* new_pl )
{
	// print traces of generated players and attributes
	short v1x = new_pl->r.v1.x;
	short v1y = new_pl->r.v1.y;
	short v2x = new_pl->r.v2.x;
	short v2y = new_pl->r.v2.y;
	fprintf(sv.f_trace_players, "%d %d %d %d \n", v1x, v1y, v2x, v2y);

	int j;
	entity_type_t* et = entity_types[ new_pl->ent_type ];
	assert( et->n_attr > 0 );
	for (j = 0; j < et->n_attr; ++j)
	{
		value_t atr = new_pl->attrs[j];
		fprintf(sv.f_trace_players, "%d %d\n", j, atr);
	}
}

void server_register_object( entity_t* ent )
{
	// print traces of generated objects and attributes
	if(ent->ent_type == ET_PLAYER)	return;

	short v1x = ent->r.v1.x;
	short v1y = ent->r.v1.y;
	short v2x = ent->r.v2.x;
	short v2y = ent->r.v2.y;
	fprintf(sv.f_trace_objects, "%d\n", ent->ent_type);
	fprintf(sv.f_trace_objects, "%d %d %d %d\n", v1x, v1y, v2x, v2y);

	int j;
	entity_type_t* et = entity_types[ ent->ent_type];	assert( et->n_attr > 0 );

	if(ent->ent_type == ET_APPLE)
		for (j = 0; j < et->n_attr; ++j)
		{
			value_t atr = ent->attrs[j];
			fprintf(sv.f_trace_objects, "%d %d\n", j, atr);
		}
	if(ent->ent_type == ET_WALL)
	{
		short sx = ent->size.x;
		short sy = ent->size.y;
		fprintf(sv.f_trace_objects, "0 %hd\n", sx);
		fprintf(sv.f_trace_objects, "1 %hd\n", sy);
	}

}

void server_register_player_move( entity_t* pl, int i )
{
	assert( sv.num_threads == 1 );
	fprintf(sv.f_trace_moves, "%d %d %d\n", i, (value_t)pl->attrs[PL_SPEED], (value_t)pl->attrs[PL_DIR]);
}


#else		//  REGISTER_TRACES

void server_register_quest( int i, int j ){}
void server_register_player( entity_t* new_pl ){}
void server_register_object( entity_t* ent ){}
void server_register_player_move( entity_t* pl, int i ){}

#endif		//  REGISTER_TRACES





#ifdef RUN_TRACE_ALL_MOVES

pl_move_t** recorded_moves;


void server_load_player_moves()
{
	int i;
	int pno, pspeed, pdir, pcycle;

	recorded_moves = (pl_move_t**)malloc_wrapper(sv.wl_cycles*sizeof(pl_move_t*)); assert(recorded_moves != NULL);
	for(i=0; i<wl_cycles; i++)
	{
		recorded_moves[i] = (pl_move_t*)malloc_wrapper(sv.wl_client_count*sizeof(pl_move_t)); assert(recorded_moves[i] != NULL);
	}

	for(pcycle=0; pcycle<sv.wl_cycles; pcycle++)
	{
		for(i=0;i<sv.wl_client_count;i++)
		{
			fscanf(sv.f_trace_moves, "%d %d %d", &pno, &pspeed, &pdir);	assert( pno == i );

			recorded_moves[pcycle][pno].speed = pspeed;
			recorded_moves[pcycle][pno].dir = pdir;

			#ifdef TRACE_DEBUG
			fprintf(sv.f_trace_check, "%d %d %d\n", pno, pspeed, pdir);
			#endif
		}
	}
	//printf("Done with trace debug...\n");
}


void server_thread_replay_moves( server_thread_t* svt, int cycle )
{
	int i;
	entity_t* pl;

	for( i= 0; i < sv.n_clients; i++ )
	{
		if( sv.clients[i]->tid != svt->tid )	continue;

		pl = sv.clients[i]->player;
		pl->attrs[PL_SPEED] = recorded_moves[cycle][i].speed;
		pl->attrs[PL_DIR] = recorded_moves[cycle][i].dir;
	}

	return;
}


#endif		// RUN_TRACE_ALL_MOVES



#ifdef RUN_QUESTS

void server_load_quests()
{
	int i, j, qi, qj;
	int x, y;
	float fx, fy;
	int rez;
	
	for( i = 0; i < sv.wl_quest_count; i++ )
	{
		for( j=0;j< sv.wl_quest_spread;j++)
		{
			if( sv.wl_proportional_quests )
			{
				rez = fscanf(sv.f_trace_quests, "%d %d %f %f", &qi, &qj, &fx, &fy );
				assert( rez == 4 && qi == i && qj == j && fx >= 0 && fy >= 0 && fx <= 1 && fy <= 1 );
				sv.quest_locations[i][j].v1.x = fx * wm.size.x;
				sv.quest_locations[i][j].v1.y = fy * wm.size.y;
			}
			else
			{
				rez = fscanf(sv.f_trace_quests, "%d %d %d %d", &qi, &qj, &x, &y );
				assert( rez == 4 && qi == i && qj == j );
				sv.quest_locations[i][j].v1.x = x;
				sv.quest_locations[i][j].v1.y = y;
			}
			sv.quest_locations[i][j].v2.x = sv.quest_locations[i][j].v1.x;
			sv.quest_locations[i][j].v2.y = sv.quest_locations[i][j].v1.y;

			#ifdef TRACE_DEBUG
			printf( "Quest[%d][%d]  Time:%d  Location(%hd;%hd)\n", i, j, sv.quest_times[i],
						sv.quest_locations[i][j].v1.x, sv.quest_locations[i][j].v1.y);
			#endif
		}
	}
}

#endif		// RUN_TRACES




