#include "endgame_piece_pos.hxx"

#include <assert.h>

#include "../board.hxx"

using namespace std;

bool piece_overlap(vector<PiecePos> pp) {
  for (vector<PiecePos>::size_type i=0; i<pp.size(); i++)
    for (vector<PiecePos>::size_type j=0; j<i; j++)
      if (pp[i].pos == pp[j].pos) return true;
  return false;
}

// Sorting order : KQRBNPkqrbnp
const bool GT[16][16] =
{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0,1,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,0,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

bool swap_piece_pos(std::vector<PiecePos> &pieces,
    bool symmetric_endgame_and_btm)
{
  switch (pieces.size()) {
  case 2:
    // NEW!
    if (symmetric_endgame_and_btm) swap(pieces[0], pieces[2]);
    return symmetric_endgame_and_btm;
  case 3:
    if (PIECE_KIND[pieces[1].piece] == KING) {
      PiecePos weak_king = pieces[0];
      pieces[0] = pieces[1];
      pieces[1] = pieces[2];
      pieces[2] = weak_king;
      return true;
    }
    return false;
  case 4:
    if (PIECE_KIND[pieces[1].piece] == KING) {
      PiecePos weak_king = pieces[0];
      pieces[0] = pieces[1];
      pieces[1] = pieces[2];
      pieces[2] = pieces[3];
      pieces[3] = weak_king;
      return true;
    }
    if (PIECE_KIND[pieces[2].piece] == KING) {
      if (PIECE_KIND[pieces[1].piece] < PIECE_KIND[pieces[3].piece]) {
        swap(pieces[0], pieces[2]);
        swap(pieces[1], pieces[3]);
        return true;
      }
      // NEW!
      if (symmetric_endgame_and_btm) {
        swap(pieces[0], pieces[2]);
        swap(pieces[1], pieces[3]);
        return true;
      }
    }
    // PIECE_KIND[pieces[3]] == KING
    return false;
#ifdef ALLOW_5_MEN_ENDGAME
  case 5:
    if (PIECE_KIND[pieces[1].piece] == KING) {
      PiecePos weak_king = pieces[0];
      pieces[0] = pieces[1];
      pieces[1] = pieces[2];
      pieces[2] = pieces[3];
      pieces[3] = pieces[4];
      pieces[4] = weak_king;
      return true;
    }
    if (PIECE_KIND[pieces[2].piece] == KING) {
      PiecePos weak_king_piece = pieces[1];
      pieces[1] = pieces[3];
      pieces[3] = pieces[0];
      pieces[0] = pieces[2];
      pieces[2] = pieces[4];
      pieces[4] = weak_king_piece;
      return true;
    }
    return false;
#endif
  }
  assert(0);
  return false;
}

void sort_piece_pos(vector<PiecePos> &pieces) {
  switch(pieces.size()) {
  case 2:
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    return;
  case 3:
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    if (GT[pieces[1].piece][pieces[2].piece]) swap(pieces[1], pieces[2]);
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    return;
  case 4:
    // Partial sort pieces[0..1] and pieces[2..3]
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    if (GT[pieces[2].piece][pieces[3].piece]) swap(pieces[2], pieces[3]);

    // Find smallest and largest element
    if (GT[pieces[0].piece][pieces[2].piece]) swap(pieces[0], pieces[2]);
    if (GT[pieces[1].piece][pieces[3].piece]) swap(pieces[1], pieces[3]);

    // Sort middle elements
    if (GT[pieces[1].piece][pieces[2].piece]) swap(pieces[1], pieces[2]);
    return;
#ifdef ALLOW_5_MEN_ENDGAME
  case 5:
    // start by sorting pieces[0,1,3,4] like sort_4_piece_pos
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    if (GT[pieces[3].piece][pieces[4].piece]) swap(pieces[3], pieces[4]);
    if (GT[pieces[0].piece][pieces[3].piece]) swap(pieces[0], pieces[3]);
    if (GT[pieces[1].piece][pieces[4].piece]) swap(pieces[1], pieces[4]);
    if (GT[pieces[1].piece][pieces[3].piece]) swap(pieces[1], pieces[3]);

    // find the place for pieces[2]
    if (GT[pieces[1].piece][pieces[2].piece]) {
      swap(pieces[1], pieces[2]);
      if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);

    } else if (GT[pieces[2].piece][pieces[3].piece]) {

      swap(pieces[2], pieces[3]);
      if (GT[pieces[3].piece][pieces[4].piece]) swap(pieces[3], pieces[4]);
    }

    assert(!GT[pieces[0].piece][pieces[1].piece]);
    assert(!GT[pieces[1].piece][pieces[2].piece]);
    assert(!GT[pieces[2].piece][pieces[3].piece]);
    assert(!GT[pieces[3].piece][pieces[4].piece]);

    return;
#endif
  }
  assert(0);
}

// If endgame is KBKP, and bishop captured by pawn is being promoted to queen,
// then s is K_KQ  (and stm is BLACK).
// Result will be the needed stm, hence BLACK in this case (B->W->B)
// Will change p if it contains a zero entry
int endgame_dependency_needed_stm(Piece *p, int num_pieces, int stm) {
  assert(stm==0  ||  stm==1);
  for (int i=0; i<num_pieces; i++) {
    if (p[i] == 0) {
      --num_pieces;
      for (int j=i; j<num_pieces; j++) {
        p[j] = p[j+1];
        assert(p[j] != 0);
      }
      break;
    }
  }

  bool symmetric = true;
  for (int i=0; i<(num_pieces>>1); i++)
    if (p[i] != p[i + (num_pieces>>1)]) {
      symmetric = false;
      break;
    }

  vector<PiecePos> pp(num_pieces);
  for (int i=0; i<num_pieces; i++)
    pp[i].piece = p[i];

  return !stm ^ swap_piece_pos(pp, stm && symmetric);
}
