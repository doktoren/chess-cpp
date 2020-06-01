#include "mapping_of_wildcards.hxx"

// Definition of BitList
#include "binary_decision_diagram.hxx"
#include "../util/help_functions.hxx"

#include <queue>
#include <assert.h>
#include <cmath>

// When block size reaches K, a more memory efficient version is used
#define K 4

// Remark: the cost can overflow
inline int cost(int diff_a, int diff_b) {
  // Returns 0 if diff_a == diff_b == 0, and diff_a*diff_b+1 otherwise
  return diff_a*diff_b;
}
inline int cost(pair<int, int> diff_ab) {
  return cost(diff_ab.first, diff_ab.second);
}

struct LHD {// Low, High, Diff
  LHD(const int &low, const int &high, const int &diff) :
    low(low), high(high), diff(diff) {}
  int low, high;
  int diff;
};
ostream &operator<<(ostream &os, const LHD &lhd) {
  return os << "LHD(" << lhd.low << "," << lhd.high << "," << lhd.diff << ")";
}


struct RecParam {
  RecParam(const LHD &a, const LHD &b, const int &depth) 
  : a(a), b(b), depth(depth)
  {
    assert(a.low != b.low  ||  a.high == b.high);// Consistency check
  }

  int cost() const { return ::cost(a.diff, b.diff); }

  LHD a,b;
  int depth;
};
ostream &operator<<(ostream &os, const RecParam &rp) {
  return os << "RecParam(" << rp.a << "," << rp.b << "," << rp.depth << ")";
}

bool operator<(const RecParam &p1, const RecParam &p2) {
  if (p1.a.low != p2.a.low)
    return p1.a.low > p2.a.low; // Process those with lowest low value first

  {
    int diff_cost = p1.cost() - p2.cost();
    if (diff_cost) return diff_cost > 0; // Process best matches first
  }

  // if (p1.depth != p2.depth)
  return p1.depth < p2.depth; // Process high depth before low

  //assert(p1.b.low != p2.b.low); // Uniqueness stuff
}

priority_queue<RecParam> rec_calls;


class ActiveSet {
public:
  // Initially, all entries are _active
  ActiveSet() : size(0) {}

  void init(int _size) {
    assert(_size > 0);
    size = _size;
    // Round size up towards nearest power of 2
    if (size & (size-1)) {
      // Remove all bits except for the most significant one.
      do {
        size &= size-1;
      } while (size & (size-1));
      size <<= 1;
    }

    _active.init(2*size, true);
    // Set all padded entries to false
    for (int i=_size; i<size; i++)
      deactivate(i);
  }

  bool initialized() { return size; }

  void clear() { _active.clear(); }

  bool active(int index) {
    assert(initialized());
    return _active[index | size];
  }

  // Returns true iff a bit is _active in [begin_index..end_index[
  bool exists_active(int begin_index, int end_index) {
    assert(initialized());
    assert(begin_index <= end_index);
    assert(0<=begin_index  &&  begin_index<=size);
    assert(0<=end_index  &&  end_index<=size);

    if (begin_index < end_index) {

      begin_index += size;
      // Optimization: (method will often return here)
      if (_active[begin_index]) return true;

      end_index += size;
      do {
        if (begin_index & 1) {
          if (_active[begin_index]) return true;
          ++begin_index;
        }
        begin_index >>= 1;

        if (end_index & 1) {
          --end_index;
          if (_active[end_index-1]) return true;
        }
      } while(begin_index < end_index);

      end_index >>= 1;
    }
    return false;
  }

  void deactivate(int index) {
    assert(initialized());
    index += size;
    _active.update(index, false);
    while(!_active[index ^ 1])
      _active.update(index >>= 1, false);
  }

private:
  int size;
  BitList _active;
};

ActiveSet active_set;


#define td(x, d) (table[blocks[x]+(d)])

uint8_t *table = 0;
int block_size = 0;
vector<int> blocks;
int last_low_value = 0;

int max_allowed_cost = 0x7FFFFFFF;
//bool only_inclusion_unification = false;

struct Match {
  Match() : left(0), right(0), left_diff(0), right_diff(0), cost(0) {}
  Match(int left, int right, int left_diff, int right_diff) :
    left(left), right(right), left_diff(left_diff),
    right_diff(right_diff), cost(::cost(left_diff, right_diff)) {}

  void adjust(int left_adjustment, int right_adjustment) {
    left_diff += left_adjustment;
    right_diff += right_adjustment;
    cost = ::cost(left_diff, right_diff);
  }

  int left, right;
  int left_diff, right_diff;
  int cost;
};
ostream &operator<<(ostream &os, const Match &m) {
  return os << "Match(" << m.right << ":" << m.left_diff
      << "," << m.right << ":" << m.right_diff << ")";
}

bool operator<(const Match &m1, const Match &m2) {
  // Return best solutions first - i.e. those with low cost.
  return m1.cost > m2.cost;
}

// Invariant: matches[0] is the best match
vector<Match> matches;
void add_match(const Match &m) {
#ifndef NDEBUG
  assert(active_set.active(m.left));
  assert(active_set.active(m.right));
  assert(m.left == last_low_value);
  for (int i=0; i<block_size; i++)
    assert(!td(m.left, i)  ||  !td(m.right, i)  ||  td(m.left, i)==td(m.right, i));
#endif
  if (m.cost > max_allowed_cost) return;

  if (matches.size()==0  ||  matches[0].cost <= m.cost) {
    matches.push_back(m);
  } else {
    matches.push_back(matches[0]);
    matches[0] = m;
  }
}



#define t(x) (table[blocks[x]+r.depth])
// r.a may not be left of r.b - i.e. r.a.low <= r.b.low
void rec(RecParam r) {
  //LHD a,b;
  //int depth;
  assert(0<=r.depth  &&  r.depth<block_size);

  assert(r.a.low <= r.b.low);


  // Increase r.a.low until we reach an active (unmapped) block
  if (!active_set.active(r.a.low)) {
    do {
      if (r.a.low == r.a.high) return;
      ++r.a.low;
    } while (!active_set.active(r.a.low));

    if (r.a.low > r.b.low) {
      r.b.low = r.a.low;
      if (r.b.low > r.b.high) return;
    }

    // Check if also the right interval still contain an active element.
    if (!active_set.exists_active(r.b.low, r.b.high+1)) return;

    // We have increased r.a.low - push back on pq. and return;
    rec_calls.push(RecParam(LHD(r.a.low, r.a.high, r.a.diff),
        LHD(r.b.low, r.b.high, r.b.diff), r.depth));
    return;
  }

  if (r.a.low==r.a.high  &&  r.b.low==r.b.high) {
    if (r.a.low == r.b.low) return; // Identity comparison

    do {
      if (t(r.a.low)!=t(r.b.low)) {
        if (!t(r.a.low)) {
          ++r.a.diff;
          if (cost(r.a.diff, r.b.diff) > max_allowed_cost) return;
        } else if (!t(r.b.low)) {
          ++r.b.diff;
          if (cost(r.a.diff, r.b.diff) > max_allowed_cost) return;
        } else {
          // t(r.a.low)  &&  t(r.b.low)  => Not uniteable!
          return;
        }
      }
      ++r.depth;
    } while (r.depth < block_size);
    // Now r.depth == block_size
    if (active_set.active(r.b.low))
      add_match(Match(r.a.low, r.b.low, r.a.diff, r.b.diff));
    return;
  }


  // This block avoids anarchistic recursion.
  // It does this by increasing the depth to a point where the blocks
  // in [r.a.low .. r.a.high] are no longer identical or the blocks
  // in [r.b.low .. r.b.high] are no longer identical
  do {
    // The blocks are sorted lexicographically. [r.a.low .. r.a.high] and
    // [r.b.low .. r.b.high] agree on the first r.depth entries.
    // If [r.a.low .. r.a.high] don't agree on the first r.depth+1 entries,
    //  then block[r.a.low][r.depth] != block[r.a.high][r.depth]
    // (because of lex. property). Similar for r.b.low, r.b.high.
    if (t(r.a.low)!=t(r.a.high)  ||  t(r.b.low)!=t(r.b.high)) break;

    // Ok, the blocks agree also on entry r.depth.
    // Check if this entry is not uniteable or uniteable at some cost
    if (t(r.a.low)!=t(r.b.low)) {
      if (!t(r.a.low)) {
        ++r.a.diff;
        if (cost(r.a.diff, r.b.diff) > max_allowed_cost) return;
      } else if (!t(r.b.low)) {
        ++r.b.diff;
        if (cost(r.a.diff, r.b.diff) > max_allowed_cost) return;
      } else {
        // t(r.a.low)  &&  t(r.b.low)  => Not uniteable!
        return;
      }
    }

    ++r.depth;
    assert(r.depth < block_size);// All blocks are different and r.a.low!=r.a.high or r.b.low!=r.b.high
  } while (true);

  // Are we at the end of the blocks (ie. no further recursion)?
  if (r.depth+1 == block_size) {
#ifndef NDEBUG
    for (int a=r.a.low; a<r.a.high; a++) assert(t(a)!=t(a+1));
    for (int b=r.b.low; b<r.b.high; b++) assert(t(b)!=t(b+1));
#endif
    // No identity matching
    if (r.a.low == r.b.low) ++r.b.low;

    // Find mathes between r.a.low  and  [r.b.low .. r.b.high]
    for (int b=r.b.low; b<=r.b.high; b++) {
      if (active_set.active(b)) {
        if (t(r.a.low)!=t(b)) {
          if (!t(r.a.low)) {
            add_match(Match(r.a.low, b, r.a.diff+1, r.b.diff));
          } else if (!t(b)) {
            add_match(Match(r.a.low, b, r.a.diff, r.b.diff+1));
          } else {
            // t(a)  &&  t(b)  => Not uniteable!
          }
        } else if (r.a.diff || r.b.diff) {
          add_match(Match(r.a.low, b, r.a.diff, r.b.diff));
        }
      }
    }

    // Mathes between [r.a.low+1 .. r.a.high] and  [r.b.low .. r.b.high] are delayed
    if (r.a.low != r.a.high  &&
        active_set.exists_active(++r.a.low, r.a.high+1)) {
      // r.a.low <= r.b.low holds
      rec_calls.push(RecParam(LHD(r.a.low, r.a.high, r.a.diff),
          LHD(r.b.low, r.b.high, r.b.diff), r.depth));
    }

    return;
  }


  // Match a-block wildcard with b-block non-wildcards
  if (t(r.a.low) == 0) {
    int a_low = r.a.low, a_high = r.a.low;
    while (a_high<r.a.high  &&  !t(a_high+1)) ++a_high;

    int b_low = r.b.low, b_high = r.b.low;
    do {
      while (b_high<r.b.high  &&  t(b_high) == t(b_high+1)) ++b_high;

      if (t(b_low)) {
        // a_low == r.a.low, and r.a.low is active
        // Check also that the right interval contains an active element
        if (active_set.exists_active(b_low, b_high+1)  &&
            cost(r.a.diff+1, r.b.diff) <= max_allowed_cost)
          rec_calls.push(RecParam(LHD(a_low, a_high, r.a.diff+1),
              LHD(b_low, b_high, r.b.diff), r.depth+1));
      }

      b_low = ++b_high;
    } while (b_low <= r.b.high);
  }

  // Match b-block wildcard with a-block non-wildcards
  // Only if a and b are different (avoid duplicates)
  if (t(r.b.low) == 0  &&  r.a.low != r.b.low) {
    int b_low = r.b.low, b_high = r.b.low;
    while (b_high<r.b.high  &&  !t(b_high+1)) ++b_high;

    int a_low = r.a.low, a_high = r.a.low;
    do {
      while (a_high<r.a.high  &&  t(a_high) == t(a_high+1)) ++a_high;

      if (t(a_low)) {
        assert(a_low < b_low);

        // Check that both intervals contains active elements
        if (active_set.exists_active(a_low, a_high+1)  &&
            active_set.exists_active(b_low, b_high+1)  &&
            cost(r.a.diff, r.b.diff+1) <= max_allowed_cost)
          rec_calls.push(RecParam(LHD(a_low, a_high, r.a.diff),
              LHD(b_low, b_high, r.b.diff+1), r.depth+1));
      }

      a_low = ++a_high;
    } while (a_low <= r.a.high);
  }

  // Exact match a-block and b-block
  int a_low = r.a.low, a_high = r.a.low;
  while (a_high<r.a.high  &&  t(a_high) == t(a_high+1)) ++a_high;
  int b_low = r.b.low, b_high = r.b.low;
  while (b_high<r.b.high  &&  t(b_high) == t(b_high+1)) ++b_high;
  do {
    bool next_a = t(a_low) <= t(b_low);
    bool next_b = t(b_low) <= t(a_low);

    if (next_a  &&  next_b) { // A match
      // Check that both intervals contains active elements
      if (active_set.exists_active(a_low, a_high+1)  &&
          active_set.exists_active(b_low, b_high+1))
        rec_calls.push(RecParam(LHD(a_low, a_high, r.a.diff),
            LHD(b_low, b_high, r.b.diff), r.depth+1));
    }

    if (next_a) {
      a_low = ++a_high;
      while (a_high<r.a.high  &&  t(a_high) == t(a_high+1)) ++a_high;
    }

    if (next_b) {
      b_low = ++b_high;
      while (b_high<r.b.high  &&  t(b_high) == t(b_high+1)) ++b_high;
    }

  } while (a_low<=r.a.high && b_low<=r.b.high);
}

int my_cmp(const void *i1, const void *i2) {
  for (int i=0; i<block_size; i++) {
    int tmp = table[*((int *)i1)+i] - table[*((int *)i2)+i];
    if (tmp) return tmp;
  }
  return 0;
}




int least_prime_larger_than(int n) {
  if (n==2) return n;
  if (!(n&1)) ++n;
  while(1) {
    bool prime = true;
    for (int i=3; i*i<=n; i+=2)
      if (n%i == 0) {
        prime = false;
        break;
      }
    if (prime) return n;
    n+=2;
  }
}

// It is assumed that 1024 extra entries at the end of the table is enough...
class _hash_table {
public:
  _hash_table() : mem(0) {}
  ~_hash_table() { if(mem) { free(mem); free(ref); } }

  void init(uint8_t *_table, int capacity, int _log_block_size) {
    assert(_table);
    assert(_log_block_size == 1  ||  _log_block_size == 2);

    log_block_size = _log_block_size;
    table = _table;

    // one third of the entries are empty at full capacity
    size = least_prime_larger_than((int)(1.5*capacity)+1);
    // Warning << can't (easily) be used to compute pattern, because
    // x << y  only considers the last 5 bits of y (if y is a variable, not if it is a constant)
    if (log_block_size == 1) {
      pattern = 0xFFFF;
    } else {
      pattern = 0xFFFFFFFF;
    }

    used_entries.init(size+1024);
    mem = (uint8_t *)malloc((size+1024) << log_block_size);
    ref = (int *)malloc(4*(size+1024));
  }

  // Returns -1 if not found
  int get_ref(int index) {
    assert(mem);
    assert(table);
    // table[index .. index+block_size[
    // WARNING: big/little endian dependend code
    uint p;
    {
      uint &e = *((uint *)(&(table[index << log_block_size])));
      p = e & pattern;
      //cerr << "get_ref: " << pattern << ", " << toString(e, 8, 16) << ", " << toString(p, 8, 16) << "\n";
    }
    int i = p % size;

    while (used_entries[i]) {
      if (p == (pattern & *((uint *)(&(mem[i << log_block_size]))))) {
        //cerr << "get_ref(" << index << ") found! (" << toString(p, 8, 16) << ")\n";
        return ref[i];
      }
      assert(i < size+1024);
      ++i;
    }

    return index; // this index has not been remapped
  }

  void set_ref(int index, int new_ref) {
    assert(mem);
    assert(table);
    // table[index .. index+block_size[
    // WARNING: big/little endian dependend code
    uint p = pattern & *((uint *)(&(table[index << log_block_size])));
    int i = p % size;

    while (used_entries[i]) {
      if (p == (pattern & *((uint *)(&(mem[i << log_block_size]))))) {
        ref[i] = new_ref;
        assert(get_ref(index) == new_ref);
        return;
      }
      assert(i < size+1024);
      ++i;
    }

    used_entries.set(i);

    uint &m = *((uint *)(&(mem[i << log_block_size])));
    //cerr << "m(" << toString(m, 8, 16) << " -> ";
    m &= ~pattern;
    m |= p;
    //cerr << toString(m, 8, 16) << ")\n";

    ref[i] = new_ref;

    assert(get_ref(index) == new_ref);
    return;
  }

private:
  int log_block_size, size;
  uint pattern;

  BitList used_entries;
  uint8_t *mem;
  int *ref;

  uint8_t *table;
};




// mergesort returns a sorted list of indexes to table without dublicates
vector<int> mergesort(int begin, int end) {
  assert(begin <= end);
  assert(!((end-begin) & (end-begin-1)));// Interval must be power of 2

  if (end-begin <= (1<<6)) { // Below this size qsort is probably faster
    // Initialize blocks
    int num_blocks = end-begin;
    vector<int> blocks(num_blocks);
    int tmp = (begin-1)*block_size;
    for (int i=0; i<num_blocks; i++)
      blocks[i] = (tmp += block_size);

    // Sort blocks
    qsort(&(blocks[0]), num_blocks, sizeof(int), my_cmp);

    // Remove duplicates
    int insert_pos = 1;
    for (int i=1; i<num_blocks; i++)
      if (my_cmp(&(blocks[i-1]), &(blocks[i])))
        blocks[insert_pos++] = blocks[i];
    blocks.resize(insert_pos);
#ifndef NDEBUG
    for (int i=1; i<insert_pos; i++)
      assert(my_cmp(&(blocks[i-1]), &(blocks[i])) < 0);
#endif
    return blocks;

  } else {

    vector<int> left = mergesort(begin, (begin+end)/2);
    vector<int> right = mergesort((begin+end)/2, end);

    int num_different_blocks = 0;
    { // minimize size - calculate size of merged list
      uint left_index = 0;
      uint right_index = 0;
      while (left_index<left.size()  &&  right_index<right.size()) {
        ++num_different_blocks;
        int cmp = my_cmp(&(left[left_index]), &(right[right_index]));
        left_index += cmp <= 0;
        right_index += cmp >= 0;
      }
      if (left_index < left.size())
        num_different_blocks += (left.size()-left_index);
      if (right_index < right.size())
        num_different_blocks += (right.size()-right_index);
    }

    int left_index = left.size()-1;
    int right_index = right.size()-1;

    // merge the vectors left and right to the vector left, starting from the end
    // (so no later used elements will be overwritten)
    left.resize(num_different_blocks);

    // Do the actual merging
    int insert_index = left.size()-1;
    bool error = false;
    (void)error;  // suppress warning about unused variable
    while (left_index != insert_index) {
      if (left_index < 0) {
        // Copy prefix of right vector to left vector
        memcpy(&(left[0]), &(right[0]), sizeof(int)*(right_index+1));
        break;
      }

      int cmp = my_cmp(&(left[left_index]), &(right[right_index]));

      // if cmp==0 it doesn't matter which value left[insert_pos] gets.
      if (cmp <= 0) {
        // right contains a biggest element;
        left[insert_index] = right[right_index--];
        if (!(insert_index+1 == (int)left.size()  ||
            my_cmp(&(left[insert_index]), &(left[insert_index+1])) <= 0)) {
          cerr << "Error: " << insert_index << ": " << left_index << ", " << right_index+1 << "\n";
          error = true;
          break;
        }
      }
      if (cmp >= 0) {
        // left contains a biggest element;
        left[insert_index] = left[left_index--];
        if (!(insert_index+1 == (int)left.size()  ||
            my_cmp(&(left[insert_index]), &(left[insert_index+1])) <= 0)) {
          cerr << "Error: " << insert_index << ": " << left_index+1 << ", " << right_index << "\n";
          error = true;
          break;
        }
      }
      --insert_index;
    }

#ifndef NDEBUG
    for (uint i=1; i<left.size(); i++) {
      if (error  ||  my_cmp(&(left[i-1]), &(left[i])) >= 0) {

        vector<int> _left = mergesort(begin, (begin+end)/2);
        vector<int> _right = mergesort((begin+end)/2, end);

        cerr << "LEFT (size " << _left.size() << "):\n";
        for (uint i=0; i<_left.size(); i++) {
          if (i) cerr << ",";
          cerr << "(";
          for (int j=0; j<block_size; j++) {
            if (j) cerr << ",";
            cerr << (int)table[_left[i] + j];
          }
          cerr << ")";
        }
        cerr << "\n";
        cerr << "LEFT (size " << _right.size() << "):\n";
        for (uint i=0; i<_right.size(); i++) {
          if (i) cerr << ",";
          cerr << "(";
          for (int j=0; j<block_size; j++) {
            if (j) cerr << ",";
            cerr << (int)table[_right[i] + j];
          }
          cerr << ")";
        }
        cerr << "\n";
        cerr << "MERGED (size " << left.size() << "):\n";
        for (uint i=0; i<left.size(); i++) {
          if (i) cerr << ",";
          cerr << "(";
          for (int j=0; j<block_size; j++) {
            if (j) cerr << ",";
            cerr << (int)table[left[i] + j];
          }
          cerr << ")";
        }
        cerr << "\n";
        assert(0);
      }
    }
#endif

    return left;
  }
}



void map_wildcards(uint8_t *bdd_table, int log_size) {
  cbo << "map_wildcards2(" << log_size << ") called.\n";
  int size = 1 << log_size;

  table = bdd_table;

  int num_diff_values = 0;
  uint8_t non_zero_value = 0;
  {
    int values_used[256];
    memset(values_used, 0, sizeof(int)*256);
    for (int i=0; i<size; i++)
      values_used[table[i]] = 1;
    for (int i=0; i<256; i++) {
      if (values_used[i]) {
        ++num_diff_values;
        non_zero_value = i;
      }
    }
  }

  for (int l=1; l<log_size; l++) {

    // rep = 0: Only inclusion unification allowed
    // rep = 1: Allowed cost i low
    // rep = 2: Allowed cost i all

    for (int rep=0; rep<2; rep++) {

      // Define global variables:
      block_size = size >> l;
      int log_block_size = log_size - l;
      int num_blocks = 1 << l;

      cbo << num_blocks << " blocks of size = " << block_size << "\n";

      switch (rep) {
      case 0:
        max_allowed_cost = 0;
        break;
      case 1:
        max_allowed_cost = block_size;//(int)(sqrt((double)block_size)+0.5);
        break;
      case 2:
        max_allowed_cost = (int)(block_size*(sqrt((double)block_size)+0.5));//0x7FFFFFFF;
        break;
      }

      cbo << "Allowed unification cost = " << max_allowed_cost << "\n";

      //cerr << "only_inclusion_unification = " << only_inclusion_unification << "\n";

      // table[blocks[i]...blocks[i]+block_size[ determines a block

      // ######################################################
      if (block_size > K) { // ################################
        // ######################################################

        blocks = vector<int>(num_blocks);
        for (int i=0; i<num_blocks; i++) blocks[i] = i*block_size;
        qsort(&(blocks[0]), num_blocks, sizeof(int), my_cmp);

        vector<int> index_ref;
        // If index_ref[bn] != bn then the block number index_ref[bn] should be
        // copied to block number bn after all wildcards have been mapped.
        index_ref.resize(num_blocks);
        for (int i=0; i<num_blocks; i++)
          index_ref[i] = i;

        // blocks[0..num_different_blocks[ contains only the different blocks
        int num_different_blocks = 0;
        {
          int block = blocks[0];
          for (int i=1; i<num_blocks; i++) {
            int tmp = my_cmp(&(blocks[i-1]), &(blocks[i]));
            if (tmp) {
              // blocks[i-1] and blocks[i] differ.
              blocks[num_different_blocks++] = block;
              block = blocks[i];
            } else {
              index_ref[blocks[i] >> log_block_size] = index_ref[block >> log_block_size];
            }
          }
          blocks[num_different_blocks++] = block;
          blocks.resize(num_different_blocks);
        }

        if (num_different_blocks == 1) {
          cerr << "num_different_blocks = 1  => Nothing to do!\n";
          // Some assertions would fail.
          continue;
        }

        active_set.init(num_different_blocks);

        cbo << "Number of different blocks is " << num_different_blocks << "\n";

        /*
	  cerr << "index_ref[";
	  for (int i=0; i<num_blocks; i++) {
	  if (i) cerr << ",";
	  cerr << index_ref[i];
	  }
	  cerr << "]\nblocks[";
	  for (int i=0; i<num_different_blocks; i++) {
	  if (i) cerr << ",";
	  cerr << (blocks[i] >> log_block_size);
	  }
	  cerr << "]\n";
         */

        rec_calls.push(RecParam(LHD(0, num_different_blocks-1, 0),
            LHD(0, num_different_blocks-1, 0), 0));

        last_low_value = 0;
        while (!rec_calls.empty()) {
          RecParam r = rec_calls.top();
          rec_calls.pop();

          if (r.a.low > last_low_value) {

            while (matches.size()) {
#ifndef NDEBUG
              for (uint i=0; i<matches.size(); i++) {
                assert(matches[i].left >= last_low_value);
                assert(active_set.active(matches[i].right));
                for (uint j=0; j<matches.size(); j++)
                  if (i!=j)
                    assert(matches[i].right != matches[j].right);
              }
#endif
              // The best match is matches[0].

              // Remember which wildcards were used in the left block
              vector<int> mapped_wildcards(matches[0].left_diff);

              // Unify block matches[0].left with matches[0].right.
              --num_different_blocks;
              // Remember which wildcards from matches[0].left were used.
              //big_output << "Unifying B" << matches[0].left << " with B" << matches[0].right
              //	     << " at cost " << matches[0].left_diff << "*" << matches[0].right_diff << "\n";
              int index = 0;
              for (int i=0; i<block_size; i++) {
                if (td(matches[0].left, i) != td(matches[0].right, i)) {
                  if (td(matches[0].left, i)) {
                    assert(td(matches[0].right,i)==0);
                    // No need to perform the copying - it will be done in the end.
                    // td(matches[0].right, i) = td(matches[0].left, i);
                  } else {
                    assert(td(matches[0].left,i)==0);
                    td(matches[0].left, i) = td(matches[0].right, i);
                    assert(index < matches[0].left_diff);
                    mapped_wildcards[index++] = i;
                  }
                }
              }

              assert(index == matches[0].left_diff);

              // Make a reference from the right block to the left block.
              //cerr << "IndexRef(" << (blocks[matches[0].right] >> log_block_size)
              //     << " -> " << (blocks[matches[0].left] >> log_block_size) << ")\n";
              index_ref[blocks[matches[0].right] >> log_block_size] =
                  index_ref[blocks[matches[0].left] >> log_block_size];

              // Make the right block inactive
              active_set.deactivate(matches[0].right);

              // Update the list of matches
              int insert_pos = 0;
              for (uint i=1; i<matches.size(); i++) {
                int left_adjustment = 0, right_adjustment = 0;
                for (uint j=0; j<mapped_wildcards.size(); j++) {
                  int index = mapped_wildcards[j];
                  assert(td(matches[i].left, index));
                  if (!td(matches[i].right, index)) {
                    // New mapping of wildcard needed in right block
                    ++right_adjustment;
                  } else if (td(matches[i].left, index) == td(matches[i].right, index)) {
                    // The wildcard in the left block is already mapped appropriately
                    --left_adjustment;
                  } else {
                    // No longer unitable!
                    right_adjustment = -1;
                    break;
                  }
                }

                if (right_adjustment != -1) {
                  matches[i].adjust(left_adjustment, right_adjustment);

                  // Assure that matches[0] contains best match
                  if (insert_pos==0  ||  matches[0].cost <= matches[i].cost) {
                    matches[insert_pos++] = matches[i];
                  } else {
                    assert((uint)insert_pos < i);
                    matches[insert_pos++] = matches[0];
                    matches[0] = matches[i];
                  }
                }
              }

              matches.resize(insert_pos);
            }

            //big_output << r << " - " << rec_calls.size() << " remaining!\n";
            last_low_value = r.a.low;
          }

          rec(r);
        }

        { // Copy the changes back to the other blocks

          // Make all index_ref direct
          bool progress = true;
          while (progress) {
            progress = false;
            for (int i=0; i<num_blocks; i++) {
              int &ir = index_ref[i];
              if (ir != index_ref[ir]) {
                ir = index_ref[ir];
                progress= true;
              }
            }
          }

          // Do the copying
          for (int i=0; i<num_blocks; i++) {
            if (i != index_ref[i]) {
              uint8_t *copy_to = &(table[i << log_block_size]);
              uint8_t *copy_from = &(table[index_ref[i] << log_block_size]);
              for (int j=0; j<block_size; j++) {
                assert(!(*copy_to  &&  *copy_to != *copy_from));
                *copy_to++ = *copy_from++;
              }
            }
          }
        }

        cbo << "Number of different blocks is " << num_different_blocks << "\n";

        // ######################################################
      } else { // block_size <= K  ############################
        // ######################################################


        blocks = mergesort(0,num_blocks);
        int num_different_blocks = blocks.size();

        _hash_table ht;
        ht.init(table, num_blocks, log_block_size);
        for (int i=0; i<num_different_blocks; i++)
          ht.set_ref(blocks[i] >> log_block_size, blocks[i] >> log_block_size);

        if (num_different_blocks == 1) {
          cerr << "num_different_blocks = 1  => Nothing to do!\n";
          // Some assertions would fail.
          continue;
        }

        active_set.init(num_different_blocks);

        cbo << "Number of different blocks is " << num_different_blocks << "\n";


        rec_calls.push(RecParam(LHD(0, num_different_blocks-1, 0),
            LHD(0, num_different_blocks-1, 0), 0));

        last_low_value = 0;
        while (!rec_calls.empty()) {
          RecParam r = rec_calls.top();
          rec_calls.pop();

          if (r.a.low > last_low_value) {

            while (matches.size()) {
#ifndef NDEBUG
              for (uint i=0; i<matches.size(); i++) {
                assert(matches[i].left >= last_low_value);
                assert(active_set.active(matches[i].right));
                for (uint j=0; j<matches.size(); j++)
                  if (i!=j)
                    assert(matches[i].right != matches[j].right);
              }
#endif
              // The best match is matches[0].

              // Remember which wildcards were used in the left block
              vector<int> mapped_wildcards(matches[0].left_diff);

              // Unify block matches[0].left with matches[0].right.
              --num_different_blocks;
              // Remember which wildcards from matches[0].left were used.
              //big_output << "Unifying B" << matches[0].left << " with B" << matches[0].right
              //	     << " at cost " << matches[0].left_diff << "*" << matches[0].right_diff << "\n";
              int index = 0;
              for (int i=0; i<block_size; i++) {
                if (td(matches[0].left, i) != td(matches[0].right, i)) {
                  if (td(matches[0].left, i)) {
                    assert(td(matches[0].right,i)==0);
                    // No need to perform the copying - it will be done in the end.
                    // td(matches[0].right, i) = td(matches[0].left, i);
                  } else {
                    assert(td(matches[0].left,i)==0);
                    td(matches[0].left, i) = td(matches[0].right, i);
                    assert(index < matches[0].left_diff);
                    mapped_wildcards[index++] = i;
                  }
                }
              }

              assert(index == matches[0].left_diff);

              // Make a reference from the right block to the left block.
              //cerr << "IndexRef(" << (blocks[matches[0].right] >> log_block_size)
              //     << " -> " << (blocks[matches[0].left] >> log_block_size) << ")\n";
              ht.set_ref(blocks[matches[0].right] >> log_block_size,
                  blocks[matches[0].left] >> log_block_size);

              // Make the right block inactive
              active_set.deactivate(matches[0].right);

              // Update the list of matches
              int insert_pos = 0;
              for (uint i=1; i<matches.size(); i++) {
                int left_adjustment = 0, right_adjustment = 0;
                for (uint j=0; j<mapped_wildcards.size(); j++) {
                  int index = mapped_wildcards[j];
                  assert(td(matches[i].left, index));
                  if (!td(matches[i].right, index)) {
                    // New mapping of wildcard needed in right block
                    ++right_adjustment;
                  } else if (td(matches[i].left, index) == td(matches[i].right, index)) {
                    // The wildcard in the left block is already mapped appropriately
                    --left_adjustment;
                  } else {
                    // No longer unitable!
                    right_adjustment = -1;
                    break;
                  }
                }

                if (right_adjustment != -1) {
                  matches[i].adjust(left_adjustment, right_adjustment);

                  // Assure that matches[0] contains best match
                  if (insert_pos==0  ||  matches[0].cost <= matches[i].cost) {
                    matches[insert_pos++] = matches[i];
                  } else {
                    assert((uint)insert_pos < i);
                    matches[insert_pos++] = matches[0];
                    matches[0] = matches[i];
                  }
                }
              }

              matches.resize(insert_pos);
            }

            //big_output << r << " - " << rec_calls.size() << " remaining!\n";
            last_low_value = r.a.low;
          }

          rec(r);
        }

        { // Copy the changes back to the other blocks

          // Make all index_ref direct
          bool progress = true;
          while (progress) {
            progress = false;
            for (int i=0; i<num_blocks; i++) {
              int i1 = ht.get_ref(i);
              int i2 = ht.get_ref(i1);
              if (i1 != i2) {
                ht.set_ref(i, i2);
                progress= true;
              }
            }
          }

          // Do the copying
          for (int i=0; i<num_blocks; i++) {
            int index = ht.get_ref(i);
            if (i != index) {
              uint8_t *copy_to = &(table[i << log_block_size]);
              uint8_t *copy_from = &(table[index << log_block_size]);
              for (int j=0; j<block_size; j++) {
                assert(!(*copy_to  &&  *copy_to != *copy_from));
                *copy_to++ = *copy_from++;
              }
            }
          }
        }


        cbo << "Number of different blocks is " << num_different_blocks << "\n";

      }
    }
  }

  // The solution for the last layer with blocks of size 1 is trivial:
  for (int i=0; i<size; i++)
    if (!table[i])
      table[i] = non_zero_value;


  // Clean up
  table = 0;
  block_size = 0;
  blocks.clear();
  last_low_value = 0;
}
#undef t
#undef td


void test_wildcard_mapping2() {
  // Se report_stuff/test_preprocessing.txt
  { // test _hash_table
    uint8_t table[128]=
    {   0, 1, 0, 2, // 0
        0, 3, 0, 4, // 1
        0, 5, 0, 6, // 2
        0, 7, 0, 8, // 3
        1, 0, 2, 0, // 4
        3, 0, 4, 0, // 5
        5, 0, 6, 0, // 6
        7, 0, 8, 0, // 7
        1, 1, 2, 2, // 8
        3, 3, 4, 4, // 9
        1, 1, 1, 1, // 10
        1, 1, 1, 1, // 11
        1, 1, 1, 1, // 12
        1, 1, 1, 1, // 13
        1, 1, 2, 2, // 14
        3, 3, 4, 4, // 15
        3, 2, 0, 0, // 16
        3, 2, 0, 0, // 17
        0, 0, 2, 3, // 18
        0, 0, 2, 3, // 19
        3, 2, 1, 1, // 20
        1, 1, 1, 1, // 21
        1, 1, 1, 1, // 22
        1, 1, 2, 3, // 23
        3, 2, 0, 0, // 24
        0, 0, 2, 3, // 25
        3, 2, 0, 0, // 26
        0, 0, 2, 3, // 27
        1, 1, 1, 1, // 28
        1, 1, 2, 3, // 29
        3, 2, 1, 1, // 30
        1, 1, 1, 1  // 31
    };
    _hash_table ht;
    ht.init(table, 32, 2); // 128 = 32*2^2, 32 blocks of size 2^2

    cerr << "ht.get_ref(4) = " << ht.get_ref(4) << "\n";
    ht.set_ref(4, 0);
    cerr << "ht.set_ref(4, 0);\n";
    cerr << "ht.get_ref(4) = " << ht.get_ref(4) << "\n";
    cerr << "ht.get_ref(24) = " << ht.get_ref(24) << "\n";
    ht.set_ref(26, 26);
    cerr << "ht.set_ref(26, 26);\n";
    cerr << "ht.get_ref(24) = " << ht.get_ref(24) << "\n";

  }

  return;

  {
    uint8_t table[128]=
    {   0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8,
        1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0,
        1, 1, 2, 2, 3, 3, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4,
        3, 2, 0, 0, 3, 2, 0, 0, 0, 0, 2, 3, 0, 0, 2, 3,
        3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3,
        3, 2, 0, 0, 0, 0, 2, 3, 3, 2, 0, 0, 0, 0, 2, 3,
        1, 1, 1, 1, 1, 1, 2, 3, 3, 2, 1, 1, 1, 1, 1, 1
    };

    map_wildcards(table, 7);
    for (int i=0; i<128; i++) {
      cout << (int)table[i] << ' ';
      if ((i&15)==15) cout << '\n';
    }
  }

  {
    uint8_t table[256]=
    {   0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8,
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
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };


    map_wildcards(table, 8);
    for (int i=0; i<256; i++) {
      cout << (int)table[i] << ' ';
      if ((i&15)==15) cout << '\n';
    }
  }
}
