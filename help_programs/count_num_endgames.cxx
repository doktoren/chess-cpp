#include <iostream>

int D(int num, int diff) {
  if (num==0) return 1;
  if (num==1) return diff;
  if (diff==1) return 1;
  int result = 0;
  for (int i=1; i<=diff; i++)
    result+=D(num-1,i);
  return result;
}

int num_endgames(int n) {
  if (n&1) {
    int result = 0;
    for (int right=0; right*2+2<n; right++)
      result += D(n-2-right, 5)*D(right, 5);
    return result;
  } else {
    // K+n vs K+n
    int result = D(n/2-1, 5);
    result = (result*result + result)/2;
    for (int right=0; right*2+2<n; right++)
      result += D(n-2-right, 5)*D(right, 5);
    return result;
  }
}

int main() {
  for (int i=2; i<=10; i++)
    std::cout << "Number of different " << i << "-men endgames = " << num_endgames(i) << "\n";
}
