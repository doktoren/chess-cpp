#ifndef _CPU_SEARCH_3_
#define _CPU_SEARCH_3_

// cpu search 3 is an alpha-beta search using iterative deepening
// and a transposition table.

#include "cpu_communication_module.hxx"
#include "engine.hxx"
#include "hash_table.hxx"
#include "hash_value.hxx"
#include "transposition_table_content.hxx"
#include "cpu_search.hxx"

#include "killer_moves.hxx"
#include "history_moves.hxx"

// Settings
struct Search3_Settings : public SettingListener {
  Search3_Settings(Settings *_settings) : SettingListener(_settings, "Search3_") {
    set_settings(_settings);
  }

  void set_settings(Settings *_settings) {
    settings = _settings;

    trace_search = get_bool("trace_search");

    use_fixed_depth = get_bool("use_fixed_depth");
    fixed_depth = get_int("fixed_depth");
    //cerr << "Fixed depth = " << *fixed_depth << "\n";

    use_tt = get_bool("use_tt");
    use_null_move = get_bool("use_null_move");
    use_futpru = get_bool("use_futpru");
    use_extfutpru = get_bool("use_extfutpru");
    use_tt_scan = get_bool("use_tt_scan");
    use_PVS = get_bool("use_PVS");
    show_line_of_play = get_bool("show_line_of_play");
    prioritise_good_see_moves = get_bool("prioritise_good_see_moves");
    use_killer_moves = get_bool("use_killer_moves");
    use_history_moves = get_bool("use_history_moves");
    do_selective_extensions = get_bool("do_selective_extensions");
    do_quiescence_search = get_bool("do_quiescence_search");
    do_iid = get_bool("do_iid");
    do_aspiration_search = get_bool("do_aspiration_search");

    restrict_tt_to_same_ply = get_bool("restrict_tt_to_same_ply");
    use_tt_on_leaf_nodes = get_bool("use_tt_on_leaf_nodes");
    use_deterministic_move_ordering = get_bool("use_deterministic_move_ordering");
    use_opening_library = get_bool("use_opening_library");
    opening_library_max_moves_played = get_int("opening_library_max_moves_played");
    print_statistics_after_calculate_move = get_bool("print_statistics_after_calculate_move");
    vulnerable_piece_penalty = get_int("vulnerable_piece_penalty");
    show_depth_0_move_order = get_bool("show_depth_0_move_order");
    show_value_of_each_move_from_root = get_bool("show_value_of_each_move_from_root");

    fast_evaluate_precision = get_int("fast_evaluate_precision");
    null_move_const = get_int("null_move_const");
    null_move_depth_reduction = get_int("null_move_depth_reduction");
    futil_margin = get_int("futil_margin");
    fextutil_margin = get_int("fextutil_margin");
    iid_depth_reduction = get_int("iid_depth_reduction");
    aspiration_window = get_int("aspiration_window");
    time_per_move_in_ms = get_int("time_per_move_in_ms");
    debug_log_tt_size = get_int("debug_log_tt_size");
    log_tt_size = get_int("log_tt_size");
  }

  void print(ostream &os) {
    os << "Search 3 settings:\n";
    pint(os, "aspiration_window");
    pbool(os, "do_aspiration_search");
    pint(os, "fixed_depth");
    pbool(os, "show_line_of_play");
    pbool(os, "use_fixed_depth");
    pbool(os, "use_opening_library");
    os << "The next setting is only used when XBOARD is not defined\n";
    pint(os, "time_per_move_in_ms");

#ifdef NDEBUG
    os << "Remark: Program compiled with NDEBUG => default values used for settings below!\n";
#endif
    pbool(os, "do_iid");
    pbool(os, "do_quiescence_search");
    pbool(os, "do_selective_extensions");
    pint(os, "fast_evaluate_precision");
    pint(os, "fextutil_margin");
    pint(os, "futil_margin");
    pint(os, "iid_depth_reduction");
    pint(os, "opening_library_max_moves_played");
    pint(os, "null_move_const");
    pint(os, "null_move_depth_reduction");
    pbool(os, "print_statistics_after_calculate_move");
    pbool(os, "prioritise_good_see_moves");
    pbool(os, "restrict_tt_to_same_ply");
    pbool(os, "show_depth_0_move_order");
    pbool(os, "show_value_of_each_move_from_root");
    pbool(os, "trace_search");
    pbool(os, "use_PVS");
    pbool(os, "use_deterministic_move_ordering");
    pbool(os, "use_extfutpru");
    pbool(os, "use_futpru");
    pbool(os, "use_history_moves");
    pbool(os, "use_killer_moves");
    pbool(os, "use_null_move");
    pbool(os, "use_tt");
    pbool(os, "use_tt_on_leaf_nodes");
    pbool(os, "use_tt_scan");
    pint(os, "debug_log_tt_size");
    pint(os, "log_tt_size");
    pint(os, "vulnerable_piece_penalty");
  }

  bool *trace_search;

  int *fixed_depth;
  bool *use_fixed_depth;

  bool *use_tt;
  bool *use_null_move;
  bool *use_futpru;
  bool *use_extfutpru;
  bool *use_tt_scan;
  bool *use_PVS;
  bool *show_line_of_play;

  bool *prioritise_good_see_moves;
  bool *use_killer_moves;
  bool *use_history_moves;
  bool *do_selective_extensions;
  bool *do_quiescence_search;
  bool *do_iid;
  bool *do_aspiration_search;

  bool *restrict_tt_to_same_ply;
  bool *use_tt_on_leaf_nodes;
  bool *use_deterministic_move_ordering;
  bool *use_opening_library;
  bool *print_statistics_after_calculate_move;
  bool *show_depth_0_move_order;
  bool *show_value_of_each_move_from_root;

  int *opening_library_max_moves_played;
  int *fast_evaluate_precision;
  int *null_move_const;
  int *null_move_depth_reduction;
  int *futil_margin;
  int *fextutil_margin;
  int *iid_depth_reduction;
  int *aspiration_window;
  int *vulnerable_piece_penalty;

  int *time_per_move_in_ms;
  int *debug_log_tt_size;
  int *log_tt_size;
};



struct AlphaBetaValue {
  AlphaBetaValue() :
    value(0), move(), game_theoretical_value(false), all_moves_searched(false) {}
  AlphaBetaValue(int value) :
    value(value), move(), game_theoretical_value(false), all_moves_searched(false) {}
  AlphaBetaValue(int value, Move move) :
    value(value), move(move), game_theoretical_value(false), all_moves_searched(false) {}
  AlphaBetaValue(int value, bool game_theoretical_value) :
    value(value), move(),
    game_theoretical_value(game_theoretical_value), all_moves_searched(false) {}
  AlphaBetaValue(int value, Move move, bool game_theoretical_value) :
    value(value), move(move),
    game_theoretical_value(game_theoretical_value), all_moves_searched(false) {}

  void replace(const AlphaBetaValue& v) {
    assert(!game_theoretical_value);
    assert(!all_moves_searched);
    value = v.value;
    move = v.move;
  }

  int& operator*() { return value; }

  int value;
  Move move;

  bool game_theoretical_value;
  bool all_moves_searched;
};
template <class S>
S& operator<<(S& os, const AlphaBetaValue& r) {
  os << "ABV(" << r.move.toString() << ',';
  if (r.game_theoretical_value) {
    if (r.value) {
      if (GUARANTEED_LOSS >= r.value) os << "-M" << toString((WIN+r.value)>>1);
      else if (GUARANTEED_WIN <= r.value) os << "M" <<  toString((WIN-r.value+1)>>1);
      else assert(0);
    } os << "draw";
  } else os << r.value;
  os << ',' << (r.game_theoretical_value ? 'T' : 'F')
     << ',' << (r.all_moves_searched ? 'T' : 'F') << ')';
  return os;
}

//see calc_extended_ply in cpu_search_3.cxx
// SINGLE_MOVE not used
#define SINGLE_MOVE 1
#define PUSHING_PASSED_PAWN (1<<2)
#define RECAPTURE (1<<5)
#define CHECKING_MOVE (1<<8)
struct AlphaBetaInfo {
  AlphaBetaInfo() : qwerty(0),
		    recapture_location(ILLEGAL_POS), search_extensions_counter(0) {}

  bool qwerty;
  Position recapture_location;// or ILLEGAL_POS

  // 0-1 single moves
  // 2-4 pushing passed pawns
  // 5-7 recaptures
  // 8-10 checking moves
  ushort search_extensions_counter;
};
template <class S>
S& operator<<(S& os, const AlphaBetaInfo& info) {
  os << "ABI(" << (int)info.qwerty
     << ',' << POS_NAME[info.recapture_location]
     << ",(" << (info.search_extensions_counter & 3)
     << ',' << ((info.search_extensions_counter>>2) & 7)
     << ',' << ((info.search_extensions_counter>>5) & 7)
     << ',' << ((info.search_extensions_counter>>8) & 7) << "))";
  return os;
}


struct MoveValue {
  MoveValue() : move(), value(-INF) {}
  MoveValue(Move move) : move(move), value(-INF) {}
  MoveValue(Move move, int value) : move(move), value(value) {}

  //Move& operator*() { return move; }

  Move move;
  int value;
};

class Search_3 : public virtual Engine {
public:
  Search_3();
  ~Search_3();

  Move calculate_move(ostream& os);

  CommandLineReceiver* get_clr_search() { return clr_search; }

  static bool clr_search(void *ignored, Board *board, ostream& os, vector<string> &p);
  void print_quiescence(ostream &os);

  Search3_Settings settings;

  // After outdate_tt_entries is called with default age_by,
  // no existing entry will block for storing new positions
  // (but the existing entries are still there).
  void outdate_tt_entries(int age_by = MAX_PLY);

  // Totally clears the transposition table, but is time consuming.
  void clear_transposition_table() {
    transposition_table.clear();
    current_time_stamp = 1;
  }

protected:
  AlphaBetaValue alpha_beta(int ply, int depth, int alpha, int beta,
			    AlphaBetaInfo info);

  KillerMoves killer_moves[MAX_SEARCH_DEPTH];
  HistoryMoves history_moves[2];

#ifdef NDEBUG
  HashTable_Simple<HashValue, EntryWithMove> transposition_table;
#else
  HashTable_Simple<HashValue, DEBUG_EntryWithMove> transposition_table;
#endif

  ushort current_time_stamp;

  SearchStuff search;

  //  MyVector<MoveValue> move_lists[MAX_SEARCH_DEPTH];
  //  MoveSet move_sets[MAX_SEARCH_DEPTH];

  int num_alpha_beta_calls;

private:
  // inlines
  int calc_extended_ply(int ply, AlphaBetaInfo &info);
  AlphaBetaInfo info_after_move(AlphaBetaInfo info, Move move);
  Move index_transposition_table_for_move(int extended_ply);
  void beta_cutoff(int extended_ply, int depth, AlphaBetaValue value);
  void update_tt_at_leaf(int depth, int value, int eval_type);
  AlphaBetaValue alpha_beta_move(int ply, int depth, int alpha, int beta,
				 AlphaBetaInfo info, Move move, bool& first_move_tried);

  bool use_fixed_depth;
};


#endif
