// g++ -ansi -pedantic create_move_tables.cxx
// ./a.out >board_move_tables.hxx

#include <iostream>

using namespace std;

#include "board.hxx"
#include "board_move_tables.hxx"

Position *WPAWN_NEXT = 0;
Position *KNIGHT_NEXT = 0;
Position *BISHOP_NEXT = 0;
Position *ROOK_NEXT = 0;
Position *QUEEN_NEXT = 0;
Position *KING_NEXT = 0;
Position *BPAWN_NEXT = 0;

Position* _PIECE_NEXT[13];


Position *WPAWN_JUMP = 0;
Position *KNIGHT_JUMP = 0;
Position *BISHOP_JUMP = 0;
Position *ROOK_JUMP = 0;
Position *QUEEN_JUMP = 0;
Position *KING_JUMP = 0;
Position *BPAWN_JUMP = 0;

Position* _PIECE_JUMP[13];


Position *WPAWN_DNEXT = 0;
Position *KNIGHT_DNEXT = 0;
Position *BISHOP_DNEXT = 0;
Position *ROOK_DNEXT = 0;
Position *QUEEN_DNEXT = 0;
Position *KING_DNEXT = 0;
Position *BPAWN_DNEXT = 0;

Position* _PIECE_DNEXT[13];


// Retro moves
Position *INV_WPAWN_NEXT = 0;
Position *INV_BPAWN_NEXT = 0;

Position *INV_WPAWN_JUMP = 0;
Position *INV_BPAWN_JUMP = 0;

Position *_INV_PIECE_NEXT[13];
Position *_INV_PIECE_JUMP[13];



void allocate_mem() {
  Position *tmp = (Position *)malloc((3*7 + 2*2)*4096);
  
  WPAWN_NEXT = tmp;
  KNIGHT_NEXT = (tmp += 4096);
  BISHOP_NEXT = (tmp += 4096);
  ROOK_NEXT = (tmp += 4096);
  QUEEN_NEXT = (tmp += 4096);
  KING_NEXT = (tmp += 4096);
  BPAWN_NEXT = (tmp += 4096);

  _PIECE_NEXT[0] = 0;
  _PIECE_NEXT[WPAWN] = WPAWN_NEXT;
  _PIECE_NEXT[WKNIGHT] = KNIGHT_NEXT;
  _PIECE_NEXT[WBISHOP] = BISHOP_NEXT;
  _PIECE_NEXT[WROOK] = ROOK_NEXT;
  _PIECE_NEXT[WQUEEN] = QUEEN_NEXT;
  _PIECE_NEXT[WKING] = KING_NEXT;
  _PIECE_NEXT[BPAWN] = BPAWN_NEXT;
  _PIECE_NEXT[BKNIGHT] = KNIGHT_NEXT;
  _PIECE_NEXT[BBISHOP] = BISHOP_NEXT;
  _PIECE_NEXT[BROOK] = ROOK_NEXT;
  _PIECE_NEXT[BQUEEN] = QUEEN_NEXT;
  _PIECE_NEXT[BKING] = KING_NEXT;

  WPAWN_JUMP = (tmp += 4096);
  KNIGHT_JUMP = (tmp += 4096);
  BISHOP_JUMP = (tmp += 4096);
  ROOK_JUMP = (tmp += 4096);
  QUEEN_JUMP = (tmp += 4096);
  KING_JUMP = (tmp += 4096);
  BPAWN_JUMP = (tmp += 4096);

  _PIECE_JUMP[0] = 0;
  _PIECE_JUMP[WPAWN] = WPAWN_JUMP;
  _PIECE_JUMP[WKNIGHT] = KNIGHT_JUMP;
  _PIECE_JUMP[WBISHOP] = BISHOP_JUMP;
  _PIECE_JUMP[WROOK] = ROOK_JUMP;
  _PIECE_JUMP[WQUEEN] = QUEEN_JUMP;
  _PIECE_JUMP[WKING] = KING_JUMP;
  _PIECE_JUMP[BPAWN] = BPAWN_JUMP;
  _PIECE_JUMP[BKNIGHT] = KNIGHT_JUMP;
  _PIECE_JUMP[BBISHOP] = BISHOP_JUMP;
  _PIECE_JUMP[BROOK] = ROOK_JUMP;
  _PIECE_JUMP[BQUEEN] = QUEEN_JUMP;
  _PIECE_JUMP[BKING] = KING_JUMP;

  WPAWN_DNEXT = (tmp += 4096);
  KNIGHT_DNEXT = (tmp += 4096);
  BISHOP_DNEXT = (tmp += 4096);
  ROOK_DNEXT = (tmp += 4096);
  QUEEN_DNEXT = (tmp += 4096);
  KING_DNEXT = (tmp += 4096);
  BPAWN_DNEXT = (tmp += 4096);

  _PIECE_DNEXT[0] = 0;
  _PIECE_DNEXT[WPAWN] = WPAWN_DNEXT;
  _PIECE_DNEXT[WKNIGHT] = KNIGHT_DNEXT;
  _PIECE_DNEXT[WBISHOP] = BISHOP_DNEXT;
  _PIECE_DNEXT[WROOK] = ROOK_DNEXT;
  _PIECE_DNEXT[WQUEEN] = QUEEN_DNEXT;
  _PIECE_DNEXT[WKING] = KING_DNEXT;
  _PIECE_DNEXT[BPAWN] = BPAWN_DNEXT;
  _PIECE_DNEXT[BKNIGHT] = KNIGHT_DNEXT;
  _PIECE_DNEXT[BBISHOP] = BISHOP_DNEXT;
  _PIECE_DNEXT[BROOK] = ROOK_DNEXT;
  _PIECE_DNEXT[BQUEEN] = QUEEN_DNEXT;
  _PIECE_DNEXT[BKING] = KING_DNEXT;


  // ########################################
  // ##########    RETRO MOVES    ###########
  // ########################################
  
  INV_WPAWN_NEXT = (tmp += 4096);
  INV_BPAWN_NEXT = (tmp += 4096);

  _INV_PIECE_NEXT[0] = 0;
  _INV_PIECE_NEXT[WPAWN] = INV_WPAWN_NEXT;//
  _INV_PIECE_NEXT[WKNIGHT] = KNIGHT_NEXT;
  _INV_PIECE_NEXT[WBISHOP] = BISHOP_NEXT;
  _INV_PIECE_NEXT[WROOK] = ROOK_NEXT;
  _INV_PIECE_NEXT[WQUEEN] = QUEEN_NEXT;
  _INV_PIECE_NEXT[WKING] = KING_NEXT;
  _INV_PIECE_NEXT[BPAWN] = INV_BPAWN_NEXT;//
  _INV_PIECE_NEXT[BKNIGHT] = KNIGHT_NEXT;
  _INV_PIECE_NEXT[BBISHOP] = BISHOP_NEXT;
  _INV_PIECE_NEXT[BROOK] = ROOK_NEXT;
  _INV_PIECE_NEXT[BQUEEN] = QUEEN_NEXT;
  _INV_PIECE_NEXT[BKING] = KING_NEXT;

  INV_WPAWN_JUMP = (tmp += 4096);
  INV_BPAWN_JUMP = (tmp += 4096);

  _INV_PIECE_JUMP[0] = 0;
  _INV_PIECE_JUMP[WPAWN] = INV_WPAWN_JUMP;//
  _INV_PIECE_JUMP[WKNIGHT] = KNIGHT_JUMP;
  _INV_PIECE_JUMP[WBISHOP] = BISHOP_JUMP;
  _INV_PIECE_JUMP[WROOK] = ROOK_JUMP;
  _INV_PIECE_JUMP[WQUEEN] = QUEEN_JUMP;
  _INV_PIECE_JUMP[WKING] = KING_JUMP;
  _INV_PIECE_JUMP[BPAWN] = INV_BPAWN_JUMP;//
  _INV_PIECE_JUMP[BKNIGHT] = KNIGHT_JUMP;
  _INV_PIECE_JUMP[BBISHOP] = BISHOP_JUMP;
  _INV_PIECE_JUMP[BROOK] = ROOK_JUMP;
  _INV_PIECE_JUMP[BQUEEN] = QUEEN_JUMP;
  _INV_PIECE_JUMP[BKING] = KING_JUMP;
}


//#######################

int dest_list[8][8][64];
bool new_direction_list[8][8][64];
int dest_list_count[8][8];
int next_move[8][8][8][8];
int next_jump[8][8][8][8];

int r,c;

void init() {
  for (int i1=0; i1<8; i1++)
    for (int i2=0; i2<8; i2++) {
      dest_list_count[i1][i2] = 0;
      for (int i3=0; i3<8; i3++)
	for (int i4=0; i4<8; i4++) {
	  dest_list[i1][i2][(i3<<3)|i4] = IMPOSSIBLE_MOVE;
	  new_direction_list[i1][i2][(i3<<3)|i4] = false;

	  next_move[i1][i2][i3][i4] = IMPOSSIBLE_MOVE;
	  next_jump[i1][i2][i3][i4] = IMPOSSIBLE_MOVE;
	}
      new_direction_list[i1][i2][1] = true;
    }
}

void add_move(int row, int colum) {
  dest_list[r][c][dest_list_count[r][c]++] = CR_TO_POS[colum][row];
}

void new_direction() {
  new_direction_list[r][c][dest_list_count[r][c]] = true;
}

//----------

void create_next_move_list() {
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      int pos, index = 0;
      pos = dest_list[r][c][index++];
      while (legal_pos(pos)) {
	int prev = pos;
	pos = dest_list[r][c][index++];
	next_move[r][c][ROW[prev]][COLUMN[prev]] = pos;
      }
    }
}

int jump_to(int index) {
  int result = dest_list[r][c][index++];
  while (legal_pos(result) && !new_direction_list[r][c][index-1]) {
    result = dest_list[r][c][index++];
  }
  return result;
}

void create_next_jump_list() {
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      int pos, index = 0;
      pos = dest_list[r][c][index++];
      while (legal_pos(pos)) {
	int next = jump_to(index);
	next_jump[r][c][ROW[pos]][COLUMN[pos]] = next;
	pos = dest_list[r][c][index++];
	/*
	int prev = pos;
	pos = jump_to(index++);
	next_jump[r][c][ROW[prev]][COLUMN[prev]] = pos;
	*/
      }
    }
}

string S(int n) {
  string result = "  ";
  if (n > 9) result[0] = (n/10)+'0';
  result[1] = (n%10)+'0';
  return result;
}

void output(Position *next, Position *jump, Position *dnext) {
  create_next_move_list();
  int index = -1;
  for (r=0; r<8; r++)
    for (c=0; c<8; c++)
      for (int r2=0; r2<8; r2++)
	for (int c2=0; c2<8; c2++)
	  next[++index] = next_move[r][c][r2][c2];
  /*
  cout << "static const Position " << s << "_NEXT[4096] =\n";
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      cout << (r+c ? " " : "{");
      for (int r2=0; r2<8; r2++)
	for (int c2=0; c2<8; c2++)
	  cout << S(next_move[r][c][r2][c2])
	       << (r2+c2==14 ? (r+c==14 ? "};\n" : ",\n") : ",");
    }
  */

  create_next_jump_list();
  index = -1;
  for (r=0; r<8; r++)
    for (c=0; c<8; c++)
      for (int r2=0; r2<8; r2++)
	for (int c2=0; c2<8; c2++)
	  jump[++index] = next_jump[r][c][r2][c2];
  /*
  cout << "\nstatic const Position " << s << "_JUMP[4096] =\n";
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      cout << (r+c ? " " : "{");
      for (int r2=0; r2<8; r2++)
	for (int c2=0; c2<8; c2++)
	  cout << S(next_jump[r][c][r2][c2])
	       << (r2+c2==14 ? (r+c==14 ? "};\n\n" : ",\n") : ",");
    }
  */
  
  if (dnext) {
    for (int i=0; i<4096; i++)
      dnext[i] = next[i]==jump[i] ? ILLEGAL_POS : next[i];
  }
}

//##################################################################

void create_wpawn_move_tables() {
  init();

  for (r=0; r<8; r++) {
    for (c=0; c<8; c++) {
      add_move(r,c);
      if (r>0 && r<7) {
	if (c>0) add_move(r+1, c-1);
	new_direction();
	if (c<7) add_move(r+1, c+1);
	new_direction();
	add_move(r+1, c);
	if (r==1) add_move(r+2, c);
      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }
  }

  output(WPAWN_NEXT, WPAWN_JUMP, WPAWN_DNEXT);

  // Retro moves:
  // Remark: legal rows are now [2..7]
  init();
  for (r=0; r<8; r++) {
    for (c=0; c<8; c++) {
      add_move(r,c);
      if (r>1) {
	if (c>0) add_move(r-1, c-1);
	new_direction();
	if (c<7) add_move(r-1, c+1);
	new_direction();
	add_move(r-1, c);
	if (r==3) add_move(r-2, c);
      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }
  }

  output(INV_WPAWN_NEXT, INV_WPAWN_JUMP, 0);
}

void create_bpawn_move_tables() {
  init();
  
  for (r=0; r<8; r++) {
    for (c=0; c<8; c++) {
      add_move(r,c);
      if (r>0 && r<7) {
	if (c>0) add_move(r-1, c-1);
	new_direction();
	if (c<7) add_move(r-1, c+1);
	new_direction();
	add_move(r-1, c);
	if (r==6) add_move(r-2, c);
      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }
  }

  output(BPAWN_NEXT, BPAWN_JUMP, BPAWN_DNEXT);
  
  // Retro moves:
  // Remark: legal rows are now [0..5]
  init();
  for (r=0; r<8; r++) {
    for (c=0; c<8; c++) {
      add_move(r,c);
      if (r<6) {
	if (c>0) add_move(r+1, c-1);
	new_direction();
	if (c<7) add_move(r+1, c+1);
	new_direction();
	add_move(r+1, c);
	if (r==4) add_move(r+2, c);
      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }
  }

  output(INV_BPAWN_NEXT, INV_BPAWN_JUMP, 0);

}

void create_knight_move_table() {
  init();

  pair<int,int> d[8];
  d[0] = pair<int, int>(-1,-2);
  d[1] = pair<int, int>(-2,-1);
  d[2] = pair<int, int>(1,2);
  d[3] = pair<int, int>(2,1);
  d[4] = pair<int, int>(-1,2);
  d[5] = pair<int, int>(-2,1);
  d[6] = pair<int, int>(1,-2);
  d[7] = pair<int, int>(2,-1);
  
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      add_move(r,c);
      for (int i=0; i<8; i++)
	if (0 <= r+d[i].first  &&  r+d[i].first < 8  &&
	    0 <= c+d[i].second  &&  c+d[i].second < 8) {
	  add_move(r+d[i].first, c+d[i].second);
	  new_direction();
	}
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }

  output(KNIGHT_NEXT, KNIGHT_JUMP, KNIGHT_DNEXT);
}

void create_bishop_move_table() {
  init();

  pair<int,int> d[4];
  d[0] = pair<int, int>(-1,-1);
  d[1] = pair<int, int>(-1,1);
  d[2] = pair<int, int>(1,-1);
  d[3] = pair<int, int>(1,1);
  
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      add_move(r,c);
      for (int i=0; i<4; i++) {
	for (int j=1; j<8; j++)
	  if (0 <= r+j*d[i].first  &&  r+j*d[i].first < 8  &&
	      0 <= c+j*d[i].second  &&  c+j*d[i].second < 8) {
	    add_move(r+j*d[i].first, c+j*d[i].second);
	  }
	new_direction();
      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }

  output(BISHOP_NEXT, BISHOP_JUMP, BISHOP_DNEXT);
}

void create_rook_move_table() {
  init();

  pair<int,int> d[4];
  d[0] = pair<int, int>(-1,0);
  d[1] = pair<int, int>(1,0);
  d[2] = pair<int, int>(0,-1);
  d[3] = pair<int, int>(0,1);
  
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      add_move(r,c);
      for (int i=0; i<4; i++) {
	for (int j=1; j<8; j++)
	  if (0 <= r+j*d[i].first  &&  r+j*d[i].first < 8  &&
	      0 <= c+j*d[i].second  &&  c+j*d[i].second < 8) {
	    add_move(r+j*d[i].first, c+j*d[i].second);
	  }
	new_direction();
      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }

  output(ROOK_NEXT, ROOK_JUMP, ROOK_DNEXT);
}

void create_queen_move_table() {
  init();

  pair<int,int> d[8];
  d[0] = pair<int, int>(-1,-1);
  d[1] = pair<int, int>(-1,1);
  d[2] = pair<int, int>(1,-1);
  d[3] = pair<int, int>(1,1);
  d[4] = pair<int, int>(-1,0);
  d[5] = pair<int, int>(1,0);
  d[6] = pair<int, int>(0,-1);
  d[7] = pair<int, int>(0,1);
  
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      add_move(r,c);
      for (int i=0; i<8; i++) {
	for (int j=1; j<8; j++)
	  if (0 <= r+j*d[i].first  &&  r+j*d[i].first < 8  &&
	      0 <= c+j*d[i].second  &&  c+j*d[i].second < 8) {
	    add_move(r+j*d[i].first, c+j*d[i].second);
	  }
	new_direction();
      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }

  output(QUEEN_NEXT, QUEEN_JUMP, QUEEN_DNEXT);
}

void create_king_move_table() {
  init();

  pair<int,int> d[8];
  d[0] = pair<int, int>(-1,-1);
  d[1] = pair<int, int>(-1,1);
  d[2] = pair<int, int>(1,-1);
  d[3] = pair<int, int>(1,1);
  d[4] = pair<int, int>(-1,0);
  d[5] = pair<int, int>(1,0);
  d[6] = pair<int, int>(0,-1);
  d[7] = pair<int, int>(0,1);
  
  for (r=0; r<8; r++)
    for (c=0; c<8; c++) {
      add_move(r,c);
      if (r==0  &&  c==4) {

	// White castling
	add_move(0,3);
	add_move(0,2); // castling move
	new_direction();
	add_move(1,3);
	new_direction();
	add_move(1,4);
	new_direction();
	add_move(1,5);
	new_direction();
	add_move(0,5);
	add_move(0,6); // castling move
	new_direction();

      } else if (r==7  &&  c==4) {

	// Black castling
	add_move(7,3);
	add_move(7,2); // castling move
	new_direction();
	add_move(6,3);
	new_direction();
	add_move(6,4);
	new_direction();
	add_move(6,5);
	new_direction();
	add_move(7,5);
	add_move(7,6); // castling move
	new_direction();

      } else {

	for (int i=0; i<8; i++) {
	  if (0 <= r+d[i].first  &&  r+d[i].first < 8  &&
	      0 <= c+d[i].second  &&  c+d[i].second < 8) {
	    add_move(r+d[i].first, c+d[i].second);
	  }
	  new_direction();
	}

      }
      dest_list[r][c][dest_list_count[r][c]++] = ILLEGAL_POS;
    }

  output(KING_NEXT, KING_JUMP, KING_DNEXT);
}

void initialize_move_tables() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  allocate_mem();

  create_wpawn_move_tables();
  create_bpawn_move_tables();
  create_knight_move_table();
  create_bishop_move_table();
  create_rook_move_table();
  create_queen_move_table();
  create_king_move_table();
}

/*
int main() {
  create_wpawn_move_table();
  create_bpawn_move_table();
  create_knight_move_table();
  create_bishop_move_table();
  create_rook_move_table();
  create_queen_move_table();
  create_king_move_table();

  cout << "static const Position* _PIECE_NEXT[13] =\n"
       << "{0,\n"
       << " WPAWN_NEXT, KNIGHT_NEXT, BISHOP_NEXT, ROOK_NEXT, QUEEN_NEXT, KING_NEXT,\n"
       << " BPAWN_NEXT, KNIGHT_NEXT, BISHOP_NEXT, ROOK_NEXT, QUEEN_NEXT, KING_NEXT};\n"
       << "#define PIECE_NEXT(piece, from, to) _PIECE_NEXT[piece][(from<<6) | to]\n";
  cout << "static const Position* _PIECE_JUMP[13] =\n"
       << "{0,\n"
       << " WPAWN_JUMP, KNIGHT_JUMP, BISHOP_JUMP, ROOK_JUMP, QUEEN_JUMP, KING_JUMP,\n"
       << " BPAWN_JUMP, KNIGHT_JUMP, BISHOP_JUMP, ROOK_JUMP, QUEEN_JUMP, KING_JUMP};\n"
       << "#define PIECE_JUMP(piece, from, to) _PIECE_JUMP[piece][(from<<6) | to]\n";
}
*/
