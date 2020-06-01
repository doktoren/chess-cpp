#include "endgame_table_bdd.hxx"

#include "../board.hxx"
#include "../board_printers.hxx"
#include "clustering_algorithm.hxx"
#include "endgame_piece_enumerations.hxx"
#include "mapping_of_wildcards.hxx"

#include <assert.h>
#include <fcntl.h>
#include <stack>
#include <cmath>

// Possible optimizations:
// When doing the split into several bdd, when the splits are determined
// all the bdd's are created again.


// Only placed here for debugging purposes!
string _endgame_value_to_string(int v) {
  if (v <= -124) {
    switch (v) {
    case -124:
      return "GW";
    case -125:
      return "draw";
    case -126:
      return "GL";
    case -127:
      return "????";
    case -128:
      return "*";
    }
  }
  if (v>0) {
    return "M"+signedToString(v);
  } else {
    return "-M"+signedToString(-v);
  }
}

inline int miin(int a, int b) { return a>b?b:a; }

#include "../endgames/endgame_database.hxx"

#define PRINT false

BDD::BDD() : clustering_kind(0) {
  clear();
}
BDD::~BDD() { clear(); }

void BDD::clear() {
  convert_table.clear();
  sub_bdds.clear();

  if (clustering_kind == 1) delete clustering.base_subsets;
  clustering_kind = 0;

  // Initialize pattern and bit_perm_and_permute_pos to the identity
  pattern[0] = 0x3F;
  pattern[1] = 0x3F << 6;
  pattern[2] = 0x3F << 12;
  pattern[3] = 0x3F << 18;
  pattern[4] = 0x3F << 24;
  for (int i=0; i<64; i++)
    bit_perm_and_permute_pos[i] = i | (i<<6) | (i<<12) | (i<<18) | (i<<24);
}

#pragma GCC diagnostic ignored "-Wunused-result"
void BDD::load(int fd) {
  clear();

  { // Load convert_table
    uint32_t size;
    read(fd, &size, sizeof(uint32_t));
    convert_table = vector<int8_t>(size);
    read(fd, &(convert_table[0]), sizeof(int8_t)*convert_table.size());
  }

  { // Load clustering information
    read(fd, &clustering_kind, sizeof(int32_t));

    switch(clustering_kind) {
    case 0:
      break;
    case 1:
      clustering.base_subsets = (uint16_t*)malloc(sizeof(uint16_t)*5*64);
      read(fd, clustering.base_subsets, sizeof(uint16_t)*5*64);
      break;
    case 2:
      read(fd, &(clustering.cf_index), sizeof(int32_t));
      break;
    default:
      assert(0);
    }

    if (clustering_kind) map_subset_to_cluster.load(fd);
    else map_subset_to_cluster.init(1,0);
  }

  read(fd, pattern, sizeof(uint32_t)*5);

  read(fd, bit_perm_and_permute_pos, sizeof(uint32_t)*64);

  { // Load sub_bdds
    uint32_t size;
    read(fd, &size, sizeof(size));
    sub_bdds = vector<BinaryDecisionDiagram *>(size);
    for (uint i=0; i<sub_bdds.size(); i++) {
      sub_bdds[i] = new BinaryDecisionDiagram();
      sub_bdds[i]->load(fd);
    }
  }
}
void BDD::save(int fd) {
  { // Save convert_table
    uint32_t size = convert_table.size();
    write(fd, &size, sizeof(uint32_t));
    write(fd, &(convert_table[0]), sizeof(int8_t)*convert_table.size());
  }

  { // Save clustering information
    write(fd, &clustering_kind, sizeof(int32_t));

    switch(clustering_kind) {
    case 0:
      break;
    case 1:
      write(fd, clustering.base_subsets, sizeof(uint16_t)*5*64);
      break;
    case 2:
      write(fd, &(clustering.cf_index), sizeof(int32_t));
      break;
    default:
      assert(0);
    }

    if (clustering_kind) map_subset_to_cluster.save(fd);
  }

  write(fd, pattern, sizeof(uint32_t)*5);

  write(fd, bit_perm_and_permute_pos, sizeof(uint32_t)*64);

  { // Save sub_bdds
    uint32_t size = sub_bdds.size();
    write(fd, &size, sizeof(uint32_t));
    for (uint i=0; i<sub_bdds.size(); i++)
      sub_bdds[i]->save(fd);
  }
}
#pragma GCC diagnostic pop

bool BDD::load(string filename) {
  int fd = open(filename.c_str(), O_RDONLY, 0);
  if (fd == -1) return false;
  load(fd);
  close(fd);
  return true;
}
void BDD::save(string filename) {
  int fd = creat(filename.c_str(), 0666);
  if (fd == -1) {
    cerr << "Error: could not save file " << filename << "\n";
    return;
  }
  save(fd);
  close(fd);
}

int BDD::memory_consumption() {
  int result = sizeof(BDD);

  result += sizeof(char)*convert_table.size();

  result += sizeof(BinaryDecisionDiagram*)*sub_bdds.size();

  for (uint i=0; i<sub_bdds.size(); i++)
    result += sub_bdds[i]->memory_consumption();

  return result;
}

// permute_square_enumeration changes bdd_table accordingly
// permutations.back()[i] is replaced by permutations.back()[INV_REMAP_BOUND_KING[i]]
// The permutations are stored in bit_perm_and_permute_pos
void BDD::permute_square_enumeration(uint8_t *bdd_table, int log_bdd_size,
    vector<vector<int> > permutations) {
#ifndef NDEBUG
  {
    int tmp = log_bdd_size%6;
    assert(tmp==4 || tmp==5);
    for (uint i=0; i<permutations.size(); i++)
      assert(permutations[i].size() == 64);
  }
#endif

  int bdd_size = 1 << log_bdd_size;
  int num_pieces = permutations.size();

  {
    // Update permutations.back() to take into account that the bound king
    // has been remapped by REMAP_BOUND_KING

    // Make a copy of permutations.back()
    int tmp[64];
    for (int i=0; i<64; i++) tmp[i] = permutations.back()[i];

    int bound_king_size = 1 << (log_bdd_size%6);
    //cerr << "bound_king_size = " << bound_king_size << "\n";
    for (int i=0; i<bound_king_size; i++) {
      permutations.back()[i] = tmp[INV_REMAP_BOUND_KING[i]];
      assert(0<=permutations.back()[i]  &&  permutations.back()[i]<bound_king_size);
      //cerr << "permutations.back()[" << i << "] = " << permutations.back()[i] << "\n";
    }
  }

  for (int i=0; i<64; i++)
    bit_perm_and_permute_pos[i] = 0;

  for (int pn=0; pn<num_pieces; pn++) {
    // Process one permutation at a time.

    int perm_size = 64;
    // If this is a bound king then perm_size is less
    if (6 > log_bdd_size - 6*pn) perm_size >>= 6 - (log_bdd_size - 6*pn);

    { // Update bit_perm_and_permute_pos
      for (int i=0; i<perm_size; i++)
        bit_perm_and_permute_pos[i] |= permutations[pn][i]<<(6*pn);
    }

    { // Relocate bdd_table entries
      int bit_pos = 6*pn;
      int for_loop_count = bdd_size / perm_size;

      for (int i=0; i<for_loop_count; i++) {
        // Example: KQRK, pn=1
        // i =         xxxx xxxxxx xxxxxx
        // ii = xxxx xxxxxx 000000 xxxxxx
        int ii = ((i<<6) & -(1<<(bit_pos+6))) | (i & ((1<<bit_pos)-1));

        uint8_t tmp[64]; // 64>=perm_size
        for (int j=0; j<perm_size; j++)
          tmp[j] = bdd_table[ii | (j<<bit_pos)];
        for (int j=0; j<perm_size; j++)
          bdd_table[ii | (permutations[pn][j]<<bit_pos)] = tmp[j];
      }
    }
  }
}


void BDD::apply_previously_found_bit_perm(uint8_t *bit_perm,
    uint8_t *bdd_table, int log_bdd_size)
{
  { // Is bit_perm the identity mapping?
    bool identity_mapping = true;
    for (int i=0; i<32; i++)
      if (i != bit_perm[i]) {
        identity_mapping = false;
        break;
      }
    if (identity_mapping) return;
  }

  // Define pattern
  for (uint i=0; i<5; i++) {
    assert(pattern[i] == (uint)(0x3F<<(6*i)));//Otherwise a bit_perm is already applied
    pattern[i] = 0;
    for (int j=0; j<6; j++)
      pattern[i] |= 1 << bit_perm[6*i+j];
  }

  // Update bit_perm_and_permute_pos (Might previously have been
  // set by permute_square_enumeration).
  for (int p=0; p<64; p++) {
    uint32_t old = bit_perm_and_permute_pos[p];
    bit_perm_and_permute_pos[p] = 0;
    for (int i=0; i<30; i++) {
      // Move bit i to position bit_perm[i]
      bit_perm_and_permute_pos[p] |= ((old>>i)&1) << bit_perm[i];
    }
  }

  // TODO: do nothing if bit_perm is identity

  if (bdd_table) {
    // Relocate bdd_table entries
    assert(log_bdd_size);

    // Optimize this process a bit by creating a table
    // such that the new location of an entry can be decided by a
    // few table lookups
    uint32_t byte_perm[4][256];
    for (int i=0; i<4; i++) {
      for (int j=0; j<256; j++) {
        byte_perm[i][j] = 0;
        for (int b=0; b<8; b++)
          byte_perm[i][j] |= ((j>>b)&1) << bit_perm[8*i+b];
      }
    }

    // Example: bit_perm = {1,2,0}
    // 000 -> 000
    // 001 -> 010 -> 100 -> 001
    // 010 % relocated[010] = true
    // 011 -> 110 -> 101 -> 011
    // 100 %
    // 101 %
    // 110 %
    // 111 -> 111
    int size = 1 << log_bdd_size;
    BitList relocated(size, false);
    for (int i=0; i<size; i++) {
      if (!relocated[i]) {
        // Remap entry i -> entry permute(i)
        int j = i;
        uint8_t replaced_value = bdd_table[i];
        do {
          relocated.set(j);
          j = byte_perm[0][j&0xFF] | byte_perm[1][(j>>8)&0xFF] |
              byte_perm[2][(j>>16)&0xFF] | byte_perm[3][(j>>24)&0xFF];
          swap(replaced_value, bdd_table[j]);
        } while (j!=i);
      }
    }
  }
}


void BDD::create_clustering_kind_1() {
  clustering_kind = 1;
  clustering.base_subsets = (uint16_t*)malloc(sizeof(uint16_t)*5*64);

  for (int i=0; i<5*64; i++)
    clustering.base_subsets[i] = 0;
}

void BDD::init_no_clustering(uint8_t *bdd_table, int log_bdd_size,
    bool do_preprocessing, bool calc_sifting,
    bool do_mapping_after_sifting)
{
  do_mapping_after_sifting &= do_preprocessing & calc_sifting;

  cbo << "BDD::init_no_clustering(" << log_bdd_size << ","
      << do_preprocessing << "," << calc_sifting << ")\n";

  assert(clustering_kind == 0);
  map_subset_to_cluster.init(1, 0);

  sub_bdds = vector<BinaryDecisionDiagram *>(1);
  sub_bdds[0] = new BinaryDecisionDiagram();

  if (do_preprocessing  &&  !do_mapping_after_sifting) {
    map_wildcards(bdd_table, log_bdd_size);
  }

  uint8_t bit_perm[32];
  for (int i=0; i<32; i++) bit_perm[i] = i;
  sub_bdds[0]->init(bdd_table, log_bdd_size, convert_table.size()-1,
      !do_mapping_after_sifting, calc_sifting, bit_perm);

  if (do_mapping_after_sifting) {
    // do_preprocessing and calc_sifting also true
    apply_previously_found_bit_perm(bit_perm, bdd_table, log_bdd_size);
    map_wildcards(bdd_table, log_bdd_size);
    sub_bdds[0]->init(bdd_table, log_bdd_size, convert_table.size()-1, true);

  } else {

    if (calc_sifting) apply_previously_found_bit_perm(bit_perm);
  }
}


void BDD::use_king_pos_as_subsets(__attribute__((unused)) uint8_t *bdd_table, int log_bdd_size) {
  assert(!clustering_kind);
  create_clustering_kind_1();

  int num_pieces = (log_bdd_size+2)/6;

  int piece = num_pieces-2;
  for (int pos=0; pos<64; pos++)
    clustering.base_subsets[(piece<<6) | pos] = pos;

  piece = num_pieces-1;
  int num_squares = 1 << (log_bdd_size - 6*piece);
  for (int pos=0; pos<num_squares; pos++)
    clustering.base_subsets[(piece<<6) | pos] = 64*pos;
}

void BDD::determine_cluster_subsets_based_on_clustering(vector<int> base_subset_sizes,
    uint8_t *bdd_table, int log_bdd_size) {
  assert(!clustering_kind);
  int num_pieces = base_subset_sizes.size();
  int bdd_size = 1 << log_bdd_size;

  // For each piece we must divide the squares of the board into
  // a specified number of subsets.
  // Example: KRK:
  // The king which has only 10 legal squares is mapped to [0..3[
  // The 2 other pieces is mapped to [0..6[
  // This divides the table into 3*6*6 = 108 subsets, which must
  // then be clustered appropriately.

  create_clustering_kind_1();

  // Calculate the number of subsets the table is divided into.
  int num_subsets = 1;
  for (uint i=0; i<base_subset_sizes.size(); i++) {
    num_subsets *= base_subset_sizes[i];
    assert(num_subsets <= 65535);
  }

  cbo << "The table is divided into subsets based on the base subsets below:\n";

  // Initialize the table base_subsets such that
  // sum_{p=0}^{n-1}(base_subsets[p][position of piece p]) = subset number
  // where subset number \in [0..num_subsets[
  int factor = 1;
  for (int piece=0; piece<num_pieces; piece++) {

    // The bound king only have 2^4 or 2^5 legal squares
    int num_squares = 64;
    if (6*piece+6 > log_bdd_size) num_squares >>= (6*piece+6)-log_bdd_size;

    vector<vector<uint> > elements(num_squares);
    // Initialize elements to find_clusters algorithm
    for (int pos=0; pos<num_squares; pos++)
      elements[pos] = vector<uint>(convert_table.size());
    for (int i=0; i<bdd_size; i++) {
      int pos = (i >> (6*piece)) & 0x3F;
      elements[pos][ bdd_table[i] ]++;
    }

    // Set the entries representing wildcards to zero.
    // Hence wildcards will not have any effect on the fitness.
    for (int pos=0; pos<num_squares; pos++)
      elements[pos][0] = 0;

    vector<int> clusters = find_clusters2(elements,
        base_subset_sizes[piece]);
    {
      int tmp[64];
      for(int i=0; i<64; i++)
        tmp[i] = i<num_squares ? clusters[i] : 99;
      print_signed_map64(cbo, tmp, 2, 10);
    }


    // Make sure that position 0 is mapped to cluster 0
    if (clusters[0] != 0) {
      int wrong = clusters[0];
      for (int pos=0; pos<num_squares; pos++)
        if (clusters[pos]==0  ||  clusters[pos]==wrong)
          clusters[pos] = wrong - clusters[pos];
    }

    for (int pos=0; pos<num_squares; pos++)
      clustering.base_subsets[(piece<<6) | pos] = factor*clusters[pos];

    factor *= base_subset_sizes[piece];
  }

  // Now the subset, that a specific position belongs to,
  // can easily be determined. Example: KRK index (4+6+6 bits)
  // subset_number = clustering.base_subsets[index & 0x3F] +
  // base_subsets[64 | ((index>>6) & 0x3F)] + base_subsets[128 | (index>>12)]
}

struct SplitHelp {
  BitList bl;
  int mem_size;
};


void BDD::init(uint8_t *bdd_table, int log_bdd_size, __attribute__((unused)) const uint8_t inv_bit_perm[5][64], // TODO: Unused ???
    bool do_preprocessing, bool calc_sifting, bool do_mapping_after_sifting) {
  assert(clustering_kind);
  do_mapping_after_sifting &= do_preprocessing & calc_sifting;

  cbo << "BDD::init(" << log_bdd_size << ","
      << do_preprocessing << "," << calc_sifting << ")\n";

  int bdd_size = 1 << log_bdd_size;

  int num_subsets = get_num_subsets();

  // BDD Split algorithm.
  // bdd_table is divided, ie. split up into 2 tables t1,t2 of same size
  // with the following property (0 represents ENDGAME_TABLE_ILLEGAL):
  // (bdd_table[i] == t1[i] == t2[i] == 0)  ||
  // (bdd_table[i] == t1[i] != 0  &&  t2[i] == 0)  ||
  // (bdd_table[i] == t2[i] != 0  &&  t1[i] == 0)
  //
  // if the tables t1 and t2 can be compressed better than bdd_table,
  // then this division is made permanent, and then it is rekursively
  // tried if t1 and t2 should be divided further.

  // Each SplitHelp entry denotes the part of bdd_table covered by this
  // cluster together with the resulting compressed size.
  // unprocessed contains the clusters which have not been testet yet.
  // processed contains cluster that should not be further subdivided.
  stack<SplitHelp> unprocessed;
  stack<SplitHelp> processed;

  // Make a copy of bdd_table to perform the mappings of wildcards on.
  uint8_t *test_table = new uint8_t[bdd_size];
  memcpy(test_table, bdd_table, bdd_size);

  { // Calculate the size of the table if it is compressed as a single
    // cluster. Also apply the sifting algorithm, and let the resulting
    // bit permutation be applied to bdd_table
    BinaryDecisionDiagram bdd;
    if (do_preprocessing  &&  !do_mapping_after_sifting) {
      map_wildcards(test_table, log_bdd_size);
    }
    uint8_t bit_perm[32];
    for (int i=0; i<32; i++) bit_perm[i] = i;
    bdd.init(test_table, log_bdd_size, convert_table.size()-1,
        false, calc_sifting, bit_perm);

    if (calc_sifting) {
      // Rearrange bdd_table entries according to the sifting result
      apply_previously_found_bit_perm(bit_perm, bdd_table, log_bdd_size);
    }

    if (do_mapping_after_sifting) {
      memcpy(test_table, bdd_table, bdd_size);
      map_wildcards(test_table, log_bdd_size);
      bdd.init(test_table, log_bdd_size, convert_table.size()-1, false);
    }

    // Add the entire table as an unprocessed cluster
    SplitHelp entire_table;
    entire_table.bl.init(num_subsets, true);
    entire_table.mem_size = bdd.memory_consumption();
    unprocessed.push(entire_table);
  }

  bool first = true;
  while (!unprocessed.empty()) {
    SplitHelp &examined_cluster = unprocessed.top();

    int count_active_subsets = 0;
    vector<int> ref(num_subsets), back_ref(num_subsets);
    for (int i=0; i<num_subsets; i++)
      if (examined_cluster.bl[i]) {
        ref[back_ref[count_active_subsets] = i] = count_active_subsets;
        count_active_subsets++;
      }

    vector<vector<uint> > elements(count_active_subsets);
    vector<double> weights(count_active_subsets);
    for (uint i=0; i<elements.size(); i++)
      elements[i] = vector<uint>(convert_table.size());

    for (int i=0; i<bdd_size; i++) {
      pair<int, int> p = old_index_to_new_index_and_subset_number(i);

      // Do not separate on don't cares!
      // Also, the cluster value might be invalid on broken positions!
      if (bdd_table[p.first]  &&  examined_cluster.bl[p.second])
        ++elements[ref[p.second]][bdd_table[p.first]];
    }

    if (first) { // Print clusters as a latex tabular
      first = false;

      if (count_active_subsets > 99) {
        cbo << "Subsets will not be written as a latex table - there are too many!\n";
      } else {

        int _count[256];
        memset(_count, 0, sizeof(int)*256);
        int *count = &(_count[128]);

        for (int i=0; i<count_active_subsets; i++) {
          for (uint j=0; j<convert_table.size(); j++) {
            count[(int)convert_table[j]] += elements[i][j];
          }
        }

        int num_diff = 0;
        for (int i=-128; i<128; i++)
          if (count[i]) ++num_diff;
        if (num_diff+1 != (int)convert_table.size()) {
          cbo << "num_diff+1 != convert_table.size() !!!\n";
          assert(0);
          exit(1);
        }

        cbo << "\\begin{center}\n"
            << "\\begin{scriptsize}\n"
            << "\\begin{tabular}{|r||";
        for (int i=0; i<=num_diff; i++)
          cbo << "@{}r@{ }|";
        cbo << "}\n\\hline\n";
        for (int i=0; i<=num_diff; i++)
          cbo << "&";
        cbo << "0'th order\\\\\ncluster:";
        for (int i=-128; i<128; i++)
          if (count[i])
            cbo << " & " << _endgame_value_to_string(i);
        cbo << " & entropy\\\\\n\\hline\n";

        double sum_entropy = 0;
        for (int i=0; i<count_active_subsets; i++) {
          cbo << i;
          double entropy = 0;
          int c = 0;
          for (int v=-128; v<128; v++)
            if (count[v])
              for (uint j=0; j<convert_table.size(); j++)
                if (v == (int)convert_table[j]) {
                  int n = (int)(elements[i][j]);
                  cbo << " & " << n;

                  if (n) {
                    entropy -= n*log2(n);
                    c += n;
                  }
                }
          if (c) entropy += c*log2(c);
          sum_entropy += entropy;

          cbo << " & " << entropy << "\\\\\n";
        }

        double entropy = 0;
        int c = 0;
        cbo << "\\hline\nSum";
        for (int v=-128; v<128; v++)
          if (count[v]) {
            cbo << " & " << count[v];

            entropy -= count[v]*log2(count[v]);
            c += count[v];
          }
        if (c) entropy += c*log2(c);

        cbo << " & " << sum_entropy << "\\\\\n";

        cbo << "\\hline\n"
            << "\\end{tabular}\n"
            << "\\end{scriptsize}\n"
            << "\\end{center}\n";

        cbo << "Total entropy without clustering = " << entropy << "\\\\\n";

        cbo << "\n";
      }
    }


    // This cluster is divided into 2 given by a and b
    vector<int> clusters = find_clusters2(elements, 2);
    SplitHelp a,b;
    a.bl.init(num_subsets, false);
    b.bl.init(num_subsets, false);

    // define the 2 new clusters a and b
    int count_a=0, count_b=0;
    for (uint i=0; i<elements.size(); i++) {
      if (clusters[i] == 0) {
        count_a++;
        a.bl.set(back_ref[i]);
      } else if (clusters[i] == 1) {
        count_b++;
        b.bl.set(back_ref[i]);
      } else {
        assert(0);
      }
    }
    cbo << "Number of subsets in each cluster = " << count_a << "," << count_b << "\n";
    //a.bl.print(cbo);
    //b.bl.print(cbo);

    // For a and b, build the BinaryDecisionDiagrams
    // and calculate their sizes.
    {// a stuff
      for (int i=0; i<bdd_size; i++) {
        pair<int, int> p = old_index_to_new_index_and_subset_number(i);

        if (a.bl[p.second]) test_table[p.first] = bdd_table[p.first];
        else test_table[p.first] = 0; // wildcard
      }

#ifndef NDEBUG
      for (int i=0; i<bdd_size; i++)
        assert(test_table[i] < convert_table.size());
#endif

      BinaryDecisionDiagram bdd;
      if (do_preprocessing) map_wildcards(test_table, log_bdd_size);

#ifndef NDEBUG
      for (int i=0; i<bdd_size; i++)
        assert(test_table[i] < convert_table.size());
#endif

      bdd.init(test_table, log_bdd_size, convert_table.size()-1, false);
      a.mem_size = bdd.memory_consumption();
    }

    {// b stuff
      for (int i=0; i<bdd_size; i++) {
        pair<int, int> p = old_index_to_new_index_and_subset_number(i);

        if (b.bl[p.second]) test_table[p.first] = bdd_table[p.first];
        else test_table[p.first] = 0; // wildcard
      }

      BinaryDecisionDiagram bdd;
      if (do_preprocessing) map_wildcards(test_table, log_bdd_size);
      bdd.init(test_table, log_bdd_size, convert_table.size()-1, false);
      b.mem_size = bdd.memory_consumption();
    }



    { // Print cluster before and clusters after as a latex tabular

      int _count[2*256];
      memset(_count, 0, sizeof(int)*2*256);
      int *count[2];
      count[0] = &(_count[128]);
      count[1] = &(_count[128+256]);

      for (uint i=0; i<elements.size(); i++)
        for (uint j=0; j<elements[i].size(); j++)
          count[clusters[i]][(int)convert_table[j]] += elements[i][j];

      int num_diff = 0;
      for (int i=-128; i<128; i++)
        if (count[0][i]+count[1][i]) ++num_diff;

      cbo << "\\begin{center}\n"
          << "\\begin{scriptsize}\n"
          << "\\begin{tabular}{|c||";
      for (int i=0; i<=num_diff; i++)
        cbo << "@{}r@{ }|";
      cbo << "@{}r@{ }|}\n\\hline\n";
      for (int i=0; i<=num_diff; i++)
        cbo << "&";
      cbo << "0'th order&\\\\\n";
      for (int i=-128; i<128; i++)
        if (count[0][i]+count[1][i])
          cbo << " & " << _endgame_value_to_string(i);
      cbo << " & entropy & Size\\\\\n\\hline\n";

      for (int _cl=0; _cl<3; _cl++) {
        int cl = _cl ? _cl-1 : 2;
        switch(cl) {
        case 0:
          cbo << "C1";
          break;
        case 1:
          cbo << "C2";
          break;
        case 2:
          cbo << "C1 $\\cup$ C2";
          break;
        }
        double entropy = 0;
        int c = 0;
        for (int v=-128; v<128; v++) {
          if (count[0][v] + count[1][v]) {
            int ct_index = 0;
            for (uint j=0; j<convert_table.size(); j++) {
              if (v == (int)convert_table[j]) {
                ct_index = j;
                break;
              }
            }

            int sum = 0;
            for (uint i=0; i<elements.size(); i++) {
              if (cl==2  ||  clusters[i] == cl) {
                sum += (int)elements[i][ct_index];
              }
            }
            cbo << " & " << sum;

            if (sum) {
              entropy -= sum*log2(sum);
              c += sum;
            }
          }
        }

        //.....

        if (c) entropy += c*log2(c);

        cbo << " & " << entropy << " & "
            << (cl == 0 ? a.mem_size : (cl == 1 ? b.mem_size : examined_cluster.mem_size))
            << "\\\\\n";

      }

      cbo << "\\hline\n"
          << "\\end{tabular}\n"
          << "\\end{scriptsize}\n"
          << "\\end{center}\n";
      cbo << "\n";
    }





    // Decide whether to split.
    // For each time every cluster has been split, the mapping from subsets
    // to clusters require 1 extra bit per entry.
    // Whatever...
    int MARGIN = num_subsets / 8;

    cbo << "Size: " << examined_cluster.mem_size << " -> " << a.mem_size << " + "
        << b.mem_size << "\n";

    if (a.mem_size + b.mem_size + MARGIN < examined_cluster.mem_size) {
      // Do split
      cbo << "Splitting cluster of (num. subsets, size) = ("
          << count_active_subsets << "," << examined_cluster.mem_size << ") into the 2 clusters ("
          << count_a << "," << a.mem_size << ") and (" << count_b << "," << b.mem_size << ")\n";

      unprocessed.pop();

      if (a.bl.count_on() >= 2) {
        unprocessed.push(a);
      } else {
        //assert(0); // just to check if it happens
        processed.push(a);
      }

      if (b.bl.count_on() >= 2) {
        unprocessed.push(b);
      } else {
        //assert(0); // just to check if it happens
        processed.push(b);
      }

    } else {

      cbo << "Do not split (" << count_active_subsets << "," << examined_cluster.mem_size << ")\n";
      processed.push(examined_cluster);
      unprocessed.pop();
    }
  }

  // Now there is no unprocessed clusters left.
  // Each element in processed defines a cluster from which
  // a BinaryDecisionDiagram will be build.

  //cerr << "Hertil ok 4\n";

  int num_clusters = processed.size();
  map_subset_to_cluster.init(num_subsets, num_clusters-1);
  sub_bdds = vector<BinaryDecisionDiagram *>(num_clusters);

  for (int num=0; num<num_clusters; num++) {
    SplitHelp &tmp = processed.top();
    for (int i=0; i<num_subsets; i++)
      if (tmp.bl[i]) map_subset_to_cluster.update(i, num);

    for (int i=0; i<bdd_size; i++) {
      pair<int, int> p = old_index_to_new_index_and_subset_number(i);

      if (tmp.bl[p.second]) test_table[p.first] = bdd_table[p.first];
      else test_table[p.first] = 0; // wildcard

      /*
      if (tmp.bl[index_to_cv(inv_bit_perm, i)]) test_table[i] = bdd_table[i];
      else test_table[i] = 0; // wildcard
       */
    }

    sub_bdds[num] = new BinaryDecisionDiagram();
    if (do_preprocessing) map_wildcards(test_table, log_bdd_size);
    sub_bdds[num]->init(test_table, log_bdd_size, convert_table.size()-1, true);

    processed.pop();
  }


  if (num_clusters > 1) {
    map_subset_to_cluster.print(cbo);

  } else {
    cerr << "Clustering failed!\n";
    clustering_kind = 0;
    map_subset_to_cluster.init(1, 0);
  }

  // todo: initialize from bit_perm...

  cbo << "Number of clusters = " << (int)num_clusters << '\n'
      << "Memory consumption = " << memory_consumption() << '\n';
}


void BDD::print(ostream &os, bool print_bdds) {
  os << "BDD:\tconvert_table(size = " << (int)convert_table.size() << "):\n\t";
  for (uint i=0; i<convert_table.size(); i++)
    os << '(' << i << ',' << (int)convert_table[i] << "),";
  os << '\n';

  //os << "Mapping: " << mapping_name((int)mapping_type) << ":\n";
  //print_map64(os, mapping, 2, 10);

  if (clustering_kind == 1) {
    for (int i=0; i<5; i++) {
      os << "base_subsets[" << i << "]:\n";
      print_signed_map64(os, &(clustering.base_subsets[i<<6]), 5, 10);
    }
  }

  os << "Number of clusters = " << (int)sub_bdds.size();
  if (sub_bdds.size() == 1) {
    os << '\n';
  } else {
    os << ", split_decision list:\n\t";
    if (sub_bdds.size() < 10) {
      for (uint i=0; i<map_subset_to_cluster.size(); i++)
        os << map_subset_to_cluster[i];
    } else {
      for (uint i=0; i<map_subset_to_cluster.size(); i++)
        os << map_subset_to_cluster[i] << ' ';
    }
    os << '\n';
  }

  {
    for (int i=0; i<5; i++)
      os << "pattern[" << i << "] = " << toString(pattern[i], 32, 2) << "\n";

    os << "bit_perm_and_permute_pos[]:\n";
    for (int i=0; i<64; i++)
      os << "\t" << i << "\t" << toString(bit_perm_and_permute_pos[i], 32, 2) << "\n";
  }

  if (print_bdds) {
    for (uint i=0; i<sub_bdds.size(); i++)
      sub_bdds[i]->print(os);
  } else {
    for (uint i=0; i<sub_bdds.size(); i++)
      os << "memory_consumption(sub_bdds[" << i << "]) = "
      << sub_bdds[i]->memory_consumption() << '\n';
  }
}
