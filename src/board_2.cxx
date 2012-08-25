#include "board_2.hxx"

#include <assert.h>
#include <bitset>

#include "util/help_functions.hxx"
#include "board_move_tables.hxx"
#include "board_2_king_lines.hxx"
#include "board_define_position_constants.hxx"
#include "endgames/endgame_castling.hxx"

#define INVALID_DIRECTION 16
#define KNIGHT_DIRECTION 8

//##########################################

uchar CHECK_TABLE[0x10000];//is 0,1 or 2
uchar DIRECTION[64][64];// "unsigned" direction: [0..3] or INVALID_DIRECTION or KNIGHT_DIRECTION

// "signed" direction: [0..7] or INVALID_DIRECTION or KNIGHT_DIRECTION
// SDIRECTION[x][y] is the direction from x to y
uchar SDIRECTION[64][64];

Position DIRECTION_MOVE_TABLE[8][64];

// BIT_BOARDS[13][x] is white pawns reflected in a1-h8 line
// BIT_BOARDS[14][x] is black pawns reflected in a1-h8 line
ull BIT_BOARDS[13+2][64];

ull BB_LINES[64][64];

Board2::Board2() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Board2 constructor called.\n";

  static bool tables_initialized = false;
  if (!tables_initialized) {
    cerr << "board initializes tables.\n";
    initialize_move_tables();
    init_CHECK_TABLE();
    init_DIRECTION();
    init_bitboards();
    init_bitboard_lines();
    tables_initialized = true;
  }

  // new_game not called here
}

/*

After Board::loadFEN, num_checks = 0
After ...1, num_checks = 1
After ..., num_checks = 1
AFTER SETUP GAME:


 */


bool Board2::loadFEN(string FEN) {
  if (!Board::loadFEN(FEN)) return false;
  //cerr << "After Board::loadFEN, num_checks = " << (int)num_checks << "\n";
  set_check_invariants();
  //cerr << "After ...1, num_checks = " << (int)num_checks << "\n";
  if (king_capture_possible()) {
    cerr << "king capture possible!\n";
    return false;
  }
  //cerr << "After ..., num_checks = " << (int)num_checks << "\n";

  // Ignore the fact that no legal position has num_checks > 2
  // (To get same number as Nalimov in endgame tables)
  return true;
  // return num_checks <= 2;
}

void Board2::reset_all() {
  threat_pos = 0; // whatever
  threat_elim = 0; // whatever

  num_checks = 0;
  memset(board_lines, 0, sizeof(board_lines));

  king_pos[0] = king_pos[1] = 0;

  // ARGH!!!!!!!!!!!!!!
  //  for (int i=0; i<13; i++)
  //  bit_boards[i][0] = bit_boards[i][1] = 0;

  for (int i=0; i<12; i++)
    bit_boards[i][0] = bit_boards[i][1] = 0;

  Board::reset_all();
}

int Board2::calc_game_status_ignore_50_move_rule_and_move_repetition() {
  // Insufficient material left?
  // Sufficient material is:
  // a) a pawn, a rook or a queen
  // b) 2 knights or 1 knight and a bishop
  // c) 2 bishop on different colors
  //
  // If eg. white has sufficient material left, then
  // both white_a and white_b will be true (iff).
  //
  // Todo: both players have bishops left but all on
  // same color => draw (probably not worth the effort).
  if (insufficient_material()) {
    game_status_reason = INSUFFICIENT_MATERIAL;
    return GAME_DRAWN;
  }

  // Check if player has any legal moves
  Move move = moves();
  if (next_move(move)) {
    // Game continues...
    return game_status_reason = GAME_OPEN;
  }

  // No legal moves!
  if (num_checks) {
    // Player is check mate
    if (player) {
      game_status_reason = BLACK_IS_CHECK_MATE;
      return WHITE_WON;
    } else {
      game_status_reason = WHITE_IS_CHECK_MATE;
      return BLACK_WON;
    }
  } else {
    // Stalemate
    game_status_reason = STALEMATE;
    return GAME_DRAWN;
  }
}

int Board2::calc_game_status() {
  // Position occured 3 times is not counted for here!

  // 50 moves rule:
  if (moves_played_since_progress >= 100) {
    game_status_reason = FIFTY_MOVE_RULE;
    return GAME_DRAWN;
  }

  return calc_game_status_ignore_50_move_rule_and_move_repetition();
}

//##################################################################

// Notes about how the move generators interpret the move structure:
// 
// When initializing a move iterator (Move with blah != 0),
// from will be set to the position of the first piece of the active player.
// to will be set equal to from, which means that none of the piece moves
// have currently been returned.
//
// Each time from changes (also first time), 2 things will be calculated:
//
// 1) below is now updated when pieces are moved instead 
// 1) if the piece is a king, then the bit_board is updated to contain
//    the threats of the opponent king, knights and pawns (short range pieces).
//
// 2) Check whether the piece will only be able to move along some diagonal
//    because it blocks its own king against a threat.
//    This information will be stored in the move structure.

static const bool DESTINATION_FIXED[6] =
{false, false, false, true, true, false};
// static const bool ORIGIN_FIXED[6] = {false, false, true, false, true, true};

static const Move EMPTY_MOVE_ITERATOR(ILLEGAL_POS, ILLEGAL_POS, 7);


Move Board2::moves() const {
  board_iterate(pos) {
    if (PIECE_COLOR[board[pos]] == player) {
      return check_if_moved(pos) ?
          Move(pos, pos, 1 | PROTECTS_KING) :
          Move(pos, pos, 1);
    }
  }
  print_board(cerr);
  assert(0);
  return EMPTY_MOVE_ITERATOR;
}

Move Board2::moves_from_pos(Position pos) const {
  if (PIECE_COLOR[board[pos]] == player) {
    return check_if_moved(pos) ?
        Move(pos, pos, 2 | PROTECTS_KING) :
        Move(pos, pos, 2);
  } else return EMPTY_MOVE_ITERATOR;
}

Move Board2::moves_to_dest(Position dest) const {
  board_iterate(pos) {
    if (PIECE_COLOR[board[pos]] == player  &&
        PIECE_NEXT(board[pos], pos, dest) != IMPOSSIBLE_MOVE) {
      // This piece *might* be able to reach dest
      // The code below can be optimized (if dest not on forced direction)
      return check_if_moved(pos) ?
          Move(pos, dest | 0x80, 3 | PROTECTS_KING) :
          Move(pos, dest | 0x80, 3);
    }
  }
  return EMPTY_MOVE_ITERATOR;
}

Move Board2::moves_from_to(Position pos, Position dest) const {
  if (PIECE_COLOR[board[pos]] == player) {

    // Unnescessary check. Might speed up or slow down. Whatever.
    if (PIECE_NEXT(board[pos], pos, dest) == IMPOSSIBLE_MOVE)
      return EMPTY_MOVE_ITERATOR;

    return check_if_moved(pos) ?
        Move(pos, dest | 0x80, 4 | PROTECTS_KING) :
        Move(pos, dest | 0x80, 4);
  } else return EMPTY_MOVE_ITERATOR;
}

//##################################################################

// This function assumes that the moving piece is a king
// Returns true if
// a) the move is a castling move
// b) something prevents the player to castle
bool Board2::is_illegal_castling_move(Move move) const {
  //cerr << "Checking " << move.toString() << "\n";

  if (CASTLING[move.from] && CASTLING[move.to]) {
    //cerr << "...is a castling move...\n";

    // Cannot castle if checked
    if (num_checks) return true;

    // gah
    if (board[move.to]) return true;

    // Have player lost castling option?
    if (!(castling & CASTLING[move.to])) return true;

    // Are there any blocking pieces?
    // (mid = (move.from+move.to)/2  has already been checked)
    if (CASTLING[move.to] & (WHITE_LONG_CASTLING + BLACK_LONG_CASTLING)) {
      // Check also that b1/b8 is not occupied
      if (board[move.to - 1]) return true;
    }

    // cerr << "...so far it seems legal\n";

    // Will the king have to pass any checking positions?
    // (legal_move will check that move.to is not threatened).

    return (check_if_king_placed((move.from + move.to) >> 1));

  } else return false;
}

// The path must be obstacle free.
// If it is a king move, bit_board must be updated.
// (move.blah & PROTECTS_KING  <=>  Check if piece moved) must hold.
// Will update move.special_move if it returns true.
bool Board2::legal_move(Move& move) const {
  assert(legal_pos(move.from) && legal_pos(move.to) && board[move.from]);

  if (PIECE_COLOR[board[move.to]] == player) return false;

  if (PIECE_KIND[board[move.from]] == KING) {
    if (is_illegal_castling_move(move)) return false;
    if (check_if_king_placed(move.to)) return false;
    move.special_move = CASTLING[move.from] & CASTLING[move.to];
    // cerr << move.toString() << " is a legal move(?)\n";
    return true;
  } else {
    move.special_move = 0;

    if (PIECE_KIND[board[move.from]] == PAWN) {
      if ((move.from ^ move.to) & 7) {
        // A diagonal move.
        // The pawn will have to capture something
        if (!board[move.to]) {
          if (move.to != en_passant) return false;
          move.special_move = EN_PASSANT;
        }
      } else {
        // Cannot capture
        if (board[move.to]) return false;
      }

      if (ROW[move.to]==0  ||  ROW[move.to]==7)
        move.special_move = 6*player + QUEEN;

      if (num_checks  &&  move.to == en_passant) {
        // If the king is checked af an en passant move, then the
        // only possible checking piece is the en passant pawn.
        // This capturing pawn move is legal if the attacking pawn
        // doesn't protect the king. (the removal of the checking
        // pawn can't result in a new check).
        return !(move.blah & PROTECTS_KING);
      }
    }

    if (num_checks) {
      if (num_checks == 2) return false;
      if (move.blah & PROTECTS_KING) {
        // ARGHH!!!:
        // Denne kode tager højde for tilfælde som fx loadfen K1r1Q1q1///////7k w - -
        // check_if_moved will være true for Q, da der vil
        // være en skakkende brik på denne linje.
        // loadfen 4Q1q1/K1r5//////7k w - -
        // DI... = DI... nødvendiggøres af fx loadfen K2Qq/2n//////k w - -
        return threat_elim[move.to]  &&
            DIRECTION[move.from][king_pos[player]] == DIRECTION[move.from][move.to];
      }
      return threat_elim[move.to];
    } else {
      if (move.blah & PROTECTS_KING)
        return DIRECTION[move.from][king_pos[player]] == DIRECTION[move.from][move.to];
      return true;
    }
  }
}

// assumptions made about move
bool Board2::obstacle_free_move(Move move) const {
  if (IS_KNIGHT_OR_KING[board[move.from]]) return true;
  if (PIECE_KIND[board[move.from]] == PAWN) {
    // obst. free if either  a) the move is not move 2 forward, or
    //                       b) the passed position is empty
    return (((move.from^move.to) & 0x18) != 0x10  ||
        board[(move.from + move.to)>>1] == 0);
  }
  // SDIRECTION must be a valid direction!
  // DIRECTIONS convert from direction number (in 0..7) to a step
  int step = DIRECTIONS[SDIRECTION[move.from][move.to]];
  for (int pos = move.from + step; pos != move.to; pos += step)
    if (board[pos]) return false;
  return true;
}

bool Board2::next_move_fixed_destination(Move &move, Piece piece_kind) const {
  if (move.to & 0x80) {
    // Try <move.from, move.to-0x80> next
    move.to -= 0x80;
    if (!piece_kind  ||  PIECE_KIND[board[move.from]] == piece_kind)
      goto examine_if_move_is_legal;
  } 

  // Iterate move.from
  if ((move.blah & 0x7) == 4) {
    // move.from is fixed. Nothing more to do.
    return false;
  }

  do {
    // find next move.from
    // (This code will not be executed in the first call)
    do {
      ++move.from;
    } while (PIECE_COLOR[board[move.from]] != player  ||
        // only addition caused by extra parameter:
        (piece_kind  &&  PIECE_KIND[board[move.from]] != piece_kind));

    if (!legal_pos(move.from)) return false;

    if (check_if_moved(move.from)) move.blah |= PROTECTS_KING;
    else move.blah &= ~PROTECTS_KING;

    examine_if_move_is_legal:
    // Ok, now examine the move <move.from, move.to>
    if (PIECE_NEXT(board[move.from], move.from, move.to) != IMPOSSIBLE_MOVE  &&
        obstacle_free_move(move)  &&
        legal_move(move)) {
      // Move is legal
      return true;
    } else {
      // Move is illegal
      if ((move.blah & 0x7) == 4) {
        // move.from is fixed. Nothing more to do.
        return false;
      }
    }
  } while (true);
}


// If piece_kind is defined (!=0), then only moves involving that
// kind of pieces are returned.
bool Board2::next_move(Move &move, Piece piece_kind) const {
  assert(move.blah & 7); // cerr << "next_move called on:\n" << move;

  // The line below is not nescessary if next_move will never
  // again be called on the same move after it has previously returned false.
  if (!legal_pos(move.from)) return false;

  if (move.special_move  &&  move.is_pawn_promotion()) {
    if (PIECE_KIND[move.special_move] != KNIGHT) {
      --move.special_move;
      return true;
    }
  }

  if (DESTINATION_FIXED[move.blah & 0x7])
    return next_move_fixed_destination(move, piece_kind);

  do {
    // Iterate move.to
    if (board[move.to]) move.to = PIECE_JUMP(board[move.from], move.from, move.to);
    else move.to = PIECE_NEXT(board[move.from], move.from, move.to);

    while (legal_pos(move.to)) {
      if (legal_move(move)) return true;

      //cerr << "The move...\n" <<  move << "...is illegal\n";

      if (board[move.to]) move.to = PIECE_JUMP(board[move.from], move.from, move.to);
      else move.to = PIECE_NEXT(board[move.from], move.from, move.to);
    }

    // No more legal move.to => iterate move.from

    if ((move.blah & 0x7) == 2) {
      // Origin fixed
      move.from = ILLEGAL_POS;
      return false;
    }

    do {
      ++move.from;
    } while (PIECE_COLOR[board[move.from]] != player  ||
        // only addition caused by extra parameter:
        (piece_kind  &&  PIECE_KIND[board[move.from]] != piece_kind));

    // cerr << "(" << (int)move.from << ") Next from: " << POS_NAME[move.from] << "\n";

    if (!legal_pos(move.from)) return false;

    move.to = move.from;

    if (check_if_moved(move.from)) move.blah |= PROTECTS_KING;
    else move.blah &= ~PROTECTS_KING;
  } while (true);
}


//######################################################################
//######################################################################
//######################################################################


Undo Board2::execute_move(Move move) {
  Undo result(en_passant, castling, moves_played_since_progress, player,
      num_checks, threat_pos, board[move.to]);

  en_passant = ILLEGAL_POS;
  castling &= CASTLING_LOST[move.from] & CASTLING_LOST[move.to];
  // threat_pos = 0; will only be used if num_checks != 0
  ++moves_played_since_progress;

  num_checks = 0;

  if (board[move.to]  ||  PIECE_KIND[board[move.from]] == PAWN)
    moves_played_since_progress = 0;

  if (move.special_move) {

    if (move.is_pawn_promotion()) {
      // rækkefølgen af remove og insert er nødvendig for at brikken
      // kan beholde sit nummer (piece_number's stack-system)
      remove_piece(move.from);
      insert_piece(move.to, move.special_move);

    } else if (move.special_move == EN_PASSANT) {

      // Kill the pawn.
      // Important: move piece first, then capture
      // Illustated by example below (gxf6)
      //Num checks = 0
      //    a b c d e f g h
      //  +-----------------+    |
      //8 |       r   k     | 8  | 39w
      //7 | p             p | 7  |
      //6 |     q     e p P | 6  | White has lost castling
      //5 |   p       p P   | 5  | Black has lost castling
      //4 |       b N       | 4  |
      //3 | P         Q     | 3  | moves played since progress = 0
      //2 |             K   | 2  |
      //1 |           R     | 1  | en passant at f6
      //  +-----------------+    |
      //    a b c d e f g h
      move_piece(move.from, move.to);
      if (player) remove_piece(move.to + 8);
      else remove_piece(move.to - 8);

    } else { // castling

      switch (move.special_move) {
      case WHITE_LONG_CASTLING:  move_piece(e1, c1); move_piece(a1, d1); break;
      case WHITE_SHORT_CASTLING: move_piece(e1, g1); move_piece(h1, f1); break;
      case BLACK_LONG_CASTLING:  move_piece(e8, c8); move_piece(a8, d8); break;
      case BLACK_SHORT_CASTLING: move_piece(e8, g8); move_piece(h8, f8); break;
      }
    }

  } else {

    move_piece(move.from, move.to);

    if (PIECE_KIND[board[move.to]] == PAWN) {
      if (((move.from ^ move.to) & 0x18) == 0x10) {
        // Moved 2 positions => Allow for en passant
        Piece tmp = WPAWN+BPAWN-board[move.to];
        if ((COLUMN[move.to] > 0  &&  board[move.to-1] == tmp)  ||
            (COLUMN[move.to] < 7  &&  board[move.to+1] == tmp)) {
          // There is an enemy pawn ready to take advantage of the
          // en passant. It is to bothersome to tjeck if this pawn
          // will be unable to use the en passant because of some check.
          en_passant = (move.from + move.to) >> 1;
        }
      }
    }
  }

  player ^= 1;
  ++moves_played;

  return result;
}


void Board2::undo_move(Move move, Undo undo) {
  // The move_piece operations require that player is changed already now.
  player ^= 1;
  assert(moves_played > 0);
  --moves_played;

  if (move.special_move) {

    if (move.is_pawn_promotion()) {
      // rækkefølgen af remove og insert er nødvendig for at brikken
      // kan beholde sit nummer (piece_number's stack-system)
      remove_piece(move.to);
      insert_piece(move.from, player ? BLACK_PAWN : WHITE_PAWN);

    } else if (move.is_en_passant()) {
      if (player) insert_piece(move.to + 8, WPAWN);
      else insert_piece(move.to - 8, BPAWN);
      move_piece(move.to, move.from);

    } else { // castling
      assert(move.is_castling());
      switch (move.special_move) {
      case WHITE_LONG_CASTLING:  move_piece(c1, e1); move_piece(d1, a1); break;
      case WHITE_SHORT_CASTLING: move_piece(g1, e1); move_piece(f1, h1); break;
      case BLACK_LONG_CASTLING:  move_piece(c8, e8); move_piece(d8, a8); break;
      case BLACK_SHORT_CASTLING: move_piece(g8, e8); move_piece(f8, h8); break;
      }
    }

  } else {
    move_piece(move.to, move.from);
  }

  if (undo.captured_piece)
    insert_piece(move.to, undo.captured_piece);

  { // Copy 4 byte block to Board
    en_passant = undo.en_passant;
    castling = undo.castling;
    moves_played_since_progress = undo.moves_played_since_progress;
    player = undo.player;// player already updated. Updated again due to optimization
  }

  { // Copy 2 byte block to Board2
    num_checks = undo.num_checks;
    threat_pos = undo.threat_pos;
  }

  threat_elim = BB_LINES[threat_pos][king_pos[player]];
}

//##################################################################

// insert_piece, remove_piece and move_piece are declared virtual.
// It will be very usefull to be able to extend them later on.

void Board2::bit_board_insert(Position pos, Piece piece, bool player) {
  ull rest = BIT_BOARDS[piece][pos];
  if (rest[king_pos[player^1]]) {
    //cerr << "Add check for " << PIECE_NAME[piece] << " at " << POS_NAME[pos] << '\n';

    // cerr << "++num_checks; " << POS_NAME[king_pos[player^1]] << " " << POS_NAME[pos] << "\n";
    ++num_checks;
    threat_pos = pos;
    threat_elim = BB_LINES[pos][pos];
    // When undone, threat_elim will be set to BB_LINES[pos][king_pos[player^1]];
    // but this has the same value
  }

  for (int i=0; rest.as_bool(); i++) {
    assert(i<12);
    ull new_rest = bit_boards[i][player] & rest;
    bit_boards[i][player] |= rest;
    rest = new_rest;
  }
}
void Board2::bit_board_remove(Position pos, Piece piece, bool player) {
  bit_boards[0][player] &= ~BIT_BOARDS[piece][pos];
  int i=0;
  ull diff = (~bit_boards[i][player]) & bit_boards[i+1][player];
  while (diff.as_bool()) {
    assert(i<11);
    bit_boards[i][player] |= diff;
    bit_boards[++i][player] &= ~diff;
    diff = (~bit_boards[i][player]) & bit_boards[i+1][player];
  }
}

void Board2::remove_piece(Position pos) {
  assert(legal_pos(pos));
  Piece piece = board[pos];
  assert(piece);
  if (IS_SHORT_DISTANCE_PIECE[piece])
    bit_board_remove(pos, piece, PIECE_COLOR[piece]);
  king_line_remove_piece(pos);
  board[pos] = 0;

  piece_count.as_pattern -= PIECE_COUNT_CONSTANTS[piece];
  endgame_hashing_insufficient_material.as_pattern -=
      ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[piece][POS_COLOR[pos]];
}

void Board2::insert_piece(Position pos, Piece piece) {
  //cerr << "insert(" << PIECE_CHAR[piece] << ", " << POS_NAME[pos] << "), " << toString(insuf_material.as_pattern, 4, 16) << ", " << toString(endgame_hashing.as_pattern, 8, 16) << "\n";

  assert(legal_pos(pos)  &&  piece  &&  piece<=BKING);


  piece_count.as_pattern += PIECE_COUNT_CONSTANTS[piece];
  endgame_hashing_insufficient_material.as_pattern +=
      ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[piece][POS_COLOR[pos]];

  if (IS_SHORT_DISTANCE_PIECE[piece]) {
    bit_board_insert(pos, piece, PIECE_COLOR[piece]);

    if (PIECE_KIND[piece] == KING)
      king_pos[player] = pos;
  }

  if (board[pos]) {
    // capture piece
    piece_count.as_pattern -= PIECE_COUNT_CONSTANTS[board[pos]];
    endgame_hashing_insufficient_material.as_pattern -=
        ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[pos]][POS_COLOR[pos]];

    if (IS_SHORT_DISTANCE_PIECE[board[pos]])
      bit_board_remove(pos, board[pos], PIECE_COLOR[board[pos]]);

    king_line_replace_piece(pos, piece);
    board[pos] = piece;

  } else {

    king_line_insert_piece(pos, piece);
    board[pos] = piece;
  }
}

void Board2::move_piece(Position from, Position to) {
  assert(legal_pos(from)  &&  legal_pos(to)  &&  board[from]);
  Piece piece = board[from];

  assert(ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[from]][POS_COLOR[from]] ==
      ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[from]][POS_COLOR[to]]);

  if (IS_SHORT_DISTANCE_PIECE[piece]) {
    if (PIECE_KIND[piece] == KING)
      king_pos[player] = to;

    bit_board_remove(from, piece, player);
    bit_board_insert(to, piece, player);
  }

  if (board[to]) {
    // a capture move.
    // In this case the order should be "first remove, then insert"
    // otherwice a possible check might be counted twice

    // capture piece
    piece_count.as_pattern -= PIECE_COUNT_CONSTANTS[board[to]];
    endgame_hashing_insufficient_material.as_pattern -=
        ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[to]][POS_COLOR[to]];

    if (IS_SHORT_DISTANCE_PIECE[board[to]])
      bit_board_remove(to, board[to], PIECE_COLOR[board[to]]);

    king_line_remove_piece(from);
    board[from] = 0;

    board[to] = piece;
    king_line_replace_piece(to, piece);

  } else {
    // Not a capture move.
    // In this case the from position can not possible result in a check.
    // Hence the order should be "first insert, then remove"
    // Otherwice "K2R3r" -> "KR5r" would result in a check when removing.

    board[to] = board[from];
    king_line_insert_piece(to, board[from]);

    king_line_remove_piece(from);
    board[from] = 0;
  }
}

void Board2::place_kings(Position white_king, Position black_king) {
  king_pos[WHITE] = white_king;
  king_pos[BLACK] = black_king;
  Board::place_kings(white_king, black_king);
}

//#####################################################################


bool Board2::clr_board2(Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Board2 &b = *dynamic_cast<Board2 *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Board2, help:\n"
        << "    print board  or  pb  or  dir\n"
        << "      - print board\n"
        << "    print move list  or  pml\n"
        << "    print moves to cr  or  pmt cr\n"
        << "    print moves from cr  or  pmf cr\n"
        << "    print moves from to cr cr  or  pmft cr cr\n"
        << "      - the 4 above commands can be extended with a character,\n"
        << "      - specifying that only moves with such a piece will be\n"
        << "      - returned. (eg. -print moves to e4 r, for rook moves (not R))\n"
        << "    print bit boards  or  pbb\n"
        << "    print king threats  or  pkt\n"
        << "    print threat pos  or  ptp\n"
        << "    retro moves [dest] or  rm [dest]\n"
        << "      - Get the complete list of moves leading to this position.\n"
        << "      - if dest is specified, then only consider moves for this piece.\n"
        << "    retro move n  or  rm n\n"
        << "      - undo retro move number n\n"
        << "    test retro moves  or  trm\n"
        << "      - test the validity of the retro moves.\n";

  } else if (dot_demand(p, 1, "dir")  ||
      dot_demand(p, 2, "print", "board")) {
    b.print_board(os);

  } else if (dot_demand(p, 3, "print", "move", "list")) {
    b.print_moves(cerr);
  } else if (dot_demand(p, 4, "print", "move", "list", (ptr_int)1)) {
    b.print_moves(cerr, ILLEGAL_POS, ILLEGAL_POS,
        PIECE_KIND[char_to_piece(parse_result[0][0])]);

  } else if (dot_demand(p, 4, "print", "moves", "to", (ptr_int)2)) {
    b.print_moves(cerr, ILLEGAL_POS, strToPos(parse_result[0]));
  } else if (dot_demand(p, 5, "print", "moves", "to", (ptr_int)2, (ptr_int)1)) {
    b.print_moves(cerr, ILLEGAL_POS, strToPos(parse_result[1]),
        PIECE_KIND[char_to_piece(parse_result[1][0])]);

  } else if (dot_demand(p, 4, "print", "moves", "from", (ptr_int)2)) {
    b.print_moves(cerr, strToPos(parse_result[0]));
  } else if (dot_demand(p, 5, "print", "moves", "from", (ptr_int)2, (ptr_int)1)) {
    b.print_moves(cerr, strToPos(parse_result[0]), ILLEGAL_POS,
        PIECE_KIND[char_to_piece(parse_result[1][0])]);

  } else if (dot_demand(p, 6, "print", "moves", "from", "to", (ptr_int)2, (ptr_int)2)) {
    b.print_moves(cerr, strToPos(parse_result[0]), strToPos(parse_result[1]));
  } else if (dot_demand(p, 7, "print", "moves", "from", "to", (ptr_int)2, (ptr_int)2, (ptr_int)1)) {
    b.print_moves(cerr, strToPos(parse_result[0]), strToPos(parse_result[1]),
        PIECE_KIND[char_to_piece(parse_result[2][0])]);

  } else if (dot_demand(p, 3, "print", "bit", "boards")) {
    b.print_bit_boards(cerr);

  } else if (dot_demand(p, 3, "print", "king", "threats")) {
    b.print_king_threats(cerr);

  } else if (dot_demand(p, 3, "print", "threat", "pos")) {
    b.print_threat_pos(cerr);

  } else if (dot_demand(p, 2, "retro", "moves")) {
    vector<triple<Move,Undo,int> > rm = b.get_retro_moves(true, true, true, true);
    os << "Complete list of the " << rm.size() << " retro move(s) from current position:\n";
    for (uint i=0; i<rm.size(); i++)
      os << i << ":\t" << rm[i].first.toString2() << "\t"
      << rm[i].third << "\t" << rm[i].second.toString() << "\n";

  } else if (dot_demand(p, 3, "retro", "moves", (ptr_int)2)) {
    vector<triple<Move,Undo,int> > rm = b.get_retro_moves(true, true, true, true);
    os << "List of retro move(s) from current position with destination " << parse_result[0] << "\n";
    for (uint i=0; i<rm.size(); i++) {
      if (POS_NAME[rm[i].first.to] == parse_result[0])
        os << i << ":\t" << rm[i].first.toString2() << "\t"
        << rm[i].third << "\t" << rm[i].second.toString() << "\n";
    }

  } else if (dot_demand(p, 3, "retro", "move", (ptr_int)0)) {
    vector<triple<Move,Undo,int> > rm = b.get_retro_moves(true, true, true, true);
    uint n = atoi(parse_result[0].c_str());
    if (0<=n  &&  n<rm.size()) {
      os << "Undoing retro move number " << n;
      if (rm[n].third) {
        os << " (transf " << rm[n].third << " nescessary)\n";
        if (!b.transform_board(rm[n].third)) {
          os << "Error: board could not be transformed!?\n";
          exit(1);
        }
        b.print_board(os);
        os << "board has been transformed.\n";
      } else os << "\n";
      b.undo_move(rm[n].first, rm[n].second);
      b.print_board(os);
    } else {
      os << "Undefined retro number (type \"retro moves\" to see list)\n";
    }

  } else if (dot_demand(p, 3, "test", "retro", "moves")) {
    vector<triple<Move,Undo,int> > rm = b.get_retro_moves(true, true, true, true);
    os << "Testing each of the " << rm.size() << " retro move(s) from current position:\n";
    bool ok = true;
    for (uint i=0; i<rm.size(); i++) {
      os << i << ":\t" << rm[i].first.toString2() << "\t"
          << rm[i].third << "\t" << rm[i].second.toString() << "...";

      if (rm[i].third) b.transform_board(rm[i].third);

      b.undo_move(rm[i].first, rm[i].second);

      bool found = false;
      Move move = b.moves();
      while (b.next_move(move)) {
        found |= move == rm[i].first;

        Undo undo = b.execute_move(move);
        b.undo_move(move, undo);
      }

      b.execute_move(rm[i].first);

      if (rm[i].third) b.inv_transform_board(rm[i].third);

      os << (found ? "ok\n" : "failed!\n");
      if (!found) ok = false;
    }
    if (ok) {
      cerr << "All moves succesfully executed from each of the possible\n"
          << "positions preceding this one.\n";
    } else {
      cerr << "FAILED!!!\n";
    }

  } else return false;
  return true;
}

void Board2::print_board(ostream& os) const {
  os << "Num checks = " << (int)num_checks;
  if (num_checks)
    os << ", threat_pos = " << POS_NAME[threat_pos];
  os << '\n';
  Board::print_board(os);
}

void Board2::print_moves(ostream& os, Position from, Position to, Piece piece) {
  os << "List of moves";
  if (from != ILLEGAL_POS) os << " from " << POS_NAME[from];
  if (to != ILLEGAL_POS) os << " to " << POS_NAME[to];
  if (piece) os << " (only " << PIECE_NAME[piece] << "s considered)";
  os << "\nBEGIN\n";

  Move move;
  if (from != ILLEGAL_POS) {
    if (to != ILLEGAL_POS) {
      move = moves_from_to(from, to);
    } else {
      move = moves_from_pos(from);
    }
  } else {
    if (to != ILLEGAL_POS) {
      move = moves_to_dest(to);
    } else {
      move = moves();
    }
  }

  while (next_move(move, piece))
    os << "    " << moveToSAN(move) << '\n';

  os << "END\n";
}

void Board2::print_bit_boards(ostream& os) {
  for (int i=0; bit_boards[i][0].as_bool(); i++) {
    os << "White bit board number " << i << ":\n";
    print_bit_board(os, bit_boards[i][0]);
    os << '\n';
  }
  for (int i=0; bit_boards[i][1].as_bool(); i++) {
    os << "Black bit board number " << i << ":\n";
    print_bit_board(os, bit_boards[i][1]);
    os << '\n';
  }
}
//#####################################################################

void Board2::init_CHECK_TABLE() {
  // 0=00: Empty
  // 1=01: own king
  // 2=10: enemy threat
  // 3=11: blocking piece
  int L[8];
  for (int i=0; i<0x10000; i++) {
    for (int j=0; j<8; j++)
      L[j] = (i>>(2*j))&3;
    uchar check = 0;
    bool threat;

    // Check in one direction:
    threat = false;
    for (int j=0; j<8; j++) {
      if (threat  &&  L[j]==1)
        ++check;
      if (L[j]) threat = L[j]==2;
    }
    // Check in other direction:
    threat = false;
    for (int j=7; j>=0; j--) {
      if (threat  &&  L[j]==1)
        ++check;
      if (L[j]) threat = L[j]==2;
    }

    CHECK_TABLE[i] = check;
    /*
    if ((rand()&0xFF) == 0) {
      for (int j=0; j<8; j++) {
	static const char K[4] = {'_','K','T','#'};
	cerr << K[L[j]];
      }
      cerr << " : " << check << "\n";
    }
     */
  }
}

void Board2::init_DIRECTION() { // unsigned direction
  //for (int i=0; i<4096; i++) DIRECTION[i&0x3F][i>>6] = 0;

  for (int col1=0; col1<8; col1++) for (int r1=0; r1<8; r1++) 
    for (int col2=0; col2<8; col2++) for (int r2=0; r2<8; r2++) {
      Position from = CR_TO_POS[col1][r1];
      Position to = CR_TO_POS[col2][r2];
      if (col1==col2) {
        if (r1==r2) {
          DIRECTION[from][to] = INVALID_DIRECTION;
          SDIRECTION[from][to] = INVALID_DIRECTION;
        } else {
          DIRECTION[from][to] = 1;
          SDIRECTION[from][to] = r1<r2 ? 1 : 5;
        }
      } else if (r1==r2) {
        DIRECTION[from][to] = 0;
        SDIRECTION[from][to] = col1<col2 ? 0 : 4;
      } else {
        int dc = col2-col1;
        int dr = r2-r1;
        if (dc == dr) {
          DIRECTION[from][to] = 2;
          SDIRECTION[from][to] = dc>0 ? 2 : 6;
        } else if (dc == -dr) {
          DIRECTION[from][to] = 3;
          SDIRECTION[from][to] = dr>0 ? 3 : 7;
        } else if (dc*dc+dr*dr == 5) {
          DIRECTION[from][to] = KNIGHT_DIRECTION;
          SDIRECTION[from][to] = KNIGHT_DIRECTION;
        } else {
          DIRECTION[from][to] = INVALID_DIRECTION;
          SDIRECTION[from][to] = INVALID_DIRECTION;
        }
      }
    }

  // initialize DIRECTION_MOVE_TABLE
  pair<int,int> D[8]; // <dColum, dRow>
  D[0] = pair<int, int>(1,0);
  D[1] = pair<int, int>(0,1);
  D[2] = pair<int, int>(1,1);
  D[3] = pair<int, int>(-1,1);
  D[4] = pair<int, int>(-1,0);
  D[5] = pair<int, int>(0,-1);
  D[6] = pair<int, int>(-1,-1);
  D[7] = pair<int, int>(1,-1);
  for (int d=0; d<7; d++) {
    for (int pos=0; pos<64; pos++) {
      int c = COLUMN[pos] + D[d].first;
      int r = ROW[pos] + D[d].second;
      if (0<=c && c<8  &&  0<=r && r<8) {
        DIRECTION_MOVE_TABLE[d][pos] = CR_TO_POS[c][r];
      } else {
        DIRECTION_MOVE_TABLE[d][pos] = ILLEGAL_POS;
      }
    }
  }
}


//      a  b  c  d  e  f  g  h
//   +-------------------------+
// 8 | 56 57 58 59 60 61 62 63 | 8
// 7 | 48 49 50 51 52 53 54 55 | 7
// 6 | 40 41 42 43 44 45 46 47 | 6
// 5 | 32 33 34 35 36 37 38 39 | 5
// 4 | 24 25 26 27 28 29 30 31 | 4
// 3 | 16 17 18 19 20 21 22 23 | 3
// 2 |  8  9 10 11 12 13 14 15 | 2
// 1 |  0  2  3  4  5  6  7  8 | 1
//   +-------------------------+
//      a  b  c  d  e  f  g  h
void Board2::init_bitboards() {
  pair<int,int> d[8];
  d[0] = pair<int, int>(-1,-2);
  d[1] = pair<int, int>(-2,-1);
  d[2] = pair<int, int>(1,2);
  d[3] = pair<int, int>(2,1);
  d[4] = pair<int, int>(-1,2);
  d[5] = pair<int, int>(-2,1);
  d[6] = pair<int, int>(1,-2);
  d[7] = pair<int, int>(2,-1);
  for (int r=0; r<8; r++)
    for (int c=0; c<8; c++) {
      Position pos = CR_TO_POS[c][r];
      for (Piece piece=0; piece<13; piece++)
        BIT_BOARDS[piece][pos] = 0;

      if (r!=0 && r!=7) {
        if (c!=0) {
          BIT_BOARDS[WPAWN][pos] |= pos+7;
          BIT_BOARDS[BPAWN][pos] |= pos-9;
        }
        if (c!=7) {
          BIT_BOARDS[WPAWN][pos] |= pos+9;
          BIT_BOARDS[BPAWN][pos] |= pos-7;
        }
      }

      if (c!=0 && c!=7) {
        if (r!=0) {
          BIT_BOARDS[13][pos] |= pos-7;
          BIT_BOARDS[14][pos] |= pos-9;
        }
        if (r!=7) {
          BIT_BOARDS[13][pos] |= pos+9;
          BIT_BOARDS[14][pos] |= pos+7;
        }
      }

      for (int i=0; i<8; i++) {
        int col2 = c+d[i].first;
        int r2 = r+d[i].second;
        if (0<=col2 && col2<8  &&  0<=r2 && r2<8) {
          BIT_BOARDS[WKNIGHT][pos] |= 8*r2 + col2;
          BIT_BOARDS[BKNIGHT][pos] |= 8*r2 + col2;
        }
      }

      for (int delta1=-1; delta1<=1; delta1++)
        for (int delta2=-1; delta2<=1; delta2++)
          if (delta1 || delta2)
            if (0<=c+delta1 && c+delta1<8  &&  0<=r+delta2 && r+delta2<8) {
              BIT_BOARDS[WKING][pos] |= CR_TO_POS[c+delta1][r+delta2];
              BIT_BOARDS[BKING][pos] |= CR_TO_POS[c+delta1][r+delta2];
            }
    }
}


void Board2::init_bitboard_lines() {
  // BB_LINES[from][to], to not included
  // if (from, to) do not form a straight line then
  // only from will be marked
  board_iterate(p1) board_iterate(p2) {
    BB_LINES[p1][p2] = 0; // if later default constr. doesn't reset
    if (p1 == p2) {
      BB_LINES[p1][p1] |= p1;
    } else {
      int direction = SDIRECTION[p1][p2];
      if (direction < 8) {
        int step = DIRECTIONS[direction];
        for (int pos = p1; pos != p2; pos += step)
          BB_LINES[p1][p2] |= pos;
      } else {
        BB_LINES[p1][p2] |= p1;
      }
    }
  }
}

bool Board2::internal_set_board() {
  // Board::internal_set_board needs not to be called (does nothing)
  set_check_invariants();
  return !king_capture_possible();
}

bool Board2::king_capture_possible() {
  player ^= 1;
  bool result = check_if_king_placed(king_pos[player]);
  player ^= 1;
  return result;
}


//###############################

inline string cts(char ch) {
  string result(" ");
  result[0] = ch;
  return result;
}

string Board2::moveToSAN(Move move) {
  // A move might not be defined!
  if (!move.is_defined()) {
    if (move.is_null_move()) return "NullMove";
    return "#move#";
  }
  assert(legal_pos(move.from) && legal_pos(move.to) && board[move.from]);

  string suffix = "";
  {
    Undo undo = execute_move(move);
    if (num_checks) {
      // a "+" or "#" suffix
      int game_state = calc_game_status();
      if (game_state  &&  game_state != GAME_DRAWN) suffix = "#";
      else suffix = "+";
    }
    undo_move(move, undo);
  }

  if (move.is_castling()) {
    if (COLUMN[move.to] == 2) return "O-O-O" + suffix;
    return "O-O" + suffix;
  }

  const Piece piece_kind = PIECE_KIND[board[move.from]];

  if (piece_kind == PAWN) {
    string pp = "";
    if (move.is_pawn_promotion()) {
      // Promoted piece must be upper case
      pp = "= ";
      pp[1] = PIECE_CHAR[PIECE_KIND[move.special_move]];
    }

    if (move.special_move==EN_PASSANT || board[move.to]) {
      // Piece captured.
      return COLUMN_NAME[move.from] + "x" + POS_NAME[move.to] + pp + suffix;
    } else {
      return POS_NAME[move.to] + pp + suffix;
    }
  }

  string hit = board[move.to] ? "x" : "";

  if (piece_kind == KING) {
    // Very simple!
    return "K" + hit + POS_NAME[move.to] + suffix;
  }

  // piece is KNIGHT, BISHOP, ROOK or QUEEN

  bool ambiguity = false;
  bool same_row = false;
  bool same_colum = false;
  Move m(moves_to_dest(move.to));
  while (next_move(m, piece_kind))
    if (m.from != move.from) {
      ambiguity = true;
      if (COLUMN[m.from] == COLUMN[move.from])
        same_colum = true;
      if (ROW[m.from] == ROW[move.from])
        same_row = true;
    }
  /*
    file==COLUMN, rank==ROW

  First, if the moving pieces can be distinguished by their originating files, the 
originating file letter of the moving piece is inserted immediately after the moving 
piece letter. 

Second (when the first step fails), if the moving pieces can be distinguished by their 
originating ranks, the originating rank digit of the moving piece is inserted 
immediately after the moving piece letter. 

Third (when both the first and the second steps fail), the two character square 
coordinate of the originating square of the moving piece is inserted immediately after 
the moving piece letter. 
   */

  if (same_colum) {
    if (same_row)
      return PIECE_CHAR[piece_kind] + POS_NAME[move.from] + hit + POS_NAME[move.to] + suffix;
    return PIECE_CHAR[piece_kind] + ROW_NAME[move.from] + hit + POS_NAME[move.to] + suffix;
  }
  if (ambiguity)
    return PIECE_CHAR[piece_kind] + COLUMN_NAME[move.from] + hit + POS_NAME[move.to] + suffix;
  return PIECE_CHAR[piece_kind] + hit + POS_NAME[move.to] + suffix;
}


Move Board2::sanToMove(string san) {
  // cerr << "Converting " << san << "...\n";
  int pawn_promote = 0;

  int last = san.length()-1;
  // strip suffix
  while (san[last] < '1'  ||  '9' < san[last]) {
    if (san[last] == 'O') {
      // castling move
      if (last==2) {
        return player ? Move(BLACK_SHORT_CASTLING) : Move(WHITE_SHORT_CASTLING);
      } else {
        return player ? Move(BLACK_LONG_CASTLING) : Move(WHITE_LONG_CASTLING);
      }
    }
    if ('A'<=san[last] && san[last]<='Z') {
      // pawn promotion
      pawn_promote = char_to_piece(san[last]);
      if (player) pawn_promote += 6;
    }
    --last;
  }

  // cerr << "last mod. to " << last << "\n";

  Position dest = CR_TO_POS[san[last-1]-'a'][san[last]-'1'];
  Piece piece_kind;

  // origin colum/row specified:
  bool colum_specified = false;
  bool row_specified = false;

  int from_c=0;
  int from_r=0;

  if ('a'<=san[0]  &&  san[0]<='h') {
    piece_kind = WPAWN;
    if (last > 1) {
      colum_specified = true;
      from_c = san[0]-'a';
    }
  } else {
    piece_kind = char_to_piece(san[0]);
    for (int i=1; i<last-1; i++) {
      if ('a'<=san[i] && san[i]<='h') {
        colum_specified = true;
        from_c = san[i]-'a';
      } else if ('1'<=san[i] && san[i]<='8') {
        row_specified = true;
        from_r = san[i]-'1';
      }
    }
  }

  // cerr << "piece kind = " << PIECE_NAME[piece_kind] << "\n";

  bool move_found = false;
  Move move = moves_to_dest(dest);
  while (next_move(move, piece_kind)) {
    // cerr << move.toString() << " - " << (int)piece_kind << " " << POS_NAME[dest] << "\n";
    bool match = true;
    if (colum_specified  &&  COLUMN[move.from]!=from_c) {
      match = false;
    } else if (row_specified  &&  ROW[move.from]!=from_r) {
      match = false;
    }
    if (match) {
      move_found = true;
      break;
    }
  }

  // cerr << "Hertil ok\n";

  if (!move_found) { return Move(); } // null move => error

  if (pawn_promote) move.special_move = pawn_promote;

  // cerr << "Move recognized as " << move.toString() << "\n";

  return move;
}



//########################################################

// Nemmest kun at tillade null move hvis en_passant mulig
// Nødvendig hvis der er skak
bool Board2::try_execute_null_move() {
  if (num_checks  ||  en_passant != ILLEGAL_POS) return false;
  player ^= 1;
  ++moves_played;
  return true;
}

void Board2::undo_null_move() {
  assert(num_checks==0  &&  en_passant==ILLEGAL_POS);
  player ^= 1;
  --moves_played;
}


// find_legal_move checks whether move.from and move.to matches a legal move.
// In case of a match move.special_move and move.blah is set
// (pawn promotion: default is queen). Is SLOW!
bool Board2::find_legal_move(Move& move) {
  Move m = moves();
  while (next_move(m)) {
    if (m.from==move.from  &&  m.to==move.to) {
      move = m;// sets move.special_move and move.blah
      return true;
    }
  }
  return false;
}



// triple<check_count, prev_num_checks, threat_pos>
// Assume it is white-to-move and we wish to the possible last moves for black.
// check_count is the number of checks against whites king. After taking back one of
// blacks moves it should be 0 - otherwise black could instead have captured whites king
// - hence the position would be illegal.
// prev_num_checks is the number of checks against black king after undoing the move.
// If prev_num_checks != 0, then threat_pos determines the position of one of the threats.
triple<int,uchar,Position> Board2::retro_move_count_checks(Position from, Position to,
    Piece original_piece, Piece piece_killed) {

  int check_count = num_checks;
  {


    if (IS_SHORT_DISTANCE_PIECE[original_piece]  &&
        BIT_BOARDS[original_piece][from][king_pos[player]]) {
      // This piece can't have moved from "from". It could have taken the king.
      return triple<int,uchar,Position>(42,0,0);
    }

    if (IS_SHORT_DISTANCE_PIECE[board[to]])
      check_count -= BIT_BOARDS[board[to]][to][king_pos[player]];

    int d_to = DIRECTION[to][king_pos[player]];
    int d_from = DIRECTION[from][king_pos[player]];

    if (d_to == d_from) {

      if (!(d_to & ~3)) {
        if (SDIRECTION[from][to] != SDIRECTION[to][king_pos[player]]) {
          // move is away from the king, can only affect number of checks in
          // case of a pawn promotion.
          // The content of "to" doesn't matter as it is blocked by "from"
          if (original_piece != board[to]) {
            // Pawn promotion
            uint pattern = board_lines[d_to][DIAG_INDEX[to][d_to]];

            check_count -= checktable(pattern, player);

            // Not necessary to remove promoted piece, as it will be blocked by the pawn
            // Original piece must be a pawn. A pawn has patter 11b
            int line_index = d_from ? (from>>3) : (from&7);
            pattern |= DIAG_PATTERN[line_index][3];

            check_count += checktable(pattern, player);

          }

        } else {
          // move is towards the king. Can affect the number of checks if
          // a piece was captured.
          // Also when undoing a rook move as part of a castling, the
          // rook "jumps over" the king.
          if (piece_killed) {
            uint pattern = board_lines[d_to][DIAG_INDEX[to][d_to]];

            check_count -= checktable(pattern, player);

            // Just or the 11 pattern of a blocking piece on top of board[to]
            int line_index = d_to ? (to>>3) : (to&7);
            pattern |= DIAG_PATTERN[line_index][ 3 ];

            check_count += checktable(pattern, player);

          } else if (PIECE_KIND[original_piece] == ROOK) {

            // Maybe a rook is being moved while undoing a castling

            //cerr << "from,to = " << POS_NAME[from] << "," << POS_NAME[to] << "\n";

            uint pattern = board_lines[d_to][DIAG_INDEX[to][d_to]];

            check_count -= checktable(pattern, player);

            pattern &= AND_DPP_PATTERN[d_to ? (to>>3) : (to&7)];

            int line_index = d_from ? (from>>3) : (from&7);
            pattern |= DIAG_PATTERN[line_index][ DIAG_PIECE_PATTERN[original_piece][d_from] ];

            check_count += checktable(pattern, player);
          }
        }
      }

    } else {

      if (straight_direction(d_to)) {
        uint pattern = board_lines[d_to][DIAG_INDEX[to][d_to]];

        check_count -= checktable(pattern, player);

        int line_index = d_to ? (to>>3) : (to&7);
        pattern &= AND_DPP_PATTERN[line_index];
        if (piece_killed) pattern |= DIAG_PATTERN[line_index][ DIAG_PIECE_PATTERN[piece_killed][d_to] ];

        check_count += checktable(pattern, player);
      }


      if (straight_direction(d_from)) {
        uint pattern = board_lines[d_from][DIAG_INDEX[from][d_from]];
        check_count -= checktable(pattern, player);
        int line_index = d_from ? (from>>3) : (from&7);
        pattern |= DIAG_PATTERN[line_index][ DIAG_PIECE_PATTERN[original_piece][d_from] ];
        check_count += checktable(pattern, player);
      }
    }
  }

  if (check_count) {
    // No reason to proceed
    return triple<int,uchar,Position>(check_count, 0, 0);
  }

  //###############################################################################
  //###############################################################################

  uchar prev_num_checks = 0;
  Position threat_pos = ILLEGAL_POS;
  {
    if (PIECE_KIND[board[to]]==KING) {
      // ################################################
      // ##########        KING MOVED    ################
      // ################################################

      if (piece_killed  &&  IS_SHORT_DISTANCE_PIECE[piece_killed]  &&
          BIT_BOARDS[piece_killed][to][from]) {
        ++prev_num_checks;
        threat_pos = to;
      }

      for (int i=0; bit_boards[i][player][from]; i++) {

        if (!prev_num_checks++) {
          // find a short range checking piece to blame
          bool found = false;
          if (!found) {
            // Try a knight
            int find_piece = player ? BKNIGHT : WKNIGHT;
            int dest = PIECE_NEXT(KNIGHT, from, from);
            while (dest != ILLEGAL_POS) {
              if (board[dest] == find_piece) {
                threat_pos = dest;
                found = true;
                break;
              }
              dest = PIECE_NEXT(KNIGHT, from, dest);
            }
          }

          if (!found) {
            // Try a pawn
            int find_piece = player ? BPAWN : WPAWN;
            if (COLUMN[from] != 0) {
              int dest = from + (player ? +7 : -9);
              if (board[dest] == find_piece) {
                threat_pos = dest;
                found = true;
              }
            }
            if (COLUMN[from] != 7) {
              int dest = from + (player ? +9 : -7);
              if (board[dest] == find_piece) {
                threat_pos = dest;
                found = true;
              }
            }
          }

          assert(found);
        }
      }

      {// Check long distance threats caused by piece_killed
        if (PIECE_KIND[piece_killed] == QUEEN) {
          ++prev_num_checks;
          threat_pos = to;
        } else {
          if (ROW[from]!=ROW[to]  &&  COLUMN[from]!=COLUMN[to]) {
            // Checked if captured bishop (or queen, but already considered)
            if (PIECE_KIND[piece_killed]==BISHOP) {
              ++prev_num_checks;
              threat_pos = to;
            }
          } else {
            // Checked if captured rook (or queen, but already considered)
            if (PIECE_KIND[piece_killed]==ROOK) {
              ++prev_num_checks;
              threat_pos = to;
            }
          }
        }
      }


      { // Check other long distance threats
        const uchar *tmp = DIAG_PIECE_PATTERN[player ? WKING : BKING];

        for (int d=0; d<4; d++) {
          uint pattern = board_lines[d][DIAG_INDEX[from][d]];
          //pattern &= AND_DPP_PATTERN[d ? (from>>3) : (from&7)];
          pattern |= DIAG_PATTERN[d ? (from>>3) : (from&7)][ tmp[d] ];
          if (checktable(pattern, player^1)) {
            if (!prev_num_checks) {
              prev_num_checks += checktable(pattern, player^1);
              pattern &= CLEARLOW_DPP_PATTERN[d ? (from>>3) : (from&7)];

              int step = DIRECTIONS[checktable(pattern, player^1) ? d : d+4];
              threat_pos = from;
              while (!board[threat_pos += step]) ;
            } else {
              prev_num_checks += checktable(pattern, player^1);
            }
          }
        }
      }

    } else {
      // ################################################
      // ##########     NOT KING MOVED    ###############
      // ################################################

      int kpos = king_pos[player^1];
      //cerr << "Position of king: " << POS_NAME[kpos] << "\n";

      if (piece_killed  &&  IS_SHORT_DISTANCE_PIECE[piece_killed]) {

        if (BIT_BOARDS[piece_killed][to][kpos]) {
          ++prev_num_checks;
          threat_pos = to;
        }

      } else {

        int d_to = DIRECTION[to][kpos];
        int d_from = DIRECTION[from][kpos];

        if (d_to == d_from) {

          // a threat might have been eliminated if the attacker moved towards the king
          if (piece_killed  &&  !(d_to & ~3)) {

            uint pattern = board_lines[d_to][DIAG_INDEX[to][d_to]];

            // add old position of attacker (just as a blocking piece)
            pattern |= DIAG_PATTERN[d_from ? (from>>3) : (from&7)][ 3 ];

            // Replace to with the killed piece
            int line_index = d_to ? (to>>3) : (to&7);
            pattern &= AND_DPP_PATTERN[line_index];
            pattern |= DIAG_PATTERN[line_index][ DIAG_PIECE_PATTERN[piece_killed][d_to] ];

            if (checktable(pattern, player^1)) {
              ++prev_num_checks;
              threat_pos = to;

            }
          }

        } else {

          if (straight_direction(d_to)) {
            //cerr << "Direction towards king: " << UDIRECTION_NAME[d_to] << "\n";

            uint pattern = board_lines[d_to][DIAG_INDEX[to][d_to]];
            int line_index = d_to ? (to>>3) : (to&7);
            pattern &= AND_DPP_PATTERN[line_index];
            if (piece_killed) pattern |= DIAG_PATTERN[line_index][ DIAG_PIECE_PATTERN[piece_killed][d_to] ];

            if (checktable(pattern, player^1)) {
              // only single check
              ++prev_num_checks;

              // Find threat pos
              threat_pos = to;
              if (!piece_killed) {
                // piece_killed can't cause the check
                // If the piece blocks the check at position to, then search
                // for the threat in the kpos->to direction.
                int step = DIRECTIONS[SDIRECTION[kpos][to]];
                while (!board[threat_pos += step]) ;
              }
            }
          }

          // inserting the piece at d_from won't create more checks. Also
          // it wont prevent any, as the king was not checked before undoing the move.
        }
      }

      // No check can be caused by from
    }
  }

  return triple<int,uchar,Position>(check_count, prev_num_checks, threat_pos);
}

struct EnPassantPP {
  EnPassantPP() : ep(ILLEGAL_POS), ep_pawn(0) {}

  // For the en passant at ep to be valid, the positions p1 and p2 may not be occupied
  EnPassantPP(Position p1, Position p2, Position ep) : ep(ep), ep_pawn(0) {
    assert(p1<=ILLEGAL_POS  &&  p2<=ILLEGAL_POS  &&  ep<=ILLEGAL_POS);
    illegal_from.set(p1);
    illegal_from.set(p2);
    illegal_from.set(ep);
    illegal_to = illegal_from;
  }

  // A piece is occupying position ep or p1 (the square behind p1).
  // Iff this piece is moved away from the positions ep and p1, then the ep will be valid
  EnPassantPP(Position p1, Position ep) : ep(ep), ep_pawn(0) {
    assert(p1<=ILLEGAL_POS  &&  ep<=ILLEGAL_POS);
    illegal_to.set();
    illegal_to.reset(p1);
    illegal_to.reset(ep);
    illegal_from.set(p1);
    illegal_from.set(ep);
  }

  // It is assumed that an en passant pawn was captured at capture_pos
  EnPassantPP(Position p1, Position capture_pos, Position ep, Piece ep_pawn) : ep(ep), ep_pawn(ep_pawn) {
    illegal_to.set();
    illegal_to.reset(capture_pos);
    illegal_from.set(p1);
    illegal_from.set(ep);
    // capture_pos may not be set as a illegal_from
  }


  bool accepted(Position from, Position to, Position &en_passant, Piece killed_piece) {
    assert(legal_pos(from)  &&  legal_pos(to));

    if (illegal_from[from]  ||  illegal_to[to]) {
      return false;
    } else {
      // No piece must be left after the move on an illegal_from square
      if (killed_piece  &&  illegal_from[to]) return false;

      // If ep_pawn != 0, then the en passant pawn currently doesn't exists.
      // Hence the en passant will only be legal if this move captured the e.p. pawn.
      if (ep_pawn  &&  killed_piece != ep_pawn) return false;

      en_passant = ep;
      return true;
    }
  }

  bitset<65> illegal_from, illegal_to;
  Position ep;
  Piece ep_pawn;
};


Position Board2::captured_en_passant_pawn(Position from, Position pawn_capture_pos) {
  int dr = player ? 8 : -8;
  // Check that:
  // 1) 2 squares behind captured pawn are empty
  // 2) The capturing piece did not move from one of these squares
  if (board[pawn_capture_pos+dr]==0  &&  board[pawn_capture_pos+2*dr]==0  &&
      from != pawn_capture_pos+dr  &&  from != pawn_capture_pos+2*dr) {

    // En passant pawn can be taken by a pawn to the left?
    if (COLUMN[pawn_capture_pos] != 0  &&  board[pawn_capture_pos-1] == (player ? WPAWN : BPAWN)) {
      return pawn_capture_pos+dr;
    }

    // En passant pawn can be taken by a pawn to the right?
    if (COLUMN[pawn_capture_pos] != 7  &&  board[pawn_capture_pos+1] == (player ? WPAWN : BPAWN)) {
      return pawn_capture_pos+dr;
    }
  }

  return ILLEGAL_POS;
}




// Last move?  loadfen 8/3K1B2/8/3q4/8/1k6/8/8 w - -
// Last move?  loadfen 8/3K1B2/8/4q3/8/8/1k6/8 w - -
// Last move?  loadfen 8/r2K1B2/8/3q4/8/1k6/8/8 w - -
// Last move?  loadfen 8/3k1b2/5N2/8/3Q4/1K6/8/8 b - -
// en passant: loadfen 8/3k1b2/5N2/pP3K2/8/8/8/8 b - -
// en passant: loadfen 8/5b2/1P1k4/8/3K4/4N3/1n6/8 b - -
// en passant: loadfen 8/8/2Pk4/8/1Q1QK3/8/8/8 b - -
// kk:         loadfen 8/1k6/8/8/8/8/6K1/8 b - -
// after e.p.: loadfen 8/8/8/1KPpk3/8/8/8/8 w - d6

// pawn pro. : loadfen Qk//K1R///// b - -

vector<triple<Move,Undo,int> >
Board2::get_retro_moves(bool allow_pawn_promotion, bool allow_castling,
    bool allow_captures,
    bool allow_transformations_of_board, uint max_moves)
{
  if (!moves_played) {
    cerr << "moves_played is 0  =>  no undoing possible!\n";
    return vector<triple<Move,Undo,int> >();
  }

  int allowed_transformations;
  if (allow_transformations_of_board) {
    allowed_transformations = 0xFF;
    if (get_num_pawns()) allowed_transformations = (1<<0) | (1<<1);
    if (castling) allowed_transformations = (1<<0);
  } else {
    allowed_transformations = (1<<0);
  }

  vector<EnPassantPP> ep_possibilities;
  // Add one entry representing no en passant possible
  ep_possibilities.push_back(EnPassantPP(ILLEGAL_POS, ILLEGAL_POS, ILLEGAL_POS));

  const uchar LEGAL_FROM = 1;
  const uchar LEGAL_TO = 2;
  const uchar PAWN_2_FORWARD_NOT_ALLOWED = 4;
  uchar LEGAL_FROM_TO[64];

  if (get_num_pawns()) {
    // if en passant is possible, then unique last move is that pawn 2 forward
    if (en_passant_possible()) {
      memset(LEGAL_FROM_TO, 0, 64);
      LEGAL_FROM_TO[player ? en_passant-8 : en_passant+8] = LEGAL_FROM;
      LEGAL_FROM_TO[player ? en_passant+8 : en_passant-8] = LEGAL_TO;
    } else {
      memset(LEGAL_FROM_TO, LEGAL_FROM+LEGAL_TO, 64);

      // Find out if some pawns could not have moved 2 forward in their last move
      // as this would give the side-to-move an en passant option.

      int offset = player ? 24 : 32;
      int dr = player ? -8 : 8;
      int pawn = player ? WPAWN : BPAWN;
      for (int c=0; c<8; c++) {
        if (board[offset+c]==pawn  &&
            board[offset+c+dr]==0  &&  board[offset+c+2*dr]==0  &&
            ((c != 0  &&  board[offset+c-1]+pawn==WPAWN+BPAWN)  ||
                (c != 7  &&  board[offset+c+1]+pawn==WPAWN+BPAWN))) {
          LEGAL_FROM_TO[offset+c] |= PAWN_2_FORWARD_NOT_ALLOWED;
          LEGAL_FROM_TO[offset+c+2*dr] |= PAWN_2_FORWARD_NOT_ALLOWED;
        }
      }
    }
  } else { // No pawns
    memset(LEGAL_FROM_TO, LEGAL_FROM+LEGAL_TO, 64);
  }



  if (get_num_pawns()) {
    // Do some preprocessing regarding en passant

    // Assume wtm. Find pawn constellations where, after black has undone a move,
    // white might previous had advanced a pawn 2 squares allowing for en passant capture.
    //
    //     a b c d e f g h
    //   +-----------------+    |
    // 8 |                 | 8  | 1w
    // 7 |                 | 7  |
    // 6 |       b   k     | 6  | White has lost castling
    // 5 |                 | 5  | Black has lost castling
    // 4 |     p P   K     | 4  |
    // 3 |                 | 3  | moves played since progress = 6
    // 2 |                 | 2  |
    // 1 |                 | 1  |
    //   +-----------------+    |
    //     a b c d e f g h
    //

    int offset = 8*(player ? 4 : 3);
    int dr = player ? 8 : -8;
    int pawn = player ? BPAWN : WPAWN;

    for (int c=0; c<8; c++) {
      if (board[offset+c]==pawn  ||  (allow_captures  &&  PIECE_COLOR[board[offset+c]] == player^1)) {
        int lpawn = c != 0  &&  board[offset+c-1]+pawn==WPAWN+BPAWN;
        int rpawn = c != 7  &&  board[offset+c+1]+pawn==WPAWN+BPAWN;
        if (lpawn | rpawn) {

          Position ep = offset+c+dr;
          Position pos2 = offset+c+2*dr;

          //cerr << "Pawn at " << POS_NAME[offset+c] << " was en passant pawn?\n";

          // The pawn at position offset+c might have moved 2 forward 2 moves ago.

          // This option remains if the undone move does not affect
          // the 2 squares behind the ep pawn and the capturer pawn if it is unique.

          // Save as: (unaffect1, unaffect2, unaffect3) => ep-square

          if (board[offset+c]==pawn) {

            if (!(board[ep] | board[pos2])) {
              // Squares behind en passant pawn are empty
              if (lpawn && rpawn) {
                // not both pawns can move, hence only the 2 squares behind ep pawn
                // Needs to be untouched
                ep_possibilities.push_back(EnPassantPP(pos2, ILLEGAL_POS, ep));
              } else {
                ep_possibilities.push_back(EnPassantPP(pos2, lpawn ? offset+c-1 : offset+c+1, ep));
              }

            } else if (PIECE_COLOR[board[ep]] != player  &&  PIECE_COLOR[board[pos2]] != player  &&
                PIECE_COLOR[board[ep]] != PIECE_COLOR[board[pos2]]) {
              // Squares behind en passant pawn are not occupied by piece belonging to stm
              // and at most one of them belong to the player retracting a move
              // If this piece is moved, the en passant will be valid
              ep_possibilities.push_back(EnPassantPP(pos2, ep));
            }

          } else {

            // The piece at offset+c might have captured a pawn that could otherwise
            // be taken en passant
            if (!(board[ep] | board[pos2]))
              ep_possibilities.push_back(EnPassantPP(pos2, offset+c, ep, pawn));
          }

        } else {

          Position ep = offset+c+dr;

          // No pawn ready to capture the offset+c pawn en passant!
          // What if a pawn move is retracted to a position from where the offset+c
          // pawn can be captured en passant?
          if (board[offset+c]==pawn  &&  !(board[ep] | board[ep+dr])) {

            bool ok=false;
            EnPassantPP e;
            e.ep = ep;
            e.illegal_to.set();
            e.illegal_from.set();

            if (c!=0) {
              if (board[ep-1]+pawn==WPAWN+BPAWN) {
                e.illegal_to.reset(ep-1);
                e.illegal_from.reset(ep-1);//Don't delete, there is a reason why it is here!
                e.illegal_from.reset(offset+c-1);
                ok = true;
              }

              if (c!=1) {
                if (board[ep-2]+pawn==WPAWN+BPAWN) {
                  e.illegal_to.reset(ep-2);
                  e.illegal_from.reset(ep-2);
                  e.illegal_from.reset(offset+c-1);
                  ok = true;
                }
              }
            }

            if (c!=7) {
              if (board[ep+1]+pawn==WPAWN+BPAWN) {
                e.illegal_to.reset(ep+1);
                e.illegal_from.reset(ep+1);
                e.illegal_from.reset(offset+c+1);
                ok = true;
              }

              if (c!=6) {
                if (board[ep+2]+pawn==WPAWN+BPAWN) {
                  e.illegal_to.reset(ep+2);
                  e.illegal_from.reset(ep+2);
                  e.illegal_from.reset(offset+c+1);
                  ok = true;
                }
              }
            }

            if (ok) ep_possibilities.push_back(e);
          }
        }
      }
    }
  }



#include "board_define_position_constants.hxx"
  // If castling possible, king and relevant rooks may not move.
  if (castling) {
    if (castling & (WHITE_LONG_CASTLING | WHITE_SHORT_CASTLING)) {
      LEGAL_FROM_TO[e1] &= ~LEGAL_TO;
      if (castling & WHITE_LONG_CASTLING) LEGAL_FROM_TO[a1] &= ~LEGAL_TO;
      if (castling & WHITE_SHORT_CASTLING) LEGAL_FROM_TO[h1] &= ~LEGAL_TO;
    }
    if (castling & (BLACK_LONG_CASTLING | BLACK_SHORT_CASTLING)) {
      LEGAL_FROM_TO[e8] &= ~LEGAL_TO;
      if (castling & BLACK_LONG_CASTLING) LEGAL_FROM_TO[a8] &= ~LEGAL_TO;
      if (castling & BLACK_SHORT_CASTLING) LEGAL_FROM_TO[h8] &= ~LEGAL_TO;
    }
  }

  // ADDED_CASTLING[p] is nonzero for an empty square, if the player retracting a
  // rook/king move to p can gain castling capability.
  // ADDED_CASTLING[p] is nonzero for a nonempty square, if a rook-capturing move
  // is retracted, and the rook may have had castling capability before.
  uchar ADDED_CASTLING[64];
  memset(ADDED_CASTLING, 0, 64);

  { // Find out if extra castling capabilities will be adding by undoing certain
    // moves.

    int long_castling = player ? WHITE_LONG_CASTLING : BLACK_LONG_CASTLING;
    int short_castling = player ? WHITE_SHORT_CASTLING : BLACK_SHORT_CASTLING;

    {
      int rook = player ? WROOK : BROOK;

      if (board[a1]==rook) {
        if (!board[e1])
          ADDED_CASTLING[e1] |= long_castling | KING_REFLECTIONS[player^1][e1];
        if (!board[a5])
          ADDED_CASTLING[a5] |= long_castling | KING_REFLECTIONS[player^1][a5];
        if (!board[d1])
          ADDED_CASTLING[d1] |= short_castling | KING_REFLECTIONS[player^1][d1];
        if (!board[a4])
          ADDED_CASTLING[a4] |= short_castling | KING_REFLECTIONS[player^1][a4];
      }
      if (board[h1]==rook) {
        if (!board[d1])
          ADDED_CASTLING[d1] |= long_castling | KING_REFLECTIONS[player^1][d1];
        if (!board[h5])
          ADDED_CASTLING[h5] |= long_castling | KING_REFLECTIONS[player^1][h5];
        if (!board[e1])
          ADDED_CASTLING[e1] |= short_castling | KING_REFLECTIONS[player^1][e1];
        if (!board[h4])
          ADDED_CASTLING[h4] |= short_castling | KING_REFLECTIONS[player^1][h4];
      }
      if (board[a8]==rook) {
        if (!board[e8])
          ADDED_CASTLING[e8] |= long_castling | KING_REFLECTIONS[player^1][e8];
        if (!board[a4])
          ADDED_CASTLING[a4] |= long_castling | KING_REFLECTIONS[player^1][a4];
        if (!board[d8])
          ADDED_CASTLING[d8] |= short_castling | KING_REFLECTIONS[player^1][d8];
        if (!board[a5])
          ADDED_CASTLING[a5] |= short_castling | KING_REFLECTIONS[player^1][a5];
      }
      if (board[h8]==rook) {
        if (!board[d8])
          ADDED_CASTLING[d8] |= long_castling | KING_REFLECTIONS[player^1][d8];
        if (!board[h4])
          ADDED_CASTLING[h4] |= long_castling | KING_REFLECTIONS[player^1][h4];
        if (!board[e8])
          ADDED_CASTLING[e8] |= short_castling | KING_REFLECTIONS[player^1][e8];
        if (!board[h5])
          ADDED_CASTLING[h5] |= short_castling | KING_REFLECTIONS[player^1][h5];
      }
    }

    {
      int refl = KING_REFLECTIONS[player^1][king_pos[player^1]];
      if (refl != -1) {
        int pos = DECODE_LONG_CASTLING_ROOK[king_pos[player^1]];
        if (!board[pos]) ADDED_CASTLING[pos] |= long_castling | refl;

        pos = DECODE_SHORT_CASTLING_ROOK[king_pos[player^1]];
        if (!board[pos]) ADDED_CASTLING[pos] |= short_castling | refl;
      }
    }
  }



  // If a corner rook was captured, then this could have associated castling rights,
  // if its king was placed correctly.
  // ADDED_CASTLING[x] is zero for x being a square with a piece belonging to stm
  // iff this piece is a rook  &&  stm has king on one of the 4 related squares.
  if (KING_REFLECTIONS[player][king_pos[player]] != -1) {
    int refl = KING_REFLECTIONS[player][king_pos[player]];

    if (board[a1]) {
      const uchar C[2][8] = {{WHITE_LONG_CASTLING, WHITE_SHORT_CASTLING, 0, 0,
          WHITE_LONG_CASTLING, 0, WHITE_SHORT_CASTLING, 0},
          {0,0,BLACK_LONG_CASTLING,BLACK_SHORT_CASTLING,
              0,BLACK_LONG_CASTLING,0,BLACK_SHORT_CASTLING}};
      if (C[player][refl]) {
        assert(!ADDED_CASTLING[a1]);
        ADDED_CASTLING[a1] = C[player][refl] | refl;
      }
    }

    if (board[h1]) {
      const uchar C[2][8] = {{WHITE_SHORT_CASTLING,WHITE_LONG_CASTLING,0,0,
          0,WHITE_LONG_CASTLING,0,WHITE_SHORT_CASTLING},
          {0,0,BLACK_SHORT_CASTLING,BLACK_LONG_CASTLING,
              BLACK_LONG_CASTLING,0,BLACK_SHORT_CASTLING,0}};
      if (C[player][refl]) {
        assert(!ADDED_CASTLING[h1]);
        ADDED_CASTLING[h1] = C[player][refl] | refl;
      }
    }

    if (board[a8]) {
      const uchar C[2][8] = {{0,0,WHITE_LONG_CASTLING,WHITE_SHORT_CASTLING,
          WHITE_SHORT_CASTLING,0,WHITE_LONG_CASTLING,0},
          {BLACK_LONG_CASTLING,BLACK_SHORT_CASTLING,0,0,
              0,BLACK_SHORT_CASTLING,0,BLACK_LONG_CASTLING}};
      if (C[player][refl]) {
        assert(!ADDED_CASTLING[a8]);
        ADDED_CASTLING[a8] = C[player][refl] | refl;
      }
    }

    if (board[h8]) {
      const uchar C[2][8] = {{0,0,WHITE_SHORT_CASTLING,WHITE_LONG_CASTLING,
          0,WHITE_SHORT_CASTLING,0,WHITE_LONG_CASTLING},
          {BLACK_SHORT_CASTLING,BLACK_LONG_CASTLING,0,0,
              BLACK_SHORT_CASTLING,0,BLACK_LONG_CASTLING,0}};
      if (C[player][refl]) {
        assert(!ADDED_CASTLING[h8]);
        ADDED_CASTLING[h8] = C[player][refl] | refl;
      }
    }
  }
#include "board_undef_position_constants.hxx"


  vector<triple<Move,Undo,int> > result;
  if (num_checks > 2) return result; // unreachable position
  result.reserve(32);


  for (int to=0; to<64; to++) {
    if (PIECE_COLOR[board[to]] == (player^1)  &&
        (LEGAL_FROM_TO[to] & LEGAL_TO)) {

      //cerr << "Examining moves for " << PIECE_NAME[board[to]] << " on " << POS_NAME[to] << "\n";

      {
        // Iterate move.from (remember this is actually where we want the piece to
        // move (back) to.
        Move move(INV_PIECE_JUMP(board[to], to, to),to);

        while (legal_pos(move.from)) {

          if (!board[move.from]  &&  (LEGAL_FROM_TO[move.from] & LEGAL_FROM)  &&//1
              (!CASTLING[move.from]  ||  !CASTLING[to]  ||  PIECE_KIND[board[to]] != KING)  &&//2
              !(LEGAL_FROM_TO[move.from] & LEGAL_FROM_TO[to] & PAWN_2_FORWARD_NOT_ALLOWED)) {//3
            // Line 2 identifies the illegal moves like Kg1e1 (inverse castling stuff)
            // Line 3 identifies a pawn 2 forward move that is illegal as it would give en passant cap.

            bool can_capture = allow_captures;
            bool can_non_capture = true;

            if (PIECE_KIND[board[to]] == PAWN) {
              if ((move.from ^ move.to) & 7) {
                // A diagonal move.
                // The pawn will have to capture something
                // (can_non_capture will not be checked for an en passant move)
                can_non_capture = false;

                // Last move en passant?
                if (can_capture  && ((player && ROW[to]==5) || (!player && ROW[to]==2))) {
                  // Correct row
                  if (board[to+8]==0  &&  board[to-8]==0) {
                    // The necessary squares are empty
                    //cerr << "Finder ep?\n";

                    triple<int,uchar,Position> t;

                    // Pawns present, only transf. 0 and 1 might be legal

                    // insertion of captured en passant pawn might prevent a check
                    int prevented_checks = 0;

                    Position pos = player ? to-8 : to+8;
                    int d = DIRECTION[pos][king_pos[player]];

                    if (straight_direction(d)) {
                      /*
			cerr << "En passant captured pawn has straight line to its king.\n"
			<< POS_NAME[pos] << " -> " << POS_NAME[king_pos[player]]
			<< ", direction = " << d << "\n";
                       */

                      uint *pattern = &(board_lines[d][DIAG_INDEX[pos][d]]);

                      prevented_checks += checktable(*pattern, player);
                      //cerr << "prevented_checks = " << prevented_checks << "\n";

                      int line_index = d ? (pos>>3) : (pos&7);
                      // insert pawn of opposite color in king lines
                      //cerr << "movement_piece = " << PIECE_NAME[8 - board[to]] << "\n";
                      *pattern |= DIAG_PATTERN[line_index][ DIAG_PIECE_PATTERN[8 - board[to]][d] ];

                      prevented_checks -= checktable(*pattern, player);
                      //cerr << "prevented_checks = " << prevented_checks << "\n";

                      t = retro_move_count_checks(move.from, to, board[to], 0);
                      t.first -= prevented_checks;

                      // Remove the captured en passant pawn from king lines again
                      *pattern &= AND_DPP_PATTERN[line_index];

                    } else {

                      t = retro_move_count_checks(move.from, to, board[to], 0);
                    }

                    // Was the captured pawn checking the king?
                    if (player) {
                      if ((COLUMN[pos]!=0  &&  board[pos-9]==WKING)  ||
                          (COLUMN[pos]!=7  &&  board[pos-7]==WKING))
                        ++t.second;
                    } else {
                      if ((COLUMN[pos]!=0  &&  board[pos+7]==BKING)  ||
                          (COLUMN[pos]!=7  &&  board[pos+9]==BKING))
                        ++t.second;
                    }


                    if (t.first == 0) {

                      // Only one possibility - en passant possible at exactly this position
                      Undo undo(to, castling,
                          moves_played_since_progress ? moves_played_since_progress-1 : 0,
                              player^1, t.second, t.third, 0);

                      if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                      move.special_move = EN_PASSANT;
                      result.push_back(triple<Move,Undo,int>(move, undo, 0));
                      if (result.size() == max_moves) return result;
                      move.special_move = 0;

                    } else {
                      //cerr << "Move " << move.toString() << " has num_checks = " << t.first << "\n";
                    }
                  }
                }
              } else {
                // Cannot capture
                can_capture = false;
              }
            }

            // KILLED[0..7]:   Used for white to move --- i.e. black captured white piece:
            //                 A->H, H->A invalid for file a and h.  WP,BP invalid for rank 1 and 8
            // KILLED[8..15]:  Used for black to move --- i.e. white captured white piece:
            //                 H->A, A->H invalid for file a and h.  BP,WP invalid for rank 1 and 8
            //
            // KILLED[16..23]: Used for white to move --- i.e. black captured white piece:
            //                 WP,BP invalid for rank 1 and 8.   A->H, H->A invalid for file a and h.
            // KILLED[24..31]: Used for black to move --- i.e. white captured white piece:
            //                 BP,WP invalid for rank 1 and 8.   H->A, A->H invalid for file a and h.
            const Piece KILLED[36] =
            {   FILE_H_TO_A_PAWN, FILE_A_TO_H_PAWN, BPAWN, WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, 0,
                FILE_A_TO_H_PAWN, FILE_H_TO_A_PAWN, WPAWN, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, 0,
                BPAWN, WPAWN, FILE_H_TO_A_PAWN, FILE_A_TO_H_PAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, 0,
                WPAWN, BPAWN, FILE_A_TO_H_PAWN, FILE_H_TO_A_PAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, 0
            };

            const Piece REAL_PIECE[38] =
            {   WPAWN, WPAWN, WPAWN, WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, 0,
                BPAWN, BPAWN, BPAWN, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, 0,
                WPAWN, WPAWN, WPAWN, WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, 0,
                BPAWN, BPAWN, BPAWN, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, 0
            };

            const uchar TRANSF[36] =
            {   5, 4, 2, 0, 0, 0, 0, 0, 0,
                5, 4, 2, 0, 0, 0, 0, 0, 0,
                2, 0, 5, 4, 0, 0, 0, 0, 0,
                2, 0, 5, 4, 0, 0, 0, 0, 0
            };

            // ALLOWED_T used when added castling puts demands on transformation
            const uchar ALLOWED_T[36] =
            {   0xA0, 0x50, 0x0C, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xA0, 0x50, 0x0C, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0x0C, 0x03, 0xA0, 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0x0C, 0x03, 0xA0, 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
            };


            int ki; // ki : killed index, index used in KILLED and TRANSF
            if (can_capture) { // calculate an index in KILLED.
              if (allowed_transformations & ~3) {

                if (ROW[to]==0 || ROW[to]==7) {
                  if (COLUMN[to]==0  ||  COLUMN[to]==7) {
                    // to is a corner square. It could not have been a pawn
                    ki = 3+9*player;
                  } else {
                    // transformation 0 and 2 can not be used for pawns
                    ki = 18+1+9*player;
                  }
                } else {
                  if (COLUMN[to]==0  ||  COLUMN[to]==7) {
                    // transformation 4 and 5 can not be used for pawns
                    ki = 0+1+9*player;
                  } else {
                    // to is an internal square, all transformations may be used
                    ki = -1+9*player;
                  }
                }

              } else {
                ki = (ROW[to]==0 || ROW[to]==7) ? (3+9*player) : (2+9*player);
              }

            } else {
              ki = 7;
            }

            do {
              ++ki;

              triple<int,uchar,Position> t = retro_move_count_checks(move.from, to, board[to], KILLED[ki]);

              if (t.first == 0) {
                // Undoing this move gives a position for which no move can capture the king.

                // An eventual extra en passant will not impose any restrictions
                // on symmetry as one other pawn is needed.
                Undo undo(ILLEGAL_POS, castling, moves_played_since_progress ? moves_played_since_progress-1 : 0,
                    player^1, t.second, t.third, REAL_PIECE[ki]);

                // "own" and "opponent" are relative to the player undoing a move

                // can't be moved out of do...while loop as it is dependent on ki
                uchar adding_own_castling = ADDED_CASTLING[move.from]  &&
                    ((1 << (ADDED_CASTLING[move.from] & 0x0F)) & allowed_transformations & ALLOWED_T[ki])  &&
                    ((KING_CASTLING_POSITIONS[move.from]  &&  PIECE_KIND[board[to]]==KING)  ||
                        (!KING_CASTLING_POSITIONS[move.from]  &&  PIECE_KIND[board[to]]==ROOK));
                uchar own_castling_transf = 0;
                if (adding_own_castling) {
                  adding_own_castling = ADDED_CASTLING[move.from] & 0xF0;
                  //cerr << "adding_own_castling = " << adding_own_castling << "\n";
                  own_castling_transf = ADDED_CASTLING[move.from] & 0x0F;
                }

                uchar adding_opponent_castling = ADDED_CASTLING[move.to]  &&
                    PIECE_KIND[KILLED[ki]]==ROOK  &&
                    ((1 << (ADDED_CASTLING[move.to] & 0x0F)) & allowed_transformations & ALLOWED_T[ki]);
                uchar opponent_castling_transf = 0;
                if (adding_opponent_castling) {
                  adding_opponent_castling = ADDED_CASTLING[move.to] & 0xF0;
                  opponent_castling_transf = ADDED_CASTLING[move.to] & 0x0F;
                }


                if ((KILLED[ki] && can_capture) || (!KILLED[ki] && can_non_capture)) {

                  // An extra iteration with i==-1 takes care of the case that the
                  // pawn captured could have been an en passant pawn.

                  for (uint i=0; i<ep_possibilities.size(); i++) {

                    if (ep_possibilities[i].accepted(move.from, to, undo.en_passant, KILLED[ki])) {

                      if (TRANSF[ki]) {
                        // We need transformed versions of move and undo
                        Move transf_move(move);
                        Undo transf_undo(undo);

                        transf_move.from = reflect(transf_move.from, TRANSF[ki]);
                        transf_move.to = reflect(transf_move.to, TRANSF[ki]);
                        if (legal_pos(transf_undo.threat_pos))
                          transf_undo.threat_pos = reflect(transf_undo.threat_pos, TRANSF[ki]);
                        if (legal_pos(transf_undo.en_passant))
                          transf_undo.en_passant = reflect(transf_undo.en_passant, TRANSF[ki]);

                        if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                        result.push_back(triple<Move,Undo,int>(transf_move, transf_undo, TRANSF[ki]));
                        if (result.size() == max_moves) return result;

                      } else {

                        if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                        result.push_back(triple<Move,Undo,int>(move, undo, TRANSF[ki]));
                        if (result.size() == max_moves) return result;
                      }

                      if (adding_opponent_castling) {
                        // Adding with castling capability for the captured rook

                        // Creating new move and undo that are updated with respect to castling
                        // and transformed accordingly.
                        Move transf_move(move);
                        Undo transf_undo(undo);
                        if (opponent_castling_transf) {
                          transf_move.from = reflect(transf_move.from, opponent_castling_transf);
                          transf_move.to = reflect(transf_move.to, opponent_castling_transf);
                          if (legal_pos(transf_undo.threat_pos))
                            transf_undo.threat_pos = reflect(transf_undo.threat_pos, opponent_castling_transf);
                          if (legal_pos(transf_undo.en_passant))
                            transf_undo.en_passant = reflect(transf_undo.en_passant, opponent_castling_transf);
                        }
                        assert(!(undo.castling & adding_opponent_castling));
                        transf_undo.castling |= adding_opponent_castling;

                        if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                        result.push_back(triple<Move,Undo,int>(transf_move, transf_undo, opponent_castling_transf));
                        if (result.size() == max_moves) return result;

                        if (adding_own_castling  && own_castling_transf==opponent_castling_transf) {
                          // Adding with castling capabilities for both rooks
                          assert(PIECE_KIND[board[to]] == ROOK  &&  board[to]+KILLED[ki] == WROOK+BROOK);

                          assert(!(undo.castling & adding_own_castling));
                          transf_undo.castling |= adding_own_castling;

                          if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                          result.push_back(triple<Move,Undo,int>(transf_move, transf_undo, own_castling_transf));
                          if (result.size() == max_moves) return result;
                        }
                      }


                      if (adding_own_castling) {

                        // Creating new move and undo that are updated with respect to castling
                        // and transformed accordingly.
                        Move transf_move(move);
                        Undo transf_undo(undo);
                        if (own_castling_transf) {
                          transf_move.from = reflect(transf_move.from, own_castling_transf);
                          transf_move.to = reflect(transf_move.to, own_castling_transf);
                          if (legal_pos(transf_undo.threat_pos))
                            transf_undo.threat_pos = reflect(transf_undo.threat_pos, own_castling_transf);
                          if (legal_pos(transf_undo.en_passant))
                            transf_undo.en_passant = reflect(transf_undo.en_passant, own_castling_transf);
                        }
                        assert(!(undo.castling & adding_own_castling));
                        transf_undo.castling |= adding_own_castling;

                        if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                        result.push_back(triple<Move,Undo,int>(transf_move, transf_undo, own_castling_transf));
                        if (result.size() == max_moves) return result;

                        if (bit_count[adding_own_castling] == 2) {
                          // 2 more cases
                          transf_undo.castling = castling |
                              (adding_own_castling & (adding_own_castling>>1));
                          if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                          result.push_back(triple<Move,Undo,int>(transf_move, transf_undo, own_castling_transf));
                          if (result.size() == max_moves) return result;

                          transf_undo.castling = castling |
                              (adding_own_castling & (adding_own_castling<<1));
                          if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                          result.push_back(triple<Move,Undo,int>(transf_move, transf_undo, own_castling_transf));
                          if (result.size() == max_moves) return result;
                        }
                      }
                    }
                  }
                }

              } else {
                //cerr << "Move " << move.toString() << " has num_checks = " << t.first << "\n";
              }
            } while (KILLED[ki]);
          }

          if (board[move.from]) move.from = INV_PIECE_JUMP(board[to], move.to, move.from);
          else move.from = INV_PIECE_NEXT(board[to], move.to, move.from);
        }
      }


      // #############################################################
      // ################     PAWN PROMOTION     #####################
      // #############################################################
      //cerr << "Examining pawn promotion moves (allowed t. = " << allowed_transformations << ")\n";
      if (allow_pawn_promotion  &&  !BORDER_DISTANCE[to]  &&
          PIECE_KIND[board[to]] != PAWN  &&  PIECE_KIND[board[to]] != KING) {
        // 8/2 = 4 kinds of ways the board can have been transformed
        // (the 2 symmetries with pawns extended to 8)
        const int TRANSF[4] = {0,2,4,5};
        // How to view TRANSF relative to white pawn
        //   a-- 0 --b
        // b           b
        // |           |
        // 5           4
        // |           |
        // a           a
        //   a-- 2 --b
        for (int r=0; r<4; r++) {
          int t = TRANSF[r];
          if (((1<<t) & allowed_transformations)  &&
              ROW[reflect(to, t)] == (player ? 7 : 0)  &&
              (LEGAL_FROM_TO[to] & LEGAL_TO)) {
            //cerr << "Considering transformation " << t << "\n";
            // t transforms the board such that the piece board[to]
            // could just have been created by a pawn promotion.


            Move move;
            {
              int pos = reflect(to, t);
              move = Move(INV_PIECE_JUMP(player ? WPAWN : BPAWN, pos, pos), pos);
              move.special_move = board[to];//pawn promotion move
            }


            while (legal_pos(move.from)) {

              bool diagonal_move = ((ROW[move.from] != ROW[move.to])  &&
                  (COLUMN[move.from] != COLUMN[move.to]));
              if (allow_captures || !diagonal_move) {
                // now move gives a move in the transformed board
                Position from = inv_reflect(move.from, t);
                // now from,to gives the squares in the untransformed board

                //cerr << "(" << POS_NAME[from] << "," << POS_NAME[to] << ") -> ("
                //<< POS_NAME[move.from] << "," << POS_NAME[move.to] << ")\n";

                if (!board[from]  &&  (LEGAL_FROM_TO[from] & LEGAL_FROM)) {
                  // Property diagonal_move invariant to transformations
                  int kill = diagonal_move ? (player ? 7 : 1) : QUEEN;

                  do {
                    if (PIECE_KIND[kill] == QUEEN) kill = 0;
                    else ++kill;

                    //cerr << "move = " << move.toString() << ", diag. = " << diagonal_move
                    //     << ", kill = " << kill << "\n";

                    if ((kill!=0)  ==  diagonal_move) {

                      // Beware!, do not use move, but instead to,from
                      uchar adding_opponent_castling = ADDED_CASTLING[to]  &&
                          PIECE_KIND[kill]==ROOK  &&  (ADDED_CASTLING[to] & 0x0F) == t;
                      if (adding_opponent_castling)
                        adding_opponent_castling = ADDED_CASTLING[to] & 0xF0;


                      Piece pawns[2][4] = {{BPAWN, WPAWN, FILE_H_TO_A_PAWN, FILE_A_TO_H_PAWN},
                          {WPAWN, BPAWN, FILE_A_TO_H_PAWN, FILE_H_TO_A_PAWN}};
                      triple<int,uchar,Position> tr = retro_move_count_checks(from, to, pawns[player][r], kill);
                      if (t  &&  tr.third!=ILLEGAL_POS)
                        tr.third = reflect(tr.third, t);

                      // Luckily no possibility for added castling capabilities

                      if (tr.first == 0) {
                        Undo undo(0, castling,
                            moves_played_since_progress ? moves_played_since_progress-1 : 0,
                                player^1, tr.second, tr.third, kill);

                        for (uint i=0; i<ep_possibilities.size(); i++) {
                          if (ep_possibilities[i].accepted(move.from, to, undo.en_passant, kill)) {
                            // Do not modify en passant. if it is defined then t==0
                            // The same holds for castling

                            if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                            result.push_back(triple<Move,Undo,int>(move, undo, t));
                            if (result.size() == max_moves) return result;

                            if (adding_opponent_castling) {
                              if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                              undo.castling |= adding_opponent_castling;
                              result.push_back(triple<Move,Undo,int>(move, undo, t));
                              undo.castling &= ~adding_opponent_castling;
                              if (result.size() == max_moves) return result;
                            }
                          }
                        }

                      } else {
                        //cerr << "Move " << move.toString() << " has num_checks = " << tr.first << "\n";
                      }
                    }
                  } while (kill);
                }
              }

              // INV_PIECE_NEXT or INV_PIECE_JUMP is the same
              move.from = INV_PIECE_NEXT(player ? WPAWN : BPAWN, move.to, move.from);
            }
          }
        }
      }


      // #############################################################
      // ################        CASTLING        #####################
      // #############################################################
      if (allow_castling  &&  PIECE_KIND[board[to]] == KING) {

        // CRS[0][x] <=> LONG CASTLING, CRS[1][x] <=> SHORT CASTLING
        const Position CASTLE_ROOK_SQUARE[2][64] =
        {{  0,0,3,0,0,4,0,0,
            0,0,0,0,0,0,0,0,
            24,0,0,0,0,0,0,31,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            32,0,0,0,0,0,0,39,
            0,0,0,0,0,0,0,0,
            0,0,59,0,0,60,0,0
        },
        {   0,2,0,0,0,0,5,0,
            16,0,0,0,0,0,0,23,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            40,0,0,0,0,0,0,47,
            0,58,0,0,0,0,61,0
        }};

        const uchar UNDONE_CASTLING[64] =
        {   0,BLACK_SHORT_CASTLING,BLACK_LONG_CASTLING,0,0,BLACK_LONG_CASTLING,BLACK_SHORT_CASTLING,0,
            BLACK_SHORT_CASTLING,0,0,0,0,0,0,BLACK_SHORT_CASTLING,
            BLACK_LONG_CASTLING,0,0,0,0,0,0,BLACK_LONG_CASTLING,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            BLACK_LONG_CASTLING,0,0,0,0,0,0,BLACK_LONG_CASTLING,
            BLACK_SHORT_CASTLING,0,0,0,0,0,0,BLACK_SHORT_CASTLING,
            0,BLACK_SHORT_CASTLING,BLACK_LONG_CASTLING,0,0,BLACK_LONG_CASTLING,BLACK_SHORT_CASTLING,0
        };


        for (int c=0; c<2; c++) {

          //cerr << "Checking " << (c?"short":"castling") << " castling.\n";

          if (CASTLE_ROOK_SQUARE[c][to]  &&
              board[CASTLE_ROOK_SQUARE[c][to]]+2 == board[to]) {
            // king and rook placed correctly!

            Move king_move(2*CASTLE_ROOK_SQUARE[c][to] - to, to);
            Move rook_move((3-c)*to - (2-c)*CASTLE_ROOK_SQUARE[c][to], CASTLE_ROOK_SQUARE[c][to]);

            if (!board[king_move.from]  &&  !board[rook_move.from]  &&
                (c || !board[2*to - CASTLE_ROOK_SQUARE[c][to]])  &&
                ((1 << KING_REFLECTIONS[player^1][king_move.from]) & allowed_transformations)) {
              /*
	      cerr << "The 2 or 3 required squares are empty.\n"
		   << "Also, the transformation " << KING_REFLECTIONS[player^1][king_move.from]
		   << " is allowed.\n";
               */

              // The lines below also asserts that the king involved in the castling
              // could not capture the other king before the castling move.
              player ^= 1;
              bool king_checked = (check_if_king_placed(rook_move.to) ||
                  check_if_king_placed(king_move.from));
              player ^= 1;

              if (!king_checked) {
                // Move rook
                triple<int,uchar,Position> t = retro_move_count_checks(rook_move.from, rook_move.to,
                    board[rook_move.to], 0);

                if (t.first == 0) {
                  //cerr << "Undoing this move gives a position for which no move can capture the king.\n";

                  Undo undo(0, castling | (UNDONE_CASTLING[king_move.to] >> (2*player)),
                      moves_played_since_progress ? moves_played_since_progress-1 : 0,
                          player^1, t.second, t.third, 0);

                  int transf = KING_REFLECTIONS[player^1][king_move.from];

                  uchar adding_own_castling = ADDED_CASTLING[king_move.from] & 0xF0;

                  for (uint i=0; i<ep_possibilities.size(); i++) {
                    if (ep_possibilities[i].accepted(rook_move.from, rook_move.to, undo.en_passant, 0)) {

                      if (undo.en_passant != ILLEGAL_POS)
                        undo.en_passant = reflect(undo.en_passant, transf);

                      if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                      //cerr << "Create new move struct - in case transf != 0, then from, to is wrong.\n";
                      result.push_back(triple<Move,Undo,int>(Move(UNDONE_CASTLING[king_move.to] >> (2*player)),
                          undo, transf));
                      if (result.size() == max_moves) return result;

                      if (adding_own_castling) {
                        // At most one further castling capability added!
                        assert(!(undo.castling & adding_own_castling));
                        assert(!(adding_own_castling & (adding_own_castling<<1)));

                        undo.castling |= adding_own_castling;

                        if (result.size() == result.capacity()) result.reserve(2*result.capacity());
                        // Create new move struct - in case transf != 0, then from, to is wrong.
                        result.push_back(triple<Move,Undo,int>(Move(UNDONE_CASTLING[king_move.to] >> (2*player)),
                            undo, transf));
                        if (result.size() == max_moves) return result;

                        undo.castling &= ~adding_own_castling;
                      }
                    }
                  }

                } else {
                  //cerr << "Move " << king_move.toString() << " has num_checks = " << t.first << "\n";
                }
              }
            }
          }
        }
      }
    }
  }

  return result;
}







int Board2::unreachable_position(int test_depth) {

  vector<triple<Move,Undo,int> > rm = get_retro_moves(true, true, true, true, 1);

  if (rm.size()==0) return 1;  // Unsuccessful
  if (test_depth==1) return 0; // Required depth can be reached :-)

  if (rm[0].third) transform_board(rm[0].third);
  undo_move(rm[0].first, rm[0].second);

  int result = unreachable_position(test_depth-1);

  execute_move(rm[0].first);
  if (rm[0].third) inv_transform_board(rm[0].third);

  if (result) {
    // Required depth could not be reached just by examining first move.
    // Do a throughout check.
    rm = get_retro_moves(true, true, true, true);

    for (uint i=0; i<rm.size(); i++) {

      if (rm[i].third) transform_board(rm[i].third);
      undo_move(rm[i].first, rm[i].second);

      int tmp = unreachable_position(test_depth-1);
      if (tmp > result) result = tmp;

      execute_move(rm[i].first);
      if (rm[i].third) inv_transform_board(rm[i].third);

      if (tmp==0) {
        // Successful!
        return 0;
      }
    }
    return result+1;
  } 
  return 0;
}
