#ifndef _BOARD_CONSTANTS_
#define _BOARD_CONSTANTS_

#include <stdint.h>
#include <stdlib.h>
#include <string>

#include "board_defines.hxx"
#include "piece_pos.hxx"

extern const bool WHITE_PIECE[13];
extern const bool BLACK_PIECE[13];
extern const int PIECE_COLOR[13];
extern const Piece PIECE_KIND[13];
extern const Piece SWAP_PIECE_COLOR[13];

//################################

extern const uint8_t CASTLING[64];
extern const uint8_t CASTLING_LOST[64];

//################################

extern const int ROW[64];
extern const int COLUMN[64];
extern const bool POS_COLOR[64];
extern const Position CR_TO_POS[8][8]; // Column, Row

extern const Position ROTATE[64];

inline bool legal_pos(Position p) { return !(p & 0xC0); }

inline int distance(Position p1, Position p2) {
  return abs(ROW[p1]-ROW[p2]) + abs(COLUMN[p1]-COLUMN[p2]);
}

//################################

extern const std::string game_result_texts[4];

extern const std::string game_status_texts[7];

union piece_count_t {
  uint32_t as_pattern;
  struct {
    // WARNING: big-little endian dependent code
    uint8_t num_colored_pieces[2];
    uint8_t num_pieces;
    uint8_t num_non_zugzwang_pieces;// 4 bits for each side
  } individual;
};

extern const piece_count_t PIECE_COUNT_CONSTANTS[13];

inline void piece_count_add(piece_count_t &piece_count, Piece piece) {
  piece_count.as_pattern += PIECE_COUNT_CONSTANTS[piece].as_pattern;
}
inline void piece_count_remove(piece_count_t &piece_count, Piece piece) {
  piece_count.as_pattern -= PIECE_COUNT_CONSTANTS[piece].as_pattern;
}

//################################

extern const char PIECE_CHAR[13];
extern const std::string PIECE_SCHAR[13];
extern const std::string PIECE_NAME[13];
extern const std::string PPIECE_NAME[13];
extern const char PLAYER_CHAR[2];
extern const std::string PLAYER_NAME[2];

extern const std::string POS_NAME[66];
extern const std::string COLUMN_NAME[66];
extern const std::string ROW_NAME[66];

inline Piece char_to_piece(char ch) {
  switch(ch) {
  case 'P': return WPAWN; break;
  case 'N': return WKNIGHT; break;
  case 'B': return WBISHOP; break;
  case 'R': return WROOK; break;
  case 'Q': return WQUEEN; break;
  case 'K': return WKING; break;
  case 'p': return BPAWN; break;
  case 'n': return BKNIGHT; break;
  case 'b': return BBISHOP; break;
  case 'r': return BROOK; break;
  case 'q': return BQUEEN; break;
  case 'k': return BKING; break;
  }
  return 0;
}
inline Position strToPos(std::string pos) {
  if (pos[0]=='#') return ILLEGAL_POS;
  return CR_TO_POS[pos[0]-'a'][pos[1]-'1'];
}

//################################

extern Position REFLECTION_TABLE[8*64];
inline Position reflect(int pos, int refl) {
  return REFLECTION_TABLE[(refl << 6) | pos];
}
inline Position inv_reflect(int pos, int inv_refl) {
  const uint_fast8_t INV[8] = {0,1,2,3,4,6,5,7};
  return REFLECTION_TABLE[(INV[inv_refl] << 6) | pos];
}

//################################

#define board_iterate(p) for(Position p=0; legal_pos(p); ++p)


#endif
