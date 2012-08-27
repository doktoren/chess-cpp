#ifndef _BOARD_3_
#define _BOARD_3_

#include "util/hash_table.hxx"
#include "hash_value.hxx"

#include "board_2.hxx"
#include "board_2_plus.hxx"
#include "engine/engine_constants.hxx"

#include <map>
#include <vector>

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
