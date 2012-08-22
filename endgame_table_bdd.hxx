#ifndef _ENDGAME_TABLE_BDD_
#define _ENDGAME_TABLE_BDD_

#include <iostream>

#include "typedefs.hxx"
#include "binary_decision_diagram.hxx"

#include "endgame_clustering_functions.hxx"

class BDD {
public:
  BDD();
  ~BDD();
  void clear();

  void load(int fd);
  void save(int fd);

  bool load(string filename);
  void save(string filename);

  void set_convert_table(const vector<char> &_convert_table) {
    convert_table = vector<char>(_convert_table);
  }
  
  // permute_square_enumeration changes bdd_table accordingly
  // permutations.back()[i] is replaced by permutations.back()[INV_REMAP_BOUND_KING[i]]
  // The permutations are stored in bit_perm_and_permute_pos
  // IMPORTANT!:
  // The permutation applied for a bound king in an endgame with pawns MUST
  // map the a1-d8 rectangle to [0..32[ and if it is a pawnless endgame then
  // the a1-d1-d4 triangle MUST be mapped to [0..16[
  // All permutations must have size 64 (even though not all entries are used for
  // the bound king)
  void permute_square_enumeration(uchar *bdd_table, int log_bdd_size,
				  vector<vector<int> > permutations);

  // IMPORTANT: Call AFTER permute_square_enumeration
  //
  // Can only be applied once. If applied, sifting algorithm may not be used.
  // final index = bit_perm(permutations(original index))
  // bit_perm must be a pointer to 32-sized array of uchar
  // 
  // pattern and bit_perm_and_permute_pos are updated.
  // If also bdd_table and log_bdd_table are specified then the entries are
  // remapped according to the bit permutation.
  void apply_previously_found_bit_perm(uchar *bit_perm,
				       uchar *bdd_table=0, int log_bdd_size=0);

  // do_mapping_after_sifting only used if do_preprocessing and calc_sifting are true.
  void init_no_clustering(uchar *bdd_table, int log_bdd_size,
			  bool do_preprocessing = true, bool calc_sifting = true,
			  bool do_mapping_after_sifting = false);

  // init constucts a (set of) BinaryDecisionDiagram(s) from bdd_table.
  //     bdd_table[0..2^log_bdd_size[ \in [0..convert_table.size()[
  // 
  // IMPORTANT: May not be called before base_subsets has been set/computed!
  //            May not be called before convert_table has been defined.
  //            May not be called before permute_square_enumeration/apply_previously_found_bit_perm
  //
  // do_preprocessing : Should the wildcards (values 0) be mapped intelligently?
  //
  // calc_sifting : Should the sifting algorithm be applied?
  //                If true, then the bit permutation will be decided once and for
  //                all by running the sifting algorithm on the unsplitted data.
  void init(uchar *bdd_table, int log_bdd_size, const uchar inv_bit_perm[5][64],
	    bool do_preprocessing = true, bool calc_sifting = true, bool do_mapping_after_sifting=false);

  char operator[](BDD_Index bdd_index) {
    /*
    cerr << "BDD_Index/c.v. = ("
	 << "(" << (int)bdd_index[0] << " " << (int)base_subsets[ bdd_index[0] ] << ")"
	 << "(" << (int)bdd_index[1] << " " << (int)base_subsets[64 | bdd_index[1] ] << ")"
	 << "(" << (int)bdd_index[2] << " " << (int)base_subsets[128 | bdd_index[2] ] << ")"
	 << "(" << (int)bdd_index[3] << " " << (int)base_subsets[192 | bdd_index[3] ] << ")"
#ifdef ALLOW_5_MEN_ENDGAME
	 << "(" << (int)bdd_index[4] << " " << (int)base_subsets[256 | bdd_index[4] ] << ")"
#endif
	 << ")\n";
    */

    int cluster_number;
    switch (clustering_kind) {
    case 0:
      cluster_number = 0;
      break;
    case 1:
      cluster_number = map_subset_to_cluster[clustering.base_subsets[ bdd_index[0] ]
					     + clustering.base_subsets[64 | bdd_index[1] ]
					     + clustering.base_subsets[128 | bdd_index[2] ]
					     + clustering.base_subsets[192 | bdd_index[3] ]
#ifdef ALLOW_5_MEN_ENDGAME
					     + clustering.base_subsets[256 | bdd_index[4] ]
#endif
      ];
      break;
    case 2:
      cluster_number =
	map_subset_to_cluster[cluster_functions[clustering.cf_index](bdd_index[0], bdd_index[1]
								     , bdd_index[2], bdd_index[3]
#ifdef ALLOW_5_MEN_ENDGAME
								     , bdd_index[4]
#endif
								     )];
      break;
    default:
      assert(0);
      cluster_number = -1; // Avoid warning!
    }

    //if (!sub_bdd_loaded[cluster_number]) {
      //todo:
      //handler_cluster_not_loaded(
    //}

    int index = ((pattern[0]  &  bit_perm_and_permute_pos[bdd_index[0]])
		 | (pattern[1]  &  bit_perm_and_permute_pos[bdd_index[1]])
		 | (pattern[2]  &  bit_perm_and_permute_pos[bdd_index[2]])
		 | (pattern[3]  &  bit_perm_and_permute_pos[bdd_index[3]])
#ifdef ALLOW_5_MEN_ENDGAME
		 | (pattern[4]  &  bit_perm_and_permute_pos[bdd_index[4]]))
#endif
      ;

    return convert_table[(*(sub_bdds[cluster_number]))[index]];
  }

  char operator[](int index) {
    BDD_Index bdd_index;
    bdd_index[0] = index & 0x3F;
    bdd_index[1] = (index >> 6) & 0x3F;
    bdd_index[2] = (index >> 12) & 0x3F;
    bdd_index[3] = (index >> 18) & 0x3F;
#ifdef ALLOW_5_MEN_ENDGAME
    bdd_index[4] = (index >> 24) & 0x3F;
#endif
    return (*this)[bdd_index];
  }

  int memory_consumption();

  void print(ostream &os, bool print_bdds = false);

  // Impossible to implement
  //vector<uchar> get_sifting_bit_perm();
  //vector<vector<int> > get_square_permutations();


  // The subset a position belongs to depends on the unmapped position of the pieces.
  // Hence the method below must be called on a bdd_table BEFORE
  // the square permutation
  //
  // base_subset_sizes[piece number] determines how many subsets the positions
  // of this endgame is divided into. Example: If all base_subset_sizes are 4,
  // Then a 3-piece endgame will be divided into 4^3 subsets, where each subset s1s2s3s4
  // contains all positions with piece pi in base subset si. The higher the base subset
  // sizes are, the "better" subsets will be made (more detailed). But the cost
  // is that the table giving the mapping from subset to cluster will increase in size.
  void determine_cluster_subsets_based_on_clustering(vector<int> base_subset_sizes,
						     uchar *bdd_table, int log_bdd_size);
  void use_king_pos_as_subsets(uchar *bdd_table, int log_bdd_size);

  bool try_using_specialized_cluster_function(int endgame_hash_value) {
    assert(!clustering_kind);
    assert(0<=endgame_hash_value && endgame_hash_value<DB_ARRAY_LENGTH);

    if (!cluster_functions[endgame_hash_value]) return false;
    clustering_kind = 2;
    clustering.cf_index = endgame_hash_value;
    return true;
  }


  int get_num_sub_bdds() { return sub_bdds.size(); }

  pair<int, int> old_index_to_new_index_and_subset_number(int index) {

    int subset_number;
    switch (clustering_kind) {
    case 0:
      assert(0);
      break;

    case 1:
      subset_number = (clustering.base_subsets[index & 0x3F]
		       + clustering.base_subsets[64 | ((index>>6) & 0x3F)]
		       + clustering.base_subsets[128 | ((index>>12) & 0x3F)]
#ifdef ALLOW_5_MEN_ENDGAME
		       + clustering.base_subsets[192 | ((index>>18) & 0x3F)]
		       + clustering.base_subsets[256 | (index>>24)]
#else
		       + clustering.base_subsets[192 | (index>>18)]
#endif
		       );
      break;

    case 2:
      //cerr << "index = " << toString(index, 8) << " (octal representation)\n";
      subset_number = cluster_functions[clustering.cf_index](index & 0x3F
							     , (index>>6) & 0x3F
							     , (index>>12) & 0x3F
#ifdef ALLOW_5_MEN_ENDGAME
							     , (index>>18) & 0x3F
							     , index>>24
#else
							     , index>>18
#endif
							     );
      break;
    default:
      assert(0);
    }


    index = ((pattern[0]  &  bit_perm_and_permute_pos[index & 0x3F])
	     | (pattern[1]  &  bit_perm_and_permute_pos[(index>>6) & 0x3F])
	     | (pattern[2]  &  bit_perm_and_permute_pos[(index>>12) & 0x3F])
#ifdef ALLOW_5_MEN_ENDGAME
	     | (pattern[3]  &  bit_perm_and_permute_pos[(index>>18) & 0x3F])
	     | (pattern[4]  &  bit_perm_and_permute_pos[index>>24])
#else
	     | (pattern[3]  &  bit_perm_and_permute_pos[index>>18])
#endif
	     );

    return pair<int,int>(index, subset_number);
  }


protected:
  void create_clustering_kind_1();

  int get_num_subsets() {
    switch (clustering_kind) {
    case 0:
      return 0;// Maybe 1 is more correct, but by using 0 blah...
    case 1:
      {
	int num_subsets = 1;
	// Find num_subsets as the maximal number that can be specified by a sum
	// from base_subsets plus 1.
	for (int piece=0; piece<5; piece++) {
	  int max = 0;
	  for (int i=0; i<64; i++)
	    if (clustering.base_subsets[(piece<<6) | i] > max)
	      max = clustering.base_subsets[(piece<<6) | i];
	  num_subsets += max;
	}
	return num_subsets;
      }
    case 2:
      return cluster_functions_num_values[clustering.cf_index];
    default:
      assert(0);
      return -1; // Avoid warning!
    }
  }

  // convert_table converts from the values in [0..ct_size[ contained
  // by the bdd's to the actual distance to mate etc. values
  vector<char> convert_table;


  int clustering_kind;//0:none, 1:ordinary, 2:function
  union {
    ushort *base_subsets;// Usage: base_subsets[(piece_num << 6) | pos]
    int cf_index;
    //ClusterFunction cluster_function;
  } clustering;

  CompressedList map_subset_to_cluster;


  // bit_perm_and_permute_pos converts a position in the following way:
  // First it remaps the position according to some permutation.
  // Then it applies a bit permutation. The lower 6 bit denoting the position
  // is mapped to 6 other specified bits.
  // bit_perm_and_permute_pos can be applied to all 5 pieces, because
  // ... = 023210442320123023444001431113, pattern[0..4] =
  //       100001000001000100000110000000
  //       000010000000100000000001001110
  //       010100001010010010000000000000
  //       001000000100001001000000010001
  //       000000110000000000111000100000
  uint pattern[5];// Do NOT replace 5 with MAX_MEN
  uint bit_perm_and_permute_pos[64];


  vector<BinaryDecisionDiagram *> sub_bdds;

  /*
#ifndef NDEBUG
  uchar debug_bit_perm[32];
  uchar debug_square_permutations[5][64];
#endif
  */
};


#endif

/*
  0: 0 0 0 0 0 0 1 1 1 1 
 10: 1 1 1 1 1 1 1 1 1 1 
 20: 1 1 1 1 0 0 1 0 0 0 
 30: 0 0 0 0 0 0 0 0 0 0 
 40: 0 0 0 0 0 0 0 0 1 1 
 50: 1 1 1 1 1 1 1 1 1 1 
 60: 0 0 0 0 0 0 0 0 0 0 
 70: 0 0 0 0 0 0 0 0 0 0 
 80: 0 0 0 0 0 0 0 0 0 0 
 90: 0 0 0 0 0 0 0 0 0 0 
100: 0 0 0 0 0 0 0 0


0 0 0 0 0 0 T T T T 
T T T T T T T T T T 
T T T T 0 0 T 0 0 0 
0 0 0 0 0 0 0 0 0 0 
0 0 0 0 0 0 0 0 T T 
T T T T T T T T T T 
0 0 0 0 0 0 0 0 0 0 
0 0 0 0 0 0 0 0 0 0 
0 0 0 0 0 0 0 0 0 0 
0 0 0 0 0 0 0 0 0 0 
0 0 0 0 0 0 0 0
*/
