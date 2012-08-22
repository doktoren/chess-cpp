#include "help_functions.hxx"

#include <assert.h>
#include <fcntl.h>

#include "board_3.hxx"//Just because of some stupid constants (INF, etc.)

const char HEX_CHAR[16] =
{'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

const int uchar_log[256] = {
  0 ,0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

const int bit_count[256] = {
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,

  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,

  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,

  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

const uchar uchar_hash[256] = {
  0x56, 0x3C, 0x86, 0x87, 0xC5, 0xCF, 0x44, 0x8E, 0xF2, 0xCC, 0x8B, 0x7A, 0x1F, 0xEF, 0x9E, 0x14, 
  0x07, 0x2C, 0x85, 0x7F, 0x31, 0x9E, 0xDE, 0x38, 0xF7, 0x55, 0x92, 0x26, 0x23, 0x79, 0x4C, 0x79, 
  0xB6, 0xD3, 0x00, 0x7B, 0xA3, 0x45, 0x0A, 0x95, 0x12, 0x95, 0x10, 0x31, 0x85, 0xAE, 0x45, 0x8D, 
  0xDA, 0xCB, 0x0C, 0x0C, 0x69, 0xEB, 0x45, 0x61, 0x40, 0xD8, 0x87, 0x64, 0x52, 0xD3, 0xDD, 0x08, 
  0xA7, 0xDE, 0x83, 0x4A, 0x24, 0x8E, 0xDF, 0x36, 0x23, 0xF0, 0x67, 0xA9, 0x9E, 0xAD, 0x36, 0x79, 
  0x78, 0x43, 0x85, 0xE2, 0x2E, 0xCB, 0x43, 0x6F, 0xA3, 0xCA, 0xD3, 0xF5, 0x9E, 0xB1, 0xFD, 0x45, 
  0x90, 0x81, 0x8F, 0xB4, 0x0F, 0x6F, 0xEA, 0x33, 0x5F, 0x52, 0xDD, 0xFE, 0x00, 0x13, 0x77, 0x79, 
  0x57, 0xFD, 0x5B, 0x86, 0xC8, 0x9F, 0xF5, 0x6B, 0x6A, 0xC9, 0x60, 0x09, 0x7B, 0x5E, 0x4F, 0x0B, 
  0xE0, 0xDE, 0xBF, 0xF0, 0x4E, 0xAA, 0x23, 0xAE, 0xFC, 0x00, 0xAC, 0xFD, 0x14, 0x24, 0x76, 0x6B, 
  0x21, 0xD2, 0xF1, 0xEA, 0x71, 0xE7, 0x56, 0xDC, 0xB1, 0xB7, 0xE5, 0x2C, 0x15, 0x34, 0x37, 0xF6, 
  0x13, 0xF7, 0xE6, 0x62, 0xA1, 0x09, 0x10, 0x9D, 0x0A, 0xBD, 0x9A, 0x1F, 0xE1, 0x11, 0x8A, 0x03, 
  0xE3, 0x7C, 0xEE, 0x55, 0x64, 0x44, 0x31, 0x15, 0xFB, 0x16, 0x42, 0x11, 0x4B, 0x79, 0x07, 0x5F, 
  0x70, 0xEE, 0xC1, 0x11, 0xF7, 0xD2, 0xAF, 0x02, 0x8F, 0x4A, 0x21, 0x71, 0x5B, 0xAC, 0x74, 0x3E, 
  0x29, 0x63, 0x93, 0x8E, 0xA7, 0xC4, 0xA3, 0xA3, 0xDB, 0xE5, 0xB4, 0x27, 0x5F, 0xBC, 0x86, 0xD0, 
  0xAA, 0x48, 0xE2, 0xA2, 0x1A, 0x91, 0xA5, 0xAA, 0xDB, 0xC7, 0x1B, 0x37, 0x73, 0x90, 0x75, 0x9D, 
  0xF3, 0x09, 0x2B, 0x9A, 0xCE, 0xCF, 0x3E, 0xA9, 0xB4, 0xF3, 0xD1, 0x14, 0xAF, 0x58, 0xE4, 0x5A};

string toString(uint number, int base) {
  assert(2 <= base  &&  base <= 16);
  char tmp[36];
  int index = 36;
  tmp[--index] = 0;
  tmp[--index] = HEX_CHAR[number%base];
  while (number /= base) {
    tmp[--index] = HEX_CHAR[number%base];
  }
  return string(&(tmp[index]));
}

string toString(uint number, int num_digits, int base) {
  assert(2<=base && base<=16  &&  1<=num_digits && num_digits<=32);
  char tmp[36];
  tmp[num_digits] = 0;
  int i = num_digits;
  while (i) {
    tmp[--i] = HEX_CHAR[number%base];
    number /= base;
  }
  assert(!number);
  return string(tmp);
}

uint toInt(string s, int base) {
  uint result = 0;
  if (base > 10) {
    for (unsigned int i=0; i<s.length(); i++) {
      result *= base;
      result += (s[i] > '9') ? 10 + (uint)((s[i] | 32)-'a') : (uint)(s[i]-'0');
    }
  } else {
    for (unsigned int i=0; i<s.length(); i++) {
      result *= base;
      result += (uint)(s[i]-'0');
    }
  }
  return result;
}


string signedToString(int number, int num_digits, int base) {
  assert(2<=base && base<=16  &&  0<=num_digits && num_digits<=32);

  if (!number) {
    char tmp[36];
    if (num_digits) {
      tmp[num_digits] = 0;
      tmp[--num_digits] = '0';
      while(num_digits) tmp[--num_digits] = ' ';
    } else {
      tmp[0] = '0';
      tmp[1] = 0;
    }
    return string(tmp);
  }

  bool negative = number<0;
  if (negative) number = -number;
  char tmp[36];
  
  if (num_digits) {
    tmp[num_digits] = 0;
    int i = num_digits;
    while (i) {
      if (number==0) {
	if (negative) {
	  tmp[--i] = '-';
	  negative = false;
	} else {
	  tmp[--i] = ' ';
	}
      } else {
	tmp[--i] = HEX_CHAR[number%base];
	number /= base;
      }
    }
  
    assert(!number);
    assert(!negative);
    return string(tmp);

  } else {

    int index = 36;
    tmp[--index] = 0;
    tmp[--index] = number%base + '0';
    while (number /= base) {
      tmp[--index] = number%base + '0';
    }
    if (negative) tmp[--index] = '-';
    return string(&(tmp[index]));
  }
}


string doubleToString(double number, int digits_before, int digits_after) {
  string result(digits_before + digits_after + 1, ' ');
  
  {
    int tmp = (int)number;
    for (int i=0; i<digits_before; i++) {
      result[(digits_before-1) - i] = (tmp%10) + '0';
      tmp /= 10;
    }
  } 

  result[digits_before] = '.';

  {
    double tmp = (number - (int)number);
    for (int i=0; i<digits_after; i++) {
      tmp *= 10.0;
      result[digits_before + 1 + i] = (int)tmp + '0';
      tmp -= (int)tmp;
    }
  }

  return result;
}


bool file_exists(string filename) {
  int fd = open(filename.c_str(), O_RDONLY, 0);
  if (fd == -1) return false;
  close(fd);
  return true;
}


string game_theoretical_value_to_string(int value) {
  if (value == INF) return "infinite";
  if (value == -INF) return "-infinite";

  if (GUARANTEED_LOSS > value) {
    assert(((WIN+value)&1) == 0);
    return string("-M") + toString((WIN+value)>>1);
  } else if (GUARANTEED_WIN < value) {
    assert(((WIN-value)&1) == 1);
    return string("M") + toString((WIN-value+1)>>1);
  }

  if (value == ORACLE_WIN)  return "WIN";
  if (value == ORACLE_LOSS) return "LOSS";

  if (value<0) return "-"+toString(-value);
  else return toString(value);
}
