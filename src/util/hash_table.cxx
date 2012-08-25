#include "hash_table.hxx"

#include "help_functions.hxx"

inline ostream& operator<<(ostream& os, const Example_HashValue& hash_value) {
  return os << "Example_HashValue(" << hash_value.hash_value << ')';
}

inline ostream& operator<<(ostream& os, const Example_Content& content) {
  return os << "Example_Content(" << content.count << ')';
}

//########################################

static const int MAX_NUM_BITS[8] = {6, 9, 13, 16, 19, 22, 32, 32};

void HTA_STAT::clear() {
  for (int i=0; i<16; i++) {
    num_with_depth[i] = 0;
    for (int j=0; j<8; j++)
      matches_found[i][j] = 0;
  }
}

void HTA_STAT::found(int depth, uint age){
  int num_bits = ceil_log(age);
  int i=0;
  while (MAX_NUM_BITS[i] < num_bits) i++;
  ++matches_found[depth][i];
}

ostream& operator<<(ostream& os, const HTA_STAT& stat) {
  int max_group = -1;
  int max_depth = -1;
  for (int g=0; g<8; g++) {
    for (int d=0; d<16; d++) {
      if (stat.matches_found[d][g]) {
	if (max_group < g) max_group = g;
	if (max_depth < d) max_depth = d;
      }
    }
  }

  if (max_depth < 0) { os << "Empty statistic!\n"; return os; }
  
  string IN[8];
  IN[0] = " [2^0,  2^6[ ";
  IN[1] = " [2^6,  2^9[ ";
  IN[2] = " [2^9,  2^13[";
  IN[3] = " [2^13, 2^16[";
  IN[4] = " [2^16, 2^19[";
  IN[5] = " [2^19, 2^22[";
  IN[6] = " [2^22, 2^32[";
  IN[7] = " [2^32, 2^32[";

  os <<   "depth|total \\ age |";
  for (int g=0; g<=max_group; g++) os << IN[g];
  os << "\n-----+------------+";
  for (int g=0; g<=max_group; g++) os << "-------------";
  os << '\n';
  for (int d=0; d<=max_depth; d++) {
    os << (d>9 ? " " : "  ") << d << "  | " << toString(stat.num_with_depth[d], 10, 10) << " |";
    for (int g=0; g<=max_group; g++)
      os << "  " << toString(stat.matches_found[d][g], 10, 9) << " ";
    os << '\n';
  }
  return os;
}

