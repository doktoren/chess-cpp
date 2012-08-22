#include "cpu_search_1.hxx"

Search_1::Search_1() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Search_1 constructor called.\n";
}

Search_1::~Search_1() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Search_1 destructor called.\n";
  //this->Engine::~Engine(); Will be called automatically
}

Move Search_1::calculate_move(ostream& os) {
  init_evaluation();
  root_depth = moves_played;

  int value = alpha_beta(4, -INF, INF);
  os << "Best move found. value = " << value << "\n";

  comm->move_now = false;
  return search.best_move();
}

bool Search_1::clr_search(void *ignored, Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Search_1 &b = *dynamic_cast<Search_1 *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Search 1, help:\n"
       << "    print board  or  pb  or  dir\n"
       << "      - print board\n"
       << "    print search stat  or  pss\n";
  } else if (dot_demand(p, 1, "dir")  ||
	     dot_demand(p, 2, "print", "board")) {
    b.print_board(os);
  } else if (dot_demand(p, 3, "print", "search", "statistic")) {
    b.print_search_stat(cerr);
  } else return false;
  return true;
}

void Search_1::print_search_stat(ostream& os) {
  search.print_line_of_play(os);
}

//#############   Protected   ###########################3


int Search_1::alpha_beta(int depth, int alpha, int beta) {
  int status = calc_game_status();
  if (status) {
    search.leaf_reached(moves_played-root_depth, LEAF_REACHED__GAME_OVER);
    switch (status) {
    case GAME_DRAWN: return 0;
    case WHITE_WON:  return player ? -WIN : WIN;
    case BLACK_WON:  return player ? WIN : -WIN;
    }
  }

  if (depth == 0) {
    search.leaf_reached(moves_played-root_depth, LEAF_REACHED__EVALUATION);
    return evaluate();
  }

  Move move = moves();

  while (next_move(move)) {
    execute_move(move);
    int val = -alpha_beta(depth-1, -beta, -alpha);
    undo_move();

    if (val >= beta) return val;
    if (val > alpha) {
      alpha = val;
      search.alpha_improved(move, moves_played-root_depth);
    }
  }

  return alpha;
}
