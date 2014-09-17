#include "LOS_graph.h"
#include "../../game/entity.h"
#include "../../utils/geometry.h"
#include "../../utils/malloc.h"
#include "../server.h"

#define MAX_INT 2147483647

#define false 0
#define true 1


void colour_tree(int root, int* parents, int* mappings, char* visited, int numpls)
{
	int i;
	for(i=0; i<numpls; i++)
	{
		if(visited[i] && parents[i] == root)
		{
			mappings[i] = mappings[root];
			visited[i] = 0;
			colour_tree(i, parents, mappings, visited, numpls);
		}
	}
}

// Build graph and find Euclidean Minimum Spanning Tree
// Do num_threads cuts on the num_threads-longest edges in the MST
int LOS_graph_build()
{
	// G = (V,E)
	// V = players
	// E = {d(x,y) | x,y in V}
	
	double** a;
	double* distance;
	int* parents;
	char* visited;
	int* mappings;
	int i, j;
	int treesize;
	int found;
	double totaldistance;
	int numpls, nthreads;
	entity_t *tmp1, *tmp2;

	printf("Building LOS graph... \n");

	numpls   = sv.n_clients;
	nthreads = sv.num_threads;
	mappings = (int*)malloc_wrapper(numpls*sizeof(int));        if(mappings == NULL) return false;
	distance = (double*)malloc_wrapper(numpls*sizeof(double));  if(distance == NULL) return false;
	parents = (int*)malloc_wrapper(numpls*sizeof(int));         if(parents == NULL) return false;
	visited = (char*)malloc_wrapper(numpls*sizeof(char));       if(visited == NULL) return false;	
	a = (double**)calloc_wrapper(numpls, sizeof(double*));      if(a == NULL) return false;

	printf("Allocate and build distance matrix... \n");
	// allocate and build distance matrix
	for(i=0; i<numpls; i++) {
		a[i] = (double*)calloc_wrapper(numpls, sizeof(double));     if(a[i] == NULL) return false;
		for(j=0; j<numpls; j++) {
			if ( i == j ) { a[i][j]=0; continue; }
			tmp1 = sv.clients[i]->player;
			tmp2 = sv.clients[j]->player;
			a[i][j] = rect_distance( &(tmp1->r), &(tmp2->r));
		}
		parents[i] = -1;
		visited[i] = 0;
		distance[i] = MAX_INT;
		mappings[i] = -1;
	}
	
	for(i=0; i<numpls; i++)
	{
		for(j=0; j<numpls; j++)
			printf("%10.3lf   ", a[i][j]);
		printf("\n");
	}

	printf("Build MST... \n");
	//----- BUILD MST -------------------------------
	// Add the first node to the tree and update distances
	visited[0] = 1;
	distance[0] = 0;
	for (i=1; i<numpls; i++)  
		if (distance[i] > a[0][i]) {
			distance[i] = a[0][i];
			parents[i] = 0;
		}
	
	totaldistance = 0; 
	for(treesize=1; treesize < numpls; treesize++) 
	{
		// Find node closest to the tree
		int min = -1;
		for (i=1; i<numpls; i++)
			if ( visited[i] == 0)
				if (min == -1 || distance[min] > distance[i])   min = i;
		
		visited[min] = 1;
		totaldistance += distance[min];

		// update the distances
		for (i=1; i<numpls; i++)  
			if ((i != min) && (distance[i] > (a[min][i] + distance[min]))) {
				distance[i] = a[min][i] + distance[min];
				parents[i] = min;
			}
	}

	for(i=0; i<numpls; i++)
		printf("P%d:  parent:%d   distance:%10.3lf   visited:%d\n", i, parents[i], distance[i], visited[i]);

	printf("Total distance: %10.3lf\n", totaldistance);
	//-----------------------------------------------

	printf("Build mappings... \n");	
	//----- BUILD MAPPINGS --------------------------
	// do num_threads cuts
	for(i=0; i < (nthreads-1); i++)
	{
		// do a cut on the maximum weighted edge of the MST
		int max = -1;
		for(j=0; j<numpls; j++)
			if(parents[j] != -1)
				if(max == -1 || distance[max] < distance[j])   { max = j; }

		parents[max] = -1; // cut edge
	}
	for(i=0; i<numpls; i++)
		printf("P%d:  parent:%d   distance:%10.3lf\n", i, parents[i], distance[i]);

	// assign mappings
	for(i=0; i<nthreads; i++)
	{
		found = false;
		for(j=0; j<numpls; j++)
			if(parents[j] == -1 && visited[j]) {
				found = true;  break;
			}
		assert(found);

		mappings[j] = i;  visited[j] = 0;
		colour_tree(i, parents, mappings, visited, numpls);
	}
	//-----------------------------------------------	

	for(i=0;i<numpls;i++)
		printf("Mapping: P%d - T%d\n", i, mappings[i]);

	printf("Setting jobs... \n");
	for(i=0;i<numpls;i++)
		sv.clients[i]->tid = mappings[i];
	
	// free memory
	for(i=0;i<numpls;i++)
		free_wrapper(a[i]);
	free_wrapper(a);
	free_wrapper(distance);
	free_wrapper(parents);
	free_wrapper(visited);
	free_wrapper(mappings);
	
	return true;
}
