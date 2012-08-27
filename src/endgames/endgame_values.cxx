#include "endgame_values.hxx"

#include <assert.h>

#include "../util/help_functions.hxx"

int endgame_cmp(char a, char b) {
  assert(!is_special_value(a) || a == ENDGAME_TABLE_DRAW);
  assert(!is_special_value(b) || b == ENDGAME_TABLE_DRAW);

  if (a<=0) { // a lost or drawn
    if (b>0) return -1;// b won
    return b-a; // b lost or drawn. small values of a and b are best
  }
  // a won
  if (b<=0) return 1; // b lost or drawn
  return b-a;// b won. small values of a and b are best
}

std::string endgame_value_to_string(char v) {
  if (is_special_value(v)) {
    switch (v) {
    case ENDGAME_TABLE_WIN:
      return "WIN";
    case ENDGAME_TABLE_LOSS:
      return "LOSS";
    case ENDGAME_TABLE_DRAW:
      return "DRAW";
    case ENDGAME_TABLE_UNKNOWN:
      return "????";
    case ENDGAME_TABLE_ILLEGAL:
      return "*";
    }
  }
  if (v>0) {
    return "M"+signedToString(v);
  } else {
    return "-M"+signedToString(-v);
  }
}
