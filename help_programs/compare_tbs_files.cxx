// g++ compare_tbs_files.cxx -o compare_tbs_files

#include <iostream>

using namespace std;

int main() {
  cout << "First give Nalimov .tbs, then <>, then number from program.\n";

  int win[2][2][300];
  int lose[2][2][300];
  int draw[2][2];

  for (int i=0; i<2; i++) for (int j=0; j<2; j++) {
    draw[i][j] = 0;
    for (int k=0; k<300; k++)
      win[i][j][k] = lose[i][j][k] = 0;
  }

  string s;
  cin >> s;
  
  int format = 0;
  int player = 0;

  while (s != ""  &&  s != "Total:") {
    //    cerr << "Hej ho: " << s << "\n";

    if (s == "wtm:" || s == "btm:") {
      player = s == "btm";
      cin >> s;
      if (s == "Mate"  ||  s == "Lost") {
	bool won = s=="Mate";
	cin >> s;cin >> s;
	int n=0;
	for (int i=0; s[i]!=':'; i++) n = 10*n + (s[i]-'0');
	int num;
	cin >> num;

	if (won) win[format][player][n] = num;
	else lose[format][player][n] = num;

      } else if (s == "Draws:") {
	// Draws:

	int num;
	cin >> num;
	draw[format][player] = num;
      }

    } else if (s == "<>") {
      ++format;
      if (format == 2) break;
    }

    s = "";
    cin >> s;
  }
  

  int c0 = 0, c1 = 0;
  

  for (int i=0; i<2; i++) {
    c0 += draw[0][i]; c1 += draw[1][i];
    if (draw[0][i] != draw[1][i])
      cout << "draw: " << draw[0][i] << " != " << draw[1][i] << ", diff = "
	   << (draw[0][i] - draw[1][i]) << "\n";
    for (int j=0; j<300; j++) {
      c0 += win[0][i][j]; c1 += win[1][i][j];
      if (win[0][i][j] != win[1][i][j])
	cout << "win(" << j << "): " << win[0][i][j] << " != " << win[1][i][j] << ", diff = "
	     << (win[0][i][j] - win[1][i][j]) << "\n";
      
      c0 += lose[0][i][j]; c1 += lose[1][i][j];
      if (lose[0][i][j] != lose[1][i][j])
	cout << "lose(" << j << "): " << lose[0][i][j] << " != " << lose[1][i][j] << ", diff = "
	     << (lose[0][i][j] - lose[1][i][j]) << "\n";
    }
  }

  if (c0==c1) {
    cout << "Congratulations! The numbers are identical!\n";
  } else {
    cout << "Total: " << c0 << " versus " << c1 << ", diff = " << c0-c1 << "\n";
  }

  return 0;
}
