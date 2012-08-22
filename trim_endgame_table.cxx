#include "trim_endgame_table.hxx"

void trim_endgame_table(uchar *table, int size, int player, vector<PiecePos>
			int (*compress_table_index)(vector<PiecePos> &),
			void (*decompress_table_index)(int, vector<PiecePos>&)) {
  vector<bool> used(size);
  for (int i=0; i<size; i++) {
    


  vector<PiecePos> pp(num_pieces);
  board.get_encoded_piece_list(pp);
  sort_piece_pos(pp);
  // From now on, the colors of the pieces will no longer be needed
  bool swapped = swap_piece_pos(pp, symmetric_endgame  &&  board.get_player());

  if (!pawnless_endgame) {

    if (swapped) {
      // fix pawn problem (their move direction dependent on their player) by mirroring board
      for (int i=0; i<num_pieces; i++) {
	pp[i].pos ^= 7<<3;
	
	// It is not nescessary to actually change the color of the pieces ?
	//pp[i].piece = SWAP_PIECE_COLOR[pp[i].piece];
      }
    }
    
    
    
    
  }
  
  
  
  
  
  
  
  
  
  
  
}
