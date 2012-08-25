#include "bit_stream.hxx"
#include "huffman_codec.hxx"

class BinaryRunLengthEncoder {
public:
  BinaryRunLengthEncoder(obstream *out) : out(out), init(false) {}

  void addBit(bool bit) {
    if (!init) {
      init = true;
      out->writeBit(bit);
      last_bit = bit;
      count = 1;
      return;
    }
    if (bit==last_bit) {
      ++count;
    } else {
      //cout << count << "\n";
      out->writeArbitraryNumber(count);
      last_bit = bit;
      count = 1;
    }
  }

  void finalize() {
    if (init) {
      out->writeArbitraryNumber(count);
    } else {
      out->writeBit(0);// 0 or 1 - doesn't matter
    }
  }

private:
  int count;
  bool init, last_bit;
  obstream *out;
};

class BinaryRunLengthDecoder {
public:
  BinaryRunLengthDecoder(ibstream *in) : in(in) {
    last_bit = !in->getBit();
    count = 0;
  }
  
  bool getBit() {
    if (!count) {
      count = in->getArbitraryNumber();
      last_bit = !last_bit;
    }
    --count;
    return last_bit;
  }

private:
  int count;
  bool last_bit;
  ibstream *in;
};


//------------------------------------

class BinaryRunLengthEncoder2 {
public:
  BinaryRunLengthEncoder2(obstream *out) : out(out), init(false), huff() {}
  
  void addBit(bool bit) {
    if (!init) {
      init = true;
      first_bit = bit;
      last_bit = bit;
      count = 1;
      return;
    }
    if (bit==last_bit) {
      ++count;
    } else {
      huff.addSymbol((uint)count);
      last_bit = bit;
      count = 1;
    }
  }
  
  // nothing will be output before finalize
  void finalize() {
    if (init) {
      out->writeBit(first_bit);
      huff.addSymbol((uint)count);
      huff.outputAll(out);
    } else {
      out->writeBit(0);// 0 or 1 - doesn't matter, won't be used
    }
  }

private:
  int count;
  bool init, first_bit, last_bit;
  obstream *out;

  HuffmanCodec2<uint, arb_int_streamer> huff;
};

class BinaryRunLengthDecoder2 {
public:
  BinaryRunLengthDecoder2(ibstream *in) : last_bit(!in->getBit()), in(in), huff(in) {
    count = 0;
  }
  
  bool getBit() {
    if (!count) {
      count = huff.decode(in);
      last_bit = !last_bit;
    }
    --count;
    return last_bit;
  }
  
private:
  int count;
  bool last_bit;
  ibstream *in;

  Huffman<uint, arb_int_streamer> huff;
};


//------------------------------------

class BinaryRunLengthEncoder3 {
public:
  BinaryRunLengthEncoder3(obstream *out) :
    out(out), init(false), huff0(), huff1() {}
  
  void addBit(bool bit) {
    if (!init) {
      init = true;
      first_bit = bit;
      last_bit = bit;
      count = 1;
      return;
    }
    if (bit==last_bit) {
      ++count;
    } else {
      if (last_bit) huff1.addSymbol((uint)count);
      else huff0.addSymbol((uint)count);
      last_bit = bit;
      count = 1;
    }
  }
  
  // nothing will be output before finalize
  void finalize() {
    if (init) {
      out->writeBit(first_bit);
      if (last_bit) huff1.addSymbol((uint)count);
      else huff0.addSymbol((uint)count);
      huff0.outputAll(out);
      huff1.outputAll(out);
    } else {
      out->writeBit(0);// 0 or 1 - doesn't matter, won't be used
    }
  }

private:
  int count;
  bool init, first_bit, last_bit;
  obstream *out;

  HuffmanCodec2<uint, arb_int_streamer> huff0;
  HuffmanCodec2<uint, arb_int_streamer> huff1;
};

class BinaryRunLengthDecoder3 {
public:
  BinaryRunLengthDecoder3(ibstream *in) :
    last_bit(!in->getBit()), in(in), huff0(in), huff1(in), count(0) {}
  
  bool getBit() {
    if (!count) {
      last_bit = !last_bit;
      if (last_bit) count = huff1.decode(in);
      else count = huff0.decode(in);
    }
    --count;
    return last_bit;
  }
  
private:
  int count;
  bool last_bit;
  ibstream *in;

  Huffman<uint, arb_int_streamer> huff0;
  Huffman<uint, arb_int_streamer> huff1;
};
