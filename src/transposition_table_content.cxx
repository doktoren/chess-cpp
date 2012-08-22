#include "transposition_table_content.hxx"

ostream& operator<<(ostream& os, const Info& info) {
  static const string EVAL_TYPE_NAMES[4] =
  {"empty info", "upper bound", "exact eval", "lower bound"};
  if (info.is_valid()) {
    os << "Info(val=" << info.value << ",depth=" << (int)info.depth << ',';
    if (info.eval_type & ~3) os << (int)info.eval_type;
    else os << EVAL_TYPE_NAMES[info.eval_type];
    os << ')';
  } else {
    os << "Info(empty)";
  }
  return os;
}
