#ifndef _UNSIGNED_LONG_LONG_
#define _UNSIGNED_LONG_LONG_

#include <iostream>

typedef unsigned long ulong;

using namespace std;

struct ull {
  ull() { u.ll.low = u.ll.high = 0; }
  ull(ulong low) {
    u.ll.low = low;
    u.ll.high = 0;
  }
  ull(ulong low, ulong high) {
    u.ll.low = low;
    u.ll.high = high;
  }
  ull(const ull& arg) {
    u.ll.low = arg.u.ll.low;
    u.ll.high = arg.u.ll.high;
  }

  ull operator+(const ull& b) {
    return ull(u.ll.low+b.u.ll.low, u.ll.high+b.u.ll.high);
  }
  ull& operator=(const ulong& a) {
    u.ll.low = a;
    u.ll.high = 0;
    return *this;
  }
  ull& operator|=(const ull& a) {
    u.ll.low |= a.u.ll.low;
    u.ll.high |= a.u.ll.high;
    return *this;
  }
  ull& operator&=(const ull& a) {
    u.ll.low &= a.u.ll.low;
    u.ll.high &= a.u.ll.high;
    return *this;
  }
  ull& operator|=(int bit_pos) {
    if (bit_pos & 0x20) u.ll.high |= 1<<(bit_pos-32);
    else u.ll.low |= 1<<bit_pos;
    return *this;
  }
  bool operator[](int bit_pos) const {
    if (bit_pos & 0x20) return (u.ll.high >> (bit_pos-32))&1;
    else return (u.ll.low >> bit_pos)&1;
  }  
  ull operator&(const ull& p) const {
    return ull(u.ll.low & p.u.ll.low, u.ll.high & p.u.ll.high);
  }
  ull operator~() {
    return ull(~u.ll.low, ~u.ll.high);
  }
  bool operator==(const ull& a) const {
    return u.ll.low == a.u.ll.low  &&  u.ll.high == a.u.ll.high;
  }
  bool as_bool() {
    return u.ll.low || u.ll.high;
  }
  void set_all() {
    u.ll.low = u.ll.high = 0xFFFFFFFF;
  }
  void clear_all() {
    u.ll.low = u.ll.high = 0;
  }
  void clear_bit(int bit_pos) {
    if (bit_pos & 0x20) u.ll.high &= ~(1<<(bit_pos-32));
    else u.ll.low &= ~(1<<bit_pos);
  }
  void set_bit(int bit_pos) {
    if (bit_pos & 0x20) u.ll.high |= 1<<(bit_pos-32);
    else u.ll.low |= 1<<bit_pos;
  }
  union {
    struct {
      ulong low, high;
    } ll;
    unsigned char lines[8];
  } u;
};

ostream& operator<<(ostream& os, const ull& val);

void print_bit_board(ostream &os, const ull& bit_board);

#endif
