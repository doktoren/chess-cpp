#ifndef _CPU_COMMUNICATION_MODULE_
#define _CPU_COMMUNICATION_MODULE_

#include <bitset>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "settings.hxx"
#include "xboard_listener.hxx" // Access to shared mem

using namespace std;

#define NEITHER_COLOR 2

class CPU_CommunicationModule {
public:
  CPU_CommunicationModule() :
    settings("default.set")
  {
    cpu_is_thinking = false;
    cpu_color = NEITHER_COLOR;
    ping_respond = "";

    move_now = false;

#ifdef XBOARD
    use_fixed_depth = false;
    fixed_depth = 6;
    
    fixed_time_per_move = false;
    seconds_per_move = 1;
    
    setting_level_used = true;
    mps = 40; base=5; inc=0;
    
    cpu_time_in_centiseconds = 30000;
#endif
  }

  void print(ostream& os);

  // Use shared memory:
  void* operator new(size_t size);
  void operator delete(void *p, size_t size);

  // Status indicators:
  bool cpu_is_thinking;// set in chess.cxx
  int cpu_color;
  string ping_respond;

  // Communication
  bool move_now;

  Settings settings;

#ifdef XBOARD
  int num_ms_for_next_move(int half_moves_played) {
    assert(!use_fixed_depth);
    if (fixed_time_per_move) return 1000*seconds_per_move;

    if (half_moves_played < 2*mps) {
      // Not yet additional time!
      return 20*cpu_time_in_centiseconds / (2*mps - half_moves_played);

    } else {

      const double expected_num_remaining_moves = 20;
      return (int)(1000.0 + 10.0*cpu_time_in_centiseconds / expected_num_remaining_moves);
    }
  }
#endif


#ifdef XBOARD
  // << "level MPS BASE INC : Examples:\n"
  // << "    - level 40 5:30 0 : 40 moves per time control,\n"
  // << "      5 1/2 minutes per time control, conventional clock\n"
  // << "    - level 0 2 12 : Only one clock period,\n"
  // << "      base = 2 minutes, inc = 12 second\n"
  // << "st TIME  : Allow exactly TIME seconds per move\n"
  // << "sd DEPTH : Limit thinking to depth DEPTH\n"
  // << "time N : Set cpu's clock to N centiseconds left\n";

  // Is set from chess.cxx
  // Exactly one of use_fixed_depth, fixed_time_per_move, setting_level_used may be true
  bool use_fixed_depth;
  int fixed_depth;

  bool fixed_time_per_move;
  int seconds_per_move;

  bool setting_level_used;
  int mps, base, inc;

  int cpu_time_in_centiseconds;
#endif
};

ostream& operator<<(ostream& os, const CPU_CommunicationModule& comm);


// Todo:
// If you start the program like
//   ./run <input_commands.txt
// and the number of lines in input_commands.txt is larger than MAX_MESSAGE_SIZE
// then it will fail.
#define MAX_UNREAD_MESSAGES 128
#define MAX_MESSAGE_SIZE 256

class MessageQueue {
public:
  MessageQueue();
  
  bool empty();
  void pop(char *destination);

  bool push(const char *message);

  void* operator new(size_t size);
  void operator delete(void *p, size_t size);

private:
  char *mem;
  int first, last;
};

#endif
