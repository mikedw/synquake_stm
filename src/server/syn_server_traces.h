#ifndef SYN_SERVER_TRACES_H_
#define SYN_SERVER_TRACES_H_


//#define REGISTER_TRACES
//#define RUN_TRACE_INITIAL
//#define RUN_TRACE_ALL_MOVES
#define RUN_QUESTS
//#define TRACE_DEBUG



#ifdef RUN_TRACE_INITIAL
#define RUN_TRACES
#undef REGISTER_TRACES
#endif

#ifdef RUN_TRACE_ALL_MOVES
#define RUN_TRACES
#undef REGISTER_TRACES

void server_load_player_moves();
void server_thread_replay_moves( server_thread_t* svt, int cycle );

#endif

#ifdef RUN_TRACES
#define RUN_QUESTS
#endif

#ifdef REGISTER_TRACES
#undef RUN_TRACES
#undef RUN_TRACE_INITIAL
#undef RUN_TRACE_ALL_MOVES
#endif



#ifdef RUN_TRACES
typedef struct
{
	int speed;
	int dir;
}pl_move_t;
#endif

#ifdef RUN_QUESTS
void server_load_quests();
#endif



void server_traces_init();
void server_traces_deinit();


void server_register_quest( int i, int j );
void server_register_player( entity_t* new_pl );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif

void server_register_object( entity_t* ent );
void server_register_player_move( entity_t* pl, int i );


#endif /* SYN_SERVER_TRACES_H_ */
