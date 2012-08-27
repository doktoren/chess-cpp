#ifndef _ENDGAME_EN_PASSANT_
#define _ENDGAME_EN_PASSANT_

#include "../typedefs.hxx"
#include "../board.hxx"

extern const Position AFTER_WHITE_PAWN_ADVANCED[8][8];
extern const Position AFTER_BLACK_PAWN_ADVANCED[8][8];
extern const Position INVERSE_ADVANCED_PAWN[64];
extern const Position INVERSE_CAPTURER_PAWN[64];

// Returns true iff (wp,bp) can encode the en passant capability
// (if e.g. a white pawn has moved 2 forward between to black
//  pawns, then both (wp,bp1) and (wp,bp2) can encode the e.p.)
inline bool is_en_passant_pawns(Position ep, Position wp, Position bp) {
  assert(ROW[ep]==3  ||  ROW[ep]==4);
  return ROW[wp] == ROW[bp]  &&
    (wp+1 == bp  ||  wp == bp+1)  &&
    (ep+8==wp  ||  bp+8==ep);
}


// wpawn must be equal to bpawn. if 0 is returned, their position
// did not relate to any en passant constellation. Otherwise
// the position of the en passant is returned and wpawn and bpawn
// are updated to their correct positions.
inline int retrieve_en_passant(Position &wpawn, Position &bpawn) {
  assert(wpawn == bpawn);
  if (!INVERSE_ADVANCED_PAWN[wpawn]) return 0;

  if (ROW[wpawn] < 4) {
    // White pawn moved 2 forward. Black to move
    wpawn = INVERSE_ADVANCED_PAWN[wpawn];
    bpawn = INVERSE_CAPTURER_PAWN[bpawn];
    return wpawn-8;

  } else {

    // Black pawn moved 2 forward. White to move
    bpawn = INVERSE_ADVANCED_PAWN[bpawn];
    wpawn = INVERSE_CAPTURER_PAWN[wpawn];
    return bpawn+8;
  }
}

inline void encode_en_passant(Position &wpawn, Position &bpawn) {
  assert(ROW[wpawn] == ROW[bpawn]);
  assert(wpawn+1==bpawn  ||  wpawn==bpawn+1);
  
  int white_column = COLUMN[wpawn];
  int black_column = COLUMN[bpawn];

  if (ROW[wpawn] == 3) {
    // White pawn moved 2 forward. Black to move
    wpawn = bpawn = AFTER_WHITE_PAWN_ADVANCED[white_column][black_column];

  } else {

    assert(ROW[wpawn] == 4);
    // Black pawn moved 2 forward. White to move
    wpawn = bpawn = AFTER_BLACK_PAWN_ADVANCED[black_column][white_column];
  }
}

#endif
