
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

#ifndef __PLAYER_AI_H
#define __PLAYER_AI_H

class PlayerAI;

#include "client.h"
#include "AStar/AStar.h"


/***************************************************************************************************
*
* Constants
*
***************************************************************************************************/

#define		AI_RETRY_COUNT		10


#define		SEEKING_QUEST		0
#define		SEEKING_FOOD		1
#define		CHASING_PLAYER		2
#define		RUNNING_AWAY		3
#define		EXPLORING			4
#define		BASIC				5


const char AI_state_names[6][20] = { "quest","food","chasing","running away","exploring","basic" };
const char AI_debug_msgs[6][48] = { "Seeking quest at","Want food at","Chasing","Run away from","Exploring starting from","[W] Couldn't find any valid purpose at" };

enum PlayerAI_Action
{
	NO_ACTION = 0,
	MOVE,
	EAT,
	ATTACK
};

const char AI_actions[4][10] = { "none","move","eat","attack" };

/***************************************************************************************************
*
* AI Classs
*
***************************************************************************************************/

class PlayerAI
{
private:
	/* AI specific */
	StepStack*	current_path;	/* the current path the client is moving on */
	StepStack*	new_path;		/* temporary path used for testing new purpose decisions */

	AStar pathfind_alg;			/* class for the pathfindg algorithm */

	int debug_AI;				/* debugging flag */
	int wait_count;				/* number of times to wait until the server	confirmes our last action */

public:
	/* constructor */
	PlayerAI();

	rect_t* foodIsVisible();
	rect_t* weakPlayerNear();
	rect_t* strongPlayerNear();
	rect_t* findNearObject( int ent_type, int attr_id, value_t min_value, value_t max_value);
	bool isPurposeValid( int p, rect_t* purpose_r );

	bool isDestinationValid( rect_t* dest_r, int p, rect_t* purpose_r );
	void generateDestination( int p, rect_t* purpose_r, rect_t* dest_r );
	bool isPathValid( StepStack* path, int p, rect_t* purpose_r );

	void computePath( StepStack* path );
	void recomputePath( StepStack* path );
	int stepAlongPath( StepStack* path, rect_t** move_dest_out );
	int  moveAlongPath( StepStack* path );

	/* main action taking method */
	PlayerAI_Action takeAction();
};

#endif
