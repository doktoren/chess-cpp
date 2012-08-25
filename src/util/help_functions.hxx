#ifndef _HELP_FUNCTIONS_
#define _HELP_FUNCTIONS_

#include <string>

#include "../typedefs.hxx"

extern const char HEX_CHAR[16];

#define BOOL_STR(b) ((b) ? "true" : "false")

// 2 <= base  &&  base <= 16
string toString(uint number, int base = 10);
// Pads with 0's
string toString(uint number, int num_digits, int base);
uint toInt(string s, int base = 10);

// if num_digits = 0, then returned string is suff. long
// Pads with ' '
string signedToString(int number, int num_digits=0, int base=10);

string doubleToString(double number, int digits_before = 1, int digits_after = 3);

inline int min(int a, int b) { return a>b?b:a; }
inline int max(int a, int b) { return a>b?a:b; }

extern const int uchar_log[256];
extern const int bit_count[256];
extern const uchar uchar_hash[256];

inline int floor_log(uint n) {
  if (n & 0xFFFF0000) {
    if (n & 0xFF000000) return 24 + uchar_log[n >> 24];
    return 16 + uchar_log[n >> 16];
  } else {
    if (n & 0xFF00) return 8 + uchar_log[n >> 8];
    return uchar_log[n];
  }
}
inline int ceil_log(uint n) {
  if (n) return floor_log(n-1)+1;
  return 0;
}


bool file_exists(string filename);


string game_theoretical_value_to_string(int value);

#endif
