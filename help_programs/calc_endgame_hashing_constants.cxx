// g++ -O3 -o test calc_endgame_hashing_constants.cxx

using namespace std;

#include <assert.h>
#include <iostream>

// NUM: num pieces besides king
// #define NUM 3

int l[10];
bool *used;

void paint(int offset, int count, int n) {
  for (int i=0; i<count; i++)
    used[offset + l[i]] = true;

  if (n==1) return;

  for (int i=0; i<count; i++)
    paint(offset + l[i], count, n-1);
}

bool accepted(int offset, int count, int n) {
  for (int i=0; i<count; i++)
    if (used[offset + l[i]]) return false;

  if (n==1) return true;

  for (int i=0; i<count; i++)
    if (!accepted(offset + l[i], count, n-1)) return false;

  return true;
}

/*
void paint(int offset, int count, int n, bool color) {
  for (int i=0; i<count; i++)
    used[offset + l[i]] = color;

  if (n==1) return;

  for (int i=0; i<count; i++)
    paint(offset + l[i], count, n-1, color);
}

bool accepted(int offset, int count, int n) {
  for (int i=0; i<count; i++) {
    if (used[offset + l[i]]) {
      for (int j=0; j<i; j++)
	used[offset + l[i]] = false;
      return false;
    }
    used[offset + l[i]] = true;
  }

  if (n==1) return true;

  for (int i=0; i<count; i++) {
    if (!accepted(offset + l[i], count, n-1)) {
      for (int j=0; j<i; j++)
	paint(offset + l[i], count, n-1, false);
      return false;
    }
    paint(offset + l[i], count, n-1, true);
  }

  return true;
}
*/
void print() {
  for (int i=0; i<10; i++)
    cout << i << " =\t" << l[i] << '\n';
  cout << "\n";
}

bool test(int NUM, int sum, int index) {
  if (NUM==0) {
    bool r = used[sum];
    used[sum] = false;
    return r;
  }
  if (index==9) {
    bool r = used[sum + NUM*l[index]];
    used[sum + NUM*l[index]] = false;
    if (!r) cerr << NUM << ' ';
    return r;
  }

  for (int i=0; i<=NUM; i++)
    if (!test(NUM-i, sum+i*l[index], index+1)) {
      cerr << i << ' ';
      return false;
    }

  return true;
}

void calc(int NUM) {
  used = new bool[0x1000000];
  memset(used, 0, 0x1000000);
  int current = 0;
  for (int i=0; i<10; i++) {
    do {
      l[i] = ++current;
    } while (used[l[i]]  ||  !accepted(l[i], i+1, NUM-1));
    paint(l[i], i+1, NUM-1);
  }

  if (!test(NUM, 0, 0)) {
    assert(0);
  }
  delete used;
}

int main() {
  calc(2);
  cout << "max 4 pieces left:\n";
  print();
  calc(3);
  cout << "max 5 pieces left:\n";
  print();
  calc(4);
  cout << "max 6 pieces left:\n";
  print();
  calc(5);
  cout << "max 7 pieces left:\n";
  print();
  calc(6);
  cout << "max 8 pieces left:\n";
  print();
  calc(7);
  cout << "max 9 pieces left:\n";
  print();
}


/*
max 4 pieces left:
0 =     1
1 =     3
2 =     7
3 =     12
4 =     20
5 =     30
6 =     44
7 =     65
8 =     80
9 =     96

max 5 pieces left:
0 =     1
1 =     4
2 =     13
3 =     32
4 =     71
5 =     124
6 =     218
7 =     375
8 =     572
9 =     744

max 6 pieces left:
0 =     1
1 =     5
2 =     21
3 =     55
4 =     153
5 =     368
6 =     856
7 =     1424
8 =     2603
9 =     4967

max 7 pieces left:
0 =     1
1 =     6
2 =     31
3 =     108
4 =     366
5 =     926
6 =     2286
7 =     5733
8 =     12905
9 =     27316

max 8 pieces left:
0 =     1
1 =     7
2 =     43
3 =     154
4 =     668
5 =     2214
6 =     6876
7 =     16864
8 =     41970
9 =     94710

max 9 pieces left:
0 =     1
1 =     8
2 =     57
3 =     256
4 =     1153
5 =     4181
6 =     14180
7 =     47381
8 =     115267
9 =     307214
*/
