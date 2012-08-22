//##########################################################
//###########                               ################
//###########           CASTLING            ################
//###########                               ################
//##########################################################

#include "endgame_castling.hxx"

/*
  Problemer med castling
  Det er ukendt hvilken spejling, stillingen er opstaaet med.
  For at castling information kan bruges til noget, skal det
  forbindes med en korrekt spejling af stillingen til en 
  stilling hvor kongen/kongerne staar som de skal
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

/*
int REFLECTION[64] =
  { 0, 0, 0, 0, 1, 1, 1, 1,
    4, 0, 0, 0, 1, 1, 1, 5,
    4, 4, 0, 0, 1, 1, 5, 5,
    4, 4, 4, 0, 1, 5, 5, 5,
    6, 6, 6, 2, 3, 7, 7, 7,
    6, 6, 2, 2, 3, 3, 7, 7,
    6, 2, 2, 2, 3, 3, 3, 7,
    2, 2, 2, 2, 3, 3, 3, 3 };
*/

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

//int castling_reflection;

// castling is a sum of
// WHITE_LONG_CASTLING (1<<4)
// WHITE_SHORT_CASTLING (2<<4)
// BLACK_LONG_CASTLING (4<<4)
// BLACK_SHORT_CASTLING (8<<4)
inline int full_symmetry_retrieve_castling(Position wking, Position bking) {
  
  //todo
  return 0;
}


/*
inline int full_symmetry_retrieve_castling(Position wking, Position bking, Position &wrook) {
  if (FS_CASTLING[wking]) {
    if (wrook==wking) {
      // Short castling
      wrook = FS_SHORT_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      return WHITE_SHORT_CASTLING;
    }
    if (wrook==bking) {
      // Long castling
      wrook = FS_LONG_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      return WHITE_LONG_CASTLING;
    }
  }
  return 0;
}

inline int full_symmetry_retrieve_castling_ww(Position wking, Position bking, Position &wrook1, Position &wrook2) {
  // if (wrook1 == wrook2) return 0; // Will never happen

  if (FS_CASTLING[wking]) {
    int result = 0;

    if (wrook1==wking) {
      // Short castling
      wrook1 = FS_SHORT_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      result |= WHITE_SHORT_CASTLING;

    } else if (wrook1==bking) {
      // Long castling
      wrook1 = FS_LONG_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      result |= WHITE_LONG_CASTLING;
    }

    if (wrook2==wking) {
      // Short castling
      wrook2 = FS_SHORT_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      result |= WHITE_SHORT_CASTLING;

    } else if (wrook1==bking) {
      // Long castling
      wrook2 = FS_LONG_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      result |= WHITE_LONG_CASTLING;
    }

    return result;
  }
  return 0;
}

inline int full_symmetry_retrieve_castling_wb(Position wking, Position bking, Position &wrook, Position &brook) {
  // if (wrook1 == wrook2) return 0; // Will never happen
  int result = 0;
  castling_reflection = -1;

  if (FS_CASTLING[wking]) {
    if (wrook==wking) {
      // Short castling
      wrook = FS_SHORT_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      result |= WHITE_SHORT_CASTLING;
      
    } else if (wrook==bking) {
      // Long castling
      wrook = FS_LONG_CASTLING_ROOK[wking];
      castling_reflection = WKING_REFLECTION[wking];
      result |= WHITE_LONG_CASTLING;
    }
  }

  if (FS_CASTLING[bking]) {
    if (brook==bking) {
      // Short castling
      brook = FS_SHORT_CASTLING_ROOK[bking];
      if (castling_reflection!=-1  &&  castling_reflection!=BKING_REFLECTION[wking]) {
	// The board cannot be rotated such that both white and black king are
	// at their start positions
	return 0;
      }
      castling_reflection = BKING_REFLECTION[wking];
      result |= BLACK_SHORT_CASTLING;
      
    } else if (brook==wking) {
      // Long castling
      brook = FS_LONG_CASTLING_ROOK[bking];
      if (castling_reflection!=-1  &&  castling_reflection!=BKING_REFLECTION[wking])
	return 0;
      castling_reflection = BKING_REFLECTION[wking];
      result |= BLACK_LONG_CASTLING;
    }
  }

  return result;
}
*/
