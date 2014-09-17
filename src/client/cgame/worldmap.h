#ifndef WORLDMAP_H_
#define WORLDMAP_H_

#include "../../utils/streamer.h"

#include "area_node.h"

typedef struct
{
	int*			n_entities;		// n_entities per entity type
	entity_t***		entities;		// array of entity_t* per entity type
	int*			et_ratios;		// array of ratios of the map covered by entities per entity type
	
	rect_t			map_r;
	rect_t			map_walls[N_DIRS];
	vect_t			size;
	int				area;
	
	area_node_t* 	area_tree;
} worldmap_t;

extern worldmap_t wm;


void worldmap_init( conf_t* c );

int worldmap_add( entity_t* ent );
void worldmap_del( entity_t* ent );
void worldmap_move( entity_t* ent, vect_t* move_v );

int worldmap_is_vacant_from_fixed( rect_t* r );
int worldmap_is_vacant_from_mobile( rect_t* r );
int worldmap_is_vacant( rect_t* r );

pentity_set_t* worldmap_get_entities( rect_t* range, int etypes );

void worldmap_unpack( streamer_t* st );
void worldmap_unpack_fixed( streamer_t* st );




#endif /*WORLDMAP_H_*/


