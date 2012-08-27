#ifndef _BOARD_PRINTERS_
#define _BOARD_PRINTERS_


template <class TYPE>
void print_map64(std::ostream &os, TYPE *list64, int num_digits=0, int base=10) {
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
void print_latex_map64(std::ostream &os, TYPE *list64, int num_digits=0, int base=10) {
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
void print_latex_signed_map64(std::ostream &os, TYPE *list64, int num_digits=0, int base=10) {
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
void print_string_map64(std::ostream &os, TYPE *list64, int padded_length) {
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
void print_latex_string_map64(std::ostream &os, TYPE *list64, int padded_length) {
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


#endif
