#ifndef _ENDGAME_INDEXING_
#define _ENDGAME_INDEXING_

#include <vector>

#include "../compression/binary_decision_diagram.hxx"

// Todo:
// All the assert(0) statements have been inserted because now a
// bdd is created for each side to move.
// The corresponding code should (after suff. debugging) be removed.

// The functions compress_*_table_index provides the mapping
//    (player,pieces)  ->  index in [-1..MAX_INDEX[
// where index == -1 represents an illegal position
//
// decompress_*_table_index o compress_*_table_index maps a position
// to its canonical form (eg. with white king on a1-d1-d4)
//
// The functions compress_*_bdd_index provides the mapping
//    pieces   ->   index in [0..MAX_BDD_INDEX[
// May not be used on illegal positions
//
// The functions *_bdd_index_to_table_index provides the mapping
//    index in [0..MAX_BDD_INDEX[   ->   index in [-1..MAX_INDEX[
// MAX_BDD_INDEX is a power of 2 and is greater than MAX_INDEX.
// The "padding entries" are all mapped to -1 by the above functions.
// Remark: If * is a non-symmetric endgame, then actually
//    index in [0..MAX_BDD_INDEX[   ->   index in [-1..MAX_INDEX/2[
// (First half of [0..MAX_INDEX[ is wtm, second half is btm)
//
// The functions preprocess_*_bdd_index provides the mapping
//    pieces   ->   list of pieces in canonical form and
//                  with bound king placed last
// The reason that the bound king must be placed last is that
//    permute_pos(a1-d1-d4 triangle) maps to [0..16[
// and
//    permute_pos(a1-d8 rectangle) maps to [0..32[
// Hence
//    sum_{i=0}^{num_men-1}(result[i]*64^i) <= 2^n
// where n is 6*num_men-2 if no pawns and 6*num_men-1 otherwise
//
// The canonical form for KXXK=w1,p1,p2,bk (and KPPK etc.) is where
// position(p1) < position(p2)
//
// In the bdd index the order KBBK = k1,b1,b2,k2
//   0-5: b1, 6-11: b2, 12-17: free_king, 18-21: remapped_bound_king

typedef int (*TableIndexToBDDIndex)(int index);

#define TABLE_INDEX_TO_BDD_INDEX(name, max_men) \
int name ## _table_index_to_bdd_index(int index)

// construct_bdd_table creates the table from which the bdd
// will be initialized. All entries are mapped to [0..convert_table.size()[
// where 0 represents a wildcard (ENDGAME_TABLE_ILLEGAL)
uchar *construct_bdd_table(const char *table, TableIndexToBDDIndex table_index_to_bdd_index,
			   int table_size, int log_bdd_size, int round_to_mult_of_n,
			   vector<char> &convert_table,
			   const BitList &unreachable);

// ###########################
// ##########  KK  ###########
// ###########################
int compress_KK_table_index(vector<PiecePos> &piece_list);
void decompress_KK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KK,2);



// ############################
// ##########  KXK  ###########
// ############################
int compress_KXK_table_index(vector<PiecePos> &piece_list);
void decompress_KXK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXK,3);

// ############################
// ##########  KPK  ###########
// ############################
int compress_KPK_table_index(vector<PiecePos>& piece_list);
void decompress_KPK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KPK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KPK,3);

// #############################
// ##########  KXXK  ###########
// #############################
int compress_KXXK_table_index(vector<PiecePos>& piece_list);
void decompress_KXXK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXXK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXXK,4);

// #############################
// ##########  KXYK  ###########
// #############################
int compress_KXYK_table_index(vector<PiecePos>& piece_list);
void decompress_KXYK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXYK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXYK,4);

// #############################
// ##########  KXKY  ###########
// #############################
int compress_KXKY_table_index(vector<PiecePos>& piece_list);
void decompress_KXKY_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXKY_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXKY,4);

// #############################
// ##########  KPPK  ###########
// #############################
int compress_KPPK_table_index(vector<PiecePos>& piece_list);
void decompress_KPPK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KPPK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KPPK,4);

// #############################
// ##########  KXPK  ###########
// #############################
int compress_KXPK_table_index(vector<PiecePos>& piece_list);
void decompress_KXPK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXPK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXPK,4);

// #############################
// ##########  KPKP  ###########
// #############################
int compress_KPKP_table_index(vector<PiecePos>& piece_list);
void decompress_KPKP_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KPKP_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KPKP,4);

// #############################
// ##########  KXKP  ###########
// #############################
int compress_KXKP_table_index(vector<PiecePos>& piece_list);
void decompress_KXKP_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXKP_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXKP,4);


#ifdef ALLOW_5_MEN_ENDGAME

// ##############################
// ##########  KXXXK  ###########
// ##############################
int compress_KXXXK_table_index(vector<PiecePos>& piece_list);
void decompress_KXXXK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXXXK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXXXK,5);


// ##############################
// ##########  KPPPK  ###########
// ##############################
int compress_KPPPK_table_index(vector<PiecePos>& piece_list);
void decompress_KPPPK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KPPPK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KPPPK,5);


// ##############################
// ##########  KXXKY  ###########
// ##############################
int compress_KXXKY_table_index(vector<PiecePos>& piece_list);
void decompress_KXXKY_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXXKY_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXXKY,5);

// ##############################
// ##########  KXXKP  ###########
// ##############################
int compress_KXXKP_table_index(vector<PiecePos>& piece_list);
void decompress_KXXKP_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXXKP_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXXKP,5);

// ##############################
// ##########  KPPKX  ###########
// ##############################
int compress_KPPKX_table_index(vector<PiecePos>& piece_list);
void decompress_KPPKX_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KPPKX_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KPPKX,5);

// ##############################
// ##########  KPPKP  ###########
// ##############################
int compress_KPPKP_table_index(vector<PiecePos>& piece_list);
void decompress_KPPKP_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KPPKP_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KPPKP,5);

// ##############################
// ##########  KXYKZ  ###########
// ##############################
int compress_KXYKZ_table_index(vector<PiecePos>& piece_list);
void decompress_KXYKZ_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXYKZ_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXYKZ,5);

// ##############################
// ##########  KXYKP  ###########
// ##############################
int compress_KXYKP_table_index(vector<PiecePos>& piece_list);
void decompress_KXYKP_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXYKP_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXYKP,5);

// ##############################
// ##########  KXPKY  ###########
// ##############################
int compress_KXPKY_table_index(vector<PiecePos>& piece_list);
void decompress_KXPKY_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXPKY_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXPKY,5);

// ##############################
// ##########  KXPKP  ###########
// ##############################
int compress_KXPKP_table_index(vector<PiecePos>& piece_list);
void decompress_KXPKP_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXPKP_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXPKP,5);


// ##############################
// ##########  KXXYK  ###########
// ##############################
int compress_KXXYK_table_index(vector<PiecePos>& piece_list);
void decompress_KXXYK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXXYK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXXYK,5);

// ##############################
// ##########  KXXPK  ###########
// ##############################
int compress_KXXPK_table_index(vector<PiecePos>& piece_list);
void decompress_KXXPK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXXPK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXXPK,5);

// ##############################
// ##########  KXYYK  ###########
// ##############################
int compress_KXYYK_table_index(vector<PiecePos>& piece_list);
void decompress_KXYYK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXYYK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXYYK,5);

// ##############################
// ##########  KXPPK  ###########
// ##############################
int compress_KXPPK_table_index(vector<PiecePos>& piece_list);
void decompress_KXPPK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXPPK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXPPK,5);

// ##############################
// ##########  KXYZK  ###########
// ##############################
int compress_KXYZK_table_index(vector<PiecePos>& piece_list);
void decompress_KXYZK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXYZK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXYZK,5);

// ##############################
// ##########  KXYPK  ###########
// ##############################
int compress_KXYPK_table_index(vector<PiecePos>& piece_list);
void decompress_KXYPK_table_index(int index, vector<PiecePos>& piece_list);
BDD_Index preprocess_KXYPK_bdd_index(const vector<PiecePos> &piece_list);
TABLE_INDEX_TO_BDD_INDEX(KXYPK,5);

#endif // #ifdef ALLOW_5_MEN_ENDGAME


#undef TABLE_INDEX_TO_BDD_INDEX

#endif // #define _ENDGAME_INDEXING_
