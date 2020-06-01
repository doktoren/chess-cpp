#include "endgame_material.hxx"

inline uint32_t make(uint32_t a, uint32_t b, uint32_t endgame_hashing) {
  return (a << 24) | (b << 16) | endgame_hashing;
}

const uint32_t ENDGAME_MATERIAL_HASHING_CONSTANTS[13][2] =
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
