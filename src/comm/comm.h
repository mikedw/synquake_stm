#ifndef NET_H_
#define NET_H_

#ifdef USE_SDL_NET

#include <SDL_net.h>

#include "../general.h"
#include "../utils/streamer.h"


#define net_init		SDLNet_Init
#define net_deinit		SDLNet_Quit


#define SV_JOIN_OK		1
#define SV_JOIN_NOK		2
#define SV_UPDATE		3
#define SV_LEAVE		4


#define CL_ACTION		5
#define CL_LEAVE		6
#define CL_JOIN			7


#define MAX_PACKET_SIZE (64 * 1024 - 1)



typedef IPaddress addr_t;

void addr_init( addr_t* a, int host, int port );
#define addr_equal( a1, a2 )					( (a1)->host == (a2)->host && (a1)->port == (a2)->port )


typedef struct
{
	UDPpacket*	p;
	streamer_t* st;
	int type;
} pack_t;


void pack_init( pack_t* p, addr_t* addr, int type, int size );
void pack_deinit( pack_t* p );

#define pack_create( addr, type )				generic_create3( pack_t, pack_init, addr, type, MAX_PACKET_SIZE )
#define pack_destroy( p )						generic_destroy( p, pack_deinit )

void pack_setaddr( pack_t* p, addr_t* addr );
addr_t* pack_getaddr( pack_t* p );


typedef struct
{
	UDPsocket 			sock;
	SDLNet_SocketSet	sset;
	
	int bytes_recv;
	int bytes_sent;
} sock_t;

void sock_init( sock_t* s, int port );
void sock_deinit( sock_t* s );

pack_t* sock_receive( sock_t* s, int timeout, unsigned long long* wait_ts );
void sock_send( sock_t* s, pack_t* p );


#endif


#endif /*NET_H_*/
