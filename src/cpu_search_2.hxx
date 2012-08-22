#ifndef _CPU_SEARCH_2_
#define _CPU_SEARCH_2_

// cpu search 2 is an alpha-beta search using a transposition table.

#include "cpu_communication_module.hxx"
#include "engine.hxx"
#include "hash_table.hxx"
#include "hash_value.hxx"
#include "transposition_table_content.hxx"
#include "cpu_search.hxx"

class Search_2 : public virtual Engine {
public:
  Search_2();
  ~Search_2();
  
  Move calculate_move(ostream& os);

  CommandLineReceiver* get_clr_search() { return clr_search; }

  static bool clr_search(void *ignored, Board *board, ostream& os, vector<string> &p);
  void print_search_stat(ostream& os);

protected:
  int alpha_beta(int depth, int ply, int alpha, int beta);

  int num_tt_used;

  void update_table(Info info, int ply) {
    table[hash_value];
    if (info.value > WIN-ply) info.value += ply;
    if (info.value < -(WIN-ply)) info.value -= ply;
    table.update(info);
  }

  HashTable_Age<HashValue, Info> table;

  SearchStuff search;

  int max_moves_played_reached;
  bool search_aborted;
};


#endif
