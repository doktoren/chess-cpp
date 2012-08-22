#include "endgame_database.hxx"
#include "help_functions.hxx"

//#################################
//#####    CompressedList    ######
//#################################

inline void CompressedList<uint>::init(uint *table, int _size, uint max_value) {
  size = _size;
  bits_per_value = floor_log(max_value)+1;
  int alloc_size = (31 + size*bits_per_value)>>5;
  mem = new uint[alloc_size];
  memset(mem, 0, 4*alloc_size);
  
  for (int i=0; i<size; i++)
    update(i, table[i]);
  
  for (int i=0; i<size; i++)
    assert((*this)[i] == table[i]);
}

CompressedList::CompressedList(uint *table, int _size, uint max_value)  {
  init(table, _size, max_value);
}

uint CompressedList::operator[](int index) {
  int offset = index*bits_per_value;
  int tmp = offset&31;
  uint result = mem[offset>>5] >> tmp;
  if (tmp+bits_per_value > 32) {
    return ((mem[(offset>>5)+1]<<(32-tmp)) | result) & ((1<<bits_per_value)-1);
  } else {
    return result & ((1<<bits_per_value)-1);
  }
}

void CompressedList::update(int index, uint value) {
  int offset = index*bits_per_value;
  int tmp = offset&31;
  //uint result = mem[offset>>5] >> tmp;
  if (tmp+bits_per_value > 32) {
    //assert((offset>>5)+1 < size);
    mem[offset>>5] &= (1<<tmp)-1;
    mem[offset>>5] |= value<<tmp;
    mem[(offset>>5)+1] &= -(1<<(bits_per_value + tmp - 32));
    mem[(offset>>5)+1] |= value>>(32-tmp);
  } else {
    //assert(offset>>5 < size);
    mem[offset>>5] &= ~(((1<<bits_per_value)-1) << tmp);
    mem[offset>>5] |= value << tmp;
  }
}

int CompressedList::memory_consumption() {
  return sizeof(CompressedList) + 4*((31 + size*bits_per_value)>>5);
}

//########################################
//#####    BinaryDecisionDiagram    ######
//########################################


BinaryDecisionDiagram::BinaryDecisionDiagram() :
  nodes(0) {}

BinaryDecisionDiagram::~BinaryDecisionDiagram() {
  if (nodes) {
    for (int i=0; i<log_size; i++) nodes[i].clear();
    delete[] nodes;
  }
}

void BinaryDecisionDiagram::init(uint *table, int _log_size, uint max_value) {
  log_size = _log_size;
  size = 1 << log_size;

  uint **_nodes = new uint *[log_size];
  int *node_index = new int[log_size];
  for (int i=0; i<log_size; i++) {
    _nodes[i] = new uint[1<<(log_size-i)];
    node_index[i] = 0;
  }

  map<pair<int, int>, int> mapping;
  int *_table = new int[size>>1];

  for (int i=0; i<size; i+=2) {
    pair<int, int> tmp(table[i], table[i+1]);
    if (mapping.count(tmp)) {
      _table[i>>1] = mapping[tmp];
    } else {
      // cerr << "Level 0: New element = (" << tmp.first << ", " << tmp.second << ")\n";
      mapping[tmp] = node_index[0];
      _table[i>>1] = node_index[0];
      //assert(node_index[0]<<1 < 1<<log_size);
      _nodes[0][node_index[0]<<1] = tmp.first;
      _nodes[0][(node_index[0]<<1)+1] = tmp.second;
      ++node_index[0];
    }
  }

  mapping.clear();

  for (int level=1; level<log_size; level++) {
    int _size = size>>level;

    for (int i=0; i<_size; i+=2) {
      pair<int, int> tmp(_table[i], _table[i+1]);
      if (mapping.count(tmp)) {
	_table[i>>1] = mapping[tmp];
      } else {
	// cerr << "Level " << level << ": New element = (" << tmp.first << ", " << tmp.second << ")\n";
	mapping[tmp] = node_index[level];
	_table[i>>1] = node_index[level];
	//assert(node_index[level]<<1 < 1<<(log_size-level));
	_nodes[level][node_index[level]<<1] = tmp.first;
	_nodes[level][(node_index[level]<<1)+1] = tmp.second;
	++node_index[level];
      }
    }
    mapping.clear();
  }

  // trim away the fat
  nodes = new CompressedList[log_size];
  for (int i=0; i<log_size; i++) {
    nodes[i].init(_nodes[i], 2*node_index[i], i ? node_index[i-1] : max_value);
    delete[] _nodes[i];
  }
  delete[] _nodes;

  cerr << "Tadaa!\n";
}


int BinaryDecisionDiagram::memory_consumption() {
  int result = sizeof(BinaryDecisionDiagram);
  //cerr << "nodes = " << nodes << "\n";
  for (int i=0; i<log_size; i++) {
    result += nodes[i].memory_consumption();
  }
  return result;
}

void BinaryDecisionDiagram::print() {
  cerr << "Sizeof(BinaryDecisionDiagram) = " << memory_consumption() << '\n';
  return;
  for (int i=log_size-1; i>=0; i--) {
    cerr << "Level " << i << ":";
    for (int j=0; j<nodes[i].get_size() >> 1; j++) {
      cerr << '(' << nodes[i][j<<1] << ',' << nodes[i][(j<<1)+1] << "),";
    }
    cerr << '\n';
  }
}


uint BinaryDecisionDiagram::operator[](int index) {
  int i=0;
  for (int level=log_size-1; level>=0; level--) {
    i = nodes[level][(i<<1) + ((index>>level)&1)];
  }

  return i;
}

