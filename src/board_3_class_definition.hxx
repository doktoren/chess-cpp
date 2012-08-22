class ClassName : public Extends {
public:
  ClassName();
  ~ClassName();

  virtual void reset_all();

  void copy_from(ClassName &b);

  int calc_game_status();

  HashValue hash_value_after_move(Move move);

  void savePGN(string filename);
  bool loadPGN(string filename);

#ifdef PLUS
  static bool clr_board3plus(Board *board, ostream& os, vector<string> &p);
#else
  static bool clr_board3(Board *board, ostream& os, vector<string> &p);
#endif

  // from_move == 0  <=>  defined by show_last_num_moves
  // from_move < 0   <=>  print last -from_move moves
  // print_move_stack(os, 1); print entire move stack
  void print_move_stack(ostream& os, int from_move = 0, bool print_undo_info = false);
  // from_move as above, but move_stack not printed if from_move == 123456789
  void print_board(ostream& os, int from_move = 123456789);

  void verify_hash_value(ostream &os);

  void execute_move(Move move);
  bool undo_move();
  
  // If this undo_move undoes another move than the last one played, 
  // the move history is erased.
  void undo_move(Move move, Undo undo);

  // try_redo_move will always succeed after an undo_move
  bool try_redo_move();

  virtual bool loadFEN(string FEN);
  
  vector<Move> get_move_history();

  HashValue hash_value;

  // If the game is a .fen loaded position (this will be stored as
  // initial_position), the moves before eg. 10w may not be defined!
  int cannot_undo_before;

  bool get_last_move(Move &move) {
    if (moves_played > cannot_undo_before) {
      move = move_stack[moves_played-1];
      return true;
    } else {
      return false;
    }
  }

  int position_repetition_count() {
    return move_repetition[hash_value].num_repetitions;
  }

  // Adjustable parameters
  int show_last_num_moves;

  bool try_execute_null_move();
  void undo_null_move();

protected:
  virtual void insert_piece(Position pos, Piece piece);
  virtual void remove_piece(Position pos);
  virtual void move_piece(Position from, Position to);

  virtual bool internal_set_board();

  Move move_stack[MAX_GAME_LENGTH];
  Undo undo_stack[MAX_GAME_LENGTH];

  HashValue partial_hash_value; // not including en passant, etc.
  HashTable<HashValue, RepetitionInfo> move_repetition;
private:
  // Private to prevent copying:
  ClassName(const ClassName&);
  ClassName& operator=(const ClassName&);
};
