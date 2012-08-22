// Moved here from endgame_square_permutations.cxx

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
