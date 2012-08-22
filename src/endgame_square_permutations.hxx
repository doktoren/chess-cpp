#ifndef _ENDGAME_SQUARE_PERMUTATIONS_
#define _ENDGAME_SQUARE_PERMUTATIONS_

#include "typedefs.hxx"

// When converting to bdd index, the position of the bound king
// has to be remapped such that 1 bit less is used for indexing.
extern int REMAP_BOUND_KING[64];
extern int INV_REMAP_BOUND_KING[64];


string mapping_name(int mapping);

#define NUM_STD_MAPPINGS 10
extern uchar mappings[NUM_STD_MAPPINGS][64];

#endif
