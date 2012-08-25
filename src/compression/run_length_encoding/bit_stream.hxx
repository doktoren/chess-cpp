#ifndef _BIT_STREAM_
#define _BIT_STREAM_

#include <assert.h>
#include <string>
#include <fstream>

using namespace std;

typedef unsigned char uchar;
typedef unsigned int uint;

#include "../../util/help_functions.hxx"

struct n_bit {
  n_bit() : num_bits(0), value(0) {}
  n_bit(int num_bits, uint value) : num_bits(num_bits), value(value) {}
  int num_bits;
  uint value;

  bool operator==(const n_bit &nb) {
    return num_bits==nb.num_bits  &&  value==nb.value;
  }
};

string toString(n_bit nb);

static const int unary_length_specifier[7] =
{2, 4, 7, 11, 16, 22, 32};

//---------------------------------------------------------

// file input bit stream
class file_ibstream {
public:
  file_ibstream(uchar *data) {
    cerr << "Can't initialize file_ibstream(uchar *data)\n"; exit(1);
  }
  file_ibstream(string filename) : file(filename.c_str()), bitsleft(8) {
    assert(file);
    file.get((char &)ch);
  }
  void close() { file.close(); }

  bool getBit();
  uchar getByte();
  uint getNBit(int n);

  // Remember to call align, if it was called during output.
  void align() { if (bitsleft != 8) { file.get((char &)ch); bitsleft = 8; } }
  void aligned_getByte(uchar& byte) { file.get((char &)byte); }
  void aligned_getNBytes(uchar* mem, int count) { for (int i=0; i<count; i++) file.get((char &)(mem[i])); }
private:
  ifstream file;
  int bitsleft;
  uchar ch;
};

// array input bit stream
class array_ibstream {
public:
  array_ibstream(string filename) {
    assert(0); cout << "Can't initialize array_ibstream(string filename)\n"; exit(1);
  }
  array_ibstream(uchar *data) : data(data), bitsleft(8) {}

  bool getBit();
  uchar getByte();
  uint getNBit(int n);

  // Remember to call align, if it was called during output.
  void align() { if (bitsleft != 8) { data++; bitsleft = 8; } }
  void aligned_getByte(uchar& byte) { byte = *data++; }
  void aligned_getNBytes(uchar* mem, int count) { memcpy(mem, data, count); data += count; }
private:
  uchar *data;
  int bitsleft;
};

// advanced input stream
template<class INPUT_MODEL>
class ibstream : public INPUT_MODEL {
public:
  ibstream(uchar *data) : INPUT_MODEL(data) {}
  ibstream(string filename) : INPUT_MODEL(filename) {}

  bool getBit() { return INPUT_MODEL::getBit(); }
  uchar getByte() { return INPUT_MODEL::getByte(); }
  uint getNBit(int n) { return INPUT_MODEL::getNBit(n); }

  void getBit(bool &bit) { bit = getBit(); }
  void getByte(uchar &byte) { byte = getByte(); }
  void getNBit(n_bit &nb) { nb.value = getNBit(nb.num_bits); }

  uint getEliasNumber();

  uint getFixedIntervalNumber(uint succ_max); // returns in [0..succ_max[
  uint getNumber(uint expected_size);
  uint getArbitraryNumber();
  uint getArbitraryNumberMax(uint max);

  ibstream& operator>>(bool& bit) { bit = getBit(); }
  ibstream& operator>>(uchar& byte) { byte = getByte(); }
  ibstream& operator>>(uint& n) { n = getEliasNumber(); }
  ibstream& operator>>(n_bit& nb) { getNBit(nb); }
};

//---------------------------------------------------------

// file output bit stream
class file_obstream {
public:
  file_obstream(uchar *data) {
    assert(0); cerr << "Can't initialize file_obstream(uchar *data)\n"; exit(1);
  }
  file_obstream(string filename) : file(filename.c_str()), bitsleft(8), ch(0) {
    assert(file);
    if (!file) {
      cerr << "file_obstream(" << filename << "): Some error!\n";
      exit(1);
    }
  }
  void close(bool always_flush = true) {// Not sure it will work if set to false
    if (always_flush || bitsleft!=8) {
      bits_written_since_mark += bitsleft;
      ch <<= bitsleft;
      file.put((char &)ch);
    }
    file.flush();
    file.close();
  }

  void writeBit(bool bit);
  void writeByte(uchar byte);
  // the lower n bits of n is written, the others don't have to be cleared.
  void writeNBit(int n, uint value);
  void writeNBit(n_bit value) { writeNBit(value.num_bits, value.value); }

  // The (byte)aligned output routines are faster,
  // but may only be called after a call to align
  // or an aligned output operation (or immediately after construction).
  void align() {
    if (bitsleft != 8) {
      bits_written_since_mark += bitsleft;
      ch <<= bitsleft;
      file.put((char &)ch);
      bitsleft = 8;
      ch = 0;
    }
  }
  void aligned_writeByte(uchar byte) {
    bits_written_since_mark += 8;
    file.put((char &)byte);
  }
  void aligned_writeNBytes(uchar* mem, int count) {
    bits_written_since_mark += 8*count;
    for (int i=0; i<count; i++) file.put(mem[i]);
  }

  void mark(uint new_count = 0) { bits_written_since_mark = new_count; }
  uint num_bits_since_mark() { return bits_written_since_mark; }
private:
  ofstream file;
  int bitsleft;
  uchar ch;

  uint bits_written_since_mark;
};

// array output bit stream
class array_obstream {
public:
  array_obstream(string filename) {
    cout << "Can't initialize array_obstream(string filename)\n"; exit(1);
  }
  array_obstream(uchar *data) : data(data), bitsleft(8) {}

  // always_flush ignored
  void close(bool always_flush = false) {
    if (bitsleft!=8) {
      bits_written_since_mark += bitsleft;
      *data <<= bitsleft;
    }
  }

  void writeBit(bool bit);
  void writeByte(uchar byte);
  // the lower n bits of n is written, the others don't have to be cleared.
  void writeNBit(int n, uint value);
  void writeNBit(n_bit value) { writeNBit(value.num_bits, value.value); }

  // The (byte)aligned output routines are faster,
  // but may only be called after a call to align
  // or an aligned output operation (or immediately after construction).
  void align() {
    if (bitsleft != 8) {
      bits_written_since_mark += bitsleft;
      *data <<= bitsleft;
      ++data;
      bitsleft = 8;
      *data = 0;
    }
  }
  void aligned_writeByte(uchar byte) {
    bits_written_since_mark += 8;
    *data = byte;
    ++data;
  }
  void aligned_writeNBytes(uchar* mem, int count) {
    bits_written_since_mark += 8*count;
    memcpy(data, mem, count);
    data += count;
  }

  void mark(uint new_count = 0) { bits_written_since_mark = new_count; }
  uint num_bits_since_mark() { return bits_written_since_mark; }
private:
  uchar *data;
  int bitsleft;

  uint bits_written_since_mark;
};

// advanced output stream
template<class OUTPUT_MODEL>
class obstream : public OUTPUT_MODEL {
public:
  // file output bit stream constructor
  obstream(uchar *data) : OUTPUT_MODEL(data) {}
  obstream(string filename) : OUTPUT_MODEL(filename) {}

  void writeEliasNumber(uint n);
  void writeFixedIntervalNumber(uint succ_max, uint n); // n must be in [0..succ_max[
  void writeNumber(uint expected_size, uint number);
  void writeArbitraryNumber(uint n);
  void writeArbitraryNumberMax(uint max, uint n); // ie. n<=max

  obstream& operator<<(bool& bit) { OUTPUT_MODEL::writeBit(bit); }
  obstream& operator<<(uchar& byte) { OUTPUT_MODEL::writeByte(byte); }
  obstream& operator<<(uint& n) { OUTPUT_MODEL::writeEliasNumber(n); }
  obstream& operator<<(n_bit& nb) { OUTPUT_MODEL::writeNBit(nb); }
private:
  ofstream file;
  int bitsleft;
  uchar ch;

  int bits_written_since_mark;
};

void test_bit_streams(string tmp_file_name);

#include "bit_stream_templ.hxx"

//------------------------------------------------------
//-------------- Element streamers ---------------------
//------------------------------------------------------

template<class TYPE>
class DefaultElementStreamer {
public:
  template<class INPUT_MODEL>
  void readElement(ibstream<INPUT_MODEL>& in, TYPE& e) {
    uchar *p = reinterpret_cast<uchar *>(&e);
    for (int i=0; i<sizeof(TYPE); i++)
      in.getByte(p[i]);
  }

  template <class OUTPUT_MODEL>
  void writeElement(obstream<OUTPUT_MODEL>& out, TYPE& e) {
    const uchar *p = reinterpret_cast<const uchar *>(&e);
    for (int i=0; i<sizeof(TYPE); i++) {
      out.writeByte(p[i]);
    }
    return out;
  }

  string toString(TYPE &element) {
    const uchar *p = reinterpret_cast<const uchar *>(&element);
    char tmp[2*sizeof(TYPE) + 2];
    tmp[0] = '0';
    tmp[1] = 'x';
    for (int i=0; i<sizeof(TYPE); i++) {
      tmp[2*i+2] = HEX_CHAR[p[i]>>4];
      tmp[2*i+3] = HEX_CHAR[p[i]&0xF];
    }
    return string(tmp, 2*sizeof(TYPE) + 2);
  }
  
  int bit_size(TYPE &elem) { return 8*sizeof(TYPE); }

  // Used for huff tree reduction
  int average_bit_size() { return 8*sizeof(TYPE); }

  // En tilfældig ordensrelation mellem elementerne
  bool operator()(const TYPE &e1, const TYPE &e2) {
    const uchar *p1 = reinterpret_cast<const uchar *>(&e1);
    const uchar *p2 = reinterpret_cast<const uchar *>(&e2);
    for (int i=0; i<sizeof(TYPE); i++) {
      if (p1[i] != p2[i]) return p1[i]<p2[i];
    }
    return false;
  }
};

//------------

class Elias_int_streamer {//: public DefaultElementStreamer<uint> {
public:
  template<class INPUT_MODEL>
  void readElement(ibstream<INPUT_MODEL> &in, uint &n) {
    n = in.getEliasNumber();
  }

  template<class OUTPUT_MODEL>
  void writeElement(obstream<OUTPUT_MODEL> &out, const uint &n) {
    out.writeEliasNumber(n);
  }

  int bit_size(uint n) {
    ++n; // Be able to represent 0.
    if (n>3) {
      int f_log = floor_log(n);
      return 1 + 2*uchar_log[f_log] + f_log;
    } else {
      return 1 + 1 + (n >> 1);
    }
  }

  int average_bit_size() { 
    return 12;// More or less arbitrary
  }

  string toString(const uint &element) {
    char tmp[12];
    sprintf(tmp, "%d", element);
    return string(tmp);
  }

  bool operator()(const uint &n1, const uint &n2) { return n1<n2; }
};

class bit_streamer {//: public DefaultElementStreamer<bool> {
public:
  template<class INPUT_MODEL>
  bool readElement(ibstream<INPUT_MODEL> &in, bool &element) {
    in.getBit(element);
    return true;
  }
  template<class OUTPUT_MODEL>
  void writeElement(obstream<OUTPUT_MODEL> &out, const bool &element) {
    out.writeBit(element);
  }
  string toString(const bool &element) {
    static const string TF[2] = {"F","T"};
    return TF[element];
  }
  int bit_size(bool &elem) { return 1; }
  int average_bit_size() { return 1; }
  bool operator()(const bool &e1, const bool &e2) const { return e1<e2; }
};

#endif
