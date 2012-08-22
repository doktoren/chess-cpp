#ifndef _CPU_EVALUATION_3_
#define _CPU_EVALUATION_3_

// Evaluation function count pieces in a clever way.

#include <vector>

#include "engine.hxx"
#include "piece_values.hxx"
#include "unsigned_long_long.hxx"

struct PawnTableHV {
  PawnTableHV() : wps(), bps() {}
  PawnTableHV(ull wps, ull bps) : wps(wps), bps(bps) {}

  const bool operator==(const PawnTableHV &p) const {
    return wps==p.wps  &&  bps==p.bps;
  }

  int get_index(unsigned int max_index) const {
    assert(max_index==1023);//omskriv kode hvis tabelstoerrelse aendres

    int result = 0;

    result ^= uchar_hash[wps.u.lines[1]];
    result ^= uchar_hash[wps.u.lines[2]] << 1;
    result ^= uchar_hash[wps.u.lines[3]] << 2;
    result ^= uchar_hash[wps.u.lines[4]];
    result ^= uchar_hash[wps.u.lines[5]] << 1;
    result ^= uchar_hash[wps.u.lines[6]] << 2;

    result ^= uchar_hash[bps.u.lines[1]];
    result ^= uchar_hash[bps.u.lines[2]] << 1;
    result ^= uchar_hash[bps.u.lines[3]] << 2;
    result ^= uchar_hash[bps.u.lines[4]];
    result ^= uchar_hash[bps.u.lines[5]] << 1;
    result ^= uchar_hash[bps.u.lines[6]] << 2;

    return result;
  }

  ull wps, bps;
};

struct Eval3_Settings : public SettingListener {
  Eval3_Settings(Settings *_settings) : SettingListener(_settings, "Eval3_") {
    set_settings(_settings);
  }

  void set_settings(Settings *_settings) {
    settings = _settings;

    show_evaluation_info = get_bool("show_evaluation_info");
  }

  void print(ostream &os) {
    pbool(os, "show_evaluation_info");
  }

  bool *show_evaluation_info;
};

class Eval_3 : public virtual Engine {
public:
  Eval_3();
  ~Eval_3();
  
  void reset_all();

  int root_evaluate() {
    init_evaluation();
    return evaluate(-INF, INF);
  }
  int evaluate(int alpha, int beta);
  int fast_evaluate() {
    int value = player ? -piece_value : piece_value;

    // Force progress
    if (moves_played_since_progress > 36) {
      value *= 100-moves_played_since_progress;
      value /= 64;  // todo >> ?
    }
    return value;
  }

  CommandLineReceiver* get_clr_evaluation() { return clr_evaluation; }
  static bool clr_evaluation(Board *board, ostream& os, vector<string> &p);
  void print_eval_stat(ostream& os);

  bool passed_pawn(Position pos);

protected:
  void init_evaluation();
  void init_piece_values();
  void init_control_values();


  void insert_piece(Position pos, Piece piece);
  void remove_piece(Position pos);
  void move_piece(Position from, Position to);

  // called by evaluate
  int castling_value();
  int control_value();
  int opening_library_value() ;

  int piece_value;
  ull pawn_bitboards[2];

  int game_phase[3];
  int game_phase_value;

  int num_evaluations;

  Eval3_Settings settings;


  int pawn_structure_value();
  HashTable_Simple<PawnTableHV, int> pawn_table;
  int passed_pawns_value();
  int double_pawns_value();

private:
  // called by reset_all
  void init_passed_pawns_bitboards();

  // called by init_evaluation
  void set_piece_values();


  // Private to prevent copying:
  Eval_3(const Eval_3&);
  Eval_3& operator=(const Eval_3&);
};

#endif
