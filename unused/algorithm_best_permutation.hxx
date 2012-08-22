#include <vector>
#include <map>

#include "typedefs.hxx"

using namespace std;

struct ABP_Edge {
  ABP_Edge() : from(-1), to(-1), value(0) {}
  ABP_Edge(int from, int to) :
    from(from), to(to), value(0) {}
  ABP_Edge(int from, int to, int value) :
    from(from), to(to), value(value) {}
  int from, to;
  int value;
};

struct ABP_Vertex {
  ABP_Vertex() : prev(-1), next(-1), id(-1) {}
  ABP_Vertex(int id) : prev(-1), next(-1), id(id) {}

  bool is_end_point() { return prev==-1 || next==-1; }
  void connect(int index) {
    if (prev==-1) {
      prev = index;
    } else {
      next = index;
    }
  }

  int prev, next;
  int id;
};


typedef int MappedType;
typedef map<MappedType, int> GraphEntry;
// Graph.size() == Graph[].size(), x->y in generel different from y->x
typedef vector<vector<GraphEntry> > Graph;
typedef vector<pair<short, short> > Solution;


// If the result eg. is [2,1,0,4,6,5]
// then ...
//
//
//    vector<vector<int> > graph(64);
//    for (int i=0; i<64; i++) graph[i] = vector<int>(i);
// 
vector<int> Algorithm_Best_Perm(const vector<vector<int> > &graph);

vector<int> Algorithm_Best_Perm_1b(const vector<vector<int> > &graph);
Solution Algorithm_Best_Perm_1c(const vector<vector<int> > &graph);

template <class TYPE>
void inverse_permutation(vector<TYPE> &perm) {
  vector<TYPE> inv_perm(perm.size());
  for (unsigned int i=0; i<perm.size(); i++)
    inv_perm[perm[i]] = i;
  for (unsigned int i=0; i<perm.size(); i++)
    perm[i] = inv_perm[i];
}

template <class TYPE>
void verify_permutation(const vector<TYPE> &perm) {
  vector<bool> used(perm.size());
  for (uint i=0; i<perm.size(); i++) {
    assert(0<=perm[i]  &&  (uint)perm[i]<perm.size());
    assert(!used[perm[i]]);
    used[perm[i]] = true;
  }
}

Solution Algorithm_Best_Perm_2(const Graph &graph, int population_size, int num_iterations);

Solution Algorithm_Best_Perm_3(const Graph &graph);

Solution Algorithm_Best_Perm_4(const Graph &graph, int depth, string s);
inline Solution Algorithm_Best_Perm_4(const Graph &graph) {
  return Algorithm_Best_Perm_4(graph, 0, "");
}
