#ifndef _PIECE_POS_
#define _PIECE_POS_

#include <iostream>
#include <vector>
#include <stdint.h>

typedef uint8_t Piece;
typedef uint8_t Position;

struct PiecePos {
  PiecePos() : piece(), pos() {}
  PiecePos(Piece piece, Position pos) : piece(piece), pos(pos) {}

  Piece piece;
  Position pos;
};

/**
 * Returns true if the piece positions in the vector contains a collision.
 */
bool piece_overlap(std::vector<PiecePos> pp);

#endif
