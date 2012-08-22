#ifndef _BOARD_MOVE_TABLES_
#define _BOARD_MOVE_TABLES_

extern Position *WPAWN_NEXT;
extern Position *KNIGHT_NEXT;
extern Position *BISHOP_NEXT;
extern Position *ROOK_NEXT;
extern Position *QUEEN_NEXT;
extern Position *KING_NEXT;
extern Position *BPAWN_NEXT;

extern Position *_PIECE_NEXT[13];

#define PIECE_NEXT(piece, from, to) _PIECE_NEXT[piece][(from<<6) | to]

extern Position *WPAWN_JUMP;
extern Position *KNIGHT_JUMP;
extern Position *BISHOP_JUMP;
extern Position *ROOK_JUMP;
extern Position *QUEEN_JUMP;
extern Position *KING_JUMP;
extern Position *BPAWN_JUMP;

extern Position *_PIECE_JUMP[13];

#define PIECE_JUMP(piece, from, to) _PIECE_JUMP[piece][(from<<6) | to]


// DNEXT: next move in current direction
// PIECE_DNEXT(piece, from, to) :=
//     (PIECE_NEXT(piece, from, to)==PIECE_JUMP(piece, from, to)) ?
//     ILLEGAL_POS : PIECE_NEXT(piece, from, to)
extern Position *WPAWN_DNEXT;
extern Position *KNIGHT_DNEXT;
extern Position *BISHOP_DNEXT;
extern Position *ROOK_DNEXT;
extern Position *QUEEN_DNEXT;
extern Position *KING_DNEXT;
extern Position *BPAWN_DNEXT;

extern Position *_PIECE_DNEXT[13];

#define PIECE_DNEXT(piece, from, to) _PIECE_DNEXT[piece][(from<<6) | to]

void initialize_move_tables();

// ########################################
// ##########    RETRO MOVES    ###########
// ########################################

extern Position *INV_WPAWN_NEXT;
extern Position *INV_BPAWN_NEXT;

extern Position *INV_WPAWN_JUMP;
extern Position *INV_BPAWN_JUMP;


extern Position *_INV_PIECE_NEXT[13];
#define INV_PIECE_NEXT(piece, from, to) _INV_PIECE_NEXT[piece][(from<<6) | to]
extern Position *_INV_PIECE_JUMP[13];
#define INV_PIECE_JUMP(piece, from, to) _INV_PIECE_JUMP[piece][(from<<6) | to]

#endif
