#ifndef _TYPEDEFS_
#define _TYPEDEFS_

typedef unsigned int uint;

/**
 * A few places in the implementation the ugly assumption has been made
 * that the machine is little-endian.
 * This function will trigger an assertion if the machine has wrong endianness.
 * TODO: This should preferable be a compile time check.
 */
void run_endian_test();

/**
 * This program only work on a ...-endian machine.
 * Produce an error if that is not the case.
 */

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


#endif
