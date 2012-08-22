#include "unsigned_long_long.hxx"

#include "help_functions.hxx"
#include "board.hxx"

ostream& operator<<(ostream& os, const ull& val) {
  return os << "ull(" << ::toString(val.u.ll.low, 8, 16) << ::toString(val.u.ll.high, 8, 16) << ')';
}

void print_bit_board(ostream &os, const ull& bit_board) {
  bool l[64];
  for (int i=0; i<64; i++) l[i] = bit_board[i];
  print_map64(os, l, 1, 10);
}
