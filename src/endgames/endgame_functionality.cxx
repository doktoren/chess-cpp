#include "endgame_functionality.hxx"

#include <fcntl.h>

#include "../board_printers.hxx"
#include "../engine/engine_constants.hxx"
#include "../compression/endgame_table_bdd.hxx"
#include "../compression/endgame_square_permutations.hxx"
#include "../compression/endgame_piece_enumerations.hxx"
#include "../compression/endgame_run_length_encoding.hxx"
#include "endgame_database.hxx"
#include "endgame_indexing.hxx"
#include "endgame_retrograde_construction.hxx"
#include "endgame_simple_construction.hxx"
#include "endgame_piece_pos.hxx"

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
          table[WHITE] = new int8_t[table_size];
          read(fd, table[WHITE], sizeof(int8_t)*table_size);
          close(fd);
        }
      }
      if (btm_needed) {
        string filename = endgames.get_directory() + name + "_btm.dat";
        cerr << "loading endgame " << filename << "\n";
        int fd = open(filename.c_str(), O_RDONLY, 0);
        if (fd != -1) {
          //load_define_value(fd, BOUND_KING, "BOUND_KING");
          table[BLACK] = new int8_t[table_size];
          read(fd, table[BLACK], sizeof(int8_t)*table_size);
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
    build_endgame_simple(this, table);
  } else {
    build_endgame_retrograde(this, table);
  }


  { // Save tables
    { // white-to-move
      string filename = endgames.get_directory() + name + "_wtm.dat";
      cerr << "saving endgame " << filename << "\n";
      //int fd = open(filename.c_str(), O_RDONLY, 0);
      int fd = creat(filename.c_str(), 0666);
      if (fd != -1) {
        //save_define_value(fd, BOUND_KING);
        write(fd, table[0], sizeof(int8_t)*table_size);
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
        write(fd, table[1], sizeof(int8_t)*table_size);
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
  vector<int8_t> convert_table;

  uint8_t *bdd_table;

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
        break;
      }
    }
  }

  cerr << "permute square enumeration\n";
  uint8_t inv_bit_perm[5][64];
  memset(inv_bit_perm, 0, 4*64);
  { // permute square enumeration
    vector<vector<int> > permutations(num_pieces);
    for (int p=0; p<num_pieces; p++) {
      permutations[p] = vector<int>(64);
      uint8_t *m = 0;
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
        break;
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
      write(fd, bdd_table, sizeof(uint8_t) << calc_log_bdd_size());
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
      write(fd, bdd_table, sizeof(uint8_t) << calc_log_bdd_size());
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

/**
 * Returns
 * 0 for an illegal position
 * 1 for an unreachable position
 * 2 for a legal position
 */
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

  int8_t _map_values[256];
  int8_t *map_values = &(_map_values[128]);
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

      int8_t table_value = map_values[table[player][i]];
      int8_t bdd_value = (*(bdd[player]))[bdd_index];

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

int8_t EndgameFunctionality::index_function(const Board2 &board) {
  vector<PiecePos> pp(num_pieces);
  board.get_encoded_piece_list(pp);
  sort_piece_pos(pp);
  // From now on, the colors of the pieces will no longer be needed
  bool swapped = swap_piece_pos(pp, symmetric_endgame  &&  board.get_player());

  if (!pawnless_endgame  &&  swapped) {
    // fix pawn problem (their move direction dependent on their player) by mirroring board
    for (int i=0; i<num_pieces; i++)
      pp[i].pos ^= 7<<3; // TODO: Move board representation dependent code
  }

  // If it is a symmetric endgame with btm then swapped will be true

  { // First try to index table
    int8_t *tmp = table[board.get_player() ^ swapped];
    if (tmp) {
      int index = compress_table_index(pp);
      return tmp[index];
    }
  }

  { // Then try to index bdd
    BDD *tmp = bdd[board.get_player() ^ swapped];
    if (tmp) return tmp->operator[](preprocess_bdd_index(pp));
  }

  return ENDGAME_TABLE_UNKNOWN;
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

            int8_t v = add_ply_to_endgame_value(table[tmp.second][tmp.first]);

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
    int8_t worst_win = 1;
    int8_t worst_loss = 0;

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
  uint8_t ok[256];
  memset(ok, 0, 256);
  ok[ENDGAME_TABLE_WIN] = ok[ENDGAME_TABLE_DRAW] = ok[ENDGAME_TABLE_LOSS] = 1;
  for (int i=0; i<size; i++)
    if (!ok[(int)(*(bdd[stm]))[i]]) return false;
  return true;
}

bool EndgameFunctionality::table_contains_only_win_draw_loss_info(int stm) {
  assert(stm==0  ||  stm==1);
  if (!table[stm]) return false;
  uint8_t ok[256];
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
