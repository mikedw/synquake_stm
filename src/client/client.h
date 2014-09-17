#ifndef __CLIENT_H
#define __CLIENT_H

#include <SDL.h>
#include <SDL_thread.h>

extern "C" {
#include "../general.h"
#include "../comm/comm.h"
#include "../game/entity.h"
}

#include "PlayerAI.h"

typedef struct
{
	/* server */
	sock_t	s;
	addr_t server_addr;
	Uint32 disconnect_timeout;		/* time to wait for a server update (in ms) */
	Uint32 time_of_last_update;
	double last_update_interval;
	double average_update_interval;
	bool updated;
	
	int client_id;
	entity_t* pl;
	int state;						/* the state of the client (finite state machine) */
	rect_t view_r;					/* the area of the map that the player can see */
	char name[MAX_NAME_LEN];		/* player name */
	
	int last_action_sv;				/* counter for the last action confirmed by the server */ 
	int last_action;				/* counter for the last action proposed by the client AI */
	
	PlayerAI *ai;
	Uint32 think_time;
		
	int quest_active;				/* indicates the presence of a quest */
	rect_t quest_r;					/* location of the current quest */
	SDL_mutex *game_mutex;

	/* graphical interface */
	int has_GUI;
	int resx,resy,bpp;				/* screen size and bits per pixel */
	int full_screen;
	int render_delay;

	/* debug */
	int purpose;
	int debug_AI;
	float fps_average_param;		/* used for weighted average when computing frames per second */
} client_t;

extern client_t cl;


enum ClientStates
{
	INITIAL = 0,
	PLAYING,
	GONE
};

const char client_states[3][24] = {"Initial","Playing","Gone"};

#endif
