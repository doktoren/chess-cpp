#include "board_3.hxx"

#include "board_define_position_constants.hxx"
#include "board_move_tables.hxx"
#include "file_loader.hxx"

#include "util/hash_table.hxx"

ostream& operator<<(ostream& os, const RepetitionInfo& rep) {
  os << "NumRep(" << rep.num_repetitions << ')';
  return os;
}

bool exec_undo_activated = false;

#define Extends Board2
#define ClassName Board3
#define nameofclass "Board3"
#include "board_3_class_implementation.hxx"
#undef nameofclass
#undef ClassName
#undef Extends
#define Extends Board2plus
#define ClassName Board3plus
#define nameofclass "Board3plus"
#define PLUS
#include "board_3_class_implementation.hxx"
#undef PLUS
#undef nameofclass
#undef ClassName
#undef Extends
