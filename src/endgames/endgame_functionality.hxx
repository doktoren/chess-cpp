#ifndef _ENDGAME_FUNCTIONALITY_
#define _ENDGAME_FUNCTIONALITY_

#include "../typedefs.hxx"
#include "../board.hxx"
#include "../compression/endgame_table_bdd.hxx"
#include "../board_2.hxx"

#include "endgame_values.hxx"

struct EFState {
  EFState() {
    table_loaded[0] = table_loaded[1] = 0;
    bdd_loaded[0] = bdd_loaded[1] = 0;
  }
  EFState(int8_t **table, BDD **bdd) {
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

  int8_t operator[](Board2 &b) { return index_function(b); }
  void destroy();
  void destroy(int stm);
  void destroy_table();
  void destroy_table(int stm);
  void destroy_bdd();
  void destroy_bdd(int stm);

  // Getter functions
  inline int get_num_pieces() const { return num_pieces; }
  inline Piece get_piece(int index) const { return pieces[index]; }
  inline bool is_symmetric_endgame() const { return symmetric_endgame; }
  inline uint get_table_size() const { return table_size; }
  inline string get_names() const { return name; }
  inline bool is_pawnless_endgame() const { return pawnless_endgame; }

  // If load_table needs to construct eg. KPK, then it will first load/construct
  // the tables KK, KNK, KBK, KRK, KQK
  bool load_table(bool restrict_to_stm = false,
      bool build_if_nescessary = false,
      int restricted_stm = WHITE);// Only used if restrict_to_stm == true
  bool load_bdd(bool restrict_to_stm = false,
      bool build_from_tables_if_nescessary = false,
      bool build_tables_if_nescessary = true,
      int restricted_stm = WHITE);// Only used if restrict_to_stm == true

  int8_t index_function(const Board2 &board);

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
  int8_t direct_bdd_index(int player, BDD_Index index) {
    assert(player==0  ||  player==1);
    if (bdd[player]) return (*(bdd[player]))[index];
    return ENDGAME_TABLE_UNKNOWN;
  }

  // Needed by endgame construction algorithms
  map<int, EFState> load_dependency(int stm);
  void release_dependency(const map<int, EFState> &old_states);

private:

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
public:
  // Needed by endgame construction algorithms
  void (*decompress_table_index)(int index, vector<PiecePos>& piece_list);
private:

  BDD_Index (*preprocess_bdd_index)(const vector<PiecePos>& piece_list);
  int (*table_index_to_bdd_index)(int index);

  int8_t *table[2];
  uint32_t table_size;
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

#endif
