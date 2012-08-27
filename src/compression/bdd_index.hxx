#ifndef _BDD_INDEX_
#define _BDD_INDEX_

#include "../experimenting.hxx"
#include "../piece_pos.hxx"

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
