#include "../utils/malloc.h"
#include "entity.h"

int				n_entity_types = 0;
entity_type_t**	entity_types = NULL;



/*		ATTRIBUTE_TYPE		*/

void attribute_type_init( attribute_type_t* attr_type, int _attr_t_id, char* _name, value_t _min, value_t _max )
{
	assert( attr_type && _attr_t_id >= 0 && _name && strlen(_name) < MAX_NAME_LEN && _min <= _max );
	attr_type->attr_t_id = _attr_t_id;
	strcpy( attr_type->name, _name );
	attr_type->min = _min;
	attr_type->max = _max;
}

value_t attribute_type_adjust_value( int ent_t_id, int attr_id, value_t attr_val )
{
	assert( ent_t_id >= 0 && attr_id < entity_types[ent_t_id]->n_attr );
	attribute_type_t* attr_type = &entity_types[ent_t_id]->attr_types[attr_id];
	
	if( attr_type->max < attr_val )		return attr_type->max;
	if( attr_val < attr_type->min )		return attr_type->min;
	return attr_val;
}


/*		ENTITY_TYPE		*/

void entity_type_init( entity_type_t* ent_type, int _ent_t_id, char* _name, int _ratio, int _fixed )
{
	assert( ent_type && _ent_t_id >= 0 && _name && strlen(_name) < MAX_NAME_LEN && _ratio >= 0 );
	ent_type->ent_t_id = _ent_t_id;
	strcpy( ent_type->name, _name );
	ent_type->n_attr = 0;
	ent_type->ratio = _ratio;
	ent_type->pl_ratio = 0;
	ent_type->fixed = _fixed;
}

void entity_type_add_attribute( entity_type_t* ent_type, char* _name, value_t _min, value_t _max )
{
	assert( ent_type && ent_type->n_attr < MAX_ATTRIBUTES );
	int i = ent_type->n_attr;
	attribute_type_init( &ent_type->attr_types[i], i, _name, _min, _max );
	ent_type->n_attr++;
}

attribute_type_t* entity_type_get_attribute_type( entity_type_t* ent_type, char* _name )
{
	assert( ent_type && _name );
	int i;
	for( i = 0; i < ent_type->n_attr; i++ )
		if( !strcmp( ent_type->attr_types[i].name, _name ) )
			return &ent_type->attr_types[i];
	return NULL;
}

void entity_types_init( conf_t* c )
{
	int i, j;
	char nm[MAX_FILE_READ_BUFFER];

	n_entity_types = conf_get_int( c, "n_entity_types" );
	assert( n_entity_types > 0 && n_entity_types <= MAX_ENTITY_TYPES );
	entity_types = (entity_type_t **) calloc_wrapper( n_entity_types, sizeof( entity_type_t* ) );

	for( i = 0; i < n_entity_types; i++ )
	{
		int n_attr, ratio, fixed;
		char* name;

		name   = conf_get_string( c, spf1( nm, "entity_type[%d].name", i ) );
		ratio  = conf_get_int( c, spf1( nm, "entity_type[%d].ratio", i ) );
		fixed  = conf_get_int( c, spf1( nm, "entity_type[%d].fixed", i ) );
		n_attr = conf_get_int( c, spf1( nm, "entity_type[%d].n_attr", i ) );

		assert( n_attr > 0 && n_attr <= MAX_ATTRIBUTES );
		entity_types[i] = (entity_type_t *) malloc_wrapper( sizeof( entity_type_t ) + n_attr * sizeof(attribute_type_t) );
		entity_type_init( entity_types[i], i, name, ratio, fixed );

		for( j = 0; j < n_attr; j++ )
		{
			value_t min, max;

			name = conf_get_string( c, spf2( nm, "entity_type[%d].attribute_type[%d].name", i, j ) );
			min  = conf_get_int( c, spf2( nm, "entity_type[%d].attribute_type[%d].min", i, j ) );
			max  = conf_get_int( c, spf2( nm, "entity_type[%d].attribute_type[%d].max", i, j ) );

			entity_type_add_attribute( entity_types[i], name, min, max );
		}
	}
}



/*		ENTITY		*/
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void entity_init( entity_t* ent, int _ent_type )
{
	assert( ent && _ent_type >= 0 && _ent_type < n_entity_types );
	elem_init( &ent->e, 0 );
//	ent->mutex = mutex_create();	assert( ent->mutex );
	ent->ent_id =  -1;
	ent->ent_type = _ent_type;
}

entity_t* entity_create( int _ent_type )
{
	assert( _ent_type >= 0 && _ent_type < n_entity_types );
	//mike: !!!!! comment out for now
	entity_t* ent = (entity_t*) malloc_wrapper( sizeof( entity_t ) + entity_types[_ent_type]->n_attr * sizeof(value_t) );
	//mike: !!!!! I added this
	//entity_t* ent = NULL;

	entity_init( ent, _ent_type );
	return ent;
}

void entity_deinit( entity_t* ent )
{
//	assert( ent && ent->mutex );
//	mutex_destroy( ent->mutex );
}

void entity_destroy( entity_t* ent )
{
	assert( ent );
	entity_deinit( ent );
	free_wrapper( ent );
}

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void entity_generate_attrs( entity_t* ent )
{
	int i;
	entity_type_t* et = entity_types[ ent->ent_type];
	assert( et->n_attr > 0 );
	for (i = 0; i < et->n_attr; ++i)
		ent->attrs[i] = attribute_type_rand( &et->attr_types[i] );
}

void entity_generate_position( entity_t* ent, rect_t* map_r )
{
	rect_generate_position( &ent->r, &ent->size, map_r );
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int entity_position_is_valid( entity_t* ent, vect_t* map_sz )
{
	vect_t aux;
	if( !vect_is_positive( &ent->size ) )	return 0;
	if( !rect_is_positive( &ent->r ) )		return 0;
	if( !rect_is_valid( &ent->r ) )			return 0;

	vect_substract( map_sz, &ent->r.v2, &aux );
	if( !vect_is_positive( &aux ) )			return 0;

	vect_substract( &ent->r.v2, &ent->r.v1, &aux );
	if( !vect_is_eq( &ent->size, &aux ) )	return 0;
	return 1;
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int entity_attrs_is_valid( entity_t* ent )
{
	int et_id = ent->ent_type;
	int i, n_attr = entity_types[ et_id ]->n_attr;
	for( i = 0; i < n_attr; i++ )
	{
		if( entity_types[et_id]->attr_types[i].min > ent->attrs[i] )		return 0;
		if( entity_types[et_id]->attr_types[i].max < ent->attrs[i] )		return 0;
	}
	return 1;
}

int entity_is_valid( entity_t* ent, vect_t* map_sz )
{
	if( !entity_position_is_valid( ent, map_sz ) )		return 0;
	if( !entity_attrs_is_valid( ent ) )					return 0;
	return 1;
}

void entity_set_attr( entity_t* ent, int attr_id, value_t attr_val )
{
	assert( ent && attr_id < entity_types[ent->ent_type]->n_attr );
	ent->attrs[attr_id ] = attribute_type_adjust_value( ent->ent_type, attr_id, attr_val );
}



void entity_pack( entity_t* ent, streamer_t* st )
{
	streamer_wrint( st, ent->ent_type );
	streamer_wrint( st, ent->ent_id );
	rect_pack( &ent->r, st );

	assert( sizeof(value_t) == sizeof(int));
	int i, n_attr = entity_types[ ent->ent_type ]->n_attr;
	for(i = 0; i < n_attr; i++ )
		streamer_wrint( st, ent->attrs[i] );
}

void entity_unpack( entity_t* ent, streamer_t* st )
{
	rect_unpack( &ent->r, st );
	vect_substract( &ent->r.v2, &ent->r.v1, &ent->size );

	assert( sizeof(value_t) == sizeof(int));
	int i, n_attr = entity_types[ ent->ent_type ]->n_attr;
	for( i = 0; i < n_attr; i++ )
		ent->attrs[i] = streamer_rdint( st );
}





void entity_pack_fixed( entity_t* ent, streamer_t* st )
{
	rect_pack( &ent->r, st );
}

void entity_unpack_fixed( entity_t* ent, streamer_t* st )
{
	rect_unpack( &ent->r, st );
	vect_substract( &ent->r.v2, &ent->r.v1, &ent->size );
}





