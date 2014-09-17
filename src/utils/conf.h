#ifndef __CONF_H
#define __CONF_H

//	Configurator
//
//	Used for storing (name,value) pairs from the configuration file


#include "hset.h"

typedef struct
{
	elem_t e;		/* required for "hset" integration (needs to be the first member of the structure) */

	char* name;
	char* value;
} conf_pair_t;

#define	conf_pair_cmp( cp1, cp2 )				strcmp( (cp1)->name, (cp2)->name )
#define	conf_pair_eq( cp1, cp2 )				( conf_pair_cmp( cp1, cp2 ) == 0 )

void	conf_pair_init( conf_pair_t* cp, const char* _name, const char* _value );
void	conf_pair_deinit( conf_pair_t* cp );

#define conf_pair_create( _name, _value )		generic_create2( conf_pair_t, conf_pair_init, _name, _value )
#define conf_pair_destroy( cp )					generic_destroy( cp, conf_pair_deinit )

#define	_conf_pair_cmp( cp1, cp2 )				conf_pair_cmp( (conf_pair_t*)cp1, (conf_pair_t*)cp2 )
#define	_conf_pair_eq( cp1, cp2 )				conf_pair_eq( (conf_pair_t*)cp1, (conf_pair_t*)cp2 )
#define _conf_pair_destroy( cp )				conf_pair_destroy( (conf_pair_t*)cp )



#define	conf_t									hset_t

#define	conf_init( conf )						hset_init( conf )
#define	conf_deinit( conf )						hset_deinit( conf, _conf_pair_destroy )

#define	conf_create()							hset_create()
#define	conf_destroy( conf )					hset_destroy( conf, _conf_pair_destroy )

#define conf_add_pair( conf, _name, _value )	hset_add( conf, conf_pair_create( _name, _value ) )
#define conf_find_pair( conf, _name )											\
({																				\
	conf_pair_t* jig = conf_pair_create( _name, NULL );							\
	conf_pair_t* cp = (conf_pair_t*) hset_find( conf, jig, _conf_pair_eq );		\
	conf_pair_destroy( jig );													\
	cp;																			\
})



void	conf_parse_file( conf_t* cfg, const char* ini_file );

char*	conf_get_string( conf_t* cfg, const char *name );
int		conf_get_int( conf_t* cfg, const char *name );
short	conf_get_short( conf_t* cfg, const char *name );
float	conf_get_float( conf_t* cfg, const char *name );

#endif
