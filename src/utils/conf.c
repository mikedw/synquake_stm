#include "../general.h"
#include "conf.h"

//	Configurator
//
//	Used for storing (name,value) pairs from the configuration file



void conf_pair_init( conf_pair_t* cp, const char* _name, const char* _value )
{
	assert( cp && _name );
	elem_init( &cp->e, str_hash( _name ) );
	cp->name = strdup( _name );
	if( _value )	cp->value = strdup( _value );
	else			cp->value = NULL;
}

void conf_pair_deinit( conf_pair_t* cp )
{
	assert(cp);
	if( cp->name )	free_wrapper( cp->name );
	if( cp->value )	free_wrapper( cp->value );
}




void conf_parse_file( conf_t* cfg, const char *ini_file )
{
	FILE *f;
	int line = 0;
	char buffer[MAX_FILE_READ_BUFFER],*name,*value;

	assert( ini_file );
	f = fopen(ini_file, "r");	assert( f );

	while ( fgets(buffer, MAX_FILE_READ_BUFFER, f) != NULL )
	{
		line++;

		if ( buffer[0] == 10  || buffer[0] == 13 )	continue;
		if ( buffer[0] == '#' || buffer[0] == '[' )	continue;

		name = buffer;
		value = strchr(buffer, '=');
		if( value == NULL ){	printf("Error parsing line %s:%d (no equal sign)\n", ini_file, line);	continue;	}
		*value = 0;
		value++;

		name  = str_strip( name );
		value = str_strip( value );
		if( *name == 0 ){		printf("Error parsing file %s:%d (no name)\n", ini_file, line);			continue;	}
		if( *value == 0 ){		printf("Error parsing file %s:%d (no value)\n", ini_file, line);		continue;	}
		
		/* DEBUG */
		//printf("'%s' -> '%s'\n", name, value);
		
		conf_add_pair( cfg, name, value );
	}

	fclose(f);
}

char* conf_get_string( conf_t* cfg, const char *name )
{
	conf_pair_t*  cp = conf_find_pair( cfg, name );
	assert( cp && cp->value );
	return cp->value;
}

int conf_get_int( conf_t* cfg, const char *name )
{
	int x = 0;
	char* value = conf_get_string( cfg, name );
	if( value ) sscanf(value, "%d", &x);
	return x;
}

short conf_get_short( conf_t* cfg, const char *name )
{
	short x = 0;
	char* value = conf_get_string( cfg, name );
	if( value ) sscanf(value, "%hd", &x);
	return x;
}

float conf_get_float( conf_t* cfg, const char *name )
{
	float x = 0;
	char *value = conf_get_string( cfg, name );
	if( value ) sscanf(value, "%f", &x);
	return x;
}
