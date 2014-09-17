#include "../utils/malloc.h"
#include "server.h"
#include "sgame/worldmap.h"
#include "server_stats.h"


char stats_names[N_STATS][32] = { "TOTAL", "PROCESS", "LOCK", "LOCK_P", "GET_ENTS", "GET_NODES", "ACTION", "UNLOCK", "UNLOCK_P", "DEST_ENTS", "DEST_NODES", "BARRIER", "CHECK_PL" };
int stats_display_tm[N_STATS] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0 };
int stats_display_n[N_STATS] = { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 };


void server_stats_print_header( FILE* f_out )
{
	int i, j;

	for( i = 0; i < sv.num_threads; ++i )
	{
		for( j = 0; j < N_STATS; ++j )
		{
			if( !stats_display_tm[j] )	continue;
			fprintf( f_out, "%s_t(%d) ", stats_names[j], i );
		}
		fprintf( f_out, " " );
		for( j = 0; j < N_STATS; ++j )
		{
			if( !stats_display_n[j] )	continue;
			fprintf( f_out, "%s_n(%d) ", stats_names[j], i );
		}
		fprintf( f_out, " " );

		if( f_out == stdout )	break;
	}
	fprintf( f_out, "\n" );
}

void server_stats_init()
{
	int i, j;
	sv.stats_f = fopen( "./results/stats.out", "a" );		assert( sv.stats_f );
	sv.next_stats_dump = get_tm() + sv.stats_interval;

	char str[256] = "", str1[256] = "", str11[256] = "", str2[256] = "", str3[256] = "";
	sprintf( str, "N_TH %d  N_CLIENTS %d  N_CYCLES %d  N_QUESTS %d  QSpread %d  QFile %s  MAP(%d,%d)  TreeDepth %d",
				sv.num_threads, sv.wl_client_count, sv.wl_cycles, sv.wl_quest_count, sv.wl_quest_spread, sv.wl_quest_file,
				wm.size.x, wm.size.y, wm.depth );
	sprintf( str1, "SpdMin %d  SpdMax %d AppleMapR %d ApplePlR %d  WallMapR %d WallPlR %d  Balance %s  Print %d  LockType %s",
				entity_types[ ET_PLAYER ]->attr_types[ PL_SPEED ].min,
				entity_types[ ET_PLAYER ]->attr_types[ PL_SPEED ].max,
				entity_types[ ET_APPLE ]->ratio, entity_types[ ET_APPLE ]->pl_ratio,
				entity_types[ ET_WALL ]->ratio, entity_types[ ET_WALL ]->pl_ratio,
				sv.balance_name, sv.print_grid, lock_type_names[ sv.lock_type ] );
	if( sv.wl_quest_count )		sprintf( str11, "Quests:  " );
	for( i = 0; i < sv.wl_quest_count; i++ )
	{
		sprintf( str11 + strlen( str11 ), "[ " );
		for( j = 0; j < sv.wl_quest_spread; j++ )
			sprintf( str11 + strlen( str11 ), "(%d,%d) ", sv.quest_locations[i][j].v1.x, sv.quest_locations[i][j].v1.y );

		sprintf( str11 + strlen( str11 ), "] " );
	}
	sprintf( str2, "N_SubActions %d  Randomized_SubActions %d  Move_Fail_Stop %d  Straight_Move %d : ",
				sv.n_multiple_actions, sv.randomized_actions, sv.move_fail_stop, sv.straight_move );
	if( !sv.randomized_actions )
	{
		for( i = 0; i < sv.n_multiple_actions; i++ )
			sprintf( str2 + strlen( str2 ), "%s ", action_names[ sv.m_actions[i] ] );
	}
	else
	{
		sprintf( str2 + strlen( str2 ), "%s(%d) ", action_names[ 0 ], sv.m_actions_ratios[0]  );
		for( i = 1; i < n_actions; i++ )
			sprintf( str2 + strlen( str2 ), "%s(%d) ", action_names[ i ], sv.m_actions_ratios[i] - sv.m_actions_ratios[i-1]  );
	}
	sprintf( str3, "N_Entities(ratios): " );
	for( i = 0; i < n_entity_types; i++ )
		sprintf( str3 + strlen( str3 ), "%d(%d/%d)(%d) ", wm.n_entities[i], wm.et_ratios[i] * MAX_RATIO / wm.area, MAX_RATIO,
																entity_types[i]->pl_ratio );

	fprintf( sv.stats_f, "%s %s %s %s %s  ", str, str1, str11, str2, str3 );	fflush( sv.stats_f );
	fprintf( stdout, "%s\n%s\n%s\n%s\n%s\n", str, str1, str11, str2, str3 );	fflush( stdout );

	if( sv.print_pls )
	{
		time_t time_value = time(NULL);
		struct tm *now = localtime(&time_value);
		sprintf( str, "players_%.2d%.2d%.2d_%.2d%.2d.out", 
					now->tm_year%100, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min );

		sv.f_players = fopen( str, "w" );assert( sv.f_players );
		fprintf( sv.f_players, "# %d %d %d %d %d %d %d %d\n", sv.num_threads, sv.wl_client_count, sv.wl_cycles, 
															wm.size.x, wm.size.y, wm.depth, sv.wl_quest_spread, sv.print_grid );
	}
}

void server_stats_deinit()
{
	if( sv.print_pls )		fclose( sv.f_players );
	fclose( sv.stats_f );
}


void server_stats_reset( server_stats_t* svt_stats )
{
	int i;
	for( i = 0; i < N_STATS; i++ )
	{
		svt_stats->tm[i] = 0;
		svt_stats->n[i] = 0;
	}
}


void server_stats_total( server_stats_t* tot_stats )
{
	int i, j;
	server_stats_reset( tot_stats );
	for( j = 0; j < N_STATS; j++ )
		for( i = 0; i < sv.num_threads; ++i )
		{
			tot_stats->tm[j] += svts[i].stats.tm[j];
			tot_stats->n[j]  += svts[i].stats.n[j];
		}
}


void server_thread_stats_dump( FILE* f_out, server_stats_t* svt_stats )
{
	char str[10];
	int j;
	
	for( j = 0; j < N_STATS; j++ )
	{
		if( !stats_display_tm[j] )	continue;
		sprintf( str, "%%%d.2lf ", (int)strlen( stats_names[j] )+5 );
		fprintf( f_out, str, c_to_t( svt_stats->tm[j] ) );
	}
	fprintf( f_out, " " );
	for( j = 0; j < N_STATS; j++ )
	{
		if( !stats_display_n[j] )	continue;
		sprintf( str, "%%%dlld ", (int)strlen( stats_names[j] )+5 );
		fprintf( f_out, str, svt_stats->n[j] );
	}
	fprintf( f_out, f_out == stdout ? "\n" : " " );
}


void server_stats_dump( FILE* f_out )
{
	int i;
	
	if( f_out == stdout )
		server_stats_print_header( f_out );

	for( i = 0; i < sv.num_threads; ++i )
		server_thread_stats_dump( f_out, &svts[i].stats );

	server_stats_t tot_stats;
	server_stats_total( &tot_stats );
	
	if( f_out == stdout )		fprintf( f_out, "Summary:\n" );
	server_thread_stats_dump( f_out, &tot_stats );
	
	fprintf( f_out, "\n" );		fflush( f_out );
}

void server_stats_print_map( FILE* f_out )
{
	int i, j, ind_x, ind_y;
	int n_regs_x = 1 << ((wm.depth+1)/2);
	int n_regs_y = 1 << (wm.depth/2);
	int dim_reg_x = wm.size.x / n_regs_x;
	int dim_reg_y = wm.size.y / n_regs_y;

	fprintf( f_out, "%d %d %d %d\n", n_regs_x, n_regs_y, dim_reg_x, dim_reg_y );

	int** map_players = (int**) malloc_wrapper( n_regs_x * sizeof(int*) );	assert( map_players );
	for( i = 0; i < n_regs_x; i++ )
	{
		map_players[i] = (int* )malloc_wrapper( n_regs_y * sizeof(int) );	assert( map_players[i] );
		for( j = 0; j < n_regs_y; j++ )	map_players[i][j] = 0;
	}

	for( i = 0; i < sv.n_clients; i++ )
	{
		sv_client_t* cl = sv.clients[i];
		entity_t* pl = cl->player;
		rect_t* prect = &pl->r;

		ind_x = prect->v1.x / dim_reg_x;
		ind_y = prect->v1.y / dim_reg_y;
		map_players[ind_x][ind_y]++;
	}

	for( j = n_regs_y-1; j >= 0; j-- )
	{
		for( i = 0; i < n_regs_x; i++ )
			fprintf( f_out, "%4d ", map_players[i][j] );
		fprintf( f_out, "\n" );
	}
	for( i = 0; i < n_regs_x; i++ )
		free_wrapper( map_players[i] );
	free_wrapper( map_players );
}

