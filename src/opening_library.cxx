#include "opening_library.hxx"

#include "board_3.hxx"
#include "hash_table.hxx"

OpeningLibrary *opening_library;

bool OpeningLibrary::load_next_opening() {
  char_pos = line;
  return in.getline(line, 256, '\n');
}

// can be used after a succesfull call of next_opening()
bool OpeningLibrary::load_next_move(Board2& board, Move& move) {
  if (*char_pos  &&  *char_pos!='\n'  &&  *char_pos!=13) {
    move = board.moves_from_to(strToPos(string(char_pos, 2)), strToPos(string(char_pos+2, 2)));
    my_assert(board.next_move(move));
    char_pos += 4;
    return true;
  }
  return false;
}


void OpeningLibrary::fill_table() {
  Board3 b;
  int num_openings = 0;
  int num_positions = 0;
  while (load_next_opening()) {
    ++num_openings;
    // cerr << line;
    b.new_game();
    Move move;
    while (load_next_move(b, move)) {
      ++num_positions;
      b.execute_move(move);

      //if (b.moves_played == 1  &&  move.toString()=="e2e4") cerr << "hash value after e4 = " << b.hash_value << '\n';

      Info info = table[b.hash_value];
      if (info.is_valid()) {
	++info.value;
      } else {
	info = Info(1, 42, 42);
      }
      table.update(info);
    }
  }
  cerr << "Num entries in opening library = " << table.get_current_fill() << '\n'
       << "(num_openings = " << num_openings << ", num_positions = " << num_positions << ")\n";
  /*
  for (int i=1; i<=12; i++) {
    for (int p=0; p<64; p++)
      cerr << "    " << hash_values[(i<<6)|p] << '\n';
  }
  */
}

int OpeningLibrary::num_occurences(HashValue hash_value) {
  Info info = table[hash_value];
  if (info.is_valid()) return info.value;
  // cerr << hash_value << '\n';
  return 0;
}


void init_opening_library() {
  static bool initialized = false;
  if (!initialized) {
    opening_library = new OpeningLibrary();
    initialized = true;
  }
}
