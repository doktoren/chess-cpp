// g++ -ggdb test.cxx -o test
// ./test

#include <iostream>
#include <string>

const std::string POS_NAME[66] =
{"a1","b1","c1","d1","e1","f1","g1","h1",
 "a2","b2","c2","d2","e2","f2","g2","h2",
 "a3","b3","c3","d3","e3","f3","g3","h3",
 "a4","b4","c4","d4","e4","f4","g4","h4",
 "a5","b5","c5","d5","e5","f5","g5","h5",
 "a6","b6","c6","d6","e6","f6","g6","h6",
 "a7","b7","c7","d7","e7","f7","g7","h7",
 "a8","b8","c8","d8","e8","f8","g8","h8",
 "##","##"};

#include "egtb.cpp"

#if defined (T_INDEX64) && defined (_MSC_VER)
typedef unsigned __int64 INDEX;
#elif defined (T_INDEX64)
typedef unsigned long long INDEX;
#else
typedef unsigned long INDEX;
#endif

using namespace std;


/*
VTbCloseFiles (void)

extern "C" int FTbSetCacheSize
	(
	void	*pv,
	ULONG	cbSize


Kun for windows :-(
extern "C" int FMapTableToMemory
	(
	int		iTb,	// IN | Tablebase
	color	side	// IN | Side to move
	)
*/

string signedToString(uint n) {
  char tmp[32];
  sprintf(tmp, "%d",n);
  return string(tmp);
}
#define ENDGAME_TABLE_WIN -124
#define ENDGAME_TABLE_DRAW -125
#define ENDGAME_TABLE_LOSS -126
#define ENDGAME_TABLE_UNKNOWN -127
#define ENDGAME_TABLE_ILLEGAL -128
string endgame_value_to_string(int v) {
  if (v<=-124) {
    switch (v) {
    case ENDGAME_TABLE_WIN:
      return "GW";
    case ENDGAME_TABLE_LOSS:
      return "GL";
    case ENDGAME_TABLE_DRAW:
      return "draw";
    case ENDGAME_TABLE_UNKNOWN:
      return "????";
    case ENDGAME_TABLE_ILLEGAL:
      return "*";
    }
  }
  if (v>0) {
    return "M"+signedToString(v);
  } else {
    return "-M"+signedToString(-v);
  }
}


int main() {
  cerr << "Initializing stuff...\n";
  IInitializeTb("../../Nalimov/");
  
  // Use a 1 MB cache size
  int EGTB_cache_size = 1<<20;
  void *EGTB_cache = malloc(EGTB_cache_size);
  FTbSetCacheSize(EGTB_cache, EGTB_cache_size);

  //cerr << "Enter 0 when ready!\n"; int aslkuhflasdf; cin >> aslkuhflasdf;

  /*
     a b c d e f g h
  +-----------------+    |
8 |                 | 8  | 59w
7 |                 | 7  |
6 |                 | 6  | White has lost castling
5 |                 | 5  | Black has lost castling
4 |                 | 4  |
3 |                 | 3  | moves played since progress = 1
2 |   r           k | 2  |
1 |     K R         | 1  |
  +-----------------+    |
    a b c d e f g h
FEN description: 8/8/8/8/8/8/1r5k/2KR4 w - - 1 59
  */


  /*
************************************************************
*                                                          *
*   initialize counters and piece arrays so the probe code *
*   can compute the modified Godel number.                 *
*                                                          *
************************************************************
*/

  // C_PIECES is a define in egtb.cpp (currently 3) that
  // defines the maximum number of each piece

  int side = 0;
  uint en_passant = 127;

  int rgiCounters[2*5];
  for (int i=0; i<10; i++) rgiCounters[i]=0;
  uint white_positions[5*C_PIECES+1];
  for (int i=0; i<5*C_PIECES+1; i++) white_positions[i]=0;
  uint black_positions[5*C_PIECES+1];
  for (int i=0; i<5*C_PIECES+1; i++) black_positions[i]=0;

  white_positions[3*C_PIECES] = 3;//ROOK is number 3 and is placed on square 3
  white_positions[5*C_PIECES] = 2;//KING is number 5 and is placed on square 2
  rgiCounters[3] = 1;      //WROOK is number 3
  black_positions[3*C_PIECES] = 9;//ROOK is number 3 and is placed on square 9
  black_positions[5*C_PIECES] = 15;//KING is number 5 and is placed on square 2
  rgiCounters[8] = 1;      //WROOK is number 3+5

  int fInvert = 0;//not invert

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
    cerr << "Endgame not registered!\n";
    return 1;
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
    cerr << "Endgame not loaded!\n";
    return 1;
  }



  PfnCalcIndex index_function = PfnIndCalc(iTb, side);
  cerr << "Hertil ok\n";
  INDEX ind = index_function(&(white_positions[0]), &(black_positions[0]), en_passant, fInvert);
  cerr << "Hertil ok\n";
  int tbValue = L_TbtProbeTable(iTb, side, ind);
  if (bev_broken == tbValue) {
    cerr << "A broken position!\n";
    return 1;
  }


  /*
#define bev_broken  (tbbe_ssL + 1)  // illegal or busted

#define bev_mi1     tbbe_ssL        // mate in 1 move
#define bev_mimin   1               // mate in 126 moves

#define bev_draw    0               // draw

#define bev_limax   (-1)            // mated in 125 moves
#define bev_li0     (-tbbe_ssL)     // mated in 0 moves

#define	bev_limaxx	(-tbbe_ssL - 1)	// mated in 126 moves
#define	bev_miminx	(-tbbe_ssL - 2)	// mate in 127 moves
*/

  int value;
  if (tbValue > 0) {
    // A win
    value = L_tbbe_ssL - tbValue + 1;
  } else if (tbValue < 0) {
    value = tbValue + L_tbbe_ssL;
  } else {
    value = ENDGAME_TABLE_DRAW;
  }

  cerr << "Succesfull! value = " << endgame_value_to_string(value) << "\n";


  // Cleanup
  free(EGTB_cache);

  return 0;
}
