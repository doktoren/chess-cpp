#include <iostream>
#include <queue>
#include <assert.h>

#include "board.hxx"

#include "endgame_castling.hxx"
#include "endgame_en_passant.hxx"

const bool WHITE_PIECE[13] =
{   false,
    true, true, true, true, true, true,
    false, false, false, false, false, false
};

const bool BLACK_PIECE[13] =
{   false,
    false, false, false, false, false, false,
    true, true, true, true, true, true
};

const int PIECE_COLOR[13] =
{   -1,
    false, false, false, false, false, false,
    true, true, true, true, true, true
};

const Piece PIECE_KIND[13] =
{   0,
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

const Piece SWAP_PIECE_COLOR[13] =
{   0,
    BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING,
    WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING
};

const uchar CASTLING[64] =
{   0,0,WHITE_LONG_CASTLING,0,WHITE_LONG_CASTLING+WHITE_SHORT_CASTLING,0,WHITE_SHORT_CASTLING,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,BLACK_LONG_CASTLING,0,BLACK_LONG_CASTLING+BLACK_SHORT_CASTLING,0,BLACK_SHORT_CASTLING,0
};

const uchar CASTLING_LOST[64] =
{   0xFF - WHITE_LONG_CASTLING,0xFF,0xFF,
    0xFF,0xFF - (WHITE_LONG_CASTLING+WHITE_SHORT_CASTLING),
    0xFF,0xFF,0xFF - WHITE_SHORT_CASTLING,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF - BLACK_LONG_CASTLING,0xFF,0xFF,
    0xFF,0xFF - (BLACK_LONG_CASTLING+BLACK_SHORT_CASTLING),
    0xFF,0xFF,0xFF - BLACK_SHORT_CASTLING
};

const int ROW[64] =
{   0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7
};

const int COLUMN[64] =
{   0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7
};

const bool POS_COLOR[64] =
{   1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1
};

const Position CR_TO_POS[8][8] = // Colum, Row
{   {  0, 8,16,24,32,40,48,56},
    { 1, 9,17,25,33,41,49,57},
    { 2,10,18,26,34,42,50,58},
    { 3,11,19,27,35,43,51,59},
    { 4,12,20,28,36,44,52,60},
    { 5,13,21,29,37,45,53,61},
    { 6,14,22,30,38,46,54,62},
    { 7,15,23,31,39,47,55,63}
};

/*
struct {
  uchar pieces_left[2];
  uchar num_pieces_left;
  uchar num_non_zugzwang_pieces_left;// 4 bits for each side
} individual;
 */
const uint PIECE_COUNT_CONSTANTS[13] =
{   0,

    0x00010001,
    0x01010001,
    0x01010001,
    0x01010001,
    0x01010001,
    0x00010001,

    0x00010100,
    0x10010100,
    0x10010100,
    0x10010100,
    0x10010100,
    0x00010100
};

/*
struct {
  bool insufficient_material_b;
  bool insufficient_material_a;
  ushort endgame_hashing;
} individual;
 */
const uint ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[13][2] = // [piece][POS_COLOR[pos]]
{   {0,0},
    {0x0101 + (DB_WPAWN_VALUE<<16), 0x0101 + (DB_WPAWN_VALUE<<16)},
    {0x00E1 + (DB_WKNIGHT_VALUE<<16), 0x00E1 + (DB_WKNIGHT_VALUE<<16)},
    {0x0021 + (DB_WBISHOP_VALUE<<16), 0x0100 + (DB_WBISHOP_VALUE<<16)},
    {0x0101 + (DB_WROOK_VALUE<<16), 0x0101 + (DB_WROOK_VALUE<<16)},
    {0x0101 + (DB_WQUEEN_VALUE<<16), 0x0101 + (DB_WQUEEN_VALUE<<16)},
    {0x0000 + (DB_WKING_VALUE<<16), 0x0000 + (DB_WKING_VALUE<<16)},
    {0x0101 + (DB_BPAWN_VALUE<<16), 0x0101 + (DB_BPAWN_VALUE<<16)},
    {0x00E1 + (DB_BKNIGHT_VALUE<<16), 0x00E1 + (DB_BKNIGHT_VALUE<<16)},
    {0x0021 + (DB_BBISHOP_VALUE<<16), 0x0100 + (DB_BBISHOP_VALUE<<16)},
    {0x0101 + (DB_BROOK_VALUE<<16), 0x0101 + (DB_BROOK_VALUE<<16)},
    {0x0101 + (DB_BQUEEN_VALUE<<16), 0x0101 + (DB_BQUEEN_VALUE<<16)},
    {0x0000 + (DB_BKING_VALUE<<16), 0x0000 + (DB_BKING_VALUE<<16)}
};


const string game_result_texts[4] = {"*", "1/2-1/2", "1-0", "0-1"};

const string game_status_texts[7] =
{   "* {Game still open}",
    "1/2-1/2 {Fifty move rule}",
    "1-0 {White mates}",
    "0-1 {Black mates}",
    "1/2-1/2 {Stalemate}",
    "1/2-1/2 {Draw by repetition}",
    "1/2-1/2 {Insufficient material}"
};


//######################################################

const char PIECE_CHAR[13] =
{' ','P','N','B','R','Q','K','p','n','b','r','q','k'};
const string PIECE_SCHAR[13] =
{" ","P","N","B","R","Q","K","p","n","b","r","q","k"};

const string PIECE_NAME[13] =
{   "no piece",
    "Pawn","Knight","Bishop","Rook","Queen","King",
    "pawn","knight","bishop","rook","queen","king"
};
const string PPIECE_NAME[13] =
{   "no piece",
    "white pawn","white knight","white bishop","white rook","white queen","white king",
    "black pawn","black knight","black bishop","black rook","black queen","black king"
};

const char PLAYER_CHAR[2] = {'w','b'};
const string PLAYER_NAME[2] = {"white","black"};


const string POS_NAME[66] =
{   "a1","b1","c1","d1","e1","f1","g1","h1",
    "a2","b2","c2","d2","e2","f2","g2","h2",
    "a3","b3","c3","d3","e3","f3","g3","h3",
    "a4","b4","c4","d4","e4","f4","g4","h4",
    "a5","b5","c5","d5","e5","f5","g5","h5",
    "a6","b6","c6","d6","e6","f6","g6","h6",
    "a7","b7","c7","d7","e7","f7","g7","h7",
    "a8","b8","c8","d8","e8","f8","g8","h8",
    "##","##"
};

const string COLUMN_NAME[66] =
{   "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "#","#"
};

const string ROW_NAME[66] =
{   "1","1","1","1","1","1","1","1",
    "2","2","2","2","2","2","2","2",
    "3","3","3","3","3","3","3","3",
    "4","4","4","4","4","4","4","4",
    "5","5","5","5","5","5","5","5",
    "6","6","6","6","6","6","6","6",
    "7","7","7","7","7","7","7","7",
    "8","8","8","8","8","8","8","8",
    "#","#"
};

//######################################################

Position REFLECTION_TABLE[8*64] =
{   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
    7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24,39,38,37,36,35,34,33,32,47,46,45,44,43,42,41,40,55,54,53,52,51,50,49,48,63,62,61,60,59,58,57,56,
    56,57,58,59,60,61,62,63,48,49,50,51,52,53,54,55,40,41,42,43,44,45,46,47,32,33,34,35,36,37,38,39,24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,
    63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
    0,8,16,24,32,40,48,56,1,9,17,25,33,41,49,57,2,10,18,26,34,42,50,58,3,11,19,27,35,43,51,59,4,12,20,28,36,44,52,60,5,13,21,29,37,45,53,61,6,14,22,30,38,46,54,62,7,15,23,31,39,47,55,63,
    56,48,40,32,24,16,8,0,57,49,41,33,25,17,9,1,58,50,42,34,26,18,10,2,59,51,43,35,27,19,11,3,60,52,44,36,28,20,12,4,61,53,45,37,29,21,13,5,62,54,46,38,30,22,14,6,63,55,47,39,31,23,15,7,
    7,15,23,31,39,47,55,63,6,14,22,30,38,46,54,62,5,13,21,29,37,45,53,61,4,12,20,28,36,44,52,60,3,11,19,27,35,43,51,59,2,10,18,26,34,42,50,58,1,9,17,25,33,41,49,57,0,8,16,24,32,40,48,56,
    63,55,47,39,31,23,15,7,62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,60,52,44,36,28,20,12,4,59,51,43,35,27,19,11,3,58,50,42,34,26,18,10,2,57,49,41,33,25,17,9,1,56,48,40,32,24,16,8,0
};

//######################################################

const int ENDGAME_HASHING_CONSTANTS[13] =
{   0,
    DB_WPAWN_VALUE, DB_WKNIGHT_VALUE, DB_WBISHOP_VALUE,
    DB_WROOK_VALUE, DB_WQUEEN_VALUE, DB_WKING_VALUE,
    DB_BPAWN_VALUE, DB_BKNIGHT_VALUE, DB_BBISHOP_VALUE,
    DB_BROOK_VALUE, DB_BQUEEN_VALUE, DB_BKING_VALUE
};

//######################################################

Board::Board() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Board constructor called\n";

  // Initialize board[64..75] to contain the different pieces
  // (optimizes move generator)
  for (Piece p = PAWN; p <= KING; p++) {
    board[62+2*p] = p;
    board[63+2*p] = p+6;
  }
}

void Board::reset_all() {
  // Reset all values.
  for (int i=0; i<64; i++) board[i]=0;

  en_passant = ILLEGAL_POS;
  castling = WHITE_LONG_CASTLING + WHITE_SHORT_CASTLING +
      BLACK_LONG_CASTLING + BLACK_SHORT_CASTLING;
  player = WHITE;

  moves_played = 0;
  moves_played_since_progress = 0;

  piece_count.as_pattern = 0;
  endgame_hashing_insufficient_material.as_pattern = 0;

  played_from_scratch = true; // whatever
  // cerr << "reset_all: Setting played_from_scratch = true\n";
}

Board::~Board() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Board destructor called.\n";
}

void Board::new_game() {
  loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  played_from_scratch = true;
  // cerr << "new_game: Setting played_from_scratch = true\n";
}

//######################################################


// Returns false if the board setup is not legal. The following is checked
// a) That no 2 pieces occupy the same square
// b) That the 2 kings is not to near
// c) That the player to move cannot capture the opponent king
// d) If an en passant is possible, then the 2 squares it has passed must be empty.
//    Furthermore, if this en passant is encoded in the position of 2 pawns,
//    then it must have the unique encoding.
//
// set_board checks whether castling/en passant capabilities are encoded in the
// piece positions (if so, _castling and _en_passant must have default values).
// If so, piece_list, _castling and _en_passant are updated accordingly.
// piece_list might be changed if piece overlap encodes castling
bool Board::set_board(vector<PiecePos> &piece_list, int player_turn,
    int &_castling, int &_en_passant,
    int _moves_played_since_progress, int full_move_number) {
#ifndef NDEBUG
  assert(full_move_number > 0);
  assert(piece_list.size() <= 32);
  for (uint i=0; i<piece_list.size(); i++) {
    assert(piece_list[i].pos < 64);
    assert(0<piece_list[i].piece && piece_list[i].piece<=12);
  }
  assert(player_turn==0 || player_turn==1);
  assert(0<=_castling && _castling<16);
  assert((0<=_en_passant && _en_passant<64) || _en_passant==ILLEGAL_POS);
  assert(0<=_moves_played_since_progress);
  assert(0<=full_move_number);
#endif
  reset_all();

  int king_pos[2];
#define wking_pos king_pos[WHITE]
#define bking_pos king_pos[BLACK]
  wking_pos = bking_pos = -1;

  // Find position of kings
  for (uint i=0; i<piece_list.size(); i++) {
    if (piece_list[i].piece == WKING) {
      assert(wking_pos == -1);
      wking_pos = piece_list[i].pos;
    }
    if (piece_list[i].piece == BKING) {
      assert(bking_pos == -1);
      bking_pos = piece_list[i].pos;
    }
  }
  assert(wking_pos != -1  &&  bking_pos != -1);

  if (abs(COLUMN[wking_pos]-COLUMN[bking_pos]) <= 1  &&
      abs(ROW[wking_pos]-ROW[bking_pos]) <= 1) return false;// kings to close

  Position en_passant_unique_encoding = 0;

  { // Decode castling/en passant
    Piece tmp_board[64];
    memset(tmp_board, 0, 64);
    tmp_board[wking_pos] = WKING;
    tmp_board[bking_pos] = BKING;

    int transf = -1;

    for (uint i=0; i<piece_list.size(); i++) {
      if (tmp_board[piece_list[i].pos]  &&  PIECE_KIND[piece_list[i].piece] != KING) {
        // piece overlap!

#if ENDGAME_TABLE_WITH_CASTLING==1
        // Encoding of castling?
        if (PIECE_KIND[piece_list[i].piece] == ROOK) {

          if (PIECE_KIND[tmp_board[piece_list[i].pos]] == KING) {
            bool rook_color = BLACK_PIECE[piece_list[i].piece];
            bool king_color = BLACK_PIECE[tmp_board[piece_list[i].pos]];

            // The board must be transformed such that the king of the
            // same color as the rook is mapped to e1 or e8 (for white resp. black)
            int new_transf = KING_REFLECTIONS[rook_color][king_pos[rook_color]];
            if (new_transf == -1  ||  (transf!=-1  &&  transf!=new_transf)) {
              // Either king not on original position. Or two castlings encoded, but they
              // require different transformations of the board.
              return false;
            }

            // If castling encoded in position of pieces, it may not be specified
            // also in _castling
            assert(transf!=-1  ||  _castling == 0);

            // Update the position of the rook!
            if (rook_color) {
              if (king_color) {
                // black rook on own king => short castling
                piece_list[i].pos = DECODE_SHORT_CASTLING_ROOK[bking_pos];
                _castling |= BLACK_SHORT_CASTLING;
              } else {
                // black rook on opponent king => long castling
                piece_list[i].pos = DECODE_LONG_CASTLING_ROOK[bking_pos];
                _castling |= BLACK_LONG_CASTLING;
              }
            } else {
              if (king_color) {
                // white rook on opponent king => long castling
                piece_list[i].pos = DECODE_LONG_CASTLING_ROOK[wking_pos];
                _castling |= WHITE_LONG_CASTLING;
              } else {
                // white rook on own king => short castling
                piece_list[i].pos = DECODE_SHORT_CASTLING_ROOK[wking_pos];
                _castling |= WHITE_SHORT_CASTLING;
              }
            }

            if (tmp_board[piece_list[i].pos]) {
              // piece overlap after decoding the rook
              return false;
            }

            transf = new_transf;

          } else return false;

        } else
#endif
          // Encoding of en passant?
          if (PIECE_KIND[piece_list[i].piece] == PAWN  &&
              PIECE_KIND[tmp_board[piece_list[i].pos]] == PAWN) {

            if (_en_passant != ILLEGAL_POS) {
              //cerr << "A position that encodes more than 1 en passant is illegal\n";
              return false;
            }

            // Find the index of the first pawn in piece_list
            int pi=0;
            while (piece_list[pi].pos != piece_list[i].pos) ++pi;

            // Remove this pawn temporary from tmp_board
            tmp_board[piece_list[pi].pos] = 0;

            // Find the decoded positions of the pawns and update _en_passant
            if (WHITE_PIECE[piece_list[pi].piece]) {
              _en_passant = retrieve_en_passant(piece_list[pi].pos, piece_list[i].pos);
            } else {
              _en_passant = retrieve_en_passant(piece_list[i].pos, piece_list[pi].pos);
            }

            // Make sure these new positions are not occupied
            if (tmp_board[piece_list[pi].pos] || tmp_board[piece_list[i].pos]) {
              //cerr << "The pawns may not be mapped to positions already taken\n";
              return false;
            }

            // Put the first pawn back on tmp_board
            // (dont care which color it was - all we need is to mark the square taken).
            tmp_board[piece_list[pi].pos] = PAWN;

            if (!_en_passant) {
              //cerr << "The square of the double pawns corresponds to no en passant constellation.\n";
              return false;
            }


            // If the square en_passant_unique_encoding contains a pawn of stm,
            // then this pawn should have been used to encode the en passant.
            if (COLUMN[_en_passant] > 3) {
              if (COLUMN[piece_list[i].pos] > COLUMN[_en_passant]) {
                en_passant_unique_encoding = piece_list[i].pos-2;
              } else if (COLUMN[piece_list[pi].pos] > COLUMN[_en_passant]) {
                en_passant_unique_encoding = piece_list[pi].pos-2;
              }
            } else {//COLUMN[_en_passant] <= 3
              if (COLUMN[piece_list[i].pos] < COLUMN[_en_passant]) {
                en_passant_unique_encoding = piece_list[i].pos+2;
              } else if (COLUMN[piece_list[pi].pos] < COLUMN[_en_passant]) {
                en_passant_unique_encoding = piece_list[pi].pos+2;
              }
            }


          } else {

            //cerr << "Overlapping pieces.\n";
            return false;
          }

      }

      // The kings have already been placed on tmp_board, but whatever
      tmp_board[piece_list[i].pos] = piece_list[i].piece;
    }

    if (transf != -1) {

      //cerr << "transf != -1\n";

      if (transf >= 2) {
        // Make sure it is a pawnfree endgame
        for (uint i=0; i<piece_list.size(); i++)
          if (PIECE_KIND[piece_list[i].piece] == PAWN) {
            //cerr << "This transformation is not allowed with pawns!\n";
            return false;
          }
      }

      // Do the transformation
      for (uint i=0; i<piece_list.size(); i++)
        piece_list[i].pos = reflect(piece_list[i].pos, transf);
      wking_pos = reflect(wking_pos, transf);
      bking_pos = reflect(bking_pos, transf);


      if (_en_passant != ILLEGAL_POS)
        _en_passant = reflect(_en_passant, transf);
    }
  }

  place_kings(wking_pos, bking_pos);

#undef wking_pos
#undef bking_pos

  for (unsigned int i=0; i<piece_list.size(); i++) {
    if (PIECE_KIND[piece_list[i].piece] != KING) {
      player = BLACK_PIECE[piece_list[i].piece];
      insert_piece(piece_list[i].pos, piece_list[i].piece);
    }
  }

  player = player_turn;
  castling = _castling;
  en_passant = _en_passant;
  moves_played_since_progress = _moves_played_since_progress;
  moves_played = 2*(full_move_number-1) + player;

  if (en_passant != ILLEGAL_POS) {
    if (board[en_passant]) {
      //cerr << "en passant square occupied or en passant in wrong side of board?\n";
      return false;
    }
    if (player) {
      if (ROW[en_passant]!=2  || board[en_passant - 8]) {
        //cerr << "ep2 " << POS_NAME[en_passant] << " " << player << "\n";
        return false;
      }
    } else {
      if (ROW[en_passant]!=5  || board[en_passant + 8]) {
        //cerr << "ep3\n";
        return false;
      }
    }

    if (en_passant_unique_encoding  &&
        ((player  &&  board[en_passant_unique_encoding] == BPAWN)  ||
            (!player  &&  board[en_passant_unique_encoding] == WPAWN))) {
      //cerr << "encoding of en passant must have the unique format.\n";
      return false;
    }
  }

  return internal_set_board();
}

vector<PiecePos> Board::get_piece_list() {
  int index = 0;
  for (int p=0; p<64; p++)
    if (board[p]) ++index;
  vector<PiecePos> result(index);
  index = 0;
  for (int p=0; p<64; p++)
    if (board[p])
      result[index++] = PiecePos(board[p], p);
  return result;
}


// get_encoded_piece_list encodes en passant and castling in the position of the pieces
void Board::get_encoded_piece_list(vector<PiecePos> &l) const {

#if ENDGAME_TABLE_WITH_CASTLING==1
  if (en_passant_possible() || castling)
#else
    if (en_passant_possible())
#endif
    {
      bool exceptions[64];
      memset(exceptions, 0, 64);
      int index = 0;

      if (en_passant_possible()) {
        //cerr << "en passant possible...\n";
        int column = COLUMN[en_passant];

        // If the en passant can be encoded in 2 different ways,
        // use the black pawn closest to the d/e file.
        int dc = column>3 ? -1 : 1;

        if (player) {
          assert(ROW[en_passant]==2);
          assert(board[en_passant+8]==WPAWN);

          // white just moved 2 forward

          // First try pawn closest to the d/e file. If there
          // is no black pawn here, then it must be on the other side.
          if (board[en_passant+8+dc]==BPAWN &&
              (column!=0 || dc!=-1)  &&  (column!=7 || dc!=1)) {
            assert((uint)index < l.size());

            Position tmp = AFTER_WHITE_PAWN_ADVANCED[column][column+dc];
            l[index++] = PiecePos(WPAWN, tmp);
            l[index++] = PiecePos(BPAWN, tmp);
            exceptions[en_passant+8] = exceptions[en_passant+8+dc] = true;
          } else {
            assert(board[en_passant+8-dc]==BPAWN);
            assert((column!=0 || -dc!=-1)  &&  (column!=7 || -dc!=1));
            assert((uint)index < l.size());

            Position tmp = AFTER_WHITE_PAWN_ADVANCED[column][column-dc];
            l[index++] = PiecePos(WPAWN, tmp);
            l[index++] = PiecePos(BPAWN, tmp);
            exceptions[en_passant+8] = exceptions[en_passant+8-dc] = true;
          }
        } else {
          assert(ROW[en_passant]==5);
          assert(board[en_passant-8]==BPAWN);

          // black just moved 2 forward
          if (board[en_passant-8+dc]==WPAWN &&
              (column!=0 || dc!=-1)  &&  (column!=7 || dc!=1)) {
            assert((uint)index < l.size());

            Position tmp = AFTER_BLACK_PAWN_ADVANCED[column][column+dc];
            l[index++] = PiecePos(WPAWN, tmp);
            l[index++] = PiecePos(BPAWN, tmp);
            exceptions[en_passant-8] = exceptions[en_passant-8+dc] = true;
          } else {
            assert(board[en_passant-8-dc]==WPAWN);
            assert((column!=0 || -dc!=-1)  &&  (column!=7 || -dc!=1));
            assert((uint)index < l.size());

            Position tmp = AFTER_BLACK_PAWN_ADVANCED[column][column-dc];
            l[index++] = PiecePos(WPAWN, tmp);
            l[index++] = PiecePos(BPAWN, tmp);
            exceptions[en_passant-8] = exceptions[en_passant-8-dc] = true;
          }
        }
      }

#if ENDGAME_TABLE_WITH_CASTLING==1
      if (castling) {
        //cerr << "castling possible...\n";
#include "board_define_position_constants.hxx"
        if (castling & WHITE_LONG_CASTLING) {
          if (board[a1]!=WROOK) print_board(cerr);
          assert(board[a1]==WROOK);
          assert(board[e1]==WKING);
          exceptions[a1] = true;
          assert((uint)index < l.size());
          { // Find black king
            int i=64;
            while (board[--i] != BKING) ;
            assert(i<64);
            l[index++] = PiecePos(WROOK, i);
          }
        }
        if (castling & WHITE_SHORT_CASTLING) {
          assert(board[h1]==WROOK);
          assert(board[e1]==WKING);
          exceptions[h1] = true;
          assert((uint)index < l.size());
          l[index++] = PiecePos(WROOK, e1);
        }

        if (castling & BLACK_LONG_CASTLING) {
          assert(board[a8]==BROOK);
          assert(board[e8]==BKING);
          exceptions[a8] = true;
          assert((uint)index < l.size());
          { // Find white king
            int i=-1;
            while (board[++i] != WKING) ;
            assert(i<64);
            l[index++] = PiecePos(BROOK, i);
          }
        }
        if (castling & BLACK_SHORT_CASTLING) {
          assert(board[h8]==BROOK);
          assert(board[e8]==BKING);
          exceptions[h8] = true;
          assert((uint)index < l.size());
          l[index++] = PiecePos(BROOK, e8);
        }
#include "board_undef_position_constants.hxx"
      }
#endif

      for (int i=0; i<64; i++)
        if (board[i]  &&  !exceptions[i]) {
          assert((uint)index < l.size());
          l[index++] = PiecePos(board[i], i);
        }

      assert((uint)index == l.size());

      /*
      for(uint i=0;i<l.size();i++)
	cerr<<"("<<PIECE_NAME[l[i].piece]<<","<<POS_NAME[l[i].pos]<<")";
      cerr<<"\n";
       */
    }
    else { // No en passant or castling possible
      return get_piece_list(l);
    }
}


//######################################################


// http://en.wikipedia.org/wiki/Forsyth-Edwards_Notation
#define reset_and_return \
    {loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); \
    return false; }
#define IS_NULL if (!FEN[index]) { \
    cerr << "loadFEN: unexpected end of fen description.\n"; \
    reset_and_return; \
}
bool Board::loadFEN(string FEN) {
  // TODO: Check if FEN is valid
  // reset_all(); delayed until after some checks

  int index=0;

  //cerr << "16.1.3.1: Piece placement data\n";
  {
    Piece m[64];
    memset(m, 0, 64);
    Position king_pos[2];
    king_pos[0] = king_pos[1] = ILLEGAL_POS;

    int pos = 0;
    int pos_offset = 56;

    while (FEN[index]  &&  FEN[index] != ' ') {
      if (pos_offset == -8) {
        cerr << "loadFEN: Error, too many rows\n";
        reset_and_return;
      }
      Piece p = char_to_piece(FEN[index]);
      if (p) {
        if (PIECE_KIND[p] == KING) {
          if (king_pos[BLACK_PIECE[p]] != ILLEGAL_POS) {
            cerr << "loadFEN: Error, to kings of same color\n";
            reset_and_return;
          }
          king_pos[BLACK_PIECE[p]] = pos_offset | pos;
        } else m[pos_offset | pos] = p;
        if (++pos==8) {
          if (pos_offset  &&  FEN[++index] != '/') {
            cerr << "loadFEN: Error, missing a \"/\" on rank "
                << (int)(1+(pos_offset>>3)) << "\n";
            reset_and_return;
          }

          pos = 0;
          pos_offset -= 8;
        }
      } else if (FEN[index] == '/') {
        // consume rest of line
        pos = 0;
        pos_offset -= 8;
      } else {
        if (FEN[index]<'0' || '9'<FEN[index]) {
          cerr << "loadFEN: Error, illegal character " << FEN[index]
                                                              << ", (code " << (int)FEN[index] << ") on rank "
                                                              << (int)(1+(pos_offset>>3)) << "\n";
          reset_and_return;
        }
        pos += FEN[index]-'0';
        if (pos > 8) {
          cerr << "loadFEN: Error, too many squares on rank "
              << (int)(1+(pos_offset>>3)) << "\n";
          reset_and_return;
        }
        if (pos==8) {
          if (pos_offset  &&  FEN[++index]!='/') {
            cerr << "loadFEN: Error, missing a \"/\" on rank "
                << (int)(1+(pos_offset>>3)) << "\n";
            reset_and_return;
          }

          pos = 0;
          pos_offset -= 8;
        }
      }

      index++;

    }

    IS_NULL;

    if (pos_offset > 0) {
      cerr << "loadFEN: Error, too few rows\n";
      reset_and_return;
    }

    if ((king_pos[WHITE] | king_pos[BLACK]) & 0x40) {
      cerr << "loadFEN: Error, missing a king\n";
      reset_and_return;
    }

    if (abs(COLUMN[king_pos[WHITE]]-COLUMN[king_pos[BLACK]]) <= 1  &&
        abs(ROW[king_pos[WHITE]]-ROW[king_pos[BLACK]]) <= 1) {
      cerr << "loadFEN: Error, kings to close\n";
      reset_and_return;
    }

    reset_all();
    place_kings(king_pos[WHITE], king_pos[BLACK]);

    for (pos = 0; legal_pos(pos); pos++)
      if (m[pos]) {
        // player must be correctly set when calling insert_piece
        player = BLACK_PIECE[m[pos]];
        insert_piece(pos, m[pos]);
      }
  }

  //cerr << "16.1.3.2: Active color\n";
  ++index;
  IS_NULL;
  player = (FEN[index++] == 'b');

  castling = 0;
  en_passant = ILLEGAL_POS;
  if (FEN[index]) {
    IS_NULL;

    //cerr << "16.1.3.3: Castling availability\n";
    ++index;
    if (!FEN[index]) {
      cerr << "Castling capabilities not specified. Assuming none.\n";
    } else if (FEN[index] != '-') {
#include "board_define_position_constants.hxx"
      if (FEN[index] == 'K') {
        castling |= WHITE_SHORT_CASTLING; index++; IS_NULL;
        if (board[e1]!=WKING || board[h1]!=WROOK) {
          cerr << "King or rook placed incorrectly for castling.\n";
          reset_and_return;
        }
      }
      if (FEN[index] == 'Q') {
        castling |= WHITE_LONG_CASTLING;  index++; IS_NULL;
        if (board[e1]!=WKING || board[a1]!=WROOK) {
          cerr << "King or rook placed incorrectly for castling.\n";
          reset_and_return;
        }
      }
      if (FEN[index] == 'k') {
        castling |= BLACK_SHORT_CASTLING; index++; IS_NULL;
        if (board[e8]!=BKING || board[h8]!=BROOK) {
          cerr << "King or rook placed incorrectly for castling.\n";
          reset_and_return;
        }
      }
      if (FEN[index] == 'q') {
        castling |= BLACK_LONG_CASTLING;  index++; IS_NULL;
        if (board[e8]!=BKING || board[a8]!=BROOK) {
          cerr << "King or rook placed incorrectly for castling.\n";
          reset_and_return;
        }
      }
#include "board_undef_position_constants.hxx"
    } else {
      index++;
      IS_NULL;
    }

    //cerr << "16.1.3.4: En passant target square\n";
    ++index;
    if (!FEN[index]) {
      cerr << "En passant not specified, assuming it is not allowed.\n";
    } else if (FEN[index] != '-') {
      en_passant = CR_TO_POS[FEN[index]-'a'][FEN[index+1]-'1'];

      {
        bool invalid = false;
        // For an en passant to be valid the following has to be satisfied
        // The en passant square and the original square of the pawn must be empty
        // A pawn of the correct color must occupy the final square.
        // Also, a pawn of the opposite color must be placed so the en passant can be exploited.
        if (board[en_passant]) invalid = true;
        if (player) {
          if (ROW[en_passant]!=2  ||  board[en_passant-8]  ||  board[en_passant+8] != WPAWN  ||
              ((COLUMN[en_passant]==0  ||  board[en_passant+7]!=BPAWN)  &&
                  (COLUMN[en_passant]==7  ||  board[en_passant+9]!=BPAWN))) invalid = true;
        } else {
          if (ROW[en_passant]!=5  || board[en_passant + 8]  ||  board[en_passant-8] != BPAWN  ||
              ((COLUMN[en_passant]==0  ||  board[en_passant-9]!=WPAWN)  &&
                  (COLUMN[en_passant]==7  ||  board[en_passant-7]!=WPAWN))) invalid = true;
        }

        if (invalid) {
          cerr << "invalid en passant.\n";
          reset_and_return;
        }
      }

      index += 2;
    } else {
      ++index;
    }
  }

  //cerr << "16.1.3.5: Halfmove clock\n";
  if (index < (int)FEN.size()) {
    ++index;
    moves_played_since_progress = FEN[index++] - '0';
    while (FEN[index]  &&  FEN[index] != ' ') {
      moves_played_since_progress *= 10;
      moves_played_since_progress += FEN[index++] - '0';
    }
  } else {
    moves_played_since_progress = 0;
    cerr << "Successive non capturing non pawn moves not specified. Assuming 0\n";
  }

  //cerr << "16.1.3.6: Fullmove number\n";
  if (index < (int)FEN.size()) {
    ++index;
    moves_played = FEN[index++] - '0';
    while (FEN[index]) {
      moves_played *= 10;
      moves_played += FEN[index++] - '0';
    }
    --moves_played;
    moves_played += moves_played;
    moves_played += player;
  } else {
    // Assumes that 2 full moves have been played for each piece being captured
    moves_played = 4*(32-get_num_pieces()) + player + 2;
    cerr << "Full move number not specified. Assuming " << 2*(32-get_num_pieces())+2 << "\n";
  }

  played_from_scratch = false;
  // cerr <<  "loadFEN: Setting played_from_scratch = false\n";
  initial_position = FEN;

  return true;
}
#undef IS_NULL

// Standard opening in FEN looks like this:
// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
string Board::toFEN() const {
  char s[92];
  int index = 0;

  // 16.x.blah comes from
  // Standard: Portable Game Notation Specification and Implementation Guide

  // 16.1.3.1: Piece placement data
  for (int r=7; r>=0; r--) {
    int num_vacant = 0;
    for (int c=0; c<8; c++) {
      Piece p = board[CR_TO_POS[c][r]];
      if (p) {
        if (num_vacant) {
          s[index++] = num_vacant+'0';
          num_vacant = 0;
        }
        s[index++] = PIECE_CHAR[p];
      } else ++num_vacant;
    }
    if (num_vacant) {
      s[index++] = num_vacant+'0';
      num_vacant = 0;
    }
    if (r) s[index++] = '/';
  }

  // 16.1.3.2: Active color
  s[index++] = ' ';
  s[index++] = PLAYER_CHAR[player];

  // 16.1.3.3: Castling availability
  s[index++] = ' ';
  if (ANY_CASTLING & castling) {
    if (castling & WHITE_SHORT_CASTLING) s[index++] = 'K';
    if (castling & WHITE_LONG_CASTLING) s[index++] = 'Q';
    if (castling & BLACK_SHORT_CASTLING) s[index++] = 'k';
    if (castling & BLACK_LONG_CASTLING) s[index++] = 'q';
  } else {
    s[index++] = '-';
  }

  // 16.1.3.4: En passant target square
  s[index++] = ' ';
  if (legal_pos(en_passant)) {
    s[index++] = COLUMN_NAME[en_passant][0];
    s[index++] = ROW_NAME[en_passant][0];
  } else {
    s[index++] = '-';
  }

  // 16.1.3.5: Halfmove clock
  s[index++] = ' ';
  {
    int tmp = moves_played_since_progress;
    if (tmp > 99) s[(tmp/=10, index++)] = '1';
    if (tmp > 9) {
      s[index++] = (tmp/10) + '0';
      tmp = tmp % 10;
    }
    s[index++] = tmp + '0';
  }

  // 16.1.3.6: Fullmove number
  s[index++] = ' ';
  {
    int tmp = (moves_played/2)+1;
    if (tmp > 999) { s[index++] = (tmp/1000) + '0'; tmp = tmp % 1000; }
    if (tmp > 99)  { s[index++] = (tmp/100) + '0';  tmp = tmp % 100;  }
    if (tmp > 9)   { s[index++] = (tmp/10) + '0';   tmp = tmp % 10;   }
    s[index++] = tmp + '0';
  }

  s[index] = 0;
  return s;
}
/*
    a b c d e f g h
  +-----------------+    |
8 |                 | 8  | 1b
7 |                 | 7  |
6 |                 | 6  | White has lost castling
5 |                 | 5  | Black has lost castling
4 | P p             | 4  |
3 | e               | 3  | moves played since progress = 0
2 |                 | 2  |
1 | K       k       | 1  | en passant at a3
  +-----------------+    |
    a b c d e f g h
loadfen 8/8/8/8/Pp6/8/8/K3k3 b - a3
index database
 */


//#############################################################


bool Board::clr_board(Board *board, ostream& os, vector<string> &p) {
  Board *_b = reinterpret_cast<Board *>(board);
  Board &b = *dynamic_cast<Board *>(_b);

  if (dot_demand(p, 1, "help")) {
    os << "Board, help:\n"
        << "    print board  or  pb  or  dir\n"
        << "      - print board\n"
        << "    print FEN board   or  pFb\n"
        << "    print counters  or  pc\n";

  } else if (dot_demand(p, 1, "dir")  ||
      dot_demand(p, 2, "print", "board")) {
    b.print_board(os);

  } else if (dot_demand(p, 3, "print", "FEN", "board")) {
    cerr << "FEN: " << b.toFEN() << "\n";

  } else if (dot_demand(p, 2, "print", "counters")) {
    b.print_counters(os);

  } else return false;
  return true;
}

void Board::print_counters(ostream &os) {
  os << "Board \"counters\":\n"
      << "piece_count: Pattern = " << toString(piece_count.as_pattern, 8, 16) << ":\n"
      << "\tget_num_pieces() = " << (int)get_num_pieces() << "\n"
      << "\tget_num_pieces(WHITE, BLACK) (" << (int)get_num_pieces(WHITE)
      << "," << (int)get_num_pieces(BLACK) << ")\n"
      << "\tget_num_non_zugzwang_pieces(WHITE,BLACK) = ("
      << (int)get_num_non_zugzwang_pieces(WHITE) << ","
      << (int)get_num_non_zugzwang_pieces(BLACK) << ")\n"
      << "endgame_hashing_insufficient_material: Pattern = "
      << toString(endgame_hashing_insufficient_material.as_pattern, 8, 16) << ":\n"
      << "\tget_insufficient_material_a() = " << (int)get_insufficient_material_a() << "\n"
      << "\tget_insufficient_material_b() = " << (int)get_insufficient_material_b() << "\n"
      << "\tget_endgame_hashing() = " << (int)get_endgame_hashing() << "\n";
}

void Board::print_board(ostream& os) const {
  os << "    a b c d e f g h\n";
  os << "  +-----------------+    |\n";
  for (int r=7; r>=0; --r) {
    os << (r+1) << " | ";
    for (int c=0; c<8; c++) {
      Position p = CR_TO_POS[c][r];
      os << (en_passant==p ? 'e' : PIECE_CHAR[board[CR_TO_POS[c][r]]]) << ' ';
    }
    os << "| " << (r+1) << "  |";
    switch (r) {
    case 7:
      os << ' ' << ((moves_played>>1)+1) << PLAYER_CHAR[player] << '\n';
      break;
    case 5:
      switch (castling&0x30) {
      case 0x00: os << " White has lost castling\n";
      break;
      case 0x10: os << " White has lost short castling\n";
      break;
      case 0x20: os << " White has lost long castling\n";
      break;
      case 0x30: os << " White can still castle\n";
      break;
      }
      break;
      case 4:
        switch (castling>>6) {
        case 0: os << " Black has lost castling\n";
        break;
        case 1: os << " Black has lost short castling\n";
        break;
        case 2: os << " Black has lost long castling\n";
        break;
        case 3: os << " Black can still castle\n";
        break;
        }
        break;
        case 2:
          os << " moves played since progress = " << (int)moves_played_since_progress << '\n';
          break;
          /*
    case 1:
      os << " number of checking pieces = " << (int)num_checks;
      if (num_checks)
	os << ", threat pos = " << POS_NAME[threat_pos];
      os << '\n';
      break;
           */
        case 0:
          if (en_passant != ILLEGAL_POS) {
            os << " en passant at " << POS_NAME[en_passant] << '\n';
            break;
          }
        default:
          os << '\n';
    }
  }
  os << "  +-----------------+    |\n";
  os << "    a b c d e f g h\n";
  os << "FEN: loadfen " << toFEN() << "\n";
}

//#############################################################

void Board::remove_piece(Position pos) {
  assert(board[pos]  &&  legal_pos(pos));

  piece_count.as_pattern -= PIECE_COUNT_CONSTANTS[board[pos]];
  endgame_hashing_insufficient_material.as_pattern -=
      ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[pos]][POS_COLOR[pos]];

  board[pos] = 0;
}

void Board::insert_piece(Position pos, Piece piece) {
  assert(legal_pos(pos));

  if (board[pos]) {
    // capture piece
    piece_count.as_pattern -= PIECE_COUNT_CONSTANTS[board[pos]];
    endgame_hashing_insufficient_material.as_pattern -=
        ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[pos]][POS_COLOR[pos]];
  }
  piece_count.as_pattern += PIECE_COUNT_CONSTANTS[piece];
  endgame_hashing_insufficient_material.as_pattern +=
      ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[piece][POS_COLOR[pos]];

  board[pos] = piece;
}

void Board::move_piece(Position from, Position to) {
  assert(legal_pos(from)  &&  legal_pos(to)  &&  board[from]);
  assert(ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[from]][POS_COLOR[from]] ==
      ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[from]][POS_COLOR[to]]);

  if (board[to]) {
    // capture piece
    piece_count.as_pattern -= PIECE_COUNT_CONSTANTS[board[to]];
    endgame_hashing_insufficient_material.as_pattern -=
        ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[board[to]][POS_COLOR[to]];
  }

  board[to] = board[from];
  board[from] = 0;
}

void Board::place_kings(Position white_king, Position black_king) {
  player = WHITE;
  insert_piece(white_king, WKING);
  player = BLACK;
  insert_piece(black_king, BKING);
}

bool Board::passed_pawn(Position pos) {
  if (PIECE_KIND[board[pos]] != PAWN) return false;

  if (board[pos] == WPAWN) {
    for (int c = max(0, COLUMN[pos]-1); c <= min(7, COLUMN[pos]+1); c++)
      for (int r = ROW[pos]+1; r<7; r++)
        if (board[CR_TO_POS[c][r]] == BPAWN) return false;
  } else {
    for (int c = max(0, COLUMN[pos]-1); c <= min(7, COLUMN[pos]+1); c++)
      for (int r = ROW[pos]-1; r; r--)
        if (board[CR_TO_POS[c][r]] == WPAWN) return false;
  }

  return true;
}



// set_board returns false if the board setup is not legal. The following is checked
// a) That no 2 pieces occupy the same square
// b) That the 2 kings is not to near
// c) That the player to move cannot capture the opponent king
// d) If an en passant is possible, then the 2 squares it has passed must be empty.
//
// set_board checks whether castling/en passant capabilities are encoded in the
// piece positions (if so, _castling and _en_passant must have default values).
// If so, piece_list, _castling and _en_passant are updated accordingly.
// piece_list might be changed if piece overlap encodes castling
bool set_board(vector<PiecePos> &piece_list, int player_turn,
    int &_castling, int &_en_passant,// use 0, ILLEGAL_POS as default
    int _moves_played_since_progress = 0, int full_move_number = 1);



bool Board::transform_board(int transformation) {
  assert(0<=transformation && transformation<8);

  // Identity mapping?
  if (!transformation) return true;

  // Check whether the transformation is allowed
  if (castling) return false;
  if (transformation != 1  &&  get_num_pawns()) return false;

  vector<PiecePos> piece_list(get_num_pieces());
  {
    int index = -1;
    for (int i=0; i<64; i++)
      if (board[i])
        piece_list[++index] = PiecePos(board[i], reflect(i, transformation));
  }

  int _castling = castling;
  int _en_passant = en_passant==ILLEGAL_POS ? ILLEGAL_POS : reflect(en_passant, transformation);
  return set_board(piece_list, player, _castling, _en_passant,
      moves_played_since_progress, (moves_played >> 1)+1);
}
