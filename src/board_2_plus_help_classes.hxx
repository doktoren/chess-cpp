#ifndef _BOARD_2_PLUS_HELP_CLASSES_
#define _BOARD_2_PLUS_HELP_CLASSES_

#include "typedefs.hxx"
#include "move_and_undo.hxx"
#include "board_printers.hxx"

#define WA_ROOK_ID 0
#define WB_KNIGHT_ID 1
#define WC_BISHOP_ID 2
#define W_QUEEN_ID 3
#define W_KING_ID 4
#define WF_BISHOP_ID 5
#define WG_KNIGHT_ID 6
#define WH_ROOK_ID 7
inline int W_PAWN_ID(int colum) { return 8 + colum; }
#define BA_ROOK_ID 16
#define BB_KNIGHT_ID 17
#define BC_BISHOP_ID 18
#define B_QUEEN_ID 19
#define B_KING_ID 20
#define BF_BISHOP_ID 21
#define BG_KNIGHT_ID 22
#define BH_ROOK_ID 23
inline int B_PAWN_ID(int colum) { return 24 + colum; }

#define PS false

// maintain piece_number[p]==42 for unoccupied p
class PieceNumber {
public:
  PieceNumber() { clear(); }
  void clear() {
    // loadFEN(...standard opening...) places the pieces in the following order:
    // white king, black king and then in order of position.

    piece_number_stack[31] = W_KING_ID;
    piece_number_stack[30] = B_KING_ID;

    piece_number_stack[0] = BH_ROOK_ID;
    piece_number_stack[1] = BG_KNIGHT_ID;
    piece_number_stack[2] = BF_BISHOP_ID;
    piece_number_stack[3] = B_QUEEN_ID;
    piece_number_stack[4] = BC_BISHOP_ID;
    piece_number_stack[5] = BB_KNIGHT_ID;
    piece_number_stack[6] = BA_ROOK_ID;

    for (int c=0; c<8; c++)
      piece_number_stack[14-c] = B_PAWN_ID(c);

    for (int c=0; c<8; c++)
      piece_number_stack[22-c] = W_PAWN_ID(c);

    piece_number_stack[23] = WH_ROOK_ID;
    piece_number_stack[24] = WG_KNIGHT_ID;
    piece_number_stack[25] = WF_BISHOP_ID;
    piece_number_stack[26] = W_QUEEN_ID;
    piece_number_stack[27] = WC_BISHOP_ID;
    piece_number_stack[28] = WB_KNIGHT_ID;
    piece_number_stack[29] = WA_ROOK_ID;

    // for (int i=0; i<32; i++) piece_number_stack[i] = i;
    stack_index = 32;

    for (int i=0; i<32; i++) piece_position[i] = ILLEGAL_POS;
    for (int p=0; p<64; p++) piece_number[p] = 42;
  }
  void print(ostream &os) {
    print_map64(os, piece_number, 2, 10);
  }
  bool sanity_check(ostream &os) {
    for (int i=0; i<32; i++) {
      if (piece_position[i] != ILLEGAL_POS) {
        if (piece_number[piece_position[i]] != i) {
          os << "piece_number[piece_position[" << i << "]] != " << i << "\n";
          return false;
        }
        for (int p=0; p<64; p++) if (p != piece_position[i]) {
          if (piece_number[p] == i) {
            os << "piece_number[" << POS_NAME[p] << "] == " << i << "\n";
            return false;
          }
        }
        for (int j=0; j<stack_index; j++)
          if (piece_number_stack[j] == i) {
            os << "piece_number_stack[" << j << "] == " << i << "\n";
            return false;
          }
      }
    }
    for (int i=0; i<stack_index; i++)
      for (int p=0; p<64; p++)
        if (piece_number_stack[i] == piece_number[p]) {
          os << "piece_number_stack[" << i << "] == piece_number[" << POS_NAME[p] << "]\n";
          return false;
        }
    return true;
  }

  // undefined result if no piece at pos
  uint8_t& operator[](Position pos) {
    assert(pos < 64);
    return piece_number[pos];
  }

  Position get_pos(uint8_t piece_number) {
    return piece_position[piece_number];
  }

  void insert_piece(Position pos, __attribute__((unused)) Piece piece) {
    assert(piece_number[pos] == 42);
    piece_number[pos] = pop_piece_number();
    piece_position[piece_number[pos]] = pos;
  }
  void remove_piece(Position pos) {
    assert(piece_number[pos] < 32);
    piece_position[piece_number[pos]] = ILLEGAL_POS;
    push_piece_number(piece_number[pos]);
    piece_number[pos] = 42;
  }
  void move_piece(Position from, Position to) {
    assert(piece_number[from] < 32);
    assert(piece_number[to] == 42);
    piece_position[piece_number[to] = piece_number[from]] = to;
    piece_number[from] = 42;
  }

  bool number_used(int number) {
    return piece_position[number] != ILLEGAL_POS;
  }

  // mapping[old_piece_number] |-> new_piece_number
  void remap_pieces(__attribute__((unused)) uint8_t *mapping) {
    // todo
  }
private:
  uint8_t pop_piece_number() {
    assert(stack_index);
    return piece_number_stack[--stack_index];
  }
  void push_piece_number(uint8_t list_index) {
    assert(stack_index != 32);
    piece_number_stack[stack_index++] = list_index;
  }

  uint8_t piece_number[64];

  Position piece_position[32];

  uint8_t piece_number_stack[32];
  int stack_index;
};


struct IndexStruct {
  IndexStruct() : piece_moves_index(0), square_move_list_index(0),
      move_list_index(0), defined(false) {}
  IndexStruct(uint8_t piece_moves_index, uint8_t square_move_list_index,
      uint8_t move_list_index) :
        piece_moves_index(piece_moves_index), square_move_list_index(square_move_list_index),
        move_list_index(move_list_index), defined(true) {}

  void clear() {
    piece_moves_index = 0;
    square_move_list_index = 0;
    move_list_index = 0;
    defined = false;
  }

  uint8_t piece_moves_index;
  uint8_t square_move_list_index;
  uint8_t move_list_index;
  bool defined;
};

class MoveToIndex {
public:
  MoveToIndex() { clear(); }
  void clear() {
    memset(internal_list, 0, sizeof(IndexStruct)*4096);
  }
  void print(ostream &os) {
    os << "MoveToIndex: todo\n";
  }

  // MoveToIndex() {}
  uint8_t& piece_moves_index(Move move) {
    return internal_list[(move.from << 6) | move.to].piece_moves_index;
  }
  uint8_t& square_move_list_index(Move move) {
    return internal_list[(move.from << 6) | move.to].square_move_list_index;
  }
  uint8_t& move_list_index(Move move) {
    return internal_list[(move.from << 6) | move.to].move_list_index;
  }
  bool& defined(Move move) {
    return internal_list[(move.from << 6) | move.to].defined;
  }

  IndexStruct& operator[](Move move) {
    return internal_list[(move.from << 6) | move.to];
  }
private:
  IndexStruct internal_list[4096];
};


class PieceMoves {
public:
  PieceMoves() { clear(); }
  void clear() {
    for (int i=0; i<32; i++)
      piece_move_lists[i].count = 0;
  }
  void print(ostream &os) {
    os << "PieceMoves: todo\n";
    //os << "Piece number " << piece_number << ", moves = ";
    //for (int i=0; i<piece_move_lists[i].count; i++) os << uint8_t
  }

  uint8_t add_to_list(uint8_t piece_number, uint8_t index) {
    if (PS) cerr << "PieceMoves::add_to_list(" << (int)piece_number << ", " << (int)index << ")\n";
    uint8_t result = piece_move_lists[piece_number].count++;
    piece_move_lists[piece_number].indexes[result] = index;
    return result;
  }

  bool remove_from_list(uint8_t piece_number, uint8_t index) {
    if (PS) cerr << "PieceMoves::remove_from_list(" << (int)piece_number << ", " << (int)index << ")\n";
    LL &l = piece_move_lists[piece_number];
    if (index == --l.count) {
      // removing from end of list :-)
      return false;
    } else {
      // creating a gap in the list, which have to be filled
      l.indexes[index] = l.indexes[l.count];
      return true;
    }
  }

  /*
  bool remove_from_list(pair<uint8_t, uint8_t> p) {
    return remove_from_list(p.first, p.second);
  }
   */

  uint8_t& index(uint8_t piece_number, uint8_t index) {
    assert(piece_number<32);
    assert(index < piece_move_lists[piece_number].count);
    return piece_move_lists[piece_number].indexes[index];
  }

  /*
  uint8_t& index(pair<uint8_t, uint8_t> p) {
    return piece_move_lists[p.first].indexes[p.second];
  }
   */

  int count(uint8_t piece_number) {
    return piece_move_lists[piece_number].count;
  }

  // mapping[old_piece_number] |-> new_piece_number
  void remap_pieces(__attribute__((unused)) uint8_t *mapping) {
    // todo
  }
private:
  struct LL {
    int count;
    uint8_t indexes[28];
  };
  LL piece_move_lists[32];
};


class SquareMoveList {
public:
  SquareMoveList() { clear(); }
  void clear() {
    for (int i=0; i<64; i++)
      move_lists[i].count = 0;
  }
  void print(ostream &os) {
    os << "SquareMoveList: counts:\n";
    uint8_t counts[64];
    for (int i=0; i<64; i++) counts[i] = move_lists[i].count;
    print_map64(os, counts, 2, 10);
  }

  uint8_t insert_move(Position pos, uint8_t move_index) {
    if (PS) cerr << "SquareMoveList::insert_move(" << POS_NAME[pos] << ", " << (int)move_index << ")\n";
    LL & l = move_lists[pos];
    uint8_t index = l.count++;
    l.move_index[index] = move_index;
    return index;
  }

  bool remove_move(Position pos, uint8_t index) {
    if (PS) cerr << "SquareMoveList::remove_move(" << POS_NAME[pos] << ", " << (int)index << ")\n";
    LL &l = move_lists[pos];

    if (index == --l.count) {
      // removing from end of list :-)
      return false;
    } else {
      // creating a gap in the list, which have to be filled
      l.move_index[index] = l.move_index[l.count];
      return true;
    }
  }

  uint8_t& index(Position pos, uint8_t index) {
    return move_lists[pos].move_index[index];
  }
  /*
  uint8_t* begin(Position pos) {
    return &(move_lists[pos].move_index[0]);
  }
  uint8_t* end(Position pos) {
    return &(move_lists[pos].move_index[move_lists[pos].count]);
  }
   */

  void init_iterator(Position pos) {
    iterator_current = &(move_lists[pos].move_index[0]) - 1;
    iterator_end = &(move_lists[pos].move_index[move_lists[pos].count]);
  }
  bool iterate() {
    return ++iterator_current != iterator_end;
  }
  bool iterate(int player) {
    while (++iterator_current != iterator_end) {
      if ((*iterator_current)>>7 == player) return true;
    }
    return false;
  }
  uint8_t deref_iterator() {
    return *iterator_current;
  }

private:
  struct LL {
    int count;
    uint8_t move_index[60];// only (7+7+7+6)+8=35 needed
  };
  LL move_lists[64];

  uint8_t *iterator_end;
  uint8_t *iterator_current;
};

class MoveList {
public:
  MoveList() { clear(); }
  void clear() {
    for (int i=0; i<256; i++)
      move_ref[i] = move_backref[i] = i;
    count[WHITE] = 0;
    count[BLACK] = 128;
  }

  void print(ostream &os) {
    os << "MoveList: num_moves(W=" << count[WHITE] << ", B="
        << count[BLACK] << ")\n";
    os << "White moves: ";
    for (int i=0; i<count[WHITE]; i++)
      os << toString(i) << ", ";
    os << '\n';
    os << "Black moves: ";
    for (int i=128; i<count[BLACK]; i++)
      os << toString(i) << ", ";
    os << '\n';
  }
  bool sanity_check(ostream &os) {
    for (int i=0; i<256; i++) {
      if (move_ref[move_backref[i]] != i) {
        os << "move_ref[move_backref[" << i << "]] != " << i << "\n";
        return false;
      }
      if ((i^move_ref[i])&128) {
        os << "(" << i << "^move_ref[" << i << "])&128\n";
        return false;
      }
    }
    return true;
  }

  uint8_t add_move(int player, Move move) {
    if (PS) cerr << "MoveList::add_move(" << player << ", " << move.toString() << ")\n";
    assert((count[player] & 0x7F) != 0x7F);
    /* The assertion above can be triggered by an exceptional (but possible) number of queens
    if (!((count[player] & 0x7F) != 0x7F)) {
      cerr << "count[WHITE] = " << count[WHITE] << ", count[BLACK] = " << count[BLACK] << "\n";
      throw Error("too many moves");
    }
     */

    uint8_t move_index = move_ref[count[player]++];
    moves[move_index] = move;
    return move_index;
  }

  void remove_move(uint8_t index) {
    if (PS) cerr << "MoveList::remove_move(" << moves[index].toString() << ")\n";
    assert(count[index_to_player(index)] & 0x7F);

    int i = move_backref[index];
    int i2 = --count[index_to_player(i)];

    swap(move_ref[i], move_ref[i2]);
    swap(move_backref[move_ref[i]], move_backref[move_ref[i2]]);
  }

  Move operator[](uint8_t index) {
    assert((move_backref[index]<128  &&  move_backref[index]<count[WHITE])  ||
        (move_backref[index]>=128  &&  move_backref[index]<count[BLACK]));
    return moves[index];
  }

  /*
  bool prev_index(uint8_t &index) {
    if (move_backref[index] & 0x7F) {
      index = move_ref[move_backref[index] - 1];
      return true;
    } else return false;
  }
  int next_to_last_index(int player) {
    return move_ref[count[player]];
  }

  iterator begin(int player) {
    return iterator(&(move_ref[player << 7]));
  }
  Move deref(int it) {
    return moves[move_ref[it]];
  }
  string toString(int it) {
    string tmp("---");
    Move m = moves[move_ref[it]];
    if (m.blah & CAN_ATTACK) tmp[0] = '+';
    if (m.blah & CAN_NON_ATTACK) tmp[1] = '+';
    if (m.blah & FURTHER_MOVEMENT_POSSIBLE) tmp[2] = '+';
    return m.toString() + tmp;
  }
  iterator end(int player) {
    return iterator(&(move_ref[count[player]]));
  }

  struct iterator {
    iterator(uint8_t* p) :
    move_ref_pointer(p) {}

    iterator operator--() { --move_ref_index;  }
    iterator operator++() { ++move_ref_index;  }

    uint8_t* move_ref_pointer;
  };
   */

  int index_to_player(uint8_t move_index) {
    return move_index >> 7;
  }

  void init_iterator(int player) {
    iterator_current = &(move_ref[(player << 7)-1]);
    iterator_end = &(move_ref[count[player]]);
  }
  bool iterate() {
    return ++iterator_current != iterator_end;
  }
  Move deref_iterator() {
    return moves[*iterator_current];
  }

  // The more potential moves, the move unlikely a zugswang situation is
  int get_num_potential_moves(int player) {
    return count[player];
  }

private:
  Move moves[256];

  uint8_t move_ref[2*128];
  uint8_t move_backref[256];

  int count[2];

  uint8_t *iterator_end;
  uint8_t *iterator_current;
};

#undef PS

#endif
