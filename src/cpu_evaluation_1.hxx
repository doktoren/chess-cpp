#ifndef _CPU_EVALUATION_1_
#define _CPU_EVALUATION_1_

// Evaluation function count pieces according to PIECE_VALUES[]

#include "engine.hxx"

class Eval_1 : public virtual Engine {
public:
  Eval_1();
  ~Eval_1();

  void reset_all();

  int root_evaluate() { return evaluate(-INF, INF); }
  int evaluate(int alpha, int beta);
  int fast_evaluate() { return evaluate(-INF, INF); }

  CommandLineReceiver* get_clr_evaluation() {
    return clr_evaluation;
  }

  static bool clr_evaluation(void *ignored, Board *board, ostream& os, vector<string> &p);
  void print_eval_stat(ostream& os);

protected:
  void init_evaluation();

  void insert_piece(Position pos, Piece piece);
  void remove_piece(Position pos);
  void move_piece(Position from, Position to);

  int value;

  int num_evaluations;
};

#endif
