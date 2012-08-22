#include "cpu_search_2.hxx"

#include "opening_library.hxx"
#include "hash_table.hxx"
#include "endgame_database.hxx"

Search_2::Search_2() : table(22) {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Search_2 constructor called.\n";
}

Search_2::~Search_2() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Search_2 destructor called.\n";
  //this->Engine::~Engine(); Will be called automatically
}

Move Search_2::calculate_move(ostream& os) {
  search.start_clock(3000);
  init_evaluation();
  num_tt_used = 0;

  table.clear_stat();

  Move best_move = moves();
  next_move(best_move);
  if (!next_move(best_move)) {
    // Only one legal move. Execute it immediately
    return best_move;
  }
  best_move = moves();
  next_move(best_move);// Just to assure a legal move is returned

  max_moves_played_reached = 0;

  search_aborted = false;

  int value = 42;
  // iterated deepening
  int idepth = 1;
  do {
    value = alpha_beta(idepth, 0, -INF, INF);
    if (!search_aborted) {
      best_move = search.best_move();
      cerr << "Printing line of play...\n";
      search.print_line_of_play(os);
      os << "Depth " << idepth << " best move = " << best_move.toString()
	 << ", value = " << game_theoretical_value_to_string(value) << "\n";
    } else {
      os << "Search was interrupted.\n";
    }
    ++idepth;

    // continue search while A && B && C
    // A: not enough time used
    // B: value is not guaranteed win or loss
    // C: The last search was as deep as required
  } while (search.percent_time_used() < 40.0  &&
	   GUARANTEED_LOSS<value  &&  value<GUARANTEED_WIN  &&
	   max_moves_played_reached < moves_played+idepth);
  
  cerr << "max_moves_played_reached = " << max_moves_played_reached << "\n"
       << "moves_played = " << moves_played << ", idepth = " << idepth << "\n";

  comm->move_now = false;
  return best_move;
}

bool Search_2::clr_search(void *ignored, Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Search_2 &b = *dynamic_cast<Search_2 *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Search 2, help:\n"
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

void Search_2::print_search_stat(ostream& os) {
  table.print_stat(os);
  if (search_aborted) {
    os << "Search was interrupted -> line of play trashed\n";
  } else {
    search.print_line_of_play(os);
  }
}

//###############  PROTECTED  ##################


int Search_2::alpha_beta(int depth, int ply, int alpha, int beta) {
  if (moves_played > max_moves_played_reached)
    max_moves_played_reached = moves_played;

  int status = calc_game_status();
  if (status) {
    search.leaf_reached(ply, LEAF_REACHED__GAME_OVER);
    switch (status) {
    case GAME_DRAWN: return 0;
    case WHITE_WON:  return player ? -(WIN-ply) : (WIN-ply);
    case BLACK_WON:  return player ? (WIN-ply) : -(WIN-ply);
    }
  }

  if (ply) {
    // Never index endgame table in first ply - otherwise move undefined
    int value;
    if (endgame_table_index(*this, value)) {
      search.leaf_reached(ply, LEAF_REACHED__ENDGAME_TABLE);
      if (value == ENDGAME_TABLE_DRAW) return 0;
      if (value <= 0) return -WIN-(2*value)+ply;
      return WIN-(2*value-1)-ply;
    }
  }

  if (depth <= 0) {
    search.leaf_reached(ply, LEAF_REACHED__EVALUATION);
    int val = evaluate();
    if (val==0) val = 0;
    update_table(Info(val, 0, EXACT_EVAL), ply);
    return val;
  }

  if (ply == 1  &&  (comm->move_now  ||  search.percent_time_used()>200.0)) {
    search_aborted = true;
    return beta;
  }

  int current = -INF;
  bool exact_evaluation = false;
  int opening_library_bonus = 0;


  Move move = moves();
  while (next_move(move)) {
    execute_move(move);

    int val;
    bool found_in_tt = false;

    Info info(table[hash_value]);
    
    if (info.is_valid()  &&  info.depth >= depth-1) {
      //cerr << "Found! " << info << '\n';
      
      // Force progress in winning situation
      if (info.value > GUARANTEED_WIN) info.value -= ply;
      if (info.value < GUARANTEED_LOSS) info.value += ply;
      
      // Now it depends on eval_type
      switch (info.eval_type) {
      case UPPER_BOUND:
	if (-info.value >= beta) {
	  found_in_tt = true;
	  val = -info.value;
	} else {
	  val = -alpha_beta(depth-1, ply+1, -beta, -alpha);
	}
	break;
      case EXACT_EVAL:
	found_in_tt = true;
	val = -info.value;
	break;
      case LOWER_BOUND: // ie. upper bound here
	if (-info.value <= alpha) {
	  found_in_tt = true;
	  val = -info.value;
	} else {
	  val = -alpha_beta(depth-1, ply+1, -beta, -alpha);
	}
	break;
      default:
	abort();
      }
      num_tt_used += found_in_tt;
    } else {
      val = -alpha_beta(depth-1, ply+1, -beta, -alpha);
    }

    if (ply == 0) {
      opening_library_bonus = (int)(16.0*sqrt((float)(opening_library->num_occurences(hash_value))));
    }

    undo_move();

    if (val > current) {
      if (found_in_tt) search.leaf_reached(depth, LEAF_REACHED__HASH_TABLE);
      else search.alpha_improved(move, ply);

      current = val;
      if (val >= beta) {
	update_table(Info(current, depth, LOWER_BOUND), ply);
	return current + opening_library_bonus;
      }
      if (val > alpha) {
	exact_evaluation = true;
	alpha = val;
	// cout << "best move = " << moveToSAN(*this, move) << '\n';
	//if (found_in_tt) search.transposition_table_used(depth);
	//else search.alpha_improved(move, ply);
      }
    }
  }

  if (exact_evaluation) update_table(Info(current, depth, EXACT_EVAL), ply);
  else update_table(Info(current, depth, UPPER_BOUND), ply);

  return current + opening_library_bonus;
}
