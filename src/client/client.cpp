#include "client.h"
#include "client_action.h"
#include "client_render.h"


extern "C" {
#include "../game/action.h"

#include "cgame/worldmap.h"
}

client_t cl;

void client_init( int argc, char *argv[] )
{
	srand((unsigned int)(time(NULL)));
	
	sock_init( &cl.s, 0 );
	cl.disconnect_timeout = DEFAULT_DISCONNECT_TIMEOUT;
	cl.time_of_last_update = SDL_GetTicks();
	cl.average_update_interval = -1;
	cl.updated = false;
	
	cl.state = INITIAL;
	cl.pl = NULL;
	cl.client_id = -1;
	strcpy( cl.name, "Player" );
	
	cl.last_action = -1;
	cl.last_action_sv = -1;
	
	cl.quest_active = 0;
	cl.game_mutex = SDL_CreateMutex();	assert( cl.game_mutex );
		
	cl.has_GUI = 0;
	cl.resx = 512;
	cl.resy = 384;
	cl.bpp = 32;
	cl.full_screen = 0;
	
	cl.debug_AI = 0;
	cl.purpose = BASIC;
	cl.ai = new PlayerAI();	assert( cl.ai );
	
	cl.fps_average_param = 0.9;
	
	if ( argc < 2 )	pexit1( "usage: %s [options] <server_name>\n", argv[0] );

	/* get other parameters */
	int i;
	for ( i = 1; i < argc-1; i++ )
	{
		if ( !strcmp(argv[i], "--gui") ){			cl.has_GUI = 1;			continue;		}
		if ( !strcmp(argv[i], "--nogui") ){			cl.has_GUI = 0;			continue;		}
		if ( !strcmp(argv[i], "--fullscreen") ){	cl.full_screen = 1;		continue;		}
		if ( !strcmp(argv[i], "--debug-AI") ){		cl.debug_AI = 0;		continue;		}
		if ( !strncmp(argv[i], "--fps_average_param=", 20 ) )
		{	sscanf(argv[i]+20, "%f", &cl.fps_average_param);				continue;		}
		
		if ( !strncmp(argv[i], "--size=", 7) )
		{
			sscanf(argv[i]+7, "%dx%d", &cl.resx, &cl.resy);
			assert( cl.resx >= 120 && cl.resy >= 90  );
			continue;
		}
		if ( !strncmp(argv[i], "--bpp=", 6 ) )
		{
			sscanf(argv[i]+6, "%d", &cl.bpp);
			assert( cl.bpp == 1 ||   cl.bpp == 8 ||  cl.bpp == 16 ||  cl.bpp == 32 );
			continue;
		}
		printf("[WARNING] Unknown parameter '%s'\n", argv[i]);
	}

	conf_t* c = conf_create();	assert(c);
	conf_parse_file( c, "./config/default.cfg" );
	
	cl.disconnect_timeout = conf_get_int( c, "client.disconnect_timeout" );
	cl.think_time = conf_get_int( c, "client.think_time" );
	cl.render_delay = conf_get_int( c, "client.render_delay" );
	assert( cl.disconnect_timeout > 0 && cl.think_time > 0 && cl.render_delay > 0 );
	
	/* initialize world */
	actions_init( c );
	entity_types_init( c );
	worldmap_init( c );
	
	/* get server name and port */
	char* server_name = strdup(argv[argc-1]);
	int server_port	= conf_get_int( c, "server.port" );
	printf("Connecting to server %s:%d\n", server_name, server_port);
	/* use DNS to get server ip address */
	SDLNet_ResolveHost(&cl.server_addr,	server_name, server_port);
}




int main(int argc, char *argv[])
{
	/* initialize SDL */
	if ( SDL_Init( SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE ) < 0 )
	{	printf("Could not initialize SDL: %s.\n", SDL_GetError());		return -1;	}
	if ( SDLNet_Init() < 0 )
	{	printf("Failed to initialize SDL_net (SDLNet_Init)");			return -1;	}

	client_init(argc, argv);
	if( !client_action_join() )	return 0;

	/* CLIENT ACTIONS */
	SDL_Thread * cl_act_th = SDL_CreateThread( client_action_main, NULL);assert( cl_act_th );

	/* RENDER */
	if( cl.has_GUI )	client_render_main();
	
	SDL_WaitThread( cl_act_th, NULL);

	/* finish SDL */
	SDLNet_Quit();
	SDL_Quit();

	return 0;
}

