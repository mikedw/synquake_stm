
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

/* see PointStack.h for details about this class */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../../utils/malloc.h"
#include "StepStack.h"




StepStack::StepStack()
{
	n = 0;
	cap = 256;
	steps = (rect_t*)malloc_wrapper( cap * sizeof(rect_t) );	assert( steps );
	rect_init4( &dest_r, 0, 0, 0, 0 );
}

StepStack::~StepStack()
{
	assert(steps);free_wrapper(steps);steps = NULL;
}

void StepStack::push(rect_t* new_step)
{
	/* when there is not enough space the stack doubles itself */
	if ( n + 1 >= cap )
	{
		int cap2 = 2 * cap;
		rect_t*	steps2 = (rect_t*)malloc_wrapper( cap2 * sizeof(rect_t) );	assert( steps2 );
		memcpy( steps2, steps, cap*sizeof(rect_t) );		free_wrapper(steps); 
		steps = steps2;
		cap = cap2;
	}

	/* add the new step on top of the stack */
	steps[n] = *new_step;
	n++;
}

rect_t* StepStack::top()
{
	if ( n > 0 && n <= cap ) return &steps[n-1];
	return NULL;
}

void StepStack::pop()
{
	if ( n > 0 ) n--;
}

int StepStack::size()
{
	return n;
}

bool StepStack::empty()
{
	return n == 0;
}

void StepStack::clear()
{
	n = 0;
}
