#ifndef SERVER_STATS_H_
#define SERVER_STATS_H_

#ifndef __SYNTHETIC__


#include "server_cl.h"

/* STATS INDEXES   */


#define		WAIT_REQ			0
#define		RECV_REQ			1
#define		PROC_REQ			2
#define		SV_PROC_REQ			3

#define		PROC_UPD			4
#define		SEND_UPD			5
#define		SV_PROC_UPD			6

#define		N_STATS				7



#else


#include "syn_server_cl.h"

/* STATS INDEXES   */


#define		TOTAL				0
#define		PROCESS				1

#define		LOCK				2
#define		LOCK_P				3
#define		GET_ENTS			4
#define		GET_NODES			5
#define		ACTION				6
#define		UNLOCK				7
#define		UNLOCK_P			8
#define		DEST_ENTS			9
#define		DEST_NODES			10

#define		BARRIER				11

#define		CHECK_PL			12

#define		N_STATS				13


#endif

typedef struct
{
	unsigned long long tm[N_STATS];
	unsigned long long n[N_STATS];
} server_stats_t;

#define time_event( ev_id, _tm )				\
({												\
	svts[__tid].stats.tm[(ev_id)] += (_tm);		\
	svts[__tid].stats.n[(ev_id)]++;				\
})

#define time_function( function, _tmp_tm )		\
({												\
	_tmp_tm = get_c();							\
	function;									\
	( get_c() - _tmp_tm );						\
})

//mike: missing declarations in the header file
void server_stats_print_header( FILE* f_out );
void server_stats_init();
void server_stats_deinit();
void server_stats_reset( server_stats_t* svt_stats );
//mike: missing declarations in the header file
void server_stats_total( server_stats_t* tot_stats );
//mike: missing declarations in the header file
void server_thread_stats_dump( FILE* f_out, server_stats_t* svt_stats );
void server_stats_dump( FILE* f_out );
void server_stats_print_map( FILE* f_out );

#endif /* SERVER_STATS_H_ */
