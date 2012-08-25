#ifndef _PIECE_POS_
#define _PIECE_POS_

typedef unsigned char Piece;
typedef unsigned char Position;

struct PiecePos {
  PiecePos() : piece(), pos() {}
  PiecePos(Piece piece, Position pos) : piece(piece), pos(pos) {}

  Piece piece;
  Position pos;
};

#endif
