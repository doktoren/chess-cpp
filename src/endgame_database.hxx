#ifndef _ENDGAME_DATABASE_
#define _ENDGAME_DATABASE_

using namespace std;

#include <vector>
#include <assert.h>

#include "board_3.hxx"
#include "endgame_table_bdd.hxx"
#include "settings.hxx"

#include "endgame_values.hxx"



struct EndgameSettings : public SettingListener {
  EndgameSettings(Settings *_settings) : SettingListener(_settings, "Endgame_") {
    set_settings(_settings);
  }

  void set_settings(Settings *_settings) {
    settings = _settings;

    bdd_mate_depth_round_up_to_multiples_of_n =
      get_int("bdd_mate_depth_round_up_to_multiples_of_n");
    do_preprocessing = get_bool("do_preprocessing");
    clustering_method = get_int("clustering_method");
    cluster_functions_preferred = get_bool("cluster_functions_preferred");
    calc_sifting = get_bool("calc_sifting");
    mark_unreachable_as_dont_cares = get_bool("mark_unreachable_as_dont_cares");
    construction_show_progress = get_bool("construction_show_progress");
    do_preprocessing_after_sifting = get_bool("do_preprocessing_after_sifting");

    construction_method = get_int("construction_method");
    unreachability_depth_test = get_int("unreachability_depth_test");

    directory = get_string("directory");

    output_bdd_tables = get_bool("output_bdd_tables");
    output_preprocessed_bdd_tables = get_bool("output_preprocessed_bdd_tables");

    verify_bdd_with_table = get_bool("verify_bdd_with_table");

    reduce_information = get_bool("reduce_information");

    // Square permutations:
    square_enum_white_pawn   = get_int("square_enum_white_pawn");
    square_enum_white_knight = get_int("square_enum_white_knight");
    square_enum_white_bishop = get_int("square_enum_white_bishop");
    square_enum_white_rook   = get_int("square_enum_white_rook");
    square_enum_white_queen  = get_int("square_enum_white_queen");
    square_enum_white_king   = get_int("square_enum_white_king");

    square_enum_black_pawn   = get_int("square_enum_black_pawn");
    square_enum_black_knight = get_int("square_enum_black_knight");
    square_enum_black_bishop = get_int("square_enum_black_bishop");
    square_enum_black_rook   = get_int("square_enum_black_rook");
    square_enum_black_queen  = get_int("square_enum_black_queen");
    square_enum_black_king   = get_int("square_enum_black_king");
  }

  int *bdd_mate_depth_round_up_to_multiples_of_n;
  bool *do_preprocessing;
  bool *calc_sifting;
  bool *mark_unreachable_as_dont_cares;
  bool *construction_show_progress;
  bool *do_preprocessing_after_sifting;

  int *clustering_method;
  bool *cluster_functions_preferred;

  int *construction_method;
  int *unreachability_depth_test;

  // directory is the path to where the endgame files are
  // eg. "/users/doktoren/public_html/master_thesis/endgames/"
  char *directory;

  bool *output_bdd_tables;
  bool *output_preprocessed_bdd_tables;

  bool *verify_bdd_with_table;

  bool *reduce_information;

  // Square permutations:
  int *square_enum_white_pawn;
  int *square_enum_white_knight;
  int *square_enum_white_bishop;
  int *square_enum_white_rook;
  int *square_enum_white_queen;
  int *square_enum_white_king;

  int *square_enum_black_pawn;
  int *square_enum_black_knight;
  int *square_enum_black_bishop;
  int *square_enum_black_rook;
  int *square_enum_black_queen;
  int *square_enum_black_king;
};

extern EndgameSettings *endgame_settings;

struct EFState {
  EFState() {
    table_loaded[0] = table_loaded[1] = 0;
    bdd_loaded[0] = bdd_loaded[1] = 0;
  }
  EFState(char **table, BDD **bdd) {
    table_loaded[0] = table[0];
    table_loaded[1] = table[1];
    bdd_loaded[0] = bdd[0];
    bdd_loaded[1] = bdd[1];
  }

  bool bdd_loaded[2];
  bool table_loaded[2];
};

struct EndgameFunctionality {
public:
  EndgameFunctionality() :
    compress_table_index(0), decompress_table_index(0),
    preprocess_bdd_index(0), table_index_to_bdd_index(0),
    table_size(0), name(), num_pieces(0),
    symmetric_endgame(false), pawnless_endgame(false)
    //, en_passant_may_occur(false), castling_may_occur(false)
  {
    table[0] = table[1] = 0;
    bdd[0] = bdd[1] = 0;
    for (int i=0; i<MAX_MEN; i++) pieces[i] = 0;
  }

  EndgameFunctionality(int (*compress_table_index)(vector<PiecePos> &),
		       void (*decompress_table_index)(int, vector<PiecePos>&),
		       BDD_Index (*preprocess_bdd_index)(const vector<PiecePos>&),
		       int (*table_index_to_bdd_index)(int),
		       uint table_size, string name);

  ~EndgameFunctionality() { destroy(); }

  char operator[](Board2 &b) { return index_function(b); }
  void destroy();
  void destroy(int stm);
  void destroy_table();
  void destroy_table(int stm);
  void destroy_bdd();
  void destroy_bdd(int stm);

  // If load_table needs to construct eg. KPK, then it will first load/construct
  // the tables KK, KNK, KBK, KRK, KQK
  bool load_table(bool restrict_to_stm = false,
		  bool build_if_nescessary = false,
		  int restricted_stm = WHITE);// Only used if restrict_to_stm == true
  bool load_bdd(bool restrict_to_stm = false,
		bool build_from_tables_if_nescessary = false,
		bool build_tables_if_nescessary = true,
		int restricted_stm = WHITE);// Only used if restrict_to_stm == true

  char index_function(const Board2 &board);
  
  // inspect(cerr, {64, e2, f7, f6}) prints map
  void inspect(ostream &os, vector<Position> positions);

  // Example: The endgame KRPKB matches the strings KRPKR, KXPKY, K2K1 and K*K*
  bool name_match(string name_pattern);

  void print(ostream &os);
  void print_bdd(ostream &os, bool print_bdds = false);
  // print_table_verifier: The table must be loaded already
  void print_table_verifier(ostream &os);
  // print_bdd_verifier: The bdd AND the table must be loaded already
  void print_bdd_verifier(ostream &os);

  // Returns: 2:legal, 1:unreachable, 0:illegal
  int legal_position(int player, int table_index);
  void find_unreachable_positions(BitList &unreachable, int player,
				  int test_depth, int print);

  pair<uint, int> get_table_index_and_stm(const Board2 &board);
  triple<uint, uint, int> get_table_and_bdd_index_and_stm(const Board2 &board);
  pair<int, int> getModifiedOBDDIndexAndClusterValue(const Board2 &board);
  bool construct_from_table_index(Board2 &board, uint index, int pl);

  void run_length_encode(int stm, int method = 2, bool map_dont_cares = true);


  bool reduce_information();

  // Status
  bool bdd_loaded() { return bdd[0] && (symmetric_endgame || bdd[1]); }
  bool bdd_loaded(int stm) {
    assert(stm==0 || stm==1);
    return (symmetric_endgame && bdd[0])  ||  bdd[stm];
  }

  bool table_loaded() { return table[0] && (symmetric_endgame || table[1]); }
  bool table_loaded(int stm) {
    assert(stm==0 || stm==1);
    return (symmetric_endgame && table[0])  ||  table[stm];
  }

  bool table_or_bdd_loaded() { return bdd_loaded() || table_loaded(); }
  bool table_or_bdd_loaded(int stm) { return bdd_loaded(stm) || table_loaded(stm); }

  string get_name() { return name; }
  int calc_log_bdd_size() { return 6*num_pieces - (pawnless_endgame ? 2 : 1); }

  int num_bdd_clusters(int stm) {
    assert(stm==0 || stm==1);
    if (!bdd[stm]) return 0;
    return bdd[stm]->get_num_sub_bdds();
  }

  int calc_hash_value() {
    int hv = 0;
    for (int i=0; i<num_pieces; i++)
      hv += ENDGAME_HASHING_CONSTANTS[pieces[i]];
    return hv;
  }

  // returns false if bdd/table not loaded
  // Warning: takes time linear in size of table (if true returned)
  bool bdd_contains_only_win_draw_loss_info(int stm);
  bool table_contains_only_win_draw_loss_info(int stm);

  bool is_symmetric() { return symmetric_endgame; }

  void compare_bdd_with_table(int player);

  // Debug
  char direct_bdd_index(int player, BDD_Index index) {
    assert(player==0  ||  player==1);
    if (bdd[player]) return (*(bdd[player]))[index];
    return ENDGAME_TABLE_UNKNOWN;
  }
private:
  map<int, EFState> load_dependency(int stm);
  void release_dependency(const map<int, EFState> &old_states);

  // Used when constructing a table.
  void restore_state(EFState state) {
    for (int stm=0; stm<2; stm++) {
      if (state.table_loaded[stm]) {
	load_table(true, true, stm);
      } else {
	destroy_table(stm);
      }
      if (state.bdd_loaded[stm]) {
	load_bdd(true, true, true, stm);
      } else {
	destroy_bdd(stm);
      }
    }
  }
  EFState get_state() { return EFState(table, bdd); }

  void init_bdd(BDD &bdd, int player, bool delete_table);

  int (*compress_table_index)(vector<PiecePos> &piece_list);
  void (*decompress_table_index)(int index, vector<PiecePos>& piece_list);

  BDD_Index (*preprocess_bdd_index)(const vector<PiecePos>& piece_list);
  int (*table_index_to_bdd_index)(int index);

  char *table[2];
  uint table_size;
  BDD *bdd[2];

  string name;
  int num_pieces;
  Piece pieces[MAX_MEN];

  // A symmetric endgame is an endgame where black and white has the
  // exact same material (eg. KQKQ).
  bool symmetric_endgame;
  // pawnless_endgame influences on the number of bits used in the OBDD
  bool pawnless_endgame;
};


class Endgames {
public:
  Endgames() : initialized(false) {}
  ~Endgames() {}

  void destroy() { destroy_tables(); destroy_bdd(); }
  void destroy_tables();
  void destroy_bdd();

  void build_tables(string name_pattern = "K*K*");
  void build_bdds(string name_pattern = "K*K*");

  void load_tables(string name_pattern = "K*K*",
		   bool restrict_to_stm = false,
		   bool build_if_nescessary = false,
		   int restricted_stm = WHITE);
  void load_bdds(string name_pattern = "K*K*",
		 bool restrict_to_stm = false,
		 bool build_from_tables_if_nescessary = false,
		 bool build_tables_if_nescessary = true,
		 int restricted_stm = WHITE);

  // inspect(cerr, "K##_Qe2_Kf7_Rf6") prints map
  void inspect(ostream &os, string s);

  EndgameFunctionality &operator[](string endgame_pieces) {
    // endgames may only be indexed on already defined entries -
    // otherwise address of entries might change.
    assert(supported(endgame_pieces));
    return endgames[endgame_pieces];
  }
  bool supported(string endgame_pieces) {
    return endgames.count(endgame_pieces);
  }

  EndgameFunctionality &operator[](int hash_value) {
    //if (!supported(hash_value)) cerr << "hash_value = " << hash_value << "\n";
    assert(supported(hash_value));
    return *(hash_list[hash_value]);
  }
  bool supported(int hash_value) {
    return hash_list[hash_value];
  }

  EndgameFunctionality &operator[](const Board2 &board) {
    assert(supported(board));
    return *(hash_list[board.get_endgame_hashing()]);
  }
  bool supported(const Board2 &board) {
    if (board.get_num_pieces() <= MAX_MEN) {
      return hash_list[board.get_endgame_hashing()];
    } else return false;
  }

  bool get_table_and_bdd_index_and_stm(const Board2 &board, triple<uint, uint, int> &indexes);
  bool construct_from_table_index(Board2 &board, string endgame_name, uint index, int player);

  string get_endgame_name(const Board2 &board);

  void print(ostream &os, string name_pattern = "K*K*");
  void print_bdds(ostream &os, string name_pattern = "K*K*", bool print_bdds = false);

  void init();

  string get_directory() { return string(endgame_settings->directory); }

  vector<int> get_name_matches(string name_pattern);
private:
  string dir;

  map<string, EndgameFunctionality> endgames;
  EndgameFunctionality *hash_list[DB_ARRAY_LENGTH];
  bool initialized;
};

// Do not create other instances of Endgames than this one.
// The code depend on endgames, a bit ugly :-(
// Remember to call endgames.init() before using it.
// Because of problems with creation order this should be done as
// one of the first things in main()
extern Endgames endgames;

extern bool ENDGAME_TABLES_LOADED;


bool clr_endgame_database(Board *board, ostream& os, vector<string> &p);

inline bool endgame_table_index(Board2 &board, int &value) {
	if (board.get_num_pieces() > MAX_MEN)
	  return false;

    if (!endgames.supported(board.get_endgame_hashing())) {
      assert(0);
      return false;
    }

    //if (!endgames[board.get_endgame_hashing()].wtm_table_or_bdd_loaded()) return false;

    value = endgames[board.get_endgame_hashing()][board];


  if (is_special_value(value)) {
    switch (value) {
    case ENDGAME_TABLE_WIN:
      value = ORACLE_WIN;
      return true;
    case ENDGAME_TABLE_LOSS:
      value = ORACLE_LOSS;
      return true;
    case ENDGAME_TABLE_DRAW:
      value = 0;
      return true;
    case ENDGAME_TABLE_UNKNOWN:
      return false;
    case ENDGAME_TABLE_ILLEGAL:
      //assert(0);
      value = 99;
      return false;
    }
  }

  if (value <= 0) {
    value = -WIN - 2*value;
  } else {
    value = WIN - (2*value-1);
  }
  return true;
}

#undef NUM

#endif
