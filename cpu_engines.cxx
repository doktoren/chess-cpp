#include "cpu_engines.hxx"

Engine *load_cpu(Engine *cpu, int search_version, int evaluation_version, CPU_CommunicationModule *comm) {
  cerr << "Loading cpu Eval_" << evaluation_version
       << "_Search_" << search_version << "...\n";

  bool pfs = true;
  bool copy_board = cpu;
  string start_pos;
  vector<Move> history;
  if (copy_board) {
    pfs = cpu->played_from_scratch;
    start_pos = cpu->initial_position;
    history = cpu->get_move_history();
    delete cpu;
  }

  switch (search_version) {
  case 1:
    switch (evaluation_version) {
    case 1: cpu = new Eval_1_Search_1(comm); break;
    case 2: cpu = new Eval_2_Search_1(comm); break;
    case 3: cpu = new Eval_3_Search_1(comm); break;
    }
    break;
  case 2:
    switch (evaluation_version) {
    case 1: cpu = new Eval_1_Search_2(comm); break;
    case 2: cpu = new Eval_2_Search_2(comm); break;
    case 3: cpu = new Eval_3_Search_2(comm); break;
    }
    break;
  case 3:
    switch (evaluation_version) {
    case 1: cpu = new Eval_1_Search_3(comm); break;
    case 2: cpu = new Eval_2_Search_3(comm); break;
    case 3: cpu = new Eval_3_Search_3(comm); break;
    }
    break;
  }
  // cerr << "cpu->comm(" << cpu->comm << ") = comm(" << comm << ")\n";;

  if (copy_board) {
    if (pfs) cpu->new_game();
    else cpu->loadFEN(start_pos);

    for (uint i=0; i<history.size(); i++)
      cpu->execute_move(history[i]);
  }
  cerr << "...loading finished!\n";

  return cpu;
}
