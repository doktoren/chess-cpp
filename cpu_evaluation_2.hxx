#ifndef _CPU_EVALUATION_2_
#define _CPU_EVALUATION_2_

// Evaluation function count pieces in a clever way.

#include "engine.hxx"

class Eval_2 : public virtual Engine {
public:
  Eval_2();
  ~Eval_2();

  void reset_all();

  int root_evaluate() { return evaluate(-INF, INF); }
  int evaluate(int alpha, int beta);
  int fast_evaluate() { return evaluate(-INF, INF); }

  CommandLineReceiver* get_clr_evaluation() { return clr_evaluation; }
  static bool clr_evaluation(void *ignored, Board *board, ostream& os, vector<string> &p);
  void print_eval_stat(ostream& os);

protected:
  void init_evaluation();

  void insert_piece(Position pos, Piece piece);
  void remove_piece(Position pos);
  void move_piece(Position from, Position to);

  int count_pieces();

  int material; // pawn 1, knights 3, etc. Independent on color
  int piece_values[3];

  int num_evaluations;
};

#endif
