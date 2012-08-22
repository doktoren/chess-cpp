#ifndef _KILLER_MOVES_
#define _KILLER_MOVES_

// NUM do not need to be a power of 2
#define NUM 3

class KillerMoves {
public:
  KillerMoves() { clear(); }
  void clear() {
    for (int i=0; i<NUM; i++)
      best_n[i] = Move(0,0);//from=to=0
  }

  void print(ostream &os) {
    os << "Best " << NUM << " killer moves:\n";
    for (int i=0; i<NUM; i++)
      os << "   " << i+1 << ":\t" << (*this)[i].toString() << '\n';
  }

  void good_move_found(Move move) {
    // move already at pos 0?
    if (move.from == best_n[0].from  &&  move.to == best_n[0].to)
      return;
    
    // move otherwise already in list?
    for (int i=1; i<NUM; i++) {
      if (move.from == best_n[i].from  &&  move.to == best_n[i].to) {
	swap(best_n[0], best_n[i]);
	return;
      }
    }

    // outdate current entries
    for (int i=NUM-1; i; i--)
      best_n[i] = best_n[i-1];

    best_n[0] = move;
  }

  // return index-best move
  Move operator[](int index) {
    assert(index < NUM);
    return best_n[index];
  }

  int size() { return NUM; }

private:

  // sorted so best_n[0] is better then best_n[1] etc.
  Move best_n[NUM];
};

#undef NUM

#endif
