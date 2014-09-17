#include "geometry.h"


char dir_names[4][10] = { "UP", "RIGHT", "DOWN", "LEFT" };
vect_t dirs[4] = { {0,1}, {1,0}, {0,-1}, {-1,0} };


vect_t* vect_init( vect_t* v, coord_t _x, coord_t _y )
{
	v->x = _x;
	v->y = _y;
	return v;
}

void vect_add( vect_t* v1, vect_t* v2, vect_t* rez )
{
	rez->x = v1->x + v2->x;
	rez->y = v1->y + v2->y;
}

void vect_substract( vect_t* v1, vect_t* v2, vect_t* rez )
{
	rez->x = v1->x - v2->x;
	rez->y = v1->y - v2->y;
}

void vect_scale( vect_t* v, double fx, vect_t* rez )
{
	rez->x = (coord_t) (((double)v->x) * fx);
	rez->y = (coord_t) (((double)v->y) * fx);
}

int vect_distance( vect_t* v1, vect_t* v2)
{
	return abs(v2->x - v1->x) + abs(v2->y - v1->y);
}

int vect_distance_x( vect_t* v1, vect_t* v2)
{
	return abs(v2->x - v1->x);
}

int vect_distance_y( vect_t* v1, vect_t* v2)
{
	return abs(v2->y - v1->y);
}

int vect_direction( vect_t* src_v, vect_t* dest_v )
{
	vect_t dir_v;
	dir_v.x = dest_v->x - src_v->x;
	dir_v.y = dest_v->y - src_v->y;
	assert( dir_v.x * dir_v.y == 0 );
	if( dir_v.x > 0 )	dir_v.x =  1;if( dir_v.x < 0 )	dir_v.x = -1;
	if( dir_v.y > 0 )	dir_v.y =  1;if( dir_v.y < 0 )	dir_v.y = -1;

	int i;
	for( i = 0; i < N_DIRS; i++ )
		if( vect_is_eq( &dir_v, &dirs[i] ) )	break;
	assert( i<N_DIRS );

	return i;
}

void vect_pack( vect_t* v, streamer_t* st )
{
	coord_pack( v->x, st );
	coord_pack( v->y, st );
}

void vect_unpack( vect_t* v, streamer_t* st )
{
	v->x = coord_unpack( st );
	v->y = coord_unpack( st );
}





rect_t* rect_init(  rect_t* r, vect_t _v1, vect_t _v2 )
{
	assert( _v1.x <= _v2.x && _v1.y <= _v2.y );
	r->v1 = _v1;
	r->v2 = _v2;
	return r;
}

rect_t* rect_init4( rect_t* r, coord_t _x1, coord_t _y1, coord_t _x2, coord_t _y2 )
{
	assert( _x1 <= _x2 && _y1 <= _y2 );
	vect_init( &r->v1, _x1, _y1 );
	vect_init( &r->v2, _x2, _y2 );
	return r;
}



void rect_generate_position( rect_t* r, vect_t* sz, rect_t* map_r )
{
	assert( map_r->v2.x - map_r->v1.x >= sz->x );
	assert( map_r->v2.y - map_r->v1.y >= sz->y );
	r->v1.x = rand_range( map_r->v1.x, map_r->v2.x - sz->x );
	r->v1.y = rand_range( map_r->v1.y, map_r->v2.y - sz->y );
	vect_add( &r->v1, sz, &r->v2 );
}

void rect_generate_overlapping( rect_t* r, rect_t* overlap_r, vect_t* sz, vect_t* map_sz )
{
	vect_t* unit = vect_create(1,1);
	rect_t aux_r;

	vect_substract( &overlap_r->v1, sz, &aux_r.v1 );
	vect_add( &aux_r.v1, unit, &aux_r.v1 );
	if( aux_r.v1.x < 0 )	aux_r.v1.x = 0;
	if( aux_r.v1.y < 0 )	aux_r.v1.y = 0;

	vect_substract( &overlap_r->v2, unit, &aux_r.v2 );
	vect_add( &aux_r.v2, sz, &aux_r.v2 );
	if( aux_r.v2.x > map_sz->x )	aux_r.v2.x = map_sz->x;
	if( aux_r.v2.y > map_sz->y )	aux_r.v2.y = map_sz->y;

	assert( rect_is_valid(&aux_r) );

	vect_init( &r->v1, rand_range(aux_r.v1.x, aux_r.v2.x), rand_range(aux_r.v1.y, aux_r.v2.y) );
	vect_add( &r->v1, sz, &r->v2 );

	vect_destroy( unit );
}

void rect_generate_nextto( rect_t* r, rect_t* nextto_r, vect_t* sz, vect_t* map_sz )
{
	rect_t aux_r = {{0,0}, {0,0}};
	int i, d, start_dir = rand_n(N_DIRS);
	for( i = 0, d = start_dir; i < N_DIRS; i++, d = (d+1) % N_DIRS )
	{
		if( d == DIR_UP )		rect_init4( &aux_r, nextto_r->v1.x-sz->x+1, nextto_r->v2.y, nextto_r->v2.x+sz->x-1, nextto_r->v2.y+sz->y );
		if( d == DIR_DOWN )		rect_init4( &aux_r, nextto_r->v1.x-sz->x+1, nextto_r->v1.y-sz->y, nextto_r->v2.x+sz->x-1, nextto_r->v1.y );
		if( d == DIR_RIGHT )	rect_init4( &aux_r, nextto_r->v2.x, nextto_r->v1.y-sz->y+1, nextto_r->v2.x+sz->x, nextto_r->v2.y+sz->y-1 );
		if( d == DIR_LEFT )		rect_init4( &aux_r, nextto_r->v1.x-sz->x, nextto_r->v1.y-sz->y+1, nextto_r->v1.x, nextto_r->v2.y+sz->y-1 );

		if( aux_r.v1.x < 0 )			aux_r.v1.x = 0;
		if( aux_r.v1.y < 0 )			aux_r.v1.y = 0;
		if( aux_r.v2.x > map_sz->x )	aux_r.v2.x = map_sz->x;
		if( aux_r.v2.y > map_sz->y )	aux_r.v2.y = map_sz->y;

		if( aux_r.v2.x - aux_r.v1.x < sz->x )	continue;
		if( aux_r.v2.y - aux_r.v1.y < sz->y )	continue;

		vect_substract( &aux_r.v2, sz, &aux_r.v2 );
		vect_init( &r->v1, rand_range(aux_r.v1.x,aux_r.v2.x), rand_range(aux_r.v1.y,aux_r.v2.y) );
		vect_add( &r->v1, sz, &r->v2 );
		break;
	}
	assert( i < N_DIRS );
}

unsigned int rect_perimeter( rect_t* r )
{
	return (unsigned int) (r->v2.x - r->v1.x + r->v2.y - r->v1.y) * 2;
}

unsigned int rect_area( rect_t* r )
{
	return (unsigned int) (r->v2.x - r->v1.x) * (r->v2.y - r->v1.y);
}

int rect_distance( rect_t* r1, rect_t* r2 )
{
	int dist = 0;
	if( rect_is_overlapping( r1, r2) )	return -1;

	if( r1->v2.x <= r2->v1.x )	dist += r2->v1.x - r1->v2.x;
	if( r1->v1.x >= r2->v2.x )	dist += r1->v1.x - r2->v2.x;
	if( r1->v2.y <= r2->v1.y )	dist += r2->v1.y - r1->v2.y;
	if( r1->v1.y >= r2->v2.y )	dist += r1->v1.y - r2->v2.y;
	return dist;
}

void rect_normalize( rect_t* r )
{
	rect_t out_r = {{0,0}, {0,0}};
	vect_init( &out_r.v1, min(r->v1.x,r->v2.x), min(r->v1.y,r->v2.y) );
	vect_init( &out_r.v2, max(r->v1.x,r->v2.x), max(r->v1.y,r->v2.y) );
	*r = out_r;
}

int rect_crop( rect_t* r, rect_t* constraint_r, rect_t* crop_r )
{
	if( !rect_is_overlapping( r, constraint_r) )	return 0;

	crop_r->v1.x = max( r->v1.x, constraint_r->v1.x );
	crop_r->v1.y = max( r->v1.y, constraint_r->v1.y );
	crop_r->v2.x = min( r->v2.x, constraint_r->v2.x );
	crop_r->v2.y = min( r->v2.y, constraint_r->v2.y );
	return 1;
}

void rect_split( rect_t* r, int split_dir, rect_t* r_left, rect_t* r_right, rect_t* r_split )
{
	assert( r && r_left && r_right );
	assert( split_dir == DIR_UP || split_dir == DIR_RIGHT );

	int mid;
	*r_left = *r;
	*r_right = *r;
	*r_split = *r;

	if( split_dir == DIR_UP )
	{
		mid = ( r->v1.x + r->v2.x ) / 2;
		r_left->v2.x = r_right->v1.x = mid;
		r_split->v1.x = r_split->v2.x = mid;
	}
	if( split_dir == DIR_RIGHT )
	{
		mid = ( r->v1.y + r->v2.y ) / 2;
		r_left->v2.y = r_right->v1.y = mid;
		r_split->v1.y = r_split->v2.y = mid;
	}
}

void rect_expand( rect_t* r, double* fx, rect_t* r_expand )
{
	vect_t aux1_v, aux2_v;
	vect_scale( &dirs[ DIR_LEFT ], fx[ DIR_LEFT ], &aux1_v );
	vect_scale( &dirs[ DIR_DOWN ], fx[ DIR_DOWN ], &aux2_v );
	vect_add( &aux1_v, &aux2_v, &aux1_v );
	vect_add( &r->v1, &aux1_v, &r_expand->v1 );

	vect_scale( &dirs[ DIR_RIGHT ], fx[ DIR_RIGHT ], &aux1_v );
	vect_scale( &dirs[ DIR_UP ]   , fx[ DIR_UP ],    &aux2_v );
	vect_add( &aux1_v, &aux2_v, &aux1_v );
	vect_add( &r->v2, &aux1_v, &r_expand->v2 );
}



/* compares 2 rectangles, returns:
 * -  0 if they overlap,
 * - -1 if "r1" is positioned lower or more to the left than "r2",
 * - +1 if "r1" is positioned higher or more to the right than "r2" */
int rect_cmp( rect_t* r1, rect_t* r2 )
{
	if( r1->v2.x <= r2->v1.x )	return -1;
	if( r1->v1.x >= r2->v2.x )	return +1;
	if( r1->v2.y <= r2->v1.y )	return -1;
	if( r1->v1.y >= r2->v2.y )	return +1;
	return 0;
}

int rect_is_overlapping( rect_t* r1, rect_t* r2 )
{
	if( r1->v2.x <= r2->v1.x || r1->v1.x >= r2->v2.x || r1->v2.y <= r2->v1.y || r1->v1.y >= r2->v2.y )
		return 0;
	return 1;
}

int rect_is_overlapping_any( rect_t* r, rect_t* ranges, int n_ranges )
{
	int i;
	for( i = 0; i < n_ranges; i++ )
		if( rect_is_overlapping( r, &ranges[i] ) )
			return 1;
	return 0;
}

int rect_quadrant(rect_t* r1, rect_t* r2)
{
	if(rect_is_overlapping(r1, r2)) return 0;
	if(r1->v2.x <= r2->v1.x   &&   r1->v1.y >= r2->v2.y)  return 1; //NW
	if(r1->v1.x >= r2->v2.x   &&   r1->v1.y >= r2->v2.y)  return 2; //NE
	if(r1->v2.x <= r2->v1.x   &&   r1->v2.y <= r2->v1.y)  return 3; //SW
	if(r1->v1.x >= r2->v2.x   &&   r1->v2.y <= r2->v1.y)  return 4; //SE

	if(((r1->v1.x <= r2->v1.x && r2->v1.x <= r1->v2.x) ||
	    (r1->v1.x <= r2->v2.x && r2->v2.x <= r1->v2.x))&&
	    (r1->v1.y >= r2->v2.y)) return 5; // go down
	if(((r1->v1.x <= r2->v1.x && r2->v1.x <= r1->v2.x) ||
	    (r1->v1.x <= r2->v2.x && r2->v2.x <= r1->v2.x))&&
	    (r1->v2.y <= r2->v1.y)) return 6; // go up
	if(((r1->v1.y <= r2->v1.y && r2->v1.y <= r1->v2.y) ||
	    (r1->v1.y <= r2->v2.y && r2->v2.y <= r1->v2.y))&&
	    (r1->v1.x >= r2->v2.x)) return 7; // go left
	if(((r1->v1.y <= r2->v1.y && r2->v1.y <= r1->v2.y) ||
	    (r1->v1.y <= r2->v2.y && r2->v2.y <= r1->v2.y))&&
	    (r1->v2.x <= r2->v1.x)) return 8; // go right

	assert(0);
	return 0;
}

int rect_is_nextto( rect_t* r1, rect_t* r2 )
{
	if( rect_distance(r1,r2) != 0  )		return 0;

	if( vect_is_eq( &r1->v2, &r2->v1 ) )	return 0;
	if( vect_is_eq( &r1->v1, &r2->v2 ) )	return 0;
	if( r1->v1.x == r2->v2.x && r1->v2.y == r2->v1.y )	return 0;
	if( r1->v2.x == r2->v1.x && r1->v1.y == r2->v2.y )	return 0;
	return 1;
}

int rect_is_contained( rect_t* r, rect_t* cont_r )
{
	return (cont_r->v1.x <= r->v1.x && r->v2.x <= cont_r->v2.x &&
			cont_r->v1.y <= r->v1.y && r->v2.y <= cont_r->v2.y);
}

int rect_can_fit_inside( rect_t* r, rect_t* cont_r )
{
	if( r->v2.x-r->v1.x > cont_r->v2.x-cont_r->v1.x )	return 0;
	if( r->v2.y-r->v1.y > cont_r->v2.y-cont_r->v1.y )	return 0;
	return 1;
}

int rect_is_outside(rect_t* in, rect_t* out)
{
    // v1: lower left
    // v2: upper right

    //Out on left
    if( in->v1.x < out->v1.x) return 1;
    //Out on right
    if( in->v2.x > out->v2.x) return 1;
    //Out on bottom
    if( in->v1.y < out->v1.y) return 1;
    //Out on top
    if( in->v2.y > out->v2.y) return 1;
    //Inside
    return 0;
}

void rect_pack( rect_t* r, streamer_t* st )
{
	vect_pack( &r->v1, st );
	vect_pack( &r->v2, st );
}

void rect_unpack( rect_t* r, streamer_t* st )
{
	vect_unpack( &r->v1, st );
	vect_unpack( &r->v2, st );
}




