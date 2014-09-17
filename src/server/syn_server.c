#include <stdio.h>
#include <stdlib.h>

#include "../utils/malloc.h"
#include "server.h"
#include "sgame/worldmap.h"
#include "loadbal/load_balancer.h"

#ifdef __SYNTHETIC__

#include "syn_server_traces.h"

server_t			sv = {0};
server_thread_t*	svts;

unsigned int rand_seed = 0;

__thread int __tid;

static value_t server_thread_generate_quest_move( rect_t* r, int pl_id )
{
	value_t pl_dir = attribute_type_rand( &entity_types[ET_PLAYER]->attr_types[PL_DIR] );
	// guide player to quest
	rect_t* q_rect = &sv.quest_locations[sv.quest_id][pl_id % sv.wl_quest_spread];
	int direction = rect_quadrant( r, q_rect );
	switch( direction )
	{
		// if overlapping, player is on quest => do random move
		case 0:		break;
		// NW of quest, move down or right		// see constants DIR_* in geometry.h
		case 1:	pl_dir = my_rand() % 2 + 1;		break;
		// NE of quest, move down or left
		case 2:	pl_dir = my_rand() % 2 + 2;		break;
		// SW of quest, move up or right
		case 3:	pl_dir = my_rand() % 2;			break;
		// SE of quest, move up or left
		case 4:	pl_dir = (my_rand() % 2) * 3;	break;

		// over quest,  move down /or left
		case 5:	pl_dir = DIR_DOWN; /*my_rand() % 2 + 2;*/		break;
		// under quest, move up /or right
		case 6:	pl_dir = DIR_UP; /*my_rand() % 2;*/				break;
		// right of quest, move left /or up	
		case 7:	pl_dir = DIR_LEFT; /*(my_rand() % 2) * 3;*/		break;
		// left of quest, move right /or down
		case 8:	pl_dir = DIR_RIGHT; /*my_rand() % 2 + 1;*/		break;
		default:
			fprintf(stderr, "Directioning player failed\n");
			abort();
	}
	return pl_dir;
}

static void server_thread_generate_moves( server_thread_t* svt, int cycle )
{
	int i, j;
	sv_client_t* cl;

	int is_quest = (sv.wl_quest_count &&
				cycle >= sv.quest_times[sv.quest_id] &&
				cycle  < sv.quest_times[sv.quest_id] + sv.wl_quest_duration
				);

	
	for( i = 0; i < sv.n_clients; i++ )
	{
		cl = sv.clients[i];
		if( cl->tid != svt->tid )	continue;

		entity_t* pl = cl->player;
		rect_t pl_r;
		rect_init4( &pl_r, pl->r.v1.x, pl->r.v1.y, pl->r.v2.x, pl->r.v2.y );
		value_t first_pl_dir = -1;
		for( j = 0; j < sv.n_multiple_actions; j++ )
		{
			if( !sv.randomized_actions )	cl->m_actions[j][M_ACT_ID] = sv.m_actions[j];
			else
			{
				int k, aux = rand_n( 100 );
				for( k = 0; k < n_actions; k++ )
					if( aux < sv.m_actions_ratios[k] )	break;
				assert( k < n_actions );
				cl->m_actions[j][M_ACT_ID] = k;
			}
			
			if( cl->m_actions[j][M_ACT_ID] == AC_MOVE )
			{
				cl->m_actions[j][M_ACT_SPD] = attribute_type_rand( &entity_types[ET_PLAYER]->attr_types[PL_SPEED] );
				cl->m_actions[j][M_ACT_DIR] = attribute_type_rand( &entity_types[ET_PLAYER]->attr_types[PL_DIR] );

				int act_index = cycle * sv.n_multiple_actions + j;
				if( is_quest && ( (act_index % 2) == 0 ) )
				{
					cl->m_actions[j][M_ACT_SPD] = cl->m_actions[j][M_ACT_SPD] / 4;
					if( entity_types[ET_PLAYER]->attr_types[PL_SPEED].min > cl->m_actions[j][M_ACT_SPD] )
						cl->m_actions[j][M_ACT_SPD] = entity_types[ET_PLAYER]->attr_types[PL_SPEED].min;
				}
				if( is_quest && ( (act_index % 2) != 0 ) )
					cl->m_actions[j][M_ACT_DIR] = server_thread_generate_quest_move( &pl_r, pl->ent_id );

				if( sv.straight_move )
				{
					if( first_pl_dir == -1 )	first_pl_dir = cl->m_actions[j][M_ACT_DIR];
					else						cl->m_actions[j][M_ACT_DIR] = first_pl_dir;
				}

				vect_t move_v;
				vect_scale( &dirs[cl->m_actions[j][M_ACT_DIR]], cl->m_actions[j][M_ACT_SPD], &move_v );
				vect_add( &pl_r.v1, &move_v, &pl_r.v1 );
				vect_add( &pl_r.v2, &move_v, &pl_r.v2 );
				// TO DO: change to allow multiple move records in file
				//server_register_player_move( pl, i );

				#ifdef PRINT_MOVES
				printf( "cl:%3d cycle:%3d j: %d ;   ", cl->cl_id, cycle, j );
				printf( "pos: %3d,%3d - last_dist %d ;  ", cl->player->r.v1.x, cl->player->r.v1.y, cl->last_dist );
				printf( "act: %5s spd: %d dir: %5s\n", action_names[ cl->m_actions[j][M_ACT_ID] ],
					cl->m_actions[j][M_ACT_SPD], dir_names[ cl->m_actions[j][M_ACT_DIR] ] );
				#endif
			}
		}
	}
}

static void server_thread_preprocess_workload( server_thread_t* svt, int cycle )
{
#ifndef RUN_TRACE_ALL_MOVES
	server_thread_generate_moves( svt, cycle );
#else
	server_thread_replay_moves( svt, cycle );
#endif
}

static void server_thread_process_workload( server_thread_t* svt )
{
	sv_client_t* cl;
	int i;
	for( i = 0; i < sv.n_clients; i++ )
	{
		cl = sv.clients[i];
		if( cl->tid == svt->tid )
			server_act_cl_synthetic( cl->cl_id );
	}
}

static void server_thread_admin(int cycle)
{
	if( sv.wl_quest_count &&
			cycle == (sv.quest_times[sv.quest_id] + sv.wl_quest_duration) &&
			sv.quest_id < sv.wl_quest_count - 1 )
		sv.quest_id++;

	if( sv.print_progress && (cycle % sv.print_progress) == 0 )
	{
		printf( "%d ", cycle / sv.print_progress );
		if( cycle + sv.print_progress >= sv.wl_cycles )		printf( "\n" );
		fflush( stdout );
	}

	// change to "==" to do load bal every n cycles
	if( cycle % 10 >= 0 )		loadb_balance( cycle );
}

static void* server_thread_run( void* arg )
{
	int cycle;
	unsigned long long now_0, now_1, now_2;
	server_thread_t* svt = (server_thread_t*) arg;
	__tid = svt->tid;

	thread_set_affinity( __tid );
	thread_set_local_id( __tid );
	log_info( "Starting workload...\n" );

	for( cycle = 0; cycle < sv.wl_cycles; cycle++ )
	{
		server_thread_preprocess_workload( svt, cycle );

		barrier_wait( &sv.barrier );			now_0 = get_c();

		server_thread_process_workload( svt );	now_1 = get_c();

		barrier_wait( &sv.barrier );			now_2 = get_c();

		time_event( PROCESS, (now_1 - now_0) );
		time_event( BARRIER, (now_2 - now_1) );
		time_event( TOTAL, (now_2 - now_0) );

		if( svt->tid == 0 )	server_thread_admin( cycle );

		barrier_wait(&sv.barrier);
	}

	barrier_wait( &sv.barrier );

	return 0;
}

void server_thread_init( server_thread_t* svt, int _tid )
{
	assert( svt && _tid >= 0 );
	svt->tid = _tid;
	svt->done = 0;

	server_stats_reset( &svt->stats );
}

static void server_generate_clients( int count )
{
	int i;
	unsigned char s[4] = { 192, 168, 1, 1 };
	char pname[MAX_NAME_LEN];
	entity_t* new_pl;

	log_info( "Generating clients... \n" );

	for( i = 0; i < count; i++ )
	{
		sprintf( pname, "Player_%hhu.%hhu.%hhu.%hhu", s[0], s[1], s[2], s[3] );
		if( s[3] == 255 )
		{
			s[2]++;
			s[3] = 1;
		}
		else s[3]++;

		new_pl = server_add_cl( pname );
		if( !new_pl )	break;

		log_info1( "[I] %s generated.\n", pname );
		server_register_player( new_pl );
	}

	log_info1( "[I] %d players generated.\n", i );
}

#ifndef RUN_QUESTS
static void server_generate_quests_random(void)
{
	int i, j;
	for( i = 0; i < sv.wl_quest_count; i++ )
	{
		for( j = 0; j < sv.wl_quest_spread; j++ )
		{
			sv.quest_locations[i][j].v1.x = rand() % ((int) wm.size.x - 2) + 1; //mike: ignore warning, coord_t is unsiged short // borders are walls
			sv.quest_locations[i][j].v1.y = rand() % ((int) wm.size.y - 2) + 1; //mike: ignore warning, coord_t is unsiged short // place quest inside map
			sv.quest_locations[i][j].v2.x = sv.quest_locations[i][j].v1.x;
			sv.quest_locations[i][j].v2.y = sv.quest_locations[i][j].v1.y;

			server_register_quest( i, j );
		}
	}
}
#endif

static void server_init_quests(void)
{
	sv.quest_id = 0;

	if( sv.wl_quest_count == 0 )		return;

	sv.wl_quest_duration = sv.wl_cycles / sv.wl_quest_count;	assert( sv.wl_quest_duration > 0 );

	sv.quest_times = (int*) malloc_wrapper( sv.wl_quest_count * sizeof(int) );				assert( sv.quest_times );
	sv.quest_locations = (rect_t**) malloc_wrapper( sv.wl_quest_count * sizeof(rect_t*) );	assert( sv.quest_locations );

	int i;
	for( i = 0; i < sv.wl_quest_count; i++ )
	{
		sv.quest_locations[i] = (rect_t*) malloc_wrapper( sv.wl_quest_spread * sizeof(rect_t) );
		assert( sv.quest_locations[i] );
		sv.quest_times[i] = sv.wl_quest_duration * i;
	}

#ifndef RUN_QUESTS
	server_generate_quests_random();
#else
	server_load_quests();
#endif
}

static void server_generate_workload(void)
{
	server_generate_clients( sv.wl_client_count );

#ifdef RUN_TRACE_ALL_MOVES
	server_load_player_moves();
#endif
}

static void server_init_multiple_actions(void)
{
	char nm[MAX_FILE_READ_BUFFER];
	FILE* actions_f = fopen( sv.m_actions_file, "r" );
	if( !actions_f )
	{
		sv.n_multiple_actions = 1;
		sv.randomized_actions = 0;
		sv.move_fail_stop = 1;
		sv.straight_move = 1;

		sv.m_actions = (int*) malloc_wrapper( sv.n_multiple_actions * sizeof(int) );
		sv.m_actions[0] = AC_MOVE;
		
		printf( "Using default configuration for actions: 1 Move per cycle (move_fail_stop=true)  (straight_move=true).\n" );
		return;
	}

	char str[64];
	int line = 0, cnt = 0, i, ratio;
	while ( fgets(nm, MAX_FILE_READ_BUFFER, actions_f) != NULL )
	{
		if ( nm[0] == 10  || nm[0] == 13 )	continue;
		if ( nm[0] == '#' || nm[0] == '[' )	continue;

		if( line == 0 )
		{
			if( sscanf( nm, "%d %d %d %d", &sv.n_multiple_actions, &sv.randomized_actions, &sv.move_fail_stop, &sv.straight_move ) != 4 )
			{		printf( "%s: Ignoring line \"%s\"", sv.m_actions_file, nm );continue;	}
			assert( sv.n_multiple_actions > 0 && sv.randomized_actions >= 0 && sv.randomized_actions <= 1 );
			assert( sv.move_fail_stop >= 0 && sv.move_fail_stop <= 1 );
			assert( sv.straight_move >= 0 && sv.straight_move <= 1 );
			if( !sv.randomized_actions )
				sv.m_actions = (int*) calloc_wrapper( sv.n_multiple_actions, sizeof(int) );
			else
				sv.m_actions_ratios = (int*) calloc_wrapper( n_actions, sizeof(int) );
		}
		else
		{
			if( !sv.randomized_actions )
			{
				if( sscanf( nm, "%s", str ) != 1 )
				{		printf( "%s: Ignoring line \"%s\"", sv.m_actions_file, nm );continue;	}
				for( i = 0; i < n_actions; i++ )
					if( !strcmp( str, action_names[i] ) )	break;
				assert( i < n_actions );
				sv.m_actions[ cnt ] = i;
				cnt++;
				if( cnt == sv.n_multiple_actions )	break;
			}
			else
			{
				if( sscanf( nm, "%s %d", str, &ratio ) != 2 )
				{		printf( "%s: Ignoring line \"%s\"\n", sv.m_actions_file, nm );continue;	}
				assert( ratio >=0 && ratio <= 100 );
				for( i = 0; i < n_actions; i++ )
					if( !strcmp( str, action_names[i] ) )	break;
				assert( i < n_actions );
				sv.m_actions_ratios[ i ] = ratio;
				cnt++;
				if( cnt == n_actions )	break;
			}
		}
		line++;
	}

	if( !sv.randomized_actions )
	{
		assert( cnt == sv.n_multiple_actions );
	}
	else
	{
		int rez = 0;
		for( i = 0; i < n_actions; i++ )	rez += sv.m_actions_ratios[ i ];
		assert( rez == 100 );
		for( i = 1; i < n_actions; i++ )	sv.m_actions_ratios[ i ] += sv.m_actions_ratios[ i-1 ];
	}
}

static void server_init( char* conf_file_name, int map_szx, int map_szy, int tdepth, int speed_min, int speed_max,
			 int apple_map_ratio, int apple_pl_ratio, int wall_map_ratio, int wall_pl_ratio, char* balname )
{
	int i;

	rand_seed = (unsigned int) time( NULL );
	srand( (unsigned int)time(NULL) );

	conf_t* c = conf_create();	assert(c);
	conf_parse_file( c, conf_file_name );

	/* server */
	sv.port				= conf_get_int( c, "server.port" );
	//sv.num_threads		= conf_get_int( c, "server.number_of_threads" );
	sv.update_interval	= conf_get_int( c, "server.update_interval" );
	sv.stats_interval	= conf_get_int( c, "server.stats_interval" );

	#ifdef REGISTER_TRACES
	if(sv.num_threads != 1) {
		printf("Cannot print trace in multithread version !\n"); exit(1);
	}
	#endif

	assert( sv.port > 1023 );
	assert( sv.num_threads > 0 && sv.num_threads <= MAX_THREADS );
	assert( sv.update_interval > 0 );
	assert( sv.stats_interval > 0 );

	/* quests */
	sv.quest_between	= conf_get_int( c, "server.quest_between" );
	sv.quest_length		= conf_get_int( c, "server.quest_length" );

	assert( sv.quest_between > 0 && sv.quest_length > 0 );
	assert( sv.quest_between > sv.update_interval && sv.quest_length > sv.update_interval );
	
	/* initialize clients array */
	sv.n_clients = 0;
	sv.clients	 = calloc_wrapper( MAX_ENTITIES, sizeof( sv_client_t* ) );assert( sv.clients );
	sv.mutex = mutex_create();

	/* initialize world */
	server_traces_init();
	actions_init( c );
	server_init_multiple_actions();
	

	entity_types_init( c );
	// override the speed settings read from the config file
	entity_types[ ET_PLAYER ]->attr_types[ PL_SPEED ].min = speed_min;
	entity_types[ ET_PLAYER ]->attr_types[ PL_SPEED ].max = speed_max;
	// override the ratio settings read from the config file
	entity_types[ ET_APPLE ]->ratio    = apple_map_ratio;
	entity_types[ ET_APPLE ]->pl_ratio = apple_pl_ratio;
	entity_types[ ET_WALL ]->ratio    = wall_map_ratio;
	entity_types[ ET_WALL ]->pl_ratio = wall_pl_ratio;
	
	worldmap_init( map_szx, map_szy, tdepth );
	server_init_quests();
	
	worldmap_generate();
	worldmap_is_valid();
	/* initialize synthetic workload */
	server_generate_workload();
	worldmap_is_valid();

	loadb_init( balname );

	/* initialize syncronization & server threads */
	barrier_init( &sv.barrier, sv.num_threads );
	server_stats_init();
	sv.done = 0;

	svts = malloc_wrapper( sv.num_threads * sizeof( server_thread_t ) );
	for( i = 0; i < sv.num_threads; ++i )	server_thread_init( &svts[i], i );

	log_info("[I] Server init done.\n");
}

static void server_deinit(void)
{
	sv.done = 1;

	int i;
	for( i = 1; i < sv.num_threads; ++i )	thread_join( svts[i].thread );

	server_stats_dump( sv.stats_f );
	server_stats_dump( stdout );

	server_stats_deinit();
	server_traces_deinit();

	log_info( "[I] Server deinit done.\n" );
}

static void decode_entity_ratio( int ent_map_ratio, int ent_pl_ratio, int* r_ent_map_ratio, int* r_ent_pl_ratio, int mapx, int mapy, int depth )
{
	long long int aux;
	if( ent_map_ratio < 0 )
	{
		ent_pl_ratio = - ent_map_ratio;
		if( sv.wl_quest_count )
			aux = (long long int)sv.wl_client_count * ent_pl_ratio * ( 1 << depth ) * 1000 / ( mapx * mapy * 4 * sv.wl_quest_spread );
		else
			aux = (long long int)sv.wl_client_count * ent_pl_ratio * 1000 / ( mapx * mapy );
		ent_map_ratio = aux;
	}
	else
	{
		if( sv.wl_quest_count )
			aux = (long long int)ent_map_ratio * mapx * mapy * 4 * sv.wl_quest_spread / ( sv.wl_client_count * ( 1 << depth ) * 1000 );
		else
			aux = (long long int)ent_map_ratio * mapx * mapy / ( sv.wl_client_count * 1000 );
		ent_pl_ratio = aux;
	}
	
	*r_ent_map_ratio = ent_map_ratio;
	*r_ent_pl_ratio = ent_pl_ratio;
}

int main(int argc, char *argv[])
{
	if( argc < 15 )
		pexit1( "Usage: %s N_THREADS N_CLIENTS N_CYCLES N_QUESTS QSpread QuestsFile MapSizeX MapSizeY TreeDepth "
				"SpeedMax AppleRatio WallRatio BalanceType ActionsFile [print= -100:100] [lock_type= lock_tree | lock_leaves]\n"
				"For more help check readme.txt .\n", argv[0] );

	sv.num_threads = atoi( argv[1] );
	sv.wl_client_count = atoi( argv[2] );
	sv.wl_cycles = atoi( argv[3] );
	sv.wl_quest_count = atoi( argv[4] );
	sv.wl_quest_spread = atoi( argv[5] );
	sv.wl_quest_file = argv[6];
	assert( !sv.wl_quest_count || sv.wl_quest_spread );
	
	int mapx = atoi( argv[7] );
	int mapy = atoi( argv[8] );
	int depth = atoi( argv[9] );
	int speed_max = atoi( argv[10] );
	int apple_map_ratio = atoi( argv[11] );
	int wall_map_ratio =  atoi( argv[12] );
	char* balance = argv[13];
	sv.m_actions_file = argv[14];
	
	sv.wl_proportional_quests = 0;
	if( sv.wl_quest_count < 0 )
	{
		sv.wl_quest_count = -sv.wl_quest_count;
		sv.wl_proportional_quests = 1;
	}
	
	int apple_pl_ratio;
	int wall_pl_ratio;

	decode_entity_ratio( apple_map_ratio, apple_pl_ratio, &apple_map_ratio, &apple_pl_ratio, mapx, mapy, depth ); //mike: ignore warning, apple_pl_ratio assigned in the function
	decode_entity_ratio( wall_map_ratio, wall_pl_ratio, &wall_map_ratio, &wall_pl_ratio, mapx, mapy, depth ); //mike: ignore warning, wall_pl_ratio assigned in the function

	if( argc >= 16 )
	{
		int print_v = atoi( argv[15] );
		if( print_v < 0 )	sv.print_pls = 1;
		print_v = abs( print_v );
		assert( print_v >= 0 && print_v <= 100 );
		
		sv.print_grid = (sv.wl_cycles * print_v ) / 100;
		if( print_v && sv.print_grid == 0 )		sv.print_grid = 1;
		
		if( sv.print_grid == 0 || sv.print_grid == sv.wl_cycles )
			sv.print_progress = ( sv.wl_cycles < 10 ) ? 1 : (sv.wl_cycles/10);
	}

	sv.lock_type = LOCK_LEAVES;
	if( argc >= 17 )
	{
		if( !strcmp( argv[16], LOCK_TREE_S ) )			sv.lock_type = LOCK_TREE;
		else if( !strcmp( argv[16], LOCK_LEAVES_S ) )	sv.lock_type = LOCK_LEAVES;
		else
		{
			fprintf(stderr, "Lock Type is not valid.");
			abort();
		}
	}

	server_init( "./config/default.cfg", mapx, mapy, depth, speed_max/4, speed_max, apple_map_ratio, apple_pl_ratio,
					wall_map_ratio, wall_pl_ratio, balance );

	int i;
	for( i = 1; i < sv.num_threads; ++i )	
	{
		svts[i].thread = thread_create( server_thread_run, (void*)&svts[i] );
		assert( svts[i].thread );
	}
	server_thread_run( (void*)&svts[0] );
	server_deinit();

	return 0;
}
#endif



