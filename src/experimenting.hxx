#ifndef _EXPERIMENTING_
#define _EXPERIMENTING_
// This file contains all the defines that control the compression

#include <iostream>
using namespace std;

// MAX_MEN must be 4 or 5
// The files generated will be the same no matter if MAX_MEN is 4 or 5.
#define MAX_MEN 5
#if MAX_MEN >= 5
#define ALLOW_5_MEN_ENDGAME
#endif

// Warning: When changing the defines in this file,
// all .bdd will have to be rebuild!

// Used by binary_decision_diagram.?xx
// Iff BDD_USE_COMPRESSED_ENCODING==1 then use the compact version.
#define BDD_USE_COMPRESSED_ENCODING 1

// Used by endgame_database.cxx
// if BOUND_KING==1 then limit black king to a1-d1-d4 triangle/a1-d8 rectangle instead of white king
#define BOUND_KING 1

#define ENDGAME_TABLE_WITH_CASTLING 1

inline void load_define_value(int fd, char value, string name) {
  char tmp;
  read(fd, &tmp, 1);
  if (tmp != value) {
    cerr << "Error: Loaded file was created using different defined" << endl << "value for " << name << "\n";
    exit(1);
  }
}

inline void save_define_value(int fd, char value) {
  char tmp = value;
  write(fd, &tmp, 1);
}

#endif
