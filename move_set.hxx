#ifndef _MOVE_SET_
#define _MOVE_SET_

// Optimized for
// 6 insert and 30 find

using namespace std;

#include <bitset>

#include "my_vector.hxx"
#include "hash_value.hxx"

class MoveSet {
public:
  MoveSet() : move_list(8) {
    init_hash_values();
    clear();
  }
  void clear() {
    hash_value.reset();
    move_list.clear();
  }

  void insert(Move move) {
    hash_value.set((hash_values[move.from].low ^ hash_values[move.to].low) & 0x3F);
    move_list.push_back(move);
  }

  bool try_insert(Move move) {
    int tmp = (hash_values[move.from].low ^ hash_values[move.to].low) & 0x3F;
    if (hash_value.test(tmp)) {
      // Throughout check
      for (int i=0; i<move_list.size(); i++)
	if (move_list[i] == move) return false;
    } else {
      hash_value.set(tmp);
    }
    move_list.push_back(move);
    return true;
  }

  bool find(Move move) {
    if (!hash_value.test((hash_values[move.from].low ^ hash_values[move.to].low) & 0x3F))
      return false;
    for (int i=0; i<move_list.size(); i++)
      if (move_list[i] == move) return true;
    return false;
  }

  int size() { return move_list.size(); }

  MyVector<Move> move_list;
private:
  bitset<64> hash_value;

  // Private to prevent copying:
  MoveSet(const MoveSet&);
  MoveSet& operator=(const MoveSet&);
};


// MoveSet2 allows each move to be associated with some element
template <class TYPE>
class MoveSet2 {
public:
  MoveSet2() : move_list(8) {
    init_hash_values();
    clear();
  }
  void clear() {
    hash_value.reset();
    move_list.clear();
  }

  void insert(Move move, TYPE element) {
    hash_value.set((hash_values[move.from].low ^ hash_values[move.to].low) & 0x3F);
    move_list.push_back(pair<Move, TYPE>(move, element));
  }

  // returns false if move already present
  bool try_insert(Move move, TYPE element) {
    int tmp = (hash_values[move.from].low ^ hash_values[move.to].low) & 0x3F;
    if (hash_value.test(tmp)) {
      // Throughout check
      for (int i=0; i<move_list.size(); i++)
	if (move_list[i].first == move) return false;
    } else {
      hash_value.set(tmp);
    }
    move_list.push_back(pair<Move, TYPE>(move, element));
    return true;
  }

  bool find(Move move, TYPE &element) {
    if (!hash_value.test((hash_values[move.from].low ^ hash_values[move.to].low) & 0x3F))
      return false;
    for (int i=0; i<move_list.size(); i++)
      if (move_list[i].first == move) {
	element = move_list[i].second;
	return true;
      }
    return false;
  }

  int size() { return move_list.size(); }

  //MyVector<pair<Move, TYPE> >& get_list() { return move_list; }
  MyVector<pair<Move, TYPE> > move_list;
private:
  bitset<64> hash_value;

  // Private to prevent copying:
  MoveSet2(const MoveSet2&);
  MoveSet2& operator=(const MoveSet2&);
};

#endif
