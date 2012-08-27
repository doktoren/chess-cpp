#include "file_loader.hxx"

bool FileLoader::load_file(string filename) {
  name = filename;

  if (fp != NULL) fclose(fp);

  fp = fopen(filename.c_str(), "r");
  if (fp == NULL) {
    cerr << "Could not open file " << filename << "\n";
    return false;
  }
  return true;
}

char* FileLoader::getline() {
  if (fp == NULL) return 0;

  int ch, i=0;
  while ((ch = ::getc(fp)) != EOF  &&  ch != '\n')
    get_line_result[i++] = (char)ch;

  if (ch==EOF  &&  i==0) return 0;
  get_line_result[i] = 0;

  return get_line_result;
}

bool FileLoader::getc(char& ch) {
  if (fp == NULL) return false;
  int tmp = ::getc(fp);
  if (tmp == EOF) return false;
  ch = (char) tmp;
  return true;
}

bool FileLoader::getToken(string &s) {
  if (fp == NULL) return false;
  int tmp, index=0;
  // Skip delimiters before token
  while ((tmp = ::getc(fp)) != EOF  &&  delimiters[tmp]) ;
  if (tmp == EOF) return false;
  buff[index++] = tmp;
  // Read token
  while ((tmp = ::getc(fp)) != EOF  &&  !delimiters[tmp])
    buff[index++] = tmp;
  buff[index] = 0;
  // Push delimiter back
  if (tmp != EOF) ::ungetc(tmp, fp);
  if (index) {
    s = string(buff);
    // cerr << "Token is " << s << "\n";
    return true;
  } else {
    return false;
  }
}

void FileLoader::reset_delimiters() {
  memset(delimiters, false, 256);
  delimiters[(int)' '] = true;
  delimiters[(int)'\t'] = true;
  delimiters[(int)'\n'] = true;
  delimiters[13] = true;
}
void FileLoader::add_delimiter(uchar ch) {
  delimiters[(uchar)ch] = true;
}
void FileLoader::remove_delimiter(uchar ch) {
  delimiters[(uchar)ch] = false;
}

//#############################################
//#############################################

string load_FEN(string filename) {
  FileLoader fl(filename);
  return fl.getline();
}

void store_FEN(string FEN, string filename) {
  ofstream out(filename.c_str());
  out << FEN << '\n';
  out.flush();
}

//#############################################
//#############################################

// A pgn game must have at least one tag
// if true, shifts to next game.
bool PGNLoader::next_game() {
  char *line;
  // Skip lines until a line starting with '['
  while ((line = getline())  &&  *line != '[') ;
  if (!line) return false;

  // Read tags
  bool ok = read_tag(line);
  assert(ok);
  while ((line = getline())  &&  *line == '[') {
    ok = read_tag(line);
    assert(ok);
  }

  game_result = "unknown";
  return true;
}

bool PGNLoader::read_tag(char* line) {
  char tag[64], value[64];
  *value = 0; // in tag value is empty - eg. [Event ""]
  if (sscanf(line, "[%s \"%[^\"]\"\n", tag, value) >= 1) {
    tags[tag] = value;
    return true;
  } else {
    return false;
  }
}


void PGNLoader::setup_game(Board2& board) {
  if (tags.count("SetUp")  &&  tags["SetUp"]=="1") {
    if (tags.count("FEN")) {
      board.loadFEN(tags["FEN"]);
    } else {
      board.new_game();
      cerr << "PGNLoader: Error! SeqUp tag defined and = 1, but no FEN tag.\n";
    }
  } else {
    board.new_game();
  }
}

bool PGNLoader::next_move(Board2& board, Move& move) {
  string s;
  while (getToken(s)) {
    reinspect_s:
    if ('0'<=s[0] && s[0]<='9') {
      if (s == "0-0") {
        s = "O-O";
        goto a_move;
      }
      if (s == "0-0-0") {
        s = "O-O-O";
        goto a_move;
      }

      if (s.find('-') != string::npos) {
        // Game result
        game_result = s;
        return false;
      } else {
        // Move number. Check if moved in same token
        for (uint i=0; i<s.size(); i++)
          if (s[i]=='.') {
            for (uint j=1; i+j<s.size(); j++)
              if (s[i+j]!='.') {
                s = string(s, i+j, s.size()-(i+j));
                goto reinspect_s;
              }
          }
      }

    } else if (s[0] == '{') {
      // A comment. Read token until '}' found
      // cerr << "PGNLoader reads comment:" << s << '\n';
      while (s.find('}') == string::npos) {
        if (!getToken(s)) {
          return false;
        }
      }
      // cerr << "string s = " << s << "\n";
      //cerr << '\n';
    } else if (s[0] == '(') {
      // Recursive variation - just skip like comment
      while (s.find(')') == string::npos)
        if (!getToken(s)) return false;
    } else if (s[0] == '*') {
      // Game abandoned
      game_result = "*";
      return false;
    } else if (s[0] == '.') {
      // A number of dots, ignored

    } else {
      //cerr << "Move("; for(unsigned int i=0; i<s.size(); i++) cerr << (int)s[i] << ' '; cerr << ")\n";

      a_move:
      move = board.sanToMove(s);
#ifndef NDEBUG
      // Assert that board.sanToMove and board.moveToSAN works
      if (board.moveToSAN(move) != s) {
        big_output << "\nError! board.moveToSAN(move) != s in this position:\n";
        board.print_board(big_output);
        big_output << "move = " << move.toString2() << ", moveToSAN = " << board.moveToSAN(move)
		       << ", s = " << s <<  "\n\n";
        string s2 = board.moveToSAN(move);
        if (s[s.size()-1] != s2[s2.size()-1]) {
          cerr << "Error. In this position:\n" << board.toFEN() << "\n";
          assert(0);
        }
      }
#endif
      if (!move.is_defined()) {
        cerr << "\nPGNLoader::next_move - Error: pgn move " << s << " was not found in position:\n";
        board.print_board(cerr);
        board.print_moves(cerr);
        board.print_king_threats(cerr);
        board.print_bit_boards(cerr);
        board.print_threat_pos(cerr);
        cerr << "Saving error.fen\n";
        store_FEN(board.toFEN(), "error.fen");
        throw NextMoveError();
        return false;
      }
      return true;
    }
  }
  return false;
}

void PGNLoader::load_game(Board2& board) {
  setup_game(board);
  Move move;
  while (next_move(board, move)) {
    board.execute_move(move);
  }
}

void PGNLoader::print_tags(ostream &os) {
  typedef map<string, string>::const_iterator CI;
  os << "PGN Tags:\n";
  for (CI i=tags.begin(); i!=tags.end(); i++)
    os << "[" << (*i).first << " \"" << (*i).second << "\"]\n";
}


//#############################################
//#############################################

const string mandantory_tags[7] = {"Event","Site","Date","Round","White","Black","Result"};

void PGNWriter::write_token(string token) {
  if (line_pos + (int)token.length() + 1 > max_length) {
    out << '\n';
    line_pos = 0;
  }
  if (line_pos) {
    out << ' ';
    ++line_pos;
  }
  out << token;
  line_pos += token.length();
}

void PGNWriter::write_tags() {
  for (int i=0; i<7; i++)
    out << '[' << mandantory_tags[i] << " \""
    << tags[mandantory_tags[i]] << "\"]\n";

  for (map<string, string>::iterator it=tags.begin(); it!=tags.end(); it++) {
    bool mandantory_tag = false;
    for (int i=0; i<7; i++) {
      if ((*it).first == mandantory_tags[i]) {
        mandantory_tag = true;
        break;
      }
    }
    if (!mandantory_tag)
      out << '[' << (*it).first << " \"" << (*it).second << "\"]\n";
  }

  out << '\n';
}

void PGNWriter::add_move(Board2& board, Move move) {
  // move == Move()  =>  assumes it is a null move!
  if (!board.get_player()) {
    char tmp[8];
    sprintf(tmp, "%d.", (board.get_moves_played() >> 1)+1);
    write_token(tmp);
  }
  write_token(board.moveToSAN(move));
}

void PGNWriter::add_comment(string comment) {
  // Todo: what if comment.length > max_length?
  write_token("{");
  write_token(comment);
  write_token("}");
}

void PGNWriter::end_game(string result) {
  write_token(result);
  out << "\n\n";
  set_mandantory_tags();
}
void PGNWriter::end_game(Board2 &board) {
  end_game(game_result_texts[board.calc_game_status()]);
}

void PGNWriter::set_mandantory_tags() {
  tags.clear();
  for (int i=0; i<7; i++)
    tags[mandantory_tags[i]] = "?";
  /*
  tags["Event"] = "?";
  tags["Site"] = "?";
  tags["Date"] = "?";
  tags["Round"] = "?";
  tags["White"] = "?";
  tags["Black"] = "?";
  tags["Result"] = "?";
   */
}

void PGNWriter::set_tag(string tag, string value) {
  tags[tag] = value;
}

void PGNWriter::remove_tag(string tag) {
  if (tags.count(tag)) tags.erase(tag);
}

void PGNWriter::print_tags(ostream &os) {
  typedef map<string, string>::const_iterator CI;
  os << "PGN Tags:\n";
  for (CI i=tags.begin(); i!=tags.end(); i++)
    os << "[" << (*i).first << " \"" << (*i).second << "\"]\n";
}
