#ifndef STREAMER_H_
#define STREAMER_H_

#include "../general.h"

typedef struct
{
	unsigned char* buff;
	int* len;
	
	unsigned char* pos;
	int maxlen;
} streamer_t;

void streamer_init( streamer_t* st, unsigned char* buff, int* len, int maxlen );
void streamer_deinit( streamer_t* st );

#define streamer_create( buff, len, maxlen )		generic_create3( streamer_t, streamer_init, buff, len, maxlen )
#define streamer_destroy( st )						generic_destroy( st, streamer_deinit )


char streamer_rdchar( streamer_t* st );
short streamer_rdshort( streamer_t* st );
int streamer_rdint( streamer_t* st );
void streamer_wrchar( streamer_t* st, char c );
void streamer_wrshort( streamer_t* st, short s );
void streamer_wrint( streamer_t* st, int i );

void streamer_copy( streamer_t* st_dst, streamer_t* st_src );

typedef struct
{
	unsigned char* buff;
	int len;
	int maxlen;
} buffer_t;


void buffer_init( buffer_t* b, int maxlen );
void buffer_deinit( buffer_t* b );

#define buffer_create( maxlen )						generic_create1( buffer_t, buffer_init, maxlen )
#define buffer_destroy( b )							generic_destroy( b, buffer_deinit )

streamer_t* buffer_getstreamer( buffer_t* b );


#endif /*STREAMER_H_*/
