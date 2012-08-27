#include "endgame_indexing.hxx"

#include "endgame_values.hxx"
#include "../compression/endgame_piece_enumerations.hxx"

// The functions compress_*_table_index provides the mapping
//    (player,pieces)  ->  index in [-1..MAX_INDEX[
// where index == -1 represents an illegal position
//
// decompress_*_table_index(compress_*_table_index(pos)) maps a position
// to its canonical form (eg. with white king on a1-d1-d4)
//
// The functions compress_*_bdd_index provides the mapping
//    pieces   ->   index in [0..MAX_BDD_INDEX[
// Must not be used on illegal positions.
//
// The functions *_bdd_index_to_table_index provides the mapping
//    index in [0..MAX_BDD_INDEX[   ->   index in [-1..MAX_INDEX[
// MAX_BDD_INDEX is a power of 2 and is greater than MAX_INDEX.
// The "padding entries" are all mapped to -1 by the above functions.
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

#define TABLE_INDEX_TO_BDD_INDEX(name, max_men) \
    int name ## _table_index_to_bdd_index(int index) { \
  vector<PiecePos> piece_list(max_men); \
  decompress_ ## name ## _table_index(index, piece_list); \
  return preprocess_ ## name ## _bdd_index(piece_list).index(); \
}

// construct_bdd_table creates the table from which the bdd
// will be initialized. All entries are mapped to [0..convert_table.size()[
// where 0 represents a wildcard (ENDGAME_TABLE_ILLEGAL)
uchar *construct_bdd_table(const char *table, TableIndexToBDDIndex table_index_to_bdd_index,
    int table_size, int log_bdd_size, int round_to_mult_of_n,
    vector<char> &convert_table,
    const BitList &unreachable) {
  // Init bdd_table, convert_table and ct_size
  // At the same time initialize bdd_table from table according to this mapping.
  // All "paddings" in bdd_table is given value ENDGAME_TABLE_ILLEGAL
  int bdd_size = 1<<log_bdd_size;
  uchar *bdd_table = new uchar[bdd_size];
  memset(bdd_table, 0, bdd_size);

  // Convert values to range 0..n
  int conv[256];
  for (int i=0; i<256; i++) conv[i] = -1;

  // Let ENDGAME_TABLE_ILLEGAL be converted to 0
  conv[ENDGAME_TABLE_ILLEGAL + 128] = 0;
  int last_index = 0;

  int _map_values[256];
  int *map_values = &(_map_values[128]);
  {
    for (int i=-128; i<-123; i++)
      map_values[i] = i;
    if (!round_to_mult_of_n) {
      for (int i=-123; i<=0; i++)
        map_values[i] = ENDGAME_TABLE_LOSS;
      for (int i=1; i<128; i++)
        map_values[i] = ENDGAME_TABLE_WIN;
    } else {
      for (int i=-123; i<=0; i++) {
        map_values[i] =
            round_to_mult_of_n*(-((-i + (round_to_mult_of_n-1))/round_to_mult_of_n));
        if (map_values[i] < -123) map_values[i] = -123;
      }
      for (int i=1; i<128; i++) {
        map_values[i] =
            round_to_mult_of_n*((i + (round_to_mult_of_n-1))/round_to_mult_of_n);
        if (map_values[i] > 123) map_values[i] = 123;
      }
    }
  }

  for (int i=0; i<table_size; i++) {
    int bdd_index = table_index_to_bdd_index(i);

#ifndef NDEBUG
    if (bdd_table[bdd_index] != 0) { // DEBUG
      cerr << "Error: BDD index hit by 2 table indexes!!!\n";
      cerr << "2. table index = " << i << "\n";
      assert(0);
      exit(1);
    }
#endif

    //cerr << "table_index(" << i << ") |-> bdd_index(" << bdd_index << ")\n";
    int table_value = map_values[(int)(table[i])];
    assert(table_value != ENDGAME_TABLE_UNKNOWN);

    if (unreachable.size() && unreachable[i])
      table_value = ENDGAME_TABLE_ILLEGAL;

    if (conv[table_value + 128] == -1)
      conv[table_value + 128] = ++last_index;
    bdd_table[bdd_index] = conv[table_value + 128];
  }


  // Construct convert_table.
  convert_table = vector<char>(last_index+1);
  for (int i=0; i<256; i++)
    if (conv[i] != -1) {
      assert(0<=conv[i]  &&  conv[i]<(int)convert_table.size());
      convert_table[conv[i]] = i-128;
    }
  for (int i=0; i<256; i++) {
    if (conv[i] != -1) {
      convert_table[conv[i]] = i-128;
    }
  }

  return bdd_table;
}


// ###########################
// ##########  KK  ###########
// ###########################
int compress_KK_table_index(vector<PiecePos> &piece_list) {
  assert(piece_list.size()==2);
  assert(piece_list[0].pos < 64  &&  piece_list[1].pos < 64);
  int refl;
  return c_king_fs_index(piece_list[0].pos, piece_list[1].pos, refl);
}
void decompress_KK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462);
  assert(piece_list.size() == 2);

  piece_list[0].pos = KING_FS_POS[index].first;
  piece_list[1].pos = KING_FS_POS[index].second;
}
BDD_Index preprocess_KK_bdd_index(const vector<PiecePos> &piece_list) {
  assert(piece_list.size()==2);
  assert(piece_list[0].pos < 64  &&  piece_list[1].pos < 64);

  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[1].pos);
  result[0] = ir.free_king();
  result[1] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KK,2)



// ############################
// ##########  KXK  ###########
// ############################
int compress_KXK_table_index(vector<PiecePos> &piece_list) {
  assert(piece_list.size()==3);
  // cerr << piece_list[0] << ", " << piece_list[1] << ", " << piece_list[2] << "\n";
  assert(piece_list[0].pos < 64);
  assert(piece_list[1].pos < 64);
  assert(piece_list[2].pos < 64);
  int refl;
  int result = c_king_fs_index(piece_list[0].pos, piece_list[2].pos, refl) << 6;
  return result | reflect(piece_list[1].pos, refl);
}
void decompress_KXK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*64  &&
      piece_list.size() == 3);

  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_FS_POS[index].first;
  piece_list[2].pos = KING_FS_POS[index].second;
}
BDD_Index preprocess_KXK_bdd_index(const vector<PiecePos> &piece_list) {
  assert(piece_list.size()==3);
  assert(piece_list[0].pos < 64  &&  piece_list[1].pos < 64  &&  piece_list[2].pos < 64);

  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[2].pos);
  result[0] = reflect(piece_list[1].pos, ir.refl);
  result[1] = ir.free_king();
  result[2] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXK,3)

// ############################
// ##########  KPK  ###########
// ############################
int compress_KPK_table_index(vector<PiecePos>& piece_list) {
  assert(piece_list.size()==3);
  assert(piece_list[0].pos < 64  &&
      8<=piece_list[1].pos && piece_list[1].pos < 56  &&
      piece_list[2].pos < 64);
  int refl;
  int result = 48 * c_king_ps_index(piece_list[0].pos, piece_list[2].pos, refl);
  return result + reflect(piece_list[1].pos - 8, refl);
}
void decompress_KPK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index && index<1806*48  &&
      piece_list.size() == 3);

  int kings_index = index / 48;
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[2].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * 48;

  piece_list[1].pos = index + 8;
}
BDD_Index preprocess_KPK_bdd_index(const vector<PiecePos> &piece_list) {
  assert(piece_list.size()==3);
  assert(piece_list[0].pos < 64  &&
      8 <= piece_list[1].pos  &&  piece_list[1].pos < 56  &&
      piece_list[2].pos < 64);

  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[2].pos);
  result[0] = reflect(piece_list[1].pos - 8, ir.refl);
  result[1] = ir.free_king();
  result[2] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KPK,3)

// #############################
// ##########  KXXK  ###########
// #############################
int compress_KXXK_table_index(vector<PiecePos>& piece_list) {
  assert(piece_list.size() == 4  &&  piece_list[1].pos != piece_list[2].pos);
  int refl;
  int result = (63*64/2) * c_king_fs_index(piece_list[0].pos, piece_list[3].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);

  int index = result + xx_compress(reflp1, reflp2);

  assert(0<=index  &&  index<462*(63*64/2));
  return index;
}
void decompress_KXXK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*(63*64/2));

  int kings_index = index / (63*64/2);
  piece_list[0].pos = KING_FS_POS[kings_index].first;
  piece_list[3].pos = KING_FS_POS[kings_index].second;
  index -= kings_index * (63*64/2);

  pair<Position, Position> tmp = XX_DECOMPRESS[index];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
}
BDD_Index preprocess_KXXK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[1].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  if (result[0] > result[1]) swap(result[0], result[1]);
  result[2] = ir.free_king();
  result[3] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXXK,4)

// #############################
// ##########  KXYK  ###########
// #############################
int compress_KXYK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = c_king_fs_index(piece_list[0].pos, piece_list[3].pos, refl) << 12;
  return result | (reflect(piece_list[1].pos, refl)<<6) | reflect(piece_list[2].pos, refl);
}
void decompress_KXYK_table_index(int index, vector<PiecePos>& piece_list) {
  piece_list[2].pos = index & 0x3F;
  index >>= 6;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_FS_POS[index].first;
  piece_list[3].pos = KING_FS_POS[index].second;
}
BDD_Index preprocess_KXYK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[1].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = ir.free_king();
  result[3] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXYK,4)

// #############################
// ##########  KXKY  ###########
// #############################
int compress_KXKY_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = c_king_fs_index(piece_list[0].pos, piece_list[2].pos, refl) << 12;
  return result | (reflect(piece_list[1].pos, refl)<<6) | reflect(piece_list[3].pos, refl);
}
void decompress_KXKY_table_index(int index, vector<PiecePos>& piece_list) {
  piece_list[3].pos = index & 0x3F;
  index >>= 6;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_FS_POS[index].first;
  piece_list[2].pos = KING_FS_POS[index].second;
}
BDD_Index preprocess_KXKY_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[2].pos);
  result[0] = reflect(piece_list[1].pos, ir.refl);
  result[1] = reflect(piece_list[3].pos, ir.refl);
  result[2] = ir.free_king();
  result[3] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXKY,4)

// #############################
// ##########  KPPK  ###########
// #############################
int compress_KPPK_table_index(vector<PiecePos>& piece_list) {
  assert(piece_list.size() == 4);
  assert(piece_list[0].pos<64  &&
      8<=piece_list[1].pos  &&  piece_list[1].pos<56  &&
      8<=piece_list[2].pos  &&  piece_list[2].pos<56  &&
      piece_list[3].pos<64);

  int refl;
  int result = (47*48/2) * c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);

  int index = result + pp_compress(reflp1, reflp2);

  assert(0<=index  &&  index<1806*(47*48/2));
  return index;
}
void decompress_KPPK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(piece_list.size()==4);
  assert(0<=index  &&  index<1806*(47*48/2));

  int kings_index = index / (47*48/2);
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[3].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (47*48/2);

  pair<Position, Position> tmp = PP_DECOMPRESS[index];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
}
BDD_Index preprocess_KPPK_bdd_index(const vector<PiecePos> &piece_list) {
  assert(piece_list.size() == 4);
  assert(piece_list[0].pos<64  &&
      8<=piece_list[1].pos  &&  piece_list[1].pos<56  &&
      8<=piece_list[2].pos  &&  piece_list[2].pos<56  &&
      piece_list[3].pos<64);

  // The piece X with lowest square number (AFTER reflection)
  // is stored in index 1 (the other in index 0)
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[1].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  if (result[0] > result[1]) swap(result[0], result[1]);
  result[2] = ir.free_king();
  result[3] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KPPK,4)

// #############################
// ##########  KXPK  ###########
// #############################
int compress_KXPK_table_index(vector<PiecePos>& piece_list) {
  assert(piece_list.size() == 4);
  assert(piece_list[0].pos<64  &&
      piece_list[1].pos<64  &&
      8<=piece_list[2].pos  &&  piece_list[2].pos<56  &&
      piece_list[3].pos<64);

  int refl;
  int result = 64*48*c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);
  return result + 48*reflect(piece_list[1].pos, refl) + reflect(piece_list[2].pos-8, refl);
}
void decompress_KXPK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(piece_list.size()==4);
  assert(0<=index  &&  index<1806*64*48);

  piece_list[2].pos = (index % 48) + 8;
  index /= 48;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_PS_POS[index].first;
  piece_list[3].pos = KING_PS_POS[index].second;
}
BDD_Index preprocess_KXPK_bdd_index(const vector<PiecePos> &piece_list) {
  assert(piece_list.size() == 4);
  assert(piece_list[0].pos<64  &&
      piece_list[1].pos<64  &&
      8<=piece_list[2].pos  &&  piece_list[2].pos<56  &&
      piece_list[3].pos<64);

  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[1].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  result[2] = ir.free_king();
  result[3] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXPK,4)

// #############################
// ##########  KPKP  ###########
// #############################
int compress_KPKP_table_index(vector<PiecePos>& piece_list) {
  // always white to move
  int refl;
  int result = 48*48*c_king_ps_index(piece_list[0].pos, piece_list[2].pos, refl);
  return result + 48*reflect(piece_list[1].pos-8, refl) + reflect(piece_list[3].pos-8, refl);
}
void decompress_KPKP_table_index(int index, vector<PiecePos>& piece_list) {
  int kings_index = index / (48*48);
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[2].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (48*48);

  piece_list[3].pos = (index % 48) + 8;
  piece_list[1].pos = (index / 48) + 8;
}
BDD_Index preprocess_KPKP_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[2].pos);
  result[0] = reflect(piece_list[1].pos-8, ir.refl);
  result[1] = reflect(piece_list[3].pos-8, ir.refl);
  result[2] = ir.free_king();
  result[3] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KPKP,4)

// #############################
// ##########  KXKP  ###########
// #############################
int compress_KXKP_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 64*48 * c_king_ps_index(piece_list[0].pos, piece_list[2].pos, refl);
  return result + 48*reflect(piece_list[1].pos, refl) + reflect(piece_list[3].pos-8, refl);
}
void decompress_KXKP_table_index(int index, vector<PiecePos>& piece_list) {
  piece_list[3].pos = (index % 48) + 8;
  index /= 48;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_PS_POS[index].first;
  piece_list[2].pos = KING_PS_POS[index].second;
}
BDD_Index preprocess_KXKP_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[2].pos);
  result[0] = reflect(piece_list[1].pos, ir.refl);
  result[1] = reflect(piece_list[3].pos-8, ir.refl);
  result[2] = ir.free_king();
  result[3] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXKP,4)


#ifdef ALLOW_5_MEN_ENDGAME

// ##############################
// ##########  KXXXK  ###########
// ##############################
int compress_KXXXK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = (62*63*64/6) * c_king_fs_index(piece_list[0].pos, piece_list[4].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);
  int reflp3 = reflect(piece_list[3].pos, refl);

  int index = result + xxx_compress(reflp1, reflp2, reflp3);

  assert(0<=index  &&  index<462*(62*63*64/6));
  return index;
}
void decompress_KXXXK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*(62*63*64/6));

  int kings_index = index / (62*63*64/6);
  piece_list[0].pos = KING_FS_POS[kings_index].first;
  piece_list[4].pos = KING_FS_POS[kings_index].second;
  index -= kings_index * (62*63*64/6);

  triple<Position, Position, Position> tmp = XXX_DECOMPRESS[index];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
  piece_list[3].pos = tmp.third;
}
BDD_Index preprocess_KXXXK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[4].pos);
  result[0] = reflect(piece_list[1].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[3].pos, ir.refl);
  if (result[0] > result[1]) swap(result[0], result[1]);
  if (result[1] > result[2]) swap(result[1], result[2]);
  if (result[0] > result[1]) swap(result[0], result[1]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXXXK,5)


// ##############################
// ##########  KPPPK  ###########
// ##############################
int compress_KPPPK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = (46*47*48/6) * c_king_ps_index(piece_list[0].pos, piece_list[4].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);
  int reflp3 = reflect(piece_list[3].pos, refl);

  int index = result + ppp_compress(reflp1, reflp2, reflp3);

  assert(0<=index  &&  index<1806*(46*47*48/6));
  return index;
}
void decompress_KPPPK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*(46*47*48/6));

  int kings_index = index / (46*47*48/6);
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[4].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (46*47*48/6);

  triple<Position, Position, Position> tmp = PPP_DECOMPRESS[index];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
  piece_list[3].pos = tmp.third;
}
BDD_Index preprocess_KPPPK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[3].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  result[2] = reflect(piece_list[1].pos-8, ir.refl);
  if (result[0] > result[1]) swap(result[0], result[1]);
  if (result[1] > result[2]) swap(result[1], result[2]);
  if (result[0] > result[1]) swap(result[0], result[1]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KPPPK,5)


// ##############################
// ##########  KXXKY  ###########
// ##############################
int compress_KXXKY_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 64 * (63*64/2) * c_king_fs_index(piece_list[0].pos, piece_list[3].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);

  int index = result + ((xx_compress(reflp1, reflp2) << 6) | reflect(piece_list[4].pos, refl));

  assert(0<=index  &&  index<462*64*(63*64/2));
  return index;
}
void decompress_KXXKY_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*(63*64/2)*64);

  piece_list[4].pos = index & 0x3F;
  index >>= 6;

  int kings_index = index / (63*64/2);
  piece_list[0].pos = KING_FS_POS[kings_index].first;
  piece_list[3].pos = KING_FS_POS[kings_index].second;
  index -= kings_index * (63*64/2);

  pair<Position, Position> tmp = XX_DECOMPRESS[index];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
}
BDD_Index preprocess_KXXKY_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  if (result[1] > result[2]) swap(result[1], result[2]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXXKY,5)

// ##############################
// ##########  KXXKP  ###########
// ##############################
int compress_KXXKP_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 48 * (63*64/2) * c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);

  int index = result + 48*xx_compress(reflp1, reflp2) + reflect(piece_list[4].pos-8, refl);

  assert(0<=index  &&  index<1806*48*(63*64/2));
  return index;
}
void decompress_KXXKP_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*48*(63*64/2));

  int kings_index = index / (48 * (63*64/2));
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[3].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (48 * (63*64/2));

  piece_list[4].pos = (index % 48) + 8;
  pair<Position, Position> tmp = XX_DECOMPRESS[index / 48];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
}
BDD_Index preprocess_KXXKP_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  if (result[1] > result[2]) swap(result[1], result[2]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXXKP,5)

// ##############################
// ##########  KPPKX  ###########
// ##############################
int compress_KPPKX_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 64 * (47*48/2) * c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);

  int reflp1 = reflect(piece_list[1].pos-8, refl);
  int reflp2 = reflect(piece_list[2].pos-8, refl);

  int index = result + (pp_compress(reflp1, reflp2) << 6) | reflect(piece_list[4].pos, refl);

  assert(0<=index  &&  index<1806*64*(47*48/2));
  return index;
}
void decompress_KPPKX_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*64*(47*48/2));

  piece_list[4].pos = index & 0x3F;
  index >>= 6;

  int kings_index = index / (47*48/2);
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[3].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (47*48/2);

  pair<Position, Position> tmp = PP_DECOMPRESS[index];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
}
BDD_Index preprocess_KPPKX_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  result[2] = reflect(piece_list[1].pos-8, ir.refl);
  if (result[1] > result[2]) swap(result[1], result[2]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KPPKX,5)

// ##############################
// ##########  KPPKP  ###########
// ##############################
int compress_KPPKP_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 48 * (47*48/2) * c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);

  int reflp1 = reflect(piece_list[1].pos-8, refl);
  int reflp2 = reflect(piece_list[2].pos-8, refl);

  int index = result + 48*pp_compress(reflp1, reflp2) + reflect(piece_list[4].pos-8, refl);

  assert(0<=index  &&  index<1806*48*(47*48/2));
  return index;
}
void decompress_KPPKP_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*48*(47*48/2));

  int kings_index = index / (48 * (47*48/2));
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[3].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (48 * (47*48/2));

  piece_list[4].pos = (index % 48) + 8;
  pair<Position, Position> tmp = PP_DECOMPRESS[index / 48];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;

}
BDD_Index preprocess_KPPKP_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  result[2] = reflect(piece_list[1].pos-8, ir.refl);
  if (result[1] > result[2]) swap(result[1], result[2]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KPPKP,5)

// ##############################
// ##########  KXYKZ  ###########
// ##############################
int compress_KXYKZ_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 64 * 64 * 64 * c_king_fs_index(piece_list[0].pos, piece_list[3].pos, refl);

  int index = result | (reflect(piece_list[1].pos, refl) << 12) |
      (reflect(piece_list[2].pos, refl) << 6) | reflect(piece_list[4].pos, refl);

  assert(0<=index  &&  index<462*64*64*64);
  return index;
}
void decompress_KXYKZ_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*64*64*64);

  piece_list[4].pos = index & 0x3F;
  index >>= 6;
  piece_list[2].pos = index & 0x3F;
  index >>= 6;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_FS_POS[index].first;
  piece_list[3].pos = KING_FS_POS[index].second;
}
BDD_Index preprocess_KXYKZ_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXYKZ,5)

// ##############################
// ##########  KXYKP  ###########
// ##############################
int compress_KXYKP_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);
  int index = 48*((result << 12) |
      (reflect(piece_list[1].pos, refl) << 6) |
      reflect(piece_list[2].pos, refl)) + reflect(piece_list[4].pos-8, refl);

  assert(0<=index  &&  index<1806*64*64*48);
  return index;
}
void decompress_KXYKP_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*64*64*48);

  piece_list[4].pos = (index % 48) + 8;
  index /= 48;
  piece_list[2].pos = index & 0x3F;
  index >>= 6;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_PS_POS[index].first;
  piece_list[3].pos = KING_PS_POS[index].second;
}
BDD_Index preprocess_KXYKP_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXYKP,5)

// ##############################
// ##########  KXPKY  ###########
// ##############################
int compress_KXPKY_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);
  int index = 48*64*((result << 6) | reflect(piece_list[1].pos, refl))
        + ((reflect(piece_list[2].pos-8, refl) << 6) | reflect(piece_list[4].pos, refl));

  assert(0<=index  &&  index<1806*64*64*48);
  return index;
}
void decompress_KXPKY_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*64*64*48);

  piece_list[4].pos = index & 0x3F;
  index >>= 6;
  piece_list[2].pos = (index % 48) + 8;
  index /= 48;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_PS_POS[index].first;
  piece_list[3].pos = KING_PS_POS[index].second;
}
BDD_Index preprocess_KXPKY_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXPKY,5)

// ##############################
// ##########  KXPKP  ###########
// ##############################
int compress_KXPKP_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = c_king_ps_index(piece_list[0].pos, piece_list[3].pos, refl);
  int index = 48*48*((result << 6) | reflect(piece_list[1].pos, refl))
        + 48*reflect(piece_list[2].pos-8, refl) + reflect(piece_list[4].pos-8, refl);

  assert(0<=index  &&  index<1806*64*48*48);
  return index;
}
void decompress_KXPKP_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*64*48*48);

  piece_list[4].pos = (index % 48) + 8;
  index /= 48;
  piece_list[2].pos = (index % 48) + 8;
  index /= 48;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_PS_POS[index].first;
  piece_list[3].pos = KING_PS_POS[index].second;
}
BDD_Index preprocess_KXPKP_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[3].pos);
  result[0] = reflect(piece_list[4].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXPKP,5)


// ##############################
// ##########  KXXYK  ###########
// ##############################
int compress_KXXYK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 64 * (63*64/2) * c_king_fs_index(piece_list[0].pos, piece_list[4].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);

  int index = result + ((xx_compress(reflp1, reflp2) << 6) | reflect(piece_list[3].pos, refl));

  assert(0<=index  &&  index<462*64*(63*64/2));
  return index;
}
void decompress_KXXYK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*(63*64/2)*64);

  piece_list[3].pos = index & 0x3F;
  index >>= 6;

  int kings_index = index / (63*64/2);
  piece_list[0].pos = KING_FS_POS[kings_index].first;
  piece_list[4].pos = KING_FS_POS[kings_index].second;
  index -= kings_index * (63*64/2);

  pair<Position, Position> tmp = XX_DECOMPRESS[index];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
}
BDD_Index preprocess_KXXYK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[4].pos);
  result[0] = reflect(piece_list[3].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  if (result[1] > result[2]) swap(result[1], result[2]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXXYK,5)

// ##############################
// ##########  KXXPK  ###########
// ##############################
int compress_KXXPK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 48 * (63*64/2) * c_king_ps_index(piece_list[0].pos, piece_list[4].pos, refl);

  int reflp1 = reflect(piece_list[1].pos, refl);
  int reflp2 = reflect(piece_list[2].pos, refl);

  int index = result + 48*xx_compress(reflp1, reflp2) + reflect(piece_list[3].pos-8, refl);

  assert(0<=index  &&  index<1806*48*(63*64/2));
  return index;
}
void decompress_KXXPK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*48*(63*64/2));

  int kings_index = index / (48 * (63*64/2));
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[4].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (48 * (63*64/2));

  piece_list[3].pos = (index % 48) + 8;
  pair<Position, Position> tmp = XX_DECOMPRESS[index / 48];
  piece_list[1].pos = tmp.first;
  piece_list[2].pos = tmp.second;
}
BDD_Index preprocess_KXXPK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[4].pos);
  result[0] = reflect(piece_list[3].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  if (result[1] > result[2]) swap(result[1], result[2]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXXPK,5)

// ##############################
// ##########  KXYYK  ###########
// ##############################
int compress_KXYYK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 64 * (63*64/2) * c_king_fs_index(piece_list[0].pos, piece_list[4].pos, refl);

  int reflp1 = reflect(piece_list[2].pos, refl);
  int reflp2 = reflect(piece_list[3].pos, refl);

  int index = result + (63*64/2)*reflect(piece_list[1].pos, refl) + xx_compress(reflp1, reflp2);

  assert(0<=index  &&  index<462*64*(63*64/2));
  return index;
}
void decompress_KXYYK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*(63*64/2)*64);

  int kings_index = index / ((63*64/2) * 64);
  piece_list[0].pos = KING_FS_POS[kings_index].first;
  piece_list[4].pos = KING_FS_POS[kings_index].second;
  index -= kings_index * ((63*64/2) * 64);

  pair<Position, Position> tmp = XX_DECOMPRESS[index % (63*64/2)];
  piece_list[2].pos = tmp.first;
  piece_list[3].pos = tmp.second;
  piece_list[1].pos = index / (63*64/2);
}
BDD_Index preprocess_KXYYK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[4].pos);
  result[0] = reflect(piece_list[3].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  if (result[0] > result[1]) swap(result[0], result[1]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXYYK,5)

// ##############################
// ##########  KXPPK  ###########
// ##############################
int compress_KXPPK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = 64 * (47*48/2) * c_king_ps_index(piece_list[0].pos, piece_list[4].pos, refl);

  int reflp1 = reflect(piece_list[2].pos, refl);
  int reflp2 = reflect(piece_list[3].pos, refl);

  int index = result + (47*48/2)*reflect(piece_list[1].pos, refl) + pp_compress(reflp1, reflp2);

  assert(0<=index  &&  index<1806*64*(47*48/2));
  return index;
}
void decompress_KXPPK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*64*(47*48/2));

  int kings_index = index / (64 * (47*48/2));
  piece_list[0].pos = KING_PS_POS[kings_index].first;
  piece_list[4].pos = KING_PS_POS[kings_index].second;
  index -= kings_index * (64 * (47*48/2));

  pair<Position, Position> tmp = PP_DECOMPRESS[index % (47*48/2)];
  piece_list[2].pos = tmp.first;
  piece_list[3].pos = tmp.second;
  piece_list[1].pos = index / (47*48/2);
}
BDD_Index preprocess_KXPPK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[4].pos);
  result[0] = reflect(piece_list[3].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos-8, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  if (result[0] > result[1]) swap(result[0], result[1]);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXPPK,5)

// ##############################
// ##########  KXYZK  ###########
// ##############################
int compress_KXYZK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = c_king_fs_index(piece_list[0].pos, piece_list[4].pos, refl) << 18;

  int index = result | (reflect(piece_list[1].pos, refl) << 12) |
      (reflect(piece_list[2].pos, refl) << 6) | reflect(piece_list[3].pos, refl);

  assert(0<=index  &&  index<462*64*64*64);
  return index;
}
void decompress_KXYZK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<462*64*64*64);

  piece_list[3].pos = index & 0x3F;
  index >>= 6;
  piece_list[2].pos = index & 0x3F;
  index >>= 6;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_FS_POS[index].first;
  piece_list[4].pos = KING_FS_POS[index].second;
}
BDD_Index preprocess_KXYZK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[4].pos);
  result[0] = reflect(piece_list[3].pos, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXYZK,5)

// ##############################
// ##########  KXYPK  ###########
// ##############################
int compress_KXYPK_table_index(vector<PiecePos>& piece_list) {
  int refl;
  int result = c_king_ps_index(piece_list[0].pos, piece_list[4].pos, refl) < 12;

  int index = 48*(result | (reflect(piece_list[1].pos, refl) << 6) |
      reflect(piece_list[2].pos, refl)) + reflect(piece_list[3].pos-8, refl);

  assert(0<=index  &&  index<1806*64*64*48);
  return index;
}
void decompress_KXYPK_table_index(int index, vector<PiecePos>& piece_list) {
  assert(0<=index  &&  index<1806*64*64*48);

  piece_list[3].pos = (index % 48) + 8;
  index /= 48;
  piece_list[2].pos = index & 0x3F;
  index >>= 6;
  piece_list[1].pos = index & 0x3F;
  index >>= 6;

  piece_list[0].pos = KING_PS_POS[index].first;
  piece_list[4].pos = KING_PS_POS[index].second;
}
BDD_Index preprocess_KXYPK_bdd_index(const vector<PiecePos> &piece_list) {
  BDD_Index result;
  BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[4].pos);
  result[0] = reflect(piece_list[3].pos-8, ir.refl);
  result[1] = reflect(piece_list[2].pos, ir.refl);
  result[2] = reflect(piece_list[1].pos, ir.refl);
  result[3] = ir.free_king();
  result[4] = ir.remapped_bound_king();
  return result;
}
TABLE_INDEX_TO_BDD_INDEX(KXYPK,5)

#endif
