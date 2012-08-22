#include "typedefs.hxx"

#include <vector>
#include <string>
#include <iostream>

#include "board.hxx"

class TestSuite {
public:
  TestSuite();

  static bool clr_test_suite(Board *board, ostream& os, vector<string> &p);

  static void test_load_pgns(string filename);
  static void test_all_pgns();

  static void test_all_pgns_plus();
  static void test_load_pgns_plus(string filename);

  static void count_endgame_stuff();

  static void test_eval3(int num);

  static void test_retro_moves();
};
