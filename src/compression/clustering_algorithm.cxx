#include <assert.h>
#include <iostream>

#include "clustering_algorithm.hxx"

#include "../streams.hxx"
#include "../util/help_functions.hxx"

//#define PRINT

// calc_fit calculates the fitness of element and center as
// the square of their distance in R^n
template <class TYPE>
double calc_fit(const vector<TYPE> &element, const vector<double> &center) {
  assert(element.size() == center.size());
  double result = 0.0;
  for (uint i=0; i<element.size(); i++) {
    double tmp = element[i] - center[i];
    result += tmp*tmp;
  }
  return result;
}

// elements[0..n-1][0..N-1], each element is a vector of N values
// and represents a point in R^N.
// Each element is assigned an weight >= 0
// Result is a mapping from elements to {0, ..., num_clusters-1}
// ie. the result tells to which group each vector<uint> belong to.
//
// Initial cluster centers are chosen randomly from elements.
// An elements is chosen as a center with probability proportional with its weight.
template <class TYPE>
vector<int> find_clusters(const vector<vector<TYPE> > &elements,
    const vector<double> &weights,
    int num_clusters, int NUM_GENERATIONS, int MAX_ITERATIONS) {
  int N = elements[0].size();

  if ((int)elements.size() <= num_clusters) {
    // Trivial solution
    vector<int> result(elements.size());
    for (uint_fast32_t i=0; i<elements.size(); i++)
      result[i] = i;
    return result;
  }

#ifndef NDEBUG
  assert((int)elements.size() >= num_clusters);
  assert(num_clusters > 0);
  for (uint_fast32_t i=0; i<elements.size(); i++)
    assert((int)elements[i].size() == N);
  assert(NUM_GENERATIONS > 0);
  assert(MAX_ITERATIONS > 0);
#endif

  if (num_clusters == (int)elements.size()) {
    vector<int> result(num_clusters);
    for (int i=0; i<num_clusters; i++)
      result[i] = i;
    return result;
  }

  vector<int> best_result(elements.size());
  double best_generation_fit = 999999999.0;
  best_generation_fit *= best_generation_fit; // should be "infinite" now

#ifdef PRINT
  vector<vector<double> > best_centers(num_clusters);
  for (int i=0; i<num_clusters; i++)
    best_centers[i] = vector<double>(N);
#endif

  double weight_sum = 0.0;
  for (vector<double>::size_type i=0; i<weights.size(); i++)
    weight_sum += weights[i];

  for (int generation=0; generation < NUM_GENERATIONS; ++generation) {

    vector<vector<double> > centers(num_clusters);
    for (int i=0; i<num_clusters; i++)
      centers[i] = vector<double>(N);

    { // Initialize centers
      vector<bool> chosen(elements.size());
      for (int i=0; i<num_clusters; i++) {
        // Pick an element as center with probability proportional with
        // its weight.
        int num_tries = 0;
        int picked;
        do {
          double cut_w = weight_sum*(1.0/((RAND_MAX+1.0)-1))*(rand()+1);
          // 0 < cut_w < weight_sum, but the <'s are close to <=
          double prefix_sum = 0.0;
          picked = -1;
          for (uint i=0; i<weights.size(); i++) {
            prefix_sum += weights[i];
            if (prefix_sum >= cut_w) {
              picked = i;
              break;
            }
          }
          assert(picked != -1);
        } while (chosen[picked]  &&  ++num_tries <= 10);

        if (chosen[picked]) {
          // Failed to pick a new element as center with the probability stuff.
          // Just pick the element with highest weight
          double max_weight = -1;
          for (uint i=0; i<weights.size(); i++) {
            if (!chosen[i]  &&  weights[i] > max_weight) {
              max_weight = weights[i];
              picked = i;
            }
          }
          assert(!chosen[picked]);
        }

        chosen[picked] = true;

        for (int j=0; j<N; j++)
          centers[i][j] = elements[picked][j];
      }
    }
    vector<int> result(elements.size());
    double total_fit = 0.0;

    int iteration = 0;
    bool progress = true;
    while (progress  &&  iteration<MAX_ITERATIONS) {
      ++iteration;
      progress = false;

      total_fit = 0.0;

      {// Init result. Ie. assign each vector to a center
        for (uint i=0; i<elements.size(); i++) {
          double best_fit = 999999999.0;
          best_fit *= best_fit; // should be "infinite" now
          int center_index = -1;

          for (int j=0; j<num_clusters; j++) {
            double fit = calc_fit(elements[i], centers[j]);
            if (fit < best_fit) {
              best_fit = fit;
              center_index = j;
            }
          }

          total_fit += weights[i] * best_fit;

          if (center_index != result[i]) {
            progress = true;
            result[i] = center_index;
          }
        }
      }

      {// Adjust centers
        for (int i=0; i<num_clusters; i++)
          for (int j=0; j<N; j++)
            centers[i][j] = 0.0;

        vector<double> attached_weight(num_clusters);
        for (uint i=0; i<elements.size(); i++) {
          attached_weight[result[i]] += weights[i];
          for (int j=0; j<N; j++)
            centers[result[i]][j] += weights[i] * elements[i][j];
        }

        for (int i=0; i<num_clusters; i++) {
          if (attached_weight[i] > 0.00001) {
            //cerr << "Center " << i << ":";
            for (int j=0; j<N; j++) {
              centers[i][j] /= attached_weight[i];
              //cerr << ' ' << centers[i][j];
            }
            //cerr << '\n';

          } else {
            cerr << "Unsuccesfull center!\n";
            // Unsuccesfull center!
            int picked = rand()%num_clusters;
            for (int j=0; j<N; j++)
              centers[i][j] = elements[picked][j];
          }
        }
      }
    }

    if (total_fit < best_generation_fit) {
      //cerr << "Fit impr. from " << best_generation_fit << " to " << total_fit << '\n';
      best_generation_fit = total_fit;
      for (uint i=0; i<result.size(); i++)
        best_result[i] = result[i];

#ifdef PRINT
      for (int i=0; i<num_clusters; i++)
        for (int j=0; j<N; j++)
          best_centers[i][j] = centers[i][j];
#endif
    }
  }

#ifdef PRINT
  big_output << "\n\nfind_clusters(" << elements.size() << "," << weights.size() << ","
      << num_clusters << "," << NUM_GENERATIONS << "," << MAX_ITERATIONS << ")\n";
  big_output << "best_centers:\n";
  for (int i=0; i<num_clusters; i++) {
    big_output << "Center " << i << " :";
    for (int j=0; j<N; j++)
      big_output << " " << doubleToString(best_centers[i][j], 1, 3);
    big_output << "\n";
  }
  big_output << "\n";
  for (uint i=0; i<elements.size(); i++) {
    big_output << "elements[" << i << "](w=" << doubleToString(weights[i], 6, 3) << ") : " << best_result[i] << " :";
    for (uint j=0; j<elements[i].size(); j++)
      big_output << " " << doubleToString(elements[i][j], 1, 3);
    big_output << "\n";
  }
#endif

  return best_result;
}

template vector<int> find_clusters(const vector<vector<uint> > &,
    const vector<double> &, int, int, int);


// A point is given a weight equal to the euclidean distance from origo, and then
// each point is scaled to a point on the N-dimensional hypersphere with radius 1.
// The find_clusters is called
vector<int> find_clusters2(const vector<vector<uint> > &_elements, int num_clusters,
    int NUM_GENERATIONS, int MAX_ITERATIONS) {
#ifndef NDEBUG
  assert((int)_elements.size() >= num_clusters);
  assert(num_clusters > 0);
  int N = _elements[0].size();
  for (uint i=0; i<_elements.size(); i++)
    assert((int)_elements[i].size() == N);
  assert(NUM_GENERATIONS > 0);
  assert(MAX_ITERATIONS > 0);
#endif

  if (num_clusters == (int)_elements.size()) {
    vector<int> result(num_clusters);
    for (int i=0; i<num_clusters; i++)
      result[i] = i;
    return result;
  }


  vector<vector<double> > elements(_elements.size());
  vector<double> weights(_elements.size());
  // Convert _elements to points on the N-dimensional hypersphere with radius 1.
  // 
  for (uint i=0; i<elements.size(); i++) {
    elements[i] = vector<double>(_elements[i].size());

    double w = 0;
    for (uint j=0; j<_elements[i].size(); j++)
      w += _elements[i][j];
    //double(_elements[i][j])*double(_elements[i][j]);

    if (w > 0.5) {
      weights[i] = w;// = sqrt(w); // Now w is euclidean distance from origo
      double f = 1.0/w;
      for (uint j=0; j<elements[i].size(); j++)
        elements[i][j] = f*_elements[i][j];

    } else {

      weights[i] = 0.0;

      for (uint j=0; j<elements[i].size(); j++)
        elements[i][j] = 0.0;
    }
  }

  return find_clusters(elements, weights, num_clusters, NUM_GENERATIONS, MAX_ITERATIONS);
}








#define N1 16
#define N2 1
#define N3 4
void test_find_clusters() {
  vector<vector<uint> > test(N1);
  for (int i=0; i<N1; i++) {
    test[i] = vector<uint>(N2);
    for (int j=0; j<N2; j++)
      test[i][j] = rand()%100;
  }
  vector<double> w(N1, 1.0);

  vector<int> result = find_clusters(test, w, N3);

  for (int i=0; i<N1; i++) {
    cerr << "Group " << result[i] << ":";
    for (int j=0; j<N2; j++)
      cerr << ' ' << test[i][j]/10 << test[i][j]%10;
    cerr << '\n';
  }
}

//g++ clustering_algorithm.cxx -o test
//int main() { test_find_clusters(); }
