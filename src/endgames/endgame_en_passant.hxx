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




/*
// wpawn must be equal to bpawn. if 0 is returned, their position
// did not relate to any en passant constellation. Otherwise
// the position of the en passant is returned and wpawn and bpawn
// are updated to their correct positions.
inline int retrieve_en_passant(Position &wpawn, Position &bpawn) {
  assert(wpawn == bpawn);

  switch(ROW[wpawn]) {
  case 2:
    if (COLUMN[wpawn] == 7) return 0;
    // White has moved 2 forward, black pawn on colum +1
    wpawn += 8;
    bpawn += 9;
    return wpawn-8;
  case 3:
    if (COLUMN[wpawn] == 0) return 0;
    // White has moved 2 forward, black pawn on colum -1
    --bpawn;
    return wpawn-8;
  case 4:
    if (COLUMN[bpawn] == 0) return 0;
    // Black has moved 2 forward, white pawn on colum -1
    --wpawn;
    return bpawn+8;
  case 5:
    if (COLUMN[bpawn] == 7) return 0;
    // Black has moved 2 forward, white pawn on colum +1
    bpawn -= 8;
    wpawn -= 7;
    return bpawn+8;
  default:
    return 0;
  }
}


inline void encode_en_passant(Position &wpawn, Position &bpawn) {
  assert(ROW[wpawn] == ROW[bpawn]);

  if (ROW[wpawn]==3) {
    // black to move, white pawn just moved 2 forward
    if (wpawn < bpawn) {
      assert(wpawn+1==bpawn);
      // White has moved 2 forward, black pawn on colum +1
      bpawn = wpawn -= 8;
    } else {
      assert(bpawn+1==wpawn);
      // White has moved 2 forward, black pawn on colum -1
      ++bpawn;
    }

  } else {
    assert(ROW[bpawn]==4);

    if (bpawn < wpawn) {
      assert(bpawn+1==wpawn);
      // Black has moved 2 forward, white pawn on colum +1
      wpawn = bpawn += 8;
    } else {
      assert(wpawn+1==bpawn);
      // Black has moved 2 forward, white pawn on colum -1
      ++wpawn;
    }
  }
}
*/

inline void verify_en_passant_encoding() {
  cerr << "Verifying en passant encoding.\n";
#include "../board_define_position_constants.hxx"
  Position wbpawn[2*2*14] =
    {a4,b4, b4,a4,  b4,c4, c4,b4,  c4,d4, d4,c4,  d4,e4, e4,d4,
     e4,f4, f4,e4,  f4,g4, g4,f4,  g4,h4, h4,g4,
     a5,b5, b5,a5,  b5,c5, c5,b5,  c5,d5, d5,c5,  d5,e5, e5,d5,
     e5,f5, f5,e5,  f5,g5, g5,f5,  g5,h5, h5,g5};
#include "../board_undef_position_constants.hxx"

  for (int i=0; i<2*14; i++) {
    Position wpawn = wbpawn[2*i];
    Position bpawn = wbpawn[2*i+1];

    encode_en_passant(wpawn, bpawn);
    assert(wpawn == bpawn);
    retrieve_en_passant(wpawn, bpawn);
    
    assert(wpawn == wbpawn[2*i]);
    assert(bpawn == wbpawn[2*i+1]);
  }
}



#endif
