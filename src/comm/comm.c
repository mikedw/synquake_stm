#include "comm.h"

#ifdef USE_SDL_NET


void addr_init( addr_t* a, int host, int port )
{
	a->host = host;
	a->port = port;
}



void pack_init( pack_t* pck, addr_t* addr, int type, int size )
{
	assert( pck && size > 0 && size <= MAX_PACKET_SIZE );
	pck->p = SDLNet_AllocPacket( size );	assert( pck->p );
	pck->p->len = 0;
	pck->st = streamer_create( pck->p->data, &pck->p->len, pck->p->maxlen );

	if( addr )
	{
		pack_setaddr( pck, addr );
		streamer_wrint( pck->st, type);
	}
}

void pack_deinit( pack_t* pck )
{
	assert( pck && pck->p && pck->st );
	SDLNet_FreePacket( pck->p );
	streamer_destroy( pck->st );
}

void pack_setaddr( pack_t* pck, addr_t* addr )
{
	pck->p->address.host = addr->host;
	pck->p->address.port = addr->port;
}

addr_t* pack_getaddr( pack_t* pck )
{
	return &pck->p->address;
}





void sock_init( sock_t* s, int port )
{
	s->sset = SDLNet_AllocSocketSet(1);
	s->sock = SDLNet_UDP_Open( port ); assert( s->sock );
	int rez = SDLNet_UDP_AddSocket(  s->sset, s->sock);	assert( rez != -1 );
	s->bytes_recv = 0;
	s->bytes_sent = 0;
}

void sock_deinit( sock_t* s )
{
	SDLNet_UDP_Close( s->sock );
}

// wait for "timeout" milliseconds to receive a packet on socket "s"
pack_t* sock_receive( sock_t* s, int timeout, unsigned long long* wait_ts )
{
	int res_cs, res_ur;
	assert( s );

	timeout = timeout < 0 ? 0 : timeout;
	res_cs = SDLNet_CheckSockets( s->sset, timeout );	assert( res_cs != -1 );
	if( wait_ts )		*wait_ts = get_t();
	if( !res_cs )		return NULL;			// no_activity

	pack_t* pck = pack_create( NULL, -1 );
	res_ur = SDLNet_UDP_Recv( s->sock, pck->p );
	if( res_ur == -1 ){printf("[E] Error receiving UDP packet");pack_destroy(pck);return NULL;}
	if( res_ur ==  0 ){printf("[W] No UDP packet received on an active port");pack_destroy(pck);return NULL;}
	s->bytes_recv += pck->p->len;

	pck->type = streamer_rdint( pck->st );
	return pck;
}

void sock_send( sock_t* s, pack_t* pck )
{
	int res_us;
	assert( s && pck );

	res_us = SDLNet_UDP_Send( s->sock, -1, pck->p );
	if( res_us == 0 )	printf("[E] Error sending UDP packet\n");

	s->bytes_sent += ( pck->p->status > 0 ? pck->p->status : 0 );
	pack_destroy(pck);
}

#endif


