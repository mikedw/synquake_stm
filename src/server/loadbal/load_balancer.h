#ifndef __LOAD_BALANCER
#define __LOAD_BALANCER



// comment this line to remove any load balancing
#define __LOAD_BALANCING__ 

// comment this line to remove load balancing output
//#define DEBUG_STATS


#define BALANCE_NONE		"none"
#define BALANCE_STATIC1		"static1"
#define BALANCE_STATIC2		"static2"
#define BALANCE_STATIC3		"static3"
#define BALANCE_LIGHTEST	"lightest"
#define BALANCE_SPREAD		"spread"

#define BALANCE_QUADTREE	"quadtree"
#define BALANCE_AREATREE	"areatree"

#define BALANCE_GRAPH		"graph"

#define BALANCE_AOIGRAPH	"aoigraph"
#define BALANCE_LOSGRAPH	"losgraph"


#define BALANCE_NONE_T		0
#define BALANCE_STATIC1_T	1
#define BALANCE_STATIC2_T	2
#define BALANCE_STATIC3_T	3
#define BALANCE_LIGHTEST_T	4
#define BALANCE_SPREAD_T	5

#define BALANCE_QUADTREE_T	6
#define BALANCE_AREATREE_T	7

#define BALANCE_GRAPH_T		8
#define BALANCE_AOIGRAPH_T	9
#define BALANCE_LOSGRAPH_T	10


/*
IN: - areanode tree (info about objects)
    - player list (connected players)
    - number of threads

OUT:
    - players assigned to threads
*/

void loadb_init( char* type );
int  loadb_balance( int cycle );
void loadb_print_results( int cycle );

#endif
