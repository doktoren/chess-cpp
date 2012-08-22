#include "cpu_search.hxx"

// #####################################
// ######   Line of play stuff  ########
// #####################################

string leaf_reached_kind[4] =
{"evaluate", "game over", "endgame table", "hash table"};

void SearchStuff::print_line_of_play(ostream &os) {
  os << "Line of play:";
  int i;
  for (i=0; principal_line[0].line[i].is_defined(); i++)
    os << ' ' << principal_line[0].line[i].toString();
  os << ' ' << leaf_reached_kind[principal_line[0].leaf_kind] << '\n';
  /*
  if (principal_line.line[0][i].special_move == 42)
    os << " h.t. used";
  os << '\n';
  */
}

void SearchStuff::alpha_improved(Move move, int depth) {
  principal_line[depth].line[depth] = move;
  int i=depth;
  do {
    ++i;
    principal_line[depth].line[i] = principal_line[depth+1].line[i];
  } while (principal_line[depth+1].line[i].is_defined());

  principal_line[depth].leaf_kind = principal_line[depth+1].leaf_kind;
}

void SearchStuff::leaf_reached(int depth, int leaf_kind) {
  principal_line[depth].line[depth] = Move();
  principal_line[depth].leaf_kind = leaf_kind;
}

Move SearchStuff::best_move() {
  return principal_line[0].line[0];
}


// #####################################
// ######   Timer/Clock stuff   ########
// #####################################

void SearchStuff::start_clock(int allowed_time_in_ms) {
  allowed_time_in_seconds = 0.001*allowed_time_in_ms;
  start_clock_time = time(NULL);
}

bool SearchStuff::out_of_time() {
  double seconds_passed = difftime(time(NULL), start_clock_time);
  return seconds_passed >= allowed_time_in_seconds;
}

int SearchStuff::percent_time_used() {
  double seconds_passed = difftime(time(NULL), start_clock_time);
  if (seconds_passed < 0.001) seconds_passed = 0.001;
  return (int)(100.0*(seconds_passed / allowed_time_in_seconds));
}

// #####################################
// ########    Other stuff    ##########
// #####################################


