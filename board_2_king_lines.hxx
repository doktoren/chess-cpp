#ifndef _BOARD_2_KING_LINES_
#define _BOARD_2_KING_LINES_

#include "board_tables.hxx"

// The line below does not work if a certain optimization is on
//#define checktable(pa, pl) (CHECK_TABLE[((ushort *)(&(pa)))[pl]])
#define checktable(pa, pl) (CHECK_TABLE[(pl) ? ((pa) >> 16) : ((pa) & 0xFFFF)])

// board_king_lines contains functionality to maintain the
// double array uint board_lines[4][16]
// Lines can be formed in 4 ways: horz. vert. or diag1 or diag2
// Hence the constants EAST, NORTH, NORTH_EAST and NORTH_WEST
// can be used as the first index.
// The next index is the line number. There are 8 h. and v. lines
// and 2*15 diagonal lines.
//
// The upper 16 bits of each entry is used for black, and the lower
// 16 bits for white.
// The idea is that only a single lookup in the CHECK_TABLE shows
// if eg. black king is checked on any specified line.
// Example:
//   CHECK_TABLE[((ushort *)(&(board_lines[NORTH][4])))[BLACK]]
// This expression will be true iff black player has its king
// on colum 4 (the e colum) and it is being checked by a long distance
// enemy piece (bishop, rook or queen) on this line (colum e).
//
// The CHECK_TABLE will not be used to examine checks from pawns,
// knights or a king. Here the bit_board will be applied instead.
//

// DIAG_INDEX[position on board][EAST, NORTH, NORTH_EAST or NORTH_WEST]
// gives the line number (second index in board_lines).
// Example:
//   board_lines[NORTH][ DIAG_INDEX[e2][NORTH] ]
// gives the vertical line through e2.
static const uchar DIAG_INDEX[64][4] =
{{0, 0, 7, 0},
 {0, 1, 6, 1},
 {0, 2, 5, 2},
 {0, 3, 4, 3},
 {0, 4, 3, 4},
 {0, 5, 2, 5},
 {0, 6, 1, 6},
 {0, 7, 0, 7},

 {1, 0, 8, 1},
 {1, 1, 7, 2},
 {1, 2, 6, 3},
 {1, 3, 5, 4},
 {1, 4, 4, 5},
 {1, 5, 3, 6},
 {1, 6, 2, 7},
 {1, 7, 1, 8},
 
 {2, 0, 9, 2},
 {2, 1, 8, 3},
 {2, 2, 7, 4},
 {2, 3, 6, 5},
 {2, 4, 5, 6},
 {2, 5, 4, 7},
 {2, 6, 3, 8},
 {2, 7, 2, 9},

 {3, 0,10, 3},
 {3, 1, 9, 4},
 {3, 2, 8, 5},
 {3, 3, 7, 6},
 {3, 4, 6, 7},
 {3, 5, 5, 8},
 {3, 6, 4, 9},
 {3, 7, 3,10},

 {4, 0,11, 4},
 {4, 1,10, 5},
 {4, 2, 9, 6},
 {4, 3, 8, 7},
 {4, 4, 7, 8},
 {4, 5, 6, 9},
 {4, 6, 5,10},
 {4, 7, 4,11},

 {5, 0,12, 5},
 {5, 1,11, 6},
 {5, 2,10, 7},
 {5, 3, 9, 8},
 {5, 4, 8, 9},
 {5, 5, 7,10},
 {5, 6, 6,11},
 {5, 7, 5,12},

 {6, 0,13, 6},
 {6, 1,12, 7},
 {6, 2,11, 8},
 {6, 3,10, 9},
 {6, 4, 9,10},
 {6, 5, 8,11},
 {6, 6, 7,12},
 {6, 7, 6,13},

 {7, 0,14, 7},
 {7, 1,13, 8},
 {7, 2,12, 9},
 {7, 3,11,10},
 {7, 4,10,11},
 {7, 5, 9,12},
 {7, 6, 8,13},
 {7, 7, 7,14}};

// Each entry of board_lines[4][16] contains 2*16 bits (low=white, high=black)
// Each horz. or vert. line of the board consists of 8 positions -
// the diagonal lines consists of at most 8.
// Hence 2 bit for each color can be assigned to each position.
// The assignments are:
//   0 = 00b : Empty position
//   1 = 01b : King of the specified color
//   2 = 10b : Enemy piece that can operate along this line (hence eg. an enemy
//             bishop will not count here on a vertical line)
//   3 = 11b : All other posibilies - ie. obstacles.
//
// The table DIAG_PIECE_PATTERN[13][4] was first a direct mapping from
// (piece, direction) to these values.
// However to use these values, they must first be shifted left two times
// the position on the line, before they can be or'ed to the line.
// This should be done both for white and black.
// By modifying DIAG_PIECE_PATTERN to give +4 for black pieces and by
// using another table, DIAG_PATTERN, to do the shifting, the work
// for white and black can be done simultaneously.
static const uchar DIAG_PIECE_PATTERN[13+2][4] = // [piece][direction]
{{0,0,0,0},
 {3,3,3,3}, // WPAWN = OBSTACLE for both players
 {3,3,3,3}, // WKNIGHT = OBSTACLE for both players
 {3,3,2,2}, // WBISHOP = OBSTACLE except for black i diag directions
 {2,2,3,3}, // WROOK = OBSTACLE except for black i h.v. directions
 {2,2,2,2}, // WQUEEN = OBSTACLE except for black
 {1,1,1,1}, // WKING = OBSTACLE for black
 {3+4,3+4,3+4,3+4}, // BPAWN
 {3+4,3+4,3+4,3+4}, // BKNIGHT
 {3+4,3+4,2+4,2+4}, // BBISHOP
 {2+4,2+4,3+4,3+4}, // BROOK
 {2+4,2+4,2+4,2+4}, // BQUEEN
 {1+4,1+4,1+4,1+4}, // BKING
 {3,3,3,3}, // Fictive file a->h pawn
 {3,3,3,3}};// Fictive file h->a pawn

// DIAG_PATTERN which effectively shift the 2 bits for both white
// and black 2*index time to the left. (The 2*2 bits are encoded in 3).
static const uint DIAG_PATTERN[8][8] = // [index][DIAG_PIECE_PATTERN]
{{0x00000000, 0x00030001, 0x00020003, 0x00030003, 0x00000000, 0x00010003, 0x00030002, 0x00030003},
 {0x00000000, 0x000C0004, 0x0008000C, 0x000C000C, 0x00000000, 0x0004000C, 0x000C0008, 0x000C000C},
 {0x00000000, 0x00300010, 0x00200030, 0x00300030, 0x00000000, 0x00100030, 0x00300020, 0x00300030},
 {0x00000000, 0x00C00040, 0x008000C0, 0x00C000C0, 0x00000000, 0x004000C0, 0x00C00080, 0x00C000C0},
 {0x00000000, 0x03000100, 0x02000300, 0x03000300, 0x00000000, 0x01000300, 0x03000200, 0x03000300},
 {0x00000000, 0x0C000400, 0x08000C00, 0x0C000C00, 0x00000000, 0x04000C00, 0x0C000800, 0x0C000C00},
 {0x00000000, 0x30001000, 0x20003000, 0x30003000, 0x00000000, 0x10003000, 0x30002000, 0x30003000},
 {0x00000000, 0xC0004000, 0x8000C000, 0xC000C000, 0x00000000, 0x4000C000, 0xC0008000, 0xC000C000}};

// This pattern array is used for clearing a given position on the
// line (clears both for white and black).
static const uint AND_DPP_PATTERN[8] = // [index]
{0xFFFCFFFC, 0xFFF3FFF3, 0xFFCFFFCF, 0xFF3FFF3F,
 0xFCFFFCFF, 0xF3FFF3FF, 0xCFFFCFFF, 0x3FFF3FFF};

// This pattern array is used for clearing the strictly lower
// positions on the line (clears both for white and black).
static const uint CLEARLOW_DPP_PATTERN[8] = // [index]
{0xFFFFFFFF, 0xFFFCFFFC, 0xFFF0FFF0, 0xFFC0FFC0,
 0xFF00FF00, 0xFC00FC00, 0xF000F000, 0xC000C000};

// unsigned direction
inline bool straight_direction(int d) { return !(d&~3); }

// This function updates the 4 lines that goes through pos
// accordingly to the new piece. pos must not be occupied -
// use the next function to reverse updates.
inline void Board2::king_line_insert_piece(Position pos, Piece piece) {
  assert(legal_pos(pos)  &&  piece  &&  legal_pos(king_pos[player^1]));

  const uchar *tmp = DIAG_PIECE_PATTERN[piece];

  uint *pattern = &(board_lines[0][DIAG_INDEX[pos][0]]);
  *pattern |= DIAG_PATTERN[pos&7][tmp[0]];

  for (int d=1; d<4; d++) {
    pattern = &(board_lines[d][DIAG_INDEX[pos][d]]);
    *pattern |= DIAG_PATTERN[pos>>3][tmp[d]];
  }

  if (IS_LONG_DISTANCE_PIECE[piece]) {
    // update num_checks if straight line to king.
    // CHECK_TABLE can only contain 1, if it's a long distance piece.
    // (this would not be true, if eg. a knight could jump (dx,dy)=(2,0))
    int king_direction = DIRECTION[pos][king_pos[player^1]];
    if (straight_direction(king_direction)) {
      uint pattern = board_lines[king_direction][DIAG_INDEX[pos][king_direction]];
      if (checktable(pattern, player^1)) {
	++num_checks;
	//cerr << "Adding check from pos " << POS_NAME[pos] << "\n";
	threat_pos = pos;
	threat_elim = BB_LINES[pos][king_pos[player^1]];
      }
    }
  }
}

// Clears the captured piece from the pattern, then calls king_line_insert_piece
inline void Board2::king_line_replace_piece(Position pos, Piece piece) {
  assert(legal_pos(pos)  &&  piece  &&  legal_pos(king_pos[player^1]));

  uint *pattern = &(board_lines[0][DIAG_INDEX[pos][0]]);
  *pattern &= AND_DPP_PATTERN[pos&7];
 
  for (int d=1; d<4; d++) {
    pattern = &(board_lines[d][DIAG_INDEX[pos][d]]);
    *pattern &= AND_DPP_PATTERN[pos>>3];
  }

  king_line_insert_piece(pos, piece);
}


// king_line_remove_piece must be called BEFORE board[pos]=0 !
inline void Board2::king_line_remove_piece(Position pos) {
  assert(legal_pos(pos)  &&  board[pos]);

  uint *pattern = &(board_lines[0][DIAG_INDEX[pos][0]]);
  *pattern &= AND_DPP_PATTERN[pos&7];
 
  for (int d=1; d<4; d++) {
    pattern = &(board_lines[d][DIAG_INDEX[pos][d]]);
    *pattern &= AND_DPP_PATTERN[pos>>3];
  }

  // update num_checks
  int king_direction = DIRECTION[pos][king_pos[player^1]];
  if (straight_direction(king_direction)) {
    uint pattern = board_lines[king_direction][DIAG_INDEX[pos][king_direction]];
    if (checktable(pattern, player^1)) {
      // same code as add_check_from_sdirection except regarding position of threatened king

      // cerr << "Add check for removing " << PIECE_NAME[board[pos]] << " at " << POS_NAME[pos] << '\n';
      ++num_checks;
      // Find the threatening piece
      int direction = SDIRECTION[king_pos[player^1]][pos];
      int step = DIRECTIONS[direction];
      // cerr << "step = " << step << "\n";
      threat_pos = pos + step;
      // cerr << "stepping threat_pos to " << POS_NAME[threat_pos] << "\n";
      while (!board[threat_pos]) {
	threat_pos += step;
	// cerr << "stepping threat_pos to " << POS_NAME[threat_pos] << "\n";
      }
      // cerr << "Threat found at " << POS_NAME[threat_pos] << "\n";
      threat_elim = BB_LINES[threat_pos][king_pos[player^1]];
    }
  }
}

int num_unblockable_checks;
// Returns a value identifying the kind of checking piece:
// 1=>pawn,2=>knight,3=>bishop,4=>internal rook,5=>internal queen,6=>border rook,7=>border queen
inline int Board2::add_check_from_sdirection(Position pos, int direction) {
  // cerr << "add_check_from_sdirection(" << POS_NAME[pos] << ", " << direction << ")\n";
  ++num_checks;
  // Find the threatening piece
  int step = DIRECTIONS[direction];
  // cerr << "step = " << step << "\n";
  threat_pos = pos + step;
  // cerr << "stepping threat_pos to " << POS_NAME[threat_pos] << "\n";
  
  const Piece BPK[13] = {0,// Border Piece Kind
			 PAWN, KNIGHT, BISHOP, ROOK+2, QUEEN+2, KING,
			 PAWN, KNIGHT, BISHOP, ROOK+2, QUEEN+2, KING};
			 
  if (board[threat_pos]) {
    ++num_unblockable_checks;

    threat_elim = BB_LINES[threat_pos][pos];

    if (BORDER_DISTANCE[threat_pos]) {
      return PIECE_KIND[board[threat_pos]];
    } else {
      return BPK[board[threat_pos]];
    }
  }

  while (!board[threat_pos += step]) ;
  /* doh
  if (PIECE_KIND[board[threat_pos]] != KING) {
    cerr << "board[threat_pos] = " << PPIECE_NAME[board[threat_pos]]
	 << ", threat_pos = " << POS_NAME[threat_pos] << ", king_pos = "
	 << POS_NAME[pos] << ", " << PPIECE_NAME[board[pos]] << "\n"
	 << "\tdirection = " << direction << "\n";
    print_board(cerr);
  }
  */
  // cerr << "Threat found at " << POS_NAME[threat_pos] << "\n";
  threat_elim = BB_LINES[threat_pos][pos];

  if (BORDER_DISTANCE[threat_pos]) {
    return PIECE_KIND[board[threat_pos]];
  } else {
    return BPK[board[threat_pos]];
  }
}

// init_check_invariants sets num_checks, threat_pos and threat_elim.
// It must be called by loadFEN and set_board since these invariants
// cannot be updated properly while setting up the board
// (or at least it is to difficult to do so).//
//
// Example: if Be4 is placed before Rd3 then an extra check will be counted for
//     a b c d e f g h
//   +-----------------+    |
// 8 |                 | 8  | 1b
// 7 |                 | 7  |
// 6 |                 | 6  | White has lost castling
// 5 |                 | 5  | Black has lost castling
// 4 |         B       | 4  |
// 3 |       R         | 3  | moves played since progress = 0
// 2 |     k           | 2  |
// 1 | K B   q         | 1  |
//   +-----------------+    |
//     a b c d e f g h
//
// set_check_invariants also checks whether the position is unreachable
// according to the checks specified in the thesis
void Board2::set_check_invariants() {
  position_is_unreachable = false;

  // According to section "Easy identifyable unreachable positions" in thesis
  Piece last_checking_piece = 0;
  const bool unreachable_double_checks[8][8] =
    {{false, false, false, false, false, false, false, false},
     {false, true , true , true , false, false, false, false},
     {false, true , true , false, false, false, false, false},
     {false, true , false, true , false, false, false, false},
     {false, false, false, false, true , false, false, false},
     {false, false, false, false, false, false, false, false},
     {false, false, false, false, false, false, false, false},
     {false, false, false, false, false, false, false, false}};

  num_checks = 0;
  Position pos = king_pos[player];

  // Check short distance threats
  for (int i=0; bit_boards[i][player^1][pos]; i++) {
    // cerr << "bit_boards[" << i << "][" << (int)(player^1) << "][" << POS_NAME[pos] << "]\n";
    
    if (num_checks++) {
      // a target has already been found.
      // 2 short distance checks is unreachable
      position_is_unreachable = true;

    } else {

      // find a checking piece to blame
      bool found = false;
      if (!found) {
	// Try a knight
	int find_piece = player ? WKNIGHT : BKNIGHT;
	int dest = PIECE_NEXT(KNIGHT, pos, pos);
	while (dest != ILLEGAL_POS) {
	  if (board[dest] == find_piece) {
	    threat_pos = dest;
	    found = true;
	    
	    if (unreachable_double_checks[last_checking_piece][KNIGHT])
	      position_is_unreachable = true;
	    last_checking_piece = KNIGHT;
	    
	    break;
	  }
	  dest = PIECE_NEXT(KNIGHT, pos, dest);
	}
      }
      
      if (!found) {
	// Try a pawn
	int find_piece = player ? WPAWN : BPAWN;
	if (COLUMN[pos] != 0) {
	  int dest = pos-1 + (player ? -8 : 8);
	  if (board[dest] == find_piece) {
	    threat_pos = dest;
	    found = true;
	  }
	}
	if (COLUMN[pos] != 7) {
	  int dest = pos+1 + (player ? -8 : 8);
	  if (board[dest] == find_piece) {
	    threat_pos = dest;
	    found = true;
	  }
	}
	
	if (found) {
	  if (unreachable_double_checks[last_checking_piece][PAWN])
	    position_is_unreachable = true;
	  last_checking_piece = PAWN;
	}
      }
      
      if (!found) {
	cerr << "Checking short distance piece not found:\n";
	print_board(cerr);
	exit(1);
      }
      threat_elim = BB_LINES[threat_pos][threat_pos];
    }
  }

  // So far all checks has been unblockable
  num_unblockable_checks = num_checks;

  // Check long distance threats
  Piece piece = board[pos];
  const uchar *tmp = DIAG_PIECE_PATTERN[piece];

  uint pattern = board_lines[0][DIAG_INDEX[pos][0]];
  pattern &= AND_DPP_PATTERN[pos&7];
  pattern += DIAG_PATTERN[pos&7][tmp[0]];
  if (checktable(pattern, player)) {

    // checks on both sides? Will not occur on any reachable position
    // but might occur on some setup position
    if (checktable(pattern, player) == 2) {
      
      position_is_unreachable = true;
      add_check_from_sdirection(pos, 0);
      add_check_from_sdirection(pos, 4);

    } else {

      // There is a threat on this line, but in which direction?
      // If the threat is still there after cutting away the pieces
      // on one side, the threat must be on the other side.
      pattern &= CLEARLOW_DPP_PATTERN[pos&7];
      
      int checking_piece;
      if (checktable(pattern, player)) {
	// Checking piece in upper direction
	//cerr << "add_check_from_sdirection(" << POS_NAME[pos] << ", " << SDIRECTION_NAME[0] << ")\n";
	checking_piece = add_check_from_sdirection(pos, 0);

      } else {
	// Checking piece in lower direction
	//cerr << "add_check_from_sdirection(" << POS_NAME[pos] << ", " << SDIRECTION_NAME[4] << ")\n";
	checking_piece = add_check_from_sdirection(pos, 4);
      }

      if (unreachable_double_checks[last_checking_piece][checking_piece])
	position_is_unreachable = true;
      last_checking_piece = checking_piece;
    }
  }

  for (int d=1; d<4; d++) {
    pattern = board_lines[d][DIAG_INDEX[pos][d]];
    pattern &= AND_DPP_PATTERN[pos>>3];
    pattern += DIAG_PATTERN[pos>>3][tmp[d]];
    if (checktable(pattern, player)) {

      // checks on both sides? Will not occur on any reachable position
      // but might occur on some setup position
      if (checktable(pattern, player) == 2) {

	position_is_unreachable = true;
	add_check_from_sdirection(pos, d);
	add_check_from_sdirection(pos, d+4);

      } else {
      
	// There is a threat on this line, but in which direction?
	// If the threat is still there after cutting away the pieces
	// on one side, the threat must be on the other side.
	pattern &= CLEARLOW_DPP_PATTERN[pos>>3];

	int checking_piece;
	if (checktable(pattern, player)) {
	  // Checking piece in upper direction
	  //cerr << "add_check_from_sdirection(" << POS_NAME[pos] << ", " << SDIRECTION_NAME[d] << ")\n";
	  checking_piece = add_check_from_sdirection(pos, d);
	} else {
	  // Checking piece in lower direction
	  //cerr << "add_check_from_sdirection(" << POS_NAME[pos] << ", " << SDIRECTION_NAME[d+4] << ")\n";
	  checking_piece = add_check_from_sdirection(pos, d+4);
	}

	if (unreachable_double_checks[last_checking_piece][checking_piece])
	  position_is_unreachable = true;
	last_checking_piece = checking_piece;
      }
    }
  }

  { // More reachability checks
    if (num_checks > 2  ||  num_unblockable_checks==2)
      position_is_unreachable = true;
  }
}


bool Board2::check_if_moved(Position pos) const {
  assert(legal_pos(pos));

  Piece piece = board[pos];
  int d = DIRECTION[pos][king_pos[player]];
  if (straight_direction(d)) {
    uint pattern = board_lines[d][DIAG_INDEX[pos][d]];
    pattern -= DIAG_PATTERN[d?(pos>>3):(pos&7)][ DIAG_PIECE_PATTERN[piece][d] ];
    return checktable(pattern, player);
  } else {
    return false;
  }
}

// it is not nescessary to remove the king from its origin
// (CHECK_TABLE is also defined for patterns with 2 kings of
//  same color next to each other)
inline bool Board2::check_if_king_placed(Position pos) const {
  assert(legal_pos(pos));

  // cerr << "Check if king placed on " << POS_NAME[pos] << "?\n";

  // Check short distance threats
  if (bit_boards[0][player^1][pos]) return true;

  // Check long distance threats
  Piece piece = KING + 6*player;
  const uchar *tmp = DIAG_PIECE_PATTERN[piece];

  uint pattern = board_lines[0][DIAG_INDEX[pos][0]];
  pattern &= AND_DPP_PATTERN[pos&7];
  pattern |= DIAG_PATTERN[pos&7][tmp[0]];
  if (checktable(pattern, player)) return true;

  for (int d=1; d<4; d++) {
    pattern = board_lines[d][DIAG_INDEX[pos][d]];
    pattern &= AND_DPP_PATTERN[pos>>3];
    pattern |= DIAG_PATTERN[pos>>3][tmp[d]];
    if (checktable(pattern, player)) return true;
  }

  /*
  cerr << "No check if king is placed on " << POS_NAME[pos] << "\n";
  
  cerr << "Pattern before = " << toString(board_lines[2][3], 16, 4) << "\n";
  pattern = board_lines[2][3] & AND_DPP_PATTERN[pos>>3];
  cerr << "Pattern mid = " << toString(pattern, 16, 4) << "\n";
  pattern += DIAG_PATTERN[0][tmp[2]];
  cerr << "Pattern after = " << toString(pattern, 16, 4) << "\n";
  */

  return false;
}

void Board2::print_threat_pos(ostream& os) {
  os << "Threat pos:\n";
  print_bit_board(os, threat_elim);
}

void Board2::print_king_threats(ostream& os) {
  for (int color=0; color<2; color++) {
    os << "                           " << (color?"BLACK":"WHITE") << " KING THREATS\n"
       << "+-----------------+-----------------+-----------------+-----------------+\n";
    for (int r=7; r>=0; r--) {
      os << '|';
      for (int d=0; d<4; d++) {
	for (int c=0; c<8; c++) {
	  Position pos = CR_TO_POS[c][r];
	  uint pattern = board_lines[d][ DIAG_INDEX[pos][d] ];
	  int pattern_offset = 2*(d?(pos>>3):(pos&7)) + 16*color;
	  static const char TMP[4] = {' ','K','T','#'};
	  os << ' ' << TMP[(pattern >> pattern_offset) & 3];
	}
	os << " |";
      }
      os << '\n';
    }
    os << "+-----------------+-----------------+-----------------+-----------------+\n";
  }

  os << "+-----------------+-----------------+-----------------+-----------------+\n";
  for (int r=7; r>=0; r--) {
    os << '|';
    for (int d=0; d<4; d++) {
      for (int c=0; c<8; c++) {
	Position pos = CR_TO_POS[c][r];
	os << ' ' << HEX_CHAR[DIAG_INDEX[pos][d]];
      }
      os << " |";
    }
    os << '\n';
  }
  os << "+-----------------+-----------------+-----------------+-----------------+\n";

  os << "    -VERTICAL LINES- -HORIZONTAL LINES- -a1->h8 PARALLEL- -a8->h1 PARALLEL-\n"
       << "    -WHITE-  -BLACK-  -WHITE-  -BLACK-  -WHITE-  -BLACK-  -WHITE-  -BLACK- \n";
  for (int i=0; i<15; i++) {
    os << toString(i, 2, 10) << ": ";
    for (int d=0; d<4; d++) {
      if (d<2 && i>=8) {
	os << "                  ";
      } else {
	os << toString(board_lines[d][i] & 0xFFFF, 8, 4) << ' '
	     << toString(board_lines[d][i] >> 16, 8, 4) << ' ';
      }
    }
    os << '\n';
  }
}

#endif
