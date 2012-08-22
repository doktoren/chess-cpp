// enforce correct indention in emacs
#define akfhlkwehkjlshdfl

akfhlkwehkjlshdfl {
#ifdef ALLOW_5_MEN_ENDGAME
  if (num_pieces == 5) {
    cerr << "################################################################\n"
	 << "################################################################\n"
	 << "################################################################\n"
	 << "##########                                          ############\n"
	 << "##########   ------------  WARNING !!!  ---------   ############\n"
	 << "##########   You are constructing a 5 men endgame   ############\n"
	 << "##########   using the slow constuction method !    ############\n"
	 << "##########   If the endgame contain pawns, it is    ############\n"
	 << "##########   furthermore NOT likely that            ############\n"
	 << "##########   sufficient memory is available!        ############\n"
	 << "##########                                          ############\n"
	 << "################################################################\n"
	 << "################################################################\n"
	 << "################################################################\n";
  }
#endif

  map<int, EFState> old_states_wtm = load_dependency(WHITE);
  map<int, EFState> old_states_btm = load_dependency(BLACK);

  Board2 board;
  vector<PiecePos> piece_list(num_pieces);
  for (int i=0; i<num_pieces; i++)
    piece_list[i].piece = pieces[i];

  bool unknown_shown = !*(endgame_settings->construction_show_progress);
  bool white_win_shown = unknown_shown;
  bool black_win_shown = white_win_shown;
  bool draw_shown = black_win_shown;

  for (int player=0; player<(symmetric_endgame ? 1 : 2); player++) {

    if (!table[player])
      table[player] = new char[table_size];

    for (uint i=0; i<table_size; i++) {
      (*decompress_table_index)(i, piece_list);

      //cerr << "decompress_table_index_en_passant: " << decompress_table_index_en_passant << "\n";

      /*
#include "board_define_position_constants.hxx"
      bool test = (piece_list[0].pos == a1)  &&  (piece_list[1].pos == a4)  &&
	(piece_list[2].pos == e1)  &&  (piece_list[3].pos == b4);
#include "board_undef_position_constan			 int &_castling, int &_en_passant,// use 0, ILLEGAL_POS as default
			 int _moves_played_since_progress = 0, int full_move_number = 1);ts.hxx"
      if (test) cerr << "Interesting position: index = " << i << "\n";
      */

      bool legal_position = board.set_board(piece_list, player);

      /* No need to trash unreachable positions in the table.
	 They are trashed when constructing the bdd instead.
      if (legal_position  &&  board.unreachable_position()  &&
	  *(endgame_settings->trash_unreachable_positions)) {
	board.print_board(big_output);
	legal_position = false;
      }
      */

      /*
      cerr << "index = " << i << ", position = ";
      for (uint j=0; j<piece_list.size(); j++)
	cerr << POS_NAME[piece_list[j].pos] << ", ";
      cerr << "player = " << player << "\n";
      */

      if (legal_position) {
	int status;
	switch (status = board.calc_game_status()) {
	case GAME_OPEN:
	  table[player][i] = ENDGAME_TABLE_UNKNOWN;
	  if (!unknown_shown) {
	    cerr << "GAME_OPEN (index " << i << "):\n";
	    board.print_board(cerr);
	    unknown_shown = true;
	  }
	  break;
	case GAME_DRAWN:
	  table[player][i] = ENDGAME_TABLE_DRAW;
	  if (!draw_shown) {
	    cerr << "GAME_DRAWN (index " << i << "):\n";
	    cerr << "Reason: " << game_status_texts[board.game_status_reason] << '\n';
	    board.print_board(cerr);
	    draw_shown = true;
	  }
	  break;
	case WHITE_WON:
	  table[player][i] = 0;
	  if (!white_win_shown) {
	    cerr << "WHITE_WON (index " << i << "):\n";
	    board.print_board(cerr);
	    white_win_shown = true;
	  }
	  break;
	case BLACK_WON:
	  table[player][i] = 0;
	  if (!black_win_shown) {
	    cerr << "BLACK_WON (index " << i << "):\n";
	    board.print_board(cerr);
	    black_win_shown = true;
	  }
	  break;
	default:
	  assert(0);
	}

      } else {

	table[player][i] = ENDGAME_TABLE_ILLEGAL;

	/*
	if (i==258376) {
	  cerr << "Illegal position:\n(k1,p1,p2,k2,p3)=(" << POS_NAME[piece_list[0].pos] << ", "
	  << POS_NAME[piece_list[1].pos] << ", " << POS_NAME[piece_list[2].pos] << ", "
	  << POS_NAME[piece_list[3].pos] << ", " << POS_NAME[piece_list[4].pos]
	  << "), player = " << player << "\n";
	}
	*/
      }
    }
  }
  
  cerr << "All illegal positions, stale mate, and check mated positions found\n";

  bool more_to_do = true;
  int mate_in = 0;
  while (more_to_do) {
    more_to_do = false;
    ++mate_in;
    
    // INVARIANT: All wins/loses in strictly less than mate_distance
    // have already been identified

    int num_remaining_positions = 0;
    for (int _player=0; _player<2; _player++) {

      bool win_shown = !*(endgame_settings->construction_show_progress);
      bool draw_shown = win_shown;
      bool loss_shown = draw_shown;

      // In a symmetric endgame, both 2 scans are performed for white-to-move
      int player = symmetric_endgame ? 0 : _player;

      for (uint i=0; i<table_size; i++) {
	//if ((i&0xFF) == 0) cerr << i << ", " << num_remaining_positions << "\n";
	if (table[player][i] == ENDGAME_TABLE_UNKNOWN) {
	  num_remaining_positions++;

	  (*decompress_table_index)(i, piece_list);
	  board.set_board(piece_list, player);

	  int best = 0;
	  Move move = board.moves();
	  bool unknown_value_found = false;
	  bool draw_possible = false;
	  //try {
	  while (board.next_move(move)) {
	    //if (i == 413816) cout << "move(" << board.moveToSAN(move) << ")";


	    Undo undo = board.execute_move(move);
	    // It is important that the nescessary endgames have been loaded!
	    // (e.g. KRPK requires KRK, KQRK, KRRK, KRBK, KRNK, KPK)
	    int value = endgames[board.get_endgame_hashing()][board];
	    board.undo_move(move, undo);

	    if (value == ENDGAME_TABLE_WIN  ||  value == ENDGAME_TABLE_LOSS) {
	      cerr << "Error! Construction of endgame table " << name << "\n"
		   << "requires that the necessary endgames are available with\n"
		   << "distance to mate information, win/loss/draw not enough.\n"
		   << "Position that has only w/d/l information:\n";
	      board.print_board(cerr);
	      exit(1);
	    }

	    // Make value relative to current player
	    if (!is_special_value(value)) {
	      if (value > 0) value = -value; else value = 1-value;
	    }
	    
	    if (is_special_value(value)) {
	      switch (value) {
	      case ENDGAME_TABLE_DRAW:
		draw_possible = true;
		break;
	      case ENDGAME_TABLE_UNKNOWN:
		unknown_value_found = true;
		break;
	      case ENDGAME_TABLE_ILLEGAL:
		{
		  cerr << "Error: ENDGAME_TABLE_ILLEGAL: Player from index = " << i << "\n"
		       << "(it is " << _player << " to move)\n";
		  board.print_board(cerr);
		  board.print_moves(cerr);
		  cerr << "Move executed was " << move.toString() << "\n";
		  Undo undo = board.execute_move(move);

		  vector<PiecePos> pp(board.get_num_pieces());
		  board.get_encoded_piece_list(pp);

		  cerr << "Pieces:";
		  for (uint i=0; i<pp.size(); i++)
		    cerr << " (" << PIECE_NAME[pp[i].piece] << "," << POS_NAME[pp[i].pos] << ")";
		  cerr << "\n";

		  if (num_pieces==board.get_num_pieces()  &&  !move.is_pawn_promotion()) {
		    cerr << "Illegal position has index "
			 << get_table_and_bdd_index_and_stm(board).first << "\n";
		  } else {
		    // No longer the same endgame => compress_table_index can't be used
		  }
		  board.print_board(cerr);
		  board.print_moves(cerr);
		  assert(0);
		  break;
		}
	      }
	    } else {
	      if (value > 0) {
		// can mate => minimize distance to mate
		if (best <= 0  ||  value < best)
		  best = value;
	      } else {
		// will be mated => maximize distance to mate
		if (best <= 0  &&  value < best)
		  best = value;
	      }
	    }
	  }
	  /*
	    }
	    catch (Error e) {
	    cerr << "Error: " << e.name << "\n";
	    decompress_index(i, player, piece_list);
	    board.set_board(piece_list, player);
	    board.print_board(cerr);
	    cerr << "Filename = " << filename << "\n";
	    assert(0);
	    }
	  */
	  

	  //todo: best<=mate_in (eller er det omvendt)

	  if (best >= mate_in) {
	    // The game is won in at least mate_in moves.

	    // If this position could be won in less than mate_in moves,
	    // we would already have identified it (the invariant).
	    // Hence mate_in is least possible number of moves to a check mate.
	    // However, if best > mate_in then we dont know yet if a check mate
	    // in less than best exists.
	    if (best == mate_in  ||  !unknown_value_found) {
	      table[player][i] = best;
	      if (!win_shown) {
		win_shown = true;
		cerr << "Winning: M" << best << ", index = " << i << '\n';
		board.print_board(cerr);
	      }
	    }

	    // Even if best > mate_in we have to set more_to_do to true, because
	    // the value of this entry havent been determined yet.
	    more_to_do = true;

	  } else if (!unknown_value_found) {
	    if (best > 0) {
	      assert(0);
	    } else if (draw_possible) {
	      if (!draw_shown) {
		draw_shown = true;
		cerr << "Draw possible, index = " << i << '\n';
		board.print_board(cerr);
	      }
	      table[player][i] = ENDGAME_TABLE_DRAW;
	      more_to_do = true;
	    } else { // ie ...
	      if (!loss_shown) {
		loss_shown = true;
		cerr << "Losing: -M" << -best << ", index = " << i << '\n';
		board.print_board(cerr);
	      }
	      table[player][i] = best;
	      more_to_do = true;
	    }
	  }
	}
      }
    }
    cerr << "All wins/losses in " << mate_in << " move(s) found\n"
	 << "(" << num_remaining_positions << " positions with unknown values were examined)\n";
  }
  
  for (int player=0; player<(symmetric_endgame ? 1 : 2); player++)
    for (uint i=0; i<table_size; i++)
      if (table[player][i] == ENDGAME_TABLE_UNKNOWN)
	table[player][i] = ENDGAME_TABLE_DRAW;

  release_dependency(old_states_wtm);
  release_dependency(old_states_btm);
}

#undef akfhlkwehkjlshdfl
