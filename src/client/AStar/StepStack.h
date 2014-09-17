
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

/***************************************************************************************************
*
* A stack of points
* - push inserts a the (x,y) values
* - getX and getY retrieves the coordinates from the point in the top of the stack
* - pop removes the top point
* - 'cap' is the capacity of the stack; the stack doubles itself if there is not enough space
* to hold a new point
*
***************************************************************************************************/

#ifndef __STEP_STACK_H
#define __STEP_STACK_H

extern "C"	{
#include "../../utils/geometry.h"
}


class StepStack
{
private:
	rect_t* steps;
	int n,cap;

public:
	rect_t dest_r;	

	/* constructor/destrctor */
	StepStack();
	~StepStack();

	/* stack operations */
	void push(rect_t* new_step);
	rect_t* top();
	void pop();

	/* size related methods */
	int size();
	bool empty();
	void clear();
};

#endif
