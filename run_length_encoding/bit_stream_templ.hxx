//-------- ADVANCED INPUT STREAM ------------

template<class INPUT_MODEL>
uint ibstream<INPUT_MODEL>::getEliasNumber() {
  if (getBit()) {
    int prefix = 1;
    bool b1,b2;
    do {
      b1 = getBit();
      b2 = getBit();
      prefix += prefix + b1;
    } while (b1 == b2);
    return (1<<prefix)-1 + getNBit(prefix);
  } else {
    if (getBit()) {
      // Number is 2-1 or 3-1
      return 1+getBit();
    } else {
      return 0;
    }
  }
}

template<class OUTPUT_MODEL>
uint ibstream<OUTPUT_MODEL>::getFixedIntervalNumber(uint succ_max) {
  if (succ_max < 2) return 0;
  uint tmp = ceil_log(succ_max);
  uint result = getNBit(tmp-1);
  tmp = (1<<tmp)-succ_max;
  if (result < tmp) return result;
  return ((result << 1) | getBit()) - tmp;
}

template<class OUTPUT_MODEL>
uint ibstream<OUTPUT_MODEL>::getNumber(uint expected_size) {
  int num_bits = ceil_log(expected_size);
  while (getBit()) ++num_bits;
  
  return getNBit(num_bits);
}

template<class OUTPUT_MODEL>
uint ibstream<OUTPUT_MODEL>::getArbitraryNumber() {
  int i=0;
  while (getBit()) ++i;
  return getNBit(unary_length_specifier[i]);
}

template<class OUTPUT_MODEL>
uint ibstream<OUTPUT_MODEL>::getArbitraryNumberMax(uint max) {
  int max_bits = floor_log(max)+1;
  int i=0;
  while (unary_length_specifier[i] < max_bits  &&  getBit()) ++i;

  int result;
  if (unary_length_specifier[i] < max_bits) {
    result = getNBit(unary_length_specifier[i]);
  } else {
    result = getFixedIntervalNumber(max+1);
  }
  // cerr << "get( " << max << " , " << result << " )\n";
  return result;
}


//-------- ADVANCED OUTPUT STREAM ------------

template<class OUTPUT_MODEL>
void obstream<OUTPUT_MODEL>::writeEliasNumber(uint n) {
  ++n; // Be able to represent 0.
  writeBit(n>3);
  if (n>3) {
    int f_log = floor_log(n);

    for (int i=uchar_log[f_log]-1; i; i--) {
      bool tmp = f_log & (1<<i);
      writeBit(tmp);
      writeBit(tmp);
    }
    
    writeBit(f_log & 1);
    writeBit(!(f_log & 1));

    writeNBit(f_log, n);
  } else {
    writeBit(n!=1);
    if (n!=1) writeBit(n&1);
  }
}

template<class OUTPUT_MODEL>
void obstream<OUTPUT_MODEL>::writeFixedIntervalNumber(uint succ_max, uint n) {
  if (succ_max < 2) return;
  int c_log = ceil_log(succ_max);
  uint tmp = (1<<c_log)-succ_max;
  if (n < tmp) writeNBit(c_log-1, n);
  else writeNBit(c_log, n+tmp);
}

template<class OUTPUT_MODEL>
void obstream<OUTPUT_MODEL>::writeNumber(uint expected_size, uint number) {
  int num_bits = ceil_log(expected_size);
  
  while (number >> num_bits) {//1<<num_bits <= number) {
    writeBit(1);
    ++num_bits;
  }
  writeBit(0);
  
  writeNBit(num_bits, number);
}

template<class OUTPUT_MODEL>
void obstream<OUTPUT_MODEL>::writeArbitraryNumber(uint n) {
  int c_log = 1+floor_log(n);
  int i=-1;
  while (unary_length_specifier[++i] < c_log)
    writeBit(1);
  writeBit(0);
  writeNBit(unary_length_specifier[i], n);
}

template<class OUTPUT_MODEL>
void obstream<OUTPUT_MODEL>::writeArbitraryNumberMax(uint max, uint n) {
  // cerr << "write( " << max << " , " << n << " )\n";

  int max_bits = 1+floor_log(max);

  int c_log = 1+floor_log(n);
  int i=-1;
  while (unary_length_specifier[++i] < c_log)
    writeBit(1);

  if (unary_length_specifier[i] < max_bits) {
    writeBit(0);
    writeNBit(unary_length_specifier[i], n);
  } else {
    writeFixedIntervalNumber(max+1, n);
    //writeNBit(max_bits, n);
  }
}
