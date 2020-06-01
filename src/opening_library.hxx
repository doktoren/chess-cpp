#ifndef _OPENING_LIBRARY_
#define _OPENING_LIBRARY_

#include <fstream>
#include <map>

#include "board_2.hxx"
#include "util/hash_table.hxx"
#include "hash_value.hxx"
#include "transposition_table_content.hxx"

class OpeningLibrary {
public:
  OpeningLibrary() : table(19), in("../NEWBOOK.TXT") { fill_table(); }

  int num_occurences(HashValue hash_value);
private:
  void fill_table();

  bool load_next_opening();
  // can be used after a successful call of next_opening()
  bool load_next_move(Board2& board, Move& move);

  HashTable<HashValue, Info> table;

  char line[256];
  char *char_pos;
  ifstream in;
};


void init_opening_library();
extern OpeningLibrary *opening_library;

#endif
