#ifndef _ENDGAME_DATABASE_
#define _ENDGAME_DATABASE_

using namespace std;

#include <vector>
#include <assert.h>


#include "endgame_values.hxx"
#include "endgame_settings.hxx"
#include "endgame_functionality.hxx"
#include "../engine/engine_constants.hxx"

extern EndgameSettings *endgame_settings;

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
    return *(hash_list[board.get_endgame_material().individual.endgame_hashing]);
  }

  bool supported(const Board2 &board) {
    if (board.get_num_pieces() <= MAX_MEN) {
      return hash_list[board.get_endgame_material().individual.endgame_hashing];
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

  if (!endgames.supported(board.get_endgame_material().individual.endgame_hashing)) {
    assert(0);
    return false;
  }

  value = endgames[board.get_endgame_material().individual.endgame_hashing][board];

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

#endif
