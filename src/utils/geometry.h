#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include "../utils/streamer.h"

#define		DIR_UP		0
#define		DIR_RIGHT	1
#define		DIR_DOWN	2
#define		DIR_LEFT	3
#define		N_DIRS		4

extern char dir_names[4][10];


typedef	int coord_t;

#define	coord_pack( c, st )		streamer_wrshort( st , c )
#define	coord_unpack( st )		streamer_rdshort( st )

typedef struct
{
	coord_t x;
	coord_t y;
} vect_t;

extern vect_t dirs[4];

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif

vect_t* vect_init( vect_t* v, coord_t _x, coord_t _y );
#define vect_create( _x, _y )						generic_create2( vect_t, vect_init, _x, _y )
#define vect_destroy( v )							generic_free( v )

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif

void vect_add( vect_t* v1, vect_t* v2, vect_t* rez );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void vect_substract( vect_t* v1, vect_t* v2, vect_t* rez );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
void vect_scale( vect_t* v, double fx, vect_t* rez );

int vect_distance( vect_t* v1, vect_t* v2);
int vect_distance_x( vect_t* v1, vect_t* v2);
int vect_distance_y( vect_t* v1, vect_t* v2);
int vect_direction( vect_t* src_v, vect_t* dest_v );

#define vect_is_positive( v )						( (v)->x >= 0 && (v)->y >=0 )
#define vect_is_less( v1, v2 )						( (v1)->x <= (v2)->x && (v1)->y <= (v2)->y )
#define vect_is_eq( v1, v2 )						( (v1)->x == (v2)->x && (v1)->y == (v2)->y )

void vect_pack( vect_t* v, streamer_t* st );
void vect_unpack( vect_t* v, streamer_t* st );



typedef struct
{
	vect_t v1, v2;		/* v1 is the lower left corner (inclusive); v2 is the upper right corner (exclusive) */ 
} rect_t;


rect_t* rect_init(  rect_t* r, vect_t _v1, vect_t _v2 );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif

rect_t* rect_init4( rect_t* r, coord_t _x1, coord_t _y1, coord_t _x2, coord_t _y2 );

#define rect_create( _v1, _v2 )						generic_create2( rect_t, rect_init, _v1, _v2 )
#define rect_create4( _x1, _y1, _x2, _y2 )			generic_create4( rect_t, rect_init4, _x1, _y1, _x2, _y2 )

#define rect_is_positive( r )						( vect_is_positive( &(r)->v1 ) && vect_is_positive( &(r)->v2 ) )
#define rect_is_valid( r )							vect_is_less( &(r)->v1, &(r)->v2 )

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif

void rect_generate_position( rect_t* r, vect_t* sz, rect_t* map_r );
void rect_generate_overlapping( rect_t* r, rect_t* overlap_r, vect_t* sz, vect_t* map_sz );
void rect_generate_nextto( rect_t* r, rect_t* nextto_r, vect_t* sz, vect_t* map_sz );

unsigned int rect_perimeter( rect_t* r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
unsigned int rect_area( rect_t* r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int rect_distance( rect_t* r1, rect_t* r2 );

void rect_normalize( rect_t* r );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
int rect_crop( rect_t* r, rect_t* constraint_r, rect_t* crop_r );
#endif
void rect_split( rect_t* r, int split_dir, rect_t* r_left, rect_t* r_right, rect_t* r_split );

void rect_expand( rect_t* r, double* fx, rect_t* r_expand );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int rect_cmp( rect_t* r1, rect_t* r2 );

int rect_quadrant(rect_t* r1, rect_t* r2);

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int rect_is_overlapping( rect_t* r1, rect_t* r2 );
int rect_is_overlapping_any( rect_t* r, rect_t* ranges, int n_ranges );
int rect_is_nextto( rect_t* r1, rect_t* r2 );

#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
int rect_is_contained( rect_t* r, rect_t* cont_r );

int rect_can_fit_inside( rect_t* r, rect_t* cont_r );
int rect_is_outside(rect_t* in, rect_t* out);

void rect_pack( rect_t* r, streamer_t* st );
void rect_unpack( rect_t* r, streamer_t* st );



#endif /*GEOMETRY_H_*/
