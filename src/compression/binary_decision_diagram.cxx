#include "binary_decision_diagram.hxx"

#include "../endgames/endgame_database.hxx"
#include "../util/help_functions.hxx"
#include "../streams.hxx"

// Only used if BDD_USE_COMPRESSED_ENCODING==1
#include "bdd_compression.hxx"

//#include "algorithm_best_permutation.hxx"

#include <assert.h>

template <class TYPE>
void verify_permutation(const vector<TYPE> &perm) {
  vector<bool> used(perm.size());
  for (uint i=0; i<perm.size(); i++) {
    assert(0<=perm[i]  &&  (uint)perm[i]<perm.size());
    assert(!used[perm[i]]);
    used[perm[i]] = true;
  }
}



//#################################
//#####        BitList       ######
//#################################


void BitList::init(uint __size, bool fill_value) {
  clear();

  _size = __size;
  int alloc_size = (31 + _size)>>5;
  mem = new uint[alloc_size];
  memset(mem, 0, 4*alloc_size);

  uint _fill_value = fill_value ? 0xFFFFFFFF : 0;
  for (int i=0; i<alloc_size; i++)
    mem[i] = _fill_value;

  //for (int i=0; i<_size; i++) update(i, fill_value);
}

void BitList::init(uint __size, bool *table) {
  clear();

  _size = __size;
  int alloc_size = (31 + _size)>>5;
  mem = new uint[alloc_size];
  memset(mem, 0, 4*alloc_size);

  for (uint i=0; i<_size; i++)
    update(i, table[i]);
}

BitList::BitList(uint __size, bool fill_value) : mem(0) {
  init(__size, fill_value);
}

BitList::BitList(uint __size, bool *table) : mem(0) {
  init(__size, table);
}

void BitList::save(int fd) {
  write(fd, &_size, sizeof(int));
  write(fd, mem, 4*((31 + _size)>>5));
}
void BitList::load(int fd) {
  clear();
  read(fd, &_size, sizeof(int));
  int alloc_size = 4*((31 + _size)>>5);
  mem = new uint[alloc_size];
  read(fd, mem, alloc_size);
}

int BitList::memory_consumption() {
  return sizeof(BitList) + 4*((31 + _size)>>5);
}

BitList::BitList(const BitList& bl) : _size(bl.size()) {
  int alloc_size = 4*((31 + _size)>>5);
  mem = new uint[alloc_size];
  memcpy(mem, bl.mem, alloc_size);
}
BitList& BitList::operator=(const BitList& bl) {
  clear();
  _size = bl.size();
  int alloc_size = 4*((31 + _size)>>5);
  mem = new uint[alloc_size];
  memcpy(mem, bl.mem, alloc_size);
  return *this;
}

//#################################
//#####    CompressedList    ######
//#################################


void CompressedList::init(uint __size, uint max_value, uint fill_value) {
  clear();

  if (max_value >= (1<<26)) {
    cerr << "CompressedList: Error: No support for values more than 26 bits long.!\n"
        << "(it would result in a significant loss of efficienfy)\n";
    exit(1);
  }

  _size = __size;
  bits_per_value = floor_log(max_value)+1;
  int alloc_size = (31 + _size*bits_per_value)>>5;
  mem = new uint[alloc_size];
  memset(mem, 0, 4*alloc_size);

  for (uint i=0; i<_size; i++)
    update(i, fill_value);
#ifndef NDEBUG
  assert(fill_value <= max_value);
  for (uint i=0; i<_size; i++)
    assert((*this)[i] == fill_value);
#endif
}

void CompressedList::init(uint __size, uint *table, uint max_value) {
  clear();

  _size = __size;
  bits_per_value = floor_log(max_value)+1;
  int alloc_size = (31 + _size*bits_per_value)>>5;
  mem = new uint[alloc_size];
  memset(mem, 0, 4*alloc_size);

  for (uint i=0; i<_size; i++)
    update(i, table[i]);
#ifndef NDEBUG
  for (uint i=0; i<_size; i++) {
    if (table[i] > max_value) {
      cerr << "table[" << i << "] = " << table[i] << " > " << max_value << " = max_value\n";
      assert(0);
    }
    if ((*this)[i] != table[i]) {
      cerr << (*this)[i] << " = (*this)[" << i << "] != table[" << i << "] = " << table[i] << "\n";
      assert(0);
    }
  }
#endif
}

CompressedList::CompressedList(uint __size, uint max_value) : mem(0) {
  init(__size, max_value);
}

CompressedList::CompressedList(uint __size, uint *table, uint max_value) : mem(0) {
  init(__size, table, max_value);
}

void CompressedList::save(int fd) {
  write(fd, &bits_per_value, sizeof(int));
  write(fd, &_size, sizeof(int));
  write(fd, mem, 4*((31 + _size*bits_per_value)>>5));
}
void CompressedList::load(int fd) {
  clear();
  read(fd, &bits_per_value, sizeof(int));
  read(fd, &_size, sizeof(int));
  int alloc_size = 4*((31 + _size*bits_per_value)>>5);
  mem = new uint[alloc_size];
  read(fd, mem, alloc_size);
}

int CompressedList::memory_consumption() {
  return sizeof(CompressedList) + 4*((31 + _size*bits_per_value)>>5);
}

//########################################
//#####    BinaryDecisionDiagram    ######
//########################################

BinaryDecisionDiagram::BinaryDecisionDiagram() : nodes(0) {
#if BDD_USE_COMPRESSED_ENCODING==1
  index_split = 0;
#endif
}

BinaryDecisionDiagram::~BinaryDecisionDiagram() {
  clear();
}

void BinaryDecisionDiagram::clear() {
  if (nodes) {
    for (int i=0; i<log_size; i++) nodes[i].clear();
    delete[] nodes;
    nodes = 0;
#if BDD_USE_COMPRESSED_ENCODING==1
    delete index_split;
    index_split = 0;
#endif 
  }
}

void BinaryDecisionDiagram::save(int fd) {
  save_define_value(fd, BDD_USE_COMPRESSED_ENCODING);

  write(fd, &log_size, sizeof(int));
  for (int i=0; i<log_size; i++)
    nodes[i].save(fd);

#if BDD_USE_COMPRESSED_ENCODING==1
  write(fd, index_split, 4*log_size);
#endif
}
void BinaryDecisionDiagram::load(int fd) {
  load_define_value(fd, BDD_USE_COMPRESSED_ENCODING, "BDD_USE_COMPRESSED_ENCODING");

  clear();
  read(fd, &log_size, sizeof(int));
  nodes = new CompressedList[log_size];
  for (int i=0; i<log_size; i++)
    nodes[i].load(fd);

#if BDD_USE_COMPRESSED_ENCODING==1
  index_split = new uint[log_size];
  read(fd, index_split, 4*log_size);
#endif
}

int BinaryDecisionDiagram::memory_consumption() {
  int result = sizeof(BinaryDecisionDiagram);
  //cerr << "nodes = " << nodes << "\n";
  for (int i=0; i<log_size; i++) {
    result += nodes[i].memory_consumption();
  }
  return result;
}

void BinaryDecisionDiagram::print(ostream &os) {
  os << "Sizeof(BinaryDecisionDiagram) = " << memory_consumption() << '\n';
  for (int i=log_size-1; i>=0; i--) {
#if BDD_USE_COMPRESSED_ENCODING==0
    os << "Layer " << i << ": " << (nodes[i].size()>>1) << " nodes\n\t";
    for (uint j=0; j<nodes[i].size() >> 1; j++)
      os << "(" << nodes[i][j<<1] << "," << nodes[i][(j<<1)+1] << "),";
#else
    os << "Layer " << i << ": " << (nodes[i].size() - index_split[i]) << " nodes\n\t";
    for (uint j=0; j<index_split[i]; j++)
      os << "(" << nodes[i][j<<1] << "," << nodes[i][(j<<1)+1] << "),";
    for (uint j=2*index_split[i]; j < nodes[i].size(); j++)
      os << "(" << nodes[i][j] << "),";
#endif
    os << "\n";

  }
}

// returns increase in size
// swaps layer upper_layer and (upper_layer-1)
// Remember layer 0 is root
int shift_bit_pos(vector<vector<uint> > &nodes, vector<uint> &node_index, int upper_layer) {
  // cerr << "Swap layer " << upper_layer << " and " << upper_layer-1 << "\n";

  // mapping[par] = index in nodes[upper_layer+1]
  map<pair<int, int>, int> mapping;

  int index = 0;
  for (uint i=0; i<node_index[upper_layer]; i++) {
    int i0 = nodes[upper_layer][2*i];
    int i1 = nodes[upper_layer][2*i+1];

    int i00 = nodes[upper_layer-1][2*i0];
    int i01 = nodes[upper_layer-1][2*i0+1];
    int i10 = nodes[upper_layer-1][2*i1];
    int i11 = nodes[upper_layer-1][2*i1+1];

    //        _X_                      _X_          //  upper_layer
    //       /   \                    /   \_
    //    _i0     i1_       =>     _j0     j1_      //  upper_layer - 1
    //   /  \    /   \            /  \    /   \_
    // i00  i01 i10  i11        i00  i10 i01  i11
    pair<int, int> pair0(i00, i10);
    pair<int, int> pair1(i01, i11);

    if (mapping.count(pair0)) {
      nodes[upper_layer][2*i] = mapping[pair0];
    } else {
      nodes[upper_layer][2*i] = index;
      mapping[pair0] = index;
      //nodes[upper_layer-1][2*index] = pair0.first;
      //nodes[upper_layer-1][2*index+1] = pair0.second;
      ++index;
    }
    // now mapping[pair0] == j0

    if (mapping.count(pair1)) {
      nodes[upper_layer][2*i+1] = mapping[pair1];
    } else {
      nodes[upper_layer][2*i+1] = index;
      mapping[pair1] = index;
      //nodes[upper_layer-1][2*index] = pair1.first;
      //nodes[upper_layer-1][2*index+1] = pair1.second;
      ++index;
    }
    // now mapping[pair1] == j1
  }

  // Expand nodes[upper_layes-1] if necessary
  if (2*index > (int)nodes[upper_layer-1].size()) {
    // Make 10% bigger than needed right here
    nodes[upper_layer-1].resize(((int)(2.2*index) | 1) + 3);
  }

  typedef map<pair<int, int>, int>::const_iterator CI;
  for (CI i=mapping.begin(); i!=mapping.end(); i++) {
    nodes[upper_layer-1][2*(i->second)] = i->first.first;
    nodes[upper_layer-1][2*(i->second)+1] = i->first.second;
  }

  //cerr << "Size of nodes[" << upper_layer-1 << "] changed from " << node_index[upper_layer-1] << " to " << index << "\n";

  int result = index - node_index[upper_layer-1];
  node_index[upper_layer-1] = index;

  return result;
}

// Currently not used!
int shift_to_bit_perm(vector<vector<uint> > &nodes, vector<uint> node_index,
    uint8_t *bit_perm, uint8_t *inv_bit_perm,
    uint8_t *new_bit_perm, int log_size) {
  int value_change = 0;

  for (int ii=0; ii<log_size; ii++)
    cerr << (int)bit_perm[ii] << " ";
  cerr << " -> ";
  for (int ii=0; ii<log_size; ii++)
    cerr << (int)new_bit_perm[ii] << " ";
  cerr << "\n";

  // "Bubble sort" - agtig
  for (int i=0; i<log_size; i++) {
    int pos = 0;
    while (new_bit_perm[pos] != i) pos++;
    pos = bit_perm[pos];

    // move from

    assert (pos >= i);

    {
      for (int ii=0; ii<log_size; ii++)
        cerr << (int)new_bit_perm[ii] << " ";
      cerr << "\n";
      for (int ii=0; ii<log_size; ii++)
        cerr << (int)bit_perm[ii] << " ";
      cerr << "\n" << pos << " " << i << "\n";
      assert(0);
    }

    while (pos != i) {
      value_change += shift_bit_pos(nodes, node_index, pos);
      swap(inv_bit_perm[pos], inv_bit_perm[pos-1]);
      bit_perm[inv_bit_perm[pos]] = pos;
      bit_perm[inv_bit_perm[pos-1]] = pos-1;

      pos--;
    }
  }

  return value_change;
}


template<class TYPE>
void BinaryDecisionDiagram::init(const TYPE *table, int _log_size, uint max_value,
    bool compress_the_representation,
    bool calc_sifting, uint8_t *bit_perm) {
  clear();

  log_size = _log_size;
  int size = 1<<log_size;

  uint8_t inv_bit_perm[32];
  if (!calc_sifting  &&  bit_perm) {
    // Ignore bit_perm if it is merely the identity mapping
    bool identity_mapping = true;
    for (int i=0; i<32; i++)
      if (bit_perm[i] != i) {
        identity_mapping = false;
        break;
      }
    if (identity_mapping) bit_perm = 0;

    if (bit_perm) {
#ifndef NDEBUG
      // test that bit_perm is bijective on [0..32[
      uint test = 0;
      for (int i=0; i<32; i++) {
        assert(bit_perm[i] < 32);
        test |= 1 << bit_perm[i];
      }
      assert(test == 0xFFFFFFFF);
#endif
      for (int i=0; i<32; i++) inv_bit_perm[bit_perm[i]] = i;
    }
  }

  // _nodes is the tree itself.
  // _nodes[dybde][2*index] and _nodes[dybde][2*index+1] gives a new index by bit 0 resp. bit 1
  // _nodes[dybde][0..2*node_index[dybde]-1] is used
  vector<vector<uint> > _nodes(log_size);

  vector<uint> node_index(log_size);//node_index[i] is number of used pairs in _nodes[i]
  for (int i=0; i<log_size; i++) {
    //cerr << "BinaryDecisionDiagram::init(...) allocating " << 4*(1<<(log_size-i)) << " bytes.\n";
    _nodes[i].resize(4);
    node_index[i] = 0;
  }

  // vector<ushort> _table(size>>1);
  uint16_t *_table = (uint16_t *)malloc(sizeof(uint16_t)*(size>>1));

  { // Special optimization for lowest layer
    int mapping[0x10000];
    for (int i=0; i<0x10000; i++)
      mapping[i] = -1;

    // Fill the first layer (the leaves) in the tree
    for (int i=0; i<size; i+=2) {

      pair<uint8_t, uint8_t> tmp((uint8_t)table[i], (uint8_t)table[i+1]);
      if (!calc_sifting  &&  bit_perm) {
        // Example: The entry number 27 = 2^4 + 2^3 + 2^1 + 2^0 should be mapped to the
        // entry number 2^bit_perm[4] + 2^bit_perm[3] + 2^bit_perm[1] + 2^bit_perm[0]
        // Here we are however searching for the entry which maps to i resp. i+1,
        // hence we use inv_bit_perm instead.

        // new_index is the common part of the calculation for i and i+1,
        // they differ only at log_i=0
        int new_index = 0;
        for (int log_i=1; log_i < log_size; log_i++) {
          new_index |= ((i >> log_i) & 1) << inv_bit_perm[log_i];
        }

        tmp = pair<uint8_t, uint8_t>((uint8_t)table[new_index],
            (uint8_t)table[new_index + (1 << inv_bit_perm[0])]);
      }

      int &index = mapping[(((int)tmp.first) << 8) | tmp.second];

      if (index != -1) {
        _table[i>>1] = index;
      } else {
        // cerr << "Layer 0: New element = (" << tmp.first << ", " << tmp.second << ")\n";
        index = node_index[0];
        _table[i>>1] = node_index[0];
        //assert(node_index[0]<<1 < 1<<log_size);
        if (_nodes[0].size() == (node_index[0]<<1)) _nodes[0].resize(2*_nodes[0].size());
        _nodes[0][node_index[0]<<1] = tmp.first;
        _nodes[0][(node_index[0]<<1)+1] = tmp.second;
        ++node_index[0];
      }
    }
  }

  if (calc_sifting) {
    // Allocate 20% extra space which might be needed by the sifting algorithm
    _nodes[0].resize(((int)(2.4*node_index[0]) | 1) + 3);
  } else {
    // Trim _nodes[0] as much as possible
    _nodes[0].resize(node_index[0] << 1);
  }

  // layer == 1
#define __table ((uint32_t *)_table)
  {
    int layer = 1;
    int _size = size>>layer;

    map<pair<uint16_t, uint16_t>, uint32_t> mapping;

    for (int i=0; i<_size; i+=2) {
      pair<uint16_t, uint16_t> tmp(_table[i], _table[i+1]);
      if (mapping.count(tmp)) {
        // Use 2 uint16_t entries in _table as one uint32_t entry!
        __table[i>>1] = mapping[tmp];
      } else {
        mapping[tmp] = node_index[layer];
        __table[i>>1] = node_index[layer];
        //assert(node_index[layer]<<1 < 1<<(log_size-layer));
        if (_nodes[layer].size() == (node_index[layer]<<1))
          _nodes[layer].resize(2*_nodes[layer].size());
        _nodes[layer][node_index[layer]<<1] = tmp.first;
        _nodes[layer][(node_index[layer]<<1)+1] = tmp.second;
        ++node_index[layer];
      }
    }
  }

  for (int layer=2; layer<log_size; layer++) {
    int _size = size>>layer;

    map<pair<uint32_t, uint32_t>, uint32_t> mapping;

    for (int i=0; i<_size; i+=2) {
      pair<uint, uint> tmp(__table[i], __table[i+1]);
      if (mapping.count(tmp)) {
        __table[i>>1] = mapping[tmp];
      } else {
        // cerr << "Layer " << layer << ": New element = (" << tmp.first << ", " << tmp.second << ")\n";
        mapping[tmp] = node_index[layer];
        __table[i>>1] = node_index[layer];
        //assert(node_index[layer]<<1 < 1<<(log_size-layer));
        if (_nodes[layer].size() == (node_index[layer]<<1))
          _nodes[layer].resize(2*_nodes[layer].size());
        _nodes[layer][node_index[layer]<<1] = tmp.first;
        _nodes[layer][(node_index[layer]<<1)+1] = tmp.second;
        ++node_index[layer];
      }
    }

    // Reduce size of _table to half
    realloc(_table, sizeof(uint32_t)*(_size>>1));
    assert(_table);

    if (calc_sifting) {
      // Allocate 20% extra space which might be needed by the sifting algorithm
      _nodes[layer].resize(((int)(2.4*node_index[layer]) | 1) + 3);
    } else {
      // Trim _nodes[layer] as much as possible
      _nodes[layer].resize(node_index[layer] << 1);
    }
  }

  free(__table);
#undef __table

  //#######################################################
  //##########       BEGIN CALC SIFTING      ##############
  //#######################################################
  if (calc_sifting) {
    for (int i=0; i<32; i++) {
      bit_perm[i] = i;
      inv_bit_perm[i] = i;
    }

    cbo << "Showing bit orderings for binary decision diagram (bit i, perm bit i)\n";

    bool progress = true;
    while (progress) {
      progress = false;

      for (int i=0; i<log_size; i++)
        cbo << "(" << i << "," << (int)bit_perm[i] << "),";
      cbo << "\n";

      // Run through all variables
      for (int bit_num = 0; bit_num<log_size; bit_num++) {
        // Find the placement of the bit
        int pos = bit_perm[bit_num];// corrected from inv_bit_perm

        int best_pos = pos;
        int cur_value = 0; // compared to best_pos

        // shift upwards
        while (pos < log_size-1) {
          ++pos;

          cur_value += shift_bit_pos(_nodes, node_index, pos);
          //assert(shift_bit_pos(_nodes, node_index, pos)+shift_bit_pos(_nodes, node_index, pos)==0);
          swap(inv_bit_perm[pos], inv_bit_perm[pos-1]);
          bit_perm[inv_bit_perm[pos]] = pos;
          bit_perm[inv_bit_perm[pos-1]] = pos-1;

          if (cur_value < 0) {
            progress = true;
            cbo << "Swapping " << best_pos << " to " << pos << " removes "
                << -cur_value << " node(s) :-)\n";
            cur_value = 0;
            best_pos = pos;
          }
        }

        // shift downwards
        while (pos > 0) {
          cur_value += shift_bit_pos(_nodes, node_index, pos);
          swap(inv_bit_perm[pos], inv_bit_perm[pos-1]);
          bit_perm[inv_bit_perm[pos]] = pos;
          bit_perm[inv_bit_perm[pos-1]] = pos-1;

          --pos;

          if (cur_value < 0) {
            progress = true;
            cbo << "Swapping " << best_pos << " to " << pos << " removes "
                << -cur_value << " node(s) :-)\n";
            cur_value = 0;
            best_pos = pos;
          }
        }

        //cerr << "bit_num, pos) = (" << bit_num << ", " << pos << ")\n";

        // shift upwards to best pos
        while (pos < best_pos) {
          ++pos;

          shift_bit_pos(_nodes, node_index, pos);
          swap(inv_bit_perm[pos], inv_bit_perm[pos-1]);
          bit_perm[inv_bit_perm[pos]] = pos;
          bit_perm[inv_bit_perm[pos-1]] = pos-1;
        }
      }
    }

    cbo << "Best bit ordering:\n";
    for (int i=0; i<log_size; i++)
      cbo << "(" << i << "," << (int)bit_perm[i] << "),";
    cbo << "\n";
  }

  //#######################################################
  //##########       END CALC SIFTING        ##############
  //#######################################################


#if BDD_USE_COMPRESSED_ENCODING==1

  if (compress_the_representation) {
    cerr << "Compressing the representation of the OBDD...\n";

    // do not optimize layer 0 (otherwise convert_table (only part of subclass) would
    // have to be permuted, and too little to gain.
    index_split = new uint[log_size];
    index_split[0] = node_index[0];

    for (int layer=1; layer<log_size; layer++) {

      { // First find out how the nodes in the layer below should be
        // permuted such as to give as many as possible nodes in this layer
        // the property

        //vector<int> yalla(int shuffle_size, uint *pairs, int num_pairs, int barrier = 0);
        vector<uint> permutation = yalla(node_index[layer-1], &(_nodes[layer][0]),
            node_index[layer], index_split[layer-1]);
        verify_permutation(permutation);

        // Change the indexes of the children according to this permutation
        for (uint i=0; i<node_index[layer]; i++) {
          _nodes[layer][2*i] = permutation[_nodes[layer][2*i]];
          _nodes[layer][2*i+1] = permutation[_nodes[layer][2*i+1]];
        }

        // inverse_permutation(permutation);
        // verify_permutation(permutation);

        // Rearrange the positions of the children according to this permutation.
        // This process have to be split up between the compressed and
        // uncompressed children.

        // Rearrange uncompressed children
        for (uint i=0; i<index_split[layer-1]; i++) {
          if (permutation[i] != 0xFFFFFFFF) {
            //if (i == permutation[i]) cerr << "\n\n\n\n" << i << "\n\n\n";
            // permute this cycle
            uint j = i;
            uint tmp0 = _nodes[layer-1][2*j];
            uint tmp1 = _nodes[layer-1][2*j+1];
            do {
              {
                uint last = j;
                j = permutation[j];
                permutation[last] = 0xFFFFFFFF;
              }

#ifndef NDEBUG
              if (!(0<=j  &&  j<index_split[layer-1])) {
                cerr << "j = " << j << ", index_split[layer-1] = " << index_split[layer-1]
                                                                                  << ", permutation.size() = " << permutation.size()
                                                                                  << ", layer = " << layer << "\n";
                assert(0);
              }
#endif

              swap(tmp0, _nodes[layer-1][2*j]);
              swap(tmp1, _nodes[layer-1][2*j+1]);
            } while (j!=i);
          }
        }

        // Rearrange compressed children
        for (uint i=index_split[layer-1]; i<node_index[layer-1]; i++) {
          if (permutation[i] != 0xFFFFFFFF) {
            // permute this cycle
            uint j = i;
            uint tmp = _nodes[layer-1][j + index_split[layer-1]];
            do {
              {
                int last = j;
                j = permutation[j];
                permutation[last] = 0xFFFFFFFF;
              }

              assert(index_split[layer-1]<=j  &&  j<node_index[layer-1]);

              swap(tmp, _nodes[layer-1][j + index_split[layer-1]]);
            } while (j!=i);
          }
        }
      } // Finished permuted nodes in layer below


      { // Sort the nodes in this layer so that the compressable nodes
        // are placed last.

        // Remember how the nodes in this layer are permuted, because the
        // indexes in the layer above have to be updated accordingly
        vector<uint> rearranging(node_index[layer]);
        for (uint i=0; i<rearranging.size(); i++)
          rearranging[i] = i;

        { // rearrange _nodes[layer] entries such that compressable entries are placed last
          int left = 0;
          int right = node_index[layer]-1;
          while (left <= right) {
            if (_nodes[layer][2*left] + 1 != _nodes[layer][2*left + 1]) {
              ++left;
            } else if (_nodes[layer][2*right] + 1 == _nodes[layer][2*right + 1]) {
              --right;
            } else {
              // remember this swap
              swap(rearranging[left], rearranging[right]);

              swap(_nodes[layer][2*left], _nodes[layer][2*right]);
              swap(_nodes[layer][2*left + 1], _nodes[layer][2*right + 1]);
              ++left;
              --right;
            }
          }
          assert(left-1 == right);

          // Set index_split[layer]
          index_split[layer] = left;

          if (index_split[layer] > 0) {
            assert(_nodes[layer][2*(index_split[layer]-1)] + 1 !=
                _nodes[layer][2*(index_split[layer]-1) + 1]);
          }
          if (index_split[layer] < node_index[layer]) {
            assert(_nodes[layer][2*index_split[layer]] + 1 ==
                _nodes[layer][2*index_split[layer] + 1]);
          }
        }

        // Compress these entries
        for (uint i=index_split[layer]; i<node_index[layer]; i++) {
          //cerr << "test(" << _nodes[layer][2*i] << ", " << _nodes[layer][2*i+1] << ")\n";
          _nodes[layer][index_split[layer] + i] = _nodes[layer][2*i];
        }

        { // Change the indexes of the above layer according to how the
          // nodes in this layer was moved around
          if (layer+1 < log_size) {
            for (uint i=0; i<node_index[layer+1]; i++) {
              _nodes[layer+1][2*i]   = rearranging[_nodes[layer+1][2*i]];
              _nodes[layer+1][2*i+1] = rearranging[_nodes[layer+1][2*i+1]];
            }
          }
        }
      }
    }

    // trim away the fat
    nodes = new CompressedList[log_size];
    for (int i=0; i<log_size; i++) {
      nodes[i].init(node_index[i] + index_split[i], &(_nodes[i][0]), i ? (node_index[i-1]-1) : max_value);
#ifndef NDEBUG
      for (uint j=0; j<node_index[i] + index_split[i]; j++)
        assert(_nodes[i][j] == nodes[i][j]);
#endif
      _nodes[i].clear();
    }
    _nodes.clear();
  }


  if (!compress_the_representation) {
#endif

    // This is executed if DD_USE_COMPRESSED_ENCODING!=1 or if !compress_the_representation

    // trim away the fat
    nodes = new CompressedList[log_size];
    for (int i=0; i<log_size; i++) {
      nodes[i].init(2*node_index[i], &(_nodes[i][0]), i ? (node_index[i-1]-1) : max_value);
#ifndef NDEBUG
      for (uint j=0; j<2*node_index[i]; j++)
        assert(_nodes[i][j] == nodes[i][j]);
#endif
      _nodes[i].clear();
    }
    _nodes.clear();

#if BDD_USE_COMPRESSED_ENCODING==1

    index_split = new uint[log_size];
    for (int i=0; i<log_size; i++)
      index_split[i] = node_index[i];
  }
#endif
}

template void BinaryDecisionDiagram::init(const uint8_t *, int, uint, bool, bool, uint8_t*);


void test_binary_decision_diagram() {
  uint table[32] =
  {0,1, 1,1,  0,1, 0,1,
      1,2, 1,2,  0,1, 0,1,
      1,1, 0,1,  1,2, 0,1,
      1,1, 1,1,  1,1, 1,1};

  BinaryDecisionDiagram bdd;

  bdd.init(table, 5, 2, true);

  bdd.print(cerr);

  cerr << "Testing:\n";
  for (int i=0; i<32; i++) {
    uint bdd_value = bdd[i];
    cerr << "\tbdd[" << i << "](" << bdd_value << ")==table[" << i << "](" << table[i] << ")?\n";
  }
}
