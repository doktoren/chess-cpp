#ifndef _ENDGAME_RECOGNIZE_FROM_PIECES_
#define _ENDGAME_RECOGNIZE_FROM_PIECES_

#include <assert.h>
#include <stdint.h>

#include "../piece_pos.hxx"
#include "../board_constants.hxx"

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


union endgame_material_t {
  struct {
    // WARNING: big-little endian dependent code
    // Sufficient material left <=> insufficient_material_b && insufficient_material_a
    // endgame_hashing will create overflow, so it must be placed last
    bool endgame_material_b;
    bool endgame_material_a;
    uint16_t endgame_hashing;
  } individual;
  uint32_t as_pattern;
};

extern const endgame_material_t ENDGAME_MATERIAL_HASHING_CONSTANTS[13][2];

inline void add_endgame_material(endgame_material_t &material, Piece piece, Position position) {
  material.as_pattern += ENDGAME_MATERIAL_HASHING_CONSTANTS[piece][POS_COLOR[position]].as_pattern;
}
inline void remove_endgame_material(endgame_material_t &material, Piece piece, Position position) {
  material.as_pattern -= ENDGAME_MATERIAL_HASHING_CONSTANTS[piece][POS_COLOR[position]].as_pattern;
}

/**
 * Insufficient material left?
 * Sufficient material is:
 * a) a pawn, a rook or a queen
 * b) 2 knights or 1 knight and a bishop
 * c) 2 bishop on different colors
 */
inline bool insufficient_material(endgame_material_t material) {
  return !material.individual.endgame_material_a || !material.individual.endgame_material_b;
}

extern const uint_fast16_t ENDGAME_HASHING_CONSTANTS[13];

#endif
