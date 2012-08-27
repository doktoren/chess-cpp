#ifndef _BOARD_
#define _BOARD_

#include <string>
#include <vector>
#include <assert.h>

#include "typedefs.hxx"
#include "util/help_functions.hxx"
#include "parser.hxx"
#include "piece_pos.hxx"

#define PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS false

//################################

#define WHITE 0
#define BLACK 1
#define NEITHER_COLOR 2

//################################

#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

#define WPAWN 1
#define WKNIGHT 2
#define WBISHOP 3
#define WROOK 4
#define WQUEEN 5
#define WKING 6
#define BPAWN 7
#define BKNIGHT 8
#define BBISHOP 9
#define BROOK 10
#define BQUEEN 11
#define BKING 12

// A piece value of 13 is used for a fictive colorless pawn with move direction
// from file a to h. Similarly a piece value of 14 denotes a pawn going the
// opposite way.
#define FILE_A_TO_H_PAWN 13
#define FILE_H_TO_A_PAWN 14

#define WHITE_PAWN 1
#define WHITE_KNIGHT 2
#define WHITE_BISHOP 3
#define WHITE_ROOK 4
#define WHITE_QUEEN 5
#define WHITE_KING 6
#define BLACK_PAWN 7
#define BLACK_KNIGHT 8
#define BLACK_BISHOP 9
#define BLACK_ROOK 10
#define BLACK_QUEEN 11
#define BLACK_KING 12

extern const bool WHITE_PIECE[13];
extern const bool BLACK_PIECE[13];
extern const int PIECE_COLOR[13];
extern const Piece PIECE_KIND[13];
extern const Piece SWAP_PIECE_COLOR[13];

//################################

#define WHITE_LONG_CASTLING (1<<4)
#define WHITE_SHORT_CASTLING (2<<4)
#define BLACK_LONG_CASTLING (4<<4)
#define BLACK_SHORT_CASTLING (8<<4)

#define WHITE_CASTLING (WHITE_LONG_CASTLING + WHITE_SHORT_CASTLING)
#define BLACK_CASTLING (BLACK_LONG_CASTLING + BLACK_SHORT_CASTLING)

#define LONG_CASTLING (WHITE_LONG_CASTLING + BLACK_LONG_CASTLING)
#define SHORT_CASTLING (WHITE_SHORT_CASTLING + BLACK_SHORT_CASTLING)

#define ANY_CASTLING (WHITE_CASTLING + BLACK_CASTLING)

extern const uchar CASTLING[64];
extern const uchar CASTLING_LOST[64];

//################################

extern const int ROW[64];
extern const int COLUMN[64];
extern const bool POS_COLOR[64];
extern const Position CR_TO_POS[8][8]; // Colum, Row

extern const Position ROTATE[64];

inline bool legal_pos(Position p) { return !(p & 0xC0); }
#define ILLEGAL_POS 64

inline int distance(Position p1, Position p2) {
  return abs(ROW[p1]-ROW[p2]) + abs(COLUMN[p1]-COLUMN[p2]);
}

//################################

#define GAME_OPEN 0
#define GAME_DRAWN 1
#define WHITE_WON 2
#define BLACK_WON 3
extern const string game_result_texts[4];

#define FIFTY_MOVE_RULE 1
#define BLACK_IS_CHECK_MATE 2
#define WHITE_IS_CHECK_MATE 3
#define STALEMATE 4
#define REPEATED_BOARD_POSITION 5
#define INSUFFICIENT_MATERIAL 6
extern const string game_status_texts[7];

// used in move tables to indicate that eg. a knight can't move
// to any diagonal position
#define IMPOSSIBLE_MOVE 96


#ifdef ALLOW_5_MEN_ENDGAME

// When at most 5 pieces is left of which 2 must be white and
// black king, the 0, 1, 2 or 3 remaining pieces can be uniquely
// determined by the weighted sum of the pieces, where the weights are:
#define DB_WPAWN_VALUE 1
#define DB_WKNIGHT_VALUE 4
#define DB_WBISHOP_VALUE 13
#define DB_WROOK_VALUE 32
#define DB_WQUEEN_VALUE 71
#define DB_WKING_VALUE 0
#define DB_BPAWN_VALUE 124
#define DB_BKNIGHT_VALUE 218
#define DB_BBISHOP_VALUE 375
#define DB_BROOK_VALUE 572
#define DB_BQUEEN_VALUE 744
#define DB_BKING_VALUE 0

#else

#define DB_WPAWN_VALUE 1
#define DB_WKNIGHT_VALUE 3
#define DB_WBISHOP_VALUE 7
#define DB_WROOK_VALUE 12
#define DB_WQUEEN_VALUE 20
#define DB_WKING_VALUE 0
#define DB_BPAWN_VALUE 30
#define DB_BKNIGHT_VALUE 44
#define DB_BBISHOP_VALUE 65
#define DB_BROOK_VALUE 80
#define DB_BQUEEN_VALUE 96
#define DB_BKING_VALUE 0

#endif
#define DB_ARRAY_LENGTH (3*DB_BQUEEN_VALUE+1)
extern const int ENDGAME_HASHING_CONSTANTS[13];

extern const uint PIECE_COUNT_CONSTANTS[13];
extern const uint ENDGAME_HASHING_INSUFFICIENT_MATERIAL_CONSTANTS[13][2];

//################################

extern const char PIECE_CHAR[13];
extern const string PIECE_SCHAR[13];
extern const string PIECE_NAME[13];
extern const string PPIECE_NAME[13];
extern const char PLAYER_CHAR[2];
extern const string PLAYER_NAME[2];

extern const string POS_NAME[66];
extern const string COLUMN_NAME[66];
extern const string ROW_NAME[66];

inline Piece char_to_piece(char ch) {
  switch(ch) {
  case 'P': return WPAWN; break;
  case 'N': return WKNIGHT; break;
  case 'B': return WBISHOP; break;
  case 'R': return WROOK; break;
  case 'Q': return WQUEEN; break;
  case 'K': return WKING; break;
  case 'p': return BPAWN; break;
  case 'n': return BKNIGHT; break;
  case 'b': return BBISHOP; break;
  case 'r': return BROOK; break;
  case 'q': return BQUEEN; break;
  case 'k': return BKING; break;
  }
  return 0;
}
inline Position strToPos(string pos) {
  if (pos[0]=='#') return ILLEGAL_POS;
  return CR_TO_POS[pos[0]-'a'][pos[1]-'1'];
}

//################################

extern Position REFLECTION_TABLE[8*64];
inline int reflect(int pos, int refl) {
  return REFLECTION_TABLE[(refl << 6) | pos];
}
inline int inv_reflect(int pos, int inv_refl) {
  const uchar INV[8] = {0,1,2,3,4,6,5,7};
  return REFLECTION_TABLE[(INV[inv_refl] << 6) | pos];
}

//################################

#define board_iterate(p) for(Position p=0; legal_pos(p); ++p)

// See also board_position_constants.hxx

template <class TYPE>
void print_map64(ostream &os, TYPE *list64, int num_digits=0, int base=10) {
  os << "   ";
  for (char i='a'; i<='h'; i++) {
    for (int j=0; j<num_digits; j++) os << ' ';
    os << i;
  }
  os << '\n';

  os << "  +";
  for (int i=0; i<num_digits+1; i++) os << "--------";
  os << "-+\n";

  for (int r=7; r>=0; r--) {
    os << (r+1) << " |";
    for (int c=0; c<8; c++)
      os << ' ' << toString(list64[8*r+c], num_digits, base);
    os << " | " << (r+1) << '\n';
  }

  os << "  +";
  for (int i=0; i<num_digits+1; i++) os << "--------";
  os << "-+\n";

  os << "   ";
  for (char i='a'; i<='h'; i++) {
    for (int j=0; j<num_digits; j++) os << ' ';
    os << i;
  }
  os << '\n';
}

template <class TYPE>
void print_latex_map64(ostream &os, TYPE *list64, int num_digits=0, int base=10) {
  os << "{\\setlength\\tabcolsep{0.7\\tabcolsep}\n"
      << "\\begin{footnotesize}\n"
      << "\\begin{center}\n"
      << "\\begin{tabular}{c|cccccccc|c}\n"
      << "\\multicolumn{1}{c}{}&  a& b& c& d& e& f& g&\\multicolumn{1}{c}{h}\\\\\n"
      << "\\cline{2-9}\n";

  for (int r=7; r>=0; r--) {
    os << (r+1) << " & ";
    for (int c=0; c<8; c++) {
      if (c) os << '&';
      os << toString(list64[8*r+c], num_digits, base);
    }
    os << " & " << (r+1) << "\\\\\n";
  }

  os << "\\cline{2-9}\n"
      << "\\multicolumn{1}{c}{}&  a& b& c& d& e& f& g&\\multicolumn{1}{c}{h}\\\\\n"
      << "\\end{tabular}\n"
      << "\\end{center}\n"
      << "\\end{footnotesize}\n"
      << "}\n";
}

// TYPE is signed value
template <class OSTREAM, class TYPE>
void print_signed_map64(OSTREAM &os, TYPE *list64, int num_digits=0, int base=10) {
  os << "   ";
  for (char i='a'; i<='h'; i++) {
    for (int j=0; j<num_digits; j++) os << ' ';
    os << i;
  }
  os << '\n';

  os << "  +";
  for (int i=0; i<num_digits+1; i++) os << "--------";
  os << "-+\n";

  for (int r=7; r>=0; r--) {
    os << (r+1) << " |";
    for (int c=0; c<8; c++)
      os << ' ' << signedToString(list64[8*r+c], num_digits, base);
    os << " | " << (r+1) << '\n';
  }

  os << "  +";
  for (int i=0; i<num_digits+1; i++) os << "--------";
  os << "-+\n";

  os << "   ";
  for (char i='a'; i<='h'; i++) {
    for (int j=0; j<num_digits; j++) os << ' ';
    os << i;
  }
  os << '\n';
}

template <class TYPE>
void print_latex_signed_map64(ostream &os, TYPE *list64, int num_digits=0, int base=10) {
  os << "{\\setlength\\tabcolsep{0.7\\tabcolsep}\n"
      << "\\begin{footnotesize}\n"
      << "\\begin{center}\n"
      << "\\begin{tabular}{c|cccccccc|c}\n"
      << "\\multicolumn{1}{c}{}&  a& b& c& d& e& f& g&\\multicolumn{1}{c}{h}\\\\\n"
      << "\\cline{2-9}\n";

  for (int r=7; r>=0; r--) {
    os << (r+1) << " & ";
    for (int c=0; c<8; c++) {
      if (c) os << '&';
      os << signedToString(list64[8*r+c], num_digits, base);
    }
    os << " & " << (r+1) << "\\\\\n";
  }

  os << "\\cline{2-9}\n"
      << "\\multicolumn{1}{c}{}&  a& b& c& d& e& f& g&\\multicolumn{1}{c}{h}\\\\\n"
      << "\\end{tabular}\n"
      << "\\end{center}\n"
      << "\\end{footnotesize}\n"
      << "}\n";
}

// TYPE is signed value
template <class TYPE>
void print_string_map64(ostream &os, TYPE *list64, int padded_length) {
  os << "   ";
  for (char i='a'; i<='h'; i++) {
    for (int j=0; j<padded_length; j++) os << ' ';
    os << i;
  }
  os << '\n';

  os << "  +";
  for (int i=0; i<=padded_length; i++) os << "--------";
  os << "-+\n";

  for (int r=7; r>=0; r--) {
    os << (r+1) << " |";
    for (int c=0; c<8; c++) {
      for (int i=list64[8*r+c].size(); i<=padded_length; i++)
        os << ' ';
      os << list64[8*r+c];
    }
    os << " | " << (r+1) << '\n';
  }

  os << "  +";
  for (int i=0; i<=padded_length; i++) os << "--------";
  os << "-+\n";

  os << "   ";
  for (char i='a'; i<='h'; i++) {
    for (int j=0; j<padded_length; j++) os << ' ';
    os << i;
  }
  os << '\n';
}

template <class TYPE>
void print_latex_string_map64(ostream &os, TYPE *list64, int padded_length) {
  os << "{\\setlength\\tabcolsep{0.7\\tabcolsep}\n"
      << "\\begin{footnotesize}\n"
      << "\\begin{center}\n"
      << "\\begin{tabular}{c|cccccccc|c}\n"
      << "\\multicolumn{1}{c}{}&  a& b& c& d& e& f& g&\\multicolumn{1}{c}{h}\\\\\n"
      << "\\cline{2-9}\n";

  for (int r=7; r>=0; r--) {
    os << (r+1) << " & ";
    for (int c=0; c<8; c++) {
      if (c) os << '&';
      for (int i=list64[8*r+c].size(); i<padded_length; i++)
        os << ' ';
      os << list64[8*r+c];
    }
    os << " & " << (r+1) << "\\\\\n";
  }

  os << "\\cline{2-9}\n"
      << "\\multicolumn{1}{c}{}&  a& b& c& d& e& f& g&\\multicolumn{1}{c}{h}\\\\\n"
      << "\\end{tabular}\n"
      << "\\end{center}\n"
      << "\\end{footnotesize}\n"
      << "}\n";
}

//################################

struct Error {
  Error() {}
  Error(string name) : name(name) {}
  void print(ostream &os) {
    os << "The error \"" << name << "\" has occurred\n";
  }
  string name;
};

//################################

struct HashBoard;

class Board {
public:
  // The constructor will NOT initialize to standard opening.
  Board();
  virtual ~Board();

  void new_game();
  void copy_from(Board &b) { loadFEN(b.toFEN()); }

  virtual bool loadFEN(string FEN);
  string toFEN() const;

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
      int _moves_played_since_progress = 0, int full_move_number = 50);
  bool set_board(vector<PiecePos> &piece_list, int player_turn) {
    int _castling = 0;
    int _en_passant = ILLEGAL_POS;
    return set_board(piece_list, player_turn, _castling, _en_passant);
  }

  vector<PiecePos> get_piece_list();
  void get_piece_list(vector<PiecePos> &l) const {
    int index = 0;
    for (int i=0; i<64; i++)
      if (board[i]) {
        assert((uint)index < l.size());
        l[index++] = PiecePos(board[i], i);
      }
    assert((uint)index == l.size());
  }

  /**
   * get_encoded_piece_list encodes en passant and castling in the position of the pieces
   * (castling only encoded if ENDGAME_TABLE_WITH_CASTLING==1)
   */
  void get_encoded_piece_list(vector<PiecePos> &l) const;

  // returns b7b6b5b4b3b2b1b0. bi is 1 iff transformation i is legal
  uchar allowed_symmetries() {
    if (castling) return 1;
    if (get_num_pawns()) return 1+2;
    return 0xFF;
  }
  // calling i_t_b inverses changes of t_b (and opposite)
  // transform_board uses set_board
  bool transform_board(int transformation);
  bool inv_transform_board(int transformation) {
    assert(0<=transformation && transformation<8);
    const uchar INV[8] = {0,1,2,3,4,6,5,7};
    return transform_board(INV[transformation]);
  }



  // clr_board : clr is an abbreviation for command line receiver
  static bool clr_board(Board *board, ostream& os, vector<string> &p);
  void print_board(ostream& os) const;
  void print_counters(ostream &os);

  // Accessor functions
  Piece operator[](Position pos) const { return board[pos]; }
  bool get_player() const { return player; }
  int get_moves_played() const { return moves_played; }

  bool en_passant_possible() const { return en_passant != ILLEGAL_POS; }
  // en_passant_square() returns ILLEGAL_POS if en passant not possible
  Position en_passant_square() const { return en_passant; }

  bool not_all_castling_capabilities_lost() { return castling; }
  uchar get_castling_capabilities() { return castling; }

  uchar get_num_pieces() const {
    return piece_count.individual.num_pieces;
  }
  uchar get_num_pieces(int player) const {
    return piece_count.individual.num_colored_pieces[player];
  }
  // Counts only knights, bishops, rooks and queens
  uchar get_num_non_zugzwang_pieces(int player) const {
    return (piece_count.individual.num_non_zugzwang_pieces >> (4*player)) & 0xF;
  }
  uchar get_num_pawns() const {
    return get_num_pieces() - 2 -
        (piece_count.individual.num_non_zugzwang_pieces & 0xF) -
        (piece_count.individual.num_non_zugzwang_pieces >> 4);
  }

  bool get_insufficient_material_a() const {
    return endgame_hashing_insufficient_material.individual.insufficient_material_a;
  }
  bool get_insufficient_material_b() const {
    return endgame_hashing_insufficient_material.individual.insufficient_material_b;
  }
  bool insufficient_material() const {
    return !get_insufficient_material_a() || !get_insufficient_material_b();
  }
  ushort get_endgame_hashing() const {
    return endgame_hashing_insufficient_material.individual.endgame_hashing;
  }

  int get_moves_played() { return moves_played; }

  bool played_from_scratch;
  string initial_position;

  // passed_pawn returns true if a pawn is on pos, and this pawn is a passed pawn
  // Inefficient implementation
  virtual bool passed_pawn(Position pos);
protected:
  // move_piece will only be called on pieces belonging to player
  virtual void move_piece(Position from, Position to);
  // that guarantee does not hold for insert_piece and remove_piece
  virtual void insert_piece(Position pos, Piece piece);
  virtual void remove_piece(Position pos);

  virtual void place_kings(Position white_king, Position black_king);

  // Clears board. Will be called before new_game, loadFEN and setBoard.
  virtual void reset_all();

  // set_board returns false if the board setup is not legal. The following is checked
  // a) That no 2 pieces occupy the same square
  // b) That the 2 kings is not to near
  // c) That the player to move cannot capture the opponent king
  // piece_list might be changed if piece overlap encodes castling
  virtual bool internal_set_board() { return true; }

  // The board itself is the first board[0..63]
  // board[64] is a white pawn and board[65] is a black pawn. etc.
  // These pieces optimize the move generator
  Piece board[64+2*6];

  // #############################################
  // ########     BEGIN 4 byte block     #########
  // #############################################

  // Do not change the order, it is the same as in Undo => optimized copying

  // en_passant is ILLEGAL_POS or the position of the en passant - i.e.
  // the position of the empty square a pawn has just passed accross.
  Position en_passant;

  // castling is a sum of
  // WHITE_LONG_CASTLING (1<<4)
  // WHITE_SHORT_CASTLING (2<<4)
  // BLACK_LONG_CASTLING (4<<4)
  // BLACK_SHORT_CASTLING (8<<4)
  uchar castling;

  // If an uninvertible move was made last turn, moves_played_since_progress is 0
  uchar moves_played_since_progress;

  // player <=> blacks turn
  // player is not used as a part of the Undo struct. It is simply here
  // to make the size 4 bytes
  bool player;


  // #############################################
  // ########      END 4 byte block      #########
  // #############################################

  // In half-moves. Eg. in move 5.b moves_played is 2*(5-1)+1
  int moves_played;

  union {
    struct {
      // WARNING: big-little endian dependent code
      uchar num_colored_pieces[2];
      uchar num_pieces;
      uchar num_non_zugzwang_pieces;// 4 bits for each side
    } individual;
    ulong as_pattern;
  } piece_count;

  union {
    struct {
      // WARNING: big-little endian dependent code
      // Sufficient material left <=> insufficient_material_b && insufficient_material_a
      // endgame_hashing will create overflow, so it must be placed last
      bool insufficient_material_b;
      bool insufficient_material_a;
      ushort endgame_hashing;
    } individual;
    uint as_pattern;
  } endgame_hashing_insufficient_material;

private:
  friend struct HashBoard;

  // Private to prevent copying:
  Board(const Board&);
  Board& operator=(const Board&);
};

#ifndef NDEBUG
// When NDEBUG not defined, HashBoard is used in transposition table
struct HashBoard {
  HashBoard() {
    memset(board, 0, 32);
    en_passant = 0;
    castling = 0;
    moves_played_since_progress = 0;
    player = 0;
    moves_played = 0;
  }

  HashBoard(const Board& b) {
    for (int i=0; i<32; i++)
      board[i] = (b.board[i<<1]<<4) | b.board[(i<<1)|1];
    en_passant = b.en_passant;
    castling = b.castling;
    moves_played_since_progress = b.moves_played_since_progress;
    player = b.player;
    moves_played = b.moves_played;
  }

  bool operator==(const HashBoard &hb) {
    for (int i=0; i<32; i++)
      if (board[i] != hb.board[i]) return false;
    if (en_passant != hb.en_passant) return false;
    if (castling != hb.castling) return false;
    //if (moves_played_since_progress != hb.moves_played_since_progress) return false;
    if (player != hb.player) return false;
    //if (moves_played != hb.moves_played) return false;
    return true;
  }
  bool operator!=(const HashBoard &hb) {
    return !operator==(hb);
  }

  void print(ostream &os) {
    Board b;
    for (int i=0; i<32; i++) {
      b.board[i<<1] = board[i]>>4;
      b.board[(i<<1)|1] = board[i]&0xF;
    }
    b.en_passant = en_passant;
    b.castling = castling;
    b.moves_played_since_progress = moves_played_since_progress;
    b.player = player;
    b.moves_played = moves_played;
    b.print_board(os);
  }

  uchar board[32];

  Position en_passant;
  uchar castling;
  uchar moves_played_since_progress;
  bool player;

  int moves_played;
};
#endif

#endif

