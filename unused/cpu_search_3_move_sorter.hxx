#ifndef _CPU_SEARCH_3_MOVE_SORTER_
#define _CPU_SEARCH_3_MOVE_SORTER_

// Based on a heap sort.
// The best moves are those that lead to a position where the
// value of the position is low (for the opponent). Hence
// the lowest values should be returned first.
class MoveSorter {
public:
  void init() {
    index = 0;
  }
  void add_move(const Move& move, const Info& info) {
    entries[index++] = pair<Move, Info>(move, info);
  }
  void done_adding() {
    // Construct heap
    size = index;
    for (int i=0; i<size; i++)
      heap[i] = eval_key(entries[i].second, i);
    heap[size] = 0x80000000;

    for (int i=(size-1)>>1; i>=0; i--)
      bubble_down(i);
  }

  bool next_move(Move &move, Info& info) {
    if (!size) return false;
    // Remember the move returned (in case push_back is later called).
    last_index_returned = heap[0] & 0xFF;
    heap[0] = heap[--size];
    heap[size] = 0x80000000;
    bubble_down(0);
    // Set move and info
    move = entries[last_index_returned].first;
    info = entries[last_index_returned].second;

    return true;
  }

  void push_back(const Info& info) {
    heap[size] = eval_key(info, last_index_returned);
    bubble_up(size);
    heap[++size] = 0x80000000;
  }


private:
  // K_MOVE is negative because it is cheaper to further explore
  // a brach with a low depth
  static const int K_MOVE = -300;
  // K_EVAL_TYPE is positive because a value stored as LOWER_BOUND is
  // much more promising as a value stored as UPPER_BOUND.
  static const int K_EVAL_TYPE = 300;
  int eval_key(const Info& info, int index) {
    // Warning! beware of overflow.
    return ((info.value + K_MOVE*info.depth + K_EVAL_TYPE*info.eval_type) << 8) | index;
  }

  void bubble_down(int index) {
    int tmp = index + index;
    while (tmp < size) {
      // heap[size] = min{x|x:int}
      tmp += heap[tmp+1] > heap[tmp];
      if (heap[tmp] >= heap[index]) return;
      swap(heap[tmp], heap[index]);

      tmp += index = tmp;
    }
  }

  void bubble_up(int index) {
    while (index) {
      int up = index>>1;
      if (heap[up] <= heap[index]) return;
      swap(heap[up], heap[index]);
      
      index = up;
    }
  }

  pair<Move, Info> entries[MAX_POSSIBLE_MOVES];
  int heap[MAX_POSSIBLE_MOVES];

  int index, size;
  int last_index_returned;
};

#endif
