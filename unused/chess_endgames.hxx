#ifndef _CHESS_ENDGAMES_
#define _CHESS_ENDGAMES_

#include <string>
#include <vector>

#include "../typedefs.hxx"

// if ALLOW_5_MEN_ENDGAME is not defined, then less memory will be required
#define ALLOW_5_MEN_ENDGAME

// Enumeration of squares:
//      a  b  c  d  e  f  g  h
//   +-------------------------+
// 8 | 56 57 58 59 60 61 62 63 | 8
// 7 | 48 49 50 51 52 53 54 55 | 7
// 6 | 40 41 42 43 44 45 46 47 | 6
// 5 | 32 33 34 35 36 37 38 39 | 5
// 4 | 24 25 26 27 28 29 30 31 | 4
// 3 | 16 17 18 19 20 21 22 23 | 3
// 2 |  8  9 10 11 12 13 14 15 | 2
// 1 |  0  1  2  3  4  5  6  7 | 1
//   +-------------------------+
//      a  b  c  d  e  f  g  h
//
// Values of white pieces:
//   pawn=1, knight=2, bishop=3, rook=4, queen=5, king=6
// Values of black pieces:
//   pawn=7, knight=8, bishop=9, rook=10, queen=11, king=12

// May not be called before main is executed (problems with creation order).
void init_endgames();

// How to load the endgame KRK:
//     load_endgame("KRK_wtm.bdd"); load_endgame("KRK_btm.bdd");
// If you only want the endgame for one side:
//     load_endgame("KRK_wtm.bdd");
// If it is a symmetric endgame, then there only is the wtm file,
// but this is also used to index positions with btm.
bool load_endgame(std::string filename);
void clear_endgame(std::string filename);

void clear_all_endgames();

// index_endgame may change piece_list
char index_endgame(std::vector<PiecePos>& piece_list, bool black_to_move);
// If -123 <= result <= 0 then side-to-move will lose in -result moves
// If 0 < result < 127 then side-to-move will win in result moves
// If result = -124 then side-to-move will win
// If result = -125 then its a draw
// If result = -126 then side-to-move will lose
// If result = -127 then the game theoretical value is unknown
//             (which means that the appropriate bdd hasn't been loaded yet)
// If result = -128 then its an illegal entry
//             (eg. if 2 pieces have the same coordinate. WARNING: when
//              index_endgame_table is given an illegal piece_list the result
//              will generelly be undefined!)

std::string endgame_value_to_string(char v);

#endif
