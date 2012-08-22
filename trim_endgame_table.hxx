#ifndef _TRIM_ENDGAME_TABLE_
#define _TRIM_ENDGAME_TABLE_

#include "typedefs.hxx"

void trim_endgame_table(uchar *table, int size,
			int (*compress_table_index)(vector<PiecePos> &),
			void (*decompress_table_index)(int, vector<PiecePos>&));

#endif
