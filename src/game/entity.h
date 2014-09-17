#ifndef ENTITY_H_
#define ENTITY_H_

#include "../general.h"
#include "../utils/geometry.h"
#include "../utils/conf.h"
#include "../utils/hset.h"
#include "../utils/thread.h"
#include "../utils/streamer.h"



#define MAX_ENTITY_TYPES	32
#define	MAX_ATTRIBUTES		8
#define MAX_ENTITIES		400240
#define MAX_RATIO			1000


/*		ATTRIBUTE_TYPE		*/
typedef int value_t;

typedef struct
{
	int attr_t_id;
	char name[MAX_NAME_LEN];
	value_t min, max;				/* min/max values for this attribute */
} attribute_type_t;

void attribute_type_init( attribute_type_t* attr_type, int _attr_t_id, char* _name, value_t _min, value_t _max );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
value_t attribute_type_adjust_value( int ent_t_id, int attr_id, value_t attr_val );

#define attribute_type_rand( attr_type )			rand_range( ((attr_type)->min), ((attr_type)->max) )


/*		ENTITY_TYPE		*/

typedef struct
{
	int ent_t_id;
	char name[MAX_NAME_LEN];

	/* percentage of the map that entities of this type can occupy */
	/* ( used when randomly populating the map) */
	int ratio;
	/* = 1 if entities of this type have a fixed position in the game */
	int fixed;
	
	/* = N_ENTS / N_PLAYERS */
	int pl_ratio;

	/* attributes for this entity type */
	int n_attr;
	attribute_type_t attr_types[0];
} entity_type_t;

extern	int				n_entity_types;
extern	entity_type_t**	entity_types;


void entity_type_init( entity_type_t* ent_type, int _ent_t_id, char* _name, int _ratio, int _fixed );
void entity_type_add_attribute( entity_type_t* ent_type, char* _name, value_t _min, value_t _max );
attribute_type_t* entity_type_get_attribute_type( entity_type_t* ent_type, char* _name );


void entity_types_init( conf_t* c );



/*		ENTITY		*/
typedef struct
{
	elem_t e;					/* required for "hset" integration (needs to be the first member of the structure) */

	rect_t r;					/* position as defined by 2 oposing corners representing the bounding box */
	vect_t size;
//	mutex_t* 	mutex;			/* synchronization */

	int ent_type;				/* = ent_id of the entity_type corresponding to this entity */
	int ent_id;					/* ent_id within this entity's type	*/
	value_t attrs[0];				/* values of the attributes for this entity */
} entity_t;


void entity_init( entity_t* ent, int _ent_type );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void entity_deinit( entity_t* ent );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
entity_t* entity_create( int _ent_type );

#ifdef INTEL_TM
[[transaction_callable]]
#endif
void entity_destroy( entity_t* ent );

#define entity_is_eq( ent1, ent2 )			( ent1->ent_id == ent2->ent_id && ent1->ent_type == ent2->ent_type )

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void entity_generate_attrs( entity_t* ent );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void entity_generate_position( entity_t* ent, rect_t* map_r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int entity_is_valid( entity_t* ent, vect_t* map_sz );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void entity_set_attr( entity_t* ent, int attr_id, value_t attr_val );

void entity_pack_fixed( entity_t* ent, streamer_t* st );
void entity_unpack_fixed( entity_t* ent, streamer_t* st );

void entity_pack( entity_t* ent, streamer_t* st );
void entity_unpack( entity_t* ent, streamer_t* st );



/*		ENTITY SET		*/

#define	entity_set_t							hset_t


#define _entity_destroy( ent )					entity_destroy( ((entity_t*)ent) )

#define	entity_set_init( ent_s )				hset_init2( ent_s, 1, 1000000000 )
#define	entity_set_deinit( ent_s )				hset_deinit( ent_s, _entity_destroy )

#define	entity_set_create()						hset_create2( 1, 1000000000 )
#define	entity_set_destroy( ent_s )				hset_destroy( ent_s, _entity_destroy )

#define entity_set_add( ent_s, _ent )			hset_add( ent_s, _ent )

#define entity_set_del( ent_s, _ent )			hset_del( ent_s, _ent )

#define entity_set_for_each( pos, ent_s )		hset_key_for_each( pos, ent_s, 0 )

#define entity_set_size( ent_s )				hset_size( ent_s )





/*		ENTITY POINTER		*/

typedef struct
{
	elem_t e;					/* required for "hset" integration (needs to be the first member of the structure) */
	entity_t* ent;
} pentity_t;

inline static void pentity_init( pentity_t* pe, entity_t* ent )
{
	assert( pe && ent );
	elem_init( &pe->e, 0 );
	pe->ent = ent;
}

#define pentity_create( ent )					generic_create1( pentity_t, pentity_init, ent )
#define pentity_destroy( pent )					generic_free( pent )



/*		ENTITY POINTER SET		*/

#define	pentity_set_t							hset_t


#define	pentity_set_init( pent_s )				hset_init( pent_s )
#define	pentity_set_deinit( pent_s )			hset_deinit( pent_s, pentity_destroy )

#define	pentity_set_create()					hset_create2( 1, 1000000000 )
#define	pentity_set_destroy( pent_s )			hset_destroy( pent_s, pentity_destroy )

#define pentity_set_add( pent_s, pent )			hset_add( pent_s, pent )
#define pentity_set_del( pent_s, pent )			hset_del( pent_s, pent )

#define pentity_set_for_each( pos, pent_s )		hset_key_for_each( pos, pent_s, 0 )

#define pentity_set_size( pent_s )				hset_size( pent_s )


#endif /*ENTITY_H_*/
