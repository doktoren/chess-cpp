#ifndef _ENDGAME_PIECE_POS_
#define _ENDGAME_PIECE_POS_

#include <iostream>
#include <vector>

#include "../piece_pos.hxx"

/**
 * Returns true if the piece positions in the vector contains a collision.
 */
bool piece_overlap(std::vector<PiecePos> pp);

/**
 * TODO
 */
bool swap_piece_pos(std::vector<PiecePos> &pieces, bool symmetric_endgame_and_btm);

/**
 * TODO
 */
void sort_piece_pos(std::vector<PiecePos> &pieces);

/**
 * If endgame is KBKP, and bishop captured by pawn is being promoted to queen,
 * then s is K_KQ  (and stm is BLACK).
 * Result will be the needed stm, hence BLACK in this case (B->W->B)
 * Will change p if it contains a zero entry
 */
int endgame_dependency_needed_stm(Piece *p, int num_pieces, int stm);

#endif
