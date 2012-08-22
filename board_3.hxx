#ifndef _BOARD_3_
#define _BOARD_3_

#include "hash_table.hxx"
#include "hash_value.hxx"

#include "board_2.hxx"
#include "board_2_plus.hxx"

#include <map>
#include <vector>

#define LOG_MAX_GAME_LENGTH 10
#define MAX_GAME_LENGTH 1<<LOG_MAX_GAME_LENGTH

#define MAX_POSSIBLE_MOVES 100 // Need not be a power of 2

#define MAX_SEARCH_DEPTH 64 // Need not be a power of 2
#define MAX_CALC_DEPTH MAX_SEARCH_DEPTH

#define MAX_MATE_DEPTH 512
#define ORACLE_WIN (WIN/2)
#define ORACLE_LOSS (-ORACLE_WIN)
#define GUARANTEED_WIN (WIN-MAX_MATE_DEPTH)
#define GUARANTEED_LOSS (-GUARANTEED_WIN)

struct RepetitionInfo {
  RepetitionInfo() : num_repetitions(0) {}
  RepetitionInfo(int num_repetitions) : num_repetitions(num_repetitions) {}

  bool is_valid() const { return num_repetitions; }
  void clear() { num_repetitions = 0; }

  int num_repetitions;
};
ostream& operator<<(ostream& os, const RepetitionInfo& rep);

// If true, then something is printed for every execute/undo move
extern bool exec_undo_activated;

#define Extends Board2
#define ClassName Board3
#define nameofclass "Board3"
#include "board_3_class_definition.hxx"
#undef nameofclass
#undef ClassName
#undef Extends
#define Extends Board2plus
#define ClassName Board3plus
#define nameofclass "Board3plus"
#define PLUS
#include "board_3_class_definition.hxx"
#undef PLUS
#undef nameofclass
#undef ClassName
#undef Extends

/*
template class Board3<Board2>;
typedef Board3<Board2> Board3a;
template class Board3<Board2plus>;
typedef Board3<Board2plus> Board3b;
*/

#endif
