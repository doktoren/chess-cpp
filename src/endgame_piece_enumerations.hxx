#ifndef _ENDGAME_PIECE_ENUMERATIONS_
#define _ENDGAME_PIECE_ENUMERATIONS_

#include "board.hxx"
#include "endgame_square_permutations.hxx"

void init_piece_enumerations();
void verify_piece_enumerations();

struct IndexRefl {
  IndexRefl() : index(-1) {}
  IndexRefl(int index, int refl) : index(index), refl(refl) {}

  bool is_valid() { return index != -1; }

  int index;
  int refl;
};


struct BDDIndexRefl {
  BDDIndexRefl() : valid(false) {}
#if BOUND_KING==0
  BDDIndexRefl(Position white_king, Position black_king, uchar refl) :
    remapped_bound_k(REMAP_BOUND_KING[white_king]),
    free_k(black_king), refl(refl), valid(true) {}
  BDDIndexRefl(pair<Position,Position> wb_king, uchar refl) :
    remapped_bound_k(REMAP_BOUND_KING[wb_king.first]),
    free_k(wb_king.second), refl(refl), valid(true) {}

  uchar white_king() { 
    assert(remapped_bound_k < 32);
    return INV_REMAP_BOUND_KING[remapped_bound_k];
  }
  uchar black_king() { return free_k; }
#else
  BDDIndexRefl(Position white_king, Position black_king, uchar refl) :
    remapped_bound_k(REMAP_BOUND_KING[black_king]),
    free_k(white_king), refl(refl), valid(true) {}
  BDDIndexRefl(pair<Position,Position> wb_king, uchar refl) :
    remapped_bound_k(REMAP_BOUND_KING[wb_king.second]),
    free_k(wb_king.first), refl(refl), valid(true) {}

  uchar white_king() { return free_k; }
  uchar black_king() {
    assert(remapped_bound_k < 32);
    return INV_REMAP_BOUND_KING[remapped_bound_k];
  }
#endif

  uchar bound_kind() { return INV_REMAP_BOUND_KING[remapped_bound_k]; }
  uchar free_king() { return free_k; }

  uchar remapped_bound_king() { return remapped_bound_k; }

  bool is_valid() { return valid; }

  // if pawnless endgame then 0<=index<1024, else 0<=index<2048
  int index() {
    return (remapped_bound_king() << 6) | free_king();
  }

  Position remapped_bound_k, free_k;
  uchar refl;
  bool valid;
};


// If BOUND_KING==1 then black king will be bound to the
// a1-d1-d4 triangle. Otherwise for white king. 0<=index<462
//
//                           KING_FULL_SYMMETRY
//                                   --->
// (pos(white king),pos(black king))      (index, reflection) : IndexRefl
//     | : pair<uchar,uchar>         <---     (index==-1  <=>  invalid)
//     |                         KING_FS_POS
//     |
//     | BDD_KING_FULL_SYMMETRY = KING_FS_POS o KING_FULL_SYMMETRY
//     V
//  (pos(white king),pos(black king),reflection,valid) : BDDIndexRefl
//     BDDIndexRefl.index() gives an 4+6 (bbbbffffff) bit index
//
extern IndexRefl KING_FULL_SYMMETRY[64*64];
extern pair<Position, Position> KING_FS_POS[462];
extern BDDIndexRefl BDD_KING_FULL_SYMMETRY[64*64];
inline IndexRefl &king_full_symmetry(uchar white_king, uchar black_king) {
  return KING_FULL_SYMMETRY[(white_king << 6) | black_king];
}
inline BDDIndexRefl &bdd_king_full_symmetry(uchar white_king, uchar black_king) {
  return BDD_KING_FULL_SYMMETRY[(white_king << 6) | black_king];
}



// If BOUND_KING==1 then black king will be bound to the
// a1-d1-d4 triangle. Otherwise for white king. 0<=index<1806
//
//                           KING_PAWN_SYMMETRY
//                                   --->
// (pos(white king),pos(black king))      (index, reflection) : IndexRefl
//     | : pair<uchar,uchar>         <---     (index==-1  <=>  invalid)
//     |                         KING_FS_POS
//     |
//     | BDD_KING_PAWN_SYMMETRY = KING_PS_POS o KING_PAWN_SYMMETRY
//     V
//  (pos(white king),pos(black king),reflection,valid) : BDDIndexRefl
//      BDDIndexRefl.index() gives an 5+6 (bbbbbffffff) bit index
//
extern IndexRefl KING_PAWN_SYMMETRY[64*64];//64*(white king) + black_king
extern pair<Position, Position> KING_PS_POS[1806];
extern BDDIndexRefl BDD_KING_PAWN_SYMMETRY[64*64];//64*(white king) + black_king
inline IndexRefl &king_pawn_symmetry(uchar white_king, uchar black_king) {
  return KING_PAWN_SYMMETRY[(white_king << 6) | black_king];
}
inline BDDIndexRefl &bdd_king_pawn_symmetry(uchar white_king, uchar black_king) {
  return BDD_KING_PAWN_SYMMETRY[(white_king << 6) | black_king];
}


// Gives the index in the king full symmetry enumeration
// and stores the reflection in refl.
inline int c_king_fs_index(int wk, int bk, int &refl) {
  IndexRefl &pr = king_full_symmetry(wk, bk);
  refl = pr.refl;
  return pr.index;
}
// Converts from king full symmetry enumeration to a canonical
// position of the 2 kings.
inline void d_king_fs_index(int index, int &wk, int &bk) {
  wk = KING_FS_POS[index].first;
  bk = KING_FS_POS[index].second;
}


// Gives the index in the king pawn symmetry enumeration
// and stores the reflection in refl.
inline int c_king_ps_index(int white_king, int black_king, int &refl) {
  IndexRefl &pr = KING_PAWN_SYMMETRY[(white_king << 6) | black_king];
  refl = pr.refl;
  return pr.index;
}
// Converts from king pawn symmetry enumeration to a canonical
// position of the 2 kings.
inline void d_king_ps_index(int index, int &white_king, int &black_king) {
  white_king = KING_PS_POS[index].first;
  black_king = KING_PS_POS[index].second;
}


inline bool kings_too_near(int white_king_pos, int black_king_pos) {
  int dc = (white_king_pos&7)-(black_king_pos&7);
  int dr = (white_king_pos>>3)-(black_king_pos>>3);
  return !(dc<-1 || 1<dc || dr<-1 || 1<dr);
}




extern int XX_COMPRESS[64*64];// XX_compress[(j<<6)|i], j<i
inline int& xx_compress(Position piece1, Position piece2) {
  return XX_COMPRESS[(piece1 << 6) | piece2];
}
extern pair<Position, Position> XX_DECOMPRESS[63*64/2];

extern int PP_COMPRESS[64*64];
inline int& pp_compress(Position pawn1, Position pawn2) {
  return PP_COMPRESS[(pawn1 << 6) | pawn2];
}
extern pair<Position, Position> PP_DECOMPRESS[47*48/2];

#ifdef ALLOW_5_MEN_ENDGAME

extern int XXX_COMPRESS_P1[62];
extern int XXX_COMPRESS_P2_MINUS_64[63];
extern triple<Position, Position, Position> XXX_DECOMPRESS[62*63*64/6];
inline int xxx_compress(Position piece1, Position piece2, Position piece3) {
  // sort pieces such that piece1 < piece2 < piece3
  if (piece1 > piece2) swap(piece1, piece2);
  if (piece2 > piece3) swap(piece2, piece3);
  if (piece1 > piece2) swap(piece1, piece2);

  return XXX_COMPRESS_P1[piece1] + XXX_COMPRESS_P2_MINUS_64[piece2] + piece3;
}
void verify_xxx_compress();


extern int PPP_COMPRESS_P1[54];
extern int PPP_COMPRESS_P2_MINUS_10480[55];
extern triple<Position, Position, Position> PPP_DECOMPRESS[46*47*48/6];
inline int ppp_compress(Position piece1, Position piece2, Position piece3) {
  assert(8<=piece1 && piece1<56);
  assert(8<=piece2 && piece2<56);
  assert(8<=piece3 && piece3<56);

  // sort pieces such that piece1 < piece2 < piece3
  if (piece1 > piece2) swap(piece1, piece2);
  if (piece2 > piece3) swap(piece2, piece3);
  if (piece1 > piece2) swap(piece1, piece2);

  return PPP_COMPRESS_P1[piece1] + PPP_COMPRESS_P2_MINUS_10480[piece2] + piece3;
}
void verify_ppp_compress();

#endif

void latex_print_king_fs_indexes(ostream &os);

#endif
