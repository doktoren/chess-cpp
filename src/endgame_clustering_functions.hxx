#ifndef _ENDGAME_CLUSTER_FUNCTIONS_
#define _ENDGAME_CLUSTER_FUNCTIONS_

// board.hxx contains definitions of DB_WPAWN_VALUE, etc.
#include "board.hxx"

#ifdef ALLOW_5_MEN_ENDGAME
typedef int (*ClusterFunction)(int,int,int,int,int);
#else
typedef int (*ClusterFunction)(int,int,int,int);
#endif

void init_cluster_functions();

extern ClusterFunction cluster_functions[DB_ARRAY_LENGTH];
extern int cluster_functions_num_values[DB_ARRAY_LENGTH];

#endif
