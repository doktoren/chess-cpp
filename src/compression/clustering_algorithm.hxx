#ifndef _CLUSTERING_ALGORITHM_
#define _CLUSTERING_ALGORITHM_

using namespace std;

#include <vector>

#include "../typedefs.hxx"

// elements[0..n-1][0..N-1], each element is a vector of N values
// and represents a point in R^N.
// Each element is assigned an weight >= 0
// Result is a mapping from elements to {0, ..., num_clusters-1}
// ie. the result tells to which group each vector<uint> belong to.
//
// Initial cluster centers are chosen randomly from elements.
// An elements is chosen as a center with probality proportional with its weight.
//
// The fitness of the result is dependent on the initial clusters
// (which the algorithm pick arbitrary). Hence its a good idea to
// run the algorithm more than one (<=> set NUM_GENERATIONS > 1)
// Each element is assigned a weight>=0
template <class TYPE>
vector<int> find_clusters(const vector<vector<TYPE> > &elements,
			  const vector<double> &weights, int num_clusters,
			  int NUM_GENERATIONS = 16, int MAX_ITERATIONS = 64);
// With TYPE==uint
//template vector<int> find_clusters(const vector<vector<uint> > &,
//				   const vector<double> &, int, int, int);



// A point p is given weight p1+p2+...+pn.
// Each point is scaled to a point on the N-dimensional hypersphere with radius 1
// (or origo if weight 0). Then the function find_clusters is called.
vector<int> find_clusters2(const vector<vector<uint> > &_elements, int num_clusters,
			   int NUM_GENERATIONS = 16, int MAX_ITERATIONS = 64);


void test_find_clusters();

#endif
