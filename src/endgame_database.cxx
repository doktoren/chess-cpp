#include <set>
#include <string>
#include <algorithm>
#include <assert.h>
#include <fcntl.h>
#include <time.h>

#include "endgame_database.hxx"
#include "endgame_table_bdd.hxx"
#include "endgame_piece_enumerations.hxx"

#include "endgame_run_length_encoding.hxx"
#include "mapping_of_wildcards.hxx"

// Implementation:

#include "endgame_en_passant.hxx"
#include "endgame_indexing.hxx"

//#include "algorithm_best_permutation.hxx"

string endgame_value_to_string(char v) {
  if (is_special_value(v)) {
    switch (v) {
    case ENDGAME_TABLE_WIN:
      return "WIN";
    case ENDGAME_TABLE_LOSS:
      return "LOSS";
    case ENDGAME_TABLE_DRAW:
      return "DRAW";
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

template <class INT_TYPE>
void print_map(INT_TYPE *m, int digits_per_num = 2) {
  for (int i=0; i<8*(digits_per_num+1)+3; i++) cerr << '+'; cerr << '\n';
  for (int r=7; r>=0; r--) {
    cerr << '+';
    for (int c=0; c<8; c++) {
      int tmp = m[(r<<3)|c];
      int f = 10;
      for (int i=1; i<digits_per_num; i++) {
        if (f>tmp) cerr << ' ';
        f*=10;
      }
      cerr << tmp << ' ';
    }
    cerr << "+\n";
  }
  for (int i=0; i<8*(digits_per_num+1)+3; i++) cerr << '+'; cerr << '\n';
}


// Sorting order : KQRBNPkqrbnp
const bool GT[16][16] =
{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0,1,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,1,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,0,1,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

void sort_piece_pos(vector<PiecePos> &pieces) {
  switch(pieces.size()) {
  case 2:
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    return;
  case 3:
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    if (GT[pieces[1].piece][pieces[2].piece]) swap(pieces[1], pieces[2]);
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    return;
  case 4:
    // Partial sort pieces[0..1] and pieces[2..3]
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    if (GT[pieces[2].piece][pieces[3].piece]) swap(pieces[2], pieces[3]);

    // Find smallest and largest element
    if (GT[pieces[0].piece][pieces[2].piece]) swap(pieces[0], pieces[2]);
    if (GT[pieces[1].piece][pieces[3].piece]) swap(pieces[1], pieces[3]);

    // Sort middle elements
    if (GT[pieces[1].piece][pieces[2].piece]) swap(pieces[1], pieces[2]);
    return;
#ifdef ALLOW_5_MEN_ENDGAME
  case 5:
    // start by sorting pieces[0,1,3,4] like sort_4_piece_pos
    if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);
    if (GT[pieces[3].piece][pieces[4].piece]) swap(pieces[3], pieces[4]);
    if (GT[pieces[0].piece][pieces[3].piece]) swap(pieces[0], pieces[3]);
    if (GT[pieces[1].piece][pieces[4].piece]) swap(pieces[1], pieces[4]);
    if (GT[pieces[1].piece][pieces[3].piece]) swap(pieces[1], pieces[3]);

    // find the place for pieces[2]
    if (GT[pieces[1].piece][pieces[2].piece]) {
      swap(pieces[1], pieces[2]);
      if (GT[pieces[0].piece][pieces[1].piece]) swap(pieces[0], pieces[1]);

    } else if (GT[pieces[2].piece][pieces[3].piece]) {

      swap(pieces[2], pieces[3]);
      if (GT[pieces[3].piece][pieces[4].piece]) swap(pieces[3], pieces[4]);
    }

    assert(!GT[pieces[0].piece][pieces[1].piece]);
    assert(!GT[pieces[1].piece][pieces[2].piece]);
    assert(!GT[pieces[2].piece][pieces[3].piece]);
    assert(!GT[pieces[3].piece][pieces[4].piece]);

    return;
#endif
  }
  assert(0);

}

bool swap_piece_pos(vector<PiecePos> &pieces,
    bool symmetric_endgame_and_btm)
{
  //cerr << "pieces.size() = " << pieces.size() << "\n";

  switch (pieces.size()) {
  case 2:
    // NEW!
    if (symmetric_endgame_and_btm) swap(pieces[0], pieces[2]);
    return symmetric_endgame_and_btm;
  case 3:
    if (PIECE_KIND[pieces[1].piece] == KING) {
      PiecePos weak_king = pieces[0];
      pieces[0] = pieces[1];
      pieces[1] = pieces[2];
      pieces[2] = weak_king;
      return true;
    }
    return false;
  case 4:
    if (PIECE_KIND[pieces[1].piece] == KING) {
      PiecePos weak_king = pieces[0];
      pieces[0] = pieces[1];
      pieces[1] = pieces[2];
      pieces[2] = pieces[3];
      pieces[3] = weak_king;
      return true;
    }
    if (PIECE_KIND[pieces[2].piece] == KING) {
      if (PIECE_KIND[pieces[1].piece] < PIECE_KIND[pieces[3].piece]) {
        swap(pieces[0], pieces[2]);
        swap(pieces[1], pieces[3]);
        return true;
      }
      // NEW!
      if (symmetric_endgame_and_btm) {
        swap(pieces[0], pieces[2]);
        swap(pieces[1], pieces[3]);
        return true;
      }
    }
    // PIECE_KIND[pieces[3]] == KING 
    return false;
#ifdef ALLOW_5_MEN_ENDGAME
  case 5:
    if (PIECE_KIND[pieces[1].piece] == KING) {
      PiecePos weak_king = pieces[0];
      pieces[0] = pieces[1];
      pieces[1] = pieces[2];
      pieces[2] = pieces[3];
      pieces[3] = pieces[4];
      pieces[4] = weak_king;
      return true;
    }
    if (PIECE_KIND[pieces[2].piece] == KING) {
      PiecePos weak_king_piece = pieces[1];
      pieces[1] = pieces[3];
      pieces[3] = pieces[0];
      pieces[0] = pieces[2];
      pieces[2] = pieces[4];
      pieces[4] = weak_king_piece;
      return true;
    }
    return false;
#endif
  }
  assert(0);
  return false;
}


EndgameFunctionality::
EndgameFunctionality(int (*compress_table_index)(vector<PiecePos> &),
    void (*decompress_table_index)(int, vector<PiecePos>&),
    BDD_Index (*preprocess_bdd_index)(const vector<PiecePos>&),
    int (*table_index_to_bdd_index)(int),
    uint table_size, string name) :
    compress_table_index(compress_table_index),
    decompress_table_index(decompress_table_index),
    preprocess_bdd_index(preprocess_bdd_index),
    table_index_to_bdd_index(table_index_to_bdd_index),
    table_size(table_size), name(name), num_pieces(name.size())
{
  table[0] = table[1] = 0;
  bdd[0] = bdd[1] = 0;

  pawnless_endgame = true;
  int ep = 0;
  for (int i=0; i<num_pieces; i++) {
    assert(!(name[i] & 32)); // Use capital letters
    if (name[i]=='P') {
      ep |= 1;
      pawnless_endgame = false;
    }
  }

  const int tmp_tbl[13] = {0,
      0x100000, 0x8000, 0x400, 0x20, 0x1, 0,
      -0x100000, -0x8000, -0x400, -0x20, -0x1, 0};
  int material_balance = 0;

  pieces[0] = WKING;
  bool white_pieces = true;
  for (int i=1; i<num_pieces; i++) {
    if (name[i]=='K') white_pieces = false;
    if (white_pieces) {
      pieces[i] = char_to_piece(name[i]);
    } else {
      pieces[i] = char_to_piece(name[i] | 32);
    }
    material_balance += tmp_tbl[pieces[i]];
  }

  symmetric_endgame = (material_balance == 0);
}

void EndgameFunctionality::destroy() {
  destroy_table();
  destroy_bdd();
}
void EndgameFunctionality::destroy(int stm) {
  destroy_table(stm);
  destroy_bdd(stm);
}
void EndgameFunctionality::destroy_table() {
  if (table[0] || table[1])
    cerr << "Destroying endgame table " << name << "\n";
  if (table[0]) { delete table[0]; table[0] = 0; }
  if (table[1]) { delete table[1]; table[1] = 0; }
}
void EndgameFunctionality::destroy_table(int stm) {
  assert(stm==0  ||  stm==1);
  if (table[stm]) {
    cerr << "Destroying endgame table " << name << (stm ? " btm\n" : " wtm\n");
    delete table[stm];
    table[stm] = 0;
  }
}
void EndgameFunctionality::destroy_bdd() {
  if (bdd[0] || bdd[1])
    cerr << "Destroying endgame bdd " << name << "\n";
  if (bdd[0]) { bdd[0]->clear(); delete bdd[0]; bdd[0] = 0; }
  if (bdd[1]) { bdd[1]->clear(); delete bdd[1]; bdd[1] = 0; }
}
void EndgameFunctionality::destroy_bdd(int stm) {
  assert(stm==0  ||  stm==1);
  if (table[stm]) {
    cerr << "Destroying endgame bdd " << name << (stm ? " btm\n" : " wtm\n");
    bdd[stm]->clear();
    delete bdd[stm];
    bdd[stm] = 0;
  }
}
/*
void EndgameFunctionality::keep_only_wtm() {
  keep_only_stm(WHITE);
}
void EndgameFunctionality::keep_only_stm(int stm) {
  assert(stm==0  ||  stm==1);
  if (symmetric_endgame) {
    cerr << name << " is a symmetric endgame - can't just keep " << PLAYER_NAME[stm] << "-to-move\n";
  } else if (stm_table_or_bdd_loaded(stm)) {
    cerr << "Restricting endgame " << name << " to " << PLAYER_NAME[stm] << "-to-move\n";
    stm ^= 1;
    if (table[stm]) { delete table[stm]; table[stm] = 0; }
    if (bdd[stm]) { bdd[stm]->clear(); delete bdd[stm]; bdd[stm] = 0; }
  }
}
 */


// If endgame is KBKP, and bishop captured by pawn is being promoted to queen,
// then s is K_KQ  (and stm is BLACK).
// Result will be the needed stm, hence BLACK in this case (B->W->B)
// Will change p if it contains a zero entry
int endgame_dependency_needed_stm(Piece *p, int num_pieces, int stm) {
  assert(stm==0  ||  stm==1);
  for (int i=0; i<num_pieces; i++) {
    if (p[i] == 0) {
      --num_pieces;
      for (int j=i; j<num_pieces; j++) {
        p[j] = p[j+1];
        assert(p[j] != 0);
      }
      break;
    }
  }

  bool symmetric = true;
  for (int i=0; i<(num_pieces>>1); i++)
    if (p[i] != p[i + (num_pieces>>1)]) {
      symmetric = false;
      break;
    }

  vector<PiecePos> pp(num_pieces);
  for (int i=0; i<num_pieces; i++)
    pp[i].piece = p[i];

  return !stm ^ swap_piece_pos(pp, stm && symmetric);
}


// Returns a mapping from endgame hashing values to previous states
map<int, EFState> EndgameFunctionality::load_dependency(int stm) {
  // Assure that the necessary tables or bdds have been loaded.
  // Eg. when constructing KRKP the following endgames must already be loaded:
  //
  // Endgame required to construct wtm table: 
  //   Capture of piece:  KRK (btm)
  // Endgames required to construct btm table:
  //   Capture of piece:  KPK (btm)
  //   Pawn promotion  :  KQKR (btm), KRKR (symmetric), KRKB (wtm), KRKN (wtm)
  //   Both at once    :  KNK (btm), KBK (btm), KQK (btm)
  // If a table or bdd is missing, then a table is constructed

  map<int, EFState> old_states;

  int hash_value = 0;
  for (int i=0; i<num_pieces; i++)
    hash_value += ENDGAME_HASHING_CONSTANTS[pieces[i]];

  Piece _pieces[MAX_MEN];

  int piece_stm = WHITE;
  for (int i=1; i<num_pieces; i++) {
    if (PIECE_KIND[pieces[i]] == KING) {
      piece_stm = BLACK;
    } else {
      if (piece_stm != stm) {
        // This piece can be captured
        int hv = hash_value - ENDGAME_HASHING_CONSTANTS[pieces[i]];

        memcpy(_pieces, pieces, MAX_MEN);
        _pieces[i] = 0;
        int needed_stm = endgame_dependency_needed_stm(_pieces, num_pieces, stm);

        if (!endgames[hv].table_or_bdd_loaded(needed_stm)) {
          cerr << "Construction of " << name << (stm ? " btm" : " wtm") << " requires "
              << endgames[hv].name  << (needed_stm ? " btm" : " wtm") << " to be loaded.\n";
          old_states[hv] = endgames[hv].get_state();
          endgames[hv].load_table(true, true, needed_stm);
        }
      } else if (PIECE_KIND[pieces[i]] == PAWN) {
        // This piece can be promoted
        for (int j=1; j<5; j++) {
          // Promoted to piece pieces[i]+j
          int hv = hash_value - ENDGAME_HASHING_CONSTANTS[pieces[i]]
                                                          + ENDGAME_HASHING_CONSTANTS[pieces[i] + j];

          memcpy(_pieces, pieces, MAX_MEN);
          _pieces[i] = PAWN + j;
          int  needed_stm = endgame_dependency_needed_stm(_pieces, num_pieces, stm);

          if (!endgames[hv].table_or_bdd_loaded(needed_stm)) {
            cerr << "Construction of " << name << (stm ? " btm" : " wtm") << " requires "
                << endgames[hv].name  << (needed_stm ? " btm" : " wtm") << " to be loaded.\n";
            old_states[hv] = endgames[hv].get_state();
            endgames[hv].load_table(true, true, needed_stm);
          }

          // It can be promoted while capturing an opponent piece!
          for (int ii=1; ii<num_pieces; ii++) {
            if (PIECE_COLOR[pieces[i]] != PIECE_COLOR[pieces[ii]]  &&
                PIECE_KIND[pieces[ii]] != KING  &&
                PIECE_KIND[pieces[ii]] != PAWN) {
              int hv2 = hv - ENDGAME_HASHING_CONSTANTS[pieces[ii]];

              memcpy(_pieces, pieces, MAX_MEN);
              _pieces[i] = PAWN + j;
              _pieces[ii] = 0;
              needed_stm = endgame_dependency_needed_stm(_pieces, num_pieces, stm);

              if (!endgames[hv2].table_or_bdd_loaded(needed_stm)) {
                cerr << "Construction of " << name << (stm ? " btm" : " wtm") << " requires "
                    << endgames[hv].name  << (needed_stm ? " btm" : " wtm") << " to be loaded.\n";
                old_states[hv2] = endgames[hv2].get_state();
                endgames[hv2].load_table(true, true, needed_stm);
              }
            }
          }
        }
      }
    }
  }

  return old_states;
}

void EndgameFunctionality::release_dependency(const map<int, EFState> &old_states) {
  map<int, EFState>::const_iterator i = old_states.begin();
  while (i != old_states.end()) {
    endgames[i->first].restore_state(i->second);
    i++;
  }
}


bool EndgameFunctionality::load_table(bool restrict_to_stm, bool build_if_nescessary,
    int restricted_stm) {
  assert(restricted_stm==0  ||  restricted_stm==1);
  cerr << "EndgameFunctionality::load_table(r_t_stm " << restrict_to_stm
      << ", b_i_n " << build_if_nescessary << ", r_stm " << restricted_stm << ")(" << name << ")\n";

  if (symmetric_endgame  &&  restrict_to_stm  &&  restricted_stm==BLACK)
    cerr << name << ":load_table(...): wtm is loaded instead of btm (symmetric endgame)\n";

  if (symmetric_endgame) {
    restrict_to_stm = true;
    restricted_stm = WHITE;
  }

  // Is the table already loaded?
  if (table[restricted_stm]  &&  (restrict_to_stm  ||  table[restricted_stm^1]))
    return true;


  { // Can the table be loaded from file?
    bool can_be_loaded = true;

    bool wtm_needed = (!restrict_to_stm  ||  restricted_stm==WHITE)  &&  !table[WHITE];
    bool btm_needed = (!restrict_to_stm  ||  restricted_stm==BLACK)  &&  !table[BLACK];

    if (wtm_needed)
      can_be_loaded &= file_exists(endgames.get_directory() + name + "_wtm.dat");
    if (btm_needed)
      can_be_loaded &= file_exists(endgames.get_directory() + name + "_btm.dat");

    if (can_be_loaded) {
      if (wtm_needed) {
        string filename = endgames.get_directory() + name + "_wtm.dat";
        cerr << "loading endgame " << filename << "\n";
        int fd = open(filename.c_str(), O_RDONLY, 0);
        if (fd != -1) {
          //load_define_value(fd, BOUND_KING, "BOUND_KING");
          table[WHITE] = new char[table_size];
          read(fd, table[WHITE], sizeof(char)*table_size);
          close(fd);
        }
      }
      if (btm_needed) {
        string filename = endgames.get_directory() + name + "_btm.dat";
        cerr << "loading endgame " << filename << "\n";
        int fd = open(filename.c_str(), O_RDONLY, 0);
        if (fd != -1) {
          //load_define_value(fd, BOUND_KING, "BOUND_KING");
          table[BLACK] = new char[table_size];
          read(fd, table[BLACK], sizeof(char)*table_size);
          close(fd);
        }
      }

      return true;
    }
  }

  if (!build_if_nescessary) return false;

  // It is the responsibility of the endgame construction to load and release
  // the dependency endgames.
  if (*(endgame_settings->construction_method) == 0) {
#include "endgame_simple_construction.cxx"
  } else {
#include "endgame_retrograde_construction.cxx"
  }


  { // Save tables
    { // white-to-move
      string filename = endgames.get_directory() + name + "_wtm.dat";
      cerr << "saving endgame " << filename << "\n";
      //int fd = open(filename.c_str(), O_RDONLY, 0);
      int fd = creat(filename.c_str(), 0666);
      if (fd != -1) {
        //save_define_value(fd, BOUND_KING);
        write(fd, table[0], sizeof(char)*table_size);
        close(fd);
      } else {
        cerr << "Error opening file " << filename << "\n";
      }
    }
    if (!symmetric_endgame) {
      // black-to-move
      string filename = endgames.get_directory() + name + "_btm.dat";
      cerr << "saving endgame " << filename << "\n";
      //int fd = open(filename.c_str(), O_RDONLY, 0);
      int fd = creat(filename.c_str(), 0666);
      if (fd != -1) {
        //save_define_value(fd, BOUND_KING);
        write(fd, table[1], sizeof(char)*table_size);
        close(fd);
      } else {
        cerr << "Error opening file " << filename << "\n";
      }
    }
  }

  if (restrict_to_stm)
    destroy_table(restricted_stm ^ 1);

  return true;
}

// Todo: split up such that only one side is constucted (more memory efficient)
bool EndgameFunctionality::load_bdd(bool restrict_to_stm,
    bool build_from_tables_if_nescessary,
    bool build_tables_if_nescessary,
    int restricted_stm) {
  assert(restricted_stm==0  ||  restricted_stm==1);

  if (symmetric_endgame  &&  restrict_to_stm  &&  restricted_stm==BLACK)
    cerr << name << ":load_bdd(...): wtm is loaded instead of btm (symmetric endgame)\n";

  if (symmetric_endgame) {
    restrict_to_stm = true;
    restricted_stm = WHITE;
  }

  // Is the table already loaded?
  if (bdd[restricted_stm]  &&  (restrict_to_stm  ||  bdd[restricted_stm^1]))
    return true;


  { // Try to load the bdd from files
    bool can_be_loaded = true;

    bool wtm_needed = (!restrict_to_stm  ||  restricted_stm==WHITE)  &&  !bdd[WHITE];
    bool btm_needed = (!restrict_to_stm  ||  restricted_stm==BLACK)  &&  !bdd[BLACK];

    if (wtm_needed)
      can_be_loaded &= file_exists(endgames.get_directory() + name + "_wtm.bdd");
    if (btm_needed)
      can_be_loaded &= file_exists(endgames.get_directory() + name + "_btm.bdd");

    if (can_be_loaded) {
      if (wtm_needed) {
        bdd[WHITE] = new BDD;
        bdd[WHITE]->load(endgames.get_directory() + name + "_wtm.bdd");
      }
      if (btm_needed) {
        bdd[BLACK] = new BDD;
        bdd[BLACK]->load(endgames.get_directory() + name + "_btm.bdd");
      }

      return true;
    }
  }

  if (!build_from_tables_if_nescessary) return false;

  bool wtm_table_was_loaded = table[0];
  bool btm_table_was_loaded = table[1];

  if (!load_table(restrict_to_stm, build_tables_if_nescessary, restricted_stm))
    return false;

  if (*(endgame_settings->reduce_information)) reduce_information();

  cerr << "REMINDER: most (all?) of the information shown will also be written to big_output.txt\n";

  if (!bdd[WHITE]  &&  (!restrict_to_stm  ||  restricted_stm==WHITE)) {
    bdd[WHITE] = new BDD;
    if (file_exists(endgames.get_directory() + name + "_wtm.bdd")) {
      bdd[WHITE]->load(endgames.get_directory() + name + "_wtm.bdd");
    } else {
      //cerr << "Ready to create wtm bdd 1\n";
      init_bdd(*(bdd[WHITE]), WHITE, !wtm_table_was_loaded);
      if ( *(endgame_settings->verify_bdd_with_table)) {
        load_table(true, true, WHITE);
        //compare_bdd_with_table(WHITE);//todo: only if NDEBUG not defined
        if (!wtm_table_was_loaded  &&  table[WHITE]) {
          delete table[WHITE];
          table[WHITE] = 0;
        }
      }
      //cerr << "Ready to create wtm bdd 2\n";
      bdd[WHITE]->save(endgames.get_directory() + name + "_wtm.bdd");
    }
  }

  if (!bdd[BLACK]  &&  (!restrict_to_stm  ||  restricted_stm==BLACK)) {
    bdd[BLACK] = new BDD;
    if (file_exists(endgames.get_directory() + name + "_btm.bdd")) {
      bdd[BLACK]->load(endgames.get_directory() + name + "_btm.bdd");
    } else {
      init_bdd(*(bdd[BLACK]), BLACK, !btm_table_was_loaded);
      if ( *(endgame_settings->verify_bdd_with_table)) {
        load_table(true, true, BLACK);
        //compare_bdd_with_table(BLACK);//todo: only if NDEBUG not defined
        if (!wtm_table_was_loaded  &&  table[BLACK]) {
          delete table[BLACK];
          table[BLACK] = 0;
        }
      }
      bdd[BLACK]->save(endgames.get_directory() + name + "_btm.bdd");
    }
  }

  return true;
}


void EndgameFunctionality::init_bdd(BDD &bdd, int player, bool delete_table) {
  vector<char> convert_table;

  uchar *bdd_table;

  if (*(endgame_settings->bdd_mate_depth_round_up_to_multiples_of_n) < 0  ||
      64 < *(endgame_settings->bdd_mate_depth_round_up_to_multiples_of_n)) {
    cerr << "bdd_mate_depth_round_up_to_multiples_of_n must be in [0..64]\n";
    exit(1);
  }

  if (*(endgame_settings->mark_unreachable_as_dont_cares)) {

    cerr << "Identify all unreachable positions...\n";
    BitList unreachable(table_size, false);
    find_unreachable_positions(unreachable, player,
        *(endgame_settings->unreachability_depth_test), 0);
    bdd_table =
        construct_bdd_table(table[player], table_index_to_bdd_index, table_size, calc_log_bdd_size(),
            *(endgame_settings->bdd_mate_depth_round_up_to_multiples_of_n),
            convert_table, unreachable);

  } else {

    bdd_table =
        construct_bdd_table(table[player], table_index_to_bdd_index, table_size, calc_log_bdd_size(),
            *(endgame_settings->bdd_mate_depth_round_up_to_multiples_of_n),
            convert_table, BitList());
  }

  bdd.set_convert_table(convert_table);

  if (delete_table) {
    delete table[player];
    table[player] = 0;
  }

  if (*(endgame_settings->clustering_method) != 0) {
    if (*(endgame_settings->cluster_functions_preferred)  &&
        bdd.try_using_specialized_cluster_function(calc_hash_value())) {
      // Using cluster function
    } else {

      switch(*(endgame_settings->clustering_method)) {
      case 0:
        // Nothing to do!
        break;
      case 1:
        // Use king positions to determine cluster
        bdd.use_king_pos_as_subsets(bdd_table, calc_log_bdd_size());
        break;
      case 2:
      {
        // Make each piece contribute with a factor of 6 to the number
        // of subsets, except for the bound king (3 or 4 depending on amount of symmetry)
        vector<int> base_subset_sizes(num_pieces, 6);
        if (pawnless_endgame) base_subset_sizes[num_pieces-1] -=3;
        else base_subset_sizes[num_pieces-1] -= 2;

        bdd.determine_cluster_subsets_based_on_clustering(base_subset_sizes,
            bdd_table, calc_log_bdd_size());
      }
      break;
      default:
        cerr << "Error: Setting Endgame_clustering_method has illegal value "
        << *(endgame_settings->clustering_method) << "\n";
        exit(1);
      }
    }
  }

  cerr << "permute square enumeration\n";
  uchar inv_bit_perm[5][64];
  memset(inv_bit_perm, 0, 4*64);
  { // permute square enumeration
    vector<vector<int> > permutations(num_pieces);
    for (int p=0; p<num_pieces; p++) {
      permutations[p] = vector<int>(64);
      uchar *m = 0;
      switch (pieces[p]) {
      case WHITE_PAWN:
        m = mappings[*(endgame_settings->square_enum_white_pawn)];
        break;
      case WHITE_KNIGHT:
        m = mappings[*(endgame_settings->square_enum_white_knight)];
        break;
      case WHITE_BISHOP:
        m = mappings[*(endgame_settings->square_enum_white_bishop)];
        break;
      case WHITE_ROOK:
        m = mappings[*(endgame_settings->square_enum_white_rook)];
        break;
      case WHITE_QUEEN:
        m = mappings[*(endgame_settings->square_enum_white_queen)];
        break;
      case WHITE_KING:
        m = mappings[*(endgame_settings->square_enum_white_king)];
        break;
      case BLACK_PAWN:
        m = mappings[*(endgame_settings->square_enum_black_pawn)];
        break;
      case BLACK_KNIGHT:
        m = mappings[*(endgame_settings->square_enum_black_knight)];
        break;
      case BLACK_BISHOP:
        m = mappings[*(endgame_settings->square_enum_black_bishop)];
        break;
      case BLACK_ROOK:
        m = mappings[*(endgame_settings->square_enum_black_rook)];
        break;
      case BLACK_QUEEN:
        m = mappings[*(endgame_settings->square_enum_black_queen)];
        break;
      case BLACK_KING:
        m = mappings[*(endgame_settings->square_enum_black_king)];
        break;
      default:
        cerr << "Wrong piece value = " << (int)(pieces[p]) << "\n";
        assert(0);
        exit(1);
      }

      for (int i=0; i<64; i++) {
        permutations[p][i] = m[i];
        inv_bit_perm[p][m[i]] = i;
      }
    }

    bdd.permute_square_enumeration(bdd_table, calc_log_bdd_size(), permutations);
  }

  if (*(endgame_settings->output_bdd_tables)) {
    string filename = endgames.get_directory() + name + (player ? "_btm" : "_wtm") + "_bdd_table.test";
    cerr << "Saving raw bdd table as " << filename << "\n";
    int fd = creat(filename.c_str(), 0666);
    if (fd != -1) {
      //save_define_value(fd, BOUND_KING);
      write(fd, bdd_table, sizeof(uchar) << calc_log_bdd_size());
      close(fd);
    } else {
      cerr << "Error opening file " << filename << "\n";
    }
  }

  if (*(endgame_settings->clustering_method) == 0) {
    //No clustering
    bdd.init_no_clustering(bdd_table, calc_log_bdd_size(),
        *(endgame_settings->do_preprocessing),
        *(endgame_settings->calc_sifting),
        *(endgame_settings->do_preprocessing_after_sifting));
  } else {
    bdd.init(bdd_table, calc_log_bdd_size(), inv_bit_perm,
        *(endgame_settings->do_preprocessing), *(endgame_settings->calc_sifting),
        *(endgame_settings->do_preprocessing_after_sifting));
  }

  if (*(endgame_settings->do_preprocessing)  &&
      *(endgame_settings->output_preprocessed_bdd_tables)) {
    string filename = endgames.get_directory() + name + (player ? "_btm" : "_wtm") + "_p_bdd_table.test";
    cerr << "Saving preprocessed raw bdd table as " << filename << "\n";
    int fd = creat(filename.c_str(), 0666);
    if (fd != -1) {
      //save_define_value(fd, BOUND_KING);
      write(fd, bdd_table, sizeof(uchar) << calc_log_bdd_size());
      close(fd);
    } else {
      cerr << "Error opening file " << filename << "\n";
    }
  }

  delete bdd_table;
}


// Example: The endgame KRPKB matches among others the strings
// KRPKB, KXPKY, K2K1, K2KX,
bool EndgameFunctionality::name_match(string name_pattern) {
  // divide name and _name into white parts and black parts
  string::size_type tmp;

  if (name_pattern == "*") name_pattern = "K*K*";

  tmp = name_pattern.find('K', 1);
  if (tmp == string::npos) return false;
  string np[2];
  np[WHITE] = name_pattern.substr(0, tmp);
  np[BLACK] = name_pattern.substr(tmp);

  tmp = name.find('K', 1);
  assert(tmp != string::npos);
  string n[2];
  n[WHITE] = name.substr(0, tmp);
  n[BLACK] = name.substr(tmp);

  bool match = true;
  char next = 'X';
  for (int side=0; side<2; side++) {
    if (n[side] == np[side]) continue;
    if (np[side] == "K*") continue;
    if (np[side].size()==2  &&
        (int)n[side].size() == 1+(np[side][1] - '0')) continue;


    string _n = n[side];
    for (uint i=1; i<_n.size(); i++) {
      assert(n[side][i] != 'K');
      if (n[side][i-1] == n[side][i]) {
        // Use last naming
        _n[i] = _n[i-1];
      } else {
        if (n[side][i] == 'P') {
          _n[i] = 'P';
        } else {
          _n[i] = next++;
        }
      }
    }
    if (_n == np[side]) continue;

    match = false;
    break;
  }

  return match;
}


int EndgameFunctionality::legal_position(int player, int table_index) {
  Board2 board;
  vector<PiecePos> piece_list(num_pieces);
  for (int i=0; i<num_pieces; i++)
    piece_list[i].piece = pieces[i];

  (*decompress_table_index)(table_index, piece_list);
  if (!board.set_board(piece_list, player)) return 0;

  if (board.unreachable_position()) return 1;

  int test_depth = *(endgame_settings->unreachability_depth_test);
  if (test_depth > 0  &&  board.unreachable_position(test_depth)) return 1;

  return 2;
}

void EndgameFunctionality::compare_bdd_with_table(int player) {
  cerr << "compare_bdd_with_table(" << player << ")\n";
  if (!bdd[player] || !table[player]) {
    cerr << "Table or bdd not loaded\n";
    return;
  }

  char _map_values[256];
  char *map_values = &(_map_values[128]);
  {
    int mult = *(endgame_settings->bdd_mate_depth_round_up_to_multiples_of_n);
    if (mult < 0  ||  64 < mult) {
      cerr << "bdd_mate_depth_round_up_to_multiples_of_n must be in [0..64]!\n";
      exit(1);
    }

    for (int i=-128; i<-123; i++)
      map_values[i] = i;
    if (!mult) {
      for (int i=-123; i<=0; i++)
        map_values[i] = ENDGAME_TABLE_LOSS;
      for (int i=1; i<128; i++)
        map_values[i] = ENDGAME_TABLE_WIN;
    } else {
      for (int i=-123; i<=0; i++) {
        int tmp = mult*(-((-i + (mult-1))/mult));
        map_values[i] = (tmp < -123) ? -123 : tmp;
      }
      for (int i=1; i<128; i++) {
        int tmp = mult*((i + (mult-1))/mult);
        map_values[i] = (tmp > 123) ? 123 : tmp;
      }
    }
  }

  bool differed_on_unreachable_positions = false;
  int error_count = 0;
  for (uint i=0; i<table_size; i++) {
    if (table[player][i] != ENDGAME_TABLE_ILLEGAL) {
      int bdd_index = table_index_to_bdd_index(i);

      char table_value = map_values[table[player][i]];
      char bdd_value = (*(bdd[player]))[bdd_index];

      if (table_value != bdd_value) {
        if (legal_position(player, i) == 1) {
          differed_on_unreachable_positions = true;
        } else {
          ++error_count;
          if (!(error_count & (error_count-1)))
            cerr << "Error number " << error_count << ": (" << (player ? "btm" : "wtm") << "): "
            << endgame_value_to_string(table_value) << " = table[" << i << "] != bdd["
            << bdd_index << "] = " << endgame_value_to_string(bdd_value) << "\n"
            << "(No more errors are shown).\n";
        }
      }
    }
  }
  if (error_count != 0) {
    cerr << "table and bdd differed on " << error_count << " entries.\n";
    exit(1);
  }
  if (differed_on_unreachable_positions) {
    cerr << "OBDD and table differed only on unreachable positions.\n";
  } else {
    cerr << "OBDD and table agreed everywhere :-)\n";
  }
}


pair<uint, int> EndgameFunctionality::get_table_index_and_stm(const Board2 &board) {
  vector<PiecePos> pp(num_pieces);
  board.get_encoded_piece_list(pp);
  sort_piece_pos(pp);
  bool swapped = swap_piece_pos(pp, symmetric_endgame  &&  board.get_player());
  if (swapped  &&  !pawnless_endgame)
    for (int i=0; i<num_pieces; i++)
      pp[i].pos ^= 7<<3;

  return pair<uint, int>(compress_table_index(pp), board.get_player() ^ swapped);
}



triple<uint, uint, int> EndgameFunctionality::get_table_and_bdd_index_and_stm(const Board2 &board) {
  // Like index_function
  vector<PiecePos> pp(num_pieces);
  board.get_encoded_piece_list(pp);
  sort_piece_pos(pp);
  bool swapped = swap_piece_pos(pp, symmetric_endgame  &&  board.get_player());

  if (swapped  &&  !pawnless_endgame)
    for (int i=0; i<num_pieces; i++)
      pp[i].pos ^= 7<<3;

  cerr << "swapped = " << (int)swapped
      << ", symmetric_endgame = " << (int)symmetric_endgame << ", player = "
      << (int)board.get_player()
      << ", && = " << (symmetric_endgame  &&  board.get_player()) << "\n";

  for (int i=0; i<num_pieces; i++) {
    if (i) cerr << ",";
    cerr << "(" << PIECE_NAME[pp[i].piece] << ", " << POS_NAME[pp[i].pos] << ")";
  }
  cerr << "\n";

  return triple<uint, uint, int>(compress_table_index(pp),
      preprocess_bdd_index(pp).index(),
      board.get_player() ^ swapped);
}


pair<int, int> EndgameFunctionality::getModifiedOBDDIndexAndClusterValue(const Board2 &board) {
  if (!(bdd[0] || bdd[1])) return pair<int,int>(-1,-1);

  // Like index_function
  vector<PiecePos> pp(num_pieces);
  board.get_encoded_piece_list(pp);
  sort_piece_pos(pp);
  bool swapped = swap_piece_pos(pp, symmetric_endgame  &&  board.get_player());
  int player = board.get_player() ^ swapped;

  if (!bdd[player]) return pair<int,int>(-1,-1);

  if (swapped  &&  !pawnless_endgame)
    for (int i=0; i<num_pieces; i++)
      pp[i].pos ^= 7<<3;

  return bdd[player]->old_index_to_new_index_and_subset_number(preprocess_bdd_index(pp).index());
}



bool EndgameFunctionality::construct_from_table_index(Board2 &board, uint index, int pl) {
  if (pl!=0  &&  pl!=1) {
    cerr << "cfti: Player must be 0 or 1\n";
    return false;
  }
  if (table_size<=index) {
    cerr << "cfti: Table index range for endgame " << name << " is [0.." << table_size << "[\n";
    return false;
  }

  vector<PiecePos> piece_list(num_pieces);
  for (int i=0; i<num_pieces; i++)
    piece_list[i].piece = pieces[i];
  (*decompress_table_index)(index, piece_list);

  for (int i=0; i<num_pieces; i++) {
    if (i) cerr << ",";
    cerr << "(" << PIECE_NAME[piece_list[i].piece] << ", " << POS_NAME[piece_list[i].pos] << ")";
  }
  cerr << "\n";

  bool legal_position = board.set_board(piece_list, pl);

  if (legal_position) {
    if (board.unreachable_position())
      cerr << "(index " << index << " gives an unreachable position.\n";
  } else {
    cerr << "Index " << index << " corresponds to no legal position.\n";
    board.new_game();
  }

  return legal_position;
}

char EndgameFunctionality::index_function(const Board2 &board) {
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

    /*
    // Encode an en passant in the position of 2 involved pawns
    if (board.en_passant_possible()) {
      // IMPORTANT: if the board has been mirrored, so must the position of the en passant

#ifdef ALLOW_5_MEN_ENDGAME
      // The 2 involved pawns must be either (pp[1],pp[3]),(pp[1],pp[4]) or (pp[2],pp[4])
      if (symmetric_endgame) {
	// (pp[1],pp[3])
	assert(PIECE_KIND[pp[1].piece]==PAWN  &&  PIECE_KIND[pp[3].piece]==PAWN);

	// symmetric endgame => white to move => only en passant on row 6
	assert(swapped);
	assert(ROW[pp[1].pos]==4  &&  ROW[pp[3].pos]==4);

	//cerr << "Encoding en passant: " << POS_NAME[pp[1].pos] << ", " << POS_NAME[pp[3].pos] << "\n";
	encode_en_passant(pp[1].pos, pp[3].pos);
	//cerr << "After: " << POS_NAME[pp[1].pos] << ", " << POS_NAME[pp[3].pos] << "\n";

      } else {
	assert(PIECE_KIND[pp[4].piece]==PAWN);
	// We know black pawn is pp[4], but (pp[1],pp[4]) or (pp[2],pp[4]) ?

	if (PIECE_KIND[pp[1].piece]==PAWN  &&
	    is_en_passant_pawns(swapped ? board.en_passant_square()^(7<<3) :
				board.en_passant_square(), pp[1].pos, pp[4].pos)) {
	  // (pp[1],pp[4])
	  encode_en_passant(pp[1].pos, pp[4].pos);

	} else {
	  assert(PIECE_KIND[pp[2].piece]==PAWN);
	  assert(is_en_passant_pawns(swapped ? board.en_passant_square()^(7<<3) :
				     board.en_passant_square(), pp[2].pos, pp[4].pos));
	  // (pp[2],pp[4])
	  encode_en_passant(pp[2].pos, pp[4].pos);
	}
      }
#else
      // The 2 involved pawns must be (pp[1],pp[3])
      assert(PIECE_KIND[pp[1].piece]==PAWN  &&  PIECE_KIND[pp[3].piece]==PAWN);
      assert(is_en_passant_pawns(swapped ? board.en_passant_square()^(7<<3) :
				 board.en_passant_square(), pp[1].pos, pp[3].pos));
      encode_en_passant(pp[1].pos, pp[3].pos);
#endif
    }


#if ENDGAME_TABLE_WITH_CASTLING
    if (board.not_all_castling_capabilities_lost()) {

      uchar castling = board.get_castling_capabilities();
      if (swapped) castling = ((castling >> 2) || (castling << 2)) & 0xF;

      // Remap the rooks with castling capabilities to king squares.
      // pl is color of currently examined rook
      int pl = 0;
      // bk_index is the index in pp of the black king
      int bk_index = board.get_num_pieces()>>1;
      if (bk_index+1 < board.get_num_pieces()  &&
	  PIECE_KIND[pp[bk_index+1].piece] == KING) ++bk_index;

      for (int i=1; i<board.get_num_pieces(); i++) {
	if (PIECE_KIND[pp[i].piece] == ROOK) {
	  // Long or short castling?
	  if (((pl==0)  &&  (CASTLING[pp[i].piece] & castling & WHITE_LONG_CASTLING))  ||
	      ((pl==1)  &&  (CASTLING[pp[i].piece] & castling & BLACK_SHORT_CASTLING))) {
	    pp[i].pos = pp[bk_index].pos;
	  } else if (((pl==0)  &&  (CASTLING[pp[i].piece] & castling & WHITE_SHORT_CASTLING))  ||
		     ((pl==1)  &&  (CASTLING[pp[i].piece] & castling & BLACK_LONG_CASTLING))) {
	    pp[i].pos = pp[0].pos;
	  }
	} else if (PIECE_KIND[pp[i].piece] == KING) {
	  // From now on we are examining black rooks
	  pl = 1;
	}
      }
    }
#endif
     */
  }

  // If it is a symmetric endgame with btm then swapped will be true

  { // First try to index table
    char *tmp = table[board.get_player() ^ swapped];
    if (tmp) {
      int index = compress_table_index(pp);
      //board.print_board(cerr);
      //cerr << "above has index(" << index << ")\n";
      return tmp[index];
    }
  }

  { // Then try to index bdd
    BDD *tmp = bdd[board.get_player() ^ swapped];
    if (tmp) return tmp->operator[](preprocess_bdd_index(pp));
  }

  // Todo: Load?
  return ENDGAME_TABLE_UNKNOWN;
}

bool piece_overlap(vector<PiecePos> pp) {
  for (uint i=0; i<pp.size(); i++)
    for (uint j=0; j<i; j++)
      if (pp[i].pos == pp[j].pos) return true;
  return false;
}

void EndgameFunctionality::print(ostream &os) {
  if (table_or_bdd_loaded()) {
    os << name << " fully loaded as a ";
    if (table_loaded()) {
      os << "table\n";
    } else {
      os << "bdd\n";
    }
  } else {
    if (table_or_bdd_loaded(WHITE)  ||  table_or_bdd_loaded(BLACK)) {
      for (int stm=0; stm<2; stm++) if (table_or_bdd_loaded(stm)) {
        os << name << (stm ? " btm" : " wtm") << "-part loaded as a ";
        if (table_loaded(stm)) {
          os << "table\n";
        } else {
          os << "bdd\n";
        }
      }
    } else {
      os << name << " not loaded.\n";
      return;
    }
  }
  os << "\ttable_size = " << table_size << "\n";
  os << "\tsymmetric_endgame = " << symmetric_endgame
      << "\tpawnless_endgame = " << pawnless_endgame << "\n";
}

void EndgameFunctionality::print_bdd(ostream &os, bool print_bdds) {
  if (bdd_loaded(WHITE) || bdd_loaded(BLACK)) {
    os << name << ":\n";
    if (bdd[WHITE]) bdd[WHITE]->print(os, print_bdds);
    if (bdd[BLACK]) bdd[BLACK]->print(os, print_bdds);
  } else {
    os << "BDD " << name << " not loaded!\n";
  }
}

void EndgameFunctionality::print_table_verifier(ostream &os) {
  os << "Statistics for table " << name << "\n";
  int total[2];
  total[WHITE] = total[BLACK] = 0;
  int total_broken[2];
  total_broken[WHITE] = total_broken[BLACK] = 0;
  for (int p=0; p<2; p++)
    if (table[p]) {
      vector<int> count(256);

      for (uint i=0; i<table_size; i++)
        ++count[table[p][i]+128];

      for (int i=0; !is_special_value(i); i--)
        if (count[i+128]) {
          os << (p ? "btm: " : "wtm: ") << "Lost in " << signedToString(-i, 3, 10)
	         << ":" << signedToString(count[i+128], 14, 10) << "\n";
          total[p] += count[i+128];
        }
      if (count[ENDGAME_TABLE_DRAW+128]) {
        os << (p ? "btm: " : "wtm: ") << "Draws:      "
            << signedToString(count[ENDGAME_TABLE_DRAW+128], 14, 10) << "\n";
        total[p] += count[ENDGAME_TABLE_DRAW+128];

      }
      for (int i=127; i; i--)
        if (count[i+128]) {
          os << (p ? "btm: " : "wtm: ") << "Mate in " << signedToString(i, 3, 10)
	         << ":" << signedToString(count[i+128], 14, 10) << "\n";
          total[p] += count[i+128];
        }
      if (count[ENDGAME_TABLE_ILLEGAL+128]) {
        os << (p ? "btm: " : "wtm: ") << "Broken positions:"
            << signedToString(count[ENDGAME_TABLE_ILLEGAL+128], 9, 10) << "\n";
        total_broken[p] += count[ENDGAME_TABLE_ILLEGAL+128];
      }
    }

  os << "Total: " << total[WHITE]+total[BLACK]
                                        << " (" << total[WHITE] << "+" << total[BLACK] << ") + broken positions: "
                                        << total_broken[WHITE]+total_broken[BLACK] << " ("
                                        << total_broken[WHITE] << "+" << total_broken[BLACK] << ")\n"
                                        << "Size is " << (100.0*(total_broken[WHITE]+total_broken[BLACK]))/(total[WHITE]+total[BLACK])
                                        << "% bigger than optimal.\n";
}

// inspect(cerr, {64, e2, f7, f6}) prints map
void EndgameFunctionality::inspect(ostream &os, vector<Position> positions) {
  assert((int)positions.size() == num_pieces);

  vector<PiecePos> pp(num_pieces);
  int index = 0;
  for (int i=0; i<num_pieces; i++) {
    pp[i] = PiecePos(pieces[i], positions[i]);
    if (positions[i] == ILLEGAL_POS) index = i;
  }

  //os << "index = " << index << "\n";

  int wk_index = 0;
  int bk_index = 1;
  while (pieces[bk_index] != BKING) ++bk_index;

  bool pawn_inspection = PIECE_KIND[pieces[index]]==PAWN;
  //os << pawn_inspection << '\n';

  for (int player=0; player<2; player++) if (table[player] || bdd[player]) {
    int values[64];
    for (int i=0; i<64; i++) {
      pp[index].pos = i;

      //os << (int)pp[wk_index].pos << " --- " << (int)pp[bk_index].pos << '\n';
      if ((pawn_inspection  &&  (ROW[i]==0  ||  ROW[i]==7))  ||
          kings_too_near(pp[wk_index].pos, pp[bk_index].pos)) {
        values[i] = ENDGAME_TABLE_ILLEGAL;

      } else {

        if (table[player]) {
          if (piece_overlap(pp)) {
            values[i] = ENDGAME_TABLE_ILLEGAL;
          } else {
            values[i] = table[player][compress_table_index(pp)];
          }

        } else if (bdd[player]) {
          /*
	    todo:
	  int index = compress_bdd_index(player, pp);
	  int features = extract_features(index, bdd_log_size, pp, player);
	  values[i] = bdd->index(index, features);
           */
          values[i] = 0;
        } else {
          assert(0);
        }
      }
      //os << i << " -> " << values[i] << '\n';
    }

    string s[64];
    for (int i=0; i<64; i++)
      s[i] = endgame_value_to_string(values[i]);
    for (int i=0; i<num_pieces; i++)
      if (i!=index)
        s[pp[i].pos] = "-"+PIECE_SCHAR[pieces[i]]+"-";
    os << "Result if " << PPIECE_NAME[pieces[index]] << " is inserted ("
        << PLAYER_NAME[player] << " to move)\n";
    print_string_map64(os, s, 4);
  }
}

// print is a sum of
// 1 : Write each position judged unreachable to big_output.txt
// 2 : Print number of positions judged legal, statically unr., etc.
// 4 : (only used if 2 also used) Print latex stuff
void EndgameFunctionality::
find_unreachable_positions(BitList &unreachable, int player, int test_depth, int print) {
  // If unreachable is specified (not dummy Bitlist()), then it must have correct size.
  assert(unreachable.size() == 0  ||  unreachable.size() == table_size);

  assert(player==0  ||  (!symmetric_endgame  &&  player==1));

  if (print) {
    if ((print & (2+4))==2)
      cerr << "Finding unreachable positions in the " << name << " endgame.\n"
      << "Each position judged unreachable will be written to big_output.txt.\n"
      << "Number of positions = " << table_size << "\n";
    if (print & 1)
      big_output << "\n\nBEGIN: EndgameFunctionality::examine_unreachable_positions\n\n\n";
  }

  Board2 board;
  vector<PiecePos> piece_list(num_pieces);
  for (int i=0; i<num_pieces; i++)
    piece_list[i].piece = pieces[i];

  vector<int> count(test_depth+3);

  for (uint i=0; i<table_size; i++) {
    if (print && (print & (2+4))==2  &&  (i&0x3FFF)==0) {
      cerr << i << " "; cerr.flush();
    }

    (*decompress_table_index)(i, piece_list);
    bool legal_position = board.set_board(piece_list, player);

    if (legal_position) {
      bool static_unreachable = board.unreachable_position();

      int max_undo_depth = 0;
      if (test_depth > 0) {
        max_undo_depth = board.unreachable_position(test_depth);

        // Verify the static unreachable test
        if (static_unreachable  &&  (max_undo_depth==0)) {
          // static_unreachable check failed (or maybe the other, but that's less likely)
          cerr << "Error: position below is judged static unreachable,\n"
              << "but it is possible to undo a move!\n";
          board.print_board(cerr);
          assert(0);
          exit(1);
        }
      }

      if (static_unreachable  ||  max_undo_depth) {

        if (unreachable.size()) unreachable.set(i);

        if (static_unreachable) {
          ++count[2];
          if (print & 1) {
            big_output << "Static decided unreachable position:\n";
            board.print_board(big_output);
          }
        } else {
          ++count[max_undo_depth+2];
          if ((print & 1)) {
            big_output << "Undoing the " << max_undo_depth << "'th move failed:\n";
            board.print_board(big_output);
          }
        }
      } else {
        ++count[1];
      }

      if (max_undo_depth >= 3){
        cerr << "Undoing the " << max_undo_depth << "'th move failed:\n";
        board.print_board(cerr);
      }

    } else {
      ++count[0];
    }
  }

  if (print & 2) {
    if (print & 4) {
      cerr << name << " - " << (player?"b":"w") << "tm";
      for (int i=0; i<test_depth+3; i++)
        cerr << "&" << count[i];
      cerr << "\\\\\n";
    } else {
      cerr << "\nResult for " << name << " with " << (player?"black":"white") << " to move.\n"
          << "Illegal positions:             " << count[0] << "\n"
          << "(Assumed) reachable positions: " << count[1] << "\n"
          << "Static decided unreachable p.: " << count[2] << "\n";
      for (int d=1; d<=test_depth; d++) {
        cerr << "Undoing the " << d << "'th move failed:  " << count[d+2] << "\n";
      }
      cerr << "(See big_output.txt for further details)\n";
    }
  }
  if (print & 1)
    big_output << "\n\n\nEND: EndgameFunctionality::examine_unreachable_positions\n\n";
}





// print is a sum of
// 1 : Write each position judged unreachable to big_output.txt
// 2 : Print number of positions judged legal, statically unr., etc.
// 4 : (only used if 2 also used) Print latex stuff
bool EndgameFunctionality::reduce_information() {
  if (!table[0] || (!symmetric_endgame && !table[1])) {
    cerr << "Error: EndgameFunctionality::reduce_information: table must be fully loaded.\n";
    return false;
  }

  BitList bl[2];
  if (table[0]) bl[0].init(table_size);
  if (table[1]) bl[1].init(table_size);

  Board2 board;
  vector<PiecePos> piece_list(num_pieces);
  for (int i=0; i<num_pieces; i++)
    piece_list[i].piece = pieces[i];

  for (int player=0; player<(symmetric_endgame ? 1 : 2); player++) {
    for (uint i=0; i<table_size; i++) {
      if ((i&0x3FFF)==0) { cerr << i << " "; cerr.flush(); }

      (*decompress_table_index)(i, piece_list);
      bool legal_position = board.set_board(piece_list, player);

      if (legal_position) {
        int max_value = 0; // -M0, worst possible
        pair<uint, int> max;

        Move move = board.moves();
        while (board.next_move(move)) {
          if (!(move.is_en_passant()  ||  move.is_pawn_promotion()  ||  board[move.to])) {
            Undo undo = board.execute_move(move);

            pair<uint, int> tmp = get_table_index_and_stm(board);

            char v = add_ply_to_endgame_value(table[tmp.second][tmp.first]);

            if (endgame_cmp(v, max_value) >= 0) {
              max_value = v;
              max = tmp;
            }

            board.undo_move(move, undo);
          }
        }

        bl[max.second].set(max.first);
      }
    }
  }

  cerr << "table_size = " << table_size << "\n";
  cerr << "white: " << bl[0].count_on() << "\n";
  if (table[1]) cerr << "black: " << bl[1].count_on() << "\n";

  for (int player=0; player<(symmetric_endgame ? 1 : 2); player++) {
    char worst_win = 1;
    char worst_loss = 0;

    for (uint i=0; i<table_size; i++) {
      if (!is_special_value(table[player][i])) {
        if (table[player][i] > 0) {
          // A win
          if (table[player][i] > worst_win) worst_win = table[player][i];
        } else {
          // A loss
          if (table[player][i] < worst_loss) worst_loss = table[player][i];
        }
      }
    }

    for (uint i=0; i<table_size; i++) {
      if (!bl[player][i]  &&  !is_special_value(table[player][i])) {
        table[player][i] = (table[player][i]>0 ? worst_win : worst_loss);
      }
    }
  }

  return true;
}

bool EndgameFunctionality::bdd_contains_only_win_draw_loss_info(int stm) {
  assert(stm==0  ||  stm==1);
  if (!bdd[stm]) return false;
  int size = 1 << calc_log_bdd_size();
  uchar ok[256];
  memset(ok, 0, 256);
  ok[ENDGAME_TABLE_WIN] = ok[ENDGAME_TABLE_DRAW] = ok[ENDGAME_TABLE_LOSS] = 1;
  for (int i=0; i<size; i++)
    if (!ok[(int)(*(bdd[stm]))[i]]) return false;
  return true;
}
bool EndgameFunctionality::table_contains_only_win_draw_loss_info(int stm) {
  assert(stm==0  ||  stm==1);
  if (!table[stm]) return false;
  uchar ok[256];
  memset(ok, 0, 256);
  ok[ENDGAME_TABLE_WIN] = ok[ENDGAME_TABLE_DRAW] = ok[ENDGAME_TABLE_LOSS] = 1;
  for (uint i=0; i<table_size; i++)
    if (!ok[(int)table[stm][i]]) return false;
  return true;
}

void EndgameFunctionality::run_length_encode(int stm, int method, bool map_dont_cares) {
  assert(stm==0  ||  stm==1);
  if (!table[stm]) {
    cerr << "EndgameFunctionality::run_length_encode(...): Table not loaded. Aborting\n";
    return;
  }

  // table contains only win/draw/loss information?
  bool restore_table = false;
  if (!table_contains_only_win_draw_loss_info(stm)) {
    cerr << "Mapping all won in n to ENDGAME_TABLE_WIN, and lost in n to ENDGAME_TABLE_LOSS\n";
    for (uint i=0; i<table_size; i++) {
      if (table[stm][i] > 0) {
        table[stm][i] = ENDGAME_TABLE_WIN;
      } else if (table[stm][i] <= 0  &&  !is_special_value(table[stm][i])) {
        table[stm][i] = ENDGAME_TABLE_LOSS;
      }
    }
    restore_table = true;
  }

  RLE_Endgame rle;
  if (map_dont_cares) {
    rle.init(table[stm], table_size, method, ENDGAME_TABLE_ILLEGAL);
  } else {
    rle.init(table[stm], table_size, method);
  }

  if (restore_table) {
    delete table[stm];
    table[stm] = 0;
    cerr << "Restoring table with full information.\n";
    load_table(!stm, true);
  }
}


bool Endgames::get_table_and_bdd_index_and_stm(const Board2 &board, triple<uint, uint, int> &indexes) {
  int hash_value = board.get_endgame_hashing();
  if (board.get_num_pieces() <= MAX_MEN) {
    if (supported(hash_value)) {
      indexes = hash_list[hash_value]->get_table_and_bdd_index_and_stm(board);
      return true;
    } else {
      cerr << "The position is not covered by any known endgames!\n";
      return false;
    }
  } else {
    cerr << "The position has too many pieces to be considered an endgame.\n";
    return false;
  }
}

bool Endgames::construct_from_table_index(Board2 &board, string endgame_name, uint index, int player) {
  if (supported(endgame_name)) {
    return endgames[endgame_name].construct_from_table_index(board, index, player);    
  } else {
    cerr << endgame_name << " is not a supported endgame.\n";
    return false;
  }
}


#define _KXK(sname, name, piece, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[DB_W ## piece ## _VALUE] = \
        tmp[DB_B ## piece ## _VALUE] = sname
#define _KXYK(sname, name, piece1, piece2, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[DB_W ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE] = \
        tmp[DB_B ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE] = sname
#define _KXKY(sname, name, piece1, piece2, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[DB_W ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE] = \
        tmp[DB_B ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE] = sname

#ifdef ALLOW_5_MEN_ENDGAME
// K+3 vs K
#define _KXXXK(sname, name, piece, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[3*DB_W ## piece ## _VALUE] = \
        tmp[3*DB_B ## piece ## _VALUE] = sname
#define _KXXYK(sname, name, piece1, piece2, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[2*DB_W ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE] = \
        tmp[2*DB_B ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE] = sname
#define _KXYYK(sname, name, piece1, piece2, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[DB_W ## piece1 ## _VALUE + 2*DB_W ## piece2 ## _VALUE] = \
        tmp[DB_B ## piece1 ## _VALUE + 2*DB_B ## piece2 ## _VALUE] = sname
#define _KXYZK(sname, name, piece1, piece2, piece3, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[DB_W ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE + DB_W ## piece3 ## _VALUE] = \
        tmp[DB_B ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE + DB_B ## piece3 ## _VALUE] = sname

// K+2 vs K+1
#define _KXXKY(sname, name, piece1, piece2, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[2*DB_W ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE] = \
        tmp[2*DB_B ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE] = sname
#define _KXYKZ(sname, name, piece1, piece2, piece3, table_size) \
    endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
        decompress_ ## name ## _table_index, \
        preprocess_ ## name ## _bdd_index, \
        name ## _table_index_to_bdd_index, \
        table_size, sname); \
        tmp[DB_W ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE + DB_B ## piece3 ## _VALUE] = \
        tmp[DB_B ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE + DB_W ## piece3 ## _VALUE] = sname

#endif
void Endgames::init() {
  if (initialized) return;
  initialized = true;

  cerr << "Initializing endgames\n";

  map<int, string> tmp;

  { // 2-men endgame
    endgames["KK"] = EndgameFunctionality(compress_KK_table_index,
        decompress_KK_table_index,
        preprocess_KK_bdd_index,
        KK_table_index_to_bdd_index,
        462, "KK");
    tmp[0] = "KK";
  }

  { // 3-men endgames
    _KXK("KNK", KXK, KNIGHT, 462*64);
    _KXK("KBK", KXK, BISHOP, 462*64);
    _KXK("KRK", KXK, ROOK  , 462*64);
    _KXK("KQK", KXK, QUEEN , 462*64);

    _KXK("KPK", KPK, PAWN  , 1806*48);
  }

  { // 4-men endgames
    _KXYK("KNNK", KXXK, KNIGHT, KNIGHT, 462*(63*64/2));
    _KXYK("KBNK", KXYK, BISHOP, KNIGHT, 462*64*64);
    _KXYK("KBBK", KXXK, BISHOP, BISHOP, 462*(63*64/2));
    _KXYK("KRNK", KXYK, ROOK  , KNIGHT, 462*64*64);
    _KXYK("KRBK", KXYK, ROOK  , BISHOP, 462*64*64);
    _KXYK("KRRK", KXXK, ROOK  , ROOK  , 462*(63*64/2));
    _KXYK("KQNK", KXYK, QUEEN , KNIGHT, 462*64*64);
    _KXYK("KQBK", KXYK, QUEEN , BISHOP, 462*64*64);
    _KXYK("KQRK", KXYK, QUEEN , ROOK  , 462*64*64);
    _KXYK("KQQK", KXXK, QUEEN , QUEEN , 462*(63*64/2));

    _KXYK("KNPK", KXPK, KNIGHT, PAWN  , 1806*64*48);
    _KXYK("KBPK", KXPK, BISHOP, PAWN  , 1806*64*48); 
    _KXYK("KRPK", KXPK, ROOK  , PAWN  , 1806*64*48);
    _KXYK("KQPK", KXPK, QUEEN , PAWN  , 1806*64*48);

    _KXYK("KPPK", KPPK, PAWN  , PAWN  , 1806*(47*48/2));



    _KXKY("KNKN", KXKY, KNIGHT, KNIGHT, 462*64*64);
    _KXKY("KBKN", KXKY, BISHOP, KNIGHT, 462*64*64);
    _KXKY("KBKB", KXKY, BISHOP, BISHOP, 462*64*64);
    _KXKY("KRKN", KXKY, ROOK  , KNIGHT, 462*64*64);
    _KXKY("KRKB", KXKY, ROOK  , BISHOP, 462*64*64);
    _KXKY("KRKR", KXKY, ROOK  , ROOK  , 462*64*64);
    _KXKY("KQKN", KXKY, QUEEN , KNIGHT, 462*64*64);
    _KXKY("KQKB", KXKY, QUEEN , BISHOP, 462*64*64);
    _KXKY("KQKR", KXKY, QUEEN , ROOK  , 462*64*64);
    _KXKY("KQKQ", KXKY, QUEEN , QUEEN , 462*64*64);

    _KXKY("KNKP", KXKP, KNIGHT, PAWN  , 1806*64*48);
    _KXKY("KBKP", KXKP, BISHOP, PAWN  , 1806*64*48);
    _KXKY("KRKP", KXKP, ROOK  , PAWN  , 1806*64*48);
    _KXKY("KQKP", KXKP, QUEEN , PAWN  , 1806*64*48);

    _KXKY("KPKP", KPKP, PAWN  , PAWN  , 1806*48*48);
  }

#ifdef ALLOW_5_MEN_ENDGAME
  { // 5-men endgames

    // TODO: check that all the stuff is correct (no wrong numbers etc.)

    { // ENDGAMES WITH K+3 vs K


      { // KXXXK endgames
        _KXXXK("KPPPK", KPPPK, PAWN  , 1806*(46*47*48/6));
        _KXXXK("KNNNK", KXXXK, KNIGHT, 462*(62*63*64/6));
        _KXXXK("KBBBK", KXXXK, BISHOP, 462*(62*63*64/6));
        _KXXXK("KRRRK", KXXXK, ROOK  , 462*(62*63*64/6));
        _KXXXK("KQQQK", KXXXK, QUEEN , 462*(62*63*64/6));
      }

      { // KXXYK/KXYYK endgames
        _KXXYK("KNNPK", KXXPK, KNIGHT, PAWN  , 1806*(63*64/2)*48);
        _KXXYK("KBBPK", KXXPK, BISHOP, PAWN  , 1806*(63*64/2)*48);
        _KXXYK("KRRPK", KXXPK, ROOK  , PAWN  , 1806*(63*64/2)*48);
        _KXXYK("KQQPK", KXXPK, QUEEN , PAWN  , 1806*(63*64/2)*48);

        _KXXYK("KBBNK", KXXYK, BISHOP, KNIGHT, 462*(63*64/2)*64);
        _KXXYK("KRRNK", KXXYK, ROOK  , KNIGHT, 462*(63*64/2)*64);
        _KXXYK("KQQNK", KXXYK, QUEEN , KNIGHT, 462*(63*64/2)*64);

        _KXXYK("KRRBK", KXXYK, ROOK  , BISHOP, 462*(63*64/2)*64);
        _KXXYK("KQQBK", KXXYK, QUEEN , BISHOP, 462*(63*64/2)*64);

        _KXXYK("KQQRK", KXXYK, QUEEN , ROOK  , 462*(63*64/2)*64);



        _KXYYK("KNPPK", KXPPK, KNIGHT, PAWN  , 1806*(47*48/2)*64);
        _KXYYK("KBPPK", KXPPK, BISHOP, PAWN  , 1806*(47*48/2)*64);
        _KXYYK("KRPPK", KXPPK, ROOK  , PAWN  , 1806*(47*48/2)*64);
        _KXYYK("KQPPK", KXPPK, QUEEN , PAWN  , 1806*(47*48/2)*64);

        _KXYYK("KBNNK", KXYYK, BISHOP, KNIGHT, 462*(63*64/2)*64);
        _KXYYK("KRNNK", KXYYK, ROOK  , KNIGHT, 462*(63*64/2)*64);
        _KXYYK("KQNNK", KXYYK, QUEEN , KNIGHT, 462*(63*64/2)*64);

        _KXYYK("KRBBK", KXYYK, ROOK  , BISHOP, 462*(63*64/2)*64);
        _KXYYK("KQBBK", KXYYK, QUEEN , BISHOP, 462*(63*64/2)*64);

        _KXYYK("KQRRK", KXYYK, QUEEN , ROOK  , 462*(63*64/2)*64);
      }

      { // KXYZK endgames
        _KXYZK("KBNPK", KXYPK, BISHOP, KNIGHT, PAWN  , 1806*64*64*48);
        _KXYZK("KRNPK", KXYPK, ROOK  , KNIGHT, PAWN  , 1806*64*64*48);
        _KXYZK("KQNPK", KXYPK, QUEEN , KNIGHT, PAWN  , 1806*64*64*48);
        _KXYZK("KRBPK", KXYPK, ROOK  , BISHOP, PAWN  , 1806*64*64*48);
        _KXYZK("KQBPK", KXYPK, QUEEN , BISHOP, PAWN  , 1806*64*64*48);
        _KXYZK("KQRPK", KXYPK, QUEEN , ROOK  , PAWN  , 1806*64*64*48);

        _KXYZK("KRBNK", KXYPK, ROOK  , BISHOP, KNIGHT, 462*64*64*64);
        _KXYZK("KQBNK", KXYPK, QUEEN , BISHOP, KNIGHT, 462*64*64*64);
        _KXYZK("KQRNK", KXYPK, QUEEN , ROOK  , KNIGHT, 462*64*64*64);

        _KXYZK("KQRBK", KXYPK, QUEEN , ROOK  , BISHOP, 462*64*64*64);
      }
    }


    { // ENDGAMES WITH K+2 vs K+1

      { // KXXKY endgames

        // IMPORTANT!!!  THERE IS A PROBLEM WITH THE KPPKP ENDGAME:
        // Some positions are won/lost in up to 127 => out of range!
        _KXXKY("KPPKP", KPPKP, PAWN  , PAWN  , 1806*(47*48/2)*48);//!!!

        _KXXKY("KNNKP", KXXKP, KNIGHT, PAWN  , 1806*(63*64/2)*48);
        _KXXKY("KBBKP", KXXKP, BISHOP, PAWN  , 1806*(63*64/2)*48);
        _KXXKY("KRRKP", KXXKP, ROOK  , PAWN  , 1806*(63*64/2)*48);
        _KXXKY("KQQKP", KXXKP, QUEEN , PAWN  , 1806*(63*64/2)*48);

        _KXXKY("KPPKN", KPPKX, PAWN  , KNIGHT, 1806*(47*48/2)*64);
        _KXXKY("KNNKN", KXXKY, KNIGHT, KNIGHT, 462*(63*64/2)*64);
        _KXXKY("KBBKN", KXXKY, BISHOP, KNIGHT, 462*(63*64/2)*64);
        _KXXKY("KRRKN", KXXKY, ROOK  , KNIGHT, 462*(63*64/2)*64);
        _KXXKY("KQQKN", KXXKY, QUEEN , KNIGHT, 462*(63*64/2)*64);

        _KXXKY("KPPKB", KPPKX, PAWN  , BISHOP, 1806*(47*48/2)*64);
        _KXXKY("KNNKB", KXXKY, KNIGHT, BISHOP, 462*(63*64/2)*64);
        _KXXKY("KBBKB", KXXKY, BISHOP, BISHOP, 462*(63*64/2)*64);
        _KXXKY("KRRKB", KXXKY, ROOK  , BISHOP, 462*(63*64/2)*64);
        _KXXKY("KQQKB", KXXKY, QUEEN , BISHOP, 462*(63*64/2)*64);

        _KXXKY("KPPKR", KPPKX, PAWN  , ROOK  , 1806*(47*48/2)*64);
        _KXXKY("KNNKR", KXXKY, KNIGHT, ROOK  , 462*(63*64/2)*64);
        _KXXKY("KBBKR", KXXKY, BISHOP, ROOK  , 462*(63*64/2)*64);
        _KXXKY("KRRKR", KXXKY, ROOK  , ROOK  , 462*(63*64/2)*64);
        _KXXKY("KQQKR", KXXKY, QUEEN , ROOK  , 462*(63*64/2)*64);

        _KXXKY("KPPKQ", KPPKX, PAWN  , QUEEN , 1806*(47*48/2)*64);
        _KXXKY("KNNKQ", KXXKY, KNIGHT, QUEEN , 462*(63*64/2)*64);
        _KXXKY("KBBKQ", KXXKY, BISHOP, QUEEN , 462*(63*64/2)*64);
        _KXXKY("KRRKQ", KXXKY, ROOK  , QUEEN , 462*(63*64/2)*64);
        _KXXKY("KQQKQ", KXXKY, QUEEN , QUEEN , 462*(63*64/2)*64);
      }

      { // KXYKZ endgames
        _KXYKZ("KNPKP", KXPKY, KNIGHT, PAWN  , PAWN  , 1806*48*48*64);
        _KXYKZ("KBPKP", KXPKY, BISHOP, PAWN  , PAWN  , 1806*48*48*64);
        _KXYKZ("KRPKP", KXPKY, ROOK  , PAWN  , PAWN  , 1806*48*48*64);
        _KXYKZ("KQPKP", KXPKY, QUEEN , PAWN  , PAWN  , 1806*48*48*64);

        _KXYKZ("KNPKN", KXPKY, KNIGHT, PAWN  , KNIGHT, 1806*48*64*64);
        _KXYKZ("KBPKN", KXPKY, BISHOP, PAWN  , KNIGHT, 1806*48*64*64);
        _KXYKZ("KRPKN", KXPKY, ROOK  , PAWN  , KNIGHT, 1806*48*64*64);
        _KXYKZ("KQPKN", KXPKY, QUEEN , PAWN  , KNIGHT, 1806*48*64*64);

        _KXYKZ("KNPKB", KXPKY, KNIGHT, PAWN  , BISHOP, 1806*48*64*64);
        _KXYKZ("KBPKB", KXPKY, BISHOP, PAWN  , BISHOP, 1806*48*64*64);
        _KXYKZ("KRPKB", KXPKY, ROOK  , PAWN  , BISHOP, 1806*48*64*64);
        _KXYKZ("KQPKB", KXPKY, QUEEN , PAWN  , BISHOP, 1806*48*64*64);

        _KXYKZ("KNPKR", KXPKY, KNIGHT, PAWN  , ROOK  , 1806*48*64*64);
        _KXYKZ("KBPKR", KXPKY, BISHOP, PAWN  , ROOK  , 1806*48*64*64);
        _KXYKZ("KRPKR", KXPKY, ROOK  , PAWN  , ROOK  , 1806*48*64*64);
        _KXYKZ("KQPKR", KXPKY, QUEEN , PAWN  , ROOK  , 1806*48*64*64);

        _KXYKZ("KNPKQ", KXPKY, KNIGHT, PAWN  , QUEEN , 1806*48*64*64);
        _KXYKZ("KBPKQ", KXPKY, BISHOP, PAWN  , QUEEN , 1806*48*64*64);
        _KXYKZ("KRPKQ", KXPKY, ROOK  , PAWN  , QUEEN , 1806*48*64*64);
        _KXYKZ("KQPKQ", KXPKY, QUEEN , PAWN  , QUEEN , 1806*48*64*64);//124>=x>=-123 (close!)



        _KXYKZ("KBNKP", KXPKY, BISHOP, KNIGHT, PAWN  , 1806*64*48*64);
        _KXYKZ("KRNKP", KXPKY, ROOK  , KNIGHT, PAWN  , 1806*64*48*64);
        _KXYKZ("KQNKP", KXPKY, QUEEN , KNIGHT, PAWN  , 1806*64*48*64);

        _KXYKZ("KBNKN", KXYKZ, BISHOP, KNIGHT, KNIGHT, 462*64*64*64);
        _KXYKZ("KRNKN", KXYKZ, ROOK  , KNIGHT, KNIGHT, 462*64*64*64);
        _KXYKZ("KQNKN", KXYKZ, QUEEN , KNIGHT, KNIGHT, 462*64*64*64);

        _KXYKZ("KBNKB", KXYKZ, BISHOP, KNIGHT, BISHOP, 462*64*64*64);
        _KXYKZ("KRNKB", KXYKZ, ROOK  , KNIGHT, BISHOP, 462*64*64*64);
        _KXYKZ("KQNKB", KXYKZ, QUEEN , KNIGHT, BISHOP, 462*64*64*64);

        _KXYKZ("KBNKR", KXYKZ, BISHOP, KNIGHT, ROOK  , 462*64*64*64);
        _KXYKZ("KRNKR", KXYKZ, ROOK  , KNIGHT, ROOK  , 462*64*64*64);
        _KXYKZ("KQNKR", KXYKZ, QUEEN , KNIGHT, ROOK  , 462*64*64*64);

        _KXYKZ("KBNKQ", KXYKZ, BISHOP, KNIGHT, QUEEN , 462*64*64*64);
        _KXYKZ("KRNKQ", KXYKZ, ROOK  , KNIGHT, QUEEN , 462*64*64*64);
        _KXYKZ("KQNKQ", KXYKZ, QUEEN , KNIGHT, QUEEN , 462*64*64*64);



        _KXYKZ("KRBKP", KXYKP, ROOK  , BISHOP, PAWN  , 1806*64*48*64);
        _KXYKZ("KQBKP", KXYKP, QUEEN , BISHOP, PAWN  , 1806*64*48*64);

        _KXYKZ("KRBKN", KXYKZ, ROOK  , BISHOP, KNIGHT, 462*64*64*64);
        _KXYKZ("KQBKN", KXYKZ, QUEEN , BISHOP, KNIGHT, 462*64*64*64);

        _KXYKZ("KRBKB", KXYKZ, ROOK  , BISHOP, BISHOP, 462*64*64*64);
        _KXYKZ("KQBKB", KXYKZ, QUEEN , BISHOP, BISHOP, 462*64*64*64);

        _KXYKZ("KRBKR", KXYKZ, ROOK  , BISHOP, ROOK  , 462*64*64*64);
        _KXYKZ("KQBKR", KXYKZ, QUEEN , BISHOP, ROOK  , 462*64*64*64);

        _KXYKZ("KRBKQ", KXYKZ, ROOK  , BISHOP, QUEEN , 462*64*64*64);
        _KXYKZ("KQBKQ", KXYKZ, QUEEN , BISHOP, QUEEN , 462*64*64*64);



        _KXYKZ("KQRKP", KXYKP, QUEEN , ROOK  , PAWN  , 1806*64*48*64);

        _KXYKZ("KQRKN", KXYKZ, QUEEN , ROOK  , KNIGHT, 462*64*64*64);

        _KXYKZ("KQRKB", KXYKZ, QUEEN , ROOK  , BISHOP, 462*64*64*64);

        _KXYKZ("KQRKR", KXYKZ, QUEEN , ROOK  , ROOK  , 462*64*64*64);

        _KXYKZ("KQRKQ", KXYKZ, QUEEN , ROOK  , QUEEN , 462*64*64*64);
      }
    }

  }
#endif

  // Assume (!!!!!!!!!!!!!!!) that the addresses of the entries
  // in the map no longer change (from here, nothing is added, deleted
  // or modified in the map)

  { // Initialize hash_list
    for (int i=0; i<DB_ARRAY_LENGTH; i++)
      hash_list[i] = 0;
    typedef map<int, string>::const_iterator CI;
    for (CI i = tmp.begin(); i != tmp.end(); i++) {
      assert(!hash_list[i->first]);
      hash_list[i->first] = &(endgames[i->second]);
    }
  }

  init_piece_enumerations();

#ifndef NDEBUG
  verify_piece_enumerations();
  verify_en_passant_encoding();
#ifdef ALLOW_5_MEN_ENDGAME
  verify_xxx_compress();
  verify_ppp_compress();
#endif
#endif

  init_cluster_functions();

}

void Endgames::destroy_tables() {
  typedef map<string, EndgameFunctionality>::iterator I;
  for (I i=endgames.begin(); i!=endgames.end(); i++)
    i->second.destroy_table();
}
void Endgames::destroy_bdd() {
  typedef map<string, EndgameFunctionality>::iterator I;
  for (I i=endgames.begin(); i!=endgames.end(); i++)
    i->second.destroy_bdd();
}
/*
void Endgames::keep_only_wtm() {
  typedef map<string, EndgameFunctionality>::iterator I;
  for (I i=endgames.begin(); i!=endgames.end(); i++)
    i->second.keep_only_wtm();
}
 */


vector<int> Endgames::get_name_matches(string name_pattern) {
  vector<int> result;
  typedef map<string, EndgameFunctionality>::iterator I;
  for (I i=endgames.begin(); i!=endgames.end(); i++)
    if (i->second.name_match(name_pattern))
      result.push_back(i->second.calc_hash_value());
  if (result.size() == 0)
    cerr << "No endgame matches " << name_pattern << "\n";
  return result;
}

void Endgames::build_tables(string name_pattern) {
  cerr << "Deleting all endgames (maximizing free memory)\n";
  destroy();
  vector<int> m = get_name_matches(name_pattern);
  for (uint i=0; i<m.size(); i++) {
    hash_list[m[i]]->load_table(false, true);
    hash_list[m[i]]->print(cerr);
    hash_list[m[i]]->destroy();
  }
}
void Endgames::build_bdds(string name_pattern) {
  cerr << "Deleting all endgames (maximizing free memory)\n";
  destroy();
  vector<int> m = get_name_matches(name_pattern);
  for (uint i=0; i<m.size(); i++) {
    hash_list[m[i]]->load_bdd(false, true);
    hash_list[m[i]]->print(cerr);
    hash_list[m[i]]->destroy();
  }
}

void Endgames::load_tables(string name_pattern,
    bool restrict_to_stm,
    bool build_if_nescessary,
    int restricted_stm)
{ 
  vector<int> m = get_name_matches(name_pattern);
  for (uint i=0; i<m.size(); i++) {
    hash_list[m[i]]->load_table(restrict_to_stm, build_if_nescessary,
        restricted_stm);
    hash_list[m[i]]->print(cerr);
  }
}
void Endgames::load_bdds(string name_pattern,
    bool restrict_to_stm,
    bool build_from_tables_if_nescessary,
    bool build_tables_if_nescessary,
    int restricted_stm)
{
  vector<int> m = get_name_matches(name_pattern);
  for (uint i=0; i<m.size(); i++) {
    hash_list[m[i]]->load_bdd(restrict_to_stm, build_from_tables_if_nescessary,
        build_tables_if_nescessary, restricted_stm);
  }
}

void Endgames::print(ostream &os, string name_pattern) {
  os << "Endgames supported:\n";
  vector<int> m = get_name_matches(name_pattern);
  for (uint i=0; i<m.size(); i++)
    hash_list[m[i]]->print(os);
}

void Endgames::print_bdds(ostream &os, string name_pattern, bool print_bdds) {
  vector<int> m = get_name_matches(name_pattern);
  for (uint i=0; i<m.size(); i++)
    hash_list[m[i]]->print_bdd(os, print_bdds);
}

// inspect(cerr, "K##_Qe2_Kf7_Rf6") prints map
void Endgames::inspect(ostream &os, string s) {
  bool ok = true;
  int num_pieces = (s.size()+1)/4;
  ok &= 4*num_pieces-1 == (int)s.size();
  bool unknown_found = false;
  if (ok) {
    for (int i=0; i<num_pieces; i++) {
      if (s[4*i+1] == '#') {
        if (unknown_found  ||  s[4*i+2] != '#') {
          ok = false;
          break;
        }
        unknown_found = true;
      } else {
        if (s[4*i+1]<'a'  ||  'h'<s[4*i+1]  ||  s[4*i+2]<'1'  ||  '8'<s[4*i+2]) {
          ok = false;
          break;
        }
      }
    }
    if (!unknown_found) {
      os << "Endgames::inspect(..., " << s << "):\n"
          << "No unknown position? All combinations tried!\n";
    }
  }
  if (ok) {
    string name(num_pieces, ' ');
    for (int i=0; i<num_pieces; i++)
      name[i] = s[4*i] & ~32;// Convert to lower case

    if (supported(name)) {
      vector<Position> positions(num_pieces);
      for (int i=0; i<num_pieces; i++)
        if (s[4*i+1] == '#') {
          positions[i] = ILLEGAL_POS;
        } else {
          positions[i] = CR_TO_POS[s[4*i+1]-'a'][s[4*i+2]-'1'];
        }

      if (unknown_found) {
        endgames[name].inspect(os, positions);
      } else {
        for (int i=0; i<num_pieces; i++) {
          int tmp = positions[i];
          positions[i] = ILLEGAL_POS;
          endgames[name].inspect(os, positions);
          positions[i] = tmp;
        }
      }

    } else {
      os << "Endgames::inspect(..., " << s << "):\n"
          << "The piece combination " << name << " was not found.\n";
    }

  } else {
    os << "Endgames::inspect(..., " << s << "): wrong format!\n"
        << "Examples of use: inspect(cerr, K## Ra2 Kg1), inspect(cerr, k## ra2 kg1)\n";
  }
}

bool clr_endgame_database(Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Board2 &b = *dynamic_cast<Board2plus *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Endgame database, help:\n"
        << "    print matches pattern  or  pm pattern\n"
        << "      - A limited version of pattern matching.\n"
        << "      - Examples \"pm K*K*\", \"pm K2K*\", \"pm KXKB\", not \"K1NK\"\n"
        << "    load table pattern [r] [b] or  lt pattern [r] [b]\n"
        << "      - b=t/f : build_if_nescessary, default is false.\n"
        << "      - r=t/f : restrict_to_wtm, default is false.\n"
        << "    build table pattern  or  bt pattern\n"
        << "    load bdd pattern [b1] [b2] [r] or  lb pattern [b1] [b2] [r]\n"
        << "      - b1=t/f : build_from_tables_if_nescessary, default is false.\n"
        << "      - b2=t/f : build_tables_if_nescessary, default is false.\n"
        << "      - r=t/f : restrict_to_wtm, default is false.\n"
        << "    build bdd pattern  or  bb pattern\n"
        << "    print bdd pattern  or  pb pattern\n"
        << "      - (load) and print endgame bdd's or just x.\n"
        << "    delete database  or  dd\n"
        << "      - remove endgame tables from memory\n"
        << "    inspect [p1 p2] [p3] [p4] [p5]\n"
        << "      - Example: \"inspect K## Ra2 Kg1\"\n"
        << "    verify table pattern  or  vt pattern\n"
        << "      - Print number of positions with draw, mate in n, etc.\n"
        << "    verify bdd pattern  or  vb pattern\n"
        << "      - Verifies the bdd against the table (both must be loaded).\n"
        << "    print settings  or  ps\n"
        << "    set name value\n"
        << "      - example \"set do_preprocessing false\"\n"
        << "    print pattern or  p pattern\n"
        << "    index database  or  id\n"
        << "      - Find value of current position in endgame table.\n"
        << "    show table index  or  sti\n"
        << "      - Shows endgame table/bdd index of position and stm.\n"
        << "    construct from table index name index stm  or  cfti name index stm\n"
        << "      - Sets board according to name, index and stm.\n"
        << "      - name is KK,KRK,...,KRKP,... index is 0..max, stm is w,b,0 or 1\n"
        << "      - Example: \"cfti KRK 2132 w\"\n"
        << "    examine unreachable positions name stm depth  or  eup name stm depth\n"
        << "      - For each position in endgame \"name\" with stm (0 or 1), try to\n"
        << "      - undo depth moves. If depth=0 use static unreachability test.\n"
        << "      - if positions spelled w. cap. P,each unr.p. wr. to big_output.txt\n"
        << "    rle pattern stm map_dont_cares\n"
        << "      - Examples \"rle KQK 0 t\", \"rle KBBK 1 t\"\n"
        << "    hrle pattern stm map_dont_cares\n"
        << "      - huffman run length encode, archieves better performance.\n";


  } else if (dot_demand(p, 3, "hej", "hej", (ptr_int)0)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    for (uint i=0; i<m.size(); i++)
      endgames[m[i]].reduce_information();

#ifdef ALLOW_5_MEN_ENDGAME
  } else if (dot_demand(p, 5, "a", "b", "c", (ptr_int)1, (ptr_int)0)) {
    cerr << "Doing secret stuff with KRRRK...\n";
    EndgameFunctionality &ef = endgames["KRRRK"];
    ef.load_bdd();
    int p = parse_result[0][0] == 'b'  ||  parse_result[0][0] == '1';
    int n = atoi(parse_result[1].c_str());
    for (int i=0; i<n; i++) {
      BDD_Index b;
      int r = rand();
      b[0] = r & 0x3F;
      b[1] = (r>>=6) & 0x3F;
      b[2] = (r>>=6) & 0x3F;
      b[3] = (r>>=6) & 0x3F;
      b[4] = (r>>=6) & 0xF;
      ef.direct_bdd_index(p, b);
    }
    cerr << "done\n";

  } else if (dot_demand(p, 5, "a", "a", "a", (ptr_int)1, (ptr_int)0)) {
    cerr << "Doing secret stuff with KNNNK...\n";
    EndgameFunctionality &ef = endgames["KNNNK"];
    ef.load_bdd();
    int p = parse_result[0][0] == 'b'  ||  parse_result[0][0] == '1';
    int n = atoi(parse_result[1].c_str());
    for (int i=0; i<n; i++) {
      BDD_Index b;
      int r = rand();
      b[0] = r & 0x3F;
      b[1] = (r>>=6) & 0x3F;
      b[2] = (r>>=6) & 0x3F;
      b[3] = (r>>=6) & 0x3F;
      b[4] = (r>>=6) & 0xF;
      ef.direct_bdd_index(p, b);
    }
    cerr << "done\n";

  } else if (dot_demand(p, 5, "c", "b", "a", 1, (ptr_int)0)) {
    cerr << "Doing secret stuff with KQKR...\n";
    EndgameFunctionality &ef = endgames["KQKR"];
    ef.load_bdd();
    int p = parse_result[0][0] == 'b'  ||  parse_result[0][0] == '1';
    int n = atoi(parse_result[1].c_str());
    for (int i=0; i<n; i++) {
      BDD_Index b;
      int r = rand();
      b[0] = r & 0x3F;
      b[1] = (r>>=6) & 0x3F;
      b[2] = (r>>=6) & 0x3F;
      b[3] = (r>>=6) & 0xF;
      ef.direct_bdd_index(p, b);
    }
    cerr << "done\n";
#endif

  } else if (dot_demand(p, 3, "print", "matches", (ptr_int)0)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    if (m.size()) {
      cerr << "Pattern " << parse_result[0] << " matches the following endgames:\n";
      for (uint i=0; i<m.size(); i++) {
        if (i) cerr << ", ";
        cerr << endgames[m[i]].get_name();
      }
      cerr << "\n";
    }

  } else if (dot_demand(p, 4, "latex", "print", "square", "permutations")) {
    for (int i=0; i<10; i++)
      print_latex_signed_map64(cerr, mappings[i], 2);

  } else if (dot_demand(p, 4, "rle", (ptr_int)0, (ptr_int)1, (ptr_int)1)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    if (m.size()) {
      cerr << "Only win/draw/loss information in the run length encoded version.\n";
      int stm = (parse_result[1][0]=='b')  ||  (parse_result[1][0]=='1');
      bool map_dont_cares = parse_result[2][0]=='t';
      for (uint i=0; i<m.size(); i++)
        endgames[m[i]].run_length_encode(stm, 1, map_dont_cares);
    }

  } else if (dot_demand(p, 4, "hrle", (ptr_int)0, (ptr_int)1, (ptr_int)1)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    if (m.size()) {
      cerr << "Only win/draw/loss information in the huffman run length encoded version.\n";
      int stm = (parse_result[1][0]=='b')  ||  (parse_result[1][0]=='1');
      bool map_dont_cares = parse_result[2][0]=='t';
      for (uint i=0; i<m.size(); i++)
        endgames[m[i]].run_length_encode(stm, 2, map_dont_cares);
    }

  } else if (dot_demand(p, 6, "examine", "unreachable", "positions", (ptr_int)0, (ptr_int)1, (ptr_int)0)  ||
      dot_demand(p, 6, "examine", "unreachable", "Positions", (ptr_int)0, (ptr_int)1, (ptr_int)0)) {
    int write_pos = dot_demand(p, 6, "examine", "unreachable", "Positions", (ptr_int)0, (ptr_int)1, (ptr_int)0) ? 1 : 0;
    if (endgames.supported(parse_result[0])) {
      int stm = atoi(parse_result[1].c_str());
      int test_depth = atoi(parse_result[2].c_str());
      if ((stm==0 || stm==1)  &&  (0<=test_depth && test_depth<10)) {
        BitList bl;
        endgames[parse_result[0]].find_unreachable_positions(bl, stm, test_depth, write_pos+2+4);
      } else {
        cerr << "Stm or depth not in legal interval ([0..1] and [0..9])\n";
      }
    } else {
      os << "Unknown endgame table " << parse_result[0] << "\n";
    }

  } else if (dot_demand(p, 3, "build", "table", (ptr_int)0)) {
    os << "Building endgame tables.\n";
    endgames.build_tables(parse_result[0]);

    // BEGIN load table
  } else if (dot_demand(p, 3, "load", "table", (ptr_int)0)) {
    os << "Loading endgame tables.\n";
    endgames.load_tables(parse_result[0], false, false);
  } else if (dot_demand(p, 4, "load", "table", (ptr_int)0, (ptr_int)1)) {
    os << "Loading endgame tables.\n";
    bool buid_if_nescessary = parse_result[1][0] == 't';
    endgames.load_tables(parse_result[0], false, buid_if_nescessary);
  } else if (dot_demand(p, 5, "load", "table", (ptr_int)0, (ptr_int)1, (ptr_int)1)) {
    os << "Loading endgame tables.\n";
    bool buid_if_nescessary = parse_result[1][0] == 't';
    bool restrict_to_wtm = parse_result[2][0] == 't';
    endgames.load_tables(parse_result[0], restrict_to_wtm, buid_if_nescessary);
    // END load tables

  } else if (dot_demand(p, 3, "build", "bdd", (ptr_int)0)) {
    os << "Building endgame bdds.\n";
    endgames.build_bdds(parse_result[0]);

    // BEGIN load bdd
  } else if (dot_demand(p, 3, "load", "bdd", (ptr_int)0)) {
    os << "Loading endgame bdds.\n";
    endgames.load_bdds(parse_result[0], false, false, false);
  } else if (dot_demand(p, 4, "load", "bdd", (ptr_int)0, (ptr_int)1)) {
    os << "Loading endgame bdds.\n";
    bool buid_from_tables_if_nescessary = parse_result[1][0] == 't';
    endgames.load_bdds(parse_result[0], false, buid_from_tables_if_nescessary, false);
  } else if (dot_demand(p, 5, "load", "bdd", (ptr_int)0, (ptr_int)1, (ptr_int)1)) {
    os << "Loading endgame bdds.\n";
    bool buid_from_tables_if_nescessary = parse_result[1][0] == 't';
    bool buid_tables_if_nescessary = parse_result[2][0] == 't';
    endgames.load_bdds(parse_result[0], false,
        buid_from_tables_if_nescessary, buid_tables_if_nescessary);
  } else if (dot_demand(p, 6, "load", "bdd", (ptr_int)0, (ptr_int)1, (ptr_int)1, (ptr_int)1)) {
    os << "Loading endgame bdds.\n";
    bool buid_from_tables_if_nescessary = parse_result[1][0] == 't';
    bool buid_tables_if_nescessary = parse_result[2][0] == 't';
    bool restrict_to_wtm = parse_result[3][0] == 't';
    endgames.load_bdds(parse_result[0], restrict_to_wtm,
        buid_from_tables_if_nescessary, buid_tables_if_nescessary);
    // END load bdd

    // BEGIN verify stuff
  } else if (dot_demand(p, 3, "verify", "table", (ptr_int)0)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    for (uint i=0; i<m.size(); i++) {
      endgames[m[i]].load_table();
      endgames[m[i]].print_table_verifier(os);
    }

  } else if (dot_demand(p, 3, "verify", "bdd", (ptr_int)0)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    for (uint i=0; i<m.size(); i++) {
      endgames[m[i]].load_table();
      endgames[m[i]].load_bdd();
      endgames[m[i]].compare_bdd_with_table(WHITE);
      if (!endgames[m[i]].is_symmetric())
        endgames[m[i]].compare_bdd_with_table(BLACK);
    }
    // END verify stuff


  } else if (dot_demand(p, 3, "print", "bdd", (ptr_int)0)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    for (uint i=0; i<m.size(); i++)
      if (endgames[m[i]].load_bdd())
        endgames[m[i]].print_bdd(os, true);

  } else if (dot_demand(p, 2, "delete", "database")) {
    os << "clearing endgame database from mem...\n";
    os.flush();
    endgames.destroy_tables();
    endgames.destroy_bdd();
    os << "done\n";

  } else if (dot_demand(p, 2, "inspect", (ptr_int)0)) {
    endgames.inspect(os, parse_result[0]);

  } else if (dot_demand(p, 3, "inspect", (ptr_int)3, (ptr_int)3)) {
    string s = parse_result[0]+"_"+parse_result[1];
    endgames.inspect(os, s);

  } else if (dot_demand(p, 4, "inspect", (ptr_int)3, (ptr_int)3, (ptr_int)3)) {
    string s = parse_result[0]+"_"+parse_result[1]+"_"+parse_result[2];
    endgames.inspect(os, s);

  } else if (dot_demand(p, 5, "inspect", (ptr_int)3, (ptr_int)3, (ptr_int)3, (ptr_int)3)) {
    string s = parse_result[0]+"_"+parse_result[1]+"_"+parse_result[2]+"_"+parse_result[3];
    endgames.inspect(os, s);

  } else if (dot_demand(p, 6, "inspect", (ptr_int)3, (ptr_int)3, (ptr_int)3, (ptr_int)3, (ptr_int)3)) {
    string s = parse_result[0]+"_"+parse_result[1]+"_"+parse_result[2]+"_"+
        parse_result[3]+"_"+parse_result[4];
    endgames.inspect(os, s);

  } else if (dot_demand(p, 3, "set", (ptr_int)0, (ptr_int)0)) {
    // Todo: what if illegal name?
    endgame_settings->define(parse_result[0], parse_result[1]);
    os << "setting " << parse_result[0] << " set to " << parse_result[1] << "\n";

  } else if (dot_demand(p, 2, "print", "settings")) {
    endgame_settings->print(os);

  } else if (dot_demand(p, 1, "print")) {
    endgames.print(os);

  } else if (dot_demand(p, 2, "print", (ptr_int)0)) {
    vector<int> m = endgames.get_name_matches(parse_result[0]);
    for (uint i=0; i<m.size(); i++)
      endgames[m[i]].print(os);

  } else if (dot_demand(p, 3, "print", "latex", "kingfs")) {
    latex_print_king_fs_indexes(os);

  } else if (dot_demand(p, 2, "index", "database")) {
    int value;
    if (endgame_table_index(b, value)) {
      os << "Found position in endgame table. value = "
          << game_theoretical_value_to_string(value) << '\n';
    } else {
      os << "Did not find position in endgame table.\n";
    }
    int status = b.calc_game_status();
    if (status == GAME_OPEN) {
      Move move = b.moves();
      while (b.next_move(move)) {
        os << b.moveToSAN(move) << ": \t";
        Undo undo = b.execute_move(move);
        if (endgame_table_index(b, value)) {
          if (value) {
            // Adjust value to this player
            value = -value;
            if (value >= GUARANTEED_WIN) value--;
            if (value <= GUARANTEED_LOSS) value++;
          }
          os << game_theoretical_value_to_string(value);
        }
        else os << "?";
        os << "\n";
        b.undo_move(move, undo);
      }
    }

  } else if (dot_demand(p, 3, "show", "table", "index")) {
    triple<uint, uint, int> i;


    int hash_value = b.get_endgame_hashing();
    if (b.get_num_pieces() <= MAX_MEN  &&  endgames.supported(hash_value)) {
      triple<uint, uint, int> i = endgames[hash_value].get_table_and_bdd_index_and_stm(b);

      os << "Endgame " << endgames.get_endgame_name(b) << ":\n"
          << "(table index, bdd index, stm) = ("
          << i.first << ", " << i.second << " ("
          << toString(i.second, endgames[b.get_endgame_hashing()].calc_log_bdd_size(), 2)
          << "b), " << i.third << ")\n";

      pair<int, int> p = endgames[hash_value].getModifiedOBDDIndexAndClusterValue(b);
      if (p.first != -1)
        cerr << "\t(Mod. bdd index, cluster) = (" << p.first << "," << p.second << ")\n";
    }

  } else if (dot_demand(p, 7, "construct", "from", "table", "index", (ptr_int)0, (ptr_int)0, (ptr_int)1)) {
    string endgame_name = parse_result[0];
    uint table_index = atoi(parse_result[1].c_str());
    char stm = parse_result[2][0] | 32;
    int player;
    if (stm == 'w' || stm == '0') player = 0;
    else if (stm == 'b' || stm == '1') player = 1;
    else {
      os << "Side-to-move must be w or 0, or b or 1\n";
      return false;
    }

    if (endgames.construct_from_table_index(b, endgame_name, table_index, player))
      b.print_board(os);

  } else return false;
  return true;

}


string Endgames::get_endgame_name(const Board2 &board) {
  int hash_value = board.get_endgame_hashing();
  if (board.get_num_pieces() <= MAX_MEN  &&  supported(hash_value)) {
    return hash_list[hash_value]->get_name();
  } else {
    return "";
  }
}



EndgameSettings *endgame_settings;
Endgames endgames;
