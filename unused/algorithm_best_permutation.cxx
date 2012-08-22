/*
struct ABP_Edge {
  ABP_Edge() : from(0), to(0), value(0) {}
  ABP_Edge(int from, int to, int value) : from(from), to(to), value(value) {}
  int from, to;
  int value;
};

struct ABP_Ver {
  ABP_Segment() : prev(0), next(0) {}
  bool is_end_point() { return prev==-1 || next==-1; }
  int prev, next;
};

vector<int> Algorithm_Best_Perm(vector<vector<int> > graph);
*/

#include "algorithm_best_permutation.hxx"

#include "board.hxx"

#include <queue>
#include <iostream>
#include <assert.h>
#include <algorithm>

#define big_output if (false) big_output

inline int calc_id(int *l, int &id) {
  while (l[id] != id) id = l[id];
  return id;
}

// if edges_in_solution.size() != 0, then the edges are stored in this vector
// (which must have size graph.size()-1)
// fitness is updated.
// illegal_edge will not be in the solution found
vector<int> Algorithm_Best_Perm(const vector<vector<int> > &graph,
				vector<ABP_Edge> &edges_in_solution,
				int &fitness, ABP_Edge illegal_edge) {
  int size = graph.size();
  int num_edges = (size*(size+1)) >> 1;

  vector<ABP_Vertex> vertices(size);
  for (int i=0; i<size; i++) vertices[i] = ABP_Vertex(i);
  vector<ABP_Edge> edges(num_edges);

  priority_queue<pair<short, short> > pq;

  int index = 0;
  for (int i=0; i<size; i++) {
    for (unsigned int j=0; j<graph[i].size(); j++) {
      edges[index] = ABP_Edge(j, i, graph[i][j]);
      pq.push(pair<short, short>(graph[i][j], index));
      index++;
    }
  }

  int *id_list = new int[2*size];
  for (int i=0; i<2*size; i++) id_list[i] = i;
  int id_list_insert_pos = size;

  fitness = 0;

  // Used for later trying different solutions
  int eis_index = 0;

  while (!pq.empty()) {
    pair<short, short> top = pq.top(); pq.pop();
    ABP_Edge edge = edges[top.second];
    
    // Trying a solution without a specific edge?
    if (edge.from==illegal_edge.from &&
	edge.to==illegal_edge.to) continue;

    if (!vertices[edge.from].is_end_point()  ||
	!vertices[edge.to].is_end_point()) continue;

    if (calc_id(id_list, vertices[edge.from].id) ==
	calc_id(id_list, vertices[edge.to].id)) continue;

    // combine
    vertices[edge.from].connect(edge.to);
    vertices[edge.to].connect(edge.from);

    id_list[vertices[edge.from].id] = id_list_insert_pos;
    id_list[vertices[edge.to].id] = id_list_insert_pos;
    ++id_list_insert_pos;

    fitness += top.first;

    // remember which edges constituted the solution
    if (edges_in_solution.size())
      edges_in_solution[eis_index++] = edge;
  }

  delete[] id_list;

  // go through the chain
  vector<int> result(size);
  int pos = 0;
  for (int i=0; i<size; i++)
    if (vertices[i].is_end_point()) {
      result[index = i] = pos++;
      // big_output << "result[" << index << "] = " << pos-1 << '\n';
      break;
    }
  int prev = -1;
  do {
    if (vertices[index].prev == prev) {
      index = vertices[prev = index].next;
    } else {
      index = vertices[prev = index].prev;
    }
    result[index] = pos++;
    // big_output << "result[" << index << "] = " << pos-1 << '\n';
  } while (!vertices[index].is_end_point());
  
  return result;
}

vector<int> Algorithm_Best_Perm(const vector<vector<int> > &graph) {

  vector<int> best_solution(graph.size());
  for (uint i=0; i<graph.size(); i++)
    best_solution[i] = i;
  int best_fitness = 0;
  for (unsigned int i=1; i<graph.size(); i++)
    best_fitness += graph[i][i-1];
  big_output << "Algorithm_Best_Perm: id. perm. fitness = " << best_fitness << '\n';

  ABP_Edge illegal_edge;
  vector<ABP_Edge> edges_in_solution(graph.size()-1);
  best_solution = Algorithm_Best_Perm(graph, edges_in_solution, best_fitness, illegal_edge);
  big_output << "Fitness of first solution = " << best_fitness << '\n';

  vector<ABP_Edge> dummy(0);
  for (unsigned int i=0; i<edges_in_solution.size(); i++) {
    int fitness;
    vector<int> solution =
      Algorithm_Best_Perm(graph, dummy, fitness, edges_in_solution[i]);
    
    if (fitness > best_fitness) {
      big_output << "Fitness improved to " << fitness << '\n';
      best_fitness = fitness;
      best_solution = solution;
    } else {
      //big_output << "Found other fitness = " << fitness << " (not usefull)\n";
    }
  }

  return best_solution;
}


//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################


// Binary bidirectional
vector<int> Algorithm_Best_Perm_1b(const vector<vector<int> > &graph) {
  int size = graph.size();
  assert(!(size&(size-1))); // Size must be power of 2
  for (int i=0; i<size; i++)
    assert((int)graph[i].size() == size);

  vector<int> result(size);

  if (size == 1) {
    result[0] = 0;
    return result;
  }

  priority_queue<pair<int, pair<int, int> > > pq;
  for (int i=0; i<size; i++)
    for (int j=0; j<size; j++)
      if (i!=j)
	pq.push(pair<int, pair<int, int> >(graph[i][j], pair<int,int>(i,j)));
  vector<bool> used(size);
  for (int i=0; i<size; i++)
    used[i] = false;
  int hsize = size>>1;
  vector<pair<int, int> > pairs(hsize);
  int pair_index = 0;
  
  while (!pq.empty()) {
    int a = pq.top().second.first;
    int b = pq.top().second.second;
    pq.pop();
    if (!used[a]  &&  !used[b]) {
      used[a] = used[b] = true;
      pairs[pair_index++] = pair<int,int>(a,b);
    }
  }

  vector<vector<int> > _graph(hsize);
  for (int i=0; i<hsize; i++) {
    _graph[i] = vector<int>(hsize);
    for (int j=0; j<hsize; j++)
      _graph[i][j] = graph[pairs[i].second][pairs[j].first];
  }
  
  vector<int> rec_result = Algorithm_Best_Perm_1b(_graph);
  for (int i=0; i<hsize; i++) {
    result[2*i] = pairs[rec_result[i]].first;
    result[2*i+1] = pairs[rec_result[i]].second;
  }

  return result;
}


//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################

// Binary bidirectional
Solution Algorithm_Best_Perm_1c(const vector<vector<int> > &graph) {
  int size = graph.size();
  assert(!(size&(size-1))); // Size must be power of 2
  for (int i=0; i<size; i++)
    assert((int)graph[i].size() == size);

  priority_queue<pair<int, pair<int, int> > > pq;
  for (int i=0; i<size; i++)
    for (int j=0; j<size; j++)
      if (i!=j)
	pq.push(pair<int, pair<int, int> >(graph[i][j], pair<int,int>(i,j)));
  vector<bool> used(size);
  for (int i=0; i<size; i++)
    used[i] = false;
  int hsize = size>>1;
  Solution result(hsize);
  int result_index = 0;
  
  while (!pq.empty()) {
    int a = pq.top().second.first;
    int b = pq.top().second.second;
    pq.pop();
    if (!used[a]  &&  !used[b]) {
      used[a] = used[b] = true;
      result[result_index++] = pair<int,int>(a,b);
    }
  }

  return result;
}


//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################
//####################################################################



void remove_edge(const Graph &graph, GraphEntry& sum, double &fitness, pair<short, short> edge) {
  const GraphEntry &tmp = graph[edge.first][edge.second];
  typedef GraphEntry::const_iterator CI;
  for (CI i=tmp.begin(); i!=tmp.end(); i++) {
    if (i->first) {
      int dec_count = i->second;
      int sum_count = sum[i->first];
      assert(sum_count);
      
      fitness -= ((double)sum_count)*((double)sum_count);
      
      if (dec_count == sum_count) {
	// Remove from sum
	sum.erase(i->first);
      } else {
	// Just decrease count
	sum[i->first] = sum_count -= dec_count;
	fitness += ((double)sum_count)*((double)sum_count);
      }
    }
  }
}

void add_edge(const Graph &graph, GraphEntry& sum, double &fitness, pair<short, short> edge) {
  const GraphEntry &tmp = graph[edge.first][edge.second];
  typedef GraphEntry::const_iterator CI;
  for (CI i=tmp.begin(); i!=tmp.end(); i++) {
    if (i->first) {
      int inc_count = i->second;
      int sum_count = sum[i->first];
      
      if (sum_count) {
	fitness -= ((double)sum_count)*((double)sum_count);
	
	// Increase count
	sum[i->first] = sum_count += inc_count;
	fitness += ((double)sum_count)*((double)sum_count);
	
      } else {
	
	sum[i->first] = inc_count;
	fitness += ((double)inc_count)*inc_count;
      }
    }
  }
}

double edge_fitness(const Graph &graph, GraphEntry& sum, pair<short, short> edge) {
  double result = 0;
  const GraphEntry &tmp = graph[edge.first][edge.second];
  typedef GraphEntry::const_iterator CI;
  for (CI i=tmp.begin(); i!=tmp.end(); i++) {
    if (i->first) {
      double c1 = sum.count(i->first) ? sum[i->first] : 0;
      double c2 = i->second;

      result += 2*c1*c2 + c2*c2;
    }
  }
  return result;
}

void add_edge_best_direction(const Graph &graph, GraphEntry& sum, double &fitness, pair<short, short> &edge) {
  pair<short, short> inv_edge = pair<short, short>(edge.second, edge.first);

  double d = edge_fitness(graph, sum, edge) - edge_fitness(graph, sum, inv_edge);
  if (d < -0.5) {
    // Inverse edge
    edge = inv_edge;
  }
  add_edge(graph, sum, fitness, edge);
}

void sanity_check_solution(const Solution &solution) {
  vector<bool> used(2*solution.size());
  for (uint i=0; i<solution.size(); i++) {
    assert(0<=(int)solution[i].first  &&  (int)solution[i].first<(int)(2*solution.size()));
    assert(!used[solution[i].first]);
    used[solution[i].first] = true;
    assert(0<=(int)solution[i].second  &&  (int)solution[i].second<(int)(2*solution.size()));
    assert(!used[solution[i].second]);
    used[solution[i].second] = true;
  }
}

double calc_fitness(const Graph &graph, const Solution &solution) {
  assert(graph.size()==graph[0].size());
  assert(2*solution.size()==graph.size());

  GraphEntry sum;
  for (uint i=0; i<solution.size(); i++) {
    const GraphEntry &tmp = graph[solution[i].first][solution[i].second];
    typedef GraphEntry::const_iterator CI;
    for (CI i=tmp.begin(); i!=tmp.end(); i++)
      if (i->first)
	sum[i->first] += i->second;
  }

  double result = 0;
  typedef GraphEntry::const_iterator CI;
  for (CI i=sum.begin(); i!=sum.end(); i++)
    result += (i->second)*(i->second);

  return result;
}
/*
void improve_edge_direction(const Graph &graph, GraphEntry &sum, Solution &solution,
			     double &fitness, int edge_number) {
  assert(graph.size()==graph[0].size());
  assert(2*solution.size()==graph.size());

  pair<short, short> tmp(solution[edge_number].second, solution[edge_number].first);

  remove_edge(graph, sum, fitness, solution[edge_number]);
  double d = edge_fitness(graph, sum, tmp) - edge_fitness(graph, sum, solution[edge_number]);
  if (d>0.5) {
    // inverse edge direction!
    solution[edge_number] = tmp;
  }
  add_edge(graph, sum, fitness, solution[edge_number]);
}
*/
void improve_edge_directions(const Graph &graph, GraphEntry &sum, Solution &solution, double &fitness) {
  assert(graph.size()==graph[0].size());
  assert(2*solution.size()==graph.size());

  for (uint i=0; i<solution.size(); i++) {
    pair<short, short> tmp(solution[i].second, solution[i].first);

    remove_edge(graph, sum, fitness, solution[i]);
    double d = edge_fitness(graph, sum, tmp) - edge_fitness(graph, sum, solution[i]);
    if (d>0.5) {
      // inverse edge direction!
      solution[i] = tmp;
    }
    add_edge(graph, sum, fitness, solution[i]);
  }
}
/*
void inverse_an_edge(const Graph &graph, GraphEntry &sum, Solution &solution, double &fitness) {
  assert(graph.size()==graph[0].size());
  assert(2*solution.size()==graph.size());

  vector<double> prop(solution.size());
  double dsum = 0.0;
  int num_neg = 0;
  for (uint i=0; i<prop.size(); i++) {
    pair<short, short> tmp(solution[i].second, solution[i].first);

    double before = fitness;
    remove_edge(graph, sum, fitness, solution[i]);
    double d = edge_fitness(graph, sum, tmp) - edge_fitness(graph, sum, solution[i]);
    add_edge(graph, sum, fitness, solution[i]);
    assert(before-0.01 <= fitness  &&  fitness <= before+0.01);

    //big_output << d << ' ';
    if (d>0) {
      dsum += prop[i] = d;
    } else {
      ++num_neg;
    }
  }
  //big_output << '\n';
  double neg_prop = 0.25 / prop.size();
  double scale = (1-num_neg*neg_prop)/dsum;
  dsum = 0.0;
  for (uint i=0; i<prop.size(); i++) {
    if (prop[i]==0) prop[i] = neg_prop;
    else prop[i] *= scale;
    dsum += prop[i];
  }
  //big_output << "dsum = " << dsum << "\n";

  // Inverse a pair
  // dsum either 0.25 (iff all swap values non-position) or 1.0
  double cut = (dsum < 0.5) ? (0.000000025 * (rand()%1000000)) : (0.0000001 * (rand()%1000000));
  int index = -1;
  for (uint i=0; i<prop.size(); i++) {
    cut -= prop[i];
    if (cut <= 0) {
      index = i;
      break;
    }
  }

  assert(index != -1);

  pair<short, short> &edge(solution[index]);
  remove_edge(graph, sum, fitness, edge);
  swap(edge.first, edge.second);
  add_edge(graph, sum, fitness, edge);
}
*/
void total_shuffle(const Graph &graph, GraphEntry &sum, Solution &solution, double &fitness) {
  // Clear solution
  sum.clear();
  fitness = 0.0;

  // Construct new random solution
  vector<pair<int, int> > random(graph.size());
  for (uint i=0; i<random.size(); i++)
    random[i] = pair<int, int>(rand(), i);
  sort(random.begin(), random.end());
  for (uint i=0; i<solution.size(); i++) {
    solution[i] = pair<short, short>(random[2*i].second, random[2*i+1].second);
    add_edge_best_direction(graph, sum, fitness, solution[i]);
  }
}

void n_shuffle(const Graph &graph, uint shuffle_num_edges,
	     GraphEntry &sum, Solution &solution, double &fitness) {
  // n_shuffle would also work with shuffle_num_edges == 1 or solution.size()
  // but it is stupid to call it with such arguments
  assert(2<=shuffle_num_edges  &&  shuffle_num_edges<solution.size());

  // Choose shuffle_num_edges edges to shuffle
  vector<bool> shuff(solution.size());
  vector<int> freed_edges(shuffle_num_edges);
  vector<int> freed_indexes(2*shuffle_num_edges);
  for (uint i=0; i<shuffle_num_edges; i++) {
    int r;
    do {
      r = rand()%solution.size();
    } while (shuff[r]);
    shuff[r] = true;
    freed_edges[i] = r;
    freed_indexes[2*i] = solution[r].first;
    freed_indexes[2*i+1] = solution[r].second;
    remove_edge(graph, sum, fitness, solution[r]);
  }

  // Combine the freed indexes in a random way
  vector<pair<int, int> > random(freed_indexes.size());
  for (uint i=0; i<random.size(); i++)
    random[i] = pair<int, int>(rand(), freed_indexes[i]);
  sort(random.begin(), random.end());
  for (uint i=0; i<shuffle_num_edges; i++) {
    solution[freed_edges[i]] = pair<short, short>(random[2*i].second, random[2*i+1].second);
    add_edge_best_direction(graph, sum, fitness, solution[freed_edges[i]]);
  }
}

// Returns number of edges set randomly
int cross_over(const Graph &graph, const Solution &s1, const Solution &s2,
	       Solution &result, double &fitness, GraphEntry &sum) {
  // In turn take a random pair from s1 and s2.
  // When taking a pair has failed n times, choose the rest at random

  fitness = 0.0;
  sum.clear();
  int num_arb_set = 0;

  vector<bool> used(graph.size());
  uint num_try = 0;
  int pick_next = 1;
  for (uint i=0; i<result.size(); i++) {
    bool succes = false;
    while (!succes  &&  num_try < result.size()) {
      // Try picking a random pair from s1 or s2 (depending on which turn it is).
      ++num_try;

      int r;
      do {
	r = rand()%graph.size();
      } while (used[r]);

      if (pick_next == 1) {

	uint j = 0;
	while (s1[j].first != r  &&  s1[j].second != r) {
	  ++j;
	  assert(j < s1.size());
	}
	
	if (!used[s1[j].first]  &&  !used[s1[j].second]) {
	  // Edge can be used.
	  succes = true;
	  result[i] = s1[j];
	  used[result[i].first] = used[result[i].second] = true;
	  
	  add_edge(graph, sum, fitness, s1[j]);
	}

      } else {

	uint j = 0;
	while (s2[j].first != r  &&  s2[j].second != r) {
	  ++j;
	  if (!(j < s2.size())) {
	    cerr << "ARGH! " << j << ' ' << r << "\n";
	    for (uint i=0; i<s2.size(); i++)
	      cerr << (int)s2[i].first << " - " << (int)s2[i].second << "\n";
	    assert(0);
	  }
	}
	
	if (!used[s2[j].first]  &&  !used[s2[j].second]) {
	  // Edge can be used.
	  succes = true;
	  result[i] = s2[j];
	  used[result[i].first] = used[result[i].second] = true;

	  add_edge(graph, sum, fitness, s2[j]);
	}

      }
      pick_next ^= 3;
    }
    if (succes) continue;

    // Pick rest of edges arbitrarily

    // Pick (r1,r2) at random
    uint r1=rand()%graph.size(),r2=rand()%graph.size();
    while (used[r1]) {
      ++r1;
      if (r1==graph.size()) r1=0;
    }
    used[r1] = true;
    while (used[r2]) {
      ++r2;
      if (r2==graph.size()) r2=0;
    }
    used[r2] = true;

    result[i] = pair<short, short>(r1, r2);

    add_edge_best_direction(graph, sum, fitness, result[i]);

    ++num_arb_set;
  }

  // Todo: tjek om kanter skal vendes

  return num_arb_set;
}

void print_solution(ostream &os, const Graph &graph, const Solution &s, const double &fitness) {
  os << "Solution with fitness " << fitness << ":\n";
  for (uint i=0; i<s.size(); i++) {
    os << (int)s[i].first << " - " << (int)s[i].second << '\n';
    if (false) {
      const GraphEntry &tmp = graph[s[i].first][s[i].second];
      typedef GraphEntry::const_iterator CI;
      priority_queue<pair<int, MappedType> > pq;
      for (CI i=tmp.begin(); i!=tmp.end(); i++)
	pq.push(pair<int, MappedType>(i->second, i->first));
      while (!pq.empty()) {
	os << "ValPair(" << pq.top().second << ") = " << pq.top().first << ", ";
	pq.pop();
      }
      big_output << '\n';
    }
  }
}


void GA(const Graph &graph, vector<Solution> &solutions, vector<GraphEntry> &sums,
	vector<double> &fitnesses, int num_iterations) {
  int population_size = solutions.size();
  assert((int)sums.size() == population_size);
  assert((int)fitnesses.size() == population_size);

  for (int iteration=0; iteration<num_iterations; iteration++) {
    big_output << "Doing iteration " << iteration << "\n";
    
    vector<int> order(population_size);
    
    {// Init order
      vector<pair<double, int> > sorter(population_size);
      for (int i=0; i<population_size; i++)
	sorter[i] = pair<double, int>(-fitnesses[i], i);
      sort(sorter.begin(), sorter.end());
      for (int i=0; i<population_size; i++)
	order[i] = sorter[i].second;
    }

    //cerr << "Iteration " << iteration << ": best fitness = " << fitnesses[order[0]] << '\n';

    big_output << "Fitnesses:\n";
    for (int i=0; i<population_size; i++)
      big_output << fitnesses[order[i]] << ' ';
    big_output << '\n';

    int keep_num_best_unmodified = 1;
    int num_killed = 6*population_size/8;
    int num_cross_over = 2*population_size/8;

    assert(num_cross_over <= num_killed);//rest is best copied
    
    {// Replace worst half
      int j = 0;
      for (int i=population_size - num_killed; i<population_size - num_cross_over; i++) {
	//big_output << (j>>2) << '\n';;
	solutions[order[i]] = solutions[order[j>>2]];
	sums[order[i]] = sums[order[j>>2]];
	fitnesses[order[i]] = fitnesses[order[j>>2]];
	++j;
      }

      for (int i=population_size - num_cross_over; i<population_size; i++) {
	int r2,r1 = rand()%(population_size - num_killed);
	do {
	  r2 = rand()%(population_size - num_killed);
	} while (r1==r2);

	int arb_set = cross_over(graph, solutions[order[r1]], solutions[order[r2]],
				 solutions[order[i]], fitnesses[order[i]], sums[order[i]]);
	big_output << i << ":\tcrossover(" << arb_set << "): " << fitnesses[order[i]] << "\n";
      }
    }

    {// Mutate all except from the crossovers and the best solution
      /*
      const int p_inverse_an_edge = 39;
      const int p_n_shuffle = solutions[0].size()>2 ? 10 : 0;
      const int p_shuffle = 1;
      */

      const int p_n_shuffle = solutions[0].size()>2 ? 49 : 0;
      const int p_improve_edge_directions = 5;
      const int p_shuffle = 1;

      for (int ii=keep_num_best_unmodified; ii<population_size - num_cross_over; ii++) {
	int i = order[ii];

	int prop = rand()%100;
	big_output << ii << ":\t";

	if ((prop -= p_improve_edge_directions) < 0) {
	  double before = fitnesses[i];
	  improve_edge_directions(graph, sums[i], solutions[i], fitnesses[i]);
	  big_output << "inverse: " << before << " --> " << fitnesses[i] << "\n";
	
	} else if ((prop -= p_n_shuffle) < 0) {
	  int n;
	  if (solutions[i].size() >= 8) {
	    int r = rand()%100;
	    if (r<10) n = 2;
	    else if (r<70) n = 3;
	    else if (r<85) n = 4;
	    else if (r<95) n = 5;
	    else if (r<98) n = 6;
	    else n = 7;
	  } else {
	    n = (rand()%(solutions[i].size()-2))+2;
	  }
	  double before = fitnesses[i];
	  n_shuffle(graph, n, sums[i], solutions[i], fitnesses[i]);
	  big_output << "n_shuffle(" << n << "): " << before << " --> " << fitnesses[i] << "\n";

  
	} else if ((prop -= p_shuffle) < 0) {
	  double before = fitnesses[i];
	  total_shuffle(graph, sums[i], solutions[i], fitnesses[i]);
	  big_output << "total_shuffle: " << before << " --> " << fitnesses[i] << "\n";

	} else {
	  big_output << "Unmodified solution: " << fitnesses[i] << "\n";
	
	}

#ifndef NDEBUG
	sanity_check_solution(solutions[i]);

	double calculated_fitness = calc_fitness(graph, solutions[i]);
	if (calculated_fitness+1.0 < fitnesses[i]) {
	  big_output << "calculated_fitness(" << calculated_fitness << ")+1.0 < fitnesses[i]("
	       << fitnesses[i] << ")\n";
	  assert(0);
	}
	if (calculated_fitness-1.0 > fitnesses[i]) {
	  big_output << "calculated_fitness(" << calculated_fitness << ")-1.0 > fitnesses[i]("
		     << fitnesses[i] << ")\n";
	  assert(0);
	}
#endif

      }
    }
  }
}

// Returns index of best solution
int GA2(const Graph &graph, vector<Solution> &solutions, vector<GraphEntry> &sums,
	vector<double> &fitnesses, int num_iterations_without_improvement) {
  int population_size = solutions.size();
  assert((int)sums.size() == population_size);
  assert((int)fitnesses.size() == population_size);

  double last_best = 0;
  int last_improvement = 0;
  for (int iteration=0; ; iteration++) {
    big_output << "Doing iteration " << iteration << "\n";
    
    vector<int> order(population_size);
    
    {// Init order
      vector<pair<double, int> > sorter(population_size);
      for (int i=0; i<population_size; i++)
	sorter[i] = pair<double, int>(-fitnesses[i], i);
      sort(sorter.begin(), sorter.end());
      for (int i=0; i<population_size; i++)
	order[i] = sorter[i].second;
    }

    if (fitnesses[order[0]] > last_best+0.5) {
      // Improvement
      last_improvement = iteration;
      last_best = fitnesses[order[0]];
    } else if (iteration-last_improvement >= num_iterations_without_improvement) {
      // Improving too slow
      return order[0];
    }

    //cerr << "Iteration " << iteration << ": best fitness = " << fitnesses[order[0]] << '\n';

    big_output << "Fitnesses:\n";
    for (int i=0; i<population_size; i++)
      big_output << fitnesses[order[i]] << ' ';
    big_output << '\n';

    int keep_num_best_unmodified = 1;
    int num_killed = 6*population_size/8;
    int num_cross_over = 2*population_size/8;

    assert(num_cross_over <= num_killed);//rest is best copied
    
    {// Replace worst half
      int j = 0;
      for (int i=population_size - num_killed; i<population_size - num_cross_over; i++) {
	//big_output << (j>>2) << '\n';;
	solutions[order[i]] = solutions[order[j>>2]];
	sums[order[i]] = sums[order[j>>2]];
	fitnesses[order[i]] = fitnesses[order[j>>2]];
	++j;
      }

      for (int i=population_size - num_cross_over; i<population_size; i++) {
	int r2,r1 = rand()%(population_size - num_killed);
	do {
	  r2 = rand()%(population_size - num_killed);
	} while (r1==r2);

	int arb_set = cross_over(graph, solutions[order[r1]], solutions[order[r2]],
				 solutions[order[i]], fitnesses[order[i]], sums[order[i]]);
	big_output << i << ":\tcrossover(" << arb_set << "): " << fitnesses[order[i]] << "\n";
      }
    }

    {// Mutate all except from the crossovers and the best solution
      /*
      const int p_inverse_an_edge = 39;
      const int p_n_shuffle = solutions[0].size()>2 ? 10 : 0;
      const int p_shuffle = 1;
      */

      const int p_n_shuffle = solutions[0].size()>2 ? 49 : 0;
      const int p_improve_edge_directions = 5;
      const int p_shuffle = 1;

      for (int ii=keep_num_best_unmodified; ii<population_size - num_cross_over; ii++) {
	int i = order[ii];

	int prop = rand()%100;
	big_output << ii << ":\t";

	if ((prop -= p_improve_edge_directions) < 0) {
	  double before = fitnesses[i];
	  improve_edge_directions(graph, sums[i], solutions[i], fitnesses[i]);
	  big_output << "inverse: " << before << " --> " << fitnesses[i] << "\n";
	
	} else if ((prop -= p_n_shuffle) < 0) {
	  int n;
	  if (solutions[i].size() >= 8) {
	    int r = rand()%100;
	    if (r<10) n = 2;
	    else if (r<70) n = 3;
	    else if (r<85) n = 4;
	    else if (r<95) n = 5;
	    else if (r<98) n = 6;
	    else n = 7;
	  } else {
	    n = (rand()%(solutions[i].size()-2))+2;
	  }
	  double before = fitnesses[i];
	  n_shuffle(graph, n, sums[i], solutions[i], fitnesses[i]);
	  big_output << "n_shuffle(" << n << "): " << before << " --> " << fitnesses[i] << "\n";

  
	} else if ((prop -= p_shuffle) < 0) {
	  double before = fitnesses[i];
	  total_shuffle(graph, sums[i], solutions[i], fitnesses[i]);
	  big_output << "total_shuffle: " << before << " --> " << fitnesses[i] << "\n";

	} else {
	  big_output << "Unmodified solution: " << fitnesses[i] << "\n";
	
	}

#ifndef NDEBUG
	sanity_check_solution(solutions[i]);

	double calculated_fitness = calc_fitness(graph, solutions[i]);
	if (calculated_fitness+1.0 < fitnesses[i]) {
	  big_output << "calculated_fitness(" << calculated_fitness << ")+1.0 < fitnesses[i]("
	       << fitnesses[i] << ")\n";
	  assert(0);
	}
	if (calculated_fitness-1.0 > fitnesses[i]) {
	  big_output << "calculated_fitness(" << calculated_fitness << ")-1.0 > fitnesses[i]("
	       << fitnesses[i] << ")\n";
	  assert(0);
	}
#endif

      }
    }
  }

  return -1;
}







// binary blah
Solution Algorithm_Best_Perm_2(const Graph &graph, int population_size, int num_iterations) {
  assert(population_size >= 8);
  
  vector<Solution> solutions(population_size);
  vector<GraphEntry> sums(population_size);
  vector<double> fitnesses(population_size);

  solutions[0] = Solution(graph.size()/2);
  fitnesses[0] = 0.0;
  for (uint i=0; i<solutions[0].size(); i++) {
    solutions[0][i] = pair<short, short>(2*i, 2*i+1);
    add_edge(graph, sums[0], fitnesses[0], solutions[0][i]);
  }

  //total_shuffle(graph, sums[0], solutions[0], fitnesses[0]);

  cerr << "Initial fitness = " << fitnesses[0] << "\n";
  improve_edge_directions(graph, sums[0], solutions[0], fitnesses[0]);

  for (int i=1; i<population_size; i++) {
    solutions[i] = solutions[0];
    sums[i] = sums[0];
    fitnesses[i] = fitnesses[0];
  }

  GA(graph, solutions, sums, fitnesses, num_iterations);

  double max_fitness = 0.0;
  int index = 0;
  for (int i=0; i<population_size; i++)
    if (fitnesses[i] > max_fitness) {
      max_fitness = fitnesses[i];
      index = i;
    }

  //print_solution(cerr, graph, solutions[index], fitnesses[index]);

  return solutions[index];
}








/* 8 grupper af 8
   

*/
Solution Algorithm_Best_Perm_3(const Graph &graph) {
  int population_size = 8;
  assert(!(population_size&1));

  vector<vector<Solution> > solutions(population_size);
  vector<vector<GraphEntry> > sums(population_size);
  vector<vector<double> > fitnesses(population_size);
  for (int i=0; i<population_size; i++) {
    solutions[i] = vector<Solution>(population_size);
    sums[i] = vector<GraphEntry>(population_size);
    fitnesses[i] = vector<double>(population_size);
  }

  for (int i=0; i<population_size; i++)
    for (int j=0; j<population_size; j++) {
      solutions[i][j] = Solution(graph.size()/2);
      fitnesses[i][j] = 0.0;

      if (i==0  &&  j==0) {
	for (uint k=0; k<solutions[i][j].size(); k++) {
	  solutions[i][j][k] = pair<short, short>(2*k, 2*k+1);
	  add_edge(graph, sums[i][j], fitnesses[i][j], solutions[i][j][k]);
	}
	improve_edge_directions(graph, sums[i][j], solutions[i][j], fitnesses[i][j]);

      } else {

	total_shuffle(graph, sums[i][j], solutions[i][j], fitnesses[i][j]);
      }
    }
    

  for (int j=0; j<50; j++) {
    for (int i=0; i<population_size; i++) {
      GA(graph, solutions[i], sums[i], fitnesses[i], 8);
      //for (int k=0; k<population_size; k++) sanity_check_solution(solutions[i][k]);
    }

    vector<vector<int> > order(population_size);
    vector<int> group_order(population_size);
    {// Init order
      for (int k=0; k<population_size; k++) {
	vector<pair<double, int> > sorter(population_size);
	for (int i=0; i<population_size; i++)
	  sorter[i] = pair<double, int>(-fitnesses[k][i], i);
	sort(sorter.begin(), sorter.end());

	order[k] = vector<int>(population_size);
	for (int i=0; i<population_size; i++)
	  order[k][i] = sorter[i].second;
      }

      vector<pair<double, int> > sorter(population_size);
      for (int i=0; i<population_size; i++)
	sorter[i] = pair<double, int>(-fitnesses[i][order[i][0]], i);
      sort(sorter.begin(), sorter.end());
      for (int i=0; i<population_size; i++)
	group_order[i] = sorter[i].second;
    }
    
    {// Scramble one of the less good groups
      int group = group_order[(population_size/2) + rand()%(population_size/2)];
      for (int i=0; i<population_size; i++)
	total_shuffle(graph, sums[group][i], solutions[group][i], fitnesses[group][i]);
    }

    // Keep best in each group and let the others be the result of cross overs
    for (int k=0; k<population_size; k++) {

      //cerr << j << ":\tBest fitness in group " << k << " = " << fitnesses[k][order[k][0]] << '\n';
      

      for (int i=1; i<population_size; i++) {
	int index = (k<i) ? i : (i-1);
	cross_over(graph, solutions[k][order[k][0]], solutions[index][order[index][0]],
		   solutions[k][order[k][i]], fitnesses[k][order[k][i]], sums[k][order[k][i]]);
	
	//sanity_check_solution(solutions[k][order[k][i]]);
      }
    }
  }

  for (int i=1; i<population_size; i++) {
    solutions[0][i] = solutions[i][0];
    sums[0][i] = sums[i][0];
    fitnesses[0][i] = fitnesses[i][0];
  }
  GA(graph, solutions[0], sums[0], fitnesses[0], 500);
  
  double max_fitness = 0.0;
  int index = 0;
  for (int i=0; i<population_size; i++)
    if (fitnesses[0][i] > max_fitness) {
      max_fitness = fitnesses[0][i];
      index = i;
    }

  //print_solution(cerr, graph, solutions[index], fitnesses[index]);

  return solutions[0][index];
}



Solution Algorithm_Best_Perm_4(const Graph &graph, int depth, string s) {
  //assert(depth <= 6);
  //const int pop_size[7] = {64,48,32,24,16,12,8};
  //const int num_iter_no_impr[7] = {300,100,50,30,10,5,2};
  const int pop_size[7] = {32,28,24,20,16,12,8};
  const int num_iter_no_impr[7] = {5000,3500,2200,1500,1000,700,500};

  int population_size = pop_size[depth];

  vector<Solution> solutions(population_size);
  vector<GraphEntry> sums(population_size);
  vector<double> fitnesses(population_size);

  if (depth == 6) {
    solutions[0] = Solution(graph.size()/2);
    total_shuffle(graph, sums[0], solutions[0], fitnesses[0]);
    improve_edge_directions(graph, sums[0], solutions[0], fitnesses[0]);

    //cerr << "Initial fitness = " << fitnesses[0] << "\n";

    for (int i=1; i<population_size; i++) {
      solutions[i] = solutions[0];
      sums[i] = sums[0];
      fitnesses[i] = fitnesses[0];
    }

  } else {

    Solution s1 = Algorithm_Best_Perm_4(graph, depth+1, s+"<<< ");
    Solution s2 = Algorithm_Best_Perm_4(graph, depth+1, s+">>> ");

    for (int i=0; i<population_size; i++) {
      solutions[i] = Solution(graph.size()/2);
      cross_over(graph, s1, s2, solutions[i], fitnesses[i], sums[i]);
    }
  }

  int index = GA2(graph, solutions, sums, fitnesses, num_iter_no_impr[depth]);

  cerr << s << fitnesses[index] << '\n';

  return solutions[index];
}
