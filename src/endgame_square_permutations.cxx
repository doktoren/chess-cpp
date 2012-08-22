#include "endgame_square_permutations.hxx"

//#include "algorithm_best_permutation.hxx"
//#include "burrows_wheeler.hxx"

int REMAP_BOUND_KING[64] =
{ 0, 1, 2, 3,-1,-1,-1,-1,
  4, 5, 6, 7,-1,-1,-1,-1,
  8, 9,10,11,-1,-1,-1,-1,
 12,13,14,15,-1,-1,-1,-1,
 16,17,18,19,-1,-1,-1,-1,
 20,21,22,23,-1,-1,-1,-1,
 24,25,26,27,-1,-1,-1,-1,
 28,29,30,31,-1,-1,-1,-1 };
int INV_REMAP_BOUND_KING[64] =
{ 0, 1, 2, 3, 8, 9,10,11,
 16,17,18,19,24,25,26,27,
 32,33,34,35,40,41,42,43,
 48,49,50,51,56,57,58,59,
 -1,-1,-1,-1,-1,-1,-1,-1,
 -1,-1,-1,-1,-1,-1,-1,-1,
 -1,-1,-1,-1,-1,-1,-1,-1,
 -1,-1,-1,-1,-1,-1,-1,-1 };


string mapping_name(int mapping) {
  switch(mapping) {
  case -5: return "Mapping found by Borrows Wheeler algorithm";
  case -4: return "Alg1c";
  case -3: return "Alg1b";
  case -2: return "Alg1";
  case -1: return "Mapping found by genetic algorithm";
  case 0: return "Identity mapping";
  case 1: return "Mapping designed for pawns";
  case 2: return "Mapping designed for bishops";
  case 3: return "Hilbert curve mapping";
  case 4: return "Z curve mapping";
  case 5: return "Mapping found by endgame_KBBK.cxx";
  case 6: return "Pawn mapping found by GA";
  case 7: return "Whirl mapping";
  case 8: return "Modified Z curve";
  }
  return "illegal mapping";
}


// IMPORTANT: The bound king MUST be mapped to a number representable
// with 4 resp. 5 bits (no pawns or pawns).
// Hence only the following mappings may be applied to the bound king:
//     3 (Hilber curve), 4 (Z curve), 8 (Modified Z curve)
uchar mappings[NUM_STD_MAPPINGS][64] = {
  {// 0: Identity mapping
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9,10,11,12,13,14,15,
   16,17,18,19,20,21,22,23,
   24,25,26,27,28,29,30,31,
   32,33,34,35,36,37,38,39,
   40,41,42,43,44,45,46,47,
   48,49,50,51,52,53,54,55,
   56,57,58,59,60,61,62,63},
  {// 1: Pawn mapping
    2, 0, 3, 1, 9,11, 8,10,
    6, 4, 7, 5,13,15,12,14,
   18,16,19,17,25,27,24,26,
   22,20,23,21,29,31,28,30,
   34,32,35,33,41,43,40,42,
   38,36,39,37,45,47,44,46,
   48,49,50,51,52,53,54,55,
   56,57,58,59,60,61,62,63},
  {// 2: Bishop mapping
    0,47, 3,40, 8,35,15,32,
   54, 2,46, 7,39,14,34,22,
    1,53, 6,45,13,38,21,33,
   59, 5,52,12,44,20,37,27,
    4,58,11,51,19,43,26,36,
   62,10,57,18,50,25,42,30,
    9,61,17,56,24,49,29,41,
   63,16,60,23,55,28,48,31},
  {// 3: Hilbert curve
    0, 3, 4, 5,58,59,60,63,
    1, 2, 7, 6,57,56,61,62,
   14,13, 8, 9,54,55,50,49,
   15,12,11,10,53,52,51,48,
   16,17,30,31,32,33,46,47,
   19,18,29,28,35,34,45,44,
   20,23,24,27,36,39,40,43,
   21,22,25,26,37,38,41,42},
  {// 4: Z curve
    0, 2, 8,10,32,34,40,42,
    1, 3, 9,11,33,35,41,43,
    4, 6,12,14,36,38,44,46,
    5, 7,13,15,37,39,45,47,
   16,18,24,26,48,50,56,58,
   17,19,25,27,49,51,57,59,
   20,22,28,30,52,54,60,62,
   21,23,29,31,53,55,61,63},
    /* Previously version did not satisfy bound king constraint
    0, 1, 4, 5,16,17,20,21,
    2, 3, 6, 7,18,19,22,23,
    8, 9,12,13,24,25,28,29,
   10,11,14,15,26,27,30,31,
   32,33,36,37,40,41,44,45,
   34,35,38,39,42,43,46,47,
   48,49,52,53,56,57,60,61,
   50,51,54,55,58,59,62,63},
    */
  {// 5: Bishop mapping 2
   53,15,35, 1,33, 3,47,21,
   28,52,16,34, 2,48,20,60,
   42,27,51,17,49,19,59,10,
    6,43,26,50,18,58,11,38,
   32, 5,44,25,57,12,37, 0,
    4,45,24,63,31,56,13,36,
   46,23,62, 8,40,30,55,14,
   22,61, 9,39, 7,41,29,54},
  {// 6: Pawn mapping 2. Found by GA
   18,16,22,20,36,39,38,32,
   19,17,23,21,37,35,34,33,
   26,27,30,31,47,46,42,43,
   24,25,28,29,45,44,40,41,
   10, 8,11, 9, 1, 3, 0, 2,
   14,12,15,13, 5, 7, 4, 6,
   54,59,58,52,50,60,63,61,
   55,56,62,53,51,48,49,57},
  {// 7: Whirl mapping
    0, 1, 2, 3,52,36,20, 4,
   16,17,18,19,53,37,21, 5,
   32,33,34,35,54,38,22, 6,
   48,49,50,51,55,39,23, 7,
   15,31,47,63,59,58,57,56,
   14,30,46,62,43,42,41,40,
   13,29,45,61,27,26,25,24,
   12,28,44,60,11,10, 9, 8},
  {// 8: Modified Z curve
    0, 2, 8,10,42,40,34,32,
    1, 3, 9,11,43,41,35,33,
    4, 6,12,14,46,44,38,36,
    5, 7,13,15,47,45,39,37,
   21,23,29,31,63,61,55,53,
   20,22,28,30,62,60,54,52,
   17,19,25,27,59,57,51,49,
   16,18,24,26,58,56,50,48},
  {// 9: King "identity mapping"
    0, 1, 2, 3,32,33,34,35,
    4, 5, 6, 7,36,37,38,39,
    8, 9,10,11,40,41,42,43,
   12,13,14,15,44,45,46,47,
   16,17,18,19,48,49,50,51,
   20,21,22,23,52,53,54,55,
   24,25,26,27,56,57,58,59,
   28,29,30,31,60,61,62,63}
  /*
-rw-r--r--   1 doktoren users     1486485 Mar 17 16:50 KRNK.bdd
  {// Knight mapping (chosen from http://web.usna.navy.mil/~wdj/knight_tour.htm)
   57,18,21, 4,39, 8,23, 6,
   20, 3,58,55,22, 5,40, 9,
   17,56,19,38,59,62, 7,24,
    2,37,52,61,54,41,10,63,
   49,16,47,36,51,60,25,34,
   30, 1,50,53,46,35,42,11,
   15,48,31,28,13,44,33,26,
    0,29,14,45,32,27,12,43}
  */
};



// OLD CODE FOR COMPUTING PERMUTATIONS.
// NO LONGER USED.


/*

#define NUM_ALG_MAPPINGS 5


struct Uchar_n {
  Uchar_n(uchar e) : data(1), all_zero(false) { data[0] = e; }
  Uchar_n(const Uchar_n &u1, const Uchar_n &u2) :
    data(u1.data.size() + u2.data.size())
  {
    if (u1.all_zero  ||  u2.all_zero) {
      make_zero();

    } else {

      all_zero = false;
      assert(u1.data.size() == u2.data.size());
      
      for (uint i=0; i<u1.data.size(); i++) {
	data[i] = u1.data[i];
	data[i+u1.data.size()] = u2.data[i];
      }
    }
  }
  
  void make_zero() {
    all_zero = true;
    data.clear();
  }

  bool operator<(const Uchar_n b) const {
    if (data.size() != b.data.size()) return data.size() < b.data.size();

    for (uint i=0; i<data.size(); i++)
      if (data[i] != b.data[i])
	return data[i] < b.data[i];

    return false;
  }
  
  vector<uchar> data;
  bool all_zero;
};

inline Uchar_n gahgah(const vector<vector<pair<short, short> > > &a, const uchar *mem,
		      int level, int i1, int i2) {
  if (level==6) return Uchar_n(mem[i1], mem[i2]);
  return Uchar_n(gahgah(a, mem, level+1, a[level][i1].first, a[level][i1].second),
		 gahgah(a, mem, level+1, a[level][i2].first, a[level][i2].second));
}

void genetic_algorithm_mapping(uchar *mapping, const uchar *_table, int size) {
  for (int i=0; i<64; i++)
    mapping[i] = i;
  
  vector<Solution> solutions(6);
  for (int i=0; i<6; i++) {
    solutions[i] = vector<pair<short, short> >(1<<i);
    for (int j=0; j<(1<<i); j++)
      solutions[i][j] = pair<short, short>(2*j, 2*j+1);
  }
  
  for (int level=5; level>=0; level--) {
    {// Print solution
      Uchar_n tmp = gahgah(solutions, mapping, 1, 0, 1);
      print_signed_map64(cerr, &(tmp.data[0]), 2, 10);
    }
    
    cerr << "Hertil ok 1 " << level << "\n";
    int num_nodes = 2<<level;
    vector<vector<GraphEntry> > graph(num_nodes);
    for (int i=0; i<num_nodes; i++) graph[i] = vector<GraphEntry>(num_nodes);
    
    cerr << "Hertil ok 2 " << level << "\n";
    // count occurences of edges
    map<Uchar_n, int> mapping;
    for (int i=0; i<size; i+=64)
      for (int i1=0; i1<num_nodes; i1++)
	for (int i2=0; i2<num_nodes; i2++) {
	  Uchar_n tmp = gahgah(solutions, &(_table[i]), level+1, i1, i2);
	  if (!tmp.all_zero) ++mapping[tmp];
	}
    
    cerr << "Hertil ok 3 " << level << "\n";
    priority_queue<pair<int, Uchar_n> > pq;
    typedef map<Uchar_n, int>::const_iterator CI;
    for (CI i=mapping.begin(); i!=mapping.end(); i++)
      pq.push(pair<int, Uchar_n>(i->second, i->first));
    
    cerr << "Hertil ok 4 " << level << "\n";
    int index = 0;
    const int max_diff_values = 10000;
    while (!pq.empty()) {
      if (index<max_diff_values) {
	mapping[pq.top().second] = ++index;
      } else {
	mapping[pq.top().second] = 0;
      }
      pq.pop();
    }
    
    cerr << "Hertil ok 5 " << level << "\n";
    for (int i=0; i<size; i+=64)
      for (int i1=0; i1<num_nodes; i1++)
	for (int i2=0; i2<num_nodes; i2++) {
	  Uchar_n tmp = gahgah(solutions, &(_table[i]), level+1, i1, i2);
	  ++graph[i1][i2][mapping[tmp]];
	}
    
    cerr << "Hertil ok 6 " << level << "\n";
    
    switch(level) {
    case 5:
      solutions[level] = Algorithm_Best_Perm_4(graph);
      break;
    case 4:
      solutions[level] = Algorithm_Best_Perm_4(graph);
      break;
    case 3:
      solutions[level] = Algorithm_Best_Perm_3(graph);
      break;
    case 2:
      solutions[level] = Algorithm_Best_Perm_2(graph, 64, 30);
      break;
    case 1:
      solutions[level] = Algorithm_Best_Perm_2(graph, 8, 4);
      break;
    case 0:
      solutions[level] = Algorithm_Best_Perm_2(graph, 8, 2);
      break;
    }
  }
  
  {// init mapping
    for (int i=0; i<64; i++) mapping[i] = i;
    Uchar_n tmp = gahgah(solutions, mapping, 1, 0, 1);
    
    for (int i=0; i<64; i++)
      mapping[tmp.data[i]] = i;
  }
  
  print_signed_map64(cerr, mapping, 2, 10);
}

void alg1_mapping(uchar *mapping, const uchar *_table, int size) {
  for (int i=0; i<64; i++)
    mapping[i] = i;
  
  vector<vector<int> > graph(64);
  for (int i=0; i<64; i++) {
    graph[i] = vector<int>(i);
    for (int j=0; j<i; j++) graph[i][j] = 0;
  }

  for (int i=0; i<size; i+=64)
    for (int i1=0; i1<64; i1++)
      for (int i2=0; i2<i1; i2++)
	if (_table[i+i1] == _table[i+i2])
	  ++graph[i1][i2];

  for (int i1=0; i1<64; i1++) {
    priority_queue<pair<int,int> > pq;
    for (int i2=0; i2<i1; i2++)
      pq.push(pair<int,int>(graph[i1][i2],i2));
    cerr << (POS_COLOR[i1] ? '+' : '-') << POS_NAME[i1] << " :";
    while (!pq.empty()) {
      cerr << ' ' << (POS_COLOR[pq.top().second] ? '+' : '-') << POS_NAME[pq.top().second];
      pq.pop();
    }
    cerr << '\n';
  }

  vector<int> perm = Algorithm_Best_Perm(graph);
  print_signed_map64(cerr, &(perm[0]), 2, 10);
  inverse_permutation(perm);
  for (int i=0; i<64; i++)
    mapping[i] = perm[i];

}

void alg1b_mapping(uchar *mapping, const uchar *_table, int size) {
  for (int i=0; i<64; i++)
    mapping[i] = i;

  vector<vector<int> > graph(64);
  for (int i=0; i<64; i++) {
    graph[i] = vector<int>(64);
    for (int j=0; j<64; j++) graph[i][j] = 0;
  }

  for (int i=0; i<size; i+=64)
    for (int i1=0; i1<64; i1++)
      for (int i2=0; i2<64; i2++)//64, ej i1
	if (i1!=i2  &&  _table[i+i1] == _table[i+i2])
	  ++graph[i1][i2];

  vector<int> perm = Algorithm_Best_Perm_1b(graph);
  print_signed_map64(cerr, &(perm[0]), 2, 10);
  inverse_permutation(perm);
  for (int i=0; i<64; i++)
    mapping[i] = perm[i];
}

struct NList {
  NList() : l(0) {}
  NList(int size) : size(size) { l = new uchar[size]; }
  ~NList() { if (l) delete l; }

  bool operator<(const NList &nl) const {
    for (int i=0; i<size; i++)
      if (l[i] != nl.l[i])
	return l[i]<nl.l[i];
    return false;
  }

  int size;
  uchar *l;
};

void alg1c_mapping(uchar *mapping, const uchar *_table, int size) {

  vector<int> _mapping(64);
  for (int i=0; i<64; i++) _mapping[i] = i;

  for (int level=5; level>=0; --level) {
    int graph_size = 2<<level;
    int block_size = 32>>level;

    vector<vector<int> > graph(graph_size);
    for (int i=0; i<graph_size; i++)
      graph[i] = vector<int>(graph_size);

    for (int i=0; i<size; i+=64)
      for (int i1=0; i1<graph_size; i1++)
	for (int i2=0; i2<graph_size; i2++)
	  if (i1!=i2) {
	    bool equal = true;
	    for (int j=0; j<block_size; j++)
	      if (_table[i+_mapping[i1*block_size+j]] != _table[i+_mapping[i2*block_size+j]]) {
		equal = false;
		break;
	      }
	    if (equal) ++graph[i1][i2];
	  }
    Solution perm = Algorithm_Best_Perm_1c(graph);

    uchar tmp[64];
    for (uint i=0; i<perm.size(); i++) {
      for (int j=0; j<block_size; j++) {
	tmp[2*block_size*i + j] = _mapping[block_size*perm[i].first + j];
	tmp[2*block_size*i + block_size + j] = _mapping[block_size*perm[i].second + j];
      }
    }
    for (int i=0; i<64; i++) _mapping[i] = tmp[i];
  }

  print_signed_map64(cerr, &(_mapping[0]), 2, 10);
  inverse_permutation(_mapping);
  for (int i=0; i<64; i++)
    mapping[i] = _mapping[i];
}

// Binary bidirectional
void BW_Best_Perm(uchar *mapping, const uchar *_table, int size) {
  vector<vector<int> > graph(64);
  for (int i=0; i<64; i++) {
    graph[i] = vector<int>(64);
    for (int j=0; j<64; j++) graph[i][j] = 0;
  }

  vector<int> bw(64);
  for (int i=0; i<size; i+=64) {
    for (int j=0; j<64; j++)
      bw[j] = _table[i+j];
    int special_pos;
    vector<int> t = bw_transform(bw, special_pos);
    for (int i=0; i<64; i++)
      if (i!=special_pos  &&  i+1!=special_pos)
	++graph[t[i]][t[i+1]];
  }

  vector<int> perm = Algorithm_Best_Perm_1b(graph);
  print_signed_map64(cerr, &(perm[0]), 2, 10);
  inverse_permutation(perm);
  for (int i=0; i<64; i++)
    mapping[i] = perm[i];
}
*/


/*
  Possible solution to 2 same piece compression
  
  A B C D E F G H
  A 
  B 0
  C 0 1
  D 0 1 2
  E 0 1 2 3
  F 4 1 2 3 4
  G 5 5 2 3 4 5
  H 6 6 6 3 4 5 6
  
  A B C D E F G H
  A 
  B 0
  C 0 1
  D 0 1 2
  E 0 1 2 3
  F 6 1 2 3 4
  G 6 5 2 3 4 5
  H 6 5 4 3 4 5 6
  
  A B C D E F G H
  A 
  B 6
  C 6 5
  D 6 5 4
  E 0 1 2 3
  F 0 1 2 3 4
  G 0 1 2 3 4 5
  H 0 1 3 3 4 5 6
*/



