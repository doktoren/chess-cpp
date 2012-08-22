#ifndef _BOARD_2_
#define _BOARD_2_

#include "move_and_undo.hxx"
#include "unsigned_long_long.hxx"

#include "board.hxx"

class Board2 : public Board {
public:
  // The constructor will NOT initialize to standard opening.
  Board2();
  ~Board2() { 
    if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
      cerr << "Board2 destructor called.\n";
  }

  // Clears board
  virtual void reset_all();

  virtual bool loadFEN(string FEN);

  // Only used by endgame table generator (?)
  /*
  bool set_player(bool new_player) {
    if (player == new_player) return true;
    if (num_checks) return false;
    return Board::set_player(new_player);
  }
  */

  Move moves() const;
  Move moves_from_pos(Position pos) const;
  Move moves_to_dest(Position dest) const;
  Move moves_from_to(Position pos, Position dest) const;

  // If you are only interestet in one kind of pieces,
  // specify which one (the kind - eg. QUEEN instead of BQUEEN)
  bool next_move(Move &move, Piece piece_kind = 0) const;

  Undo execute_move(Move move);
  void undo_move(Move move, Undo undo);

  // can be overloaded by more efficient check in board_2_plus
  virtual int calc_game_status_ignore_50_move_rule_and_move_repetition();

  // Also ignores move repetition issue, but is being extended by
  // method in board_3 which handles it. Calls above
  virtual int calc_game_status();
  // game_status_reason is set by calling calc_game_status()
  int game_status_reason;

  static bool clr_board2(void *ignored, Board *board, ostream& os, vector<string> &p);
  void print_board(ostream& os) const;

  void print_king_threats(ostream& os);
  void print_moves(ostream& os, Position from = ILLEGAL_POS,
		   Position to = ILLEGAL_POS, Piece piece = 0);
  void print_bit_boards(ostream& os);
  void print_threat_pos(ostream& os);

  // More efficient implementation in board_2_plus
  virtual string moveToSAN(Move move);
  virtual Move sanToMove(string san);

  // After Board::set_board this function will check whether a king
  // can be captured (illegal).
  bool king_capture_possible();

  // find_legal_move checks whether move.from and move.to matches a legal move.
  // In case of a match move.special_move and move.blah is set
  // (pawn promotion: default is queen). Is SLOW!
  virtual bool find_legal_move(Move& move);

  // Returns a list of moves that could have led to this position.
  // If en passant is possible, then there is at most one legal move.
  // When undoing a retro move, moves_played_since_progress will be set to 0
  // if the move undone is a pawn move or a capturing move. Otherwise it will
  // be counted down one (but never below 0).
  // IMPORTANT: be careful about undoing too many capturing moves ---
  // it is not checked if for example white gets 15 queens this way. Due to
  // effiency the program assumes among other things that there will never
  // be more than 127 legal moves, and then this will break down => seg. fault.
  // When undoing, num_checks will 
  // Example
  // tmp = get_retro_moves(true, true, true, true, true, false);
  // if (transform_board(tmp[0].third)) {
  //   undo_move(tmp[0].first, tmp[0].second);
  //   ...
  //   execute_move(tmp[0].first);
  //   inv_transform_board(tmp[0].third);
  // }
  vector<triple<Move,Undo,int> > get_retro_moves(bool allow_pawn_promotion = true,
						 bool allow_castling = true,
						 bool allow_captures = true,
						 bool allow_transformations_of_board = false,
						 uint max_moves = 0xFFFFFFFF);

  // Irreversible moves are:
  // a) Captures
  // b) Pawn moves
  // c) Castling or move with rook or king that could otherwise have been part of castling move
  // REMARK! In the 50 move rule, moves of type c is considered ordinary.
  bool is_irreversible_move(Move move) {
    return board[move.to]  ||
      PIECE_KIND[board[move.from]] == PAWN  ||
      (~(CASTLING_LOST[move.from] & CASTLING_LOST[move.to]) & castling);
  }

  uchar get_num_checks() { return num_checks; }

  // position_is_unreachable is set by set_check_invariants().
  // Hence it will abso be set by loadFEN and set_board.
  bool unreachable_position() { return position_is_unreachable; }
  // Uses get_retro_moves to try to get test_depth moves back.
  // All kinds of retro moves are accepted (including those transforming the board)
  // If this is not posible, the depth which fails are returned. Otherwise 0.
  int unreachable_position(int test_depth);


  virtual bool try_execute_null_move();
  virtual void undo_null_move();

  Position get_king_position(int stm) { return king_pos[stm]; }

protected:

  // move_piece will only be called on pieces belonging to player
  virtual void move_piece(Position from, Position to);
  // that guarantee does not hold for insert_piece and remove_piece
  virtual void insert_piece(Position pos, Piece piece);
  virtual void remove_piece(Position pos);

  void place_kings(Position white_king, Position black_king);

  virtual bool internal_set_board();


  // #######  BEGIN board_2.hxx 2 byte block  ########
  // Do not change the order, it is the same as in Undo => optimized copying
  uchar num_checks;
  // king threats
  Position threat_pos; // only defined if num_checks != 0
  // #######   END board_2.hxx 2 byte block   ########


  // position_is_unreachable is set by set_check_invariants().
  // Hence it will abso be set by loadFEN and set_board.
  char position_is_unreachable;


  ull threat_elim; // only defined if num_checks != 0
  Position king_pos[2]; // [player]
  uint board_lines[4][16]; // [direction][line num]

  // Number of pawns+knights+kings that can attack position p =
  // max{n | (bit_boards[n-1][player] & p) == 1}
  // (ie. a unary count)
  ull bit_boards[12][2]; // At most 11 kings, pawns and knights can threaten a pos.
  friend ostream& operator<<(ostream& os, const ull& bit_board);

  // set_check_invariants sets num_checks, threat_pos and threat_elim.
  // It must be called by loadFEN and set_board since these invariants
  // cannot be updated properly while setting up the board
  // (or at least it is to difficult to do so).
  void set_check_invariants();

  // Will own king be checked if the piece on pos vanished?
  bool check_if_moved(Position pos) const;

  // Is own king checked if placed on pos!=its current position (illegal move).
  // If pos is a position that the king can move to, then it
  // is not nescessary to remove the king from its current position.
  // If a piece is placed on pos, it is assumed this is captured.
  bool check_if_king_placed(Position pos) const;

private:
  Position captured_en_passant_pawn(Position from, Position pawn_capture_pos);
  triple<int,uchar,Position> retro_move_count_checks(Position from, Position to,
						     Piece original_piece, Piece piece_killed);

  // static tables (how to assure only one copy ?)
  void init_CHECK_TABLE();
  void init_DIRECTION();
  void init_bitboards();
  void init_bitboard_lines();

  void bit_board_insert(Position pos, Piece piece, bool player);
  void bit_board_remove(Position pos, Piece piece, bool player);

  void king_line_insert_piece(Position pos, Piece piece);
  void king_line_replace_piece(Position pos, Piece piece);
  void king_line_remove_piece(Position pos);
  int add_check_from_sdirection(Position pos, int direction);

  bool is_illegal_castling_move(Move move) const;
  bool legal_move(Move& move) const;
  bool obstacle_free_move(Move move) const;
  bool next_move_fixed_destination(Move &move, Piece piece_kind) const;


  // Private to prevent copying:
  Board2(const Board2&);
  Board2& operator=(const Board2&);
};
ostream& operator<<(ostream& os, const ull& bit_board);

extern uchar CHECK_TABLE[0x10000];//0,1 or 2
extern uchar DIRECTION[64][64];// "unsigned" direction: [0..3] or ...
extern uchar SDIRECTION[64][64];// "signed" direction: [0..7] or ...
extern Position DIRECTION_MOVE_TABLE[8][64];
extern ull BIT_BOARDS[13+2][64];
extern ull BB_LINES[64][64];

#endif
