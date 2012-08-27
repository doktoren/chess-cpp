#ifndef _MOVE_AND_UNDO_
#define _MOVE_AND_UNDO_

#include "board.hxx"

#define PROTECTS_KING (1<<3)
#define CAN_ATTACK (1<<4)
#define CAN_NON_ATTACK (1<<5)
#define FURTHER_MOVEMENT_POSSIBLE (1<<6)
#define IS_NULL_MOVE (1<<7)

// Constant for Move::special_move
#define EN_PASSANT (16+32)

struct Move {
  Move() : from(ILLEGAL_POS), to(ILLEGAL_POS), special_move(0), blah(0) {}

  explicit Move(Position from, Position to) :
    from(from), to(to), special_move(0), blah(0) {}

  // castling move
  explicit Move(int castling_constant) : special_move(castling_constant), blah(0) {
    switch (special_move) {
    case WHITE_LONG_CASTLING:  from = 4; to = 2; break;
    case WHITE_SHORT_CASTLING: from = 4; to = 6; break;
    case BLACK_LONG_CASTLING:  from = 56+4; to = 56+2; break;
    case BLACK_SHORT_CASTLING: from = 56+4; to = 56+6; break;
    }
  }

  // promote_pawn_to must be color-named
  explicit Move(Position from, Position to, uint8_t blah) :
    from(from), to(to), special_move(0), blah(blah) {}

  Move(const Move& move) : from(move.from), to(move.to), 
    special_move(move.special_move), blah(move.blah) {}

  static Move NULL_MOVE() {
    Move result;
    result.blah = IS_NULL_MOVE;
    return result;
  }

  // CAG : coordinate algebraic notation
  // This notation is very simple: eg e2e4, e1g1 (castling),
  // e7e8q (pawn promotion). No special markers for piece
  // captures, checks or alike.
  Move(string cag, bool player);
  string toString() const; // CAG
  string toString2() const; // CAG extended with info of special_move and blah

  bool is_null_move() const { return blah & IS_NULL_MOVE; }

  bool is_defined() const { return from != ILLEGAL_POS; }

  bool is_pawn_promotion() const { return special_move & 0xF; }

  bool is_castling() const { return special_move &&//lazy evaluation will typically stop here.
			       (special_move & 0xF0) &&
			       special_move != EN_PASSANT; }
  bool is_en_passant() const { return special_move == EN_PASSANT; }

  int to_index() const { return (from << 6) | to; }

  Position from, to;

  // special_move can have one of the following values:
  // 0 : Ordinary move
  //
  //  2 = WKNIGHT: promote pawn to white knight
  //  3 = WBISHOP: promote pawn to white bishop
  //  4 = WROOK:   promote pawn to white rook
  //  5 = WQUEEN:  promote pawn to white queen
  //  8 = BKNIGHT: promote pawn to black knight
  //  9 = BBISHOP: promote pawn to black bishop
  // 10 = BROOK:   promote pawn to black rook
  // 11 = BQUEEN:  promote pawn to black queen
  //
  // 16 = WHITE_SHORT_CASTLING: white short castling
  // 32 = WHITE_LONG_CASTLING:  white long castling
  // 64 = BLACK_SHORT_CASTLING: black short castling
  //128 = BLACK_LONG_CASTLING:  black long castling
  //
  // 48 = EN_PASSANT: en passant
  uint8_t special_move;

  // blah contains information used by Board2 or Board2plus move generator.
  // blah is ignored by execute_move.
  // (see defines in top of this file)
  //
  // blah = b7b6b5b4b3b2b1b0
  //
  // b3b2b1b0 is used by board2 move generator:
  // b2b1b0 denotes which kind of move_iteration is performed:
  // 0: move does not belong to any iterator!
  // 1: All possible moves
  // 2: All moves from a given position
  // 3: All moves to a given position
  // 4: All moves from and to given positions (only in case of
  //    a pawn promotion will there be more than one)
  //
  // b3 is 1 if the current piece protects a king from a check
  //    (num_checks would increase if this piece vanished).
  //
  // b6b5b4 is used by board2plus move generator:
  // see #defines in top of this file
  // 
  // b7: is null move?
  uint8_t blah;
};
inline bool operator==(const Move& m1, const Move& m2) {
  return m1.from == m2.from  &&  m1.to == m2.to  &&  m1.special_move == m2.special_move;
}
inline bool operator!=(const Move& m1, const Move& m2) {
  return m1.from != m2.from  ||  m1.to != m2.to  ||  m1.special_move != m2.special_move;
}
inline bool pos_equal(Move m1, Move m2) {
  return m1.from == m2.from  &&  m1.to == m2.to;
}
// operator<< outputs newlines
ostream& operator<<(ostream& os, const Move& move);



// The idea is that when 8 bytes is used, no compression is necessary
// and hopefully the variables will be copied in blocks (of more than 1
// byte each) in execute-/undo-move, making the program faster.
struct Undo {
  Undo() :  en_passant(0), castling(0), moves_played_since_progress(0),
	    player(0), num_checks(0), threat_pos(0), captured_piece(0), padding(0) {}


  Undo(Position en_passant, uint8_t castling, uint8_t moves_played_since_progress,
	bool player, uint8_t num_checks, Position threat_pos, Piece captured_piece) :
    en_passant(en_passant), castling(castling),
    moves_played_since_progress(moves_played_since_progress),
    player(player), num_checks(num_checks), threat_pos(threat_pos),
    captured_piece(captured_piece), padding(0) {}

  Undo(const Undo &u) :
    en_passant(u.en_passant), castling(u.castling),
    moves_played_since_progress(u.moves_played_since_progress),
    player(u.player), num_checks(u.num_checks), threat_pos(u.threat_pos),
    captured_piece(u.captured_piece), padding(u.padding) {}

  string toString() const;


  // #######   BEGIN board.hxx 4 byte block ##########
  Position en_passant;
  uint8_t castling;
  uint8_t moves_played_since_progress;
  bool player;
  // #######    END board.hxx 4 byte block  ##########

  // #######  BEGIN board_2.hxx 2 byte block  ########
  uint8_t num_checks;
  Position threat_pos; // only defined if num_checks != 0
  // #######   END board_2.hxx 2 byte block   ########

  Piece captured_piece;
  
  // padding must be 0 for operator== to work
  uint8_t padding;
};
// operator<< outputs newlines
ostream& operator<<(ostream& os, const Undo& undo);

// Undo may not be compared on threat_pos as this is not unique when
// num_checks > 1
// Also, moves_played_since_progress and player are not checked!
inline bool operator==(const Undo& u1, const Undo& u2) {
  return u1.en_passant==u2.en_passant  &&  u1.castling==u2.castling  &&
    u1.num_checks==u2.num_checks  &&  u1.captured_piece==u2.captured_piece;
}
inline bool operator!=(const Undo& u1, const Undo& u2) {
  return !(u1==u2);
}

#endif
