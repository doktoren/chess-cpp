#include "mapping_of_wildcards.hxx"

#include <queue>
#include <assert.h>

// VERY unlikely that the cost will overflow
inline int cost(int diff_a, int diff_b) {
  // Returns 0 if diff_a == diff_b == 0, and diff_a*diff_b+1 otherwise
  return (diff_a || diff_b) + diff_a*diff_b;
  //return diff_a + diff_b;
}

namespace M1 {

struct KLAF {
  KLAF() {}
  KLAF(int diff_a, int diff_b, int a, int b) :
    diff_a(diff_a), diff_b(diff_b), a(a), b(b) {}
  int cost() const { return ::cost(diff_a, diff_b); }
  int diff_a, diff_b;
  int a,b;
};
bool operator<(const KLAF &a, const KLAF &b) {
  // Best: Small cost and small indexes
  if (a.cost() != b.cost())
    return a.cost() > b.cost();

  if (a.a != b.a)
    return a.a > b.a;
  return a.b > b.b;
}

priority_queue<KLAF> pq; // Returns big elements first (?)
int block_size, num_blocks;
vector<int> indexes;
uchar *table;

bool allowed_cost_exceeded;
int allowed_pq_size, allowed_cost, prev_allowed_cost;
double allowed_cost_inc_factor;

#define do_print false
#define stream if (do_print) big_output

inline void print(int diff_a, int diff_b, int a1, int a2, int b1, int b2) {
  if (!do_print) return;
  stream << '\n' << block_size << ": The blocks [" << a1 << "..." << a2 << "]:\n\t";
  for (int i=a1; i<=a2; i++)
    stream << "[" << indexes[i] << "..." << (indexes[i]+block_size) << "[, ";
  stream << "\nMatches the blocks [" << b1 << "..." << b2 << "]:\n\t";
  for (int i=b1; i<=b2; i++)
    stream << "[" << indexes[i] << "..." << (indexes[i]+block_size) << "[, ";
  stream << "\nIf (" << diff_a << "," << diff_b << ") wildcard(s) are changed.\n";
}

#define td(x, d) (table[indexes[x]+(d)])

// returns (-1,-1) if infinite
inline pair<int,int> calc_diffs(int a, int b) {
  int diff_a = 0;
  int diff_b = 0;
  for (int i=0; i<block_size; i++) {
    if (td(a, i) == td(b, i)) continue;
    if (!td(a, i)) {
      ++diff_a;
    } else if (!td(b, i)) {
      ++diff_b;
    } else {
      return pair<int,int>(-1,-1);
    }
  }
  return pair<int,int>(diff_a, diff_b);
}

// returns -1 if infinite
inline int calc_cost(int a, int b) {
  pair<int,int> diffs = calc_diffs(a,b);
  return cost(diffs.first, diffs.second);
}

void unify(int a, int b) {
  for (int i=0; i<block_size; i++) {
    if (td(a, i) != td(b, i)) {
      if (td(a, i)) td(b, i) = td(a, i);
      else td(a, i) = td(b, i);
    }
  }
}
#undef td

inline void my_push(int diff_a, int diff_b, int a, int b) {
  pq.push(KLAF(diff_a, diff_b, a, b));
  if ((pq.size() & 0xFFF)==0) cerr << "pq.size() = " << pq.size() << '\n';

  if ((int)pq.size() > allowed_pq_size  &&  allowed_cost > 1) {
    allowed_pq_size = (int)(1.5*allowed_pq_size);
    allowed_cost_inc_factor = sqrt(allowed_cost_inc_factor);
    allowed_cost = (int)(allowed_cost_inc_factor * prev_allowed_cost);
    if (allowed_cost == prev_allowed_cost) ++allowed_cost;
    cerr << "Priority queue is getting too filled, lowering allowed cost to " << allowed_cost << "\n";
  }
}


#define t(x) (table[indexes[x]+depth])
int _stat[4];
// remember: 0 is wildcard
// rec(a1,a2, b1,b2, diff, depth)
void rec(int a1, int a2, int b1, int b2, int diff_a, int diff_b, int depth) {
  assert(0<=depth  &&  depth<block_size);

  if (a1==a2) _stat[0]++; else _stat[1]++;
  if (b1==b2) _stat[2]++; else _stat[3]++;

  // avoid symmetric duplicates
  if (a1>b1) return;

  if (a1==a2  &&  b1==b2) {
    if (a1==b1) return;
    
    do {
      if (t(a1)!=t(b1)) {
	if (!t(a1)) {
	  ++diff_a;
	  if (cost(diff_a, diff_b) > allowed_cost) {
	    allowed_cost_exceeded = true;
	    return;
	  }
	} else if (!t(b1)) {
	  ++diff_b;
	  if (cost(diff_a, diff_b) > allowed_cost) {
	    allowed_cost_exceeded = true;
	    return;
	  }
	} else {
	  // t(a1)  &&  t(b1)  => Not uniteable!
	  return;
	}
      }
      ++depth;
    } while (depth < block_size);
    // Now depth == block_size
    my_push(diff_a, diff_b, a1, b1);
    return;
  }


  // This block avoids anarchistic recursion.
  // It does this by increasing the depth to a point where the blocks
  // in [a1..a2] are no longer identical or the blocks in [b1..b2] are no longer identical
  do {
    // The blocks are sorted lexicographically. [a1..a2] and [b1..b2] agree on the first depth entries.
    // If [a1..a2] don't agree on the first depth+1 entries, then block[a1][depth]!=block[a2][depth]
    // (because of lex. property). Similar for b1,b2.
    if (t(a1)!=t(a2)  ||  t(b1)!=t(b2)) break;
    
    // Ok, the blocks agree also on entry depth.
    // Check if this entry is not uniteable or uniteable at some cost
    if (t(a1)!=t(b1)) {
      if (!t(a1)) {
	++diff_a;
	if (cost(diff_a, diff_b) > allowed_cost) {
	  allowed_cost_exceeded = true;
	  return;
	}
      } else if (!t(b1)) {
	++diff_b;
	if (cost(diff_a, diff_b) > allowed_cost) {
	  allowed_cost_exceeded = true;
	  return;
	}
      } else {
	// t(a1)  &&  t(b1)  => Not uniteable!
	return;
      }
    }

    ++depth;
    assert(depth < block_size);// All blocks are different and a1!=a2 or b1!=b2
  } while (true);

  // Are we at the end of the blocks (ie. no further recursion)?
  if (depth+1 == block_size) {
#ifndef NDEBUG
    for (int a=a1; a<a2; a++) assert(t(a)!=t(a+1));
    for (int b=b1; b<b2; b++) assert(t(b)!=t(b+1));
#endif
    for (int a=a1; a<=a2; a++) {
      for (int b=b1; b<=b2; b++) {
	if (a < b) {
	  if (t(a)!=t(b)) {
	    if (!t(a)) {
	      if (cost(diff_a+1, diff_b) <= allowed_cost) {
		my_push(diff_a+1, diff_b, a, b);
	      } else {
		allowed_cost_exceeded = true;
	      }
	    } else if (!t(b1)) {
	      if (cost(diff_a+1, diff_b) <= allowed_cost) {
		my_push(diff_a, diff_b+1, a, b);
	      } else {
		allowed_cost_exceeded = true;
	      }
	    } else {
	      // t(a)  &&  t(b)  => Not uniteable!
	    }
	  } else if (diff_a || diff_b) {
	    my_push(diff_a, diff_b, a, b);
	  }
	}
      }
    }

    return;
  }


  // Match a-block wildcard with b-block non-wildcards
  if (t(a1) == 0) {
    if (cost(diff_a+1, diff_b) <= allowed_cost) {
      int _a1 = a1, _a2 = a1;
      while (_a2<a2  &&  !t(_a2+1)) ++_a2;
      
      int _b1 = b1, _b2 = b1;
      do {
	while (_b2<b2  &&  t(_b2) == t(_b2+1)) ++_b2;
	
	if (t(_b1))
	  rec(_a1, _a2, _b1, _b2, diff_a+1, diff_b, depth+1);
	
	_b1 = ++_b2;
      } while (_b1 <= b2);
    } else {
      // Maybe no matches would have been found anyway, but setting
      // allowed_cost_exceeded = true  at most harms the execution time
      allowed_cost_exceeded = true;
    }
  }

  // Match b-block wildcard with a-block non-wildcards
  // Only if a and b are different (avoid duplicates)
  if (t(b1) == 0  &&  a1 != b1) {
    if (cost(diff_a, diff_b+1) <= allowed_cost) {
      int _b1 = b1, _b2 = b1;
      while (_b2<b2  &&  !t(_b2+1)) ++_b2;

      int _a1 = a1, _a2 = a1;
      do {
	while (_a2<a2  &&  t(_a2) == t(_a2+1)) ++_a2;

	if (t(_a1))
	  rec(_a1, _a2, _b1, _b2, diff_a, diff_b+1, depth+1);
	
	_a1 = ++_a2;
      } while (_a1 <= a2);
    } else {
      // Maybe no matches would have been found anyway, but setting
      // allowed_cost_exceeded = true  at most harms the execution time
      allowed_cost_exceeded = true;
    }
  }

  // Exact match a-block and b-block
  int _a1 = a1, _a2 = a1;
  while (_a2<a2  &&  t(_a2) == t(_a2+1)) ++_a2;
  int _b1 = b1, _b2 = b1;
  while (_b2<b2  &&  t(_b2) == t(_b2+1)) ++_b2;
  do {
    bool next_a = t(_a1) <= t(_b1);
    bool next_b = t(_b1) <= t(_a1);

    if (next_a  &&  next_b) // A match
      rec(_a1, _a2, _b1, _b2, diff_a, diff_b, depth+1);

    if (next_a) {
      _a1 = ++_a2;
      while (_a2<a2  &&  t(_a2) == t(_a2+1)) ++_a2;
    }

    if (next_b) {
      _b1 = ++_b2;
      while (_b2<b2  &&  t(_b2) == t(_b2+1)) ++_b2;
    }

  } while (_a1<=a2 && _b1<=b2);
}


int my_cmp(const void *i1, const void *i2) {
  for (int i=0; i<block_size; i++) {
    int tmp = table[*((int *)i1)+i] - table[*((int *)i2)+i];
    if (tmp) return tmp;
  }
  return 0;
}

void _map_wildcards(uchar *bdd_table, int log_size) {
  cbo << "map_wildcards(" << (int)bdd_table << ", " << log_size << ") called.\n";
  int size = 1 << log_size;

  int num = 0;
  for (int l=1; l<=log_size; l++) {

    // Define global variables:
    table = bdd_table;
#ifndef NDEBUG
    while (!pq.empty()) {
      cerr << "Error: popping elements from pq! Why?!?\n";
      assert(0);
    }
#endif
    block_size = size >> l;
    num_blocks = 1 << l;
    
    cbo << "map_wildcards: block_size = " << block_size << "\n";

    allowed_pq_size = 1024 + (num_blocks/64);
    allowed_cost = prev_allowed_cost = 1;
    allowed_cost_inc_factor = 10;

    allowed_cost_exceeded = true;
    while (allowed_cost_exceeded) {
      allowed_cost_exceeded = false;

      int num_mergings = 0;
      cerr << "Allowed cost = " << allowed_cost << "\n";

      indexes = vector<int>(num_blocks);
      for (int i=0; i<num_blocks; i++) indexes[i] = i*block_size;
      qsort(&(indexes[0]), num_blocks, sizeof(int), my_cmp);

      // index_ref[].first = index of beginning of block in table that...
      vector<pair<int, int> > index_ref(num_blocks);
      int num_block_ref = 0;
      int num_different_blocks = 0;
      for (int i=1; i<num_blocks; i++) {
	int tmp = my_cmp(&(indexes[i-1]), &(indexes[i]));
	if (tmp) {
	  indexes[num_different_blocks++] = indexes[i-1];
	} else {
	  // indexes[i-1] and indexes[i] are the same
	  // Remember indexes[i-1] with an index_ref element
	  // indexes[num_different_blocks] will point to a block identical to indexes[i-1]
	  index_ref[num_block_ref++] = pair<int, int>(indexes[i-1], num_different_blocks);
	}
      }
      indexes[num_different_blocks++] = indexes[num_blocks-1];

      
      if (do_print)
	cerr << "(Num different blocks, block ref) = (" << num_different_blocks
	     << ", " << num_block_ref << ")\n";
      _stat[0] = _stat[1] = _stat[2] = _stat[3] = 0;
      rec(0, num_different_blocks-1, 0, num_different_blocks-1, 0, 0, 0);
      if (do_print)
	cerr << "preprocess_data_0_is_wildcard: recursive call completed\n"
	     << "\t_stat = " << _stat[0] << ", " << _stat[1] << ", " << _stat[2] << ", " << _stat[3] << "\n";

      while (!pq.empty()) {
	int original_cost = pq.top().cost();
	int a = pq.top().a;
	int b = pq.top().b;
	pq.pop();
	if (do_print)
	  stream << "Popping([" << indexes[a] << "..." << indexes[a]+block_size << "[, ["
		 << indexes[b] << "..." << indexes[b]+block_size
		 << "[, cost = " << original_cost << "), size of pq now " << pq.size() << "\n";
	
	// Check that they still can be united
	pair<int,int> diffs = calc_diffs(a, b);
	int new_cost = cost(diffs.first, diffs.second);

	if (diffs.first != -1) {
	  if (diffs.first  ||  diffs.second) {
	    if (new_cost <= original_cost) {
	      ++num;
	      if (do_print) {
		cerr << "Uniting the blocks by converting (" << diffs.first << ","
		     << diffs.second << ") wildcards.\n";
		cerr << "Num = " << num << "\n";
	      }
	      unify(a,b);
	      ++num_mergings;
	    } else {
	      if (do_print)
		cerr << "Uniting cost increased from " << original_cost << " to " << new_cost << "\n";
	      pq.push(KLAF(diffs.first, diffs.second, a, b));
	    }
	  } else {
	    if (do_print)
	      cerr << "Blocks already united.\n";
	  }
	} else {
	  if (do_print)
	    cerr << "Blocks no longer unitable.\n";
	}
      }

      for (int i=0; i<num_block_ref; i++) {
	for (int j=0; j<block_size; j++) {
	  int a = index_ref[i].first + j;
	  int b = indexes[index_ref[i].second] + j;
	  assert(table[a]==table[b]  ||  !table[a]);
	  table[a] = table[b];
	}
      }

      if (do_print)
	cerr << "Num mergings = " << num_mergings << "\n";

      prev_allowed_cost = allowed_cost;
      // Tackle possible overflow
      if (allowed_cost_inc_factor * allowed_cost > 0x7FFFFFFF) {
	allowed_cost = 0x7FFFFFFF;
      } else {
	allowed_cost = (int)(allowed_cost_inc_factor * allowed_cost);
	if (allowed_cost == prev_allowed_cost) ++allowed_cost;
      }
    }
  }
  cbo << "In total " << num << " blocks united\n";
}
#undef t

#undef do_print
#undef stream

}// End namespace

void map_wildcards(uchar *bdd_table, int log_size) {
  map_wildcards2(bdd_table, log_size);
  //M1::_map_wildcards(bdd_table, log_size);

  /*
  { // Map don't cares to frequently most occuring value
    int size = 1 << log_size;
    int count[256];
    memset(count, 0, sizeof(int)*256);
    for (int i=0; i<size; i++)
      ++count[(int)(bdd_table[i])];
    int best = 1;
    cerr << "Mapping don't cares to value " << best << "\n";
    for (int i=2; i<256; i++)
      if (count[i] > count[best]) best = i;
    for (int i=0; i<size; i++)
      if (!bdd_table[i]) bdd_table[i] = best;
  }
  */
}


void test_wildcard_mapping() {
  // Se report_stuff/test_preprocessing.txt

  {
    uchar table[128]=
      { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8,
	1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0,
	1, 1, 2, 2, 3, 3, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4,
	3, 2, 0, 0, 3, 2, 0, 0, 0, 0, 2, 3, 0, 0, 2, 3,
	3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3,
	3, 2, 0, 0, 0, 0, 2, 3, 3, 2, 0, 0, 0, 0, 2, 3,
	1, 1, 1, 1, 1, 1, 2, 3, 3, 2, 1, 1, 1, 1, 1, 1};

    map_wildcards(table, 7);
    for (int i=0; i<128; i++) {
      cout << (int)table[i] << ' ';
      if ((i&15)==15) cout << '\n';
    }
  }

  {
    uchar table[256]=
      { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8,
	1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0,
	1, 1, 2, 2, 3, 3, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4,
	3, 2, 0, 0, 3, 2, 0, 0, 0, 0, 2, 3, 0, 0, 2, 3,
	3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3,
	3, 2, 0, 0, 0, 0, 2, 3, 3, 2, 0, 0, 0, 0, 2, 3,
	1, 1, 1, 1, 1, 1, 2, 3, 3, 2, 1, 1, 1, 1, 1, 1,
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 3, 0, 0, 2, 3, 3, 2, 0, 0, 4, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 3, 3, 2, 0, 0, 0, 0, 2, 3, 4, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


    map_wildcards(table, 8);
    for (int i=0; i<256; i++) {
      cout << (int)table[i] << ' ';
      if ((i&15)==15) cout << '\n';
    }
  }

}

