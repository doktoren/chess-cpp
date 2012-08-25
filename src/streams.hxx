#ifndef _STREAMS_
#define _STREAMS_

#include <iostream>
#include <fstream>

using namespace std;


// cout is used solely for messages sent to XBoard
// cerr is used as the standard output for messages concerning:
//     a) Printing status messages (ie. "board initializes tables.")
//     b) Output for interaction with user (when XBoard is disabled)
//     c) Debugging messages
// big_output is used when large amounts of output have to be generated
// for debugging purposes (if cerr was used then all other messages
// directed here would drown in the amount of output generated).

// Messages send to big_output is unbuffered directed to "big_output.txt"
extern ofstream big_output;

// It is possible to redirect cerr to a file when running the program
// without XBoard connected. Example: ">>backup.set: print settings"
extern ostream *current_cerr;
// Make it possible to send messages to cerr without being redirected
#define real_cerr cerr

#ifdef XBOARD
// With XBoard connected, cerr is unbuffered redirected to "error.txt"
extern ofstream my_error;
#define cerr my_error
#else
// If current_cerr is defined, then this is used instead of the normal cerr
#define cerr (current_cerr ? *current_cerr : cerr)
#endif



// If XBoard is not used, then all messages intended for XBoard
// is prefixed with ">xboard: "
#ifndef XBOARD
struct My_ostream {
  My_ostream() : newline(true) {}
  void flush() { cout.flush(); }
  void setf(std::_Ios_Fmtflags gah) { cout.setf(gah); }
  bool newline;
};
template<class TYPE>
inline My_ostream& operator<<(My_ostream& os, const TYPE& type) {
  //cout << "#const TYPE&#";
  if (os.newline) cout << ">xboard: ";
  cout << type;
  os.newline = false;
  return os;
}
inline My_ostream& operator<<(My_ostream& os, const string& str) {
  //cout << "#const string&#";
  if (os.newline) cout << ">xboard: ";
  os.newline = str.find('\n') != string::npos;
  cout << str;
  return os;
}
inline My_ostream& operator<<(My_ostream& os, const char* str) {
  //cout << "#const char*#";
  return os << string(str);
}
inline My_ostream& operator<<(My_ostream& os, const char& ch) {
  //cout << "#const char&#";
  if (os.newline) {
    cout << ">xboard: ";
    os.newline = false;
  }

  if (ch == '\n') {
    return os << "\n";
  } else {
    return os << string(ch, 1);
  }
}
#define cout xboard_cout
extern My_ostream cout;
#endif



// pbp stands for print break point
#ifdef NDEBUG
#define pbp(output)
#else
extern int break_point_number;
#define pbp(output) cerr << "Break point " << ++break_point_number << " reached: " << (output) << '\n'
#endif




// cbo : cerr_big_output. An output stream that sends output both to
// cerr AND big_output

struct struct_cbo {};
template<class TYPE>
inline struct_cbo& operator<<(struct_cbo& os, const TYPE& type) {
  cerr << type;
  big_output << type;
  return os;
}
extern struct_cbo cbo;

#endif
