#ifndef GAME_H_
#define GAME_H_

#include "entity.h"
#include "action.h"


#define		ET_PLAYER	0

#define		PL_SIZE		0
#define		PL_SPEED	1
#define		PL_DIR		2
#define		PL_LIFE		3
#define		PL_HIT		4

#define		ET_APPLE	1

#define		AP_SIZE		0
#define		AP_FOOD		1


#define		ET_WALL		2

#define		WL_SIZE_X	0
#define		WL_SIZE_Y	1

#define		AC_MOVE		0
#define		AC_EAT		1
#define		AC_ATTACK	2
#define		AC_VIEW		3

#define		QT_INACTIVE	0
#define		QT_ACTIVE	1
#define		QT_NEW		2

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
rect_t game_action_range2( int a_id, rect_t* loc, value_t speed, value_t dir, rect_t* map_r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
rect_t game_action_range( int a_id, entity_t* pl, rect_t* map_r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int    game_action_etypes( int a_id );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void game_entity_generate_size( entity_t* ent );
void game_entity_pack( entity_t* ent, streamer_t* st );
void game_entity_unpack( entity_t* ent, streamer_t* st );


#endif /*GAME_H_*/
