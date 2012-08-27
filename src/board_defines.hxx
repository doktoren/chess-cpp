#ifndef _BOARD_DEFINES_
#define _BOARD_DEFINES_

#define PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS false

//################################

#define WHITE 0
#define BLACK 1
#define NEITHER_COLOR 2

//################################

#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

#define WPAWN 1
#define WKNIGHT 2
#define WBISHOP 3
#define WROOK 4
#define WQUEEN 5
#define WKING 6
#define BPAWN 7
#define BKNIGHT 8
#define BBISHOP 9
#define BROOK 10
#define BQUEEN 11
#define BKING 12

// A piece value of 13 is used for a fictive colorless pawn with move direction
// from file a to h. Similarly a piece value of 14 denotes a pawn going the
// opposite way.
#define FILE_A_TO_H_PAWN 13
#define FILE_H_TO_A_PAWN 14

#define WHITE_PAWN 1
#define WHITE_KNIGHT 2
#define WHITE_BISHOP 3
#define WHITE_ROOK 4
#define WHITE_QUEEN 5
#define WHITE_KING 6
#define BLACK_PAWN 7
#define BLACK_KNIGHT 8
#define BLACK_BISHOP 9
#define BLACK_ROOK 10
#define BLACK_QUEEN 11
#define BLACK_KING 12

//################################

#define WHITE_LONG_CASTLING (1<<4)
#define WHITE_SHORT_CASTLING (2<<4)
#define BLACK_LONG_CASTLING (4<<4)
#define BLACK_SHORT_CASTLING (8<<4)

#define WHITE_CASTLING (WHITE_LONG_CASTLING + WHITE_SHORT_CASTLING)
#define BLACK_CASTLING (BLACK_LONG_CASTLING + BLACK_SHORT_CASTLING)

#define LONG_CASTLING (WHITE_LONG_CASTLING + BLACK_LONG_CASTLING)
#define SHORT_CASTLING (WHITE_SHORT_CASTLING + BLACK_SHORT_CASTLING)

#define ANY_CASTLING (WHITE_CASTLING + BLACK_CASTLING)

//################################

#define GAME_OPEN 0
#define GAME_DRAWN 1
#define WHITE_WON 2
#define BLACK_WON 3

#define FIFTY_MOVE_RULE 1
#define BLACK_IS_CHECK_MATE 2
#define WHITE_IS_CHECK_MATE 3
#define STALEMATE 4
#define REPEATED_BOARD_POSITION 5
#define INSUFFICIENT_MATERIAL 6


#define ILLEGAL_POS 64


/**
 * Used in move tables to indicate that eg. a knight can't move
 * to any diagonal position
 */
#define IMPOSSIBLE_MOVE 96


#endif
