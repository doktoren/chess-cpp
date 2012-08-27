#ifndef _STATIC_EXCHANGE_EVALUATION_
#define _STATIC_EXCHANGE_EVALUATION_

#include "move_and_undo.hxx"
#include "my_vector.hxx"
#include "piece_values.hxx"

extern const uint BOARD_CONTROL_CONSTANTS[13];
extern const int PIECE_CONTROL_MEASURES[7];

extern uint8_t COMPRESS_CAPTURE_LIST[1<<16];
#define CCLL (197+1) // compressed capture list length
extern uint16_t DECOMPRESS_CAPTURE_LIST[CCLL];

extern uint8_t NUM_PIECES_DEFENDING[CCLL];

extern int CONTROL_MEASURE[CCLL];

extern int8_t SEE_LIST[5*CCLL*CCLL];

class Board2plus;

class SEE {
public:
  SEE(Board2plus &board) : board(board) {
    clear();
    init_compress_capture_list();
    init_num_pieces_defending();
    init_control_measure();
    init_see_list();
    //for (int i=0; i<16; i++) for (int j=0; j<16; j++) protect_count[i][j] = 0;
  }
  void clear() {
    for (int i=0; i<64; i++)
      board_control[i] = 0;

    num_targets[WHITE] = num_targets[BLACK] = 0;
    for (int i=0; i<64; i++)
      target_ref[i] = 0;
  }
  void print(ostream &os);

  string capture_list_to_string(uint16_t cl);

  // Update functions
  void insert_piece(Position pos, Piece piece) { reevaluate_position(pos, piece); }
  void remove_piece(Position pos) { reevaluate_position(pos, 0); }
  void increase_control(Move move, Piece piece);
  void decrease_control(Move move, Piece piece);

  void print_targets(ostream &os);
  void print_compression_list(ostream &os);

  // Access functions

  // has_control(player, pos) <=> player has a piece that can attack pos
  bool has_control(int player, int pos) {
    return bc(player, pos);
  }
  // number of player's pieces that can attack pos
  uint8_t num_pieces_defending(int player, int pos) {
    return NUM_PIECES_DEFENDING[COMPRESS_CAPTURE_LIST[bc(player, pos)]];
  }
  // Same as above, but smaller pieces have more weight
  // (it is almost always to attack with a pawn, but rarely a queen)
  // each piece currently weighted between 1 and 5
  int control_measure(int player, int pos) {
    return CONTROL_MEASURE[COMPRESS_CAPTURE_LIST[bc(player, pos)]];
  }
  // control_measure(pos) is relative to white
  int control_measure(int pos) {
    return CONTROL_MEASURE[COMPRESS_CAPTURE_LIST[bc(WHITE, pos)]] -
      CONTROL_MEASURE[COMPRESS_CAPTURE_LIST[bc(BLACK, pos)]];
  }

  // see(piece, pos), piece is on pos
  int see(int pos, int piece);

  int8_t calc_see(int piece, uint16_t aggressor, uint16_t defender);

  // size() returns the number of squares with a piece that with gain
  // can be captured by the opponent (according to the s.e.e.)
  int size(int attacking_side) {
    assert(attacking_side==0  ||  attacking_side==1);
    return num_targets[attacking_side];
  }

  //int operator[](int index) { return targets[index].first; }
  int target_position(int attacking_side, int index) {
    assert(attacking_side==0  ||  attacking_side==1);
    return targets[attacking_side][index].first;
  }
  int target_value(int attacking_side, int index) {
    assert(attacking_side==0  ||  attacking_side==1);
    return targets[attacking_side][index].second;
  }

  int8_t& index_see(int piece_kind, int aggressor, int defender) {
    assert(PAWN<=piece_kind  &&  piece_kind<=QUEEN  &&
	   1<=aggressor  &&  aggressor<CCLL  &&
	   1<=defender  &&  defender<CCLL);
    int8_t &result = SEE_LIST[CCLL*CCLL*(piece_kind-1) + CCLL*aggressor + defender];
    //cerr << "index_see(" << piece_kind << ", " << aggressor << ", " << defender << ") = " << (int)result << '\n';
    return result;
  }

  void test_see_list();

  int best_see_capture_value(int player);
  //int see_result(int player);

  int move_result(Move move);

private:
  void reevaluate_position(Position pos, Piece piece);

  void init_compress_capture_list();
  void init_num_pieces_defending();
  void init_control_measure();
  void init_see_list();
  uint16_t bc(int player, int pos) {
    return board_control[pos] >> (player << 4);
    //return player ? (board_control[pos]>>16) : board_control[pos];
    //return ((ushort *)&(board_control[pos]))[player];
  }

  Board2plus &board;

  uint32_t board_control[64];

  // targets[attacking side][...]
  pair<int, int> targets[2][16];//<Position, value>
  int num_targets[2];

  // target_ref[p]==0 => no gainfull capture possible
  int target_ref[64];
  void clear_target_ref(int pos) { target_ref[pos] = 0; }
  void set_target_ref(int pos, int attacking_side, int index) {
    target_ref[pos] = 1 + (attacking_side << 4) + index;
  }
  bool is_target(int pos) { return target_ref[pos]; }
  bool target_color(int pos) { return target_ref[pos] >= 17; }
  pair<int, int> &get_target(int pos) {
    return ((pair<int, int> *)&(targets[0][-1]))[target_ref[pos]];
  }
};

#endif
