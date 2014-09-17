
/***************************************************************************************************
*
* SUBJECT:
*    A Benckmark for Massive Multiplayer Online Games
*    Game Server and Client
*
* AUTHOR:
*    Mihai Paslariu
*    Politehnica University of Bucharest, Bucharest, Romania
*    mihplaesu@yahoo.com
*
* TIME AND PLACE:
*    University of Toronto, Toronto, Canada
*    March - August 2007
*
***************************************************************************************************/

/* see AStar.h for details about this class */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "AStar.h"

/***************************************************************************************************
*
* Constructor and setup
*
***************************************************************************************************/



void AStar::createPath( StepStack* path, Node *current, vect_t* sz )
{
	Node *q = current, *q2;
	rect_t step_r;
	vect_t dir_v;
	int i, dir = -1;
	
	while( q != NULL )
	{
		if( q->path_next != NULL ) /* don't add source node */
		{
			q2 = q->path_next;
			
			vect_init( &dir_v, q->x-q2->x, q->y-q2->y );
			for( i = 0; i < N_DIRS; i++ )
				if( vect_is_eq( &dir_v, &dirs[i] ) )	break;
			assert( i<N_DIRS );
			
			if( i != dir )
			{
				rect_init4( &step_r, q->x, q->y, q->x + sz->x, q->y + sz->y );
				path->push( &step_r );
			}
			dir = i;		
		}		
		q = q->path_next;
	}
}

/***************************************************************************************************
*
* Main pathfinding method ( see AStar.h for details )
*
***************************************************************************************************/

void AStar::findPath( StepStack* path, vect_t* start_v, vect_t* sz,	int max_explored,
										 bool return_partial_path, bool only_around_fixed)
{
	vect_t* stop_v = &path->dest_r.v1;
	
	/* estimate the distance between the current point and the destination */
	#define estimate(_x,_y)		( abs((_x) - stop_v->x) + abs((_y) - stop_v->y) )
	/* the current node is the solution if it has the specified coordinates */
	#define is_solution(x,y)	( estimate(x,y) == 0 )

	Node *node;		/* starting node */
	Node *current;	/* node currently explored */
	Node *next,*q;
	Node *closest;	/* closest node to the destination (used to return a partial path) */
	int i,x,y;
	int he,ge;	/* estimate for h, and new g */

	/* create starting node */
	node = new Node( start_v->x,start_v->y );
	node->ghf( 0, estimate(start_v->x,start_v->y) );
	closest = NULL;

	open.add(node);
	current = NULL;

	/* explore the space around */
	bool path_found = false;
	while( !open.empty() )
	{
		/* get best node (best guess) */
		current = open.getMin();

		/* check if this node is the closest (used for partial path) */
		if( current->x != start_v->x || current->y != start_v->y )
			if( !closest || closest->h >= current->h ) closest = current;

		/* check if this is the destination */
		if( is_solution(current->x, current->y) )
		{
			path_found = true;
			closest = current;
			break;
		}

		/* add node to explored nodes */
		close.add(current);
		/* abort A* if limit is reached */
		if( close.size() > max_explored )			break;
		
		/* get nodes from all 4 directions */
		for( i = 0; i < N_DIRS; i++ )
		{
			/* coordinates of new node */
			x = current->x + dirs[i].x;
			y = current->y + dirs[i].y;
			ge = current->g + 1;
			he = estimate(x,y);

			/* check coodinates */
			rect_t pos_r;
			rect_init4( &pos_r, x, y, x+sz->x, y+sz->y );
			if( !rect_is_contained( &pos_r, &wm.map_r ) )		continue;
			
			bool not_vacant = false;
			int etypes = 0, et;
			for( et = 0; et < n_entity_types; et++ )
			{
				if( !entity_types[et]->fixed && only_around_fixed )		continue;
				etypes |= (1 << et);
			}			
			pentity_set_t* pe_set = worldmap_get_entities( &pos_r, etypes );
			elem_t* pos;
			pentity_set_for_each( pos, pe_set )
			{
				entity_t* ent = ((pentity_t*)pos)->ent;
				if( !vect_is_eq( &ent->r.v1, start_v ) )
				{
					//printf("%d,%d  %d,%d\n", pos_r.v1.x, pos_r.v1.y, ent->r.v1.x, ent->r.v1.y);	
					not_vacant = true;	break;		
				}
			}
			pentity_set_destroy( pe_set );
			
			if( not_vacant )	continue;
			
			
			/* check if the node is already in open */
			q = open.find(x,y);
			if ( q != NULL )
			{
				if ( q->g > ge )
				{
					q->ghf(ge,he);
					q->path_next = current;
				}
				continue;
			}

			/* check if the node is already in close */
			q = close.find(x,y);
			if ( q != NULL )
			{
				if ( q->g > ge )
				{
					close.remove(x,y);
					q->ghf(ge,he);
					q->path_next = current;
					open.add(q);
				}
				continue;
			}

			/* create new node and add it to open */
			next = new Node(x,y);
			next->ghf(ge,he);
			next->path_next = current;
			open.add(next);
		}
	}

	/*
	printf("A* %d,%d->%d,%d found=%s\n",
		startx,starty, stopx,stopy,
		open.empty()?"false":"true");
	*/

	/* create path */
	path->clear();
	if( closest && (path_found || ( return_partial_path && close.size() > max_explored) ) )
		createPath( path, closest, sz );
		
	open.clearNodes();
	close.eraseAll();
	return;
}


