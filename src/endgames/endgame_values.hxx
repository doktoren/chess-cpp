#ifndef _ENDGAME_VALUES_
#define _ENDGAME_VALUES_

#include <string>

#define ENDGAME_TABLE_WIN -124
#define ENDGAME_TABLE_DRAW -125
#define ENDGAME_TABLE_LOSS -126
#define ENDGAME_TABLE_UNKNOWN -127
#define ENDGAME_TABLE_ILLEGAL -128
inline bool is_special_value(int val) { return val <= -124; }

// -123 <= a,b <= 124 or they can represent a draw:
// return negative if a worse than b, 0 if equal, positive otherwise
int endgame_cmp(char a, char b);

inline char add_ply_to_endgame_value(char v) {
  if (is_special_value(v)) return v;
  if (v > 0) return -v;
  return -v+1;
}

std::string endgame_value_to_string(char v);

#endif
