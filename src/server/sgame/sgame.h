#ifndef SGAME_H_
#define SGAME_H_

#include "../../game/game.h"
#include "worldmap.h"
#include "../server.h"

void ANT_lock( area_node_t* at, rect_t* ranges, int n_ranges );
void ANT_unlock( area_node_t* at, rect_t* ranges, int n_ranges );

void game_action( int a_id, entity_t* pl, value_t pl_speed, value_t pl_dir );
void game_multiple_action( sv_client_t* cl );

#endif /*SGAME_H_*/
