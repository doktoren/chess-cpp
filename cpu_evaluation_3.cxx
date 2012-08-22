#include "cpu_evaluation_3.hxx"

#include "game_phase.hxx"

#include "my_vector.hxx"
#include "help_functions.hxx"
#include "piece_values.hxx"

#include "opening_library.hxx"

#ifndef NDEBUG

#define EntryWithMove DEBUG_EntryWithMove
#define TRUE(dah) (*(settings.dah))
#define FALSE(dah) (*(settings.dah))
#define DEFAULT(name, def_value) (*(settings.name))

#else

#define TRUE(dah) true
#define FALSE(dah) false
#define DEFAULT(name, def_value) (def_value)

#endif


// todo pawn formation hash table
// hash value = pawn_bitboards[WHITE]...

// loadfen 8/8/8/8/8/8/4P3/k3K3 w - -

int piece_values[13][64];
int control_values[64];

Eval_3::Eval_3() : settings(&(comm->settings)), pawn_table(10)
{
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Eval_3 constructor called.\n";
  init_passed_pawns_bitboards();
}

Eval_3::~Eval_3() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Eval_3 destructor called.\n";
  //this->SearchRoutine::~SearchRoutine(); Will be called automatically
}

void Eval_3::reset_all() {
  piece_value = 0;

  pawn_bitboards[WHITE] = 0;
  pawn_bitboards[BLACK] = 0;

  Board3plus::reset_all();
}

bool Eval_3::clr_evaluation(void *ignored, Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Eval_3 &b = *dynamic_cast<Eval_3 *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Evaluation 3, help:\n"
       << "    print board  or  pb  or  dir\n"
       << "      - print board\n"
       << "    print eval stat  or  pes\n"
       << "    print evaluation  or  pe\n"
       << "    print evaluation 2  or  pe2\n"
       << "      - show evaluation after each possible move\n";
  } else if (dot_demand(p, 1, "dir")  ||
	     dot_demand(p, 2, "print", "board")) {
    b.print_board(os);
  } else if (dot_demand(p, 3, "print", "eval", "statistic")) {
    b.print_eval_stat(cerr);
  } else if (dot_demand(p, 2, "print", "evaluation")) {
    int value = b.root_evaluate();
    os << "Evaluation(current position) = " << value << '\n';
  } else if (dot_demand(p, 1, "pe2")) {
    Move move = b.moves();
    while (b.next_move(move)) {
      b.execute_move(move);
      int value = -b.root_evaluate();
      b.undo_move();
      os << "move " << move.toString() << " - value = " << value << "\n";
    }


  } else return false;
  return true;
}

void Eval_3::print_eval_stat(ostream& os) {
  /*
  os << "num_evaluations = " << num_evaluations << "\n";
  os << "Material left = " << material << '\n';
  os << "Game_phase(material) = (" << GAME_PHASE_MATERIAL[material][OPENING_GAME]
     << ',' << GAME_PHASE_MATERIAL[material][MID_GAME]
     << ',' << GAME_PHASE_MATERIAL[material][END_GAME] << ")\n";
  if (moves_played & 0xFFFFFF80) {
    os << "Game_phase(moves_played) = pure end game, since moves_played >= 128\n";
  } else {
    os << "Game_phase(moves_playes) = (" << GAME_PHASE_PLY[moves_played][OPENING_GAME]
       << ',' << GAME_PHASE_PLY[moves_played][MID_GAME]
       << ',' << GAME_PHASE_PLY[moves_played][END_GAME] << ")\n";
  }
  */
}

//###############  PROTECTED  ##################


void Eval_3::init_evaluation() {
  if (FALSE(show_evaluation_info))
    cerr << "Eval_3::init_evaluation called\n";
  num_evaluations = 0;
  set_piece_values();
}

void Eval_3::set_piece_values() {
  // Find amount of material
  int material = 0; // pawn 1, knights 3, etc. Independent on color
  for (int p=0; p<64; p++)
    if (board[p]) material += MATERIAL[board[p]];
  assert(material <= MAX_MATERIAL);
  if (FALSE(show_evaluation_info))
    cerr << "Material = " << material << '\n';

  // Decide the game phase
  int game_phase[3];
  for (int phase=0; phase<3; phase++) {
    game_phase[phase] = GAME_PHASE_MATERIAL[material][phase];
    if (moves_played & ~0x7F) {
      // num ply says end game.
      if (phase==2) game_phase[phase] += 256;
    } else {
      game_phase[phase] += GAME_PHASE_PLY[moves_played][phase];
    }
    game_phase[phase] /= 2;
  }

  game_phase_value = (game_phase[1] + 2*game_phase[2]) >> 1;
  assert(0<=game_phase_value  &&  game_phase_value<=256);
  if (FALSE(show_evaluation_info))
    cerr << "Game phase (" << game_phase[0] << "," << game_phase[1] << ","
	 << game_phase[2] << "), value(0..256) = " << game_phase_value << '\n';

  init_piece_values();
  init_control_values();

  // Evaluation piece_value
  piece_value = 0;
  for (int p=0; p<64; p++)
    if (board[p]) {
      piece_value += piece_values[board[p]][p];
      if (false && FALSE(show_evaluation_info))
	cerr << "Piece " << PIECE_SCHAR[board[p]] << " on " << POS_NAME[p]
	     << " value = " << piece_values[board[p]][p] << '\n';
    }
  if (FALSE(show_evaluation_info)) {
    cerr << "Piece material value in root position = " << piece_value << '\n';

    int pv[64];
    for (int i=0; i<64; i++) pv[i] = piece_values[board[i]][i];
    print_signed_map64(cerr, pv, 5, 10);
  }
}

ull PASSED_PAWN_BITBOARDS[2][64];
inline void Eval_3::init_passed_pawns_bitboards() {
  for (int player=0; player<2; player++) 
    for (int r=0; r<8; r++)
      for (int c=0; c<8; c++) {
	int p = CR_TO_POS[c][r];
	
	if (r==0 || r==7) {
	  PASSED_PAWN_BITBOARDS[player][p] = 0;
	} else {
	  
	  ull tmp = 0;
	  for (int c2=(c?(c-1):c); c2<=(c==7?c:(c+1)); c2++) {
	    if (player==WHITE) {
	      for (int r2=r+1; r2<8; r2++) {
		int p2 = CR_TO_POS[c2][r2];
		tmp |= p2;
	      }
	    } else {
	      for (int r2=0; r2<r; r2++) {
		int p2 = CR_TO_POS[c2][r2];
		tmp |= p2;
	      }
	    }
	  }
	  PASSED_PAWN_BITBOARDS[player][p] = tmp;
	}
      }
}

const int PASSED_PAWN_BONUS[8] = {0,10,15,25,35,48,80,0};
const int PAWN_PROTECTED_PASSED_PAWN_BONUS[8] = {0,10,15,25,35,48,80,0};
inline int Eval_3::passed_pawns_value() {
  int result = 0;
  for (int i=0; i<8; i++) {
    if (piece_number.number_used(W_PAWN_ID(i))) {
      int position = piece_number.get_pos(W_PAWN_ID(i));

      // the piece is either a pawn, or a piece promoted from a pawn
      if (board[position]==WPAWN) {
	if (!(pawn_bitboards[BLACK] & PASSED_PAWN_BITBOARDS[WHITE][position]).as_bool()) {
	  // a passed pawn!

	  int num_protecting_pawns = 0;
	  if (COLUMN[position]!=0  &&  board[position-9]==WPAWN) ++num_protecting_pawns;
	  if (COLUMN[position]!=7  &&  board[position-7]==WPAWN) ++num_protecting_pawns;

	  // control will typically be around [-5,...,5]
	  int control = 4;
	  //control += see.control_measure(position) + see.control_measure(position+8);
	  // further bonus for pawn backup:
	  control += 4*num_protecting_pawns;

	  // It may not be penalized
	  if (control > 0) {
	    if (FALSE(show_evaluation_info)) {
	      cerr << "Passed pawn on " << POS_NAME[position] << " bonus = control("
		   << control << ") * PPB(" << PASSED_PAWN_BONUS[ROW[position]] << ")\n";
	      cerr << "Black pawns:\n";
	      print_bit_board(cerr, pawn_bitboards[BLACK]);
	      cerr << "White passed pawn bitboard:\n";
	      print_bit_board(cerr, PASSED_PAWN_BITBOARDS[WHITE][position]);
	    }
	    result += control * PASSED_PAWN_BONUS[ROW[position]];
	  }
	}
      }
    }
    if (piece_number.number_used(B_PAWN_ID(i))) {
      int position = piece_number.get_pos(B_PAWN_ID(i));
      // the piece is either a pawn, or a piece promoted from a pawn
      if (board[position]==BPAWN) {

	if (!(pawn_bitboards[WHITE] & PASSED_PAWN_BITBOARDS[BLACK][position]).as_bool()) {
	  // a passed pawn!

	  int num_protecting_pawns = 0;
	  if (COLUMN[position]!=0  &&  board[position+7]==BPAWN) ++num_protecting_pawns;
	  if (COLUMN[position]!=7  &&  board[position+9]==BPAWN) ++num_protecting_pawns;

	  // control will typically be around [-5,...,5]
	  // see.control_measure returns control relative to white, hence the minus
	  int control = 4;
	  //control -= see.control_measure(position) + see.control_measure(position-8);
	  // further bonus for pawn backup:
	  control += 4*num_protecting_pawns;

	  // It may not be penalized
	  if (control > 0) {
	    if (FALSE(show_evaluation_info)) {
	      cerr << "Passed pawn on " << POS_NAME[position] << " bonus = control("
		   << control << ") * PPB(" << PASSED_PAWN_BONUS[7^ROW[position]] << ")\n";
	      cerr << "White pawns:\n";
	      print_bit_board(cerr, pawn_bitboards[WHITE]);
	      cerr << "Black passed pawn bitboard:\n";
	      print_bit_board(cerr, PASSED_PAWN_BITBOARDS[BLACK][position]);
	    }
	    result -= control * PASSED_PAWN_BONUS[7^ROW[position]];
	  }
	}
      }
    }
  }
  if (FALSE(show_evaluation_info))
    cerr << "Passed_pawns_value = " << result << '\n';
  return result;
}

// DOUBLE_PAWN_BONUS[1][0] : isolated double pawn
// DOUBLE_PAWN_BONUS[2][0] : 2 pairs of isolated double pawns
// DOUBLE_PAWN_BONUS[1][2] : double pawns, with pawns at both sides (!)
const int DOUBLE_PAWN_BONUS[5][8] = 
  {{0, 0, 0, 0, 0, 0, 0, 0},
   {-240, -80, -10, 0, 0, 0, 0, 0},
   {-480, -320, -160, -90, -20, 0, 0, 0},
   {-720, -560, -400, 0, 0, 0, 0, 0},
   {-960, 0, 0, 0, 0, 0, 0, 0}};
inline int Eval_3::double_pawns_value() {
  int value = 0;
  uchar row;

  uchar wrow = 0;
  for (int r=1; r<7; r++) wrow |= pawn_bitboards[WHITE].u.lines[r];

  row = pawn_bitboards[WHITE].u.lines[1];
  for (int r=2; r<7; r++) {
    uchar next = pawn_bitboards[WHITE].u.lines[r];
    uchar tmp = row & next;
    if (tmp) { 
      //if (FALSE(show_evaluation_info))cerr << "(" << bit_count[tmp] << " " << bit_count[wrow & ((tmp << 1)|(tmp >> 1))] << "\n";
      value += DOUBLE_PAWN_BONUS[ bit_count[tmp] ][ bit_count[wrow & ((tmp << 1)|(tmp >> 1))] ];
    }
    row |= next;
  }


  uchar brow = 0;
  for (int r=1; r<7; r++) brow |= pawn_bitboards[BLACK].u.lines[r];

  row = pawn_bitboards[BLACK].u.lines[1];
  for (int r=2; r<7; r++) {
    uchar next = pawn_bitboards[BLACK].u.lines[r];
    uchar tmp = row & next;
    if (tmp) { 
      //if (FALSE(show_evaluation_info)) cerr << "(" << bit_count[tmp] << " " << bit_count[brow & ((tmp << 1)|(tmp >> 1))] << "\n";
      value -= DOUBLE_PAWN_BONUS[ bit_count[tmp] ][ bit_count[brow & ((tmp << 1)|(tmp >> 1))] ];
    }
    row |= next;
  }

  if (FALSE(show_evaluation_info))
    cerr << "Double_pawns_value = " << value << '\n';
  
  return value;
}

int Eval_3::pawn_structure_value() {
  int *value;
  if (pawn_table.find2(PawnTableHV(pawn_bitboards[WHITE], pawn_bitboards[BLACK]), &value)) {
    if (FALSE(show_evaluation_info))
      cerr << "Pawn structure found in table, value = " << *value << '\n';
    return *value;
  }

  *value = passed_pawns_value();
  *value += double_pawns_value();

  return *value;
}



#include "board_define_position_constants.hxx"
const int POTENTIAL_CASTLING_BONUS[4] = {0, 160, 120, 200};
inline int Eval_3::castling_value() {
  int value = 0;

  // bonus for potential castlings
  value += POTENTIAL_CASTLING_BONUS[(castling & (WHITE_LONG_CASTLING+WHITE_SHORT_CASTLING))>>4];
  value -= POTENTIAL_CASTLING_BONUS[(castling & (BLACK_LONG_CASTLING+BLACK_SHORT_CASTLING))>>6];

  if (FALSE(show_evaluation_info))
    cerr << "Potential castling bonus = " << value << '\n';
  return value;
}
#include "board_undef_position_constants.hxx"


inline int Eval_3::control_value() {
  int value = 0;

  for (int p=0; p<64; p++) {
    int tmp = see.control_measure(p) * control_values[p];
    //if (board[p]) value += 2*tmp; else
    value += tmp;
  }

  if (FALSE(show_evaluation_info)) {
    //cerr << "control_values[positions]:\n"; print_map64(cerr, control_values, 3, 10);
    cerr << "control_measure(positions):\n";
    int m[64];
    for (int i=0; i<64; i++) m[i] = see.control_measure(i);
    print_signed_map64(cerr, m, 3, 10);

    cerr << "bonus for control:\n";
    for (int i=0; i<64; i++) {
      m[i] = (m[i]*control_values[i]) / 32;
      //if (board[i]) m[i] *= 2;
    }
    print_signed_map64(cerr, m, 4, 10);
  }

  return value/32;
}

inline int Eval_3::opening_library_value() {
  if (get_num_pieces() > 26) {
    int value = opening_library->num_occurences(hash_value);
    if (FALSE(show_evaluation_info))
      cerr << "position occur in opening library " << value << " times.\n";
    if (value) {
      value = 32*(floor_log(value)+1);
      if (FALSE(show_evaluation_info))
	cerr << "\tvalue = " << value << '\n';
      return value;
    }
  } else {
    if (FALSE(show_evaluation_info))
      cerr << "opening_library not checked\n";
  }
  return 0;
}


//const bool LONG_RANGE_PIECE[13]= {0, 0,0,1,1,1,0, 0,0,1,1,1,0};
inline int Eval_3::diverse() {
  int value = 0;

  return value;
}


const int CHECKED_PENALTY = 100;
int Eval_3::evaluate(int alpha, int beta) {
  if (!(++num_evaluations & 0xFFFF)) {
    cerr << num_evaluations << " positions evaluated\n";
  }

  int value = piece_value;
  if (FALSE(show_evaluation_info)) cerr << "Value a: " << value << "\n";
  value += pawn_structure_value();
  if (FALSE(show_evaluation_info)) cerr << "Value b: " << value << "\n";

  value += castling_value();
  if (FALSE(show_evaluation_info)) cerr << "Value c: " << value << "\n";
  value += control_value();
  if (FALSE(show_evaluation_info)) cerr << "Value d: " << value << "\n";
  //value += opening_library_value();
  //if (FALSE(show_evaluation_info)) cerr << "Value e: " << value << "\n";
  value += diverse();
  if (FALSE(show_evaluation_info)) cerr << "Value f: " << value << "\n";

  if (player) value = -value;
  if (FALSE(show_evaluation_info)) cerr << "Value g: " << value << "\n";
  // From now on value is for the current player

  if (num_checks) value -= CHECKED_PENALTY;
  if (FALSE(show_evaluation_info)) cerr << "Value h: " << value << "\n";

  if (FALSE(show_evaluation_info))
    cerr << "Evaluation before force progress = " << value << '\n';
  // Force progress
  if (moves_played_since_progress > 36) {
    value *= 100-moves_played_since_progress;
    value /= 64;  // todo >> ?
  }
  if (FALSE(show_evaluation_info))
    cerr << "Evaluation after force progress = " << value << '\n';

  return value;
}


//#######################################################
//#######################################################
//#######################################################


// piece value init stuff

inline int test(int c, int r) {
  return (0<=c && c<8 && 0<=r && r<8);
}

#include "board_define_position_constants.hxx"
void Eval_3::init_piece_values() {
  // game_phase_value=0  =>  OPENING_GAME
  // game_phase_value=256  =>  END_GAME

  for (int p=0; p<64; p++) piece_values[0][p] = 0;
  for (int i=WPAWN; i<=BKING; i++)
    for (int p=0; p<64; p++)
      piece_values[i][p] = PIECE_VALUES[i];

  // BONUS FOR HAVING A ROOK ON 7'TH ROW
  if (game_phase_value < 200)
    for (int c=0; c<7; c++) {
      piece_values[WROOK][CR_TO_POS[c][6]] += 80;
      piece_values[BROOK][CR_TO_POS[c][6]] -= 80;
    }

  // BONUS FOR CASTLING, IF POSSIBLE
  if (castling & WHITE_LONG_CASTLING) {
    piece_values[WKING][c1] += 200;
    piece_values[WROOK][d1] += 100;
  }
  if (castling & WHITE_SHORT_CASTLING) {
    piece_values[WKING][g1] += 300;
    piece_values[WROOK][f1] += 100;
  }
  if (castling & BLACK_LONG_CASTLING) {
    piece_values[BKING][c8] -= 200;
    piece_values[BROOK][d8] -= 100;
  }
  if (castling & BLACK_SHORT_CASTLING) {
    piece_values[BKING][g8] -= 300;
    piece_values[BROOK][f8] -= 100;
  }

  // PAWN STUFF
  piece_values[WPAWN][d2] -= 50;
  piece_values[WPAWN][e2] -= 50;
  piece_values[BPAWN][d7] += 50;
  piece_values[BPAWN][e7] += 50;


  // KEEP QUEEN AT START POSITION IN BEGINNING OF GAME
  if (game_phase_value < 50) {
    piece_values[WQUEEN][d1] += 2*(50-game_phase_value);
    piece_values[BQUEEN][d8] -= 2*(50-game_phase_value);
  }

  // MOVE KING TOWARDS CENTER AT END OF GAME
  if (game_phase_value > 192) {
    for (int p=0; p<64; p++) {
      // 0 <= wking_center_dist <= 14
      int center_dist = abs(2*ROW[p] - 7) + abs(2*COLUMN[p] - 7);
      
      // ()*() <= 64*14
      int bonus = ((game_phase_value-192)*(14-center_dist)) / 16;

      piece_values[WKING][p] += bonus;
      piece_values[BKING][p] -= bonus;
    }
  }

  // DEVELOP KNIGHTS AND BISHOPS EARLY
  if (game_phase_value < 100) {
    int stay_penalty = (game_phase_value < 50) ? 50 : (100-game_phase_value);
    stay_penalty /= 4;

    piece_values[WKNIGHT][b1] -= stay_penalty;
    piece_values[WKNIGHT][g1] -= stay_penalty;
    piece_values[BKNIGHT][b8] += stay_penalty;
    piece_values[BKNIGHT][g8] += stay_penalty;

    piece_values[WBISHOP][c1] -= stay_penalty;
    piece_values[WBISHOP][f1] -= stay_penalty;
    piece_values[BBISHOP][c8] += stay_penalty;
    piece_values[BBISHOP][f8] += stay_penalty;
  }

  // ENCOURAGE FIANCHETTA BISHOPS
  if (game_phase_value < 160) {
    int bonus = (game_phase_value < 96) ? 32 : ((160-game_phase_value)/2);
    piece_values[WBISHOP][g2] += bonus;
    piece_values[BBISHOP][g7] += bonus;
  }

  // MOVE PAWNS FORWARD
  {
    int B1 = (256 - game_phase_value);
    int B2 = game_phase_value;
    for (int c=0; c<8; c++) {
      piece_values[WPAWN][CR_TO_POS[c][6]] += (int)(0.20*B1 + 1.00*B2);
      piece_values[WPAWN][CR_TO_POS[c][5]] += (int)(0.30*B1 + 0.60*B2);
      piece_values[WPAWN][CR_TO_POS[c][4]] += (int)(0.40*B1 + 0.30*B2);
      piece_values[WPAWN][CR_TO_POS[c][3]] += (int)(0.30*B1 + 0.10*B2);
      piece_values[WPAWN][CR_TO_POS[c][2]] += (int)(0.20*B1 + 0.00*B2);

      piece_values[BPAWN][CR_TO_POS[c][1]] -= (int)(0.20*B1 + 1.00*B2);
      piece_values[BPAWN][CR_TO_POS[c][2]] -= (int)(0.30*B1 + 0.60*B2);
      piece_values[BPAWN][CR_TO_POS[c][3]] -= (int)(0.40*B1 + 0.30*B2);
      piece_values[BPAWN][CR_TO_POS[c][4]] -= (int)(0.30*B1 + 0.10*B2);
      piece_values[BPAWN][CR_TO_POS[c][5]] -= (int)(0.20*B1 + 0.00*B2);
    }
  }

  // MOVE PAWNS IN COLUMNS NEAR OPPONENT KING FORWARD
  // (will also give d and e pawn bonus for advancement in beginning)
  {
    int max_bonus = game_phase_value;
    int c;

    c = COLUMN[king_pos[BLACK]];
    if (c != 0) {
      piece_values[WPAWN][CR_TO_POS[c-1][6]] += (int)(0.80 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c-1][5]] += (int)(0.40 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c-1][4]] += (int)(0.20 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c-1][3]] += (int)(0.10 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c-1][2]] += (int)(0.05 * max_bonus);
    }
    piece_values[WPAWN][CR_TO_POS[c][6]] += (int)(0.80 * max_bonus);
    piece_values[WPAWN][CR_TO_POS[c][5]] += (int)(0.40 * max_bonus);
    piece_values[WPAWN][CR_TO_POS[c][4]] += (int)(0.20 * max_bonus);
    piece_values[WPAWN][CR_TO_POS[c][3]] += (int)(0.10 * max_bonus);
    piece_values[WPAWN][CR_TO_POS[c][2]] += (int)(0.05 * max_bonus);
    if (c != 7) {
      piece_values[WPAWN][CR_TO_POS[c+1][6]] += (int)(0.80 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c+1][5]] += (int)(0.40 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c+1][4]] += (int)(0.20 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c+1][3]] += (int)(0.10 * max_bonus);
      piece_values[WPAWN][CR_TO_POS[c+1][2]] += (int)(0.05 * max_bonus);
    }

    c = COLUMN[king_pos[WHITE]];
    if (c != 0) {
      piece_values[BPAWN][CR_TO_POS[c-1][1]] -= (int)(0.80 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c-1][2]] -= (int)(0.40 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c-1][3]] -= (int)(0.20 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c-1][4]] -= (int)(0.10 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c-1][5]] -= (int)(0.05 * max_bonus);
    }
    piece_values[BPAWN][CR_TO_POS[c][1]] -= (int)(0.80 * max_bonus);
    piece_values[BPAWN][CR_TO_POS[c][2]] -= (int)(0.40 * max_bonus);
    piece_values[BPAWN][CR_TO_POS[c][3]] -= (int)(0.20 * max_bonus);
    piece_values[BPAWN][CR_TO_POS[c][4]] -= (int)(0.10 * max_bonus);
    piece_values[BPAWN][CR_TO_POS[c][5]] -= (int)(0.05 * max_bonus);
    if (c != 7) {
      piece_values[BPAWN][CR_TO_POS[c+1][1]] -= (int)(0.80 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c+1][2]] -= (int)(0.40 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c+1][3]] -= (int)(0.20 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c+1][4]] -= (int)(0.10 * max_bonus);
      piece_values[BPAWN][CR_TO_POS[c+1][5]] -= (int)(0.05 * max_bonus);
    }
  }

  // PAWN FORMATIONS NEAR KING if no castling possible
  {
    // assuming white king is on e2:
    int p_e3 = 2*40;
    int p_e4 = 2*15;
    int p_f2 = 2*40;
    int p_f3 = 2*40;
    int p_f4 = 2*15;
    int p_g2 = 2*10;
    int p_g3 = 2*15;
    // PAWN FORMATIONS NEAR KING
    // størst bonus i starten
    if ((castling & (WHITE_CASTLING)) == 0) {
      // white cannot castle.
      // keep king behind pawns
      int c = COLUMN[king_pos[WHITE]];
      int r = ROW[king_pos[WHITE]];
    
      if (test(c, r+1)) piece_values[WPAWN][CR_TO_POS[c][r+1]] += p_e3;
      if (test(c, r+2)) piece_values[WPAWN][CR_TO_POS[c][r+2]] += p_e4;
      if (test(c-1, r)) piece_values[WPAWN][CR_TO_POS[c-1][r]] += p_f2;
      if (test(c+1, r)) piece_values[WPAWN][CR_TO_POS[c+1][r]] += p_f2;
      if (test(c-2, r)) piece_values[WPAWN][CR_TO_POS[c-2][r]] += p_g2;
      if (test(c+2, r)) piece_values[WPAWN][CR_TO_POS[c+2][r]] += p_g2;
      if (test(c-1, r+1)) piece_values[WPAWN][CR_TO_POS[c-1][r+1]] += p_f3;
      if (test(c+1, r+1)) piece_values[WPAWN][CR_TO_POS[c+1][r+1]] += p_f3;
      if (test(c-2, r+1)) piece_values[WPAWN][CR_TO_POS[c-2][r+1]] += p_g3;
      if (test(c+2, r+1)) piece_values[WPAWN][CR_TO_POS[c+2][r+1]] += p_g3;
      if (test(c-1, r+2)) piece_values[WPAWN][CR_TO_POS[c-1][r+2]] += p_f4;
      if (test(c+1, r+2)) piece_values[WPAWN][CR_TO_POS[c+1][r+2]] += p_f4;
    }
    if ((castling & (BLACK_CASTLING)) == 0) {
      // black cannot castle.
      // keep king behind pawns
      int c = COLUMN[king_pos[BLACK]];
      int r = ROW[king_pos[BLACK]];
    
      if (test(c, r-1)) piece_values[BPAWN][CR_TO_POS[c][r-1]] -= p_e3;
      if (test(c, r-2)) piece_values[BPAWN][CR_TO_POS[c][r-2]] -= p_e4;
      if (test(c-1, r)) piece_values[BPAWN][CR_TO_POS[c-1][r]] -= p_f2;
      if (test(c+1, r)) piece_values[BPAWN][CR_TO_POS[c+1][r]] -= p_f2;
      if (test(c-2, r)) piece_values[BPAWN][CR_TO_POS[c-2][r]] -= p_g2;
      if (test(c+2, r)) piece_values[BPAWN][CR_TO_POS[c+2][r]] -= p_g2;
      if (test(c-1, r-1)) piece_values[BPAWN][CR_TO_POS[c-1][r-1]] -= p_f3;
      if (test(c+1, r-1)) piece_values[BPAWN][CR_TO_POS[c+1][r-1]] -= p_f3;
      if (test(c-2, r-1)) piece_values[BPAWN][CR_TO_POS[c-2][r-1]] -= p_g3;
      if (test(c+2, r-1)) piece_values[BPAWN][CR_TO_POS[c+2][r-1]] -= p_g3;
      if (test(c-1, r-2)) piece_values[BPAWN][CR_TO_POS[c-1][r-2]] -= p_f4;
      if (test(c+1, r-2)) piece_values[BPAWN][CR_TO_POS[c+1][r-2]] -= p_f4;
    }
  }

  // PAWN FORMATIONS NEAR KING if only long castling possible
  {
    int p_2 = 80;
    int p_3 = 50;
    int p_2d = 0;//20;
    int p_3d = 0;//10;


    if ((castling & (WHITE_CASTLING)) == WHITE_LONG_CASTLING) {
      // white can only castle long
      piece_values[WPAWN][a2] += p_2;
      piece_values[WPAWN][b2] += p_2;
      piece_values[WPAWN][c2] += p_2;
      piece_values[WPAWN][d2] += p_2d;
      piece_values[WPAWN][a3] += p_3;
      piece_values[WPAWN][b3] += p_3;
      piece_values[WPAWN][c3] += p_3;
      piece_values[WPAWN][d3] += p_3d;
    }
    if ((castling & (BLACK_CASTLING)) == BLACK_LONG_CASTLING) {
      // black can only castle long
      piece_values[BPAWN][a7] -= p_2;
      piece_values[BPAWN][b7] -= p_2;
      piece_values[BPAWN][c7] -= p_2;
      piece_values[BPAWN][d7] -= p_2d;
      piece_values[BPAWN][a6] -= p_3;
      piece_values[BPAWN][b6] -= p_3;
      piece_values[BPAWN][c6] -= p_3;
      piece_values[BPAWN][d6] -= p_3d;
    }
  }


  // PAWN FORMATIONS NEAR KING if only short castling possible
  {
    int p_2 = 90;
    int p_3 = 40;

    if ((castling & (WHITE_CASTLING)) == WHITE_SHORT_CASTLING) {
      // white can only castle short
      piece_values[WPAWN][f2] += p_2;
      piece_values[WPAWN][g2] += p_2;
      piece_values[WPAWN][h2] += p_2;
      piece_values[WPAWN][g3] += p_3;
      piece_values[WPAWN][h3] += p_3;
    }
    if ((castling & (BLACK_CASTLING)) == BLACK_SHORT_CASTLING) {
      // black can only castle short
      piece_values[BPAWN][f7] -= p_2;
      piece_values[BPAWN][g7] -= p_2;
      piece_values[BPAWN][h7] -= p_2;
      piece_values[BPAWN][g6] -= p_3;
      piece_values[BPAWN][h6] -= p_3;
    }
  }


  // PAWN FORMATIONS NEAR KING if both castlings possible
  {
    int p_2l = 60;
    int p_3l = 30;
    int p_2s = 80;
    int p_3s = 35;

    if ((castling & (WHITE_CASTLING)) == WHITE_CASTLING) {
      // white can both castle long and short
      piece_values[WPAWN][a2] += p_2l;
      piece_values[WPAWN][b2] += p_2l;
      piece_values[WPAWN][c2] += p_2l;
      //piece_values[WPAWN][d2] += p_2l;
      piece_values[WPAWN][a3] += p_3l;
      piece_values[WPAWN][b3] += p_3l;
      piece_values[WPAWN][c3] += p_3l;
      //piece_values[WPAWN][d3] += p_3l;

      piece_values[WPAWN][f2] += p_2s;
      piece_values[WPAWN][g2] += p_2s;
      piece_values[WPAWN][h2] += p_2s;
      piece_values[WPAWN][g3] += p_3s;
      piece_values[WPAWN][h3] += p_3s;
    }
    if ((castling & (BLACK_CASTLING)) == BLACK_CASTLING) {
      // black can both castle long and short
      piece_values[BPAWN][a7] -= p_2l;
      piece_values[BPAWN][b7] -= p_2l;
      piece_values[BPAWN][c7] -= p_2l;
      //piece_values[BPAWN][d7] -= p_2l;
      piece_values[BPAWN][a6] -= p_3l;
      piece_values[BPAWN][b6] -= p_3l;
      piece_values[BPAWN][c6] -= p_3l;
      //piece_values[BPAWN][d6] -= p_3l;
 
      piece_values[BPAWN][f7] -= p_2s;
      piece_values[BPAWN][g7] -= p_2s;
      piece_values[BPAWN][h7] -= p_2s;
      piece_values[BPAWN][g6] -= p_3s;
      piece_values[BPAWN][h6] -= p_3s;
    }
  }
}


void Eval_3::init_control_values() {
  const int _DEFAULT[64] =
    {00, 20, 40, 60, 60, 40, 20, 00,
     20, 40, 60, 70, 70, 60, 40, 20,
     40, 60, 80, 90, 90, 80, 60, 40,
     50, 75, 90, 99, 99, 90, 75, 50,
     50, 75, 90, 99, 99, 90, 75, 50,
     40, 60, 80, 90, 90, 80, 60, 40,
     20, 40, 60, 70, 70, 60, 40, 20,
     00, 20, 40, 60, 60, 40, 20, 00};
  
  // 255 - 99 = 156, 156/2 = 78
  // max distance = 7+7
  //const int BLAH[15] = {78, 70, 60, 30, 0, 0, 0,0,0,0,0,0,0,0,0};

  const int BLAH[4][4] =
    {{78, 60, 30, 0},
     {60, 50, 18, 0},
     {30, 18, 5, 0},
     {0, 0, 0, 0}};
  
  for (int p=0; p<64; p++) {
    // king protection not important in opening game
    int king_protection_importance = min(game_phase_value, 1<<7);
    int dc,dr;

    dc = abs(COLUMN[p]-COLUMN[king_pos[WHITE]]);
    dr = abs(ROW[p]-ROW[king_pos[WHITE]]);
    int wking_protection = (dc<4 && dr<4 ? king_protection_importance*BLAH[dc][dr] : 0);

    dc = abs(COLUMN[p]-COLUMN[king_pos[BLACK]]);
    dr = abs(ROW[p]-ROW[king_pos[BLACK]]);
    int bking_protection = (dc<4 && dr<4 ? king_protection_importance*BLAH[dc][dr] : 0);

    control_values[p] = _DEFAULT[p] + ((wking_protection + bking_protection)>>7);

    //int dwk = distance(p, king_pos[WHITE]);
    //int dbk = distance(p, king_pos[BLACK]);
  }

  if (FALSE(show_evaluation_info)) {
    cerr << "control importances:\n";
    print_map64(cerr, control_values, 3, 10);
  }
}

inline bool Eval_3::passed_pawn(Position pos) {
  if (PIECE_KIND[board[pos]] != PAWN) return false;
  
  if (board[pos]==WPAWN) {
    return !(pawn_bitboards[BLACK] & PASSED_PAWN_BITBOARDS[WHITE][pos]).as_bool();
  } else {
    return !(pawn_bitboards[WHITE] & PASSED_PAWN_BITBOARDS[BLACK][pos]).as_bool();
  }
}


// #############################################
// #############################################
// #############################################


void Eval_3::insert_piece(Position pos, Piece piece) {
  if (board[pos]) {
    Piece piece = board[pos];
    piece_value -= piece_values[piece][pos];
    
    if (PIECE_KIND[piece]==PAWN) {
      pawn_bitboards[PIECE_COLOR[piece]].clear_bit(pos);
    }
  }
  piece_value += piece_values[piece][pos];
  if (PIECE_KIND[piece]==PAWN) {
    pawn_bitboards[PIECE_COLOR[piece]].set_bit(pos);
  }

  Board3plus::insert_piece(pos, piece);
}

void Eval_3::remove_piece(Position pos) {
  Piece piece = board[pos];
  piece_value -= piece_values[piece][pos];
  if (PIECE_KIND[piece]==PAWN) {
    pawn_bitboards[PIECE_COLOR[piece]].clear_bit(pos);
  }

  Board3plus::remove_piece(pos);
}


void Eval_3::move_piece(Position from, Position to) {
  if (board[to]) {
    Piece piece = board[to];
    piece_value -= piece_values[piece][to];

    if (PIECE_KIND[piece]==PAWN) {
      pawn_bitboards[PIECE_COLOR[piece]].clear_bit(to);
    }
  }
  Piece piece = board[from];
  piece_value -= piece_values[piece][from];
  piece_value += piece_values[piece][to];
  if (PIECE_KIND[piece]==PAWN) {
    pawn_bitboards[PIECE_COLOR[piece]].clear_bit(from);
    pawn_bitboards[PIECE_COLOR[piece]].set_bit(to);
  }
  Board3plus::move_piece(from, to);
}

#include "board_undef_position_constants.hxx"
