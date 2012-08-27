#include <iostream>

#include "bit_stream.hxx"

const uint8_t ONE_PATTERN[9] = {0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF};

string toString(n_bit nb) {
  string result;
  while (nb.num_bits) {
    --nb.num_bits;
    result += (nb.value & (1 << nb.num_bits)) ? '1' : '0';
  }
  return result;
}

//------ FILE INPUT BYTE STREAM ---------

bool file_ibstream::getBit() {
  bool result = (ch >> --bitsleft) & 1;
  if (!bitsleft) {
    file.get((char &)ch);
    bitsleft = 8;
  }
  return result;
}

uint8_t file_ibstream::getByte() {
  uint8_t prev_char = ch;
  file.get((char &)ch);
  return (prev_char << (8-bitsleft)) | (ch >> bitsleft);
}

uint file_ibstream::getNBit(int n) {
  assert(n <= 32);
  uint result = 0;
  if (n >= bitsleft) {
    do {
      n -= bitsleft;
      result |= (ch & ONE_PATTERN[bitsleft]) << n;

      file.get((char &)ch);
      bitsleft = 8;
    } while (n >= bitsleft);
    result |= ch >> (bitsleft-n);
    bitsleft -= n;
    return result;
  }

  result |= (ch & ONE_PATTERN[bitsleft]) >> (bitsleft-n);
  bitsleft -= n;
  return result;
}

//------ ARRAY INPUT BYTE STREAM --------

bool array_ibstream::getBit() {
  bool result = (*data >> --bitsleft) & 1;
  if (!bitsleft) {
    data++;
    bitsleft = 8;
  }
  return result;
}

uint8_t array_ibstream::getByte() {
  uint8_t prev_char = *data;
  ++data;
  return (prev_char << (8-bitsleft)) | (*data >> bitsleft);
}

uint array_ibstream::getNBit(int n) {
  assert(n <= 32);
  uint result = 0;
  if (n >= bitsleft) {
    do {
      n -= bitsleft;
      result |= (*data & ONE_PATTERN[bitsleft]) << n;

      ++data;
      bitsleft = 8;
    } while (n >= bitsleft);
    result |= *data >> (bitsleft-n);
    bitsleft -= n;
    return result;
  }

  result |= (*data & ONE_PATTERN[bitsleft]) >> (bitsleft-n);
  bitsleft -= n;
  return result;
}

// #########################################################################
// #########################################################################

//-------- FILE OUTPUT BYTE STREAM ------------

void file_obstream::writeBit(bool bit) {
  ++bits_written_since_mark;

  ch <<= 1;
  ch |= bit;
  if (!--bitsleft) {
    file.put((char &)ch);
    bitsleft = 8;
    ch = 0;
  }
}

void file_obstream::writeByte(uint8_t byte) {
  bits_written_since_mark += 8;

  ch = (ch << bitsleft) | (byte >> (8-bitsleft));
  file.put(ch);
  ch = byte;
}

void file_obstream::writeNBit(int n, uint value) {
  bits_written_since_mark += n;

  while (bitsleft <= n) {
    ch <<= bitsleft;
    n -= bitsleft;
    ch |= (value >> n) & ONE_PATTERN[bitsleft];

    file.put((char &)ch);
    bitsleft = 8;
    ch = 0;
  }
  ch <<= n;
  ch |= value & ONE_PATTERN[n];
  bitsleft -= n;
}

//-------- ARRAY OUTPUT BYTE STREAM ------------

void array_obstream::writeBit(bool bit) {
  ++bits_written_since_mark;

  *data <<= 1;
  *data |= bit;
  if (!--bitsleft) {
    ++data;
    bitsleft = 8;
    *data = 0;
  }
}

void array_obstream::writeByte(uint8_t byte) {
  bits_written_since_mark += 8;

  *data = (*data << bitsleft) | (byte >> (8-bitsleft));
  ++data;
  *data = byte;
}

void array_obstream::writeNBit(int n, uint value) {
  bits_written_since_mark += n;

  while (bitsleft <= n) {
    *data <<= bitsleft;
    n -= bitsleft;
    *data |= (value >> n) & ONE_PATTERN[bitsleft];

    ++data;
    bitsleft = 8;
    *data = 0;
  }
  *data <<= n;
  *data |= value & ONE_PATTERN[n];
  bitsleft -= n;
}

// #########################################################################
// #########################################################################

#define cmp(a, b, c) if (!((a)==(b))) { cerr << "Error: test_bit_streams " << c << "\n"; assert(0); exit(1); }
void test_bit_streams(string tmp_file_name) {
  cerr << "Testing array_obstream and array_ibstream...\n";
  {
    uint8_t data[9999];
    array_obstream out(data);

    bool r[999];
    for (int i=0; i<999; i++) {
      r[i] = (rand()>>12) & 1;
      out.writeBit(r[i]);
    }

    uint8_t b[999];
    for (int i=0; i<999; i++) {
      b[i] = rand()&0xFF;
      out.writeByte(b[i]);
    }

    n_bit nb[32][32];
    for (int i=0; i<32; i++)
      for (int j=0; j<32; j++) {
        nb[i][j] = n_bit(i, rand() & ((1<<i)-1));
        out.writeNBit(nb[i][j]);
      }

    out.close();

    array_ibstream in(data);

    for (int i=0; i<999; i++) {
      cmp(r[i], in.getBit(), "bit " << i);
    }

    for (int i=0; i<999; i++) {
      cmp(b[i], in.getByte(), "byte " << i);
    }

    for (int i=0; i<32; i++)
      for (int j=0; j<32; j++) {
        cmp(nb[i][j].value, in.getNBit(i), "nbit " << i << " " << j);
      }
  }

  cerr << "Testing file_obstream and file_ibstream...\n";
  {
    file_obstream out(tmp_file_name);

    bool r[999];
    for (int i=0; i<999; i++) {
      r[i] = (rand()>>12) & 1;
      out.writeBit(r[i]);
    }

    uint8_t b[999];
    for (int i=0; i<999; i++) {
      b[i] = rand()&0xFF;
      out.writeByte(b[i]);
    }

    n_bit nb[32][32];
    for (int i=0; i<32; i++)
      for (int j=0; j<32; j++) {
        nb[i][j] = n_bit(i, rand() & ((1<<i)-1));
        out.writeNBit(nb[i][j]);
      }

    out.close();

    file_ibstream in(tmp_file_name);

    for (int i=0; i<999; i++) {
      cmp(r[i], in.getBit(), "bit " << i);
    }

    for (int i=0; i<999; i++) {
      cmp(b[i], in.getByte(), "byte " << i);
    }

    for (int i=0; i<32; i++)
      for (int j=0; j<32; j++) {
        cmp(nb[i][j].value, in.getNBit(i), "nbit " << i << " " << j);
      }
  }

  cerr << "Testing obstream<OUTPUT_MODEL> and ibstream<INPUT_MODEL>...\n";
  {
    uint8_t data[99999];
    obstream<array_obstream> out(data);

    uint elias[999];
    for (int i=0; i<999; i++) {
      elias[i] = (((uint)rand() << 16) ^ rand()) & ((1<<((rand()&31)+1)) - 1);
      out.writeEliasNumber(elias[i]);
    }

    uint I[10] = {3,4,7,123,134123,123413,433245,565,234,324};
    uint fi[10][99];
    for (int i=0; i<10; i++)
      for (int j=0; j<99; j++) {
        fi[i][j] = rand()%I[i];
        out.writeFixedIntervalNumber(I[i], fi[i][j]);
      }

    uint exp1[999], exp2[999];
    for (int i=0; i<999; i++) {
      int bl1 = (rand()%20)+5;
      int bl2 = bl1 + (rand()%11) - 5;
      exp1[i] = (((uint)rand() << 16) ^ rand()) & ((1<<bl1) - 1);
      exp2[i] = (((uint)rand() << 16) ^ rand()) & ((1<<bl2) - 1);

      out.writeNumber(exp1[i], exp2[i]);
    }

    uint arb[999];
    for (int i=0; i<999; i++) {
      arb[i] = (((uint)rand() << 16) ^ rand()) & ((1<<((rand()&31)+1)) - 1);
      out.writeArbitraryNumber(arb[i]);
    }

    uint arb_max[10][99];
    for (int i=0; i<10; i++)
      for (int j=0; j<99; j++) {
        arb_max[i][j] = rand()%I[i];
        out.writeArbitraryNumberMax(I[i]-1, arb_max[i][j]);
      }

    out.close();

    ibstream<array_ibstream> in(data);

    for (int i=0; i<999; i++) {
      cmp(elias[i], in.getEliasNumber(), "elias " << i);
    }

    for (int i=0; i<10; i++)
      for (int j=0; j<99; j++) {
        cmp(fi[i][j], in.getFixedIntervalNumber(I[i]), "fixed interval number " << i);
      }

    for (int i=0; i<999; i++) {
      cmp(exp2[i], in.getNumber(exp1[i]), "expected number " << i);
    }

    for (int i=0; i<999; i++) {
      cmp(arb[i], in.getArbitraryNumber(), "arbitrary number " << i);
    }

    for (int i=0; i<10; i++)
      for (int j=0; j<99; j++) {
        cmp(arb_max[i][j], in.getArbitraryNumberMax(I[i]-1), "arb. number max " << i);
      }
  }


  cerr << "test_bit_streams: No errors!\n";
}
