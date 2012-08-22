// ./probe_Nalimov/egtb.cpp includes ./probe_Nalimov/lock.h and ./probe_Nalimov/tbdecode.h
#include "./probe_Nalimov/egtb.cpp"

#include "endgame_Nalimov.hxx"

#include <iostream>

using namespace std;

#if defined (T_INDEX64) && defined (_MSC_VER)
typedef unsigned __int64 INDEX;
#elif defined (T_INDEX64)
typedef unsigned long long INDEX;
#else
typedef unsigned long INDEX;
#endif


// FReadTableToMemory


bool Nalimov_egtb_initialized = false;
void init_Nalimov_egtb(string directory, int cache_size) {
  // Use a 1 MB cache size
  if (Nalimov_egtb_initialized) {
    //cerr << "Nalimov endgame tables has already been initialized.\n";
    return;
  }
  Nalimov_egtb_initialized = true;
  char tmp[256];
  strcpy(tmp, directory.c_str());
  IInitializeTb(tmp);
  int EGTB_cache_size = cache_size;
  void *EGTB_cache = malloc(EGTB_cache_size);
  FTbSetCacheSize(EGTB_cache, EGTB_cache_size);
  cerr << "Nalimov's endgame tables initialized.\n";
}
bool Nalimov_egtb_is_initialized() {
  return Nalimov_egtb_initialized;
}

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

#define ENDGAME_TABLE_WIN -124
#define ENDGAME_TABLE_DRAW -125
#define ENDGAME_TABLE_LOSS -126
#define ENDGAME_TABLE_UNKNOWN -127
#define ENDGAME_TABLE_ILLEGAL -128

const Piece PIECE_KIND[13] =
  {0,
   PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
   PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

const int PIECE_COLOR[13] = {-1,
			     false, false, false, false, false, false,
			     true, true, true, true, true, true};

char index_Nalimov_egtb(vector<PiecePos>& piece_list, bool black_to_move, Position en_passant) {
  // rgiCounters counts the number of each piece.
  // {WPAWN,WKNIGHT,WBISHOP,WROOK,WQUEEN,BPAWN,BKNIGHT,BBISHOP,BROOK,BQUEEN}
  int rgiCounters[2*5];
  for (int i=0; i<10; i++) rgiCounters[i]=0;

  // white_positions and black_positions contains the position of each piece
  // Example. White has 2 rooks. Their squares are determined by
  //          white_positions[3*C_PIECES] and white_positions[3*C_PIECES+1]
  uint white_positions[5*C_PIECES+1], black_positions[5*C_PIECES+1];
  {
    for (int i=0; i<5*C_PIECES+1; i++)
      white_positions[i] = black_positions[i] = 0;

    for (uint i=0; i<piece_list.size(); i++) {
      if (PIECE_KIND[piece_list[i].piece] == KING) {
	if (piece_list[i].piece == WKING) {
	  white_positions[5*C_PIECES] = piece_list[i].pos;
	} else {
	  black_positions[5*C_PIECES] = piece_list[i].pos;
	}
      } else {
	if (PIECE_COLOR[piece_list[i].piece]) {
	  black_positions[(piece_list[i].piece-6-1)*C_PIECES +
			  rgiCounters[piece_list[i].piece-2]++] = piece_list[i].pos;
	} else {
	  white_positions[(piece_list[i].piece-1)*C_PIECES +
			  rgiCounters[piece_list[i].piece-1]++] = piece_list[i].pos;
	}
      }
    }
  }

  /*
  cerr << "white_positions =";
  for (int i=0; i<5*C_PIECES+1; i++)
    cerr << " " << white_positions[i] << ",";
  cerr << "\n";
   cerr << "black_positions =";
  for (int i=0; i<5*C_PIECES+1; i++)
    cerr << " " << black_positions[i] << ",";
  cerr << "\n";
   cerr << "rgiCounters =";
  for (int i=0; i<10; i++)
    cerr << " " << rgiCounters[i] << ",";
  cerr << "\n";
  */
  
/*
 ************************************************************
 *                                                          *
 *   quick early exit.  is the tablebase for the current    *
 *   set of pieces registered?                              *
 *                                                          *
 ************************************************************
 */
  int iTb = IDescFindFromCounters(rgiCounters);
  if (!iTb) {
    //cerr << "Endgame not registered!\n";
    return ENDGAME_TABLE_UNKNOWN;
  }
/*
 ************************************************************
 *                                                          *
 *   yes, finish setting up to probe the tablebase.  if     *
 *   black is the "winning" side (more pieces) then we need *
 *   to "invert" the pieces in the lists.                   *
 *                                                          *
 ************************************************************
 */
  int fInvert, side;
  uint *wp,*bp;
  if (iTb > 0) {
    side = black_to_move;
    fInvert = 0;
    wp = white_positions;
    bp = black_positions;
  } else {
    side = 1^black_to_move;
    fInvert = 1;
    wp = black_positions;
    bp = white_positions;
    iTb = -iTb;
  }
/*
 ************************************************************
 *                                                          *
 *   now check to see if this particular tablebase for this *
 *   color to move is registered.                           *
 *                                                          *
 ************************************************************
 */ 
  if (!FRegistered(iTb, side)) {
    //cerr << "Endgame not loaded!\n";
    return ENDGAME_TABLE_UNKNOWN;
  }

  uint sqEnP = en_passant == 64 ? 127 : en_passant;
  INDEX ind = PfnIndCalc(iTb, side) (wp, bp, sqEnP, fInvert);
  int tbValue = L_TbtProbeTable(iTb, side, ind);
  if (tbValue  ==  L_bev_broken) {
    cerr << "Error: tbValue  ==  L_bev_broken\n";
    //exit(1);
    return ENDGAME_TABLE_ILLEGAL;
  } else if (tbValue > 0) {
    // A win
    return L_tbbe_ssL - tbValue + 1;
  } else if (tbValue < 0) {
    // A loss
    return -L_tbbe_ssL - tbValue;
  } else {
    // A draw
    return ENDGAME_TABLE_DRAW;
  }
}

/*
// g++ -ggdb endgame_Nalimov.cxx -o test
// ./test
int main() {
  init_Nalimov_egtb("/users/doktoren/public_html/master_thesis/Nalimov/", 1<<20);

  //    a b c d e f g h
  //  +-----------------+    |
  //8 |               r | 8  | 58w
  //7 |                 | 7  |
  //6 |         K       | 6  | White can still castle
  //5 |         R       | 5  | Black can still castle
  //4 |                 | 4  |
  //3 |                 | 3  | moves played since progress = 0
  //2 |             k   | 2  |
  //1 |                 | 1  |
  //  +-----------------+    |
  //    a b c d e f g h

#include "board_define_position_constants.hxx"
  vector<PiecePos> pp(4);
  pp[0] = PiecePos(WKING, e6);
  pp[1] = PiecePos(WROOK, e5);
  pp[2] = PiecePos(BROOK, h8);
  pp[3] = PiecePos(BKING, g2);
#include "board_undef_position_constants.hxx"

  cerr << "Value = " << (int)index_Nalimov_egtb(pp, 0, 64) << "\n";
}
*/
