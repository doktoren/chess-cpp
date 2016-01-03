#ifndef _BDT_COMPRESSION_
#define _BDT_COMPRESSION_

typedef unsigned int uint;

#include <vector>

// yalla returns a permutation of [0..shuffle_size[, p, such that
// p[0..barrier[ is a permutation of [0..barrier[  and
// p[barrier..shuffle_size[  is a permutation of  p[barrier..shuffle_size[
// (barrier == 0  has the effect of no barrier)
// The goal of the permutation is that after the entries in pairs
// are permuted by
//     pairs[i] = yalla_result[pairs[i]]
// a lot of the pairs will have the property
//     pairs[2*i]+1 = pairs[2*i+1]
std::vector<uint> yalla(int shuffle_size, uint *pairs, int num_pairs, int barrier = 0);

void test_yalla();

#endif
