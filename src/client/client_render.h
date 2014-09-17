#ifndef __CLIENT_RENDER_H
#define __CLIENT_RENDER_H

#define MAX_PLAYER_MODELS 16

typedef struct
{
	int id;							/* id of the entity */
	float lastx,lasty;				/* last position */
	float pfx,pfy;					/* current position (interpolated) */
	float arrivalx,arrivaly;		/* next position */
	Uint32 arrival_time,last_time;	/* time for last and next position */
} MovementData;


void client_render_init();
void client_render_main();
void client_render_quit();


/* graphics setup */
void graphics_init();
void graphics_quit();

/* player movement */
void updatePosition(MovementData *md, int px, int py);
void updatePositionForPlayer(IPaddress id, int px, int py, float *pfx, float *pfy);
void updateMovementDataVectors();

/* rendering */
void renderSingleFrame();


#endif

