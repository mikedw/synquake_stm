#include "client.h"
#include "client_render.h"

#include "../graphics/Graphics.h"
#include "../graphics/OglObject.h"
#include "../graphics/OglWorld.h"
#include "../graphics/font/BitmapFont.h"

#define __USE_3DS_GRAPHICS__

#ifdef __USE_3DS_GRAPHICS__
#include "../graphics/3ds/Ogl3dsObject.h"
#else
#include "../graphics/vrml/OglVrmlObject.h"
#endif


extern "C" {
#include "../general.h"
#include "../game/game.h"

#include "cgame/worldmap.h"
}


OglWorld w;
OglObject *player_model, *apple_model, *wall_model, *floor_model;
BitmapFont *font;

MovementData pmd;			/* movement data for main player */
MovementData *md1,*md2;		/* movement data for other players (two vectors) */
int nmd,nmd2;				/* number of players with movement data (+ for second vector) */

float frame_render_interval;


void client_render_init()
{
	frame_render_interval = 1;

	md1 = new MovementData[MAX_ENTITIES];
	md2 = new MovementData[MAX_ENTITIES];
	assert( md1 != NULL && md2 != NULL );
	nmd = nmd2 = 0;

	graphics_init();
}

void client_render_main()
{
	Uint32 start;
	int interval;

	client_render_init();
	printf("ClientRenderModule started\n");

	while ( cl.state == PLAYING )
	{
		/* render frame */
		start = SDL_GetTicks();
		renderSingleFrame();

		/* handle user actions */
		if ( w.checkMessages() ){	cl.state = GONE;	break;		}

		/* limit frames per second */
		interval = SDL_GetTicks() - start;
		assert( interval > 0 && interval < 100000 );
		if( cl.render_delay > interval ) SDL_Delay( cl.render_delay - interval );

		/* compute frames per second average */
		frame_render_interval = interval * ( 1 - cl.fps_average_param )	+ frame_render_interval * cl.fps_average_param;
	}

	client_render_quit();
}

void client_render_quit()
{
	graphics_quit();

	if ( md1 != NULL ) delete md1;
	if ( md2 != NULL ) delete md2;
}

/***************************************************************************************************
*
* Graphics init/end
*
***************************************************************************************************/

void graphics_init()
{
	if( SDL_InitSubSystem(SDL_INIT_VIDEO) )
	{	printf("[SDL] %s\n", SDL_GetError());	assert( !"Cannot initialize SDL graphics" );	}

	/* set icon */
	SDL_Surface *icon = SDL_LoadBMP("data/icon32.bmp");
	if ( icon )
	{
		SDL_SetColorKey( icon, SDL_RLEACCEL | SDL_SRCCOLORKEY, SDL_MapRGB( icon->format, 255,255,255 ) );
		SDL_WM_SetIcon(icon, NULL);
	} else printf("[W] Cannot find icon for this window\n");

	printf("Starting GUI %dx%d %s\n",	cl.resx, cl.resy, cl.full_screen ? "full screen":"");

	/* set up window */
	if ( cl.full_screen )		w.createFullScreen( cl.resx, cl.resy, cl.bpp);
	else						w.create( cl.resx, cl.resy, cl.bpp);
	SDL_WM_SetCaption("Client", NULL);

	/* load models */
	#ifdef __USE_3DS_GRAPHICS__
	wall_model = new Ogl3dsObject("data/wall.3ds");
	apple_model = new Ogl3dsObject("data/food.3ds");
	floor_model = new Ogl3dsObject("data/floor.3ds");
	player_model = new Ogl3dsObject("data/player.3ds");
	#else
	wall_model = new OglVrmlObject("data/wall.wrl");
	apple_model = new OglVrmlObject("data/food.wrl");
	floor_model = new OglVrmlObject("data/floor.wrl");
	player_model = new OglVrmlObject("data/player.wrl");
	#endif
	assert( player_model && apple_model && wall_model && floor_model );

	/* load font */
	font = new BitmapFont("data/mainfont.bmp");		assert( font );
}

void graphics_quit()
{
	delete player_model;
	delete apple_model;
	delete wall_model;
	delete floor_model;
	delete font;
	w.destroy();
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/***************************************************************************************************
*
* Rendering
*
***************************************************************************************************/

void updatePosition(MovementData *md, int px, int py)
{
	if ( px != md->arrivalx || py != md->arrivaly )
	{
		/* current time and position */
		md->lastx = md->arrivalx;
		md->lasty = md->arrivaly;
		md->last_time = SDL_GetTicks();
		/* new time and position */
		md->arrivalx = px;
		md->arrivaly = py;
		md->arrival_time = md->last_time + cl.think_time;
		/* the player should reach the destination in cl.think_time miliseconds */
	}
	Uint32 current_time = SDL_GetTicks();
	if ( current_time >= md->arrival_time )
	{
		/* if the time interval expired */
		md->pfx = px;
		md->pfy = py;
	} else {
		/* the player is on his way to the arrival coordinates */
		float r = (float)( current_time - md->last_time ) / (float)cl.think_time;
		md->pfx = md->lastx + r * ( md->arrivalx - md->lastx );
		md->pfy = md->lasty + r * ( md->arrivaly - md->lasty );
	}
}

void updatePositionForPlayer(int id, int px, int py, float *pfx, float *pfy)
{
	*pfx = px;
	*pfy = py;

	MovementData *md = NULL;

	/* get movement data */
	for ( int i = 0; i < nmd; i++ )
		if ( id == md1[i].id )
		{
			md = md1 + i;
			break;
		}

	/* add new entry in vector if this player is not present */
	if ( md == NULL )
	{
		md = md2 + nmd2;
		md->id = id;
		*pfx = md->pfx = md->lastx = md->arrivalx = px;
		*pfy = md->pfy = md->lasty = md->arrivaly = py;
		md->arrival_time = md->last_time = SDL_GetTicks();
		nmd2++;
		return;
	}

	/* store movement data in new vector */
	memcpy(md2 + nmd2, md, sizeof(MovementData));
	md = md2 + nmd2;
	nmd2++;

	/* update movement data */
	updatePosition(md, px,py);
	*pfx = md->pfx;
	*pfy = md->pfy;
}

void updateMovementDataVectors()
{
	/* interchange vectors */
	MovementData *md_aux;
	md_aux = md1;
	md1 = md2;
	md2 = md_aux;

	/* reset size */
	nmd = nmd2;
	nmd2 = 0;
}

/***************************************************************************************************
*
* Rendering
*
***************************************************************************************************/

void render_entity( entity_t* ent )
{
	int i, j;
	coord_t x = ent->r.v1.x;
	coord_t y = ent->r.v1.y;

	if( ent->ent_type == ET_WALL )
	{
		for( i = 0; i < ent->size.x; i++ )
			for( j = 0; j < ent->size.y; j++ )
				wall_model->render( x+i,y+j );
	}
	if( ent->ent_type == ET_APPLE )
	{
		value_t food = ent->attrs[AP_FOOD];
		apple_model->render( x, y, (0.3 + 0.1 * food)*ent->size.x );
	}
	if( ent->ent_type == ET_PLAYER )
	{
		value_t dir = ent->attrs[PL_DIR];
		value_t life = ent->attrs[PL_LIFE];
		value_t hit  = ent->attrs[PL_HIT];
		float pfx2,pfy2;		/* interpolated player position */

		updatePositionForPlayer( ent->ent_id, x, y, &pfx2, &pfy2 );
		if( !hit )	player_model->render( pfx2, pfy2, (0.5 + 0.7*life/100)*ent->size.x, dir*90 );
		else		player_model->render( pfx2, pfy2, (0.5 + 0.7*life/100)*ent->size.x,-dir*90, -90);
	}
}

void renderSingleFrame()
{
	coord_t px,py;			/* player position */
	value_t life;
	float pfx,pfy;			/* interpolated player position (for main player) */

	if( !cl.updated )		return;

	SDL_LockMutex(cl.game_mutex);

	px = cl.pl->r.v1.x;
	py = cl.pl->r.v1.y;
	life = cl.pl->attrs[PL_LIFE];

	/* get new player position */
	updatePosition(&pmd, px,py);
	pfx = pmd.pfx;
	pfy = pmd.pfy;

	/* compute render range */
	vect_init( &cl.pl->r.v1, (coord_t)pfx, (coord_t)pfy );
	vect_add( &cl.pl->r.v1, &cl.pl->size, &cl.pl->r.v2 );
	rect_t render_r = game_action_range( AC_VIEW, cl.pl, &wm.map_r );
	vect_init( &cl.pl->r.v1, px, py );
	vect_add( &cl.pl->r.v1, &cl.pl->size, &cl.pl->r.v2 );

	/* begin rendering */
	w.beginRender();

	/* set camera and lights */  /* position */  /* look at */  /* up direction */
	action_range_t* ar = &action_ranges[AC_VIEW];
	gluLookAt( pfx-ar->front,1.5*ar->front,pfy-ar->front, 	pfx-0.1*ar->front,0.0,pfy-0.1*ar->front,	0,1,0);

	/* draw floor */
	int i, j;
	for ( i = render_r.v1.x; i <= render_r.v2.x; i++ )
		for ( j = render_r.v1.y; j <= render_r.v2.y; j++ )
			floor_model->render(i,j);

	/* draw entities */
	int et;
	for( et = n_entity_types-1; et >= 0; et-- )
	{
		pentity_set_t* pe_set = worldmap_get_entities( &render_r, 1 << et );
		elem_t* pos;
		pentity_set_for_each( pos, pe_set )
			render_entity( ((pentity_t*)pos)->ent );
		pentity_set_destroy( pe_set );
	}


	SDL_UnlockMutex(cl.game_mutex);


	/* get coordinates for the status area */
	SDL_Surface *screen = w.getScreen();
	SDL_Rect rect;
	rect.w = cl.resx;
	rect.h = 40;
	rect.x = 0;
	rect.y = cl.resy - rect.h;

	/* display status */
	SDL_FillRect( screen, &rect, SDL_MapRGB( screen->format, 0x00, 0x00, 0x00 ) );
	SDL_UpdateRect(screen, rect.x,rect.y, rect.w,rect.h);

	char sbuffer[256];
	SDL_WM_SetCaption( cl.name, NULL);
	if ( cl.state != PLAYING )		sprintf(sbuffer, "%s", client_states[cl.state]);
	else	sprintf(sbuffer, "playing -> %s ( coord = %d,%d life = %d )",	AI_state_names[cl.purpose],	px, py, life);
	font->render(rect.x + 9, rect.y + 2, sbuffer, screen);

	sprintf(sbuffer, "update interval: average = %.1lfms, last = %.1lfms, fps = %.1f",
		cl.average_update_interval, cl.last_update_interval, 1000.0 / frame_render_interval );
	font->render(rect.x + 9, rect.y + 21, sbuffer, screen);

	SDL_UpdateRect(screen, rect.x,rect.y, rect.w,rect.h);

	/* finish frame rendering */
	updateMovementDataVectors();
	w.endRender();
}
