#ifndef _BINARY_DECISION_TREE_
#define _BINARY_DECISION_TREE_

#include "../typedefs.hxx"

#include <map>
#include <queue>
#include <assert.h>

class BitList {
 public:
  BitList() : _size(0), mem(0) {};
  BitList(uint size, bool fill_value = false);
  BitList(uint size, bool *table);
  void init(uint size, bool fill_value = false);
  void init(uint size, bool *table);
  
  ~BitList() { clear(); }
  void clear() { if (mem) { delete mem; mem = 0; } }

  void save(int fd);
  void load(int fd);

  bool operator[](int index) const {
    return (mem[index >> 5] >> (index & 31)) & 1;
  }
  void update(int index, bool value) {
    if (value) mem[index >> 5] |= 1<<(index & 31);
    else mem[index >> 5] &= ~(1<<(index & 31));
  }
  void set(int index) {
    mem[index >> 5] |= 1<<(index & 31);
  }
  void reset(int index) {
    mem[index >> 5] &= ~(1<<(index & 31));
  }

  int memory_consumption();

  uint size() const { return _size; }

  template<class OSTREAM>
  void print(OSTREAM &os);

  int count_on() const {
    const uchar bit_count[16] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
    int max = (_size+31)>>5;
    int result = 0;
    for (int i=0; i<max; i++) {
      for (int j=0; j<32; j+= 4) 
	result += bit_count[(mem[i] >> j) & 15];
    }
    return result;
  }

  BitList(const BitList& bl);
  BitList& operator=(const BitList& bl);
private:
  uint _size;
  uint *mem;
};

const int ONE_BITS[33] =
  {0x0,
   0x1, 0x3, 0x7, 0xF,
   0x1F, 0x3F, 0x7F, 0xFF,
   0x1FF, 0x3FF, 0x7FF, 0xFFF,
   0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
   0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
   0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
   0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
   0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF};

class CompressedList {
public:
  CompressedList() : mem(0) {};
  CompressedList(uint size, uint max_value);
  CompressedList(uint size, uint *table, uint max_value);
  void init(uint size, uint max_value, uint fill_value = 0);
  void init(uint size, uint *table, uint max_value);

  ~CompressedList() { clear(); }
  void clear() { if (mem) { delete mem; mem = 0; } }

  void save(int fd);
  void load(int fd);

  uint operator[](int index) {
    index *= bits_per_value;
    return ONE_BITS[bits_per_value] &
      (*((uint *)(&(((uchar *)mem)[index >> 3]))) >> (index & 7));
  }
  void update(int index, uint value) {
#ifndef NDEBUG
    if (!(0<=value  &&  (int)value<(1<<bits_per_value)))
      cerr << "index = " << index << ", value = " << value << ", btv = " << bits_per_value << "\n";
    assert(0<=value  &&  (int)value<(1<<bits_per_value));
#endif
    index *= bits_per_value;
    uint &tmp = *((uint *)(&(((uchar *)mem)[index >> 3])));
    
    tmp &= ~(ONE_BITS[bits_per_value] << (index & 7));
    tmp |= value << (index & 7);
  }

  template<class OSTREAM>
  void print(OSTREAM &os);

  int memory_consumption();

  uint size() { return _size; }
private:
  int bits_per_value;
  uint _size;
  uint *mem;
  // Private to prevent copying:
  CompressedList(const CompressedList&);
  CompressedList& operator=(const CompressedList&);
};


class BinaryDecisionDiagram {
public:
  BinaryDecisionDiagram();
  ~BinaryDecisionDiagram();

  void test_preprocessing();

  // If calc_sifting = true, then the resulting bit permutation is stored in bit_perm
  // (must be a pointer to uchar[32]).
  // If calc_sifting = false and bit_perm != 0 (and not identity mapping),
  // then table is stored as if bit_perm had been applied to it.
  // compress_the_representation is ignored if compiled without #define BDD_USE_COMPRESSED_ENCODING 1
  template<class TYPE> void init(const TYPE *table, int _log_size, uint max_value,
				 bool compress_the_representation,
				 bool calc_sifting = false, uchar *bit_perm = 0);
  void clear();

  void save(int fd);
  void load(int fd);

  int memory_consumption();
  void print(ostream &os);

  inline uint operator[](int index);
protected:
  int log_size;
  CompressedList *nodes;

#if BDD_USE_COMPRESSED_ENCODING==1
  uint *index_split;
#endif

  // Private to prevent copying:
  BinaryDecisionDiagram(const BinaryDecisionDiagram&);
  BinaryDecisionDiagram& operator=(const BinaryDecisionDiagram&);
};



#if BDD_USE_COMPRESSED_ENCODING==1
inline uint BinaryDecisionDiagram::operator[](int index) {
  uint i=0;
  for (int layer=log_size-1; layer>=0; layer--) {
    int next_bit = (index>>layer)&1;
    if (i >= index_split[layer]) {
      assert(0<=index_split[layer]+i  &&  index_split[layer]+i<nodes[layer].size());
      i = nodes[layer][index_split[layer] + i] + next_bit;
    } else {
      assert(0<=((i<<1) | next_bit)  &&  ((i<<1) | next_bit)<nodes[layer].size());
      i = nodes[layer][(i<<1) | next_bit];
    }
  }
  return i;
}
#else
inline uint BinaryDecisionDiagram::operator[](int index) {
  int i=0;
  for (int layer=log_size-1; layer>=0; layer--) {
    int next_bit = (index>>layer)&1;
    assert(0<=((i<<1) | next_bit)  &&  ((i<<1) | next_bit)<nodes[layer].size());
    i = nodes[layer][(i<<1) | next_bit];
  }
  return i;
}
#endif



void test_binary_decision_diagram();



// TEMPLATE IMPLEMENTATIONS:


template<class OSTREAM>
void BitList::print(OSTREAM &os) {
  os << "BitList contents (size = " << _size << ")\n";
  for (uint i=0; i<_size; i++)
    if ((*this)[i]) os << 'T';
    else os << 'F';
  os << '\n';
}

template <class OSTREAM>
void CompressedList::print(OSTREAM &os) {
  os << "CompressedList contents (size = " << _size << ")\n";
  for (uint i=0; i<_size; i++)
    os << (*this)[i] << ' ';
  os << '\n';
}





#endif
