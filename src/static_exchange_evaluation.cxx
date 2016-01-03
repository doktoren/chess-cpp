#include "static_exchange_evaluation.hxx"

#include "board_2_plus.hxx"
#include "piece_values.hxx"

const int BCC_POS[13] =
{0,
    0, 2, 5, 8,11,15,
    16,18,21,24,27,31};
const int BCC_AND[13] =
{0,
    3,7,7,7,15,1,
    3,7,7,7,15,1};

const uint32_t BOARD_CONTROL_CONSTANTS[13] =
{0,
    1<<0, 1<<2, 1<<5, 1<<8, 1<<11, 1<<15,
    1<<16, 1<<18, 1<<21, 1<<24, 1<<27, 1<<31};

const int PIECE_CONTROL_MEASURES[7] =
{0, 4, 3, 3, 2, 2, 2};

uint8_t COMPRESS_CAPTURE_LIST[1<<16];
uint16_t DECOMPRESS_CAPTURE_LIST[CCLL];

void SEE::init_compress_capture_list() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  int index = 0;

  for (int i=0; i<(1<<16); i++) {
    int pawns = i&3;
    int knights = (i>>2)&7;
    int bishops = (i>>5)&7;
    int rooks = (i>>8)&7;
    int queens = (i>>11)&15;
    int kings = i>>15;

    if (pawns<=2  &&  knights<=2  &&  bishops<=1  &&  rooks<=2  &&  queens<=2  &&  kings<=1  &&
        // 3*3*2*3*3*2 = 18*18 = 324
        pawns+knights+bishops+rooks+queens+kings <= 5) {
      //cerr << "(p" << pawns << ",n" << knights << ",b" << bishops << ",r" << rooks << ",q" << queens << ",k" << kings << ")\n";
      COMPRESS_CAPTURE_LIST[i] = ++index;
      DECOMPRESS_CAPTURE_LIST[index] = i;
    } else {
      COMPRESS_CAPTURE_LIST[i] = 0;
    }
  }
  DECOMPRESS_CAPTURE_LIST[0] = 0;

  cerr << "Number of piece defences = " << index << "\n";

  // verify
  for (int i=1; i<CCLL; i++)
    assert(COMPRESS_CAPTURE_LIST[DECOMPRESS_CAPTURE_LIST[i]] == i);
}

uint8_t NUM_PIECES_DEFENDING[CCLL];

void SEE::init_num_pieces_defending() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;
  init_compress_capture_list();//just to be sure, whatever

  NUM_PIECES_DEFENDING[0] = 0;
  for (int i=1; i<CCLL; i++) {
    uint16_t p = DECOMPRESS_CAPTURE_LIST[i];
    //1<<0, 1<<2, 1<<5, 1<<8, 1<<11, 1<<15,
    NUM_PIECES_DEFENDING[i] = (p&3)+((p>>2)&7)+((p>>5)&7)+((p>>8)&7)+((p>>11)&15)+(p>>15);
  }
}

int CONTROL_MEASURE[CCLL];

void SEE::init_control_measure() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;
  init_compress_capture_list();//just to be sure, whatever

  DECOMPRESS_CAPTURE_LIST[0] = 0;
  for (int i=1; i<CCLL; i++) {
    uint16_t p = DECOMPRESS_CAPTURE_LIST[i];

    CONTROL_MEASURE[i] = 0;
    int pawns = p&3;
    CONTROL_MEASURE[i] += PIECE_CONTROL_MEASURES[PAWN] * pawns;
    int knights = (p>>2)&7;
    CONTROL_MEASURE[i] += PIECE_CONTROL_MEASURES[KNIGHT] * knights;
    int bishops = (p>>5)&7;
    CONTROL_MEASURE[i] += PIECE_CONTROL_MEASURES[BISHOP] * bishops;
    int rooks = (p>>8)&7;
    CONTROL_MEASURE[i] += PIECE_CONTROL_MEASURES[ROOK] * rooks;
    int queens = (p>>11)&15;
    CONTROL_MEASURE[i] += PIECE_CONTROL_MEASURES[QUEEN] * queens;
    int kings = p>>15;
    CONTROL_MEASURE[i] += PIECE_CONTROL_MEASURES[KING] * kings;
  }
}


int8_t SEE_LIST[5*CCLL*CCLL];

void SEE::init_see_list() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;
  memset(SEE_LIST, 0, sizeof(char)*5*CCLL*CCLL);

  for (int p=PAWN; p<=QUEEN; p++)
    for (int i=1; i<CCLL; i++) for (int j=1; j<CCLL; j++) {
      assert(COMPRESS_CAPTURE_LIST[DECOMPRESS_CAPTURE_LIST[i]] == i);
      assert(COMPRESS_CAPTURE_LIST[DECOMPRESS_CAPTURE_LIST[j]] == j);
      index_see(p, i, j) = calc_see(p, DECOMPRESS_CAPTURE_LIST[i], DECOMPRESS_CAPTURE_LIST[j]);
    }
}

void SEE::test_see_list() {
  cerr << "Testing see_list...\n";
  for (int p=PAWN; p<=QUEEN; p++)
    for (int i=1; i<CCLL; i++) for (int j=1; j<CCLL; j++) {
      int tmp = calc_see(p, DECOMPRESS_CAPTURE_LIST[i], DECOMPRESS_CAPTURE_LIST[j]);
      if (tmp != index_see(p, i, j)) {
        cerr << PIECE_NAME[p] << ", " << i << ", " << j << ", new = "
            << index_see(p, i, j) << ", old = " << tmp << "\n";
      }
    }
  cerr << "Testing see list done.\n";
}

//############################################

void SEE::print(ostream &os) {
  os << "Static Exchange Evaluation:\n";
  uint8_t npd[64];
  os << "SEE: Num white pieces defending:\n";
  for (int i=0; i<64; i++)
    npd[i] = num_pieces_defending(WHITE, i);
  print_map64(os, npd, 2, 10);
  os << "SEE: Num black pieces defending:\n";
  for (int i=0; i<64; i++)
    npd[i] = num_pieces_defending(BLACK, i);
  print_map64(os, npd, 2, 10);
  os << "SEE: Board control:\n";
  print_map64(os, board_control, 8, 16);
  for (int p=0; p<2; p++) {
    os << "SEE: The " << num_targets[p] << " targets attacked by " << (p?"black\n":"white\n");
    for (int i=0; i<num_targets[p]; i++) {
      os << POS_NAME[target_position(p, i)];
      if (i+1==num_targets[p]) os << "\n";
      else os << " ";
    }
  }
  /*
  os << "SEE: Protect count:\n"
     << "|   |  - |  P  N  B  R  Q  K |  p  n  b  r  q  k |\n"
     << "|---+----+-------------------+-------------------|\n";
  for (int i=1; i<13; i++) {
    os << "| " << PIECE_SCHAR[i] << " | " << toString(protect_count[i][0], 2, 10) << " | ";
    for (int j=1; j<7; j++)
      os << toString(protect_count[i][j], 2, 10) << " ";
    os << "| ";
    for (int j=7; j<13; j++)
      os << toString(protect_count[i][j], 2, 10) << " ";
    os << "|\n";
    if (i==6) os << "|------------------------------------------------|\n";
  }
  os << "SEE: test stuff: ";
  int count = 0;
  for (int i=0; i<64; i++) {
    count += NUM_PIECES_DEFENDING[COMPRESS_CAPTURE_LIST[board_control[i] & 0xFFFF]];
    count += NUM_PIECES_DEFENDING[COMPRESS_CAPTURE_LIST[board_control[i] >> 16]];
  }
  os << count << "\n";
   */
}

void SEE::print_targets(ostream &os) {
  for (int p=0; p<2; p++) {
    os << "SEE: The " << num_targets[p] << " targets attacked by " << (p?"black\n":"white\n");
    for (int i=0; i<num_targets[p]; i++)
      os << "Target " << PIECE_SCHAR[board[target_position(p, i)]] << " on pos "
      << POS_NAME[target_position(p, i)] << " has value "
      << (int)target_value(p, i) << "\n";
  }
}

void SEE::print_compression_list(ostream &os) {
  for (int i=1; i<CCLL; i++) {
    os << i << " ->";
    for (int j=1; j<=6; j++)
      os << " " << PIECE_SCHAR[j]
         << ((DECOMPRESS_CAPTURE_LIST[i] >> BCC_POS[j]) & BCC_AND[j]);
    os << "\n";
  }
}

string SEE::capture_list_to_string(uint16_t cl) {
  string s;
  bool gah = false;
  for (int p=1; p<7; p++) {
    int tmp = (cl >> BCC_POS[p]) & BCC_AND[p];
    if (tmp) {
      if (gah) s += ", ";
      s += toString(tmp) + " " + PIECE_NAME[p];
      gah = true;
    }
  }
  return s;
}


//############################################

int8_t SEE::calc_see(int32_t piece, uint16_t aggressor, uint16_t defender) {
  assert(PAWN<=piece  &&  piece<=QUEEN);

  // ASSUMPTION! num_a + num_b + 1 <= 20
  int gah[20];

  // gah[even] = value relative to aggressor
  // gah[uneven] = value relative to defender
  gah[0] = 0;
  int i = 0;
  int pa = PAWN;
  int pd = PAWN;
  while (aggressor) {
    ++i;
    gah[i] = -gah[i-1] - SEE_PIECE_UNITS[piece];

    while (pa < 7) {
      int tmp = (aggressor >> BCC_POS[pa]) & BCC_AND[pa];
      if (tmp) {
        aggressor -= (1 << BCC_POS[pa]);
        piece = pa;
        break;
      } else {
        ++pa;
      }
    }

    if (defender) {
      ++i;
      gah[i] = -gah[i-1] - SEE_PIECE_UNITS[piece];

      while (pd < 7) {
        int tmp = (defender >> BCC_POS[pd]) & BCC_AND[pd];
        if (tmp) {
          defender -= (1<< BCC_POS[pd]);
          piece = pd;
          break;
        } else {
          ++pd;
        }
      }
    } else {
      break;
    }
  }

  while (i--) {
    if (gah[i] < -gah[i+1]) gah[i] = -gah[i+1];
  }
  /*
  big_output << "calc_see(" << PIECE_CHAR[p] << ", " << toString(a, 4, 16)
	     << ", " << toString(d, 4, 16) << ") = " << (int)(gah[0]) << "\n";

  big_output << "calc_see(" << PIECE_CHAR[p] << ", (" << capture_list_to_string(a)
	     << "), (" << capture_list_to_string(d) << ")) = " << (int)(gah[0]) << "\n";
   */
  return gah[0];
}

void SEE::reevaluate_position(Position pos, Piece piece) {
  int new_see = see(pos, piece);

  if (new_see) {

    if (WHITE_PIECE[piece]) {
      // Black is the attacker
      if (!is_target(pos)) {
        // new target!
        targets[BLACK][num_targets[BLACK]] = pair<int, int>(pos, new_see);
        set_target_ref(pos, BLACK, num_targets[BLACK]++);
        //cerr << "num_targets[white attacker] increased to " << num_targets[BLACK] << " by "
        //     << PPIECE_NAME[piece] << " on " << POS_NAME[pos] << ":\n";
      } else {
        // target already exists - update value
        get_target(pos).second = new_see;
      }

    } else { // White is the attacker

      if (!target_ref[pos]) {
        // new target!
        targets[WHITE][num_targets[WHITE]] = pair<int, int>(pos, new_see);
        set_target_ref(pos, WHITE, num_targets[WHITE]++);
        //cerr << "num_targets[black attacker] increased to " << num_targets[WHITE] << " by "
        //     << PPIECE_NAME[piece] << " on " << POS_NAME[pos] << ":\n";
      } else {
        // target already exists - update value
        get_target(pos).second = new_see;
      }
    }

  } else {

    if (target_ref[pos]) {
      // target must be removed!
      // Fill the gap with the last target in the list
      if (target_color(pos)) {
        get_target(pos) = targets[BLACK][--num_targets[BLACK]];
        //cerr << "num_targets[white attacker] decreased to " << num_targets[BLACK] << " by "
        //     << PPIECE_NAME[piece] << " on " << POS_NAME[pos] << ":\n";

        target_ref[targets[BLACK][num_targets[BLACK]].first] = target_ref[pos];
        target_ref[pos] = 0;

      } else {

        get_target(pos) = targets[WHITE][--num_targets[WHITE]];
        //cerr << "num_targets[white attacker] decreased to " << num_targets[WHITE] << " by "
        //     << PPIECE_NAME[piece] << " on " << POS_NAME[pos] << ":\n";

        target_ref[targets[WHITE][num_targets[WHITE]].first] = target_ref[pos];
        target_ref[pos] = 0;
      }
    }
  }
}

void SEE::increase_control(Move move, Piece piece) {
  if (!(move.blah & CAN_ATTACK)) return;

  //++protect_count[piece][board[move.to]];

  //big_output << "#### increase_control[" << POS_NAME[move.to] << "] " << toString(board_control[move.to], 8, 16) << " -> ";
  board_control[move.to] += BOARD_CONTROL_CONSTANTS[piece];
  //big_output << toString(board_control[move.to], 8, 16) << ", caused by " << move.toString() << "\n";

  if (board[move.to]) reevaluate_position(move.to, board[move.to]);
}

void SEE::decrease_control(Move move, Piece piece) {
  if (!(move.blah & CAN_ATTACK)) return;

  //--protect_count[piece][board[move.to]];

  //big_output << "#### decrease_control[" << POS_NAME[move.to] << "] " << toString(board_control[move.to], 8, 16) << " -> ";
  board_control[move.to] -= BOARD_CONTROL_CONSTANTS[piece];
  //big_output << toString(board_control[move.to], 8, 16) << ", caused by " << move.toString() << "\n";

  // Exact same code as in increase_control
  if (board[move.to]) reevaluate_position(move.to, board[move.to]);
}

// see(piece, pos), piece is on pos
int SEE::see(int pos, int piece) {
  const bool UNTABULATED[13] = {1,0,0,0,0,0,1,0,0,0,0,0,1};
  if (UNTABULATED[piece]) {
    if (!piece) return 0;
    if (piece == WKING) {
      return (board_control[pos] >> 16) ? SEE_WKING_VALUE : 0;
    } else {
      return (board_control[pos] & 0xFFFF) ? SEE_WKING_VALUE : 0;
    }
    /*
    const int UNTABULATED_VALUES[13] = {0, 0,0,0,0,0, SEE_WKING_VALUE, 0,0,0,0,0, SEE_WKING_VALUE};
    return UNTABULATED_VALUES[piece];
     */
  }
  assert(WPAWN <= piece  &&  piece <= BKING);
  assert(0<=pos  &&  pos<64);

  int white_cl = COMPRESS_CAPTURE_LIST[board_control[pos] & 0xFFFF];
  int black_cl = COMPRESS_CAPTURE_LIST[board_control[pos] >> 16];
  //cerr << "(w_cl=" << white_cl << ", b_cl=" << black_cl << ")";

  if (!white_cl || !black_cl) {
    if (DEBUG) {
      // cerr << "remark: see::see(" << POS_NAME[pos] << ", " << PIECE_SCHAR[piece] << ") not tabulated\n";
      /*
      board.print_board(cerr);
      print(cerr);
      cerr << "bc(" << toString(board_control[pos], 8, 16) << "), w_cl=" << white_cl << ", b_cl=" << black_cl << "\n";
      assert(0);
       */
    }
    if (WHITE_PIECE[piece]) {
      // black aggressor
      return UNIT_FACTOR * calc_see(PIECE_KIND[piece], board_control[pos] >> 16, board_control[pos] & 0xFFFF);
    } else {
      // white aggressor
      return UNIT_FACTOR * calc_see(PIECE_KIND[piece], board_control[pos] & 0xFFFF, board_control[pos] >> 16);
    }

  } else {

    if (WHITE_PIECE[piece]) {
      // black aggressor
      return UNIT_FACTOR * index_see(PIECE_KIND[piece], black_cl, white_cl);
    } else {
      // white aggressor
      return UNIT_FACTOR * index_see(PIECE_KIND[piece], white_cl, black_cl);
    }
  }
}

int SEE::best_see_capture_value(int player) {
  int best = 0;
  for (int i=0; i<num_targets[player]; i++) {
    if (target_value(player, i) > best)
      best = target_value(player, i);
  }
  return best;
}

/*
// best_capture - worst_loss
// -second_worst_loss
int SEE::see_result(int player) {

  // This doesn't work:
  //      a b c d e f g h
  //  +-----------------+    |
  //8 | r n b q k b   r | 8  | 5w
  //7 | p p p   p p p p | 7  |
  //6 |                 | 6  | White can still castle
  //5 |       p         | 5  | Black can still castle
  //4 |       P       B | 4  |
  //3 |           P     | 3  | moves played since progress = 1
  //2 | P P P   P n P P | 2  |
  //1 | R N   Q K B N R | 1  |
  //  +-----------------+    |
  //    a b c d e f g h
  //SEE: The 1 targets attacked by white
  //Target n on pos f2 has value 3072
  //SEE: The 2 targets attacked by black
  //Target Q on pos d1 has value 6144
  //Target R on pos h1 has value 5504

  switch (size(player^1)) {
  case 0:
    // If no weak pieces, then result is best attack
    return best_see_capture_value(player);
  case 1:
    // Player choses best of 2 options
    // Protect own piece (assumed possible), value = 0
    // Perform most valuable attack and lose piece
    {
      int attack_value = best_see_capture_value(player);
      if (attack_value > 0) return attack_value;
      return 0;
    }
  default:
    // Player choses best of 2 options
    // 1) Protects worst defended piece (assumed possible), player loses
    //    second worst defended piece
    // 2) Perform most valuable attack and lose worst defended piece
    int attack_value = best_see_capture_value(player);
    int worst_loss = 0;
    int second_worst_loss = 0;
    for (int i=0; i<num_targets[player^1]; i++) {
      if (target_value(player^1, i) > second_worst_loss)
	if (target_value(player^1, i) > worst_loss) {
	  second_worst_loss = worst_loss;
	  worst_loss = target_value(player^1, i);
	} else {
	  second_worst_loss = target_value(player^1, i);
	}
    }
    if (attack_value - worst_loss > -second_worst_loss) {
      return attack_value - worst_loss;
    } else {
      return -second_worst_loss;
    }
  }
}
 */

int SEE::move_result(Move move) {
  assert(board.move_to_index.defined(move));

  if (move.blah & CAN_ATTACK) {
    int result = 0;
    Piece piece = board[move.from];
    if (is_target(move.from)) {
      // Moving piece was previous a target
      // cerr << "Moving piece was previous a target\n";
      result += get_target(move.from).second;
    }
    if (board[move.to]) {
      // Piece captured
      // cerr << "Piece captured\n";
      result += ABS_PIECE_VALUES[board[move.to]];
    }

    // Penalty for being new target:
    board_control[move.to] -= BOARD_CONTROL_CONSTANTS[piece];
    // cerr << "see = " << see(move.to, piece) << "\n";
    result -= see(move.to, piece);
    board_control[move.to] += BOARD_CONTROL_CONSTANTS[piece];

    return result;

  } else {

    int result = 0;
    Piece piece = board[move.from];
    if (is_target(move.from)) {
      // Moving piece was previous a target
      // cerr << "Moving piece was previous a target\n";
      result += get_target(move.from).second;
    }

    // Penalty for being new target:
    result -= see(move.to, piece);

    return result;
  }
}
