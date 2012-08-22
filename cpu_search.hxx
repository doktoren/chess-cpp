#ifndef _SPU_SEARCH_
#define _SPU_SEARCH_

#include "engine.hxx"

#include <iostream>
#include <time.h>

#define LEAF_REACHED__EVALUATION 0
#define LEAF_REACHED__GAME_OVER 1
#define LEAF_REACHED__ENDGAME_TABLE 2
#define LEAF_REACHED__HASH_TABLE 3

class SearchStuff {
public:
  SearchStuff() { principal_line[0].line[0] = Move(); }

  // Line of play stuff
  void print_line_of_play(ostream &os);
  void alpha_improved(Move move, int depth);
  void leaf_reached(int depth, int leaf_type);
  Move best_move();

  // Clock stuff
  void start_clock(int allowed_time_in_ms);
  int percent_time_used();
  bool out_of_time();
private:
  // Line of play stuff
  struct PrincipalLine {
    Move line[MAX_SEARCH_DEPTH];
    int leaf_kind;
  };
  PrincipalLine principal_line[MAX_SEARCH_DEPTH];
  // Clock stuff
  double allowed_time_in_seconds;
  time_t start_clock_time;
};

#endif
