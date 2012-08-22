#include "cpu_evaluation_1.hxx"

Eval_1::Eval_1() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Eval_1 constructor called.\n";
}

Eval_1::~Eval_1() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Eval_1 destructor called.\n";
  //this->SearchRoutine::~SearchRoutine(); Will be called automatically
}

void Eval_1::reset_all() {
  value = 0;
  Board3plus::reset_all();
}

int Eval_1::evaluate(int alpha, int beta) {
  if (!(++num_evaluations & 0xFFFF)) {
    cerr << num_evaluations << " positions evaluated\n";
  }

  if (player) return -value;
  else return value;
}

bool Eval_1::clr_evaluation(Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Eval_1 &b = *dynamic_cast<Eval_1 *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Evaluation 1, help:\n"
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


void Eval_1::print_eval_stat(ostream& os) {
  os << "num_evaluations = " << num_evaluations << "\n";
}

//###############  PROTECTED  ##################

void Eval_1::init_evaluation() {
  num_evaluations = 0;
}

void Eval_1::insert_piece(Position pos, Piece piece) {
  if (board[pos])
    value -= PIECE_VALUES[board[pos]];
  value += PIECE_VALUES[piece];
  Board3plus::insert_piece(pos, piece);
}

void Eval_1::remove_piece(Position pos) {
  value -= PIECE_VALUES[board[pos]];
  Board3plus::remove_piece(pos);
}


void Eval_1::move_piece(Position from, Position to) {
  if (board[to])
    value -= PIECE_VALUES[board[to]];
  Board3plus::move_piece(from, to);
}

