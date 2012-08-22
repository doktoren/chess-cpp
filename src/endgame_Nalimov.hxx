#ifndef _ENDGAME_NALIMOV_
#define _ENDGAME_NALIMOV_

#include <string>
#include <vector>

#include "piece_pos.hxx"

bool Nalimov_egtb_is_initialized();

// directory is where the endgames in Nalimov's format can be found
void init_Nalimov_egtb(std::string directory = "/users/doktoren/public_html/master_thesis/Nalimov/",
		       int cache_size = 1<<20);


// index_endgame_table may change piece_list
char index_Nalimov_egtb(std::vector<PiecePos>& piece_list, bool black_to_move,
			Position en_passant = 64);
// If -123 <= result <= 0 then side-to-move will lose in -result moves
// If 0 < result < 127 then side-to-move will win in result moves
// If result = -124 then side-to-move will win
// If result = -125 then its a draw
// If result = -126 then side-to-move will lose
// If result = -127 then the game theoretical value is unknown
//             (which means that the appropriate table/bdd hasn't been loaded yet)
// If result = -128 then its an illegal entry
//             (eg. if 2 pieces have the same coordinate. WARNING: when
//              index_endgame_table is given an illegal piece_list the result
//              will generelly be undefined!)

#endif
