#ifndef _CPU_SEARCH_1_
#define _CPU_SEARCH_1_

// Implements a simple alpha beta search

#include "cpu_communication_module.hxx"
#include "engine.hxx"
#include "cpu_search.hxx"

class Search_1 : public virtual Engine {
public:
  Search_1();
  ~Search_1();
  
  Move calculate_move(ostream& os);

  CommandLineReceiver* get_clr_search() { return clr_search; }

  static bool clr_search(Board *board, ostream& os, vector<string> &p);
  void print_search_stat(ostream& os);
protected:
  int alpha_beta(int depth, int alpha, int beta);

  int root_depth;

  SearchStuff search;
};

#endif
