//##########################################################
//###########                               ################
//###########           CASTLING            ################
//###########                               ################
//##########################################################

#include "endgame_castling.hxx"

/*
 * Problems with castling:
 * It's unknown by which rotation the current position has been reached.
 * In order for the castling information to be useful it must be combined
 * with a correct mirroring of the position on one where the king or kings
 * are correctly placed.
*/

const bool KING_CASTLING_POSITIONS[64] =
  {0,0,0,1,1,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   1,0,0,0,0,0,0,1,
   1,0,0,0,0,0,0,1,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,1,1,0,0,0};

const Position DECODE_SHORT_CASTLING_ROOK[64] =
  {0,0,0,0,7,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,7,
   56,0,0,0,0,0,0,63,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,56,63,0,0,0};

const Position DECODE_LONG_CASTLING_ROOK[64] =
  {0,0,0,7,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   56,0,0,0,0,0,0,63,
   0,0,0,0,0,0,0,7,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,63,56,0,0,0};

// 1: vertical reflection
// 2: horizontal reflection
// 4: diagonal reflection
const int KING_REFLECTIONS[2][64] =
  {{-1,-1,-1, 1, 0,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,
     6,-1,-1,-1,-1,-1,-1, 7,
     4,-1,-1,-1,-1,-1,-1, 5,
    -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1, 3, 2,-1,-1,-1},

   {-1,-1,-1, 3, 2,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,
     7,-1,-1,-1,-1,-1,-1, 6,
     5,-1,-1,-1,-1,-1,-1, 4,
    -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1, 1, 0,-1,-1,-1}};