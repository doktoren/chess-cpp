#ifndef _ENDGAME_VALUES_
#define _ENDGAME_VALUES_

#define ENDGAME_TABLE_WIN -124
#define ENDGAME_TABLE_DRAW -125
#define ENDGAME_TABLE_LOSS -126
#define ENDGAME_TABLE_UNKNOWN -127
#define ENDGAME_TABLE_ILLEGAL -128
inline bool is_special_value(int val) { return val <= -124; }

string endgame_value_to_string(char v);

// -123 <= a,b <= 124 or they can represent a draw:
// return negative if a worse than b, 0 if equal, positive otherwise
inline int endgame_cmp(char a, char b) {
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

inline char add_ply_to_endgame_value(char v) {
  if (is_special_value(v)) return v;
  if (v > 0) return -v;
  return -v+1;
}


#endif
