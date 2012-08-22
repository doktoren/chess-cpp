#include "streams.hxx"


ofstream big_output("big_output.txt");

// This dummy struct makes big_output unbuffered.
struct DUMMY_gah {
  DUMMY_gah() {
    big_output.setf(ios::unitbuf);
  }
};
DUMMY_gah Balrog_gah;



#ifdef XBOARD
// When XBoard is used, cerr is redirected to error.txt
ofstream my_error("error.txt");

// This dummy struct makes my_error unbuffered.
struct DUMMY {
  DUMMY() {
    my_error.setf(ios::unitbuf);
  }
};
DUMMY Balrog;

#else

// When XBoard is not used, current_cerr may override cerr
ostream *current_cerr = 0;
#endif



#ifndef XBOARD
My_ostream cout;
#endif



#ifndef NDEBUG
int break_point_number = 0;
#endif


struct_cbo cbo;
