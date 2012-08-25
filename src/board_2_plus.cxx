#include "board_2_plus.hxx"

#include <assert.h>

#include "util/help_functions.hxx"
#include "board_define_position_constants.hxx"
#include "board_move_tables.hxx"
#include "board_tables.hxx"

#define PS false

Board2plus::Board2plus() : Board2(), see(*this)
{
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Board2plus constructor called.\n";
  initialize_move_tables();

  // new_game not called here
}

void Board2plus::reset_all() {
  see.clear();
  piece_number.clear();
  move_to_index.clear();
  piece_moves.clear();
  square_move_list.clear();
  move_list.clear();
  Board2::reset_all();
}

// More efficient impl than in board_2
int Board2plus::calc_game_status_ignore_50_move_rule_and_move_repetition() {
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

  // Improved part
  // Check if player has any legal moves
  move_list.init_iterator(player);
  while (move_list.iterate()) {
    Move move = move_list.deref_iterator();
    // todo: totally_legal_move fucks up the move_list order
    if (legal_move(move)) {
      // Game continues...
      return game_status_reason = GAME_OPEN;
    }
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


//##################################################################

// if this function returns true, and hash_value_after_move(move)
// is found in transposition table, then the move is legal
bool Board2plus::legal_move(Move& move) {
  assert(legal_pos(move.from) && legal_pos(move.to) && board[move.from]);

  // Cannot move enemy pieces (error in line below)
  //  if (PIECE_COLOR[board[move.from]] != player) return false;

  // can only move own pieces
  // This check is not nescessary with moves from MoveList (always satisfied).
  if (PIECE_COLOR[board[move.from]] != player) {
    if (PS) cerr << "move " << move.toString() << " rejected - can only move own pieces.\n";
    return false;
  }

  // cannot capture own pieces
  if (PIECE_COLOR[board[move.to]] == player) {
    if (PS) cerr << "move " << move.toString() << " rejected - can't capture own piece.\n";
    return false;
  }

  // pawns must attack when diagonal, castling can't attack etc.
  if (board[move.to]) {
    if (!(move.blah & CAN_ATTACK)) {
      if (PS) cerr << "move " << move.toString() << " rejected - piece can't attack.\n";
      return false;
    }
  } else {
    if (!(move.blah & CAN_NON_ATTACK)) {
      // Only exception is en passant capture
      if (move.to == en_passant  &&  PIECE_KIND[board[move.from]] == PAWN) {
	move.special_move = EN_PASSANT;
      } else {
	if (PS) cerr << "move " << move.toString() << " rejected - piece must attack.\n";
	return false;
      }
    }
  }

  switch (PIECE_KIND[board[move.from]]) {
  case KING:
    if (move.is_castling()) {
      
      // Cannot castle if checked
      if (num_checks) {
	if (PS) cerr << "move " << move.toString() << " rejected - can't castle when checked.\n";
	return false;
      }
      
      // already checked
      //if (board[move.to]) return false;
      
      // Have player lost castling option?
      if (!(castling & CASTLING[move.to])) {
	if (PS) cerr << "move " << move.toString() << " rejected - can't castle, have lost ability.\n";
	return false;
      }
      
      // Are there any blocking pieces?
      // (mid = (move.from+move.to)/2  has already been checked)
      if (CASTLING[move.to] & (WHITE_LONG_CASTLING + BLACK_LONG_CASTLING)) {
	// Check also that b1/b8 is not occupied
	if (board[move.to - 1]) {
	  if (PS) cerr << "move " << move.toString() << " rejected - can't castle, a piece block the path.\n";
	  return false;
	}
      }
      
      // Will the king have to pass any checking positions?
      
      if (check_if_king_placed((move.from + move.to) >> 1)) {
	if (PS) cerr << "move " << move.toString() << " rejected - can't castle, will pass threat.\n";
	return false;
      }
    }
    return !check_if_king_placed(move.to);

  case PAWN:
    if (num_checks  &&  move.to == en_passant) {
      // If the king is checked af an en passant move, then the
      // only possible checking piece is the en passant pawn.
      // This capturing pawn move is legal if the pawns doesn't
      // protect the king.
      return !(check_if_moved(move.from));
    }
    // no break;

  default:
    if (num_checks) {
      if (num_checks == 2) return false;
      if (check_if_moved(move.from)) {
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
      if (check_if_moved(move.from))
	return DIRECTION[move.from][king_pos[player]] == DIRECTION[move.from][move.to];
      return true;
    }
  }
}

// The path must be obstacle free.
// todo: define_move_type need only be set once for every piece
void Board2plus::define_move_type(Move& move) {
  // set move.special_move, move.blah

  assert(legal_pos(move.from) && legal_pos(move.to) && board[move.from]);

  move.special_move = move.blah = 0;

  switch (PIECE_KIND[board[move.from]]) {
  case PAWN:
    if ((move.from ^ move.to) & 7) {
      // A diagonal move.
      move.blah = CAN_ATTACK;
      
      if (ROW[move.to]==0) move.special_move = BQUEEN;
      else if (ROW[move.to]==7) move.special_move = WQUEEN;
    } else {
      // A forward move.
      if ((ROW[move.from]==1  &&  ROW[move.to]==2)  ||
	  (ROW[move.from]==6  &&  ROW[move.to]==5)) {
	// This move blocks the 2 forward move, if move.to is occupied
	move.blah = CAN_NON_ATTACK | FURTHER_MOVEMENT_POSSIBLE;
      } else {
	// No further movement in direction possible
	move.blah = CAN_NON_ATTACK;
	
	if (ROW[move.to]==0  ||  ROW[move.to]==7)
	  move.special_move = board[move.from] + (QUEEN-PAWN);
      }
    }
    break;
    
  case KNIGHT:
    move.blah = CAN_ATTACK | CAN_NON_ATTACK;
    break;

  case KING:
    if (CASTLING[move.from]) {
      if (CASTLING[move.to]) {
	// Do not tjeck here if king has lost ability to castle -
	// will be to difficult to maintain.
	move.special_move = CASTLING[move.to];
	move.blah = CAN_NON_ATTACK;
      } else {
	move.blah = CAN_ATTACK | CAN_NON_ATTACK;
	if (move.to==d1 || move.to==f1 || move.to==d8 || move.to==f8)
	  move.blah |= FURTHER_MOVEMENT_POSSIBLE;
      }
    } else {
      move.blah = CAN_ATTACK | CAN_NON_ATTACK;
    }
    break;

  default:
    move.blah = CAN_ATTACK | CAN_NON_ATTACK | FURTHER_MOVEMENT_POSSIBLE;
    break;
  }
}


void Board2plus::add_move(Move move) {
  if (PS) cerr << "Board2plus::add_move(" << move.toString() << ")\n";
  see.increase_control(move, board[move.from]);
  // update move_to_index, piece_moves, square_move_list and move_list
  uchar move_index = move_list.add_move(PIECE_COLOR[board[move.from]], move);
  // cerr << "TEST: " << (int)move_index << "\n";
  move_to_index[move] = IndexStruct(piece_moves.add_to_list(piece_number[move.from], move_index),
				    square_move_list.insert_move(move.to, move_index),
				    move_index);
}

bool Board2plus::remove_move(Move move) {
  if (PS) cerr << "Board2plus::remove_move(" << move.toString() << ")\n";
  // update move_to_index, piece_moves, square_move_list and move_list
  IndexStruct index = move_to_index[move];
  if (!index.defined) {
    //cerr << "move " << move.toString() << " not removed!\n";
    return false;
  }

  see.decrease_control(move, board[move.from]);

  if (piece_moves.remove_from_list(piece_number[move.from], index.piece_moves_index)) {
    // gap and relocation occured in piece_moves
    int move_index = piece_moves.index(piece_number[move.from], index.piece_moves_index);
    move_to_index.piece_moves_index(move_list[move_index]) = index.piece_moves_index;
  }

  if (square_move_list.remove_move(move.to, index.square_move_list_index)) {
    // gap and relocation occured in square_move_list
    int move_index = square_move_list.index(move.to, index.square_move_list_index);
    move_to_index.square_move_list_index(move_list[move_index]) = index.square_move_list_index;
  }

  move_list.remove_move(index.move_list_index);

  move_to_index[move].clear();

  return true;
}


void Board2plus::remove_blocked_moves(Position pos) {
  if (PS) cerr << "BEGIN remove_blocked_moves(" << POS_NAME[pos] << ")\n";
  // remove all moves that will be blocked by this piece:
  square_move_list.init_iterator(pos);
  while (square_move_list.iterate()) {
    Move move = move_list[square_move_list.deref_iterator()];
    if (move.blah & FURTHER_MOVEMENT_POSSIBLE) {
      if (PS) cerr << "Further movement for move " << move.toString() << "\n";
      // The moves generated after *m might be blocked
      move.to = PIECE_DNEXT(board[move.from], move.from, move.to);
      while (legal_pos(move.to)) {
	define_move_type(move);
	// removing *m won't influence square_move_list on pos. It won't mess
	// with the iterator either. Hence it can safely be done here.
	if (!remove_move(move)) break;

	if (board[move.to]) break;
	move.to = PIECE_DNEXT(board[move.from], move.from, move.to);
      }
    } else {
      if (PS) cerr << "Move " << move.toString() << " has not further movement.\n";
    }
  }
  if (PS) cerr << "END remove_blocked_moves(" << POS_NAME[pos] << ")\n";
}


void Board2plus::add_blocked_moves(Position pos) {
  // add all moves that was being blocked by this piece:
  // only difference in the code below is that remove_move(...) is replaced by add_move(...)
  if (PS) cerr << "BEGIN add_blocked_moves(" << POS_NAME[pos] << ")\n";
  square_move_list.init_iterator(pos);
  while (square_move_list.iterate()) {
    Move move = move_list[square_move_list.deref_iterator()];
    if (PS) cerr << "move " << move.toString() << "\n";
    if (move.blah & FURTHER_MOVEMENT_POSSIBLE) {
      if (PS) cerr << "Further movement for move " << move.toString() << "\n";
      // The moves generated after *m might be blocked
      move.to = PIECE_DNEXT(board[move.from], move.from, move.to);
      while (legal_pos(move.to)) {
	define_move_type(move);
	// adding *m won't influence square_move_list on pos. It won't mess
	// with the iterator either. Hence it can safely be done here.
	add_move(move);

	if (board[move.to]) break;
	move.to = PIECE_DNEXT(board[move.from], move.from, move.to);
      }
    } else {
      if (PS) cerr << "Move " << move.toString() << " has not further movement.\n";
    }
  }
  if (PS) cerr << "END add_blocked_moves(" << POS_NAME[pos] << ")\n";
}

void Board2plus::remove_moves_by_piece_on_pos(Position pos) {
  int piece_nr = piece_number[pos];
  int i = piece_moves.count(piece_nr);
  while (i--) {
    // delete the list (piece_moves.index(index, ...) from the
    // back to avoid creating gaps (and thereby relocations)
    remove_move(move_list[piece_moves.index(piece_nr, i)]);
  }
}

void Board2plus::add_moves_by_piece_on_pos(Position pos) {
  Piece piece = board[pos];
  Move move(pos, PIECE_JUMP(piece, pos, pos));
  while (legal_pos(move.to)) {
    define_move_type(move);
    add_move(move);
    if (board[move.to]) move.to = PIECE_JUMP(piece, move.from, move.to);
    else move.to = PIECE_NEXT(piece, move.from, move.to);
  }
}

/* Not used, not needed
void Board2plus::initialize_move_list() {
  for (int p=0; p<64; p++)
    if (board[p]) {
      Move move(p, p);
	
      if (board[move.to]) move.to = PIECE_JUMP(board[move.from], move.from, move.to);
      else move.to = PIECE_NEXT(board[move.from], move.from, move.to);

      while (legal_pos(move.to)) {
	
	define_move_type(move);
	add_move(move);
	
	if (board[move.to]) move.to = PIECE_JUMP(board[move.from], move.from, move.to);
	else move.to = PIECE_NEXT(board[move.from], move.from, move.to);
      }
    }
}
*/


// legal_move tjecks whether move.from and move.to matches a legal move.
// In case of a match move.special_move and move.blah is set
// (pawn promotion: default is queen).
bool Board2plus::find_legal_move(Move& move) {
  if (!move_to_index.defined(move)) return false;

  move = move_list[move_to_index.move_list_index(move)];
  return legal_move(move);
}

//######################################################################
//######################################################################
//######################################################################

/*
 Optimization: Capture move/undoing this: two calls can be optimized away.
 REMOVE_PIECE(TO):
 remove_moves_by_piece(to)
 add_blocked_moves(to) // *
 remove_piece(to)
 REMOVE_PIECE(FROM):
 remove_moves_by_piece(from)
 add_blocked_moves(from)
 remove_piece(from)
 INSERT_PIECE(TO):
 insert_piece(to)
 remove_blocked_moves(to) // *
 add_moves_by_piece(to)
*/

void Board2plus::remove_piece(Position pos) {
  assert(legal_pos(pos)  &&  board[pos]);

  see.remove_piece(pos);
  add_blocked_moves(pos);

  remove_moves_by_piece_on_pos(pos);
  piece_number.remove_piece(pos);

  Board2::remove_piece(pos);
}

// hvis en bonde forfremmes til en dronning samtidig med at den slår en brik
// går der vist rod i nummereringen
void Board2plus::insert_piece(Position pos, Piece piece) {
  if (PS) cerr << "BEFORE INSERT PIECE " << PIECE_NAME[piece] << " on "
	       << POS_NAME[pos] << ", num_checks = " << (int)num_checks << "\n";
  // square_move_list.print(big_output);

  assert(legal_pos(pos));

  /*
  if (!legal_pos(pos)) {
    cerr << "Trying to place piece " << PIECE_NAME[piece] << " on " << POS_NAME[pos] << "\n";
    print_board(cerr);
    throw Error("place piece");
  }
  */

  Piece captured_piece = board[pos];
  if (captured_piece) {
    see.remove_piece(pos);
    remove_moves_by_piece_on_pos(pos);
    piece_number.remove_piece(pos);
  }

  Board2::insert_piece(pos, piece);

  if (!captured_piece)
    remove_blocked_moves(pos);

  see.insert_piece(pos, piece);
  piece_number.insert_piece(pos, piece);
  add_moves_by_piece_on_pos(pos);
}

void Board2plus::move_piece(Position from, Position to) {
  if (!(legal_pos(from)  &&  legal_pos(to)  &&  board[from])) {
    cerr << "Error while moving from " << POS_NAME[from] << " to "
	 << POS_NAME[to] << " in position\n";
    print_board(cerr);
    assert(0);
  }
  
  see.remove_piece(from);
  remove_moves_by_piece_on_pos(from);

  Piece captured_piece = board[to];
  if (captured_piece) {
    see.remove_piece(to);
    remove_moves_by_piece_on_pos(to);
    piece_number.remove_piece(to);
  }

  Board2::move_piece(from, to);

  // This call must be between remove_moves_by_piece_on_pos(from) and add_moves_by_piece_on_pos(to)
  piece_number.move_piece(from, to);

  see.insert_piece(to, board[to]);
  add_blocked_moves(from);
  if (!captured_piece)
    remove_blocked_moves(to);

  add_moves_by_piece_on_pos(to);
}

//#####################################################################

bool Board2plus::loadFEN(string FEN) {
  if (Board2::loadFEN(FEN)) {
    remap_piece_numbers();
    return true;
  }
  return false;
}
bool Board2plus::internal_set_board() {
  if (Board2::internal_set_board()) {
    remap_piece_numbers();
    return true;
  }
  return false;
}

// Standard mapping: se #define's in board_2b_help_classes
void Board2plus::remap_piece_numbers() {
  // todo. Currently the piece numbers are not used for anything anyway
  /*
  bool used[32];
  memset(used, false, 32);

  Position pos[13][9];
  int pos_count[13];
  for (int i=0; i<13; i++) pos_count[i] = 0;

  for (int i=0; i<64; i++) if (board[i])
    pos[board[i]][pos_count[board[i]]++] = i;

  uchar mapping[32];
  
  // Kings
  mapping[piece_number[pos[WKING][0]]] = W_KING_ID;
  mapping[piece_number[pos[BKING][0]]] = B_KING_ID;
  
  // Pawns
  
  // todo...

  // Apply mapping
  piece_number.remap_pieces(mapping);
  piece_moves.remap_pieces(mapping);
  */
}



// sanity_check checks whether the moves generated by board_2_plus
// in the current position correspond to those generated by board_2
bool Board2plus::sanity_check_moves(ostream &os) {
  if (!piece_number.sanity_check(cerr)) return false;
  if (!move_list.sanity_check(cerr)) return false;

  bool found[128];
  memset(found, false, 128);
  MyVector<Move> mlist(48);
  get_move_list(mlist);
  //cerr << "mlist.size() = " << mlist.size() << "\n";
  //for (int i=0; i<mlist.size(); i++)
  //  cerr << "mlist[" << i << "] = " << mlist[i].toString() << "\n";

  bool result = true;
  Move move = moves();
  while (next_move(move)) {
    // find and erase from mlist
    bool ok = false;
    for (int i=0; i<mlist.size(); i++)
      if (mlist[i] == move) {
	found[i] = true;
	ok = true;
	break;
      }
    if (!ok) {
      os << "move " << move.toString() << " not returned by Board2plus\n";
      result = false;
    }
  }

  for (int i=0; i<mlist.size(); i++) {
    if (!found[i]  &&  legal_move(mlist[i])) {
      os << "move " << mlist[i].toString() << " not returned by Board2\n";
      result = false;
    }
  }
  
  return result;
}



// #############################################


inline string cts(char ch) {
  string result(" ");
  result[0] = ch;
  return result;
}

string Board2plus::moveToSAN(Move move) {
  if (!move.is_defined()) {
    if (move.is_null_move()) return "NullMove";
    return "#move#";
  }

  assert(legal_pos(move.from) && legal_pos(move.to) && board[move.from]);

  string suffix = "";
  {
    Undo undo = execute_move(move);
    //cerr << "moveToSAN(" << move.toString() << "), num_checks after move = " << (int)num_checks << "\n";
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
  
  //cerr << "Board2plus::moveToSAN(" << move.toString() << ")...\n";

  bool ambiguity = false;
  bool same_row = false;
  bool same_colum = false;
  square_move_list.init_iterator(move.to);
  while (square_move_list.iterate()) {
    Move m = move_list[square_move_list.deref_iterator()];
    if (m.from != move.from  &&
	PIECE_COLOR[board[m.from]] == player  &&
	PIECE_KIND[board[m.from]] == piece_kind  &&
	legal_move(m)) {

      //cerr << "move " << move.toString() << " disambiguet by " << m.toString() << "\n";

      ambiguity = true;
      if (COLUMN[m.from] == COLUMN[move.from])
	same_colum = true;
      if (ROW[m.from] == ROW[move.from])
	same_row = true;
    }
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


Move Board2plus::sanToMove(string san) {
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
    /*
    if (san[1] == 'x') {
      if (board[dest] == 0) {
	// en_passant!
	move.special_move = EN_PASSANT;
      }
    }
    */
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

  Move move;
  square_move_list.init_iterator(dest);
  while (square_move_list.iterate()) {
    move = move_list[square_move_list.deref_iterator()];
    if (PIECE_COLOR[board[move.from]] == player  &&
	PIECE_KIND[board[move.from]] == piece_kind  &&
	legal_move(move)) {
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
  }

  // cerr << "Hertil ok\n";

  if (!move_found) { return Move(); } // null move => error

  if (pawn_promote) move.special_move = pawn_promote;

  // cerr << "Move recognized as " << move.toString() << "\n";

  return move;
}

//###################################################3

void Board2plus::test_see() {
  //loadFEN("rnbqkb1r/pppppppp/5n2/8/4P3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 0 1");
  //print_board(cerr);

  MyVector<pair<int, Position> > see_list;
  for (int i=0; i<32; i++) {
    cerr << "Piece number " << i << ' ';
    if (piece_number.number_used(i)) {
      Position pos = piece_number.get_pos(i);
      cerr << "pos(" << POS_NAME[pos] << ") ";
      int tmp = see.see(pos, board[pos]);
      cerr << " see-value = " << (int)tmp << "\n";
      if (tmp) see_list.push_back(pair<int, Position>(tmp, pos));
    } else {
      cerr << " not used\n";
    }
  }

  for (int i=0; i<see_list.size(); i++) {
    cerr << "Position[" << POS_NAME[see_list[i].second] << "], value = "
	 << (int)(see_list[i].first) << "\n";
  }

  cerr << "Other method:\n";
  for (int player=0; player<2; player++) {
    cerr << (player?"Black":"White") << ":\n";
    for (int i=0; i<see.size(player); i++) 
      cerr << "Pos(" << POS_NAME[see.target_position(player, i)] << "), value = "
	   << see.target_value(player, i) << "\n";
  }
}


void Board2plus::init_see_list(MyVector<pair<Position, char> > &see_list, bool player,
			       int &num_protect, int &num_attack) {
  num_protect = num_attack = 0;

  for (int i=0; i<32; i++) {
    if (piece_number.number_used(i)) {
      Position pos = piece_number.get_pos(i);
      char tmp = see.see(pos, board[pos]);
      if (tmp) {
	if (PIECE_COLOR[board[pos]] == player) {
	  ++num_protect;
	  see_list.push_back(pair<Position, char>(pos, -tmp));
	} else {
	  ++num_attack;
	  see_list.push_back(pair<Position, char>(pos, tmp));
	}
      }
    }
  }
}




bool Board2plus::clr_board2plus(Board *board, ostream &os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Board2plus &b = *dynamic_cast<Board2plus *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Board2plus, help:\n"
       << "    print board  or  pb  or  dir\n"
       << "      - print board\n"
       << "    test see  or  ts\n"
       << "    print see  or  ps\n"
       << "    print targets  or  pt\n"
       << "    see list  or  sl\n"
       << "    test see list  or  tsl\n"
       << "    see W kqrbnp KQRBNP\n"
       << "      - example: \"see P 101000 000100\" will print out 0,\n"
       << "        because it does not pay for black king or rook to attack\n"
       << "        the pawn being defended by a bishop.\n"
       << "    see2 x y z\n"
       << "      - x is 'P','N',... or 'K'"
       << "        y and z are 3-digit decimal numbers in [0..198[\n"
       << "    print piece numbers  or  ppn\n";

  } else if (dot_demand(p, 2, "test", "see")) {
    b.test_see();

  } else if (dot_demand(p, 2, "print", "see")) {
    b.see.print(cerr);

  } else if (dot_demand(p, 2, "print", "targets")) {
    b.see.print_targets(cerr);

  } else if (dot_demand(p, 2, "see", "list")) {
    b.see.print_compression_list(cerr);

  } else if (dot_demand(p, 3, "test", "see", "list")) {
    b.see.test_see_list();

  } else if (dot_demand(p, 4, "see", (ptr_int)1, (ptr_int)6, (ptr_int)6)) {
    Piece victim = PIECE_KIND[char_to_piece(parse_result[0][0])];
    if (victim) {
      ushort a = 0;
      a += (parse_result[1][5]-'0');
      a += (parse_result[1][4]-'0') << 2;
      a += (parse_result[1][3]-'0') << 5;
      a += (parse_result[1][2]-'0') << 8;
      a += (parse_result[1][1]-'0') << 11;
      a += (parse_result[1][0]-'0') << 15;
      cerr << "Aggressor = " << b.see.capture_list_to_string(a) << "\n";
      ushort d = 0;
      d += (parse_result[2][5]-'0');
      d += (parse_result[2][4]-'0') << 2;
      d += (parse_result[2][3]-'0') << 5;
      d += (parse_result[2][2]-'0') << 8;
      d += (parse_result[2][1]-'0') << 11;
      d += (parse_result[2][0]-'0') << 15;
      cerr << "Defender = " << b.see.capture_list_to_string(d) << "\n";
      
      int result = b.see.calc_see(victim, a, d);
      cerr << "Result of battle = " << result << " units (a pawn is 8 units).\n";
    } else {
      cerr << "Invalid target!\n";
    }

  } else if (dot_demand(p, 4, "see2", (ptr_int)1, (ptr_int)3, (ptr_int)3)) {
    Piece victim = char_to_piece(parse_result[0][0]);
    int a = 100*(parse_result[1][0]-'0') + 10*(parse_result[1][1]-'0') + (parse_result[1][2]-'0');
    int d = 100*(parse_result[2][0]-'0') + 10*(parse_result[2][1]-'0') + (parse_result[2][2]-'0');

    int result = b.see.index_see(victim, a, d);
    cerr << "Result of battle = " << result << " units (a pawn is 8 units).\n";

  } else if (dot_demand(p, 1, "dir")  ||
	     dot_demand(p, 2, "print", "board")) {
    b.print_board(os);

  } else if (dot_demand(p, 3, "print", "piece", "numbers")) {
    b.piece_number.print(os);

  } else return false;
  return true;

  
/*
    see.print(os);
    piece_number.print(os);
    move_to_index.print(os);
    piece_moves.print(os);
    square_move_list.print(os);
    move_list.print(os);
*/

}


#undef PS
