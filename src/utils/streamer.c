#include "malloc.h"
#include "streamer.h"

#include <arpa/inet.h>

void streamer_init( streamer_t* st, unsigned char* buff, int* len, int maxlen )
{
	assert( st && buff && len && maxlen > 0 );
	st->buff = buff;
	st->len = len;
	st->maxlen = maxlen;
	st->pos = buff;
	*st->len = 0;
}

void streamer_deinit( streamer_t* st )
{
	assert( st );
	st->buff = NULL;
	st->len = NULL;
	st->maxlen = 0;
	st->pos = NULL;
}


char streamer_rdchar( streamer_t* st )
{
	assert( st && st->pos + sizeof(char) <= st->buff + *st->len );
	return *(st->pos++);
}

short streamer_rdshort( streamer_t* st )
{
	short s;
	assert( st && st->pos + sizeof(short) <= st->buff + *st->len );
	memcpy( (char*)&s, st->pos, sizeof(short) );
	st->pos += sizeof(short);
	return ntohs( s );
}

int streamer_rdint( streamer_t* st )
{
	int i;
	assert( st && st->pos + sizeof(int) <= st->buff + *st->len );
	memcpy( (char*)&i, st->pos, sizeof(int) );
	st->pos += sizeof(int);
	return ntohl( i );
}



void streamer_wrchar( streamer_t* st, char c )
{
	assert( st && st->pos + sizeof(char) <= st->buff + st->maxlen );
	*(st->pos++) = c;
	(*st->len) ++;
}

void streamer_wrshort( streamer_t* st, short s )
{
	assert( st && st->pos + sizeof(short) <= st->buff + st->maxlen );
	s = htons( s );
	memcpy( st->pos, (char*)&s, sizeof(short) );
	st->pos += sizeof(short);
	(*st->len) += sizeof(short);
}

void streamer_wrint( streamer_t* st, int i )
{
	assert( st && st->pos + sizeof(int) <= st->buff + st->maxlen );
	i = htonl( i );
	memcpy( st->pos, (char*)&i, sizeof(int) );
	st->pos += sizeof(int);
	(*st->len) += sizeof(int);
}

void streamer_copy( streamer_t* st_dst, streamer_t* st_src )
{
	assert( st_dst && st_src && st_dst->maxlen - (*st_dst->len) >= (*st_src->len) );
	memcpy( st_dst->pos, st_src->buff, *st_src->len );
	st_dst->pos += (*st_src->len);
	(*st_dst->len) += (*st_src->len);
}



void buffer_init( buffer_t* b, int maxlen )
{
	assert( b );
	b->buff = (unsigned char *)malloc_wrapper( maxlen );	assert( b->buff );
	b->len = 0;
	b->maxlen = maxlen;
}
void buffer_deinit( buffer_t* b )
{
	assert( b && b->buff );
	free_wrapper( b->buff );
}

streamer_t* buffer_getstreamer( buffer_t* b )
{
	return streamer_create( b->buff, &b->len, b->maxlen );
}

