#ifndef _TRANSPOSITION_TABLE_CONTENT_
#define _TRANSPOSITION_TABLE_CONTENT_

#include "typedefs.hxx"
#include "move_and_undo.hxx"

// These constants must not be changed. They are used in a comparison
// operation (EXACT_EVAL is most useful, then UPPER_BOUND, etc.)
// The values may not be zero (see is_valid() below)
#define EMPTY_INFO 0
#define UPPER_BOUND 1
#define EXACT_EVAL 2
#define EXACT_VALUE 3
#define LOWER_BOUND 4

inline int swap_eval_type(int et) {
  return (et & 2) ? et : (et^5);
}

// a position with the game theoretical value known is stored with ply = MAX_PLY
#define MAX_PLY 64

struct Info {
  Info() : value(0), depth(0), eval_type(0) {}
  Info(int value, uint8_t depth, uint8_t eval_type) :
    value(value), depth(depth), eval_type(eval_type) {}

  bool is_valid() const { return eval_type; }
  void clear() { eval_type = 0; }

  int32_t value;
  int8_t depth;
  uint8_t eval_type;
};
ostream& operator<<(ostream& os, const Info& info);

struct EntryWithMove {
  EntryWithMove() : value(0), ply(0), eval_type(0), time_stamp(0), move() {}
  EntryWithMove(int32_t value, uint8_t ply, uint8_t eval_type, uint16_t time_stamp, Move move) :
    value(value), ply(ply), eval_type(eval_type), time_stamp(time_stamp),
    move(move) {}

  bool is_valid() const { return eval_type; }
  void clear() {
    ply = 0;
    eval_type = 0;
    time_stamp = 0;
  }

  int32_t value;
  int8_t ply;
  uint8_t eval_type;
  uint16_t time_stamp;
  Move move;
};

#ifndef NDEBUG
struct DEBUG_EntryWithMove {
  DEBUG_EntryWithMove() : value(0), ply(0), eval_type(0), time_stamp(0), move(), hboard() {}
  DEBUG_EntryWithMove(int32_t value, uint8_t ply, uint8_t eval_type, uint16_t time_stamp,
		      Move move, HashBoard hboard) :
    value(value), ply(ply), eval_type(eval_type), time_stamp(time_stamp),
    move(move), hboard(hboard) {}

  bool is_valid() const { return eval_type; }
  void clear() {
    ply = 0;
    eval_type = 0;
    time_stamp = 0;
  }

  int32_t value;
  int8_t ply;
  uint8_t eval_type;
  uint16_t time_stamp;
  Move move;

  HashBoard hboard;
};
#endif

#endif
