#ifndef __SERVER_H
#define __SERVER_H

#define	    MAX_THREADS	        16

#define __SYNTHETIC__


#include "server_stats.h"


#ifndef __SYNTHETIC__
#include "../comm/comm.h"
#include "server_cl.h"
#else
#include "syn_server_cl.h"
#endif



typedef struct _server_thread_t
{
	int			tid;
	int			done;

#ifndef __SYNTHETIC__
	sock_t	s;
#endif
	thread_t*	thread;

	server_stats_t stats;
} server_thread_t;


extern server_thread_t*	svts;

extern __thread int __tid;

void server_thread_init( server_thread_t* svt, int _tid );

typedef struct
{
	int				n_clients;
	sv_client_t**	clients;
	mutex_t*		mutex;

#ifdef __SYNTHETIC__
	int wl_cycles;
	int wl_client_count;
#endif

	/* server */
	int port;						/* server's port */
	int num_threads;				/* number of threads */
	int update_interval;			/* time between two consecutive client updates */
	int stats_interval;

	int lock_type;

	/* MULTIPLE ACTIONS PER CYCLE */
	char* m_actions_file;
	int n_multiple_actions;
	int* m_actions;
	int randomized_actions;
	int* m_actions_ratios;
	int move_fail_stop;
	int straight_move;

	FILE* stats_f;
	FILE* f_players;
	unsigned long long next_stats_dump;
	int print_grid;
	int print_pls;
	int print_progress;
	
	/* execution traces */
	FILE* f_trace_players;
	FILE* f_trace_quests;
	FILE* f_trace_objects;
	FILE* f_trace_moves;
	FILE* f_trace_check;

	/* quests */
	int quest_between;				/* time between two consecutive quests */
	int quest_length;
	unsigned long long start_quest, stop_quest;
	char quest_state;
	rect_t quest_loc;
	vect_t quest_sz;

	int wl_quest_count;
	int wl_quest_spread;
	int wl_quest_duration;
	char* wl_quest_file;
	int* quest_times;
	rect_t** quest_locations;
	int quest_id;					// managed by thread 0 in server_thread_admin()
	int wl_proportional_quests;


	/* load-balance */
	char*	balance_name;
	int		balance_type;
	double overloaded_level;		/* overloaded and light server level */
	double light_level;
	int balance_interval;			/* time between two consecutive load balances */
	int last_balance;

	barrier_t barrier;

	buffer_t map_buff;
	streamer_t* map_st;

	int done;
} server_t;

extern server_t sv;



#endif
