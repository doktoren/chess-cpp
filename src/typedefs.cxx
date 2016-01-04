#include <assert.h>
#include <stdint.h>

#include "typedefs.hxx"

enum
{
    O32_LITTLE_ENDIAN = 0x03020100ul,
    O32_BIG_ENDIAN = 0x00010203ul,
    O32_PDP_ENDIAN = 0x01000302ul
};

#ifndef NDEBUG
static const union { uint8_t bytes[4]; uint32_t value; } o32_host_order = { { 0, 1, 2, 3 } };
#endif

void run_endian_test() {
  assert(o32_host_order.value == O32_LITTLE_ENDIAN);
}
