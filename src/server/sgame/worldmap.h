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
	int			area;

	area_node_t* 	area_tree;
	int		depth;
} worldmap_t;

extern worldmap_t wm;


void worldmap_init( coord_t szx, coord_t szy, int tdepth );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
int worldmap_add( entity_t* ent );

void worldmap_del( entity_t* ent );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void worldmap_move( entity_t* ent, vect_t* move_v );

int worldmap_is_vacant_from_fixed( rect_t* r );
int worldmap_is_vacant_from_mobile( rect_t* r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int worldmap_is_vacant( rect_t* r );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
entity_t* worldmap_generate_ent(int ent_type, rect_t* where );

#ifdef RUN_TRACES
entity_t* worldmap_generate_ent_from_trace(int ent_type );
#endif
void worldmap_generate();

parea_node_set_t* worldmap_get_nodes( rect_t* range );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
parea_node_set_t* worldmap_get_leaves( rect_t* range );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
parea_node_set_t* worldmap_get_parents( rect_t* range );

void worldmap_pack( streamer_t* st, rect_t* range );
void worldmap_pack_fixed( streamer_t* st );

void worldmap_is_valid();
void worldmap_print( int verbose );


//         OBSOLETE
pentity_set_t* worldmap_get_entities( rect_t* range, int etypes );


#endif /*WORLDMAP_H_*/


