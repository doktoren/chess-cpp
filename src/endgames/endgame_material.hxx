#ifndef _ENDGAME_RECOGNIZE_FROM_PIECES_
#define _ENDGAME_RECOGNIZE_FROM_PIECES_

#include <assert.h>
#include <stdint.h>

#include "../piece_pos.hxx"
#include "../board_constants.hxx"
#include "../experimenting.hxx"

#ifdef ALLOW_5_MEN_ENDGAME

// When at most 5 pieces is left of which 2 must be white and
// black king, the 0, 1, 2 or 3 remaining pieces can be uniquely
// determined by the weighted sum of the pieces, where the weights are:
#define DB_WPAWN_VALUE 1
#define DB_WKNIGHT_VALUE 4
#define DB_WBISHOP_VALUE 13
#define DB_WROOK_VALUE 32
#define DB_WQUEEN_VALUE 71
#define DB_WKING_VALUE 0
#define DB_BPAWN_VALUE 124
#define DB_BKNIGHT_VALUE 218
#define DB_BBISHOP_VALUE 375
#define DB_BROOK_VALUE 572
#define DB_BQUEEN_VALUE 744
#define DB_BKING_VALUE 0

#else

#define DB_WPAWN_VALUE 1
#define DB_WKNIGHT_VALUE 3
#define DB_WBISHOP_VALUE 7
#define DB_WROOK_VALUE 12
#define DB_WQUEEN_VALUE 20
#define DB_WKING_VALUE 0
#define DB_BPAWN_VALUE 30
#define DB_BKNIGHT_VALUE 44
#define DB_BBISHOP_VALUE 65
#define DB_BROOK_VALUE 80
#define DB_BQUEEN_VALUE 96
#define DB_BKING_VALUE 0

#endif
#define DB_ARRAY_LENGTH (3*DB_BQUEEN_VALUE+1)


extern const uint32_t ENDGAME_MATERIAL_HASHING_CONSTANTS[13][2];

inline uint32_t endgame_hashing(uint32_t material) {
  return material & 0xFFFF;
}
inline void add_endgame_material(uint32_t &material, Piece piece, Position position) {
  material += ENDGAME_MATERIAL_HASHING_CONSTANTS[piece][POS_COLOR[position]];
}
inline void remove_endgame_material(uint32_t &material, Piece piece, Position position) {
  material -= ENDGAME_MATERIAL_HASHING_CONSTANTS[piece][POS_COLOR[position]];
}

/**
 * Insufficient material left?
 * Sufficient material is:
 * a) a pawn, a rook or a queen
 * b) 2 knights or 1 knight and a bishop
 * c) 2 bishop on different colors
 */
inline bool insufficient_material(uint32_t material) {
  return (material & 0xFF000000U) == 0 || (material & 0x00FF0000U) == 0;
}

extern const uint_fast16_t ENDGAME_HASHING_CONSTANTS[13];

#endif
