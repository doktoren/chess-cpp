#ifndef _ENDGAME_INDEXING_
#define _ENDGAME_INDEXING_

#include <vector>

#include "../compression/binary_decision_diagram.hxx"
#include "../piece_pos.hxx"
#include "../compression/bdd_index.hxx"

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

// construct_bdd_table creates the table from which the bdd
// will be initialized. All entries are mapped to [0..convert_table.size()[
// where 0 represents a wildcard (ENDGAME_TABLE_ILLEGAL)
uchar *construct_bdd_table(const char *table, TableIndexToBDDIndex table_index_to_bdd_index,
			   int table_size, int log_bdd_size, int round_to_mult_of_n,
			   vector<char> &convert_table,
			   const BitList &unreachable);

#define TABLES(endgame) \
    int compress_ ## endgame ## _table_index(vector<PiecePos> &piece_list); \
    void decompress_ ## endgame ## _table_index(int index, vector<PiecePos>& piece_list); \
    BDD_Index preprocess_ ## endgame ## _bdd_index(const vector<PiecePos> &piece_list); \
    int endgame ## _table_index_to_bdd_index(int index);

TABLES(KK)
TABLES(KXK)
TABLES(KPK)
TABLES(KXKX)
TABLES(KXYK)
TABLES(KXKY)
TABLES(KPPK)
TABLES(KXPK)
TABLES(KPKP)
TABLES(KXKP)
#ifdef ALLOW_5_MEN_ENDGAME
TABLES(KXXXK)
TABLES(KPPPK)
TABLES(KXXKY)
TABLES(KXXKP)
TABLES(KPPKX)
TABLES(KPPKP)
TABLES(KXYKZ)
TABLES(KXYKP)
TABLES(KXPKY)
TABLES(KXPKP)
TABLES(KXXYK)
TABLES(KXXPK)
TABLES(KXYYK)
TABLES(KXPPK)
TABLES(KXYZK)
TABLES(KXYPK)
#endif

#undef TABLES

#endif
