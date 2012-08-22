#ifndef _ENGINE_
#define _ENGINE_

#include "cpu_communication_module.hxx"
#include "board_3.hxx"
#include "piece_values.hxx"

class Engine : public Board3plus {
public:
  Engine(CPU_CommunicationModule *comm = 0);
  ~Engine();

  virtual Move calculate_move(ostream& os) = 0;

  virtual int root_evaluate() = 0;
  int evaluate() { return evaluate(-INF, INF); }
  virtual int evaluate(int alpha, int beta) = 0;
  // fast_evaluate typically counts pieces
  virtual int fast_evaluate() = 0;

  CPU_CommunicationModule *comm;

  virtual CommandLineReceiver* get_clr_evaluation() = 0;
  virtual CommandLineReceiver* get_clr_search() = 0;
protected:
  // init_evaluation must be called from the root of the search
  // tree (before the search)
  virtual void init_evaluation() = 0;

private:
  // Private to prevent copying:
  Engine(const Engine&);
  Engine& operator=(const Engine&);
};

#endif
