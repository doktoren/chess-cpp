#ifndef _BOARD_
#define _BOARD_

#include <string>
#include <vector>
#include <assert.h>

#include "typedefs.hxx"
#include "util/help_functions.hxx"
#include "parser.hxx"
#include "piece_pos.hxx"

#include "board_defines.hxx"
#include "board_constants.hxx"
#include "endgames/endgame_material.hxx"


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
  inline Piece operator[](Position pos) const { return board[pos]; }
  inline bool get_player() const { return player; }
  inline int get_moves_played() const { return moves_played; }

  inline bool en_passant_possible() const { return en_passant != ILLEGAL_POS; }
  // en_passant_square() returns ILLEGAL_POS if en passant not possible
  inline Position en_passant_square() const { return en_passant; }

  inline bool not_all_castling_capabilities_lost() { return castling; }
  inline uchar get_castling_capabilities() { return castling; }

  inline uchar get_num_pieces() const {
    return piece_count.individual.num_pieces;
  }
  inline uchar get_num_pieces(int player) const {
    return piece_count.individual.num_colored_pieces[player];
  }
  // Counts only knights, bishops, rooks and queens
  inline uchar get_num_non_zugzwang_pieces(int player) const {
    return (piece_count.individual.num_non_zugzwang_pieces >> (4*player)) & 0xF;
  }
  inline uchar get_num_pawns() const {
    return get_num_pieces() - 2 -
        (piece_count.individual.num_non_zugzwang_pieces & 0xF) -
        (piece_count.individual.num_non_zugzwang_pieces >> 4);
  }

  inline piece_count_t get_piece_count() const { return piece_count; }
  inline endgame_material_t get_endgame_material() const { return endgame_material; }


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

  piece_count_t piece_count;
  endgame_material_t endgame_material;

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
    if (player != hb.player) return false;
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

