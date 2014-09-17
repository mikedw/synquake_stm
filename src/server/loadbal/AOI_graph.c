#include "AOI_graph.h"
#include "../../game/entity.h"
#include "../../game/game.h"
#include "../../utils/malloc.h"
#include "../sgame/worldmap.h"
#include "../server.h"

//===== Weighted union-find with path compression =====
int Root(int i, int* parent) 
{
	while (i != parent[i])
	{
		parent[i] = parent[parent[i]];
		i = parent[i];
	}
	return i;
}

int Find(int p, int q, int* parent)
{
	return (Root(p, parent) == Root(q, parent));
}

void Union(int p, int q, int* parent, int* size)
{
	int i = Root(p, parent);
	int j = Root(q, parent);
	//merge smaller tree into larger tree
	if (size[i] < size[j]) { parent[i] = j; size[j] += size[i]; }
	else                   { parent[j] = i; size[i] += size[j]; }
}
//=====================================================

//===== Cost Function ========================
double get_cost(entity_t* pl1, entity_t* pl2)
{
	double cost=0;
	rect_t ar;
	int etypes;
	pentity_set_t *pe_set1, *pe_set2;
	
	int a_id = max_action_id; // id for action with maximum range possible

	ar = game_action_range( a_id, pl1, &wm.map_r );	// get the area from the map affected by largest range action
	etypes = game_action_etypes( a_id );		// get the bitset of entity_types affected by this action
	pe_set1 = worldmap_get_entities( &ar, etypes );	// get all the entities present in the "ar" area
	if (pentity_set_size(pe_set1) == 0) return 0;

	ar = game_action_range( a_id, pl2, &wm.map_r );	// get the area from the map affected by largest range action
	etypes = game_action_etypes( a_id );		// get the bitset of entity_types affected by this action
	pe_set2 = worldmap_get_entities( &ar, etypes );	// get all the entities present in the "ar" area
	if (pentity_set_size(pe_set2) == 0) return 0;

	elem_t *pos1, *pos2;
	pentity_set_for_each( pos1, pe_set1 )
	{
		entity_t* ent1 = ((pentity_t*)pos1)->ent;

		pentity_set_for_each( pos2, pe_set2 )
		{
			entity_t* ent2 = ((pentity_t*)pos2)->ent;
			if (entity_is_eq(ent1, ent2))
			{
				printf("\tPlayers (%d;%d-%d;%d) and (%d;%d-%d;%d) share obj(%d;%d-%d;%d)\n",
					pl1->r.v1.x, pl1->r.v1.y, pl1->r.v2.x, pl1->r.v2.y, 
					pl2->r.v1.x, pl2->r.v1.y, pl2->r.v2.x, pl2->r.v2.y, 
					ent1->r.v1.x, ent1->r.v1.y, ent1->r.v2.x, ent1->r.v2.y);
				cost++;
			}
		}
	}

/*
	GET ALL ENTITIES
	elem_t* pos;
	pentity_set_for_each( pos, pe_set )
	{
		entity_t* ent = ((pentity_t*)pos)->ent;
		if( entity_is_eq( pl1, ent )  )		continue;
		if( entity_is_eq( pl2, ent )  )		continue;

		//if( ent->ent_type == ET_PLAYER ){}
		
		if(rect_distance( &(pl1->r), &(ent->r)) <= AREA_OF_INTEREST &&
		   rect_distance( &(pl2->r), &(ent->r)) <= AREA_OF_INTEREST)
		{
			printf("\tPlayers (%d;%d-%d;%d) and (%d;%d) share obj(%d,%d)\n",
				pl1->r.v1.x, pl1->r.v1.y, pl1->r.v2.x, pl1->r.v2.y, 
				pl2->r.v1.x, pl2->r.v1.y, pl2->r.v2.x, pl2->r.v2.y, 
				ent->r.v1.x, ent->r.v1.y, ent->r.v2.x, ent->r.v2.y);
			cost++;
		}
	}
*/	
	return cost;
}
//============================================

#define false 0
#define true 1

int AOI_graph_build()
{
	// G = (V,E)
	// V = players
	// E = {cost(x,y) | x,y in V}	
	double** a;
	int* parent;
	int* size;
	int* visited;
	int* mappings;
	int i, j, u, v;
	int nrcomp; 
	int* compsize;
	int numpls, nthreads;
	entity_t *tmp1, *tmp2;

	printf("Building LOS graph... \n");

	numpls   = sv.n_clients;
	nthreads = sv.num_threads;	
	mappings = (int*)malloc_wrapper(numpls*sizeof(int));       if(mappings == NULL) return false;
	parent = (int*)malloc_wrapper(numpls*sizeof(int));         if(parent == NULL) return false;	
	size = (int*)malloc_wrapper(numpls*sizeof(int));           if(size == NULL) return false;	
	visited = (int*)malloc_wrapper(numpls*sizeof(int));        if(visited == NULL) return false;	
	compsize = (int*)malloc_wrapper(numpls*sizeof(int));       if(compsize == NULL) return false;	
	a = (double**)calloc_wrapper(numpls, sizeof(double*));     if(a == NULL) return false;

	printf("Allocate and build distance matrix... \n");
	// allocate and build distance matrix
	for(i=0; i<numpls; i++)
		a[i] = (double*)calloc_wrapper(numpls, sizeof(double));     if(a[i] == NULL) return false;

	for(i=0; i<numpls; i++) {
		for(j = i; j<numpls; j++) {
			if ( i == j ) { a[i][j]=0; continue; }
			
			tmp1 = sv.clients[i]->player;
			tmp2 = sv.clients[j]->player;
			a[i][j] = get_cost(tmp1, tmp2);
			a[j][i] = a[i][j];
		}
		mappings[i] = -1;
		parent[i] = i;
		size[i] = 1;
		visited[i] = 0;
		compsize[i] = 0;
	}
	
	for(i=0; i<numpls; i++)
	{
		for(j=0; j<numpls; j++)
			printf("%10.3lf   ", a[i][j]);
		printf("\n");
	}

	//----- BUILD CONNECTED COMPONENTS ----------
	
	printf("Build connected components... \n");
	for(u=0; u<numpls; u++)
		for(v=0; v<numpls; v++)
		{
			if(a[u][v] != 0)
				if(!Find(u, v, parent))
					Union(u, v, parent, size);
		}
	//-------------------------------------------

	printf("Build mappings... \n");	
	//----- BUILD MAPPINGS ----------------------
	nrcomp = 0;
	for(i=0; i<numpls; i++)
	{
		if(visited[i] == 1) continue;

		mappings[i] = nrcomp;	
		compsize[nrcomp]++;	
		for(j=i+1; j<numpls; j++)
			if(visited[j] == 0 && parent[i] == parent[j])
			{
				mappings[j] = nrcomp;
				visited[j] = 1;
				compsize[nrcomp]++;
			}
		nrcomp++;
	}
	//-------------------------------------------
	
	for(i=0;i<numpls;i++)
		printf("Mapping: P%d - T%d\n", i, mappings[i]);
	
	//----- ANALYZE COMPONENTS ------------------
	// Ncomp <= Nthreads,  each thread gets one component, some may get no players
	// Ncomp >  Nthreads,  distribute extra components in descending order of weight (using less loaded thread criterion)
	
	if(nthreads < nrcomp)
	{
		printf("Ncomp=%d  >  %d=Nthreads\n", nrcomp, nthreads);
		// distribute the extra components
		for(i=nthreads; i<nrcomp; i++)
		{
			printf("Reassigning component %d\n", i);
			int min = 0;
			for(j=1; j<nthreads; j++)
				if(compsize[j] < compsize[min]) min = j;
			

			printf("         to component %d\n", min);
			// add component "i" to thread "min"
			compsize[min] += compsize[i];
			for(j=0; j<numpls; j++)
				if(mappings[j] == i)   mappings[j] = min;
		}
	}
	
	//-------------------------------------------

	printf("Setting jobs... \n");
	for(i=0; i<numpls; i++)
		sv.clients[i]->tid = mappings[i];
	
	// free memory
	for(i=0;i<numpls;i++)
		free_wrapper(a[i]);
	free_wrapper(a);
	free_wrapper(parent);
	free_wrapper(size);
	free_wrapper(visited);
	free_wrapper(mappings);
	free_wrapper(compsize);

	return true;
}
