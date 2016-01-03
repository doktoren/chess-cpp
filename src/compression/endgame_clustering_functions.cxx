#include "endgame_clustering_functions.hxx"

#include "endgame_square_permutations.hxx"

// _FK_ : Free King, _BK_ : Bound King
#if BOUND_KING==0
#define _FK_ bk
#define _BK_ wk
#else
#define _FK_ wk
#define _BK_ bk
#endif

#ifdef ALLOW_5_MEN_ENDGAME
#define FIFTH_PARAM , __attribute__((unused)) int _5
#else
#define FIFTH_PARAM
#endif


uint8_t CORNER_DIST[64] = {
  0,1,2,3,3,2,1,0,
  1,1,2,3,3,2,1,1,
  2,2,2,3,3,2,2,2,
  3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,
  2,2,2,3,3,2,2,2,
  1,1,2,3,3,2,1,1,
  0,1,2,3,3,2,1,0};

uint8_t EDGE_DIST[64] = {
  0,0,0,0,0,0,0,0,
  0,1,1,1,1,1,1,0,
  0,1,2,2,2,2,1,0,
  0,1,2,3,3,2,1,0,
  0,1,2,3,3,2,1,0,
  0,1,2,2,2,2,1,0,
  0,1,1,1,1,1,1,0,
  0,0,0,0,0,0,0,0};

inline uint8_t dist(Position p1, Position p2) {
  return max(abs(ROW[p1]-ROW[p2]), abs(COLUMN[p1]-COLUMN[p2]));
}

// IMPORTANT! Remember to include the line
//     _BK_ = INV_REMAP_BOUND_KING[_BK_];
// in the top of every cluster function to convert the bound king to its
// correct position.

// A function XXX_subset_number must return values in
// [0...num_values_XXX[ for all positions - broken or not!


#define num_values_KRK (1 + 3 + 7)
int KRK_subset_number(__attribute__((unused)) int rook, int _FK_, int _BK_, __attribute__((unused)) int ignored FIFTH_PARAM) {
  _BK_ = INV_REMAP_BOUND_KING[_BK_];

  return CORNER_DIST[bk] + dist(wk, bk);
}

#define num_values_KBBK (1 + 2*3 + 7)
int KBBK_subset_number(__attribute__((unused)) int bishop1, __attribute__((unused)) int bishop2, int _FK_, int _BK_ FIFTH_PARAM) {
  _BK_ = INV_REMAP_BOUND_KING[_BK_];
  /*
  cerr << "KBBK_subset_number(" << POS_NAME[wk] << "," << POS_NAME[bishop1]
       << "," << POS_NAME[bishop2] << "," << POS_NAME[bk] << ") = "
       << (2*CORNER_DIST[bk] + dist(wk, bk)) << "\n";
  */
  return 2*CORNER_DIST[bk] + dist(wk, bk);
}


#define num_values_KBNK (1 + 2*3 + 7)
int KBNK_subset_number(__attribute__((unused)) int knight, int bishop, int _FK_, int _BK_ FIFTH_PARAM) {
  _BK_ = INV_REMAP_BOUND_KING[_BK_];
  return 2*CORNER_DIST[bishop] + dist(wk, bk);
}


#define num_values_KRKB (1 + 4*7 + 2*3 + 7)
int KRKB_subset_number(int bishop, __attribute__((unused)) int rook, int _FK_, int _BK_ FIFTH_PARAM) {
  _BK_ = INV_REMAP_BOUND_KING[_BK_];
  return 13 + 4*dist(bk, bishop) - 2*EDGE_DIST[bk] - dist(wk, bk);
}


#define num_values_KQKR (1 + 3 + 3 + 7 + 7)
int KQKR_subset_number(int brook, __attribute__((unused)) int wqueen, int _FK_, int _BK_ FIFTH_PARAM) {
  _BK_ = INV_REMAP_BOUND_KING[_BK_];
  
  return EDGE_DIST[bk] + CORNER_DIST[bk] + dist(bk,wk) + (7 - dist(bk, brook));

}


#ifdef ALLOW_5_MEN_ENDGAME
//...
#endif

ClusterFunction cluster_functions[DB_ARRAY_LENGTH];
int cluster_functions_num_values[DB_ARRAY_LENGTH];

void init_cluster_functions() {
  for (int i=0; i<DB_ARRAY_LENGTH; i++) {
    cluster_functions[i] = 0;
    cluster_functions_num_values[i] = 0;
  }

  // KRK
  cluster_functions[DB_WROOK_VALUE] = 
    cluster_functions[DB_BROOK_VALUE] = KRK_subset_number;
  cluster_functions_num_values[DB_WROOK_VALUE] = 
    cluster_functions_num_values[DB_BROOK_VALUE] = num_values_KRK;

  // Use KRK function for KQK also
  cluster_functions[DB_WQUEEN_VALUE] = 
    cluster_functions[DB_BQUEEN_VALUE] = KRK_subset_number;
  cluster_functions_num_values[DB_WQUEEN_VALUE] = 
    cluster_functions_num_values[DB_BQUEEN_VALUE] = num_values_KRK;

  // KBBK
  cluster_functions[DB_WBISHOP_VALUE + DB_WBISHOP_VALUE] =
    cluster_functions[DB_BBISHOP_VALUE + DB_BBISHOP_VALUE] = KBBK_subset_number;
  cluster_functions_num_values[DB_WBISHOP_VALUE + DB_WBISHOP_VALUE] =
    cluster_functions_num_values[DB_BBISHOP_VALUE + DB_BBISHOP_VALUE] = num_values_KBBK;

  // KBNK
  cluster_functions[DB_WBISHOP_VALUE + DB_WKNIGHT_VALUE] =
    cluster_functions[DB_BBISHOP_VALUE + DB_BKNIGHT_VALUE] = KBNK_subset_number;
  cluster_functions_num_values[DB_WBISHOP_VALUE + DB_WKNIGHT_VALUE] =
    cluster_functions_num_values[DB_BBISHOP_VALUE + DB_BKNIGHT_VALUE] = num_values_KBNK;

  // KRKB
  cluster_functions[DB_WROOK_VALUE + DB_WBISHOP_VALUE] =
    cluster_functions[DB_BROOK_VALUE + DB_BBISHOP_VALUE] = KRKB_subset_number;
  cluster_functions_num_values[DB_WROOK_VALUE + DB_WBISHOP_VALUE] =
    cluster_functions_num_values[DB_BROOK_VALUE + DB_BBISHOP_VALUE] = num_values_KRKB;

  // KQKR
  cluster_functions[DB_WQUEEN_VALUE + DB_BROOK_VALUE] =
    cluster_functions[DB_BQUEEN_VALUE + DB_WROOK_VALUE] = KQKR_subset_number;
  cluster_functions_num_values[DB_WQUEEN_VALUE + DB_BROOK_VALUE] =
    cluster_functions_num_values[DB_BQUEEN_VALUE + DB_WROOK_VALUE] = num_values_KQKR;
}

#undef FIFTH_PARAM
#undef _FK_
#undef _BK_
