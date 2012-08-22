
  void KBBK_save_to_file();
  bool KBBK_load_from_file();
  void KBBK_compress_indeces(int &k1, int &b1a, int &b1b, int &k2);
  void KBBK_decompress_indeces(int &k1, int &b1a, int &b1b, int &k2);
  int KBBK_index(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color);
  void KBBK_update(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color, uchar new_value);
  void KBBK_test(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color);

  void KBBK_analyze();

  // return of -1 implies even
  int moves_to_mate_KBBK(Position king1, Position bishop1a,
			 Position bishop1b, Position king2);


  int database_value(vector<pair<Piece, Position> > pieces, int player_turn);


  void build_KBBK();
  void KBBK_save_perms_to_file(vector<int> k1_perm, vector<int> b1a_perm,
			       vector<int> b1b_perm, vector<int> k2_perm);
  void ___KBBK_test(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color,
		    vector<int> b1a_perm, vector<int> b1b_perm);





int _KBBK(Board2a &board) {
  // todo:
  return 0;
}

void destroy_KBBK() {}



unsigned char KBBK[10][32][32][64][2];

void KBBK_save_perms_to_file(vector<int> k1_perm, vector<int> b1a_perm,
				       vector<int> b1b_perm, vector<int> k2_perm) {
  // Oops!
  inverse_permutation(k1_perm);
  inverse_permutation(b1a_perm);
  inverse_permutation(b1b_perm);
  inverse_permutation(k2_perm);

  const int L[5] = {10, 32, 32, 64, 2};
  int index[5];
  for (int i=0; i<5; i++) index[i]=i;
  do {
    string name = "KBBK_perm_xxxxx.dat";
    for (int i=0; i<5; i++)
      name[10+i] = '0'+index[i];
    cerr << "Saving file " << name << '\n';

    ofstream out(name.c_str());
    int li[5];
    for (li[index[0]]=0; li[index[0]]<L[index[0]]; li[index[0]]++)
      for (li[index[1]]=0; li[index[1]]<L[index[1]]; li[index[1]]++)
	for (li[index[2]]=0; li[index[2]]<L[index[2]]; li[index[2]]++)
	  for (li[index[3]]=0; li[index[3]]<L[index[3]]; li[index[3]]++)
	    for (li[index[4]]=0; li[index[4]]<L[index[4]]; li[index[4]]++)
	      out << KBBK[k1_perm[li[0]]][b1a_perm[li[1]]][b1b_perm[li[2]]][k2_perm[li[3]]][li[4]];

    out.close();
    string command = "gzip " + name;
    cerr << "Executing command: " << command << '\n';
    system(command.c_str());

  } while (next_permutation(index, index+5));

}

void KBBK_save_to_file() {
  /*
  // Stream the table in every possible way
  const int L[5] = {10, 32, 32, 64, 2};
  int index[5];
  for (int i=0; i<5; i++) index[i]=i;
  do {
    string name = "KBBK_xxxxx.dat";
    for (int i=0; i<5; i++)
      name[5+i] = '0'+index[i];
    cerr << "Saving file " << name << '\n';

    ofstream out(name.c_str());
    int li[5];
    for (li[index[0]]=0; li[index[0]]<L[index[0]]; li[index[0]]++)
      for (li[index[1]]=0; li[index[1]]<L[index[1]]; li[index[1]]++)
	for (li[index[2]]=0; li[index[2]]<L[index[2]]; li[index[2]]++)
	  for (li[index[3]]=0; li[index[3]]<L[index[3]]; li[index[3]]++)
	    for (li[index[4]]=0; li[index[4]]<L[index[4]]; li[index[4]]++)
	      out << KBBK[li[0]][li[1]][li[2]][li[3]][li[4]];

  } while (next_permutation(index, index+5));
  */

  {
    // gzip can compress KBBK_43120.dat to 208731 bytes

    cerr << "Saving file KBBK_43120.dat\n";
    ofstream out("KBBK_43120.dat");
    for (int color=0; color<2; color++)
      for (int k2=0; k2<64; k2++)
	for (int b1a=0; b1a<32; b1a++)
	  for (int b1b=0; b1b<32; b1b++)
	    for (int k1=0; k1<10; k1++)
	      out.put((char)KBBK[k1][b1a][b1b][k2][color]);
    out.flush();
  }

}

bool KBBK_load_from_file() {
  ifstream in("KBBK_43120.dat");
  if (!in) return false;
  cerr << "Loading file KBBK_43120.dat\n";
  for (int color=0; color<2; color++)
    for (int k2=0; k2<64; k2++)
      for (int b1a=0; b1a<32; b1a++)
	for (int b1b=0; b1b<32; b1b++)
	  for (int k1=0; k1<10; k1++) {
	    // Saadan noget fucking boevl
	    char ch;
	    in.get(ch);
	    KBBK[k1][b1a][b1b][k2][color] = ch;

	    /* WHAT THE FUCK !?!?!?!?!
	    in.get(reinterpret_cast<char>(KBBK[k1][b1a][b1b][k2][color]));

	    end_game_database.cxx:69: error: invalid reinterpret_cast from type `unsigned 
	    char' to type `char'
	    */
	    /*
	      virker ikke:
	    in.get(static_cast<char>(KBBK[k1][b1a][b1b][k2][color]));
	    */
	    /*
	      virker ikke:
	      in.get((char)KBBK[k1][b1a][b1b][k2][color]);
	    */
	  }
  return true;
}


void ___KBBK_test(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color,
			    vector<int> b1a_perm, vector<int> b1b_perm) {
  bool a = k1_pos == ILLEGAL_POS;
  bool b = b1a_pos == ILLEGAL_POS;
  bool c = b1b_pos == ILLEGAL_POS;
  bool d = k2_pos == ILLEGAL_POS;

  cerr << "KBBK_test(" << k1_pos << ',' << b1a_pos << ',' << b1b_pos
       << ',' << k2_pos << ',' << color << ")\n";

  if (a+b+c+d != 1) {
    cerr << "KBBK_test: Exactly one must be ILLEGAL_POS\n";
    return;
  }

  uchar map64[64];

  for (int i=0; i<64; i++)
    map64[i] = 0;
  if (!a) map64[k1_pos] = 1;
  if (!b) map64[b1a_pos] = 2;
  if (!c) map64[b1b_pos] = 3;
  if (!d) map64[k2_pos] = 4;
  cerr << "Piece placements:\n";
  print_map64<uchar>(cerr, map64, 3, 10);

  for (int i=0; i<64; i++)
    map64[i] = KBBK_index(a ? i : k1_pos, b ? i : b1a_pos, c ? i : b1b_pos, d ? i : k2_pos, color);

  if (a) cerr << "King 1 positions:\n";
  if (b) cerr << "Bishop 1a positions:\n";
  if (c) cerr << "Bishop 1b positions:\n";
  if (d) cerr << "King 2 positions:\n";

  print_map64<uchar>(cerr, map64, 3, 10);


  int k1  = ((k1_pos == ILLEGAL_POS)  ? 0 : k1_pos);
  int b1a = ((b1a_pos == ILLEGAL_POS) ? 0 : b1a_pos);
  int b1b = ((b1b_pos == ILLEGAL_POS) ? 0 : b1b_pos);
  int k2  = ((k2_pos == ILLEGAL_POS)  ? 0 : k2_pos);
  KBBK_compress_indeces(k1, b1a, b1b, k2);

  cerr << '\n';
  if (a) {
    for (int j=0; j<32; j++) {
      for (int i=0; i<10; i++) cerr << (int)KBBK[i][j][b1b][k2][color] << ' ';
      cerr << '\n';
    }
  }
  if (b | c) {
    for (int j=0; j<32; j++) {
      for (int i=0; i<32; i++) cerr << (int)KBBK[k1][b1a_perm[i]][b1b_perm[j]][k2][color] << ' ';
      cerr << '\n';
    }
    cerr << "\n --- or ---\n";
    for (int j=0; j<32; j++) {
      for (int i=0; i<32; i++) cerr << (int)KBBK[k1][b1a_perm[j]][b1b_perm[i]][k2][color] << ' ';
      cerr << '\n';
    }
  }
  // if (d) for (int i=0; i<64; i++) cerr << (int)KBBK[k1][b1a][b1b][i][color] << ' ';
  cerr << '\n';
}



void KBBK_analyze() {
  
  /* test
  vector<vector<int> > test(6);
  for (int i=0; i<6; i++) {
    test[i] = vector<int>(i);
    for (int j=0; j<i; j++) {
      test[i][j] = rand()%100;
      cerr << "test[" << i << "][" << j << "] = " << test[i][j] << '\n';
    }
  }

  vector<int> result = Algorithm_Best_Perm(test);
  cerr << "KBBK_analyze result:\n";
  for (unsigned int i=0; i<result.size(); i++)
    cerr << result[i] << '\n';
  */

  vector<int> k1_perm, b1a_perm, b1b_perm, k2_perm;

  { // k1 analysis
    vector<vector<int> > test(10);
    for (int i=0; i<10; i++) test[i] = vector<int>(i);

    for (int color=0; color<2; color++)
      for (int k2=0; k2<64; k2++)
	for (int b1a=0; b1a<32; b1a++)
	  for (int b1b=0; b1b<32; b1b++) {

	    for (int k1_1=0; k1_1<10; k1_1++)
	      for (int k1_2=0; k1_2<k1_1; k1_2++)
		if (same(KBBK[k1_1][b1a][b1b][k2][color], KBBK[k1_2][b1a][b1b][k2][color]))
		  ++test[k1_1][k1_2];

	  }

    /*
    for (int k1_1=0; k1_1<10; k1_1++)
      for (int k1_2=0; k1_2<k1_1; k1_2++)
	cerr << "test[" << k1_1 << "][" << k1_2 << "] = " << test[k1_1][k1_2] << '\n';
    */

    k1_perm = Algorithm_Best_Perm(test);
    cerr << "KBBK_analyze k1_perm:\n";
    for (unsigned int i=0; i<k1_perm.size(); i++)
      cerr << k1_perm[i] << ", ";
    cerr << '\n';
  }


  { // b1a analysis
    vector<vector<int> > test(32);
    for (int i=0; i<32; i++) test[i] = vector<int>(i);

    for (int color=0; color<2; color++)
      for (int k2=0; k2<64; k2++)
	for (int k1=0; k1<10; k1++)
	  for (int b1b=0; b1b<32; b1b++) {

	    for (int b1a_1=0; b1a_1<32; b1a_1++)
	      for (int b1a_2=0; b1a_2<b1a_1; b1a_2++)
		if (same(KBBK[k1][b1a_1][b1b][k2][color], KBBK[k1][b1a_2][b1b][k2][color]))
		  ++test[b1a_1][b1a_2];

	  }

    /*
    for (int b1a_1=0; b1a_1<32; b1a_1++)
      for (int b1a_2=0; b1a_2<b1a_1; b1a_2++)
	cerr << "test[" << b1a_1 << "][" << b1a_2 << "] = " << test[b1a_1][b1a_2] << '\n';
    */

    b1a_perm = Algorithm_Best_Perm(test);
    cerr << "KBBK_analyze b1a_perm:\n";
    for (unsigned int i=0; i<b1a_perm.size(); i++)
      cerr << b1a_perm[i] << ", ";
    cerr << '\n';
  }

  { // b1b analysis
    vector<vector<int> > test(32);
    for (int i=0; i<32; i++) test[i] = vector<int>(i);

    for (int color=0; color<2; color++)
      for (int k2=0; k2<64; k2++)
	for (int k1=0; k1<10; k1++)
	  for (int b1a=0; b1a<32; b1a++) {

	    for (int b1b_1=0; b1b_1<32; b1b_1++)
	      for (int b1b_2=0; b1b_2<b1b_1; b1b_2++)
		if (same(KBBK[k1][b1a][b1b_1][k2][color], KBBK[k1][b1a][b1b_2][k2][color]))
		  ++test[b1b_1][b1b_2];

	  }

    b1b_perm = Algorithm_Best_Perm(test);
    cerr << "KBBK_analyze b1b_perm:\n";
    for (unsigned int i=0; i<b1b_perm.size(); i++)
      cerr << b1b_perm[i] << ", ";
    cerr << '\n';
  }

  { // b1b analysis
    vector<vector<int> > test(64);
    for (int i=0; i<64; i++) test[i] = vector<int>(i);

    for (int color=0; color<2; color++)
      for (int k1=0; k1<10; k1++)
	for (int b1a=0; b1a<32; b1a++)
	  for (int b1b=0; b1b<32; b1b++) {

	    for (int k2_1=0; k2_1<64; k2_1++)
	      for (int k2_2=0; k2_2<k2_1; k2_2++)
		if (same(KBBK[k1][b1a][b1b][k2_1][color], KBBK[k1][b1a][b1b][k2_2][color]))
		  ++test[k2_1][k2_2];

	  }

    k2_perm = Algorithm_Best_Perm(test);
    cerr << "KBBK_analyze k2_perm:\n";
    for (unsigned int i=0; i<k2_perm.size(); i++)
      cerr << k2_perm[i] << ", ";
    cerr << '\n';

  }

  //KBBK_save_perms_to_file(k1_perm, b1a_perm, b1b_perm, k2_perm);

  ___KBBK_test(10, ILLEGAL_POS, 26, 13, 1, b1a_perm, b1b_perm);
}


void KBBK_test(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color) {
  bool a = k1_pos == ILLEGAL_POS;
  bool b = b1a_pos == ILLEGAL_POS;
  bool c = b1b_pos == ILLEGAL_POS;
  bool d = k2_pos == ILLEGAL_POS;

  cerr << "KBBK_test(" << k1_pos << ',' << b1a_pos << ',' << b1b_pos
       << ',' << k2_pos << ',' << color << ")\n";

  if (a+b+c+d != 1) {
    cerr << "KBBK_test: Exactly one must be ILLEGAL_POS\n";
    return;
  }

  uchar map64[64];

  for (int i=0; i<64; i++)
    map64[i] = 0;
  if (!a) map64[k1_pos] = 1;
  if (!b) map64[b1a_pos] = 2;
  if (!c) map64[b1b_pos] = 3;
  if (!d) map64[k2_pos] = 4;
  cerr << "Piece placements:\n";
  print_map64<uchar>(cerr, map64, 3, 10);

  for (int i=0; i<64; i++)
    map64[i] = KBBK_index(a ? i : k1_pos, b ? i : b1a_pos, c ? i : b1b_pos, d ? i : k2_pos, color);

  if (a) cerr << "King 1 positions:\n";
  if (b) cerr << "Bishop 1a positions:\n";
  if (c) cerr << "Bishop 1b positions:\n";
  if (d) cerr << "King 2 positions:\n";

  print_map64<uchar>(cerr, map64, 3, 10);


  int k1  = ((k1_pos == ILLEGAL_POS)  ? 0 : k1_pos);
  int b1a = ((b1a_pos == ILLEGAL_POS) ? 0 : b1a_pos);
  int b1b = ((b1b_pos == ILLEGAL_POS) ? 0 : b1b_pos);
  int k2  = ((k2_pos == ILLEGAL_POS)  ? 0 : k2_pos);
  KBBK_compress_indeces(k1, b1a, b1b, k2);

  cerr << '\n';
  if (a) {
    for (int j=0; j<32; j++) {
      for (int i=0; i<10; i++) cerr << (int)KBBK[i][j][b1b][k2][color] << ' ';
      cerr << '\n';
    }
  }
  if (b | c) {
    for (int j=0; j<32; j++) {
      for (int i=0; i<32; i++) cerr << (int)KBBK[k1][i][j][k2][color] << ' ';
      cerr << '\n';
    }
    cerr << "\n --- or ---\n";
    for (int j=0; j<32; j++) {
      for (int i=0; i<32; i++) cerr << (int)KBBK[k1][j][i][k2][color] << ' ';
      cerr << '\n';
    }
  }
  // if (d) for (int i=0; i<64; i++) cerr << (int)KBBK[k1][b1a][b1b][i][color] << ' ';
  cerr << '\n';
}


inline void KBBK_compress_indeces(int &k1, int &b1a, int &b1b, int &k2) {
  // if ((b1a&9)==0  ||  (b1a&9)==9) swap(b1a, b1b);
  b1a = reflect(b1a, REFLECTION[k1]);// >> 1;
  b1b = reflect(b1b, REFLECTION[k1]);// >> 1;
  if ((b1a&9)==0  ||  (b1a&9)==9) swap(b1a, b1b);
  b1a >>= 1;
  b1b >>= 1;

  k2 = reflect(k2, REFLECTION[k1]);
  k1 = KING_SYMMETRY_POS[k1];
}
inline void KBBK_decompress_indeces(int &k1, int &b1a, int &b1b, int &k2) {
  k1 = INV_KING_SYMMETRY_POS[k1];
  b1a += b1a;
  b1a += ((b1a&8)==0);
  b1b += b1b;
  b1b += ((b1b&8)==8);
}
inline int KBBK_index(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color) {
  int tmp = (b1a_pos^b1b_pos)&9;
  if (tmp==0 || tmp==9) return 254;//2 bishops on same color

  KBBK_compress_indeces(k1_pos, b1a_pos, b1b_pos, k2_pos);
  // cerr << "KBBK[" << k1_pos << "][" << b1a_pos << "][" << b1b_pos << "][" << k2_pos
  //     << "][" << color << "] = " << (int)KBBK[k1_pos][b1a_pos][b1b_pos][k2_pos][color] << "\n";
  return KBBK[k1_pos][b1a_pos][b1b_pos][k2_pos][color];
}
inline void KBBK_update(int k1_pos, int b1a_pos, int b1b_pos, int k2_pos, int color, uchar new_value) {
  KBBK_compress_indeces(k1_pos, b1a_pos, b1b_pos, k2_pos);
  KBBK[k1_pos][b1a_pos][b1b_pos][k2_pos][color] = new_value;
}


int reduce_problem(Board2a &board) {
  bool hit_possible = false;
  bool move_possible = false;
  Move move = board.moves();
  while (board.next_move(move)) {
    move_possible = true;
    if (board.read_board(move.to)) {
      hit_possible = true;
      break;
    }
  }
  if (!move_possible) return 254;
  return hit_possible ? 254 : 253;
}

void build_KBBK() {
  if (KBBK_load_from_file()) return;
  cerr << "Building KBBK end game table\n";
  
  // KBBK[king1][bishop1a][bishop1b][king2][player]
  // unsigned char KBBK[10][32][32][64][2];

  Board3 board;
  for (int k1=0; k1<10; k1++) for (int k2=0; k2<64; k2++)
    for (int b1a=0; b1a<32; b1a++) for (int b1b=0; b1b<32; b1b++) {
      int k1_pos = k1;
      int b1a_pos = b1a;
      int b1b_pos = b1b;
      int k2_pos = k2;
      KBBK_decompress_indeces(k1_pos, b1a_pos, b1b_pos, k2_pos);
      
      // if (!kings_too_near(k1_pos, k2_pos)  &&  !piece_overlap(4, k1_pos, b1a_pos, b1b_pos, k2_pos)) {

      vector<pair<Piece, Position> > piece_list(4);
      piece_list[0] = pair<Piece, Position>(WKING, k1_pos);
      piece_list[1] = pair<Piece, Position>(WBISHOP, b1a_pos);
      piece_list[2] = pair<Piece, Position>(WBISHOP, b1b_pos);
      piece_list[3] = pair<Piece, Position>(BKING, k2_pos);
	
      // BLACK to move...
      if (board.set_board(piece_list, BLACK)) {
	
	if (board.get_num_checks()) {
	  // illegal pos if white to move - can hit black king
	  KBBK[k1][b1a][b1b][k2][WHITE] = 255;
	  if (!(k1!=1 || b1a!=2 || b1b!=1 || k2!=4))
	    board.print_board(cerr);

	  KBBK[k1][b1a][b1b][k2][BLACK] =
	    board.calc_game_status()==WHITE_WON ? 0 : reduce_problem(board);
	  /*
	  status ? (status==WHITE_WON ? 0 : 254) :
	    (reduce_problem(board) ? 254 : 253);
	  */

	} else {

	  KBBK[k1][b1a][b1b][k2][BLACK] = reduce_problem(board);
	  // white cannot be pat
	  KBBK[k1][b1a][b1b][k2][WHITE] = 253;
	}
      } else {
	// illegal pos
	assert(k1!=1 || b1a!=2 || b1b!=1 || k2!=4);

	KBBK[k1][b1a][b1b][k2][WHITE] = 255;
	KBBK[k1][b1a][b1b][k2][BLACK] = 255;
      }
    }

  // for (int i=10; i<20; i++) cerr << "KBBK[5][9][13][" << i << "][1] = " << (int)KBBK[5][9][13][i][1] << '\n';
  

  bool progress = true;
  int search_for_mate_in_n = 0;
  int player = BLACK;
  while (progress) {
    progress = false;
    player ^= 1;
    cerr << "Searching for mate in " << ++search_for_mate_in_n
	 << " (only " << PLAYER_CHAR[player] << ")\n";

    for (int k1=0; k1<10; k1++) for (int k2=0; k2<64; k2++)
      for (int b1a=0; b1a<32; b1a++) for (int b1b=0; b1b<32; b1b++)
	if (KBBK[k1][b1a][b1b][k2][player] == 253) {

	  int k1_pos = k1;
	  int b1a_pos = b1a;
	  int b1b_pos = b1b;
	  int k2_pos = k2;
	  KBBK_decompress_indeces(k1_pos, b1a_pos, b1b_pos, k2_pos);
      
	  vector<pair<Piece, Position> > piece_list(4);
	  piece_list[0] = pair<Piece, Position>(WKING, k1_pos);
	  piece_list[1] = pair<Piece, Position>(WBISHOP, b1a_pos);
	  piece_list[2] = pair<Piece, Position>(WBISHOP, b1b_pos);
	  piece_list[3] = pair<Piece, Position>(BKING, k2_pos);

	  board.set_board(piece_list, player);

	  int best = player ? 0 : 253;

	  Move best_move;
	  Move move = board.moves();
	  while (board.next_move(move)) {

	    if (board.read_board(move.to)) {
	      // black hit a piece => draw
	      assert(false);
	      best = 254;
	      break;
	    }

	    int _k1 = move.from==k1_pos ? move.to : k1_pos;
	    int _b1a = move.from==b1a_pos ? move.to : b1a_pos;
	    int _b1b = move.from==b1b_pos ? move.to : b1b_pos;
	    int _k2 = move.from==k2_pos ? move.to : k2_pos;

	    int tmp = KBBK_index(_k1, _b1a, _b1b, _k2, player^1);

	    /*
	    if (tmp == 255) {
	      int __k1=_k1, __b1a=_b1a, __b1b=_b1b, __k2=_k2;
	      KBBK_compress_indeces(__k1, __b1a, __b1b, __k2);
	      //KBBK_decompress_indeces(__k1, __b1a, __b1b, __k2);

	      cerr << "Error: " << _k1 << ',' << _b1a << ',' << _b1b
		   << ',' << _k2 << ", move = " << move.toString()
		   << " (conv. to " << __k1 << ',' << __b1a << ','
		   << __b1b << ',' << __k2 << ")\n";
	    }

	    if (k1_pos==10  &&  k2_pos==0  &&
		b1b_pos==11  &&  b1a_pos==26)
	      cerr << "Move " << move.toString() << ", tmp = " << tmp << '\n';
	    */

	    if (player==WHITE) {
	      if (tmp < best) {
		//cerr << "White best updated " << best << " to " << tmp << '\n';
		best = tmp;
		best_move = move;
	      }
	    } else {
	      if (tmp > best) {
		//cerr << "Black best updated " << best << " to " << tmp << '\n';
		best = tmp;
		best_move = move;
	      }
	    }
	  }


	  if (best != 253) {
	    if (best != 254) ++best; // mate in n+1

	    if (!progress) {
	      cerr << "best_move = " << best_move.toString() << " (" << best << ")\n";
	      board.print_board(cerr);
	    }

	    progress = true;
	    KBBK[k1][b1a][b1b][k2][player] = best;
	  }
	}
  }


  /*
    a b c d e f g h
  +-----------------+    |
8 |                 | 8  | 1w
7 |                 | 7  |
6 |                 | 6  | White has lost castling
5 |                 | 5  | Black has lost castling
4 |                 | 4  |
3 |                 | 3  | moves played since progress = 0
2 | B               | 2  |
1 | k   K   B       | 1  |
  +-----------------+    |
  */

  // All remaining undecided positions is drawn (like the one above)
  for (int k1=0; k1<10; k1++) for (int k2=0; k2<64; k2++)
    for (int b1a=0; b1a<32; b1a++) for (int b1b=0; b1b<32; b1b++)
      for (int player=0; player<2; player++)
	if (KBBK[k1][b1a][b1b][k2][player] == 253)
	  KBBK[k1][b1a][b1b][k2][player] = 254;

  KBBK_save_to_file();
}
