#include "test_suite.hxx"

#include "file_loader.hxx"
#include "util/help_functions.hxx"

#include "engine/cpu_engines.hxx"
#include "board_3.hxx"
#include "engine/cpu_evaluation_1.hxx"
#include "engine/cpu_evaluation_2.hxx"
#include "engine/cpu_evaluation_3.hxx"
#include "engine/cpu_search_1.hxx"
#include "engine/cpu_search_2.hxx"
#include "engine/cpu_search_3.hxx"

#include <map>
#include <queue>

bool TestSuite::clr_test_suite(__attribute__((unused)) Board *board, ostream& os, vector<string> &p) {
  if (dot_demand(p, 1, "help")) {
    os << "Test suite, help:\n"
        << "    test all pgns plus  or  tapp\n"
        << "    test all pgns  or  tap\n"
        << "    count endgame stuff  or  ces\n"
        << "    compare eval functions n\n"
        << "       - compare Eval2 and Eval3 on n positions.\n"
        << "    test retro moves  or  trm\n";

  } else if (dot_demand(p, 3, "test", "all", "pgns")) {
    TestSuite::test_all_pgns();

  } else if (dot_demand(p, 4, "test", "all", "pgns", "plus")) {
    TestSuite::test_all_pgns_plus();

  } else if (dot_demand(p, 3, "count", "endgame", "stuff")) {
    TestSuite::count_endgame_stuff();

  } else if (dot_demand(p, 4, "compare", "eval", "functions", (uintptr_t)0)) {
    TestSuite::test_eval3(atoi(p[3].c_str()));

  } else if (dot_demand(p, 3, "test", "retro", "moves")) {
    TestSuite::test_retro_moves();

  } else return false;
  return true;
}


class GameExaminer {
public:
  bool load_file(__attribute__((unused)) string filename) { return true; }
  bool next_game() { return true; }
  void next_move(__attribute__((unused)) Move& move) {}
  void game_result(__attribute__((unused)) string result) {}
};

template <class GAME_EXAMINER, class BOARD>
class Examine_a00_e99 {
public:
  Examine_a00_e99() : ge() {}

  void go() {
    cerr << "Trying to load all games in ../pgn\n"
        << "This will take a long time!\n";
    for (int c='a'; c<='e'; c++) {
      string tmp = "../pgn/x";
      tmp[7] = c;
      for (int n=0; n<100; n++) {
        string filename = tmp + toString(n) + ".pgn";
        if (ge.load_file(filename)) {
          system(("gunzip " + filename + ".gz").c_str());
          load_pgn_file(filename);
          system(("gzip " + filename).c_str());
        }
      }
    }
  }

  GAME_EXAMINER ge;

private:
  void load_pgn_file(string filename) {
    cerr << "Trying to load pgn games in " << filename << "\n";
    cerr << "(one # will be printed for every game succesfully loaded)\n";
    BOARD board;
    PGNLoader loader(filename.c_str());
    while (loader.next_game()) {
      if (!ge.next_game()) break;

      loader.setup_game(board);
      Move move;
      while (loader.next_move(board, move)) {
        ge.next_move(move);
        board.execute_move(move);
      }

      ge.game_result(loader.get_game_result());

      cerr << "#";
      cerr.flush();
    }
    cerr << "\n";
  }
};



void TestSuite::test_all_pgns() {
  Examine_a00_e99<GameExaminer, Board3> examine;
  examine.go();
}


class TestAllPGNsPlus {
public:
  bool load_file(__attribute__((unused)) string filename) { return true; }
  bool next_game() { b.new_game(); return true; }
  void next_move(Move& move) {
    try {
      b.execute_move(move);
      if (!b.sanity_check_moves(cerr)) {
        b.print_board(cerr);
        abort();
      }
    }
    catch (NextMoveError) {
      //b.print_help_classes(cerr);
      b.sanity_check_moves(cerr);
      b.print_board(cerr);
      abort();
    }
    catch (Error) {
      b.print_board(cerr);
      abort();
    }
  }
  void game_result(__attribute__((unused)) string result) {}
  Board3plus b;
};

void TestSuite::test_all_pgns_plus() {
  Examine_a00_e99<TestAllPGNsPlus, Board2> examine;
  examine.go();
}



//##############################################################################


// copy-pasted from endgame_database.cxx
const bool GT[16][16] =
{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0,1,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,0,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
void sort_5_piece_pos(vector<PiecePos> &pieces) {
  // start by sorting pieces[0,1,3,4] like sort_4_piece_pos
  if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
  if (GT[pieces[3].piece][pieces[4].piece]) swap(pieces[3], pieces[4]);
  if (GT[pieces[0].piece][pieces[3].piece]) swap(pieces[0], pieces[3]);
  if (GT[pieces[1].piece][pieces[4].piece]) swap(pieces[1], pieces[4]);
  if (GT[pieces[1].piece][pieces[3].piece]) swap(pieces[1], pieces[3]);

  // find the place for pieces[2]
  if (GT[pieces[1].piece][pieces[2].piece]) {
    swap(pieces[1], pieces[2]);
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);

  } else if (GT[pieces[2].piece][pieces[3].piece]) {

    swap(pieces[2], pieces[3]);
    if (GT[pieces[3].piece][pieces[4].piece]) swap(pieces[3], pieces[4]);
  }
}
bool swap_5_piece_pos(vector<PiecePos> &pieces) {
  if (PIECE_KIND[pieces[1].piece] == KING) {
    PiecePos weak_king = pieces[0];
    pieces[0] = pieces[1];
    pieces[1] = pieces[2];
    pieces[2] = pieces[3];
    pieces[3] = pieces[4];
    pieces[4] = weak_king;
    return true;
  }
  if (PIECE_KIND[pieces[2].piece] == KING) {
    PiecePos weak_piece_1 = pieces[0];
    PiecePos weak_piece_2 = pieces[1];
    pieces[0] = pieces[2];
    pieces[1] = pieces[3];
    pieces[2] = pieces[4];
    pieces[3] = weak_piece_1;
    pieces[4] = weak_piece_2;
    return true;
  }
  return false;
}

void TestSuite::count_endgame_stuff() {
  cerr << "Trying to load all games in ../pgn\n"
      << "This will take a long time!\n";
  map<string, int> counts;
  Board2 board;
  for (int c='a'; c<='e'; c++) {
    string tmp = "../pgn/x";
    tmp[7] = c;
    for (int n=0; n<100; n++) {
      string filename = tmp + toString(n) + ".pgn";
      system(("gunzip " + filename + ".gz").c_str());

      PGNLoader loader(filename.c_str());
      while (loader.next_game()) {
        loader.setup_game(board);
        Move move;
        while (loader.next_move(board, move)) {
          board.execute_move(move);
          if (board.get_num_pieces() == 5) {
            vector<PiecePos> pieces(5);
            board.get_piece_list(pieces);
            sort_5_piece_pos(pieces);
            if (swap_5_piece_pos(pieces)) {
              for (int i=0; i<5; i++)
                pieces[i].piece += (pieces[i].piece>WKING) ? -6 : 6;
            }
            string s = "XXXXX";
            for (int i=0; i<5; i++)
              s[i] = PIECE_CHAR[pieces[i].piece];
            cerr << s << " ";
            counts[s]++;
            break;
          }
        }
      }

      system(("gzip " + filename).c_str());
    }
  }

  cerr << "############  TADAAAA!!!!   ###############\n";
  priority_queue<pair<int, string> > pq;
  for (map<string,int>::const_iterator ci = counts.begin(); ci != counts.end(); ci++)
    pq.push(pair<int, string>((*ci).second, (*ci).first));

  while (!pq.empty()) {
    cerr << pq.top().second << ": " << pq.top().first << "\n";
    pq.pop();
  }
}

/*
KRPkr: 1976
KPPkp: 1264
KPPkr: 342
KNPkp: 331
KBPkp: 302
KRPkp: 252
KNPkn: 175
KNPkb: 160
KPPkn: 157
KQPkq: 140
KPPkb: 127
KRBkr: 122
KRPkb: 118
KBPkb: 114
KBPkr: 98
KBPkn: 96
KRPkn: 94
KQPkp: 83
KRNkr: 82
KNPkr: 77
KRPkq: 50
KPPkq: 49
KPPPk: 45
KRPPk: 39
KBPPk: 37
KBNkp: 34
KNPPk: 28
KQPPk: 25
KBNkb: 19
KQPkr: 17
KBNkr: 16
KBPkq: 15
KNNkp: 12
KRBPk: 11
KRNkp: 10
KRBkp: 10
KNPkq: 10
KBNkn: 10
KQNkq: 9
KQRkp: 8
KBNPk: 8
KBBkb: 8
KQRPk: 7
KNNkr: 7
KRNPk: 6
KQNkp: 6
KRRkr: 5
KQPkn: 5
KQBPk: 5
KBBkp: 4
KRRkn: 3
KQRkr: 3
KQNPk: 3
KQBkq: 3
KBBkn: 3
KRRPk: 2
KRNkq: 2
KRNkn: 2
KRBkq: 2
KRBkb: 2
KQRkq: 2
KQRkn: 2
KQBkp: 2
KNNkn: 2
KBBkr: 2
KBBkq: 2
KBBPk: 2
KRRkq: 1
KRRkp: 1
KRRkb: 1
KRNkb: 1
KRBNk: 1
KQRBk: 1
KQQPk: 1
KQPkb: 1
KQNkn: 1
KQNkb: 1
KQBkr: 1
KNNkq: 1
KNNPk: 1
KBNkq: 1
 */


class CompareEval {
public:
  ~CompareEval() { if (l) delete l; }

  bool load_file(__attribute__((unused)) string filename) { return load_files; }

  void init(Engine *_cpu, int _num_positions) {
    assert(status == 0);
    load_files = true;
    cpu = _cpu;
    num_positions = _num_positions;
    l = new int[num_positions];
    store = true;
    status = 1;
  }

  void compare_with(Engine *_cpu) {
    assert(status == 1);
    load_files = true;
    cpu = _cpu;
    allowed_diff = 0;
    index = 0;
    store = false;
    status = 2;
  }

  bool next_game() {
    assert(status);
    cpu->new_game();
    return load_files;
  }
  void next_move(Move& move) {
    assert(status); 
    if (index == num_positions) return;

    cpu->execute_move(move);
    if (store) {
      l[index] = cpu->root_evaluate();
      //cerr << "Evaluate2 result = " << l[index] << "\n";
    } else {
      int e = cpu->root_evaluate();
      //cerr << "Evaluate3 result = " << l[index] << "\n";
      if (e < l[index]-allowed_diff  ||  l[index]+allowed_diff < e) {
        cerr << "Evaluations differ than previous record position below!\n";
        cpu->print_board(cerr);
        cerr << cpu->toFEN() << "\n";
        cerr << "Stored evaluation = " << l[index]
                                            << ", new evaluation = " << e
                                            << ", fast evaluate = " << cpu->fast_evaluate() << "\n\n";
        allowed_diff = abs(l[index]-e);
        if (allowed_diff > 10000)
          *(comm->settings.get_bool_setting("Eval3_show_evaluation_info")) = true;
      }
    }
    if (++index == num_positions) load_files = false;
  }
  void game_result(__attribute__((unused)) string result) {}

  bool store;
  int allowed_diff;
  Engine *cpu;
  int index, num_positions;
  int *l;
  bool load_files;
private:
  int status;
};

void TestSuite::test_eval3(int num) {
  cerr << "TestSuite::test_eval3(" << num << ") called!\n";
  assert(num > 0);
  Engine *cpu = new Eval_2_Search_1(comm);
  Examine_a00_e99<CompareEval, Board2> examine;
  examine.ge.init(cpu, num);
  examine.go();
  delete cpu;
  cpu = new Eval_3_Search_1(comm);
  examine.ge.compare_with(cpu);
  examine.go();
  delete cpu;
  cerr << "TestSuite::test_eval3(" << num << ") finished!\n";
}


//##############################################################################


class TestRetroMoves {
public:
  TestRetroMoves() : b(), max_retro_move_count(0) {}

  bool load_file(string filename) {
    current_file = filename;
    game_number_in_file = 0;
    return true;
  }

  bool next_game() {
    ++game_number_in_file;
    b.new_game();
    return true;
  }


  void next_move(Move& move) {
    //cerr << "+";
    try {
      gah[b.get_moves_played()] = move;

      uint8_t before = b.allowed_symmetries();
      Undo undo = b.execute_move(move);
      uint8_t after = b.allowed_symmetries();


      uint8_t transform[8];
      int num_transforms = 0;

      if (before==3  &&  after==0xFF) {
        transform[num_transforms++] = 0;
        transform[num_transforms++] = 2;//inverse of 2
        transform[num_transforms++] = 4;//inverse of 4
        transform[num_transforms++] = 6;//inverse of 5
      } else if (before==1  &&  after==0xFF) {
        do {
          transform[num_transforms] = num_transforms;
        } while (++num_transforms<8);
      } else if (before==1  &&  after==3) {
        transform[num_transforms++] = 0;
        transform[num_transforms++] = 1;//inverse of 1
      } else if (before == after) {
        transform[num_transforms++] = 0;
      } else {
        cerr << "Error: transform " << (int)before << " -> " << (int)after << "\n";
        assert(0);
        exit(1);
      }


      for (int j=0; j<num_transforms; j++) {

        Move tmove(move);
        Undo tundo(undo);

        if (transform[j]) {
          /*
	  // modify tmove, tundo
	  tmove.from = reflect(move.from, transform[j]);
	  tmove.to = reflect(move.to, transform[j]);
	  if (legal_pos(tundo.threat_pos))
	    tundo.threat_pos = reflect(tundo.threat_pos, transform[j]);
	  if (legal_pos(tundo.en_passant))
	    tundo.en_passant = reflect(tundo.en_passant, transform[j]);
           */

          // transform board
          b.transform_board(transform[j]);
        }


        vector<triple<Move,Undo,int> > rm = b.get_retro_moves(true, true, true, true);

        if (rm.size() > max_retro_move_count) {
          cerr << "\nNew record! The position with description\n\t"
              << b.toFEN() << "\nhas " << rm.size() << " retro moves.\n";
          max_retro_move_count = rm.size();
        }

        bool found2 = false;

        for (uint i=0; i<rm.size(); i++) {
          // Don't think it will cause any error that positions with more
          // than 32 occur.

          if (rm[i].third) b.transform_board(rm[i].third);

          b.undo_move(rm[i].first, rm[i].second);

          bool found = false;
          Move m = b.moves();
          while (b.next_move(m)) {
            found |= m == rm[i].first;

            Undo u = b.execute_move(m);
            b.undo_move(m, u);
          }

          b.execute_move(rm[i].first);

          if (rm[i].third) b.inv_transform_board(rm[i].third);

          if (!found) {
            cerr << "File " << current_file << ", game " << game_number_in_file << "\n"
                << i << ":\t" << rm[i].first.toString2() << "\t"
                << rm[i].third << "\t" << rm[i].second.toString() << "...";
            cerr << "Move not found:\n";

            {
              if (rm[i].third) b.transform_board(rm[i].third);
              b.undo_move(rm[i].first, rm[i].second);
              b.print_board(cerr);
              Move m = b.moves();
              while (b.next_move(m)) {
                cerr << "(" << m.toString2() << ")";
                Undo u = b.execute_move(m);
                b.undo_move(m, u);
              }
              b.execute_move(rm[i].first);
              if (rm[i].third) b.inv_transform_board(rm[i].third);
            }


            b.print_board(cerr);
            assert(0);
            exit(1);
          }

          found2 |= (rm[i].first == tmove)  &&  (rm[i].second == tundo);
        }

        if (!found2) {
          cerr << "File " << current_file << ", game " << game_number_in_file << ":\n"
              << "\nLast played move (" << tmove.toString2() << ", " << tundo.toString() << ")\n"
              << "in current position not returned by retro moves.\n";
          for (uint i=0; i<rm.size(); i++) {
            // Why the fuck is "" necessary?!?
            cerr << "" << i << ":\t" << rm[i].first.toString2() << "\t"
                << rm[i].third << "\t" << rm[i].second.toString() << "\n";
          }
          b.print_board(cerr);
          for (int i=0; i<b.get_moves_played(); i++)
            cerr << gah[i].toString2() << " ";
          cerr << "\n";
          assert(0);
          exit(1);
        }


        if (transform[j]) {
          // transform board back!
          b.inv_transform_board(transform[j]);
        }
      }
    }
    catch (Error) {
      cerr << "Error in this position:\n";
      b.print_board(cerr);
      exit(1);
    }

  }
  void game_result(__attribute__((unused)) string result) {}

  Board2 b;
  uint max_retro_move_count;

  string current_file;
  int game_number_in_file;
  Move gah[512];
};

void TestSuite::test_retro_moves() {
  cerr << "TestSuite::test_retro_moves() called!\n";
  Examine_a00_e99<TestRetroMoves, Board2> examine;
  examine.go();
}
