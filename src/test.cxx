// g++ -O3 -Wall -pedantic -ansi test.cxx

#include <iostream>
#include <fstream>
#include <queue>

#include <fcntl.h>
#include <time.h>

using namespace std;

typedef unsigned char uchar;

int foo(int (&inv_bit_perm)[5][64], int i1, int i2) {
  inv_bit_perm[0][0] = 1;
  return inv_bit_perm[i1][i2];
}

struct gah{
union {
  ushort cluster_value[5][64];// Do NOT replace 5 with MAX_MEN
  int gaaaag;
} clustering;
};


class test1 {
public:
  int n1;
};

class test2 {
public:
  int n2;
};

template<class TEST>
class test : public TEST {
  int n;
};





char *p;

class CompressedList {
public:
  void* operator new(size_t size) { return (void *)p; }
  void operator delete(void *p, size_t size) {}

  CompressedList() {}

  void init(int fd) {
    read(fd, &bits_per_value, sizeof(int));
    read(fd, &_size, sizeof(int));
    int alloc_size = 4*((31 + _size*bits_per_value)>>5);
    read(fd, mem(), alloc_size);

    p += 2*4 + alloc_size;
  }

  uint operator[](int index) {
    int offset = index*bits_per_value;
    int tmp = offset&31;
    uint result = mem()[offset>>5] >> tmp;
    if (tmp+bits_per_value > 32) {
      return ((mem()[(offset>>5)+1]<<(32-tmp)) | result) & ((1<<bits_per_value)-1);
    } else {
      return result & ((1<<bits_per_value)-1);
    }
  }

  template<class OSTREAM>
  void print(OSTREAM &os);

  int memory_consumption();

  uint size() { return _size; }
private:
  int bits_per_value;
  uint _size;
  uint *mem() { return &_size + 1; }
  // Private to prevent copying:
  CompressedList(const CompressedList&);
  CompressedList& operator=(const CompressedList&);
};

class BinaryDecisionDiagram {
public:
  void* operator new(size_t size) { return (void *)p; }
  void operator delete(void *p, size_t size) {}

  void init(int fd) {
    read(fd, &log_size, 4);
    read(fd, index_split(), 4*log_size);
    p += 4 + 4*log_size + 4*log_size;
    for (int i=0; i<log_size; i++) {
      nodes()[i] = new CompressedList();
      nodes()[i]->init(fd);
    }
  }

  uint operator[](int index) {
    uint i=0;
    CompressedList *n = nodes()[log_size];
    for (int layer=log_size-1; layer>=0; layer--) {
      --n;
      int next_bit = (index>>layer)&1;
      if (i >= index_split()[layer]) {
	i = n[layer][index_split()[layer] + i] + next_bit;
      } else {
	i = n[layer][(i<<1) | next_bit];
      }
    }
    return i;
  }
protected:
  int log_size;

  uint *index_split() { return (uint *)(&log_size + 1); }
  CompressedList **nodes() { return (CompressedList **)(&log_size + 1 + log_size); }

  // Private to prevent copying:
  BinaryDecisionDiagram(const BinaryDecisionDiagram&);
  BinaryDecisionDiagram& operator=(const BinaryDecisionDiagram&);
};







int main() {

  /*			
    int offset = index*bits_per_value;
    int tmp = offset&31;
    offset >>= 5;
    uint result = mem[offset] >> tmp;
    if (tmp+bits_per_value > 32) {
      return ((mem[offset+1]<<(32-tmp)) | result) & NUM_1_BITS[bits_per_value];
    } else {
      return result & NUM_1_BITS[bits_per_value];
    }
  */


  int n = 424242;
  int s = 3*32+13;
  cerr << (n>>(s&31)) << " " << (n>>s) << "\n";
  cerr << (n<<(s&31)) << " " << (n<<s) << "\n";




  exit(0);







  int mult = 64;
  char _map_values[256];
  char *map_values = &(_map_values[128]);
  for (int i=-128; i<-123; i++)
    map_values[i] = i;
  for (int i=-123; i<=0; i++)
    map_values[i] = mult ? mult*(-((-i + (mult-1))/mult)) : -124;
  for (int i=1; i<128; i++)
    map_values[i] = mult ? mult*((i + (mult-1))/mult) : -126;
  for (int i=-128; i<128; i++)
    cerr << "(" << i << "," << (int)map_values[i] << ")";
  cerr << "\n";
  exit(0);


  int adf = 0;
  uchar *ahs = (uchar *)malloc(adf);
  cerr << (int)ahs << " " << adf << "\n";


  int lll=0;
  time_t start_clock_time = time(NULL);
  for (int i=0; i<800111222; i++)
    lll += i;
  double d = difftime(time(NULL), start_clock_time);
  cerr << lll << ", time = " << d << "\n";
  exit(0);

  uchar l[32] = {1,0,0,0,2,0,0,0,3,0,0,0,4,0,0,0,5,0,0,0,6,0,0,0,7,0,0,0,8,0,0,0};
  cerr << ((uint *)l)[4] << "\n";

  cerr << (4*2/4) << "\n";

  exit(0);



  




  vector<int> v(10);
  for (int i=0; i<10; i++) v[i]=i;
  v.resize(20);
  for (int i=0; i<10; i++) cerr << v[i] << " ";
  cerr << "\n";

  for (int i=0; i<40; i++)
    cerr << (1 << i) << "\n";

  priority_queue<int> pq;
  pq.push(24);
  pq.push(42);
  cerr << pq.top() << "\n";

  uchar table[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

  cerr << *((int *)(&(table[2]))) << " " << *((int *)(&(table[4]))) << "\n";

  int &ref1 = *((int *)(&(table[4])));
  int &ref2 = *((int *)(&(table[2])));
  ref1 = ref2;

  cerr << *((int *)(&(table[2]))) << " " << *((int *)(&(table[4]))) << "\n";

  for (int i=0; i<16; i++) cerr << (int)table[i] << " ";
  cerr << "\n";
  

  return 0;
}


