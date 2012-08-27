#ifndef _FILE_LOADER_
#define _FILE_LOADER_

#include <stdio.h>
#include <assert.h>
#include <string>
#include <map>
#include <iostream>
#include <fstream>

#include "board_3.hxx"

using namespace std;

struct NextMoveError {
  int gah;
};

class FileLoader {
public:
  FileLoader(string filename = "") : fp(NULL) {
    if (filename != "") load_file(filename);
    reset_delimiters();
  }
  ~FileLoader() {
    if (fp != NULL) fclose(fp);
  }

  bool load_file(string filename);

  string getfilename() { return name; }

  // Make sure to copy the result before calling getline again.
  // Returns 0, if no line could be read.
  // It is possible to read an empty line.
  // Assumes '\n' is endline marker
  char* getline();

  bool getc(char& ch);

  bool getToken(string &s);
  void reset_delimiters();//to {' ','\t','\n'}
  void add_delimiter(uchar ch);
  void remove_delimiter(uchar ch);
private:
  FILE *fp;
  string name;
  char get_line_result[256];
  char buff[256];
  bool delimiters[256];
};



string load_FEN(string filename);
void store_FEN(string FEN, string filename);



class PGNLoader : private FileLoader {
public:
  PGNLoader(string filename = "") : FileLoader(filename) {}
  ~PGNLoader() {}

  bool load_file(string filename) {
    return FileLoader::load_file(filename);
  }

  string getfilename() { return FileLoader::getfilename(); }

  void print_tags(ostream &os);

  // if true, shifts to next game.
  bool next_game();

  // setup_game puts board in standard opening or opening specified by FEN tag.
  void setup_game(Board2& board);

  // can be used after a succesfull call of next_game & setup_game
  bool next_move(Board2& board, Move& move);
  
  // if next_move returns false, game_result can be read
  string get_game_result() { return game_result; }

  // load_game will call setup_game
  void load_game(Board2& board);

  map<string, string> tags;

private:
  bool read_tag(char *line);

  string game_result;
};


extern const string mandantory_tags[7];

class PGNWriter {
public:
  PGNWriter(string filename, int max_line_length=80) :
    max_length(max_line_length), filename(filename), out(filename.c_str())
  {
    line_pos = 0;
    assert(max_length >= 24);
  }
  ~PGNWriter() { out.flush(); out.close(); }

  string getfilename() { return filename; }

  void print_tags(ostream &os);

  void set_tag(string tag, string value);
  void remove_tag(string tag);
  // write_tags must be called before any call of add_move or write_game
  void write_tags();

  template<class Board3> void output_game(Board3 &board);
  void add_move(Board2& board, Move move);
  void add_comment(string comment); // may not contain { or }
  void end_game(string result);
  void end_game(Board2& board);
private:
  void write_token(string token);
  void set_mandantory_tags();

  int max_length, line_pos;
  string filename;
  ofstream out;
  map<string, string> tags;

  friend bool Board2::make_null_move();
};


template<class Board3>
void PGNWriter::output_game(Board3 &board) {
  Board2 b;
  int first_known_move = 0;
  if (!board.played_from_scratch) {
    //cerr << "Not played from scratch???\n";
    set_tag("SetUp", "1");
    set_tag("FEN", board.initial_position);
    first_known_move = board.cannot_undo_before;
  }

  write_tags();

  b.loadFEN(board.initial_position);
  vector<Move> history = board.get_move_history();
  for (uint i=first_known_move; i<history.size(); i++) {
    add_move(b, history[i]);
    if (history[i].is_defined()) b.execute_move(history[i]);
    else my_assert(b.make_null_move());
  }
  end_game(board); // b can't take move repetition into account
}

/*
18: Formal syntax

<PGN-database> ::= <PGN-game> <PGN-database>
                   <empty>

<PGN-game> ::= <tag-section> <movetext-section>

<tag-section> ::= <tag-pair> <tag-section>
                  <empty>

<tag-pair> ::= [ <tag-name> <tag-value> ]

<tag-name> ::= <identifier>

<tag-value> ::= <string>

<movetext-section> ::= <element-sequence> <game-termination>

<element-sequence> ::= <element> <element-sequence>
                       <recursive-variation> <element-sequence>
                       <empty>

<element> ::= <move-number-indication>
              <SAN-move>
              <numeric-annotation-glyph>

<recursive-variation> ::= ( <element-sequence> )

<game-termination> ::= 1-0
                       0-1
                       1/2-1/2
                       *
<empty> ::=
*/

#endif
