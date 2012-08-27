#include "endgame_retrograde_construction.hxx"

void build_endgame_retrograde(EndgameFunctionality* endgame, char **table) {
  int num_pieces = endgame->get_num_pieces();
  Board2 board;
  vector<PiecePos> piece_list(num_pieces);
  for (int i=0; i<num_pieces; i++)
    piece_list[i].piece = endgame->get_piece(i);

  // Each table entry contains the value of its best move so far,
  // and the number of remaining untried moves.

  // Perform forward scan to identify all leaf nodes, count the remaining
  // untried edges of each node, and use examine all edges to reduced positions.
  uchar *move_count[2];

  // Remember the maximal mate depth in the table - do not stop
  // until this is reached
  int max_mate_depth = 0;

  // SAME_DIAGONAL[p1]==SAME_DIAGONAL[p2] && p1!=p2   iff
  // p1 and p2 are positions on the same diagonal.
  const uchar SAME_DIAGONAL[64] =
  {   0, 1, 2, 3, 4, 5, 6, 7,
      8, 0,10,11,12,13, 7,15,
      16,17, 0,19,20, 7,22,23,
      24,25,26, 0, 7,29,30,31,
      32,33,34, 7, 0,37,38,39,
      40,41, 7,43,44, 0,46,47,
      48, 7,50,51,52,53, 0,55,
      7,57,58,59,60,61,62, 0
  };



  // ##################################################################
  // ############   Use the dependency endgames    ####################
  // ##################################################################

  for (int player=0; player<(endgame->is_symmetric_endgame() ? 1 : 2); player++) {

    if (!table[player])
      table[player] = new char[endgame->get_table_size()];

    map<int, EFState> old_states = endgame->load_dependency(player);

    for (uint i=0; i<endgame->get_table_size(); i++) {
      endgame->decompress_table_index(i, piece_list);
      bool legal_position = board.set_board(piece_list, player);

      if (legal_position) {
        int status;
        switch (status = board.calc_game_status()) {
        case GAME_OPEN:
        {
          int best_value = 0;// remember 0 represents a lost position, worst possible

          Move move = board.moves();
          while (board.next_move(move)) {
            if (move.is_en_passant()  ||  move.is_pawn_promotion()  ||  board[move.to]) {
              // this edge go to a position not in this endgame
              Undo undo = board.execute_move(move);
              int value = endgames[board.get_endgame_hashing()][board];
              board.undo_move(move, undo);

              if (value == ENDGAME_TABLE_WIN  ||  value == ENDGAME_TABLE_LOSS) {
                cerr << "Error! Construction of endgame table " << endgame->get_name() << "\n"
                    << "requires that the necessary endgames are available with\n"
                    << "distance to mate information, win/loss/draw not enough.\n"
                    << "Position that has only w/d/l information:\n";
                board.print_board(cerr);
                exit(1);
              }

              if (value == ENDGAME_TABLE_UNKNOWN  ||  value == ENDGAME_TABLE_ILLEGAL) {
                cerr << "Error: Retrograde constr.: Player from index = " << i << "\n"
                    << "(it is " << player << " to move)\n";
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
                      << endgame->get_table_and_bdd_index_and_stm(board).first << "\n";
                } else {
                  // No longer the same endgame => compress_table_index can't be used
                }
                board.print_board(cerr);
                board.print_moves(cerr);
                assert(0);
                exit(1);
              }


              // Make value relative to current player
              if (!is_special_value(value)) {
                if (value > 0) value = -value; else value = 1-value;
              }

              if (endgame_cmp(value, best_value) > 0) {
                //cerr << "Updating " << best_value << " to " << value << "\n";
                best_value = value;
              }
            }
          }

          table[player][i] = best_value;

          //if (best_value > 0)
          // cerr << "table[" << player << "][" << i << "] = " << best_value << "\n";

          if (best_value > max_mate_depth) max_mate_depth = best_value;
        }
        break;
        case GAME_DRAWN:
          table[player][i] = ENDGAME_TABLE_DRAW;
          break;
        case WHITE_WON:
        case BLACK_WON:
          table[player][i] = 0;//-M0
          break;
        default:
          assert(0);
        }

      } else {
        table[player][i] = ENDGAME_TABLE_ILLEGAL;
      }
    }

    endgame->release_dependency(old_states);
  }

  // ##################################################################
  // ############   Initialize the edge counts     ####################
  // ##################################################################

  for (int player=0; player<(endgame->is_symmetric_endgame() ? 1 : 2); player++) {

    move_count[player] = new uchar[endgame->get_table_size()];
    //original_move_count[player] = new uchar[table_size];//debug

    for (uint i=0; i<endgame->get_table_size(); i++) {
      //if ((i&0x3FFF)==0) { cerr << i << " "; cerr.flush(); }
      if (table[player][i] != ENDGAME_TABLE_ILLEGAL) {

        endgame->decompress_table_index(i, piece_list);
        my_assert(board.set_board(piece_list, player));

        bool reduced_symmetry = !endgame->is_pawnless_endgame() ? false :
            (SAME_DIAGONAL[board.get_king_position(0)] ==
                SAME_DIAGONAL[board.get_king_position(1)]);

        move_count[player][i] = 0;

        if (board.calc_game_status() == GAME_OPEN) {
          // In a pawnless endgame without castling, the positions with both kings
          // on the a1-h8 diagonal don't enjoy this diagonal symmetry. Let B be a position
          // with both kings on the a1-h8 diagonal that can be reached by a reversible move
          // from position A that only has one king on the a1-h8 diagonal.
          // From A, B can be reached by making move m. But from both B and its version reflected
          // in the a1-h8 diagonal, move m (or its refl. variant) can be undone such that A
          // is reached.
          // blah blah blah
          //
          int num_untried_edges = 0;

          Move move = board.moves();
          while (board.next_move(move)) {
            if (!(move.is_en_passant()  ||  move.is_pawn_promotion()  ||  board[move.to])) {
              Undo undo = board.execute_move(move);

              bool reduced_symmetry_dest = !endgame->is_pawnless_endgame() ? false :
                  (SAME_DIAGONAL[board.get_king_position(0)] ==
                      SAME_DIAGONAL[board.get_king_position(1)]);

              if (!reduced_symmetry  &&  reduced_symmetry_dest) {
                num_untried_edges += 2;
              } else {
                num_untried_edges += 1;
              }

              board.undo_move(move, undo);
            }

            move_count[player][i] = num_untried_edges;
          }
        }
      }
    }
  }



  // ##################################################################
  // ############   The rest of the construction   ####################
  // ############   will stay inside this endgame  ####################
  // ##################################################################


  for (int n=0; n<=max_mate_depth; n++) {
    if (n > 123) {
      cerr << "Error: Searching for mate depth above 123 not supported.\n";
      exit(1);
    }


    // INVARIANT: All wins/loses in strictly less than mate_distance
    // have already been identified
    bool win_shown = !*(endgame_settings->construction_show_progress);
    bool loss_shown = win_shown;

    for (int _player=0; _player<2; _player++) {
      int n_win = endgame->is_symmetric_endgame() ? (_player ? n+1 : 9999) : (n+_player);
      if (n_win==0) n_win = 9999;
      int n_loss = (endgame->is_symmetric_endgame() && _player) ? 9999 : -n;

      int player = endgame->is_symmetric_endgame() ? 0 : _player;

      for (uint i=0; i<endgame->get_table_size(); i++) {

        if (table[player][i] == n_win  ||
            (table[player][i] == n_loss  &&  move_count[player][i] == 0)) {
          // Either: Position is won in n moves
          // or: Position is lost in n moves (only if all moves have been checked)

          endgame->decompress_table_index(i, piece_list);
          board.set_board(piece_list, player);
          vector<triple<Move,Undo,int> > retro_moves =
              board.get_retro_moves(false, true, false, true);

          bool reduced_symmetry = !endgame->is_pawnless_endgame() ? false :
              (SAME_DIAGONAL[board.get_king_position(0)] ==
                  SAME_DIAGONAL[board.get_king_position(1)]);

          for (uint j=0; j<retro_moves.size(); j++) {
            assert(!retro_moves[j].second.captured_piece);
            assert(!retro_moves[j].first.is_en_passant());
            assert(!retro_moves[j].first.is_pawn_promotion());

            my_assert(board.transform_board(retro_moves[j].third));
            board.undo_move(retro_moves[j].first, retro_moves[j].second);

            bool reduced_symmetry_before = !endgame->is_pawnless_endgame() ? false :
                (SAME_DIAGONAL[board.get_king_position(0)] ==
                    SAME_DIAGONAL[board.get_king_position(1)]);

            pair<uint, int> back = endgame->get_table_index_and_stm(board);
            char *ref = &(table[back.second][back.first]);

#ifndef NDEBUG
            if (!reduced_symmetry  &&  reduced_symmetry_before) {
              // Verify that the symmetric version is identical
              my_assert(board.transform_board(4));

              pair<uint, int> back2 = endgame->get_table_index_and_stm(board);
              assert(table[back2.second][back2.first] == table[back.second][back.first]);
              assert(move_count[back2.second][back2.first] = move_count[back.second][back.first]);

              my_assert(board.transform_board(4));
            }
#endif

            char *ref2 = 0;
            if (!reduced_symmetry  &&  reduced_symmetry_before) {
              // Make the same changes to the symmetric version
              my_assert(board.transform_board(4));

              pair<uint, int> back2 = endgame->get_table_index_and_stm(board);
              ref2 = &(table[back2.second][back2.first]);
              --move_count[back2.second][back2.first];

              my_assert(board.transform_board(4));
            }


            if (table[player][i] == n_win) {
              // This position is won. The other player will have no interest in reaching
              // this position unless it is his last choice.

              if (*ref <= 0  &&  *ref > -n_win) {
                // The previous stored value was an even quicker loss
                *ref = -n_win;
              }

              assert(move_count[back.second][back.first] > 0);

              if (!--move_count[back.second][back.first]  &&
                  *ref != ENDGAME_TABLE_DRAW  &&  *ref <= 0) {

                // Now its for sure that the position is lost!
                // Maybe the mate depth is longer than the maximum so far.
                if (-*ref > max_mate_depth) max_mate_depth = -*ref;

                if (!loss_shown) {
                  loss_shown = true;
                  cerr << "Losing: -M" << (int)n_win << ", index = " << i << "\n"
                      << "Max mate depth so far = " << max_mate_depth << "\n";
                  board.print_board(cerr);
                }
              }


            } else {
              assert(table[player][i] == n_loss  &&  move_count[player][i] == 0);
              assert(move_count[back.second][back.first] > 0);

              --move_count[back.second][back.first];

              // This position is lost. It is optimal for the other player to reach this
              // node if he can't already mate faster (than in n+1)
              if (*ref <= 0  ||  -n_loss+1 < *ref) {

                // The is the best move for the back vertex
                if ((*ref = -n_loss+1) > max_mate_depth) max_mate_depth = *ref;

                if (!win_shown) {
                  win_shown = true;
                  cerr << "Winning: M" << (int)*ref << ", index = " << i << "\n"
                      << "Max mate depth so far = " << max_mate_depth << "\n";
                  board.print_board(cerr);
                }
              }
            }

            // Copy update to symmetric version
            if (ref2) *ref2 = *ref;

#ifdef NDEBUG
            board.execute_move(retro_moves[j].first);
#else
            // More testing of retro moves
            assert(board.execute_move(retro_moves[j].first) == retro_moves[j].second);
#endif
            my_assert(board.inv_transform_board(retro_moves[j].third));
          }
        }
      }
    }
  }


  // No more winning nodes! The nodes that still have edges connected
  // must be drawn.

  for (int player=0; player<(endgame->is_symmetric_endgame() ? 1 : 2); player++) {
    for (uint i=0; i<endgame->get_table_size(); i++) {
      if (move_count[player][i]  &&  table[player][i] <= 0)
        table[player][i] = ENDGAME_TABLE_DRAW;
    }

    delete move_count[player];
    //delete original_move_count[player];//debug
  }

}
