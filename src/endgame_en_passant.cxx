#include "endgame_en_passant.hxx"


#include "board_define_position_constants.hxx"
// ...[white pawn column][black pawn column]
const Position AFTER_WHITE_PAWN_ADVANCED[8][8] =
  {{ 0,a3, 0, 0, 0, 0, 0, 0},
   {b4, 0,b3, 0, 0, 0, 0, 0},
   { 0,c4, 0,c3, 0, 0, 0, 0},
   { 0, 0,d4, 0,d3, 0, 0, 0},
   { 0, 0, 0,e3, 0,e4, 0, 0},
   { 0, 0, 0, 0,f3, 0,f4, 0},
   { 0, 0, 0, 0, 0,g3, 0,g4},
   { 0, 0, 0, 0, 0, 0,h3, 0}};
// ...[black pawn column][white pawn column]
const Position AFTER_BLACK_PAWN_ADVANCED[8][8] =
  {{ 0,a6, 0, 0, 0, 0, 0, 0},
   {b5, 0,b6, 0, 0, 0, 0, 0},
   { 0,c5, 0,c6, 0, 0, 0, 0},
   { 0, 0,d5, 0,d6, 0, 0, 0},
   { 0, 0, 0,e6, 0,e5, 0, 0},
   { 0, 0, 0, 0,f6, 0,f5, 0},
   { 0, 0, 0, 0, 0,g6, 0,g5},
   { 0, 0, 0, 0, 0, 0,h6, 0}};


const Position INVERSE_ADVANCED_PAWN[64] =
  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
   a4,b4,c4,d4,e4,f4,g4,h4,
    0,b4,c4,d4,e4,f4,g4, 0,
    0,b5,c5,d5,e5,f5,g5, 0,
   a5,b5,c5,d5,e5,f5,g5,h5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};
const Position INVERSE_CAPTURER_PAWN[64] =
  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
   b4,c4,d4,e4,d4,e4,f4,g4,
    0,a4,b4,c4,f4,g4,h4, 0,
    0,a5,b5,c5,f5,g5,h5, 0,
   b5,c5,d5,e5,d5,e5,f5,g5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};
#include "board_undef_position_constants.hxx"

