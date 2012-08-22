#include "hash_value.hxx"

#include "help_functions.hxx"

ostream& operator<<(ostream& os, const HashValue& hash_value) {
  os << "HashValue(" << toString(hash_value.low, 8, 16) << ','
     << toString(hash_value.high, 8, 16) << ')';
  return os;
}

HashValue *hash_values;

void init_hash_values() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  hash_values = new HashValue[13*64];
  for (int i=0; i<13*64; i++) {
    uint low = rand();
    low ^= rand() << 16;
    low ^= rand() >> 16;
    uint high = rand();
    high ^= rand() << 16;
    high ^= rand() >> 16;
    hash_values[i] = HashValue(low, high);
    //cerr << PPIECE_NAME[i>>6] << ' ' << POS_NAME[i&63] << ' ' << hash_values[i] << '\n';
  }
}
