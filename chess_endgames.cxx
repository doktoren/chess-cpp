#include "chess_endgames.hxx"

using namespace std;

typedef unsigned char uchar;


std::string endgame_value_to_string(char v) {
  if (v <= -124) {
    switch (v) {
    case -124: return "Win";
    case -125: return "Loss";
    case -126: return "Draw";
    case -127: return "????";
    case -128: return "illegal";
    }
  }
  if (v>0) {
    char tmp[12];
    sprintf(tmp, "M%d", v);
    return std::string(tmp);
  } else {
    char tmp[12];
    sprintf(tmp, "-M%d", -v);
    return std::string(tmp);
  }
}




#define ENDGAME_TABLE_WIN -124
#define ENDGAME_TABLE_LOSS -125
#define ENDGAME_TABLE_DRAW -126
#define ENDGAME_TABLE_UNKNOWN -127
#define ENDGAME_TABLE_ILLEGAL -128


#ifdef ALLOW_5_MEN_ENDGAME

#define DB_WPAWN_VALUE (2*1)
#define DB_WKNIGHT_VALUE (2*4)
#define DB_WBISHOP_VALUE (2*13)
#define DB_WROOK_VALUE (2*32)
#define DB_WQUEEN_VALUE (2*71)
#define DB_WKING_VALUE 0
#define DB_BPAWN_VALUE (2*124)
#define DB_BKNIGHT_VALUE (2*218)
#define DB_BBISHOP_VALUE (2*375)
#define DB_BROOK_VALUE (2*572)
#define DB_BQUEEN_VALUE (2*744)
#define DB_BKING_VALUE 0

#define DB_TABLE_SIZE (2*(3*744+1))

#else

#define DB_WPAWN_VALUE (2*1)
#define DB_WKNIGHT_VALUE (2*3)
#define DB_WBISHOP_VALUE (2*7)
#define DB_WROOK_VALUE (2*12)
#define DB_WQUEEN_VALUE (2*20)
#define DB_WKING_VALUE 0
#define DB_BPAWN_VALUE (2*30)
#define DB_BKNIGHT_VALUE (2*44)
#define DB_BBISHOP_VALUE (2*65)
#define DB_BROOK_VALUE (2*80)
#define DB_BQUEEN_VALUE (2*96)
#define DB_BKING_VALUE 0

#define DB_TABLE_SIZE (2*(2*96+1))

#endif

const int ENDGAME_HASHING_CONSTANTS[13] =
  {0,
   DB_WPAWN_VALUE, DB_WKNIGHT_VALUE, DB_WBISHOP_VALUE,
   DB_WROOK_VALUE, DB_WQUEEN_VALUE, DB_WKING_VALUE,
   DB_BPAWN_VALUE, DB_BKNIGHT_VALUE, DB_BBISHOP_VALUE,
   DB_BROOK_VALUE, DB_BQUEEN_VALUE, DB_BKING_VALUE};

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



// ##################################

OBDD *obdds[DB_TABLE_SIZE];
bool pawnless_endgames[DB_TABLE_SIZE];

// ##################################

char *_allocate_pointer;

class CompressedList {
public:
  void* operator new(size_t size) { return (void *)_allocate_pointer; }
  void operator delete(void *p, size_t size) {}

  CompressedList() {}

  void init(int fd) {
    read(fd, &bits_per_value, sizeof(int));
    read(fd, &_size, sizeof(int));
    int alloc_size = 4*((31 + _size*bits_per_value)>>5);
    read(fd, mem(), alloc_size);

    _allocate_pointer += 2*4 + alloc_size;
  }

  uint operator[](int index) {
    int offset = index*bits_per_value;
    int tmp = offset&31;
    uint result = mem()[offset>>5] >> tmp;
    if (tmp+bits_per_value > 32) {
      return ((mem()[(offset>>5)+1]<<(32-tmp)) | result) & ((1<<bits_per_value)-1);
    } else {
      return result & ((1<<bits_per_value)-1);
    }
  }

  template<class OSTREAM>
  void print(OSTREAM &os);

  int memory_consumption();

  uint size() { return _size; }
private:
  int bits_per_value;
  uint _size;
  uint *mem() { return &_size + 1; }
  // Private to prevent copying:
  CompressedList(const CompressedList&);
  CompressedList& operator=(const CompressedList&);
};


class BinaryDecisionDiagram {
public:
  void* operator new(size_t size) { return (void *)_allocate_pointer; }
  void operator delete(void *p, size_t size) {}

  void init(int fd) {
    read(fd, &log_size, 4);
    read(fd, index_split(), 4*log_size);
    _allocate_pointer += 4 + 4*log_size + 4*log_size;
    for (int i=0; i<log_size; i++) {
      nodes()[i] = new CompressedList();
      nodes()[i]->init(fd);
    }
  }

  uint operator[](int index) {
    uint i=0;
    CompressedList *n = nodes()[log_size];
    for (int layer=log_size-1; layer>=0; layer--) {
      --n;
      int next_bit = (index>>layer)&1;
      if (i >= index_split()[layer]) {
	i = n[layer][index_split()[layer] + i] + next_bit;
      } else {
	i = n[layer][(i<<1) | next_bit];
      }
    }
    return i;
  }
protected:
  int log_size;

  uint *index_split() { return (uint *)(&log_size + 1); }
  CompressedList **nodes() { return (CompressedList **)(&log_size + 1 + log_size); }

  // Private to prevent copying:
  BinaryDecisionDiagram(const BinaryDecisionDiagram&);
  BinaryDecisionDiagram& operator=(const BinaryDecisionDiagram&);
};


class OBDD {
public:
  void* operator new(size_t size) { return (void *)_allocate_pointer; }
  void operator delete(void *p, size_t size) { delete p; }

  OBDD() {}

  void init(int fd, char *p) {
    { // Load convert_table
      uint size;
      read(fd, &size, sizeof(uint));
      convert_table = vector<char>(size);
      read(fd, &(convert_table[0]), sizeof(char)*convert_table.size());
    }

    { // Load clustering information
      read(fd, &clustering_kind, sizeof(int));

      switch(clustering_kind) {
      case 0:
	break;
      case 1:
	clustering.base_subsets = (ushort*)malloc(sizeof(ushort)*5*64);
	read(fd, clustering.base_subsets, sizeof(ushort)*5*64);
	break;
      case 2:
	read(fd, &(clustering.cf_index), sizeof(int));
	break;
      default:
	assert(0);
      }
      
      if (clustering_kind) map_subset_to_cluster.load(fd);
      else map_subset_to_cluster.init(1,0);
    }

    read(fd, pattern, sizeof(uint)*5);
    
    read(fd, bit_perm_and_permute_pos, sizeof(uint)*64);
    
    { // Load sub_bdds
      uint size;
      read(fd, &size, sizeof(size));
      sub_bdds = vector<BinaryDecisionDiagram *>(size);
      for (uint i=0; i<sub_bdds.size(); i++) {
	sub_bdds[i] = new BinaryDecisionDiagram();
	sub_bdds[i]->load(fd);
      }
    }
  }

  char operator[](OBDD_Index bdd_index) {
    int index = ((pattern[0]  &  bit_perm_and_permute_pos[bdd_index[0]])
		 | (pattern[1]  &  bit_perm_and_permute_pos[bdd_index[1]])
		 | (pattern[2]  &  bit_perm_and_permute_pos[bdd_index[2]])
		 | (pattern[3]  &  bit_perm_and_permute_pos[bdd_index[3]])
#ifdef ALLOW_5_MEN_ENDGAME
		 | (pattern[4]  &  bit_perm_and_permute_pos[bdd_index[4]]))
#endif
      ;
    return convert_table[obdd[index]];
  }

protected:
  // convert_table converts from the values in [0..ct_size[ contained
  // by the bdd's to the actual distance to mate etc. values
  vector<char> convert_table;

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

  BinaryDecisionDiagram obdd;
};


inline Piece char_to_piece(char ch) {
  switch(ch) {
    // case ' ': return 0; break;
  case 'P': return WPAWN; break;
  case 'N': return WKNIGHT; break;
  case 'B': return WBISHOP; break;
  case 'R': return WROOK; break;
  case 'Q': return WQUEEN; break;
  case 'K': return WKING; break;
  case 'p': return BPAWN; break;
  case 'n': return BKNIGHT; break;
  case 'b': return BBISHOP; break;
  case 'r': return BROOK; break;
  case 'q': return BQUEEN; break;
  case 'k': return BKING; break;
  }
  return 0;
}

// Returns -1 if illegal filename
int filename_to_hash_value(string filename) {
  int i=filename.size()-1;
  while (i>=0  &&  filename[i] != '/') i--;
  i++;
  i++;// Skip the first king

  int result = 0;
  int color = 0;
  while (i<filename.size()  &&  filename[i]!='_') {
    switch (filename[i++]) {
    case 'P': result += ENDGAME_HASHING_CONSTANTS[PAWN + color]; break;
    case 'N': result += ENDGAME_HASHING_CONSTANTS[KNIGHT + color]; break;
    case 'B': result += ENDGAME_HASHING_CONSTANTS[BISHOP + color]; break;
    case 'R': result += ENDGAME_HASHING_CONSTANTS[ROOK + color]; break;
    case 'Q': result += ENDGAME_HASHING_CONSTANTS[QUEEN + color]; break;
    case 'K': color = 6; break; // The remaining pieces are black
    }
  }

  result += (++i<filename.size()  &&  filename[i]=='w');
  return result;
}

bool load_endgame(string filename) {
  int hash_value = filename_to_hash_value(filename);
  if (hash_value >= DB_TABLE_SIZE) return false;// illegal filename
  if (obdds[hash_value]) return true;

  int fd = open(filename.c_str(), O_RDONLY, 0);
  if (fd == -1) return false;

  // Allocate the same size as the file
  p = (char *)malloc(lseek(fd, 0, 2));
  lseek(fd, 0, 0);

  obdds[hash_value] = new OBDD();
  obdds[hash_value]->init(fd);
  close(fd);
}

void clear_endgame(string filename) {
  int hash_value = filename_to_hash_value(filename);
  if (hash_value >= DB_TABLE_SIZE) return;
  if (obdds[hash_value]) {
    delete obdds[hash_value];
    obdds[hash_value] = 0;
  }
}

void clear_all_endgames() {
  for (int i=0; i<DB_TABLE_SIZE; i++)
    if (obdds[i]) {
      delete obdds[i];
      obdds[i] = 0;
    }
}

// ##########################################################
// #############          BDD Index         #################
// ##########################################################


struct BDD_Index {
  BDD_Index() {
    u.raw[0] = 0;
#if MAX_MEN >= 5
    u.raw[1] = 0;
#endif
  }

  Position &operator[](int index) { return u.positions[index];}

  int index() {
    return u.positions[0] | (u.positions[1] << 6) |
      (u.positions[2] << 12) | (u.positions[3] << 18)
#if MAX_MEN >= 5
      | (u.positions[4] << 24)
#endif
      ;
  }

  BDD_Index &operator==(const BDD_Index &bddi) {
    u.raw[0] = bddi.u.raw[0];
#if MAX_MEN >= 5
    u.raw[1] = bddi.u.raw[1];
#endif
    return *this;
  }
  
private:
  union {
    Position positions[MAX_MEN];
    uint raw[(MAX_MEN+3)/4];
  } u;
};

// ##########################################################
// #######          BDD Index Reflection      ###############
// ##########################################################

struct BDDIndexRefl {
  BDDIndexRefl() : valid(false) {}

  BDDIndexRefl(Position white_king, Position black_king, uchar refl) :
    remapped_bound_k(REMAP_BOUND_KING[black_king]),
    free_k(white_king), refl(refl), valid(true) {}
  BDDIndexRefl(pair<Position,Position> wb_king, uchar refl) :
    remapped_bound_k(REMAP_BOUND_KING[wb_king.second]),
    free_k(wb_king.first), refl(refl), valid(true) {}

  Position remapped_bound_k, free_k;
  uchar refl;
};

extern BDDIndexRefl BDD_KING_FULL_SYMMETRY[64*64];
inline BDDIndexRefl &bdd_king_full_symmetry(uchar white_king, uchar black_king) {
  return BDD_KING_FULL_SYMMETRY[(white_king << 6) | black_king];
}

extern BDDIndexRefl BDD_KING_PAWN_SYMMETRY[64*64];
inline BDDIndexRefl &bdd_king_pawn_symmetry(uchar white_king, uchar black_king) {
  return BDD_KING_PAWN_SYMMETRY[(white_king << 6) | black_king];
}

// Initialize stuff


Position REFLECTION_TABLE[8*64] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24,39,38,37,36,35,34,33,32,47,46,45,44,43,42,41,40,55,54,53,52,51,50,49,48,63,62,61,60,59,58,57,56,56,57,58,59,60,61,62,63,48,49,50,51,52,53,54,55,40,41,42,43,44,45,46,47,32,33,34,35,36,37,38,39,24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,8,16,24,32,40,48,56,1,9,17,25,33,41,49,57,2,10,18,26,34,42,50,58,3,11,19,27,35,43,51,59,4,12,20,28,36,44,52,60,5,13,21,29,37,45,53,61,6,14,22,30,38,46,54,62,7,15,23,31,39,47,55,63,56,48,40,32,24,16,8,0,57,49,41,33,25,17,9,1,58,50,42,34,26,18,10,2,59,51,43,35,27,19,11,3,60,52,44,36,28,20,12,4,61,53,45,37,29,21,13,5,62,54,46,38,30,22,14,6,63,55,47,39,31,23,15,7,7,15,23,31,39,47,55,63,6,14,22,30,38,46,54,62,5,13,21,29,37,45,53,61,4,12,20,28,36,44,52,60,3,11,19,27,35,43,51,59,2,10,18,26,34,42,50,58,1,9,17,25,33,41,49,57,0,8,16,24,32,40,48,56,63,55,47,39,31,23,15,7,62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,60,52,44,36,28,20,12,4,59,51,43,35,27,19,11,3,58,50,42,34,26,18,10,2,57,49,41,33,25,17,9,1,56,48,40,32,24,16,8,0};

inline int reflect(int pos, int refl) {
  return REFLECTION_TABLE[(refl << 6) | pos];
}





char index_endgame(vector<PiecePos>& piece_list, bool black_to_move) {
  // Sort piece_list in KQRBNPkqrbnp order
  //
  // Decide if the players should be swapped
  //   - If so, then also mirror the board
  //
  //
  int hash_value = ENDGAME_HASHING_CONSTANTS[piece_list[0].piece] +
    ENDGAME_HASHING_CONSTANTS[piece_list[1].piece];
  for (int i=2; i<piece_list.size(); i++)
    hash_value += ENDGAME_HASHING_CONSTANTS[piece_list[i].piece];
  if (hash_value+1>=DB_TABLE_SIZE  || // If too many pieces or illegal input
      !(tables[hash_value]  ||  tables[hash_value | 1]))
    return ENDGAME_TABLE_UNKNOWN;

  switch (piece_list.size()) {
  case 2:
    return ENDGAME_TABLE_DRAW;
  case 3:
    {
      {// Sort piece_list in KQRBNPkqrbnp order
	if (GT[piece_list[0].piece][piece_list[1].piece]) swap(piece_list[0], piece_list[1]);
	if (GT[piece_list[1].piece][piece_list[2].piece]) swap(piece_list[1], piece_list[2]);
	if (GT[piece_list[0].piece][piece_list[1].piece]) swap(piece_list[0], piece_list[1]);
      }
      
      {// Decide if the players should be swapped
	if (PIECE_KIND[piece_list[1].piece] == KING) {
	  // swap!
	  black_to_move ^= 1;
	
	  PiecePos weak_king = piece_list[0];
	  piece_list[0] = piece_list[1];
	  piece_list[1] = piece_list[2];
	  piece_list[2] = weak_king;

	  piece_list[0].pos ^= (7<<3);
	  piece_list[1].pos ^= (7<<3);
	  piece_list[2].pos ^= (7<<3);
	}
      }
      if (!tables[hash_value |= black_to_move])
	return ENDGAME_TABLE_UNKNOWN;

      BDD_Index result;
      BDDIndexRefl ir;
      if (PIECE_KIND[piece_list[1]] == PAWN) {
	BDDIndexRefl ir = bdd_king_pawn_symmetry(piece_list[0].pos, piece_list[2]pos);
	result[0] = reflect(piece_list[1].pos - 8, ir.refl);
      } else {
	BDDIndexRefl ir = bdd_king_full_symmetry(piece_list[0].pos, piece_list[2]pos);
	result[0] = reflect(piece_list[1].pos, ir.refl);
      }
      result[1] = ir.free_king();
      result[2] = ir.remapped_bound_king();
      
      return (*(tables[hash_value]))[result];
    }

  case 4:
    {// Sort piece_list in KQRBNPkqrbnp order
      // Partial sort piece_list[0..1] and piece_list[2..3]
      if (GT[piece_list[0].piece][piece_list[1].piece]) swap(piece_list[0], piece_list[1]);
      if (GT[piece_list[2].piece][piece_list[3].piece]) swap(piece_list[2], piece_list[3]);
      // Find smallest and largest element
      if (GT[piece_list[0].piece][piece_list[2].piece]) swap(piece_list[0], piece_list[2]);
      if (GT[piece_list[1].piece][piece_list[3].piece]) swap(piece_list[1], piece_list[3]);
      // Sort middle elements
      if (GT[piece_list[1].piece][piece_list[2].piece]) swap(piece_list[1], piece_list[2]);
    }

    {// Decide if the players should be swapped
      if (PIECE_KIND[piece_list[1].piece] == KING) {
	// swap!
	black_to_move ^= 1;

	PiecePos weak_king = piece_list[0];
	piece_list[0] = piece_list[1];
	piece_list[1] = piece_list[2];
	piece_list[2] = piece_list[3];
	piece_list[3] = weak_king;

	piece_list[0].pos ^= (7<<3);
	piece_list[1].pos ^= (7<<3);
	piece_list[2].pos ^= (7<<3);
	piece_list[3].pos ^= (7<<3);

      } else if (PIECE_KIND[piece_list[2].piece] == KING  &&
		 PIECE_KIND[piece_list[1].piece] < PIECE_KIND[piece_list[3].piece]) {
	// swap!
	black_to_move ^= 1;

	swap(piece_list[0], piece_list[2]);
	swap(piece_list[1], piece_list[3]);

	piece_list[0].pos ^= (7<<3);
	piece_list[1].pos ^= (7<<3);
	piece_list[2].pos ^= (7<<3);
	piece_list[3].pos ^= (7<<3);
      }
    }
    if (!tables[hash_value |= black_to_move])
      return ENDGAME_TABLE_UNKNOWN;
    
    // ....
    break;

#ifdef ALLOW_5_MEN_ENDGAME
  case 5:
    {// Sort piece_list in KQRBNPkqrbnp order
      // start by sorting piece_list[0,1,3,4] like sort_4_piece_pos
      if (GT[piece_list[0].piece][piece_list[1].piece]) swap(piece_list[0], piece_list[1]);
      if (GT[piece_list[2].piece][piece_list[3].piece]) swap(piece_list[2], piece_list[3]);
      if (GT[piece_list[0].piece][piece_list[3].piece]) swap(piece_list[0], piece_list[3]);
      if (GT[piece_list[1].piece][piece_list[4].piece]) swap(piece_list[1], piece_list[4]);
      if (GT[piece_list[1].piece][piece_list[3].piece]) swap(piece_list[1], piece_list[3]);
      // find the place for piece_list[2]
      if (GT[piece_list[1].piece][piece_list[2].piece]) {
	swap(piece_list[1], piece_list[2]);
	if (GT[piece_list[0].piece][piece_list[1].piece]) swap(piece_list[0], piece_list[1]);
	
      } else if (GT[piece_list[2].piece][piece_list[3].piece]) {
	swap(piece_list[2], piece_list[3]);
	if (GT[piece_list[3].piece][piece_list[4].piece]) swap(piece_list[3], piece_list[4]);
      }
    }

    {// Decide if the players should be swapped
      if (PIECE_KIND[piece_list[1].piece] == KING) {
	// swap!
	black_to_move ^= 1;

	PiecePos weak_king = piece_list[0];
	piece_list[0] = piece_list[1];
	piece_list[1] = piece_list[2];
	piece_list[2] = piece_list[3];
	piece_list[3] = piece_list[4];
	piece_list[4] = weak_king;

	piece_list[0].pos ^= (7<<3);
	piece_list[1].pos ^= (7<<3);
	piece_list[2].pos ^= (7<<3);
	piece_list[3].pos ^= (7<<3);
	piece_list[4].pos ^= (7<<3);
	
      } else if (PIECE_KIND[piece_list[2].piece] == KING) {
	// swap!
	black_to_move ^= 1;

	PiecePos weak_king_piece = piece_list[1];
	piece_list[1] = piece_list[2];
	piece_list[2] = piece_list[3];
	piece_list[3] = piece_list[4];
	piece_list[4] = weak_king_piece;

	piece_list[0].pos ^= (7<<3);
	piece_list[1].pos ^= (7<<3);
	piece_list[2].pos ^= (7<<3);
	piece_list[3].pos ^= (7<<3);
	piece_list[4].pos ^= (7<<3);
      }
    }
    if (!tables[hash_value |= black_to_move])
      return ENDGAME_TABLE_UNKNOWN;


    // ...
    break;

#endif
  default:
    return ENDGAME_TABLE_UNKNOWN;
  }

  if (!tables[hash_value]) return ENDGAME_TABLE_UNKNOWN;

}




void init_endgames() {
  for (int i=0; i<DB_TABLE_SIZE; i++)
    obdds[i] = 0;

  
}





// EXAMPLE:

int main() {
  init_endgames();
  
  if (!load_endgame("KRK_btm.bdd")) {
    cerr << "Download the file KRK_btm.bdd to the same folder as this program.\n";
    exit(1);
  }

  {
    cerr << "    a b c d e f g h\n"
	 << "  +-----------------+    |\n"
	 << "8 |                 | 8  | 60 black to move\n"
	 << "7 |                 | 7  |\n"
	 << "6 |                 | 6  | White has lost castling\n"
	 << "5 |       R         | 5  | Black has lost castling\n"
	 << "4 | k     K         | 4  |\n"
	 << "3 |                 | 3  | moves played since progress = 0\n"
	 << "2 |                 | 2  |\n"
	 << "1 |                 | 1  |\n"
	 << "  +-----------------+    |\n"
	 << "    a b c d e f g h\n"
	 << "\n"
	 << "FEN description: 8/8/8/3R4/k2K4/8/8/8 b - - 0 60\n"
	 << "\n";
    
    vector<PiecePos> pp(3);
    pp[0] = PiecePos(6, (4-1)*8+(4-1)); // white king, (rank 4, file d)
    pp[1] = PiecePos(4, (5-1)*8+(4-1)); // white rook, (rank 5, file d)
    pp[2] = PiecePos(12, (4-1)*8+(1-1)); // white king, (rank 4, file a)
    
    cerr << "Game theoretical value: "
	 << endgame_value_to_string(index_endgame(pp, true)) << "\n";
  }
  
  return 0;
}
