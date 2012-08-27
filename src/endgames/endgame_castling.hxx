#ifndef _ENDGAME_CASTLING_
#define _ENDGAME_CASTLING_

#include "../typedefs.hxx"
#include "../piece_pos.hxx"

extern const bool KING_CASTLING_POSITIONS[64];
extern const Position DECODE_SHORT_CASTLING_ROOK[64];
extern const Position DECODE_LONG_CASTLING_ROOK[64];
extern const int KING_REFLECTIONS[2][64];

#endif
