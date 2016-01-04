#include "endgame_piece_enumerations.hxx"

#include <stdarg.h>
#include <map>

bool piece_overlap(int num_pieces, ...) {
  int pos[8];
  va_list ap;
  va_start(ap, num_pieces);
  for (int i=0; i<num_pieces; i++) {
    pos[i] = va_arg(ap, int);
    for (int j=0; j<i; j++)
      if (pos[i]==pos[j]) return true;
  }
  return false;
}


/*
  if BOUND_KING==1 then the board will be rotated/mirrored such that
  the position of the black king will be changed accordingly to the table below.
  Similar for white king if BOUND_KING==0
+++++++++++++++++++++++++++
+  0  1  2  3  3  2  1  0 +
+  1  9 10 11 11 10  9  1 +
+  2 10 18 19 19 18 10  2 +
+  3 11 19 27 27 19 11  3 +
+  3 11 19 27 27 19 11  3 +
+  2 10 18 19 19 18 10  2 +
+  1  9 10 11 11 10  9  1 +
+  0  1  2  3  3  2  1  0 +
+++++++++++++++++++++++++++
 */

int XX_COMPRESS[64*64];// XX_compress[(j<<6)|i], j<i
pair<Position, Position> XX_DECOMPRESS[63*64/2];
void init_XX_tables() {
  int index = 0;
  for (int piece1=0; piece1<64; piece1++) {
    for (int piece2=piece1+1; piece2<64; piece2++) {
      xx_compress(piece1, piece2) = index;
      XX_DECOMPRESS[index] = pair<Position, Position>(piece1, piece2);
      ++index;
    }
  }
  for (int piece1=0; piece1<64; piece1++) {
    for (int piece2=0; piece2<piece1; piece2++)
      xx_compress(piece1, piece2) = xx_compress(piece2, piece1);
    xx_compress(piece1, piece1) = -1;
  }

  assert(index == 63*64/2);
}

int PP_COMPRESS[64*64];
pair<Position, Position> PP_DECOMPRESS[47*48/2];
void init_PP_tables() {
  int index = 0;

  for (int pawn1=0; pawn1<64; pawn1++)
    for (int pawn2=0; pawn2<64; pawn2++)
      pp_compress(pawn1, pawn2) = -1;

  for (int pawn1=8; pawn1<56; pawn1++) {
    for (int pawn2=pawn1+1; pawn2<56; pawn2++) {
      pp_compress(pawn1, pawn2) = index;
      PP_DECOMPRESS[index] = pair<Position, Position>(pawn1,pawn2);
      ++index;
    }
  }
  for (int pawn1=8; pawn1<56; pawn1++) {
    for (int pawn2=8; pawn2<pawn1; pawn2++)
      pp_compress(pawn1, pawn2) = pp_compress(pawn2, pawn1);
  }

  assert(index == 47*48/2);
}


#ifdef ALLOW_5_MEN_ENDGAME

int XXX_COMPRESS_P1[62];
int XXX_COMPRESS_P2_MINUS_64[63];
triple<Position, Position, Position> XXX_DECOMPRESS[62*63*64/6];
void init_XXX_tables() {
  // pieces x<y<z mapped to index
  // $-64 + \frac{11531*x-186*x^2+x^3}{6} + \frac{125*y-y^2}{2} + z$

  for (int p1=0; p1<62; p1++)
    XXX_COMPRESS_P1[p1] = (11531*p1-186*p1*p1+p1*p1*p1)/6;

  for (int p2=0; p2<63; p2++)
    XXX_COMPRESS_P2_MINUS_64[p2] = (125*p2-p2*p2)/2 - 64;

  for (int p1=0; p1<62; p1++)
    for (int p2=p1+1; p2<63; p2++)
      for (int p3=p2+1; p3<64; p3++)
        XXX_DECOMPRESS[xxx_compress(p1, p2, p3)] =
            triple<Position, Position, Position>(p1,p2,p3);
}


int PPP_COMPRESS_P1[54];
int PPP_COMPRESS_P2_MINUS_10480[55];
triple<Position, Position, Position> PPP_DECOMPRESS[46*47*48/6];
void init_PPP_tables() {
  // pawns x<y<z mapped to index
  // $-10480 + \frac{17494*x - 324*x*x + 2*x*x*x}{12} + \frac{109*y - y*y}{2} + z$

  for (int pawn1=8; pawn1<54; pawn1++)
    PPP_COMPRESS_P1[pawn1] = (17494*pawn1 - 324*pawn1*pawn1 + 2*pawn1*pawn1*pawn1)/12;

  for (int pawn2=9; pawn2<55; pawn2++)
    PPP_COMPRESS_P2_MINUS_10480[pawn2] = (109*pawn2 - pawn2*pawn2)/2 - 10480;

  for (int pawn1=8; pawn1<54; pawn1++)
    for (int pawn2=pawn1+1; pawn2<55; pawn2++)
      for (int pawn3=pawn2+1; pawn3<56; pawn3++)
        PPP_DECOMPRESS[ppp_compress(pawn1, pawn2, pawn3)] =
            triple<Position, Position, Position>(pawn1,pawn2,pawn3);
}

#endif



// If BOUND_KING==1 then black king will be bound to the
// a1-d1-d4 triangle. Otherwise for white king.
//
//                           KING_FULL_SYMMETRY
//                                   --->
// (pos(white king),pos(black king))      (index, reflection) : IndexRefl
//     | : pair<uint8_t,uint8_t>     <---     (index==-1  <=>  invalid)
//     |                         KING_FS_POS
//     |
//     | BDD_KING_FULL_SYMMETRY = KING_FS_POS o KING_FULL_SYMMETRY
//     V
//  (pos(white king),pos(black king),reflection,valid) : BDDIndexRefl
//
IndexRefl KING_FULL_SYMMETRY[64*64];
pair<Position, Position> KING_FS_POS[462];
BDDIndexRefl BDD_KING_FULL_SYMMETRY[64*64];

void init_king_full_symmetry() {
  const bool print_pairs = false;

  // first vertical reflection is applied, then horizontal and
  // finally diagonal
  // 1: vertical reflection
  // 2: horizontal reflection
  // 4: diagonal reflection
  int REFLECTION[64] =
  { 0, 0, 0, 0, 1, 1, 1, 1,
      4, 0, 0, 0, 1, 1, 1, 5,
      4, 4, 0, 0, 1, 1, 5, 5,
      4, 4, 4, 0, 1, 5, 5, 5,
      6, 6, 6, 2, 3, 7, 7, 7,
      6, 6, 2, 2, 3, 3, 7, 7,
      6, 2, 2, 2, 3, 3, 3, 7,
      2, 2, 2, 2, 3, 3, 3, 3 };

  map<pair<int,int>, int> tmp;
  int number = 0;

  for (int bound_king=0; bound_king<64; bound_king++)
    for (int free_king=0; free_king<64; free_king++) {

#if BOUND_KING==1
      int white_king = free_king;
      int black_king = bound_king;
#else
      int white_king = bound_king;
      int black_king = free_king;
#endif


      if (kings_too_near(white_king, black_king)) {
        // discarded
        king_full_symmetry(white_king, black_king) = IndexRefl(-1, 0);
      } else {

        int refl = REFLECTION[bound_king];

        int bound_pos = reflect(bound_king, refl);
        int free_pos = reflect(free_king, refl);
        if (ROW[bound_pos]==COLUMN[bound_pos]  &&
            ROW[free_pos]>COLUMN[free_pos]) {
          refl += 4;
          free_pos = reflect(free_pos, 4);
        }

#if BOUND_KING==1
        pair<int, int> index(free_pos, bound_pos);
#else
        pair<int, int> index(bound_pos, free_pos);
#endif

        if (tmp.count(index)==0) {
          if (print_pairs)
            cerr << '(' << POS_NAME[index.first] << ", " << POS_NAME[index.second] << "), ";
          KING_FS_POS[number] = index;
          tmp[index] = ++number;
        }

        king_full_symmetry(white_king, black_king) = IndexRefl(tmp[index]-1, refl);
      }
    }

  assert(number == 462);


  // Initialize BDD_KING_FULL_SYMMETRY
  for (int white_king=0; white_king<64; white_king++) {
    for (int black_king=0; black_king<64; black_king++) {
      IndexRefl ir = king_full_symmetry(white_king, black_king);
      if (ir.is_valid()) {
        bdd_king_full_symmetry(white_king, black_king) = BDDIndexRefl(KING_FS_POS[ir.index], ir.refl);
      } else {
        bdd_king_full_symmetry(white_king, black_king).valid = false;
      }
    }
  }
}
void verify_king_full_symmetry() {
  cerr << "verifying king full symmetry...\n";
  for (int white_king=0; white_king<64; white_king++) {
    for (int black_king=0; black_king<64; black_king++) {
      if (!kings_too_near(white_king, black_king)) {
        IndexRefl ir = king_full_symmetry(white_king,black_king);
        pair<Position, Position> wblack_king = KING_FS_POS[ir.index];

        int _white_king = reflect(white_king, ir.refl);
        int _black_king = reflect(black_king, ir.refl);

        if (wblack_king.first != _white_king  ||  wblack_king.second != _black_king) {
          cerr << "Error: k1k2 = (" << POS_NAME[wblack_king.first] << ','
              << POS_NAME[wblack_king.second]
                          << "), (k1,k2) = (" << POS_NAME[white_king] << ',' << POS_NAME[black_king]
                                                                                         << "), (_k1,_k2) = (" << POS_NAME[_white_king] << ',' << POS_NAME[_black_king]
                                                                                                                                                           << "), index = " << ir.index << ", refl = " << ir.refl << "\n";
        }

        __attribute__((unused)) BDDIndexRefl bir = bdd_king_full_symmetry(white_king, black_king);
        assert(bir.is_valid());
        assert(reflect(white_king, bir.refl) == bir.white_king());
        assert(reflect(black_king, bir.refl) == bir.black_king());
      }
    }
  }
}

// If BOUND_KING==1 then black king will be bound to the
// a1-d1-d4 triangle. Otherwise for white king.
//
//                           KING_PAWN_SYMMETRY
//                                   --->
// (pos(white king),pos(black king))      (index, reflection) : IndexRefl
//     | : pair<uchar,uchar>         <---     (index==-1  <=>  invalid)
//     |                         KING_FS_POS
//     |
//     | BDD_KING_PAWN_SYMMETRY = KING_PS_POS o KING_PAWN_SYMMETRY
//     V
//  (pos(white king),pos(black king),reflection,valid) : BDDIndexRefl
//
IndexRefl KING_PAWN_SYMMETRY[64*64];
pair<Position, Position> KING_PS_POS[1806];
BDDIndexRefl BDD_KING_PAWN_SYMMETRY[64*64];

void init_king_pawn_symmetry() {
  const bool print_pairs = false;

  map<pair<int,int>, int> tmp;// Not very efficient, but whatever
  int number = 0;

  for (int bound_king=0; bound_king<64; bound_king++)
    for (int free_king=0; free_king<64; free_king++) {

#if BOUND_KING==1
      int white_king = free_king;
      int black_king = bound_king;
#else
      int white_king = bound_king;
      int black_king = free_king;
#endif

      if (kings_too_near(white_king, black_king)) {
        // discarded
        king_pawn_symmetry(white_king, black_king) = IndexRefl(-1, 0);
      } else {

        int refl = COLUMN[bound_king]>3 ? 1 : 0;
        pair<int, int> index(reflect(white_king, refl), reflect(black_king, refl));

        if (tmp.count(index)==0) {
          if (print_pairs)
            cerr << '(' << POS_NAME[index.first] << ", " << POS_NAME[index.second] << "), ";
          KING_PS_POS[number] = index;
          tmp[index] = ++number;
        }

        king_pawn_symmetry(white_king, black_king) = IndexRefl(tmp[index]-1, refl);
      }
    }

  assert(number == 1806);

  // Initialize BDD_KING_PAWN_SYMMETRY
  for (int white_king=0; white_king<64; white_king++) {
    for (int black_king=0; black_king<64; black_king++) {
      IndexRefl ir = king_pawn_symmetry(white_king, black_king);
      if (ir.is_valid()) {
        assert(ir.refl==0  ||  ir.refl==1);
        bdd_king_pawn_symmetry(white_king, black_king) =
            BDDIndexRefl(KING_PS_POS[ir.index], ir.refl);

        /*
	cerr << "ir.refl = " << ir.refl << "\n";
	cerr << (int)KING_PS_POS[ir.index].first << " = KING_PS_POS[" << ir.index << "].first =?= "
	     << "bdd_king_pawn_symmetry(" << white_king << ", " << black_king << ").white_king() = "
	     << (int)bdd_king_pawn_symmetry(white_king, black_king).white_king() << "\n";
	cerr << (int)KING_PS_POS[ir.index].second << " = KING_PS_POS[" << ir.index << "].second =?= "
	     << "bdd_king_pawn_symmetry(" << white_king << ", " << black_king << ").black_king() = "
	     << (int)bdd_king_pawn_symmetry(white_king, black_king).black_king() << "\n";
         */
        assert(KING_PS_POS[ir.index].first ==
            bdd_king_pawn_symmetry(white_king, black_king).white_king());
        assert(KING_PS_POS[ir.index].second ==
            bdd_king_pawn_symmetry(white_king, black_king).black_king());

        assert(KING_PS_POS[ir.index].first == reflect(white_king, ir.refl));
        assert(KING_PS_POS[ir.index].second == reflect(black_king, ir.refl));

        assert(bdd_king_pawn_symmetry(white_king, black_king).white_king() ==
            reflect(white_king, ir.refl));
        assert(bdd_king_pawn_symmetry(white_king, black_king).black_king() ==
            reflect(black_king, ir.refl));
      } else {
        bdd_king_pawn_symmetry(white_king, black_king).valid = false;
      }
    }
  }
}
void verify_king_pawn_symmetry() {
  cerr << "verifying king pawn symmetry...\n";
  for (int white_king=0; white_king<64; white_king++) {
    for (int black_king=0; black_king<64; black_king++) {
      if (!kings_too_near(white_king, black_king)) {
        IndexRefl ir = KING_PAWN_SYMMETRY[(white_king << 6) | black_king];
        pair<Position, Position> wblack_king = KING_PS_POS[ir.index];

        int _white_king = reflect(white_king, ir.refl);
        int _black_king = reflect(black_king, ir.refl);

        if (wblack_king.first != _white_king  ||  wblack_king.second != _black_king) {
          cerr << "Error: wblack_king = (" << POS_NAME[wblack_king.first] << ','
              << POS_NAME[wblack_king.second] << "), (white_king,black_king) = ("
              << POS_NAME[white_king] << ',' << POS_NAME[black_king]
                                                         << "), (_white_king,_black_king) = (" << POS_NAME[_white_king]
                                                                                                           << ',' << POS_NAME[_black_king]
                                                                                                                              << "), index = " << ir.index << ", refl = " << ir.refl << "\n";
        }


        BDDIndexRefl bir = bdd_king_pawn_symmetry(white_king, black_king);
        if (!bir.is_valid()  ||
            reflect(white_king, bir.refl) != bir.white_king()  ||
            reflect(black_king, bir.refl) != bir.black_king()) {
          cerr << "verify_king_pawn_symmetry error:\n"
              << "white_king = " << white_king << ", black_king = "
              << black_king << ", bir.refl = " << (int)bir.refl
              << ", bir.white_king = " << (int)bir.white_king()
              << ", bir.black_king = " << (int)bir.black_king() << "\n";
          assert(0);
        }
      }
    }
  }
}


#ifdef ALLOW_5_MEN_ENDGAME
void verify_xxx_compress() {
  cerr << "Verifying xxx_compress()...\n";
  for (int i3=2; i3<64; i3++)
    for (int i2=1; i2<i3; i2++)
      for (int i1=0; i1<i2; i1++) {
        int i = xxx_compress(i1, i2, i3);
        triple<Position, Position, Position> t = XXX_DECOMPRESS[i];
        if (i1 != t.first || i2 != t.second || i3 != t.third) {
          cerr << "Error: verify_xxx_compress()\n";
          assert(0);
          exit(1);
        }
      }
}


void verify_ppp_compress() {
  cerr << "Verifying ppp_compress()...\n";
  for (int i3=10; i3<56; i3++)
    for (int i2=9; i2<i3; i2++)
      for (int i1=8; i1<i2; i1++) {
        int i = ppp_compress(i1, i2, i3);
        triple<Position, Position, Position> t = PPP_DECOMPRESS[i];
        if (i1 != t.first || i2 != t.second || i3 != t.third) {
          cerr << "Error: verify_ppp_compress()\n";
          assert(0);
          exit(1);
        }
      }
}
#endif

void verify_piece_enumerations() {
  verify_king_full_symmetry();
  verify_king_pawn_symmetry();
}


int init_piece_enumerations() {
  static bool initialized = false;
  if (initialized) return 0;
  initialized = true;

  init_XX_tables();
  init_PP_tables();
  init_king_full_symmetry();
  init_king_pawn_symmetry();

#ifdef ALLOW_5_MEN_ENDGAME
  init_XXX_tables();
  init_PPP_tables();
#endif

#ifndef NDEBUG
  verify_piece_enumerations();
#ifdef ALLOW_5_MEN_ENDGAME
  verify_xxx_compress();
  verify_ppp_compress();
#endif
#endif
  return 0;
}

int foo = init_piece_enumerations();
