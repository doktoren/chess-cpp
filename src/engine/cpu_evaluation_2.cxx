#include "cpu_evaluation_2.hxx"

#include "game_phase.hxx"
#include "cpu_evaluation_2_const.hxx"

Eval_2::Eval_2() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Eval_2 constructor called.\n";
  init_evaluation_constants();
}

Eval_2::~Eval_2() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Eval_2 destructor called.\n";
  //this->SearchRoutine::~SearchRoutine(); Will be called automatically
}

void Eval_2::reset_all() {
  material = 0;
  
  for (int phase=0; phase<3; phase++)
    piece_values[phase] = 0;

  Board3plus::reset_all();
}

int Eval_2::evaluate(__attribute__((unused)) int alpha, __attribute__((unused)) int beta) {
  if (!(++num_evaluations & 0xFFFF)) {
    cerr << num_evaluations << " positions evaluated\n";
  }

  int value = count_pieces();

  if (player) return -value;
  else return value;
}

bool Eval_2::clr_evaluation(Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Eval_2 &b = *dynamic_cast<Eval_2 *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Evaluation 2, help:\n"
       << "    print board  or  pb  or  dir\n"
       << "      - print board\n"
       << "    print eval stat  or  pes\n"
       << "    print evaluation  or  pe\n";
  } else if (dot_demand(p, 1, "dir")  ||
	     dot_demand(p, 2, "print", "board")) {
    b.print_board(os);
  } else if (dot_demand(p, 3, "print", "eval", "statistic")) {
    b.print_eval_stat(cerr);
  } else if (dot_demand(p, 2, "print", "evaluation")) {
    int value = b.root_evaluate();
    os << "Evaluation(current position) = " << value << '\n';

  } else return false;
  return true;
}

void Eval_2::print_eval_stat(ostream& os) {
  os << "num_evaluations = " << num_evaluations << "\n";
}

//###############  PROTECTED  ##################

void Eval_2::init_evaluation() {
  num_evaluations = 0;
}

int Eval_2::count_pieces() {
  /*
  // Consider the game phases in 2 ways:
  // 1) as a function of moves played
  // 2) as a function of material left
  
  int result;

  if (moves_played < END_OPENING_GAME_IN_PLY) {
    // moves played says opening game.
    result = ((END_OPENING_GAME_IN_PLY - moves_played) * piece_values_opening_game +
	     moves_played * piece_values_mid_game) / END_OPENING_GAME_IN_PLY;
  } else if (moves_played < END_MID_GAME_IN_PLY) {
    // moves played says mid game.
    result = ((END_MID_GAME_IN_PLY - moves_played) * piece_values_mid_game +
	     (moves_played - END_OPENING_GAME_IN_PLY) * piece_values_end_game) /
      (END_MID_GAME_IN_PLY - END_OPENING_GAME_IN_PLY);
  } else {
    // pure end game
    result = piece_values_end_game;
  }

  if (material > END_OPENING_GAME_IN_MATERIAL) {
    // material says opening game.
    result += ((material - END_OPENING_GAME_IN_MATERIAL) * piece_values_opening_game +
	     (SUM_MATERIAL - material) * piece_values_mid_game) /
      (SUM_MATERIAL - END_OPENING_GAME_IN_MATERIAL);
  } else if (moves_played > END_MID_GAME_IN_MATERIAL) {
    // material says mid game.
    result += ((material - END_MID_GAME_IN_MATERIAL) * piece_values_mid_game +
	      (END_OPENING_GAME_IN_MATERIAL - material) * piece_values_end_game) /
      (END_OPENING_GAME_IN_MATERIAL - END_MID_GAME_IN_MATERIAL);
  } else {
    // pure end game
    result += piece_values_end_game;
  }

  // All material has been counted twice
  return result >>= 1;
  */

  //return piece_values[0];

  int result = 0;
  for (int phase=0; phase<3; phase++)
    result += GAME_PHASE_MATERIAL[material][phase] * piece_values[phase];
  if (moves_played & ~0x7F) {// <=>  moves_played >= 128
    // num ply says end game.
    result += piece_values[2] << 8;
  } else {
    for (int phase=0; phase<3; phase++)
      result += GAME_PHASE_PLY[moves_played][phase] * piece_values[phase];
  }

  return result >> 9;
}


//------------------------------------------

void Eval_2::insert_piece(Position pos, Piece piece) {
  if (board[pos]) {
    Piece piece = board[pos];
    for (int phase=0; phase<3; phase++)
      piece_values[phase] -= TPIECE_VALUES[phase][piece][pos];
    material -= MATERIAL[piece];
  }
  for (int phase=0; phase<3; phase++)
    piece_values[phase] += TPIECE_VALUES[phase][piece][pos];
  material += MATERIAL[piece];
  Board3plus::insert_piece(pos, piece);
}

void Eval_2::remove_piece(Position pos) {
  Piece piece = board[pos];
  for (int phase=0; phase<3; phase++)
    piece_values[phase] -= TPIECE_VALUES[phase][piece][pos];
  material -= MATERIAL[piece];
  Board3plus::remove_piece(pos);
}


void Eval_2::move_piece(Position from, Position to) {
  if (board[to]) {
    Piece piece = board[to];
    for (int phase=0; phase<3; phase++)
      piece_values[phase] -= TPIECE_VALUES[phase][piece][to];
    material -= MATERIAL[piece];
  }
  Piece piece = board[from];
  for (int phase=0; phase<3; phase++) {
    piece_values[phase] -= TPIECE_VALUES[phase][piece][from];
    piece_values[phase] += TPIECE_VALUES[phase][piece][to];
  }
  Board3plus::move_piece(from, to);
}
