#include "piece_pos.hxx"

bool piece_overlap(std::vector<PiecePos> pp) {
  for (uint_fast8_t i=0; i<pp.size(); i++)
    for (uint_fast8_t j=0; j<i; j++)
      if (pp[i].pos == pp[j].pos) return true;
  return false;
}
