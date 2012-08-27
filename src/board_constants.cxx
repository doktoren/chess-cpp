#include "board_constants.hxx"

const bool WHITE_PIECE[13] =
{   false,
    true, true, true, true, true, true,
    false, false, false, false, false, false
};

const bool BLACK_PIECE[13] =
{   false,
    false, false, false, false, false, false,
    true, true, true, true, true, true
};

const int PIECE_COLOR[13] =
{   -1,
    0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1
};

const Piece PIECE_KIND[13] =
{   0,
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

const Piece SWAP_PIECE_COLOR[13] =
{   0,
    BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING,
    WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING
};

const uint8_t CASTLING[64] =
{   0,0,WHITE_LONG_CASTLING,0,WHITE_LONG_CASTLING+WHITE_SHORT_CASTLING,0,WHITE_SHORT_CASTLING,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,BLACK_LONG_CASTLING,0,BLACK_LONG_CASTLING+BLACK_SHORT_CASTLING,0,BLACK_SHORT_CASTLING,0
};

const uint8_t CASTLING_LOST[64] =
{   0xFF - WHITE_LONG_CASTLING,0xFF,0xFF,
    0xFF,0xFF - (WHITE_LONG_CASTLING+WHITE_SHORT_CASTLING),
    0xFF,0xFF,0xFF - WHITE_SHORT_CASTLING,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF - BLACK_LONG_CASTLING,0xFF,0xFF,
    0xFF,0xFF - (BLACK_LONG_CASTLING+BLACK_SHORT_CASTLING),
    0xFF,0xFF,0xFF - BLACK_SHORT_CASTLING
};

const int ROW[64] =
{   0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7
};

const int COLUMN[64] =
{   0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7,
    0,1,2,3,4,5,6,7
};

const bool POS_COLOR[64] =
{   1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1
};

const Position CR_TO_POS[8][8] = // Colum, Row
{   {  0, 8,16,24,32,40,48,56},
    { 1, 9,17,25,33,41,49,57},
    { 2,10,18,26,34,42,50,58},
    { 3,11,19,27,35,43,51,59},
    { 4,12,20,28,36,44,52,60},
    { 5,13,21,29,37,45,53,61},
    { 6,14,22,30,38,46,54,62},
    { 7,15,23,31,39,47,55,63}
};

inline piece_count_t make(int player, bool zugzwang_piece) {
  piece_count_t result;
  result.individual.num_colored_pieces[player] = 1;
  result.individual.num_colored_pieces[player ^ 1] = 0;
  result.individual.num_pieces = 1;
  result.individual.num_non_zugzwang_pieces = zugzwang_piece ? 0 : 1;
  return result;
}

const piece_count_t PIECE_COUNT_CONSTANTS[13] =
{   {0},

    make(WHITE, true),
    make(WHITE, false),
    make(WHITE, false),
    make(WHITE, false),
    make(WHITE, false),
    make(WHITE, true),

    make(BLACK, true),
    make(BLACK, false),
    make(BLACK, false),
    make(BLACK, false),
    make(BLACK, false),
    make(BLACK, true)
};


const std::string game_result_texts[4] = {"*", "1/2-1/2", "1-0", "0-1"};

const std::string game_status_texts[7] =
{   "* {Game still open}",
    "1/2-1/2 {Fifty move rule}",
    "1-0 {White mates}",
    "0-1 {Black mates}",
    "1/2-1/2 {Stalemate}",
    "1/2-1/2 {Draw by repetition}",
    "1/2-1/2 {Insufficient material}"
};


//######################################################

const char PIECE_CHAR[13] =
{' ','P','N','B','R','Q','K','p','n','b','r','q','k'};
const std::string PIECE_SCHAR[13] =
{" ","P","N","B","R","Q","K","p","n","b","r","q","k"};

const std::string PIECE_NAME[13] =
{   "no piece",
    "Pawn","Knight","Bishop","Rook","Queen","King",
    "pawn","knight","bishop","rook","queen","king"
};
const std::string PPIECE_NAME[13] =
{   "no piece",
    "white pawn","white knight","white bishop","white rook","white queen","white king",
    "black pawn","black knight","black bishop","black rook","black queen","black king"
};

const char PLAYER_CHAR[2] = {'w','b'};
const std::string PLAYER_NAME[2] = {"white","black"};


const std::string POS_NAME[66] =
{   "a1","b1","c1","d1","e1","f1","g1","h1",
    "a2","b2","c2","d2","e2","f2","g2","h2",
    "a3","b3","c3","d3","e3","f3","g3","h3",
    "a4","b4","c4","d4","e4","f4","g4","h4",
    "a5","b5","c5","d5","e5","f5","g5","h5",
    "a6","b6","c6","d6","e6","f6","g6","h6",
    "a7","b7","c7","d7","e7","f7","g7","h7",
    "a8","b8","c8","d8","e8","f8","g8","h8",
    "##","##"
};

const std::string COLUMN_NAME[66] =
{   "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "a","b","c","d","e","f","g","h",
    "#","#"
};

const std::string ROW_NAME[66] =
{   "1","1","1","1","1","1","1","1",
    "2","2","2","2","2","2","2","2",
    "3","3","3","3","3","3","3","3",
    "4","4","4","4","4","4","4","4",
    "5","5","5","5","5","5","5","5",
    "6","6","6","6","6","6","6","6",
    "7","7","7","7","7","7","7","7",
    "8","8","8","8","8","8","8","8",
    "#","#"
};

//######################################################

Position REFLECTION_TABLE[8*64] =
{   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
    7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24,39,38,37,36,35,34,33,32,47,46,45,44,43,42,41,40,55,54,53,52,51,50,49,48,63,62,61,60,59,58,57,56,
    56,57,58,59,60,61,62,63,48,49,50,51,52,53,54,55,40,41,42,43,44,45,46,47,32,33,34,35,36,37,38,39,24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,
    63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
    0,8,16,24,32,40,48,56,1,9,17,25,33,41,49,57,2,10,18,26,34,42,50,58,3,11,19,27,35,43,51,59,4,12,20,28,36,44,52,60,5,13,21,29,37,45,53,61,6,14,22,30,38,46,54,62,7,15,23,31,39,47,55,63,
    56,48,40,32,24,16,8,0,57,49,41,33,25,17,9,1,58,50,42,34,26,18,10,2,59,51,43,35,27,19,11,3,60,52,44,36,28,20,12,4,61,53,45,37,29,21,13,5,62,54,46,38,30,22,14,6,63,55,47,39,31,23,15,7,
    7,15,23,31,39,47,55,63,6,14,22,30,38,46,54,62,5,13,21,29,37,45,53,61,4,12,20,28,36,44,52,60,3,11,19,27,35,43,51,59,2,10,18,26,34,42,50,58,1,9,17,25,33,41,49,57,0,8,16,24,32,40,48,56,
    63,55,47,39,31,23,15,7,62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,60,52,44,36,28,20,12,4,59,51,43,35,27,19,11,3,58,50,42,34,26,18,10,2,57,49,41,33,25,17,9,1,56,48,40,32,24,16,8,0
};
