#ifndef _HISTORY_MOVES_
#define _HISTORY_MOVES_

// NUM do not need to be a power of 2
#define NUM 6

#include <iostream>
using namespace std;

class HistoryMoves {
public:
  HistoryMoves() { clear(); }
  void clear() {
    for (int i=0; i<4096; i++)
      values[i] = 0;
    for (int i=0; i<=NUM; i++)
      best_n[i] = i;
  }

  void print(ostream &os) {
    os << "Best " << NUM << " history moves:\n";
    for (int i=0; i<NUM; i++)
      os << "   " << i+1 << ":\t" << (*this)[i].toString() << '\n';
  }

  void inc_value(Move move, uint value) {
    int index = move.to_index();
    //big_output << move.toString() << " incr. from " << values[index] << " with " << value << '\n';
    values[index] += value;

    int i;
    if (values[index] >= values[best_n[NUM-1]]) {
      // try to find index in best_n
      i = 0;
      while (i<NUM  &&  best_n[i] != index) i++;
      best_n[i] = index;
    } else {
      best_n[i = NUM] = index;
    }

    while (i  &&  values[best_n[i-1]] < values[best_n[i]]) {
      swap(best_n[i-1], best_n[i]);
      --i;
    }

    // At some point an overflow might occur.
    // Therefore downscale all values if the largest one is above some threshold.
    if (values[best_n[0]] > 0xF0000000) {
      cerr << "Downscaling history moves values by 0.5\n";
      for (int i=0; i<4096; i++)
	values[i] >>= 1;
    }
  }

  // good_move_found called by beta cutoff
  void good_move_found(Move move, int ply) {
    ply += ply>>1;
    inc_value(move, 1 << ply);
  }

  // best_move_found called by best move in a given position
  // (where no beta cutoff occur)
  void best_move_found(Move move, int ply) {
    ply += ply>>1;
    inc_value(move, 2 << ply);
  }

  // return index-best move
  Move operator[](int index) {
    assert(index < NUM);
    return Move(best_n[index] >> 6, best_n[index] & 0x3F);
  }

  uint get_move_score(Move move) {
    return values[best_n[move.to_index()]];
  }

  int size() { return NUM; }

private:
  uint values[4096];
  int best_n[NUM+1];
};

#undef NUM

#endif
