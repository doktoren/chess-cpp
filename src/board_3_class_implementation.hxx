#define exec_undo false

#define setting(x) (*(settings.x))

void ClassName::savePGN(string filename) {
  PGNWriter writer(filename);
  writer.output_game(*this);
  cerr << "Game saved as " << filename << "\n";
}

bool ClassName::loadPGN(string filename) {
  PGNLoader loader(filename);
  if (loader.next_game()) {
    loader.print_tags(cerr);
    loader.setup_game(*this);
    Move move;
    while (loader.next_move(*this, move))
      execute_move(move);
    return true;
  }
  return false;
}

#ifdef PLUS
bool ClassName::clr_board3plus(void *ignored, Board *board, ostream& os, vector<string> &p)
#else
bool ClassName::clr_board3(void *ignored, Board *board, ostream& os, vector<string> &p)
#endif
{
  Board *_b = reinterpret_cast<Board *>(board);
  ClassName &b = *dynamic_cast<ClassName *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Board, help:\n"
       << "    print board  or  pb  or  dir\n"
       << "      - print board\n"
       << "    print move stack  or  pms\n";

  } else if (dot_demand(p, 1, "dir")  ||
	     dot_demand(p, 2, "print", "board")) {
    b.print_board(os);

  } else if (dot_demand(p, 3, "print", "move", "stack")) {
    b.print_move_stack(os, false, 0);

  } else return false;
  return true;
}


void ClassName::print_move_stack(ostream& os, int from_move, bool print_undo_info) {

  if (from_move < 0) {
    from_move = (moves_played/2 + 1) + from_move;
  } else if (from_move == 0) {
    if (show_last_num_moves == 0) return;
    from_move = (moves_played/2 + 1) - show_last_num_moves;
  }

  if (from_move < 1) from_move = 1;
  from_move = 2*(from_move-1);// Now it is in ply

  if (from_move < cannot_undo_before) from_move = cannot_undo_before;

  if (print_undo_info) {
    for (int i=from_move; i<moves_played; i++) {
      os << (i >> 1)+1 << ((i&1) ? 'b' : 'w') << "  "
	 << move_stack[i].toString() << "  - " << undo_stack[i].toString() << '\n';
    }
  } else {
    if ((from_move & 1)  &&  from_move<moves_played) {
      ++from_move;
      os << (from_move>>1) << ":       " << move_stack[from_move].toString() << '\n';
    }

    for (int i=from_move>>1; 2*i<moves_played; i++) {
      os << i+1 << ": " << move_stack[i+i].toString();
      if (i+i+1<moves_played) os << "  " << move_stack[i+i+1].toString();
      os << '\n';
    }
  }
}

void ClassName::print_board(ostream& os, int from_move) {
  if (from_move != 123456789) print_move_stack(os, from_move);
  Board2::print_board(os);
}

//-----------

void ClassName::reset_all() {
  partial_hash_value = HashValue();
  move_repetition.clear();
  Extends::reset_all();
}

bool ClassName::loadFEN(string FEN) {
  if (Extends::loadFEN(FEN)) {
    cannot_undo_before = moves_played;

    // complete the hash value:
    hash_value = partial_hash_value;
    hash_value.low ^= player;
    hash_value.low ^= castling;
    hash_value.high ^= en_passant;
    return true;
  }
  return false;
}
bool ClassName::internal_set_board() {
  if (Extends::internal_set_board()) {
    cannot_undo_before = moves_played;

    // complete the hash value:
    hash_value = partial_hash_value;
    hash_value.low ^= player;
    hash_value.low ^= castling;
    hash_value.high ^= en_passant;
    return true;
  }
  return false;
}

vector<Move> ClassName::get_move_history() {
  vector<Move> result(moves_played);
  for (int i=0; i<moves_played; i++)
    result[i] = move_stack[i];
  return result;
}



ClassName::ClassName() :
  Extends(), move_repetition(LOG_MAX_GAME_LENGTH+2)
{
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << nameofclass << " constructor called.\n";
  init_hash_values();

  show_last_num_moves = 10;
}


ClassName::~ClassName() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << nameofclass << " destructor called.\n";
}


void ClassName::copy_from(ClassName &b) {
  if (b.played_from_scratch) new_game();
  else loadFEN(b.initial_position);

  vector<Move> history = b.get_move_history();
  for (uint i=0; i<history.size(); i++)
    execute_move(history[i]);
}


int ClassName::calc_game_status() {
  if (move_repetition[hash_value].num_repetitions == 3) {
    // cerr << "Hash value = " << hash_value << "\n";
    // Position occured 2 times before.
    game_status_reason = REPEATED_BOARD_POSITION;
    return GAME_DRAWN;
  }
  return Board2::calc_game_status();
}


void ClassName::execute_move(Move move) {
  // cerr << "ClassName::execute_move(" << moveToSAN(move) << ")\n";
  if (exec_undo  &&  exec_undo_activated) {
    const char L[10] = {' ','|','I','X','#','#','#','#','#','#'};
    for (int i=1; i<=moves_played; i++) {
      int p = 0;
      while (!(i & (1<<p))) ++p;
      big_output << ' ' << L[p] << ' ';
    }
    big_output << "execute_move(" << move.toString() << ")\n";
    big_output.flush();
  }

  move_stack[moves_played] = move;
  //Denne kode beder selv om at komme i problemer!
  //(execute_move har side-effekter, bl.a. på moves_played)
  //undo_stack[moves_played] = Board2::execute_move(move);
  Undo &undo = undo_stack[moves_played];
  undo = Board2::execute_move(move);

  // complete the hash value:
  hash_value = partial_hash_value;
  hash_value.low ^= player;
  hash_value.low ^= castling;
  hash_value.high ^= en_passant;

  RepetitionInfo ri = move_repetition[hash_value];
  ++ri.num_repetitions;
  move_repetition.update(ri);
}


bool ClassName::undo_move() {
  assert(moves_played >= cannot_undo_before);

  if (moves_played == cannot_undo_before) {
    cerr << "moves_played = " << moves_played << ", cannot_undo_before = "
	 << cannot_undo_before << "\n";
    return false;
  }

  if (exec_undo  &&  exec_undo_activated) {
    const char L[10] = {' ','|','I','X','#','#','#','#','#','#'};
    for (int i=1; i<=moves_played-1; i++) {
      int p = 0;
      while (!(i & (1<<p))) ++p;
      big_output << ' ' << L[p] << ' ';
    }
    big_output << "undo_move(" << move_stack[moves_played-1].toString() << ")\n";
    big_output.flush();
  }

  RepetitionInfo ri = move_repetition[hash_value];
  if (--ri.num_repetitions) move_repetition.update(ri);
  else move_repetition.remove();

  int mp = moves_played-1;
  Board2::undo_move(move_stack[mp], undo_stack[mp]);
  // complete the hash value:
  hash_value = partial_hash_value;
  hash_value.low ^= player;
  hash_value.low ^= castling;
  hash_value.high ^= en_passant;
  return true;
}

void ClassName::undo_move(Move move, Undo undo) {
  assert(moves_played >= cannot_undo_before);

  if (moves_played == cannot_undo_before  ||
      move_stack[moves_played-1] != move  ||
      undo_stack[moves_played-1] != undo) {

    if (moves_played > cannot_undo_before) {
      cerr << "Undone move is different from last played. Clearing move history.\n";
      move_repetition.clear();
      cannot_undo_before = moves_played;

    } else {//moves_played == cannot_undo_before
      --cannot_undo_before;
    }

    move_stack[moves_played-1] != move;
    undo_stack[moves_played-1] != undo;

    Board2::undo_move(move, undo);

    hash_value = partial_hash_value;
    hash_value.low ^= player;
    hash_value.low ^= castling;
    hash_value.high ^= en_passant;
  } else {
    undo_move();
  }
}

// Null moves is not theoretical sound.
// Therefore move repetition stuff is ignored
bool ClassName::try_execute_null_move() {
  // moveToSAN(Move()) = "NullMove"
  move_stack[moves_played] = Move();

  bool result = Board2::try_execute_null_move();

  // complete the hash value:
  hash_value = partial_hash_value;
  hash_value.low ^= player;
  hash_value.low ^= castling;
  hash_value.high ^= en_passant;

  return result;
}
void ClassName::undo_null_move() {
  Board2::undo_null_move();

  // complete the hash value:
  hash_value = partial_hash_value;
  hash_value.low ^= player;
  hash_value.low ^= castling;
  hash_value.high ^= en_passant;
}

void ClassName::verify_hash_value(ostream &os) {
  HashValue hv;
  for (int i=0; i<64; i++)
    if (board[i]) {
      int v = (board[i]<<6) | i;
      os << "board[" << POS_NAME[i] << "] = " << hash_values[v].toBitString() << '\n';
      hv ^= hash_values[v];
    }
  os << "player :    " << HashValue(player, 0).toBitString() << '\n';
  hv.low ^= player;
  os << "castling :  " << HashValue(castling, 0).toBitString() << '\n';
  hv.low ^= castling;
  os << "en passant: " << HashValue(0, en_passant).toBitString() << '\n';
  hv.high ^= en_passant;
  os << "Result =    " << hv.toBitString() << '\n';
  os << "RealHV =    " << hash_value.toBitString() << '\n';
  assert(hv == hash_value);
}

#define insert_piece(pos, piece) result ^= hash_values[(piece << 6) | pos]
#define remove_piece(pos) result ^= hash_values[(board[pos] << 6) | pos]
#define move_piece(from, to) {\
int tmp = board[from] << 6;\
result ^= hash_values[tmp | from];\
result ^= hash_values[tmp | to];\
}
HashValue ClassName::hash_value_after_move(Move move) {
  HashValue result = partial_hash_value;
  uchar _en_passant = ILLEGAL_POS;

  // This is largely code copy-pasted from (old version of) execute move.
  if (board[move.to]) remove_piece(move.to);

  if (move.special_move) {
    if (move.is_pawn_promotion()) {
      remove_piece(move.from);
      insert_piece(move.to, move.special_move);
    } else if (move.special_move == EN_PASSANT) {
      // Kill the pawn
      if (player) remove_piece(move.to + 8);
      else remove_piece(move.to - 8);
      move_piece(move.from, move.to);
    } else { // castling
      switch (move.special_move) {
      case WHITE_LONG_CASTLING:  move_piece(e1, c1); move_piece(a1, d1); break;
      case WHITE_SHORT_CASTLING: move_piece(e1, g1); move_piece(h1, f1); break;
      case BLACK_LONG_CASTLING:  move_piece(e8, c8); move_piece(a8, d8); break;
      case BLACK_SHORT_CASTLING: move_piece(e8, g8); move_piece(h8, f8); break;
      }
    }
  } else {
    if (PIECE_KIND[board[move.from]] == PAWN) {
      if (((move.from ^ move.to) & 0x18) == 0x10) {
	// Moved 2 positions => Allow for en passant
	Piece tmp = WPAWN+BPAWN-board[move.from];
	if ((COLUMN[move.to] > 0  &&  board[move.to-1] == tmp)  ||
	    (COLUMN[move.to] < 7  &&  board[move.to+1] == tmp)) {
	  // There is an enemy pawn ready to take advantage of the
	  // en passant. It is to bothersome to tjeck if this pawn
	  // will be unable to use the en passant because of some check.
	  _en_passant ^= (move.from + move.to) >> 1;
	}
      }
    }
    move_piece(move.from, move.to);
  }

  result.low ^= 1^player;
  result.low ^= (castling & CASTLING_LOST[move.from] & CASTLING_LOST[move.to]);
  result.high ^= _en_passant;
  return result;
}
#undef insert_piece
#undef remove_piece
#undef move_piece

// try_redo_move will always succeed after an undo_move
bool ClassName::try_redo_move() {
  Move move = move_stack[moves_played];
  int pawn_promote = move.is_pawn_promotion() ? move.blah : 0;

  if (Extends::find_legal_move(move)) {
    if (pawn_promote) move.blah = pawn_promote;
    execute_move(move);
    return true;
  }
  return false;
}

//####################################################################


void ClassName::insert_piece(Position pos, Piece piece) {
  if (board[pos])
    partial_hash_value ^= hash_values[(board[pos] << 6) | pos];
  partial_hash_value ^= hash_values[(piece << 6) | pos];
  Extends::insert_piece(pos, piece);
}


void ClassName::remove_piece(Position pos) {
  partial_hash_value ^= hash_values[(board[pos] << 6) | pos];
  Extends::remove_piece(pos);
}


void ClassName::move_piece(Position from, Position to) {
#ifndef NDEBUG
  if (!(legal_pos(from)  &&  legal_pos(to)  &&  board[from])) {
    cerr << "Error while moving from " << POS_NAME[from] << " to "
	 << POS_NAME[to] << " in position\n";
    print_board(cerr);
    assert(0);
  }
#endif
  if (board[to])
    partial_hash_value ^= hash_values[(board[to] << 6) | to];
  int tmp = board[from] << 6;
  partial_hash_value ^= hash_values[tmp | from];
  partial_hash_value ^= hash_values[tmp | to];
  Extends::move_piece(from, to);
}

#undef exec_undo
