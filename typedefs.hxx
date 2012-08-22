#ifndef _TYPEDEFS_
#define _TYPEDEFS_


using namespace std;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#include "piece_pos.hxx"

template<class TYPE1, class TYPE2=TYPE1, class TYPE3=TYPE2>
class triple {
public:
  triple() : first(), second(), third() {}

  triple(TYPE1 first, TYPE2 second, TYPE3 third) :
    first(first), second(second), third(third) {}

  triple& operator=(const triple &t) {
    first = t.first;
    second = t.second;
    third = t.third;
    return *this;
  }
  
  TYPE1 first;
  TYPE2 second;
  TYPE3 third;
};

#include "streams.hxx"


#ifdef NDEBUG
#define DEBUG false
#define my_assert(x) x
#else
#include <assert.h>
#include <stdlib.h>
#include <iostream>
inline void __exit(int n) { cerr.flush(); cout.flush(); assert(!n); exit(n); }
#define exit(n) __exit(n)
#define DEBUG true
#define my_assert(x) assert(x)
#endif

#include "experimenting.hxx"// Just to make sure it is included everywhere

struct BDD_Index {
  BDD_Index() {
    u.raw[0] = 0;
#if MAX_MEN >= 5
    u.raw[1] = 0;
#endif
  }

  Position &operator[](int index) { return u.positions[index];}

  int index() {
    return u.positions[0] | (u.positions[1] << 6) |
      (u.positions[2] << 12) | (u.positions[3] << 18)
#if MAX_MEN >= 5
      | (u.positions[4] << 24)
#endif
      ;
  }


  BDD_Index &operator==(const BDD_Index &bddi) {
    u.raw[0] = bddi.u.raw[0];
#if MAX_MEN >= 5
    u.raw[1] = bddi.u.raw[1];
#endif
    return *this;
  }
  
private:
  union {
    Position positions[MAX_MEN];
    uint raw[(MAX_MEN+3)/4];
  } u;
};


#endif
