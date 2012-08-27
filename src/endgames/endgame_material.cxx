#include "endgame_material.hxx"

inline endgame_material_t make(uint8_t b, uint8_t a, uint16_t endgame_hashing) {
  endgame_material_t result;
  result.individual.endgame_material_b = b;
  result.individual.endgame_material_a = a;
  result.individual.endgame_hashing = endgame_hashing;
  return result;
}

const endgame_material_t ENDGAME_MATERIAL_HASHING_CONSTANTS[13][2] =
{   {make(0, 0, 0), make(0, 0, 0)},
    {make(0x01, 0x01, DB_WPAWN_VALUE),   make(0x01, 0x01, DB_WPAWN_VALUE)},
    {make(0x00, 0xE1, DB_WKNIGHT_VALUE), make(0x00, 0xE1, DB_WKNIGHT_VALUE)},
    {make(0x00, 0x21, DB_WBISHOP_VALUE), make(0x01, 0x00, DB_WBISHOP_VALUE)},
    {make(0x01, 0x01, DB_WROOK_VALUE),   make(0x01, 0x01, DB_WROOK_VALUE)},
    {make(0x01, 0x01, DB_WQUEEN_VALUE),  make(0x01, 0x01, DB_WQUEEN_VALUE)},
    {make(0x00, 0x00, DB_WKING_VALUE),   make(0x00, 0x00, DB_WKING_VALUE)},
    {make(0x01, 0x01, DB_BPAWN_VALUE),   make(0x01, 0x01, DB_BPAWN_VALUE)},
    {make(0x00, 0xE1, DB_BKNIGHT_VALUE), make(0x00, 0xE1, DB_BKNIGHT_VALUE)},
    {make(0x00, 0x21, DB_BBISHOP_VALUE), make(0x01, 0x00, DB_BBISHOP_VALUE)},
    {make(0x01, 0x01, DB_BROOK_VALUE),   make(0x01, 0x01, DB_BROOK_VALUE)},
    {make(0x01, 0x01, DB_BQUEEN_VALUE),  make(0x01, 0x01, DB_BQUEEN_VALUE)},
    {make(0x00, 0x00, DB_BKING_VALUE),   make(0x00, 0x00, DB_BKING_VALUE)}
};

const uint_fast16_t ENDGAME_HASHING_CONSTANTS[13] =
{   0,
    DB_WPAWN_VALUE, DB_WKNIGHT_VALUE, DB_WBISHOP_VALUE,
    DB_WROOK_VALUE, DB_WQUEEN_VALUE, DB_WKING_VALUE,
    DB_BPAWN_VALUE, DB_BKNIGHT_VALUE, DB_BBISHOP_VALUE,
    DB_BROOK_VALUE, DB_BQUEEN_VALUE, DB_BKING_VALUE
};
