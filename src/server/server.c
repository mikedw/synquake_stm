#include "server.h"
#include "sgame/worldmap.h"
#include "../utils/malloc.h"
#include "loadbal/load_balancer.h"


#ifndef __SYNTHETIC__



server_t			sv;
server_thread_t*	svts;

__thread int __tid;

char stats_names[N_STATS][32] = { "WAIT_REQ", "RECV_REQ", "PROC_REQ", "SV_PROC_REQ", "PROC_UPD", "SEND_UPD", "SV_PROC_UPD" };

void server_on_join( pack_t* pck, sock_t* s )
{
	addr_t* addr = pack_getaddr( pck );
	char pname[ MAX_NAME_LEN ];
	unsigned char* hn = (unsigned char*)&addr->host;
	sprintf( pname, "Player_%u.%u.%u.%u_%u", hn[0], hn[1], hn[2], hn[3], addr->port );

	pack_t* pck_r;
	entity_t* new_pl = server_add_cl( pname, addr );
	if( new_pl )
	{
		pck_r = pack_create( addr, SV_JOIN_OK );
		entity_pack( new_pl, pck_r->st );
		streamer_copy( pck_r->st, sv.map_st );

		printf("[I] %s joined.\n", pname );
	}
	else	pck_r = pack_create( addr, SV_JOIN_NOK );

	sock_send( s, pck_r );
}

void server_on_leave( pack_t* pck )
{
	int cl_id = streamer_rdint( pck->st );
	assert( cl_id >= 0 && cl_id < MAX_ENTITIES );
	server_del_cl( cl_id, pack_getaddr(pck) );
}

void server_on_action( pack_t* pck )
{
	int cl_id = streamer_rdint( pck->st );
	assert( cl_id >= 0 && cl_id < MAX_ENTITIES );
	server_act_cl( cl_id, pack_getaddr(pck), pck->st );
}

void server_thread_process_req( server_thread_t* svt )
{
	pack_t* pck;
	unsigned long long now_0 = get_t(), now_1, now_12, now_2;
	unsigned long long recv_until = now_0/1000000 + sv.update_interval;

	now_1 = now_0;
	while( 1 )
	{
		pck = sock_receive( &svt->s, (int)(recv_until - now_1/1000000), &now_12 );
		time_event( WAIT_REQ, now_12 - now_1  );

		if( pck == NULL )	break;
		now_2 = get_t();	time_event( RECV_REQ, now_2  - now_12 );

		switch( pck->type )
		{
			case CL_JOIN:	server_on_join( pck, &svt->s );	break;
			case CL_LEAVE:	server_on_leave( pck );			break;
			case CL_ACTION:	server_on_action( pck );		break;
			default:		printf("[W] Unknown message (%d) received.\n", pck->type );
		}
		pack_destroy( pck );

		now_1 = get_t();	time_event( PROC_REQ, now_1 - now_2  );
	}

	time_event( SV_PROC_REQ, now_12 - now_0  );
}

void server_thread_process_upd( server_thread_t* svt )
{
	unsigned long long now_0 = get_t(), now_1, now_2;
	pack_t* pck;
	int i;

	now_1 = now_0;
	for( i = 0; i < MAX_ENTITIES; i++ )
	{
		sv_client_t* cl = sv.clients[i];
		if( !cl || cl->tid != svt->tid )	continue;

		pck = pack_create( &cl->addr, SV_UPDATE );

		server_update_cl( cl, pck->st );
		streamer_wrchar( pck->st, sv.quest_state );
		if( sv.quest_state == QT_NEW )		rect_pack( &sv.quest_loc, pck->st );

		now_2 = get_t();	time_event( PROC_UPD, now_2 - now_1  );

		sock_send( &svt->s, pck );
		now_1 = get_t();	time_event( SEND_UPD, now_1 - now_2  );
	}

	time_event( SV_PROC_UPD, now_1 - now_0  );
}

static void server_thread_admin(int cycle)
{
	int i;
	unsigned long long now = get_tm();

	if( now < sv.start_quest )		sv.quest_state = QT_INACTIVE;
	else if( sv.start_quest <= now && now < sv.stop_quest )
	{
		if( sv.quest_state == QT_INACTIVE )
		{
			sv.quest_state = QT_NEW;
			printf("[I] New quest at: (%d,%d) (%d,%d).\n", sv.quest_loc.v1.x, sv.quest_loc.v1.y,
			 											   sv.quest_loc.v2.x, sv.quest_loc.v2.y );
		}
		else if( sv.quest_state == QT_NEW )			sv.quest_state = QT_ACTIVE;
	}
	else if( sv.stop_quest <= now )
	{
		printf("[I] Quest stoped.\n" );
		sv.quest_state = QT_INACTIVE;

		sv.start_quest = sv.stop_quest + sv.quest_between;
		sv.stop_quest  = sv.start_quest + sv.quest_length;
		rect_generate_position( &sv.quest_loc, &sv.quest_sz, &wm.map_r );
	}

	if( now >= sv.next_stats_dump )
	{
		sv.next_stats_dump += sv.stats_interval;
		for( i = 0; i < sv.num_threads; ++i )
			server_thread_stats_dump( &svts[i] );
		fprintf( sv.stats_f, "\n" );
	}

	if(cycle%10 >= 0) // change to "==" to do load bal every n cycles
		loadb_balance(wm.balance_type, cycle);

	if( sv.done )	for( i = 0; i < sv.num_threads; ++i )	svts[i].done = 1;
}

void server_thread_disconnect( server_thread_t* svt )
{
	int i;
	for( i = 0; i < MAX_ENTITIES; i++ )
	{
		sv_client_t* cl = sv.clients[i];
		if( !cl || cl->tid != svt->tid )	continue;

		sock_send( &svt->s, pack_create( &cl->addr, SV_LEAVE ) );
	}
}

static void* server_thread_run( void* arg )
{
	server_thread_t* svt = (server_thread_t*)arg;
	__tid = svt->tid;

	thread_set_affinity(  __tid );

	while( !svt->done )
	{
		server_thread_process_req(svt);
		barrier_wait(&sv.barrier);

		if( svt->tid == 0 )		server_thread_admin();
		barrier_wait(&sv.barrier);

		server_thread_process_upd(svt);
		barrier_wait(&sv.barrier);
	}

	server_thread_disconnect(svt);

	return 0;
}

void server_thread_stats_reset( server_thread_t* svt )
{
	int i;
	for( i = 0; i < N_STATS; i++ )
	{
		svt->stats_tm[i] = 0;
		svt->stats_n[i] = 0;
	}
	svt->s.bytes_recv = 0;
	svt->s.bytes_sent = 0;
}

void server_thread_stats_dump( server_thread_t* svt )
{
	int i;
	for( i = 0; i < N_STATS; i++ )
		fprintf( sv.stats_f, "%lf %lld  ", svt->stats_tm[i]/(double)1000000000.0, svt->stats_n[i] );
	fprintf( sv.stats_f, "%f %f   ", svt->s.bytes_recv/1024.0, svt->s.bytes_sent/1024.0 );

	server_thread_stats_reset( svt );
}


void server_thread_init( server_thread_t* svt, int _tid )
{
	assert( svt && _tid >= 0 );
	svt->tid = _tid;
	svt->done = 0;
	sock_init( &svt->s, sv.port + _tid );

	server_thread_stats_reset( svt );

	svt->thread = thread_create( server_thread_run, (void*)svt );
	assert( svt->thread );
}

void server_stats_init()
{
	int i,j;

	sv.stats_f = fopen( "results/stats.out", "a" );		assert( sv.stats_f );
	sv.next_stats_dump = get_tm() + sv.stats_interval;

	for( i = 0; i < sv.num_threads; ++i )
	{
		for( j = 0; j < N_STATS; ++j )
			fprintf( sv.stats_f, "%s_TM(%d) %s_N(%d)  ", stats_names[j], i, stats_names[j], i );
		fprintf( sv.stats_f, "Kbytes_recv(%d) Kbytes_sent(%d)   ", i, i );
	}
	fprintf( sv.stats_f, "\n\n" );
}

static void server_init( char* conf_file_name, int map_szx, int map_szy, int tdepth, char* baltype )
{
	char str[128];
	srand( (unsigned int)time(NULL) );
	int rez = net_init(); assert( rez >= 0 );

	conf_t* c = conf_create();	assert(c);
	conf_parse_file( c, conf_file_name );

	/* server */
	sv.port				= conf_get_int( c, "server.port" );
	sv.num_threads		= conf_get_int( c, "server.number_of_threads" );
	sv.update_interval	= conf_get_int( c, "server.update_interval" );
	sv.stats_interval	= conf_get_int( c, "server.stats_interval" );

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

	/* initialize world */
	actions_init( c );
	entity_types_init( c );
	worldmap_init( map_szx, map_szy, tdepth, baltype );
	server_stats_init(); /* initialize stats - needs balance type*/
	worldmap_generate();

	printf( "%d %d %d\n", wm.n_entities[0], wm.n_entities[1], wm.n_entities[2] );

	buffer_init( &sv.map_buff, MAX_PACKET_SIZE );
	sv.map_st = buffer_getstreamer( &sv.map_buff );
	worldmap_pack_fixed( sv.map_st );

	/* initialize quest */
	vect_init( &sv.quest_sz, action_ranges[AC_VIEW].front, action_ranges[AC_VIEW].front );
	sv.start_quest = get_tm() + sv.quest_between;
	sv.stop_quest  = sv.start_quest + sv.quest_length;
	rect_generate_position( &sv.quest_loc, &sv.quest_sz, &wm.map_r );

	/* initialize syncronization & server threads */
	barrier_init( &sv.barrier, sv.num_threads );
	sv.mutex = mutex_create();
	sv.done = 0;

	svts = (server_thread_t *)malloc_wrapper( sv.num_threads * sizeof( server_thread_t ) );
	for( i = 0; i < sv.num_threads; ++i )		server_thread_init( &svts[i], i );

	printf("[I] Server init done. Waiting for clients ...\n");
}

static void server_deinit(void)
{
	sv.done = 1;

	int i;
	for( i = 0; i < sv.num_threads; ++i )	thread_join( svts[i].thread );

	net_deinit();
}

int main(int argc, char *argv[])
{
	if( argc < 6 )
		pexit1( "Usage: %s <config_file> MapSizeX MapSizeY TreeDepth BalanceType\n", argv[0] );

	server_init( argv[1], atoi(argv[2], atoi(argv[3]), atoi(argv[4]), argv[5]);

	//* User input loop (type 'quit' to exit)
	char cmd[256];
	while( 1 )
	{
		scanf("%s", cmd);
		if ( !strcmp(cmd, "exit") || !strcmp(cmd, "quit") || !strcmp(cmd, "q") )	break;
	}

	server_deinit();

	return 0;
}
#endif


