#include <iostream>
#include <assert.h>

#include "move_and_undo.hxx"

#include "help_functions.hxx"

ostream& operator<<(ostream& os, const Move& move) {
  switch (move.blah&7) {
  case 0: os << "Move: Non generator move:\n"; break;
  case 1: os << "Move: All-generator:\n"; break;
  case 2: os << "Move: All-from-generator:\n"; break;
  case 3: os << "Move: All-to-generator:\n"; break;
  case 4: os << "Move: All-from-to-generator:\n"; break;
  case 5: os << "Move: All-from-direction-generator: (not impl. yet?)\n"; break;
  case 6: os << "Move: Fast (all) generator:\n"; break;
  default:
    assert(0);
  }
  os << "    (from, to) = (" << POS_NAME[move.from] << ", "
     << POS_NAME[move.to] << ")\n";
  if (move.special_move) {
    if (move.special_move & 0xF) {
      os << "    Pawn promote to " << PIECE_NAME[move.special_move] << '\n';
    } else {
      switch (move.special_move) {
      case EN_PASSANT:
	os << "    Using en passant on " << POS_NAME[move.to] << '\n'; break;
      case WHITE_LONG_CASTLING:
	os << "    White long castling\n"; break;
      case WHITE_SHORT_CASTLING:
	os << "    White short castling\n"; break;
      case BLACK_LONG_CASTLING:
	os << "    Black long castling\n"; break;
      case BLACK_SHORT_CASTLING:
	os << "    Black short castling\n"; break;
      default:
	os << "    Error: illegal special_move value: "
	   << (int)move.special_move << "\n";
      }
    }
  }
  return os;
}

/*
Old Undo:
ostream& operator<<(ostream& os, const Undo& undo) {
  os << "Undo information:\n";
  if (undo.killed_piece_or_en_passant & 0xF0) {
    os << " - en passant was possible at colum "
       << COLUMN_NAME[(undo.killed_piece_or_en_passant >> 4)-8] << '\n';
  }
  if (undo.killed_piece_or_en_passant & 0xF) {
    os << " - " << PPIECE_NAME[undo.killed_piece_or_en_passant & 0xF]
       << " was captured.\n";
  }
  os << " - castling (White long, short), (B.l., s.) = ("
     << ((undo.castling&0x10) ? "YES" : "NO") << ","
     << ((undo.castling&0x20) ? "YES" : "NO") << "),("
     << ((undo.castling&0x40) ? "YES" : "NO") << ","
     << ((undo.castling&0x80) ? "YES" : "NO") << ")\n";
  os << " - num checks = " << (int)(undo.num_checks_and_threat_pos>>6) << '\n';
  if (undo.num_checks_and_threat_pos>>6) {
    os << " - (threat position = " << POS_NAME[undo.num_checks_and_threat_pos & 0x3F] << ")\n";
  }
  os << " - moves played since progress = "
     << (int)undo.moves_played_since_progress << '\n';
  return os;
}

string Undo::toString() const {
  string result = "Undo(";
  // En passant:
  if (killed_piece_or_en_passant & 0xF0) {
    result += "ep(" + COLUMN_NAME[(undo.killed_piece_or_en_passant >> 4)-8] + "),";
  // Captures:
  }
  if (killed_piece_or_en_passant & 0x0F) {
    result += "captured(" + string(1, PIECE_CHAR[killed_piece_or_en_passant]) + "),";
  }
  // Castling:
  result += string("castling(wl") + ((castling&0x10) ? "+" : "%") + "s" + ((castling&0x20) ? "+" : "%")
    + "bl" + ((castling&0x40) ? "+" : "%") + "s" + ((castling&0x80) ? "+" : "%") + "),";
  // Num checks:
  result += "num+(" + ::toString((int)(num_checks_and_threat_pos>>6), 10) + "),";
  // Moves played since progress:
  result += "mpsp(" + ::toString((int)moves_played_since_progress, 10) + "))";

  return result;
}
*/



string Undo::toString() const {
  string result = "Undo(";
  // Piece captured?
  if (captured_piece) {
    result += "captured(" + string(1, PIECE_CHAR[captured_piece]) + "),";
  }
  // King in check ?
  if (num_checks) {
    result += "checked(" + POS_NAME[threat_pos] + "," + 
      ::toString(num_checks, 10) + "),";
  }
  // En passant ?
  if (en_passant != ILLEGAL_POS) {
    result += "en_passant(" + POS_NAME[en_passant] + "),";
  }
  // castling:
  result += string("castling(wl") + ((castling&0x10) ? "+" : "%") + "s" + ((castling&0x20) ? "+" : "%")
    + "bl" + ((castling&0x40) ? "+" : "%") + "s" + ((castling&0x80) ? "+" : "%") + "),";
  // Moves played since progress:
  result += "mpsp(" + ::toString((int)moves_played_since_progress, 10) + "),";

  // player
  if (player) result += "btm)"; else result += "wtm)";

  return result;
}

ostream& operator<<(ostream& os, const Undo& undo) {
  os << undo.toString() << "\n";
  return os;
}



string Move::toString() const {
  if (is_pawn_promotion())
    return POS_NAME[from] + POS_NAME[to&0x7F] +
      string(1, PIECE_CHAR[PIECE_KIND[special_move]+6]);
  return POS_NAME[from] + POS_NAME[to&0x7F];
}

string Move::toString2() const {
  string m = toString();
  if (special_move) {
    if (is_castling()) m += "-c";
    if (is_en_passant()) m += "-ep";
  }
  if (blah) {
    if (blah&0xF) {
      m += "-mg ";
      m[m.size()-1] = (blah&0xF)+'0';
    }
    if (blah&0xF0) {
      if (blah & CAN_ATTACK) m += "-ca";
      if (blah & CAN_NON_ATTACK) m += "-cna";
      if (blah & FURTHER_MOVEMENT_POSSIBLE) m += "-fmp";
    }
  }
  return m;
}


Move::Move(string cag, bool player) {
  from = strToPos(string(cag,0,2));
  to = strToPos(string(cag,2,2));
  special_move = blah = 0;
  if (cag.length() == 5) {
    // pawn promotion
    switch (cag[4]) {
    case 'n': special_move = KNIGHT; break;
    case 'b': special_move = BISHOP; break;
    case 'r': special_move = ROOK;   break;
    case 'q': special_move = QUEEN;  break;
    default:  assert(0);
    }
    if (player) special_move += 6;
  }
}
