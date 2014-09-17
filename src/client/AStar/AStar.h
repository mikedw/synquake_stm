
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
* Implementation of the A* pathfinding algorithm
* - we use a matrix of size m x n where 0 means empty cell where the player can move, and != 0
* means a blocked cell
* - the programmer must first call setMatrix before findPath
* - the pathfinding method returns a stack of points => then the user can call stack.pop to get the
* next point in the path
* - the stack returned doesn't contain any points if a path is not found
* - the second matrix cand be used to provide aditional blocked cells (eg. for object that change
* position in the game)
*
***************************************************************************************************/


#ifndef __ASTAR_H
#define __ASTAR_H

#include "Node.h"
#include "NodeSet.h"
#include "StepStack.h"
#include "MinHeap.h"

extern "C" {
#include "../cgame/worldmap.h"
}

class AStar
{
private:
	MinHeap open;
	NodeSet close;

private:
	void createPath( StepStack* path, Node *current, vect_t* sz );

public:
	/* pathfinding method
	   - returns a queue of points
	   - the queue doesn't contain any points if no path is found
	   - max_explored is the maximum number of locations explored
	   ( use a very large number to search the whole space )
	   - if return_partial_path is true and no path is found, the method returns
	   a path that doesn't lead to the destination but is in the right direction
	*/
	void findPath( StepStack* path, vect_t* start_v, vect_t* sz, int max_explored, 
									bool return_partial_path, bool only_around_fixed);	
};

#endif
