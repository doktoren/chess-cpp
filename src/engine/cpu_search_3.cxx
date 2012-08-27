#include "cpu_search_3.hxx"

#include "../move_set.hxx"
#include "../endgames/endgame_database.hxx"

#include "../opening_library.hxx"

#ifndef NDEBUG

#define EntryWithMove DEBUG_EntryWithMove
#define TRUE(dah) (*(settings.dah))
#define FALSE(dah) (*(settings.dah))
#define DEFAULT(name, def_value) (*(settings.name))

#else

#define TRUE(dah) true
#define FALSE(dah) false
#define DEFAULT(name, def_value) (def_value)

#endif

#define Setting(x) (*(settings.x))


#define trace if (DEFAULT(trace_search, false)) big_output << string(4*depth, ' ')
#define khm if (false) big_output

// how precise must fast_evaluate be
#define FAST_EVALUATE_PRECISION DEFAULT(fast_evaluate_precision, WPAWN_VALUE)

#define NULL_MOVE_DEPTH_REDUCTION DEFAULT(null_move_depth_reduction, 2)
#define NULL_MOVE_CONST DEFAULT(null_move_const, 100)

#define FUTIL_MARGIN DEFAULT(futil_margin, 2*WPAWN_VALUE)
#define FEXTUTIL_MARGIN DEFAULT(fextutil_margin, WBISHOP_VALUE)

#define IID_DEPTH_REDUCTION DEFAULT(iid_depth_reduction, 1)

#define MTD_MARGIN (PAWN_VALUE/16)

#define VULNERABLE_PIECE_PENALTY DEFAULT(vulnerable_piece_penalty, 50)

#define trying_move(name, move) if (FALSE(show_depth_0_move_order)  &&  depth == 0)\
    big_output << "Depth 1: trying move " << moveToSAN(move) << "(" << name << ")\n";


ostream& operator<<(ostream &os, const pair<int,int> &p) {
  return os << '(' << p.first << ", " << p.second << ')';
}

#ifndef NDEBUG
class Statistics {
public:
  Statistics() { clear(); }

  void clear() {
    num_alpha_beta = 0;
    num_stable_position = 0;
    num_easy_qs = 0;
    num_qs = 0;
    min_extended_ply_reached = 0;
    max_ply_extension = 0;

    num_futpru = num_extfutpru = num_pure_tt = num_null = 
        num_iid = num_tt = avg_avail_moves = pair<int,int>(0,0);

    for (int i=0; i<2*MAX_SEARCH_DEPTH; i++) {
      ply_count[i] = 0;
      game_over_count[i] = 0;
    }
  }

  void print(ostream &os) {
    os << "Statistics:\n"
        << "Calls of alpha beta = " << num_alpha_beta << "\n"
        << "Stable positions = " << num_stable_position << "\n"
        << "Easy decidable qs = " << num_easy_qs << "\n"
        << "Hard qs = " << num_qs << "\n"
        << "Futility pruning (ply = 1) = " << num_futpru << "\n"
        << "Ext. Futility pruning (ply = 2) = " << num_extfutpru << "\n"
        << "Internal iterative deepening (ply >= 1+iid_d_r), (tt,iid) = " << num_iid << "\n"
        << "Transposition table (succ, unsucc) = " << num_tt << "\n"
        << "Min extended ply reached = " << min_extended_ply_reached << "\n"
        << "Pure tt, ply >= 2 (+, all) = " << num_pure_tt << "\n"
        << "Null moves (ply >= 1+null_m_d_r) (succ, unsucc) = " << num_null << "\n";
    if (avg_avail_moves.second)
      os << "Avg. avail moves = " << (double)avg_avail_moves.first/avg_avail_moves.second << "\n";

    int f,t;
    f=INF, t=-INF;
    for (int i=-MAX_SEARCH_DEPTH; i<MAX_SEARCH_DEPTH; i++)
      if (num_ply(i)) {
        if (i<f) f=i;
        if (i>t) t=i;
      }

    os << "Number of searches with specified ply:\n";
    for (int i=f; i<=t; i++)
      os << "Ply " << i << ":   \t" << num_ply(i) << "\n";

    f=INF, t=-INF;
    for (int i=-MAX_SEARCH_DEPTH; i<MAX_SEARCH_DEPTH; i++)
      if (num_game_over(i)) {
        if (i<f) f=i;
        if (i>t) t=i;
      }

    os << "Number of game overs with specified ply:\n";
    for (int i=f; i<=t; i++)
      os << "Ply " << i << ":   \t" << num_game_over(i) << "\n";
  }

  int num_alpha_beta;
  int num_stable_position;
  int num_easy_qs;
  int num_qs;
  int min_extended_ply_reached;
  int max_ply_extension;

  pair<int, int> num_futpru, num_extfutpru, num_pure_tt, num_null, num_iid, num_tt, avg_avail_moves;

  int &num_ply(int ply) {
    return ply_count[ply+MAX_SEARCH_DEPTH];
  }
  int &num_game_over(int ply) {
    return game_over_count[ply+MAX_SEARCH_DEPTH];
  }

private:
  int ply_count[2*MAX_SEARCH_DEPTH];
  int game_over_count[2*MAX_SEARCH_DEPTH];

  pair<int,int> km_num, mh_num;
};
Statistics stat;

#define stat(x) stat.x
#define inc(x) ++x
#define dec(x) --x

#else

#define stat(x)
#define inc(x)
#define dec(x)

#endif



uchar SEARCH_EXTENSION_TABLE[2048];// allow search extension table
//allow <=> ASE_TABLE[search_extensions_performed] & new_search_extension

void init_SEARCH_EXTENSION_TABLE() {
  static bool initialized = false;
  if (initialized) return;

  for (int i=0; i<2048; i++) {
    SEARCH_EXTENSION_TABLE[i] = 0;

    int num_single_moves = i & 3;
    SEARCH_EXTENSION_TABLE[i] += num_single_moves;

    int num_passed_pawn_moves = (i >> 2) & 7;
    const uchar NPPM[8] = {0, 1, 2, 2, 3, 3, 4, 4};
    SEARCH_EXTENSION_TABLE[i] += NPPM[num_passed_pawn_moves];

    int num_recaptures = (i >> 5) & 7;
    const uchar NR[8] = {0, 1, 1, 2, 2, 3, 3, 4};
    SEARCH_EXTENSION_TABLE[i] += NR[num_recaptures];

    int num_checks = i >> 8;
    const uchar NC[8] = {0, 1, 1, 1, 1, 1, 1, 1};
    SEARCH_EXTENSION_TABLE[i] += NC[num_checks];
  }

  initialized = true;
}

inline int Search_3::calc_extended_ply(int ply, AlphaBetaInfo &info) {
  if (!TRUE(do_selective_extensions)) return ply;
  return ply + SEARCH_EXTENSION_TABLE[info.search_extensions_counter & 2047];
}

void Search_3::outdate_tt_entries(int age_by) {
  assert(age_by < 512);

  if ((current_time_stamp += age_by) >= 0xFE00) {
    cerr << "Time stamps in transposition table needs correction.\n";
    for (int i=0; i<transposition_table.size; i++) {
      if (transposition_table.table[i].content.is_valid()) {
        uint16_t &time_stamp = transposition_table.table[i].content.time_stamp;
        if (time_stamp < 0x8000) time_stamp = 0;
        else time_stamp -= 0x8000;
      }
    }
  }
}

Search_3::Search_3() :
      settings(&(comm->settings)),
      transposition_table(DEBUG ? *(settings.debug_log_tt_size) : *(settings.log_tt_size))
{
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Search_3 constructor called.\n";

  current_time_stamp = 1;

  init_SEARCH_EXTENSION_TABLE();
}

Search_3::~Search_3() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Search_3 destructor called.\n";
  //this->Engine::~Engine(); Will be called automatically
}


int num_succesfull_endgame_probes;
Move Search_3::calculate_move(ostream& os) {
  os << "Search3::calculate_move called!\n";

  int fixed_depth;
#ifdef XBOARD
  // Search3 will use up to time_precision more ms than allowed
  const int time_precision = 100;

  use_fixed_depth = comm->use_fixed_depth;
  fixed_depth = comm->fixed_depth;
  if (!use_fixed_depth) {
    int num_ms = comm->num_ms_for_next_move(moves_played);
    num_ms -= time_precision;
    os << "Cpu will be thinking for " << num_ms << " ms\n";
    search.start_clock(num_ms);
  }
#else
  use_fixed_depth = Setting(use_fixed_depth);
  fixed_depth = Setting(fixed_depth);
  if (!use_fixed_depth) {
    int num_ms = Setting(time_per_move_in_ms);
    //num_ms -= time_precision;
    os << "Cpu will be thinking for " << num_ms << " ms\n";
    search.start_clock(num_ms);
  }
#endif


  num_succesfull_endgame_probes = 0;

  // Last search is presumably outdated by 2 ply
  outdate_tt_entries(2);

  comm->move_now = false;
  init_evaluation();

  Move best_move;

  // If only one legal move, execute it immediately.
  best_move = moves();
  my_assert(next_move(best_move));
  if (!next_move(best_move)) {
    os << "Only one move possible - returning immediately.\n";
    best_move = moves();
    next_move(best_move);
    return best_move;
  }

  // Check opening book
  if (Setting(use_opening_library)  &&  moves_played < Setting(opening_library_max_moves_played)) {
    Move move = moves();
    int total = 0;
    while (next_move(move)) {
      execute_move(move);
      int tmp = opening_library->num_occurences(hash_value);
      total += tmp*tmp;
      undo_move();
    }

    if (total) {
      os << "Picking move from opening library (sum sqr = " << total << ")\n";
      total = rand()%total;
      int selected = 0;
      move = moves();
      while (next_move(move)) {
        execute_move(move);
        selected = opening_library->num_occurences(hash_value);
        undo_move();
        total -= selected*selected;
        if (total < 0) break;
      }

      os << "Choosing " << moveToSAN(move) << " with count = " << selected << "\n";
      return move;
    }
  }

  // Reset best_move
  best_move = Move();

  // Transposition table and move repetition is a bad combination!
  if (position_repetition_count() > 1) {
    os << "Position with move repetition => clearing transposition table.\n";
    clear_transposition_table();
  }

  history_moves[WHITE].clear();
  history_moves[BLACK].clear();

  stat(clear());
  if (use_fixed_depth)
    os << "Fixed depth = " << fixed_depth << "\n";

  // iterated deepening
  int iterative_ply = 0;
  AlphaBetaValue value;

  num_alpha_beta_calls = 0;

  do {
    ++iterative_ply;

    if (Setting(do_aspiration_search)) {

      int alpha = *value - Setting(aspiration_window);
      int beta = *value + Setting(aspiration_window);

      value = alpha_beta(iterative_ply, 0, alpha, beta, AlphaBetaInfo());
      if (value.move.is_defined()) {
        best_move = value.move;
      } else {
        os << "best_move not defined (0)\n";
      }

      if (!(comm->move_now)  &&
          (*value <= alpha  ||  beta <= *value)) {
        os << "aspiration search failed. (" << alpha << ',' << beta << "), value = "
            << *value << ", move = " << moveToSAN(value.move) << "\n";
        value = alpha_beta(iterative_ply, 0, -INF, INF, AlphaBetaInfo());
        if (value.move.is_defined()) {
          best_move = value.move;
        } else {
          os << "best_move not defined (1)\n";
        }
      }

    } else {

      value = alpha_beta(iterative_ply, 0, -INF, INF, AlphaBetaInfo());
      if (value.move.is_defined()) {
        best_move = value.move;
      } else {
        os << "best_move not defined (2)\n";
      }
    }


    if (!(comm->move_now)) {
      os << "Depth " << iterative_ply << " best move = " << best_move.toString()
	     << ", value = " << game_theoretical_value_to_string(*value) << "\n\n";
    } else {
      os << "Depth " << iterative_ply << " aborted. ";
      if (value.move.is_defined()) {
        os << "Move = " << value.move.toString()
	       << ", value = " << game_theoretical_value_to_string(*value) << "\n";
      } else {
        os << "No move searched completely. Using move from previous iteration.\n";
      }
    }

#ifndef NDEBUG
    if (TRUE(use_history_moves)) {
      big_output << "White: "; history_moves[WHITE].print(big_output);
      big_output << "Black: "; history_moves[BLACK].print(big_output);
    }
    if (TRUE(use_killer_moves)) {
      for (int i=0; i<iterative_ply; i++) {
        big_output << "Depth " << i << ":\n";
        killer_moves[i].print(big_output);
      }
    }
#endif

    // Continue while:
    // search has not been aborted and has not timed out  &&
    // ((don't use fixed depth)  ||  (fixed depth not reached yet))  &&
    // last value was only a approximate
  } while (!(comm->move_now)  &&
      (!use_fixed_depth  ||  iterative_ply!=fixed_depth)  &&
      !value.game_theoretical_value);

  if (value.game_theoretical_value)
    os << "Search stopped: Game theoretical value found!\n";
  os << "moves_played = " << moves_played << ", iterative_ply = " << iterative_ply << "\n";

  if (Setting(show_line_of_play)) {
    if (best_move.is_defined()) {
      os << "Line of play (Only approximately, eg all search extensions ignored):\n"
          << "\t" << moveToSAN(best_move);
      execute_move(best_move);
      int num_undo = 1;

      int alpha = *value;
      int beta = alpha+1;

      // If not fixed depth, then allow 0.5 second cpu-time to calculate the line of play!
      search.start_clock(500);
      comm->move_now = false;

      bool old_value = *(settings.show_value_of_each_move_from_root);
      *(settings.show_value_of_each_move_from_root) = false;

      for (int ply = iterative_ply-2; ply>=1; ply--) {
        //Move move = alpha_beta(ply, 0, alpha, beta, AlphaBetaInfo()).move;
        Move move = alpha_beta(ply, 0, -INF, INF, AlphaBetaInfo()).move;

        if (!move.is_defined()) break;
        os << " " << moveToSAN(move);
        execute_move(move);
        ++num_undo;

        int tmp = -alpha;
        alpha = -beta;
        beta = tmp;
      }

      *(settings.show_value_of_each_move_from_root) = old_value;

      os << "\nPosition after line of play:\n";
      print_board(os);

      for (int i=0; i<num_undo; i++) undo_move();
    }
  }

  comm->move_now = false;

  if (!best_move.is_defined()) {
    os << "best_move is undefined after search! (?), doing an extra 1-ply search.\n";
    // This might happen eg. if an endgame table were applied
    // Do a 1-ply search.
    int current = -INF;
    Move move = moves();
    while (next_move(move)) {
      execute_move(move);
      AlphaBetaValue value = alpha_beta(0, 0, -INF, INF, AlphaBetaInfo());
      *value = -*value;
      if (*value > current) {
        current = *value;
        best_move = move;
      }
      undo_move();
    }
    assert(best_move.is_defined());
  }

#ifndef NDEBUG
  if (Setting(print_statistics_after_calculate_move)) stat.print(os);
#endif

  if (num_succesfull_endgame_probes)
    os << "Endgame tables probed succesfully " << num_succesfull_endgame_probes << " times.\n";

  return best_move;
}

//##############################################


bool Search_3::clr_search(Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Search_3 &b = *dynamic_cast<Search_3 *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Search 3, help:\n"
        << "    print board  or  pb  or  dir\n"
        << "      - print board\n"
        << "    print quiescence search  or  pqs\n"
        << "      - print first moves in a quiescence search\n"
        << "    print Stat  or  pS\n"
#ifdef NDEBUG
        << "      - DISABLED! (compiled with -DNDEBUG)\n"
#else
        << "      - print statistics from last search\n"
#endif
        << "    print transposition table info  or  ptti\n"
        << "    verify hash value  or  vhv\n"
        << "    clear transposition table  or  ctt\n"
        << "    print settings  or  ps\n"
        << "    set name value\n"
        << "      - example \"set use_tt false\"\n"
        << "    save settings  or  ss\n";

  } else if (dot_demand(p, 3, "print", "quiescence", "search")) {
    b.print_quiescence(os);

#ifndef NDEBUG
  } else if (dot_demand(p, 2, "print", "Stat")) {
    stat.print(os);
#endif

  } else if (dot_demand(p, 3, "clear", "transposition", "table")) {
    os << "Clearing transposition table...";
    b.clear_transposition_table();
    os << "done\n";

  } else if (dot_demand(p, 4, "print", "transposition", "table", "info")) {
    b.transposition_table.print(os);

  } else if (dot_demand(p, 3, "verify", "hash", "value")) {
    b.verify_hash_value(os);

  } else if (dot_demand(p, 2, "print", "settings")) {
    b.settings.print(os);

  } else if (dot_demand(p, 3, "set", (uintptr_t)0, (uintptr_t)0)) {
    b.settings.define(parse_result[0], parse_result[1]);

  } else if (dot_demand(p, 2, "save", "settings")) {
    b.comm->settings.save();

  } else return false;
  return true;
}


//###############  PROTECTED  ##################

int my_sort(const void *a, const void *b) {
  return ((pair<Move, int> *)b)->second - ((pair<Move, int> *)a)->second;
}

void Search_3::print_quiescence(ostream &os) {
  os << "print_quiescence(...): Todo!\n";
  /*
  see.update_variables(player);
  os << "best_attack = " << see.best_attack << '\n'
     << "worst_defence = " << see.worst_defence << '\n'
     << "second_worst_defence = " << see.second_worst_defence << '\n'
     << "best_see_result = " << see.best_see_result << '\n';

  if (see.best_attack  ||  see.second_worst_defence) {
    os << "Do quiescence search...\n";

    MoveSet2<int> good_moves;
    for (int i=0; i<see.size(); i++) {
      Position pos = see.target_position(i);
      os << "Pos(" << POS_NAME[pos] << ")\n";

      if (PIECE_COLOR[board[pos]] == player) {
	os << "- contains own piece\n";

	int pnum = piece_number[pos];
	int count = piece_moves.count(pnum);
	for (int i=0; i<count; i++) {
	  Move move = move_list[piece_moves.index(pnum, i)];
	  os << "  Move(" << move.toString() << ')';
	  if (legal_move(move)) {
	    // Todo: pawn promotion moves currently do not get special attention
	    int move_result = see.move_result(move);
	    os << " - is legal, result = " << move_result << "\n";

	    if (move_result > 0)
	      good_moves.try_insert(move, move_result);
	  } else os << '\n';
	}

      } else {
	os << "- contains opponent piece\n";

	square_move_list.init_iterator(pos);
	while (square_move_list.iterate(player)) {
	  Move move = move_list[square_move_list.deref_iterator()];
	  os << "  Move(" << move.toString() << ')';
	  if (legal_move(move)) {
	    // Todo: pawn promotion moves currently do not get special attention
	    int move_result = see.move_result(move);
	    os << " - is legal, result = " << move_result << "\n";

	    if (move_result > 0)
	      good_moves.try_insert(move, move_result);
	  } else os << '\n';
	}
      }
    }


    // good_moves indeholder nu de gode træk i den aktuelle stilling.
    // Sorter disse efter hvor gode de er
    good_moves.move_list.sort(my_sort);
    for (int i=0; i<good_moves.move_list.size(); i++) {
      Move move = good_moves.move_list[i].first;
      os << "Next quiescence move searched: " << move.toString() << '\n';
    }
    os << "Do quiescence search...done\n";
  }
   */
}

AlphaBetaInfo Search_3::info_after_move(AlphaBetaInfo info, Move move) {
  AlphaBetaInfo new_info = info;

  if (move.to == info.recapture_location) new_info.search_extensions_counter += RECAPTURE;
  if (passed_pawn(move.from)) new_info.search_extensions_counter += PUSHING_PASSED_PAWN;

  if (board[move.to]) new_info.recapture_location = move.to;
  else new_info.recapture_location = ILLEGAL_POS;

  return new_info;
}


#define compare_with_alpha_beta(value) \
    game_theoretical_value &= value.game_theoretical_value;\
    if (*value > *current) {\
      trace << "*value = " << *value << " > " << *current << " = *current\n";\
      current.replace(value);\
      if (*value > alpha) {\
        trace << "*value = " << *value << " > " << alpha << " = alpha\n";\
        alpha = *value;\
        evaluation_type = EXACT_EVAL;\
        if (*value >= beta) {\
          trace << "*value = " << *value << " >= " << beta << " = beta\n";\
          beta_cutoff(extended_ply, depth, value);\
          return value;\
        }\
      }\
    }


void Search_3::beta_cutoff(int extended_ply, int depth, AlphaBetaValue value) {
  assert(extended_ply > 0);

  khm << "Good move: " << value.move.toString() << ", extended_ply = " << extended_ply
      << ", sd = " << (depth+extended_ply) << ", " << PLAYER_NAME[player] << '\n';

  killer_moves[depth].good_move_found(value.move);
  history_moves[player].good_move_found(value.move, extended_ply);

  // store in transposition table
  if (TRUE(use_tt)  &&  !(comm->move_now)) {
    EntryWithMove &entry = transposition_table[hash_value];
    int importance = ((int)current_time_stamp + (int)extended_ply) - ((int)entry.time_stamp + (int)entry.ply);

    if (importance >= 0) {
      // New content more important
      int store_value = *value;
      if (store_value >= GUARANTEED_WIN) {
        store_value += depth;
        assert((store_value & 1) == 0);
      } else if (store_value <= GUARANTEED_LOSS) {
        store_value -= depth;
        assert((store_value & 1) == 1);
      }

#ifdef NDEBUG
      transposition_table.update(hash_value,
          EntryWithMove(store_value, extended_ply, LOWER_BOUND,
              current_time_stamp, value.move));
#else
      transposition_table.update(hash_value,
          EntryWithMove(store_value, extended_ply, LOWER_BOUND,
              current_time_stamp, value.move, HashBoard(*this)));
#endif
    }
  }
}



void Search_3::update_tt_at_leaf(int depth, int value, int eval_type) {

  if (!TRUE(use_tt)  ||  !TRUE(use_tt_on_leaf_nodes)) return;

  // store in transposition table
  // when comm->move_now, search is abrupted and values can no longer be trusted
  if (TRUE(use_tt)  &&  !(comm->move_now)) {
    EntryWithMove &entry = transposition_table[hash_value];
    int importance = (int)current_time_stamp - ((int)entry.time_stamp + (int)entry.ply);

    if (importance >= 0) {
      // New content more important
      if (value >= GUARANTEED_WIN) {
        value += depth;
        assert((value & 1) == 0);
      } else if (value <= GUARANTEED_LOSS) {
        value -= depth;
        assert((value & 1) == 1);
      }

#ifdef NDEBUG
      transposition_table.update(hash_value,
          EntryWithMove(value, 0, eval_type,
              current_time_stamp, Move()));
#else
      transposition_table.update(hash_value,
          EntryWithMove(value, 0, eval_type,
              current_time_stamp, Move(), HashBoard(*this)));
#endif
    }
  }
}


Move Search_3::index_transposition_table_for_move(int extended_ply) {

  if (!TRUE(use_tt)) return Move();

  EntryWithMove *entry;

  if (transposition_table.find(hash_value, &entry)  &&
      entry->move.is_defined()  &&
      (entry->eval_type == EXACT_VALUE  ||
          entry->ply >= extended_ply-IID_DEPTH_REDUCTION)) {

#ifndef NDEBUG
    //assert(entry->hboard == HashBoard(*this));
    if (entry->hboard != HashBoard(*this)) {
      cerr << "Transposition table lookup error!\n"
          << "Current position:\n";
      print_board(cerr);
      cerr << "Wrongly matched with:\n";
      entry->hboard.print(cerr);
      cerr << "tt move = " << entry->move.toString() << "\n";

      verify_hash_value(cerr);
    }
#endif

#ifndef NDEBUG
    Move tmp_move = entry->move;
    if (!find_legal_move(tmp_move)) {
      cerr << "Move from transposition table is not legal?!?!\n"
          << "transposition table move = " << tmp_move.toString() << "\n";
      print_board(cerr);
      print_king_threats(cerr);
      print_bit_boards(cerr);
      print_threat_pos(cerr);

      assert(0);
    }
#endif


    // best_candidate found in transposition table
    //inc(stat(num_iid.first));

    return entry->move;
  }

  return Move();
}



AlphaBetaValue Search_3::alpha_beta_move(int ply, int depth, int alpha, int beta,
    AlphaBetaInfo info, Move move, bool &first_move_tried) {

  trace << "alpha_beta_move(" << ply << ',' << depth << ',' << alpha << ',' << beta << ','
      << info << ',' << moveToSAN(move) << ',' << first_move_tried << ")\n";

  if (comm->move_now) {
    // This can be placed before ++depth

    // The search has been interrupted.
    // The algorithm should return the best move COMPLETELY SEARCHED so far.
    // Here something should be returned so that in the root loop,
    // every remaining move will evaluate infinitely bad.
    if (depth&1) return AlphaBetaValue(INF);
    return AlphaBetaValue(-INF);
  }

  // IMPORTANT!
  --ply;
  ++depth;

  AlphaBetaInfo new_info = info_after_move(info, move);
  int extended_ply = calc_extended_ply(ply, new_info);

  AlphaBetaValue value;

  if (TRUE(use_tt)) {
    HashValue hv = hash_value_after_move(move);
    EntryWithMove *entry;
    if (transposition_table.find(hv, &entry)  &&
        (entry->eval_type == EXACT_VALUE  ||
            entry->ply == extended_ply  ||
            (!TRUE(restrict_tt_to_same_ply)  &&  entry->ply > extended_ply))) {

#ifndef NDEBUG
      execute_move(move);
      //assert(entry->hboard == HashBoard(*this));
      if (entry->hboard != HashBoard(*this)) {
        cerr << "Transposition table lookup error!\n"
            << "Current position (move " << move.toString() << ":\n";
        print_board(cerr);
        cerr << "Wrongly matched with:\n";
        entry->hboard.print(cerr);
        assert(0);
      }
      undo_move();
#endif

      int eval_type = swap_eval_type(entry->eval_type);

      if (eval_type != UPPER_BOUND) {
        inc(stat(num_tt.first));
        value = AlphaBetaValue(-entry->value, move, eval_type == EXACT_VALUE);

        if (*value >= GUARANTEED_WIN) {
          assert((*value & 1) == 1);
          *value -= depth;
        } else if (*value <= GUARANTEED_LOSS) {
          assert((*value & 1) == 0);
          *value += depth;
        }

        if (eval_type != LOWER_BOUND  ||  *value >= beta) {
          if (FALSE(show_value_of_each_move_from_root)  &&  depth==1) {
            cerr << "Move " << moveToSAN(move) << " gives value " << *value
                << ", (a,b)=(" << alpha << "," << beta << ")\n";
          }
          value.move = move;
          return value;
        }
      }

    } else {
      inc(stat(num_tt.second));
    }
  }

  execute_move(move);
  // Do not use PVS if the remaining search depth is 0
  // (VERY little gained at the expence of potentially doubling the cost)
  if (TRUE(use_PVS)  &&  extended_ply>1  &&  first_move_tried) {
    value = alpha_beta(ply, depth, -(alpha+1), -alpha, new_info);
    *value = -*value;
    if (*value > alpha) {// research
      value = alpha_beta(ply, depth, -beta, -alpha, new_info);
      *value = -*value;
    }
  } else {
    value = alpha_beta(ply, depth, -beta, -alpha, new_info);
    *value = -*value;
    first_move_tried = true;
  }
  undo_move();

  if (FALSE(show_value_of_each_move_from_root)  &&  depth==1) {
    cerr << "Move " << moveToSAN(move) << " gives value " << *value
        << ", (a,b)=(" << alpha << "," << beta << ")\n";
  }

  value.move = move;
  return value;
}



AlphaBetaValue Search_3::alpha_beta(int ply, int depth, int alpha, int beta,
    AlphaBetaInfo info) {
  // MAX_SEARCH_DEPTH set too low?
  if (depth == MAX_SEARCH_DEPTH) {
    cerr << "WARNING! Max search depth (" << depth << ") reached.\n";
    print_board(cerr);
    savePGN("max_search_depth_exceeded.pgn");
#ifdef NDEBUG
    cerr << "Returning 0\n";
    return AlphaBetaValue(0);
#else
    assert(0);
#endif
  }

  inc(stat(num_alpha_beta));
  inc(stat(num_ply(ply)));

  if (!(++num_alpha_beta_calls & 0xFF)) {
    if (!use_fixed_depth  &&
        search.percent_time_used() > 100.0  &&
        !comm->move_now) {
      cerr << "Cpu has runned out of time!\n";
      comm->move_now = true;
    }
  }

  if (comm->move_now) {
    // The search has been interrupted.
    // The algorithm should return the best move COMPLETELY SEARCHED so far.
    // Here something should be returned so that in the root loop,
    // every remaining move will evaluate infinitely bad.
    if (depth&1) return AlphaBetaValue(INF);
    return AlphaBetaValue(-INF);
  }

  trace << "alpha_beta(" << ply << "," << depth << "," << alpha << "," << beta\
      << ", " << info << ")\n";

  // Look in endgame table
  {
    int value;
    if (endgame_table_index(*this, value)) {
      trace << "endgame table lookup succeeded\n";
      ++num_succesfull_endgame_probes;
      if (value==0) {

      } else if (value >= GUARANTEED_WIN) {
        value -= depth;
      } else if (value <= GUARANTEED_LOSS) {
        value += depth;
      } else {
        assert(0);
      }
      update_tt_at_leaf(depth, value, EXACT_VALUE);
      return AlphaBetaValue(value, true);
    }
  }

  // Test for game over
  {
    int status = calc_game_status();
    if (status) {
      inc(stat(num_game_over(ply)));
      trace << "game has ended: " << game_result_texts[status] << '\n';
      int value;
      switch (status) {
      case GAME_DRAWN:
        value = 0;
        break;
      case WHITE_WON:
      case BLACK_WON:
        value = -(WIN-depth);
        break;
      default:
        value = 0;//Avoid warning
        assert(0);
      }
      update_tt_at_leaf(depth, value, EXACT_VALUE);
      return AlphaBetaValue(value, true);
    }
  }

  int extended_ply = calc_extended_ply(ply, info);
  // extended ply for check is delayed (otherwise hard to predict)
  // That is: If king checked in a leaf position, this can't result in added ply
  if (num_checks) info.search_extensions_counter += CHECKING_MOVE;

#ifndef NDEBUG
  if (stat.max_ply_extension < extended_ply - ply) {
    stat.max_ply_extension = extended_ply - ply;
    /*
    cerr << "Ply increased by " << stat.max_ply_extension << ", info = " << info << ":\n";
    print_board(cerr, -5);
    cerr << '\n';
     */
  }
#endif

  //########################################################################################
  //##################################               #######################################
  if (extended_ply <= 0) {//########   LEAF NODE   #######################################
    //##################################               #######################################
    //########################################################################################
    // quiescence search

#ifndef NDEBUG
    if (extended_ply < stat(min_extended_ply_reached)) {
      stat(min_extended_ply_reached) = extended_ply;
      cerr << "Quiescence search to extended_ply = " << extended_ply
          << " performed.\n";
      //print_board(cerr, -5);
      char tmp[32];
      sprintf(tmp, "quiescence%d.pgn", -extended_ply);
      savePGN(tmp);
    }
#endif

    // Do we have a stable position?
    // That is, player can't capture with gain and opponent can't capture
    // with gain at 2 places (if only 1 place, then assume that the piece
    // can move away). Todo: It should be punished to have a weak piece.
    if (!see.size(player)  &&  see.size(player^1) <= 1) {
      inc(stat(num_stable_position));

      int value = evaluate(alpha, beta);
      trace << "see.size()==0 => stable position, value = " << value << "\n";

      update_tt_at_leaf(depth, value,
          (value <= alpha) ? UPPER_BOUND : ((value >= beta) ? LOWER_BOUND : EXACT_EVAL));
      return AlphaBetaValue(value);
    }


    if (TRUE(do_quiescence_search)) {

      // Quiescence algorithm:
      // value = -INF
      // for each good see attacking move do
      //     if the value of the attacking move is better than value, update value
      // if value == -INF and player has at least 2 weak pieces according to see then
      //     perform extra ply

      int current = evaluate(alpha, beta);

      { // Is performing best_hit not good enough compared to alpha?
        int upper_bound_value = current + see.best_see_capture_value(player);
        if (upper_bound_value <= alpha) {
          inc(stat(num_easy_qs));
          update_tt_at_leaf(depth, upper_bound_value, UPPER_BOUND);
          return AlphaBetaValue(current);
        }
      }

      bool has_2_weak_pieces = see.size(player^1) >= 2;

      { // Can we estimate the value better than beta?
        int lower_bound = current - (has_2_weak_pieces ? see.best_see_capture_value(player^1) : 0);
        if (lower_bound >= beta) {
          inc(stat(num_easy_qs));
          update_tt_at_leaf(depth, lower_bound, LOWER_BOUND);
          return AlphaBetaValue(current);
        }
      }

      // If we have to perform a full width search, then use the original
      // value of alpha
      int old_alpha = alpha;

      // If the player does not have 2 weak pieces, then we are only interestet
      // in capturing moves that improves the situation.
      if (!has_2_weak_pieces  &&  current > alpha) {
        // current < beta
        alpha = current;
      }

      bool first_move_tried = false;
      if (see.size(player)) {
        inc(stat(num_qs));
        trace << "Try improving captures\n";

        MoveSet2<int> good_capture_moves;
        for (int i=0; i<see.size(player); i++) {
          Position pos = see.target_position(player, i);

          square_move_list.init_iterator(pos);
          while (square_move_list.iterate(player)) {
            Move move = move_list[square_move_list.deref_iterator()];
            if (legal_move(move)) {
              // Todo: pawn promotion moves currently do not get special attention
              int capture_move_improvement = see.move_result(move);

              // Does this capturing move improves the situation enough compared to alpha?
              if (current + capture_move_improvement >= alpha)
                good_capture_moves.try_insert(move, capture_move_improvement);
            }
          }
        }

        if (good_capture_moves.size() != 0) {
          // good_moves now contain the usefull capture moves.
          // Sort such that best moves are placed first
          good_capture_moves.move_list.sort(my_sort);

          for (int i=0; i<good_capture_moves.move_list.size(); i++) {
            Move move = good_capture_moves.move_list[i].first;
            assert(move.is_defined());
            trace << "quiescence search - " << move.toString() << '\n';

            //AlphaBetaInfo new_info = info;
            // What should we consider a boring move?
            //if (is_irreversible_move(move)) new_info.successive_boring_moves = 0;
            //else new_info.successive_boring_moves++;

            AlphaBetaValue capture_value = alpha_beta_move(ply, depth, alpha, beta, info,
                move, first_move_tried);
            if (*capture_value > current) {
              current = *capture_value;
              if (*capture_value > alpha) {
                alpha = *capture_value;
                if (*capture_value >= beta) {
                  // todo: the value should maybe be stored with ply==-1
                  // as ... (?)
                  update_tt_at_leaf(depth, *capture_value, LOWER_BOUND);
                  return capture_value;
                }
              }
            }

            first_move_tried = true;
          }
        }
      }

      if (!has_2_weak_pieces  ||  first_move_tried  ||  info.qwerty) {
        trace << "Returning value " << current << "\n";
        return AlphaBetaValue(current);
      }

      // full width search performed
      info.qwerty = true;
      extended_ply = 1;
      alpha = old_alpha;
      goto INTERNAL_NODE;

    } else { // No quiescence search! Use static exchange evaluation only

      // Use a (primitive) combination of evaluation value and s.e.e.
      int see_value = see.best_see_capture_value(player);
      int value = evaluate(alpha-see_value,beta-see_value) + see_value;

      update_tt_at_leaf(depth, value,
          (value <= alpha) ? UPPER_BOUND : ((value >= beta) ? LOWER_BOUND : EXACT_EVAL));
      return AlphaBetaValue(value);
    }

    //########################################################################################
    //##############################                   #######################################
  } else {//####################   INTERNAL NODE   #######################################
    //##############################                   #######################################
    //########################################################################################

    INTERNAL_NODE:

    if (get_num_non_zugzwang_pieces(player) > 1) {
      //###################################################################################
      //#############             FUTILITY PRUNING and NULL MOVE            ###############
      //###################################################################################

      switch (extended_ply) {
      case 1:
        //futility pruning, good static exchance evaluation moves,
        //killer moves, history moves, remaining moves

        // Futility pruning
        // Transposition table ignored
        if (TRUE(use_futpru)  &&  num_checks == 0) {// TODO: and not endgame (zugzwang)
          trace << "Trying futility pruning at frontier node.\n";
          // Ignorer hash tabellen her, vil ikke gaa ud over performance

          int tmp = see.best_see_capture_value(player) + FUTIL_MARGIN;
          int upper_limit = evaluate(alpha-tmp, beta-tmp) + tmp;

          trace << "tmp=" << tmp << ",a-tmp=" << alpha-tmp << ",b-tmp=" << beta-tmp << "\n";

          if (upper_limit <= alpha) {
            inc(stat(num_futpru.first));
            return AlphaBetaValue(upper_limit);
          }
          inc(stat(num_futpru.second));
        }
        break;


      case 2:
        // Extended futility pruning
        // Transposition table ignored
        if (TRUE(use_extfutpru)  &&  num_checks == 0) {
          trace << "Trying extended futility pruning at pre-frontier node.\n";

          int tmp = see.best_see_capture_value(player) + FEXTUTIL_MARGIN;
          int upper_limit = evaluate(alpha-tmp, beta-tmp) + tmp;

          if (upper_limit <= alpha) {
            inc(stat(num_extfutpru.first));
            return AlphaBetaValue(upper_limit);
          }
          inc(stat(num_extfutpru.second));
        }
        break;


      default:
        assert(extended_ply >= 1+NULL_MOVE_DEPTH_REDUCTION);
        // Check if null move should be executed
        if (Setting(use_null_move)  &&  num_checks == 0) {

          // try the null move if (zugzwang situation unlikely) and
          // (fast evaluation - most valuable victim > beta)
          int tmp = NULL_MOVE_CONST - see.best_see_capture_value(player^1);
          bool try_null_move = evaluate(alpha-tmp, beta-tmp) + tmp >= beta;

          if (try_null_move  &&  make_null_move()) {

            trace << "Trying null move\n";

            int value = -alpha_beta(ply-1-NULL_MOVE_DEPTH_REDUCTION, depth+1, -beta, -alpha, info).value;
            undo_null_move();

            /*
	      big_output << "\nNull move is tried. (alpha, beta) = (" << alpha << ", "
	      << beta << ")\n";
	      big_output << "fe() = " << fast_evaluate() << ", opponent_best_hit = "
	      << see.best_see_hit(player^1) << ", const = " << NULL_MOVE_CONST <<  ", x-y+z = "
	      << fast_evaluate()-see.best_see_hit(player^1)+NULL_MOVE_CONST
	      << " > " << beta << " = beta\n";
	      print_board(big_output, -5);
	      big_output << "With depth = " << depth << " and extended_ply = " << extended_ply << "\n";
             */

            if (value >= beta) {
              /*
		big_output << "##########################################################################\n"
		<< "###############                                       ####################\n"
		<< "###############     NULL MOVE WAS SUCCESSFULL!!!!     ####################\n"
		<< "###############                                       ####################\n"
		<< "##########################################################################\n\n";
               */
              inc(stat(num_null.first));
              return AlphaBetaValue(value);
            }
            inc(stat(num_null.second));
          }
        }
      }
    }

    //###################################################################################

    bool game_theoretical_value = true;
    AlphaBetaValue current(-INF);
    int evaluation_type = UPPER_BOUND;
    bool first_move_tried = false;

    //#define all_moves move_lists[depth]
    //#define move_set move_sets[depth]
    //all_moves.clear();
    // move_set.clear();

    // Better to place out here - if resize is needed it will be performed only once.
    // Also problems with goto's that crosses initializations
    MyVector<MoveValue> all_moves(40);
    MoveSet move_set;

    //###################################################################################
    //#############              TRANSPOSITION TABLE SCAN                 ###############
    //###################################################################################

    // only search extensions on SINGLE_MOVE if tt enabled!
    if (extended_ply >= 2  &&  TRUE(use_tt)  &&  TRUE(use_tt_scan)  &&  depth > 0) {
      // Check if we can cause a beta-cutoff by only looking in the transposition table
      // This takes a bit time, hence we only do it if extended_ply >= 2

      // Why depth > 0? Consider a position where a move will allow the opponent
      // to do a check mating move. If the position after this move is in the tt
      // it will probably have MAX_PLY associated (as it is game_theoretical_value)
      // If the iterative deepening has just started on a deeper search and then the
      // time runs out then only the tt_scan will succeed - moves not found during
      // tt_scan will evaluate to -INF. But it is very likely that only the move
      // with value -M1 has high enough associated ply (MAX_PLY) to be accepted
      // in the tt_scan. The basic problem is that the tt_scan destroys the property
      // that moves are examined in expected best first order, with is essential for
      // accepting the best move found in an aborted partial search.
      // Todo! Still problems if d1-first_move,d2-last_move,d3 tt_scan and out of time


      get_move_list(all_moves);
      //if (all_moves.size() == 1) info.search_extensions_counter += SINGLE_MOVE;

      bool all_moves_searched = true;

      // Even if no beta-cutoff occur, we can still hope to increase alpha,
      // such that the searches afterwards goes faster.
      // We must therefore use current to remember the best move,
      // in the case that alpha is raised only here (otherwise current undefined).

      // We detect if we succesfully find all positons in the transposition table.
      // Also we detects if this allows for a game theoretical value.

      for (int i=0; i<all_moves.size(); i++) {
        Move &move = all_moves[i].move;

        AlphaBetaInfo new_info = info_after_move(info, move);
        int required_ply = calc_extended_ply(ply-1, new_info);

        HashValue hv = hash_value_after_move(move);
        EntryWithMove *entry;

        if (transposition_table.find(hv, &entry)  &&
            (entry->eval_type == EXACT_VALUE  ||
                entry->ply == required_ply  ||
                (!TRUE(restrict_tt_to_same_ply)  &&  entry->ply > required_ply))) {

          trace << "TT-scan: " << moveToSAN(move) << " found in tt.\n";

          assert(entry->eval_type != EMPTY_INFO);

          game_theoretical_value &= (entry->eval_type == EXACT_VALUE);

#ifndef NDEBUG
          execute_move(move);
          //assert(entry->hboard == HashBoard(*this));
          if (entry->hboard != HashBoard(*this)) {
            cerr << "Transposition table lookup error!\n"
                << "Current position (move " << move.toString() << ":\n";
            print_board(cerr);
            cerr << "Wrongly matched with:\n";
            entry->hboard.print(cerr);
            assert(0);
          }
          undo_move();
#endif

          // Adjust value. Important!!! This scan is actually looking 1 deeper
          int value = -entry->value;
          if (value >= GUARANTEED_WIN) {
            assert((value & 1) == 1);
            value -= (depth+1);
          } else if (value <= GUARANTEED_LOSS) {
            assert((value & 1) == 0);
            value += (depth+1);
          }

          if (value > *current) {
            current.value = value;
            current.move = move;
            if (value > alpha) {
              alpha = value;
              evaluation_type = EXACT_EVAL;
              // transposition table scan has small cost, try to find even better value
              // even if value >= beta
            }
          }
        } else {
          game_theoretical_value = all_moves_searched = false;
        }
      }
      // Assume succesfull
      inc(stat(num_pure_tt.first));
      inc(stat(num_pure_tt.second));

      current.game_theoretical_value = game_theoretical_value;

      // If all moves led to a position in the transposition table (very unlikely),
      // then there is no need to do the actual search.
      if (all_moves_searched) {
        trace << "Transposition table scan: All moves searched!\n";
        goto ALL_MOVES_SEARCHED;
      }

      // Check if a move caused a beta-cutoff (which are what we are hoping for)
      if (*current >= beta) {
        trace << "*current = " << *current << " >= " << beta << " = beta\n";
        beta_cutoff(extended_ply, depth, current);
        return current;
      }

      // Not succesfull after all :-(
      dec(stat(num_pure_tt.first));

      // No beta-cutoff was triggered, but still the scan might have improved the alpha-value!
      all_moves.clear();//TODO: Is generated twice - not optimal!
      game_theoretical_value = true;
    }


    //###################################################################################
    //#########        EXAMINE THE MOVES IN EXPECTED BEST FIRST ORDER       #############
    //###################################################################################

    trace << "EXAMINE THE MOVES IN EXPECTED BEST FIRST ORDER\n";

    // INDEX TRANSPOSITION TABLE TO GET EXPECTED BEST MOVE
    if (TRUE(use_tt)) {
      Move move = index_transposition_table_for_move(extended_ply);

      if (move.is_defined()) {
        trying_move("tt", move);
        AlphaBetaValue value;
        value = alpha_beta_move(ply, depth, alpha, beta, info, move, first_move_tried);
        compare_with_alpha_beta(value);
        move_set.insert(move);
        all_moves.add(MoveValue(move, value.value));
      }
    }

    // DO INTERNAL ITERATIVE DEEPENING TO GET EXPECTED BEST MOVE
    if (TRUE(do_iid)  &&  extended_ply > 2  &&  all_moves.size() == 0) {
      /* What should alpha and beta be set to in this call?
	 All we are interested in is to get the best move.
	 The MTD(f) technique seems interesting.
       */
      inc(stat(num_iid.second));

      if (FALSE(show_value_of_each_move_from_root)  &&  depth == 0) {
        cerr << "Performing iterative deepening from ply " << ply << ".\n";
      }

      Move move = alpha_beta(ply-IID_DEPTH_REDUCTION, depth, alpha, beta, info).move;

      if (FALSE(show_value_of_each_move_from_root)  &&  depth == 0) {
        cerr << "I. d. from ply " << ply << " completed with move " << moveToSAN(move) << ".\n";
        Move tt_move = index_transposition_table_for_move(extended_ply);
        cerr << "Can the move now be found in the tt? Move = " << moveToSAN(tt_move) << ".\n";
      }

      if (move.is_defined()) {
        trying_move("iid", move);
        AlphaBetaValue value;
        value = alpha_beta_move(ply, depth, alpha, beta, info, move, first_move_tried);
        compare_with_alpha_beta(value);
        move_set.insert(move);
        all_moves.add(MoveValue(move, value.value));
      }
    }

    // CHECK GOOD STATIC_EXCHANGE_EVALUATION MOVES
    if (TRUE(prioritise_good_see_moves)  &&  see.size(player)) {
      MoveSet2<int> good_capture_moves;

      for (int i=0; i<see.size(player); i++) {
        Position pos = see.target_position(player, i);

        square_move_list.init_iterator(pos);
        while (square_move_list.iterate(player)) {
          Move move = move_list[square_move_list.deref_iterator()];
          if (legal_move(move)) {
            // Todo: pawn promotion moves currently do not get special attention
            int move_result = see.move_result(move);

            if (move_result > 0)
              good_capture_moves.try_insert(move, move_result);
          }
        }
      }

      good_capture_moves.move_list.sort(my_sort);
      for (int i=0; i<good_capture_moves.move_list.size(); i++) {
        Move move = good_capture_moves.move_list[i].first;
        if (!move_set.find(move)) {
          trying_move("see", move);

          move_set.insert(move);
          assert(move.is_defined());
          trace << "good see move - " << move.toString() << '\n';

          AlphaBetaValue value = alpha_beta_move(ply, depth, alpha, beta, info, move, first_move_tried);
          all_moves.add(MoveValue(move, *value));
          compare_with_alpha_beta(value);
        }
      }
    }

    // CHECK KILLER MOVES
    if (TRUE(use_killer_moves)) {
      for (int i=0; i<killer_moves[depth].size(); i++) {
        Move move = killer_moves[depth][i];
        if (find_legal_move(move)) {
          if (!move_set.find(move)) {
            trying_move("killer", move);

            move_set.insert(move);
            assert(move.is_defined());
            trace << "killer move - " << move.toString() << '\n';

            AlphaBetaValue value = alpha_beta_move(ply, depth, alpha, beta, info, move, first_move_tried);
            all_moves.add(MoveValue(move, *value));
            compare_with_alpha_beta(value);
          }
        }
      }
    }

    // CHECK HISTORY MOVES
    if (TRUE(use_history_moves)) {
      for (int i=0; i<history_moves[player].size(); i++) {
        Move move = history_moves[player][i];
        if (find_legal_move(move)) {
          if (!move_set.find(move)) {
            trying_move("history", move);
            move_set.insert(move);
            assert(move.is_defined());
            trace << "history move - " << move.toString() << '\n';

            AlphaBetaValue value = alpha_beta_move(ply, depth, alpha, beta, info, move, first_move_tried);
            all_moves.add(MoveValue(move, *value));
            compare_with_alpha_beta(value);
          }
        }
      }
    }

    // CHECK REMAINING MOVES
    // Todo: is it a good idea to sort according to history move score?
    {
      int prev_size = all_moves.size();
      if (FALSE(use_deterministic_move_ordering)) {
        Move move = moves();
        while (next_move(move))
          if (!move_set.find(move))
            all_moves.push_back(move);
      } else {
        get_move_list(all_moves, move_set);
      }

      trace << "Remaining moves: prev_size = " << prev_size
          << ", all_moves.size() = " << all_moves.size() << '\n';

      for (int i=prev_size; i<all_moves.size(); i++) {
        Move move = all_moves[i].move;
        trying_move("remaining", move);

        assert(move.is_defined());
        trace << "remaining move - " << move.toString() << '\n';

        AlphaBetaValue value = alpha_beta_move(ply, depth, alpha, beta, info, move, first_move_tried);
        trace << "Result of alpha_beta_move = " << value << '\n';
        all_moves[i].value = *value;
        compare_with_alpha_beta(value);
      }
    }

    //###################################################################################
    //#############             EVERY MOVE HAS BEEN SEARCHED              ###############
    //###################################################################################

    ALL_MOVES_SEARCHED:
    // All moves searched without causing a beta-cutoff!
    current.all_moves_searched = true;

    // update killer_moves and move_history
    if (current.move.is_defined()) {
      khm << "Best move: " << current.move.toString() << ", extended_ply = " << extended_ply
          << ", sd = " << (depth+ply) << ", " << PLAYER_NAME[player] << '\n';
      killer_moves[depth].good_move_found(current.move);
      history_moves[player].best_move_found(current.move, extended_ply);
    }

#ifndef NDEBUG
    // Do statistics on how many legal moves there are in average
    stat.avg_avail_moves.first += all_moves.size();
    stat.avg_avail_moves.second++;
#endif

    // If there was a check mate in less than extended_ply from this position,
    // it would have been found (wrongly assuming that null move etc. is sound).
    // Any check mate found within this depth must hence be optimal (game_theoretical_value)
    {
      int best_win = WIN-(depth+extended_ply);
      if ((*current <= -best_win  ||  best_win <= *current)  &&  !(comm->move_now))
        game_theoretical_value = true;
    }

    if (game_theoretical_value) {

      if (DEFAULT(trace_search, false)) {
        big_output << "Game theoretical value "
            << game_theoretical_value_to_string((depth&1) ? -*current : *current)
            << " found in position below:\n";
        print_board(big_output);
      }

      evaluation_type = EXACT_VALUE;
      extended_ply = MAX_PLY;
      current.game_theoretical_value = true;
    }

    // store in transposition table
    if (TRUE(use_tt)  &&  !(comm->move_now)) {
      EntryWithMove &entry = transposition_table[hash_value];
      int importance = ((int)current_time_stamp + (int)extended_ply) - ((int)entry.time_stamp + (int)entry.ply);
      if (importance >= 0) {
        // New content more important
        int store_value = *current;
        if (store_value >= GUARANTEED_WIN) {
          store_value += depth;
          assert((store_value & 1) == 0);
        } else if (store_value <= GUARANTEED_LOSS) {
          store_value -= depth;
          assert((store_value & 1) == 1);
        }

#ifdef NDEBUG
        transposition_table.update(hash_value,
            EntryWithMove(store_value, extended_ply, evaluation_type,
                current_time_stamp, current.move));
        assert(0);
#else
        transposition_table.update(hash_value,
            EntryWithMove(store_value, extended_ply, evaluation_type,
                current_time_stamp, current.move, HashBoard(*this)));
#endif
      }
    }

    trace << "All moves considered. value = " << *current << "\n";

    return current;
  }
}

#undef stat
#undef inc
#undef dec

#undef khm
#undef trace
