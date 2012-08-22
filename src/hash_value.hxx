#ifndef _HASH_VALUE_
#define _HASH_VALUE_

#include <iostream>
#include <assert.h>
#include <string>

using namespace std;

struct HashValue {
  // Problem med HashValue() : low(0), high(0) {}
  // Constructor bliver kaldt paa HashValue hash_values[13*64]
  // EFTER at denne er blevet initialiseret andetsteds fra.
  // HashValue() : low(0), high(0) {}
  // Ny løsning, ændr hash_values[13*64] til *hash_values
  HashValue() : low(0), high(0) {}
  HashValue(unsigned int low, unsigned int high) : low(low), high(high) {}
  // max_index will always be on the form (2^n)-1
  int get_index(unsigned int max_index) const {
    assert(!(max_index & (max_index+1)));
    return low & max_index;
  }

  void clear() { low = high = 0; }

  string toBitString() {
    string result("0123456701234567012345670123456701234567012345670123456701234567");
    for (int i=0; i<32; i++) {
      result[31^i] = '0'+((high>>i)&1);
      result[63^i] = '0'+((low>>i)&1);
    }
    return result;
  }

  unsigned int low, high;
};
ostream& operator<<(ostream& os, const HashValue& hash_value);
inline HashValue& operator^=(HashValue& h1, const HashValue& h2) {
  h1.low ^= h2.low;
  h1.high ^= h2.high;
  return h1;
}
inline bool operator==(const HashValue& h1, const HashValue& h2) {
  return (h1.low==h2.low)  &&  (h1.high==h2.high);
}
inline bool operator<(const HashValue& h1, const HashValue& h2) {
  return (h1.low < h2.low)  ||
    (h1.low == h2.low  &&  h1.high < h2.high);
}

extern HashValue *hash_values;
void init_hash_values();

#endif
