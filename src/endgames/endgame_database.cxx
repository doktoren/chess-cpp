#include <set>
#include <string>
#include <algorithm>
#include <assert.h>
#include <time.h>

#include "../board_2_plus.hxx"
#include "endgame_database.hxx"
#include "endgame_indexing.hxx"
#include "endgame_en_passant.hxx"
#include "endgame_functionality.hxx"
#include "../compression/endgame_piece_enumerations.hxx"

void latex_print_king_fs_indexes(ostream &os) {
  string l[64];
  for (int white_king=0; white_king<64; white_king++) {
    os << "White king on " << POS_NAME[white_king] << '\n';
    for (int black_king=0; black_king<64; black_king++) {
      IndexRefl ir = king_full_symmetry(white_king, black_king);
      int index = ir.index;
      if (index == -1) l[black_king] = "-";
      else {
        l[black_king] = toString(index);
        string tmp = " ---";
        if (ir.refl & 1) tmp[1] = 'v';
        if (ir.refl & 2) tmp[2] = 'h';
        if (ir.refl & 4) tmp[3] = 'd';
        l[black_king] += tmp;
      }
    }
    print_latex_string_map64(os, l, 7);
  }

  for (int black_king=0; black_king<64; black_king++) {
    os << "Black king on " << POS_NAME[black_king] << '\n';
    for (int white_king=0; white_king<64; white_king++) {
      IndexRefl ir = king_full_symmetry(white_king, black_king);
      int index = ir.index;
      if (index == -1) l[white_king] = "-";
      else {
        l[white_king] = toString(index);
        string tmp = " ---";
        if (ir.refl & 1) tmp[1] = 'v';
        if (ir.refl & 2) tmp[2] = 'h';
        if (ir.refl & 4) tmp[3] = 'd';
        l[white_king] += tmp;
      }
    }
    print_latex_string_map64(os, l, 7);
  }
}

template <class INT_TYPE>
void print_map(INT_TYPE *m, int digits_per_num = 2) {
	for (int i=0; i<8*(digits_per_num+1)+3; i++) cerr << '+'; cerr << '\n';
	for (int r=7; r>=0; r--) {
		cerr << '+';
		for (int c=0; c<8; c++) {
			int tmp = m[(r<<3)|c];
			int f = 10;
			for (int i=1; i<digits_per_num; i++) {
				if (f>tmp) cerr << ' ';
				f*=10;
			}
			cerr << tmp << ' ';
		}
		cerr << "+\n";
	}
	for (int i=0; i<8*(digits_per_num+1)+3; i++) cerr << '+'; cerr << '\n';
}

bool Endgames::get_table_and_bdd_index_and_stm(const Board2 &board, triple<uint, uint, int> &indexes) {
	int hash_value = board.get_endgame_material().individual.endgame_hashing;
	if (board.get_num_pieces() <= MAX_MEN) {
		if (supported(hash_value)) {
			indexes = hash_list[hash_value]->get_table_and_bdd_index_and_stm(board);
			return true;
		} else {
			cerr << "The position is not covered by any known endgames!\n";
			return false;
		}
	} else {
		cerr << "The position has too many pieces to be considered an endgame.\n";
		return false;
	}
}

bool Endgames::construct_from_table_index(Board2 &board, string endgame_name, uint index, int player) {
	if (supported(endgame_name)) {
		return endgames[endgame_name].construct_from_table_index(board, index, player);
	} else {
		cerr << endgame_name << " is not a supported endgame.\n";
		return false;
	}
}


#define _KXK(sname, name, piece, table_size) \
		endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
				decompress_ ## name ## _table_index, \
				preprocess_ ## name ## _bdd_index, \
				name ## _table_index_to_bdd_index, \
				table_size, sname); \
				weighted_piece_sum_to_endgame_name[DB_W ## piece ## _VALUE] = \
				weighted_piece_sum_to_endgame_name[DB_B ## piece ## _VALUE] = sname

#define _KXYK(sname, name, piece1, piece2, table_size) \
		endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
				decompress_ ## name ## _table_index, \
				preprocess_ ## name ## _bdd_index, \
				name ## _table_index_to_bdd_index, \
				table_size, sname); \
				weighted_piece_sum_to_endgame_name[DB_W ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE] = \
				weighted_piece_sum_to_endgame_name[DB_B ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE] = sname
#define _KXKY(sname, name, piece1, piece2, table_size) \
		endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
				decompress_ ## name ## _table_index, \
				preprocess_ ## name ## _bdd_index, \
				name ## _table_index_to_bdd_index, \
				table_size, sname); \
				weighted_piece_sum_to_endgame_name[DB_W ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE] = \
				weighted_piece_sum_to_endgame_name[DB_B ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE] = sname

#ifdef ALLOW_5_MEN_ENDGAME
#define _KXYZK(sname, name, piece1, piece2, piece3, table_size) \
		endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
				decompress_ ## name ## _table_index, \
				preprocess_ ## name ## _bdd_index, \
				name ## _table_index_to_bdd_index, \
				table_size, sname); \
				weighted_piece_sum_to_endgame_name[DB_W ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE + DB_W ## piece3 ## _VALUE] = \
				weighted_piece_sum_to_endgame_name[DB_B ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE + DB_B ## piece3 ## _VALUE] = sname
#define _KXYKZ(sname, name, piece1, piece2, piece3, table_size) \
		endgames[sname] = EndgameFunctionality(compress_ ## name ## _table_index, \
				decompress_ ## name ## _table_index, \
				preprocess_ ## name ## _bdd_index, \
				name ## _table_index_to_bdd_index, \
				table_size, sname); \
				weighted_piece_sum_to_endgame_name[DB_W ## piece1 ## _VALUE + DB_W ## piece2 ## _VALUE + DB_B ## piece3 ## _VALUE] = \
				weighted_piece_sum_to_endgame_name[DB_B ## piece1 ## _VALUE + DB_B ## piece2 ## _VALUE + DB_W ## piece3 ## _VALUE] = sname
#endif

void Endgames::init() {
	if (initialized) return;
	initialized = true;

	cerr << "Initializing endgames\n";

	map<int, string> weighted_piece_sum_to_endgame_name;

	{ // 2-men endgame
		endgames["KK"] = EndgameFunctionality(compress_KK_table_index,
				decompress_KK_table_index,
				preprocess_KK_bdd_index,
				KK_table_index_to_bdd_index,
				462, "KK");
		weighted_piece_sum_to_endgame_name[0] = "KK";
	}

	{ // 3-men endgames
		_KXK("KNK", KXK, KNIGHT, 462*64);
		_KXK("KBK", KXK, BISHOP, 462*64);
		_KXK("KRK", KXK, ROOK  , 462*64);
		_KXK("KQK", KXK, QUEEN , 462*64);

		_KXK("KPK", KPK, PAWN  , 1806*48);
	}

	{ // 4-men endgames
		_KXYK("KNNK", KXXK, KNIGHT, KNIGHT, 462*(63*64/2));
		_KXYK("KBNK", KXYK, BISHOP, KNIGHT, 462*64*64);
		_KXYK("KBBK", KXXK, BISHOP, BISHOP, 462*(63*64/2));
		_KXYK("KRNK", KXYK, ROOK  , KNIGHT, 462*64*64);
		_KXYK("KRBK", KXYK, ROOK  , BISHOP, 462*64*64);
		_KXYK("KRRK", KXXK, ROOK  , ROOK  , 462*(63*64/2));
		_KXYK("KQNK", KXYK, QUEEN , KNIGHT, 462*64*64);
		_KXYK("KQBK", KXYK, QUEEN , BISHOP, 462*64*64);
		_KXYK("KQRK", KXYK, QUEEN , ROOK  , 462*64*64);
		_KXYK("KQQK", KXXK, QUEEN , QUEEN , 462*(63*64/2));

		_KXYK("KNPK", KXPK, KNIGHT, PAWN  , 1806*64*48);
		_KXYK("KBPK", KXPK, BISHOP, PAWN  , 1806*64*48);
		_KXYK("KRPK", KXPK, ROOK  , PAWN  , 1806*64*48);
		_KXYK("KQPK", KXPK, QUEEN , PAWN  , 1806*64*48);

		_KXYK("KPPK", KPPK, PAWN  , PAWN  , 1806*(47*48/2));



		_KXKY("KNKN", KXKY, KNIGHT, KNIGHT, 462*64*64);
		_KXKY("KBKN", KXKY, BISHOP, KNIGHT, 462*64*64);
		_KXKY("KBKB", KXKY, BISHOP, BISHOP, 462*64*64);
		_KXKY("KRKN", KXKY, ROOK  , KNIGHT, 462*64*64);
		_KXKY("KRKB", KXKY, ROOK  , BISHOP, 462*64*64);
		_KXKY("KRKR", KXKY, ROOK  , ROOK  , 462*64*64);
		_KXKY("KQKN", KXKY, QUEEN , KNIGHT, 462*64*64);
		_KXKY("KQKB", KXKY, QUEEN , BISHOP, 462*64*64);
		_KXKY("KQKR", KXKY, QUEEN , ROOK  , 462*64*64);
		_KXKY("KQKQ", KXKY, QUEEN , QUEEN , 462*64*64);

		_KXKY("KNKP", KXKP, KNIGHT, PAWN  , 1806*64*48);
		_KXKY("KBKP", KXKP, BISHOP, PAWN  , 1806*64*48);
		_KXKY("KRKP", KXKP, ROOK  , PAWN  , 1806*64*48);
		_KXKY("KQKP", KXKP, QUEEN , PAWN  , 1806*64*48);

		_KXKY("KPKP", KPKP, PAWN  , PAWN  , 1806*48*48);
	}

#ifdef ALLOW_5_MEN_ENDGAME
	{ // 5-men endgames

		{ // ENDGAMES WITH K+3 vs K

		  { // KXXXK endgames
				_KXYZK("KPPPK", KPPPK, PAWN  , PAWN  , PAWN  , 1806*(46*47*48/6));
				_KXYZK("KNNNK", KXXXK, KNIGHT, KNIGHT, KNIGHT, 462*(62*63*64/6));
				_KXYZK("KBBBK", KXXXK, BISHOP, BISHOP, BISHOP, 462*(62*63*64/6));
				_KXYZK("KRRRK", KXXXK, ROOK  , ROOK  , ROOK  , 462*(62*63*64/6));
				_KXYZK("KQQQK", KXXXK, QUEEN , QUEEN , QUEEN , 462*(62*63*64/6));
			}

			{ // KXXYK endgames
				_KXYZK("KNNPK", KXXPK, KNIGHT, KNIGHT, PAWN  , 1806*(63*64/2)*48);
				_KXYZK("KBBPK", KXXPK, BISHOP, BISHOP, PAWN  , 1806*(63*64/2)*48);
				_KXYZK("KRRPK", KXXPK, ROOK  , ROOK  , PAWN  , 1806*(63*64/2)*48);
				_KXYZK("KQQPK", KXXPK, QUEEN , QUEEN , PAWN  , 1806*(63*64/2)*48);

				_KXYZK("KBBNK", KXXYK, BISHOP, BISHOP, KNIGHT, 462*(63*64/2)*64);
				_KXYZK("KRRNK", KXXYK, ROOK  , ROOK  , KNIGHT, 462*(63*64/2)*64);
				_KXYZK("KQQNK", KXXYK, QUEEN , QUEEN , KNIGHT, 462*(63*64/2)*64);

				_KXYZK("KRRBK", KXXYK, ROOK  , ROOK  , BISHOP, 462*(63*64/2)*64);
				_KXYZK("KQQBK", KXXYK, QUEEN , QUEEN , BISHOP, 462*(63*64/2)*64);

				_KXYZK("KQQRK", KXXYK, QUEEN , QUEEN , ROOK  , 462*(63*64/2)*64);
			}

      { // KXYYK endgames
				_KXYZK("KNPPK", KXPPK, KNIGHT, PAWN  , PAWN  , 1806*(47*48/2)*64);
				_KXYZK("KBPPK", KXPPK, BISHOP, PAWN  , PAWN  , 1806*(47*48/2)*64);
				_KXYZK("KRPPK", KXPPK, ROOK  , PAWN  , PAWN  , 1806*(47*48/2)*64);
				_KXYZK("KQPPK", KXPPK, QUEEN , PAWN  , PAWN  , 1806*(47*48/2)*64);

				_KXYZK("KBNNK", KXYYK, BISHOP, KNIGHT, KNIGHT, 462*(63*64/2)*64);
				_KXYZK("KRNNK", KXYYK, ROOK  , KNIGHT, KNIGHT, 462*(63*64/2)*64);
				_KXYZK("KQNNK", KXYYK, QUEEN , KNIGHT, KNIGHT, 462*(63*64/2)*64);

				_KXYZK("KRBBK", KXYYK, ROOK  , BISHOP, BISHOP, 462*(63*64/2)*64);
				_KXYZK("KQBBK", KXYYK, QUEEN , BISHOP, BISHOP, 462*(63*64/2)*64);

				_KXYZK("KQRRK", KXYYK, QUEEN , ROOK  , ROOK  , 462*(63*64/2)*64);
			}

			{ // KXYZK endgames
				_KXYZK("KBNPK", KXYPK, BISHOP, KNIGHT, PAWN  , 1806*64*64*48);
				_KXYZK("KRNPK", KXYPK, ROOK  , KNIGHT, PAWN  , 1806*64*64*48);
				_KXYZK("KQNPK", KXYPK, QUEEN , KNIGHT, PAWN  , 1806*64*64*48);
				_KXYZK("KRBPK", KXYPK, ROOK  , BISHOP, PAWN  , 1806*64*64*48);
				_KXYZK("KQBPK", KXYPK, QUEEN , BISHOP, PAWN  , 1806*64*64*48);
				_KXYZK("KQRPK", KXYPK, QUEEN , ROOK  , PAWN  , 1806*64*64*48);

				_KXYZK("KRBNK", KXYZK, ROOK  , BISHOP, KNIGHT, 462*64*64*64);
				_KXYZK("KQBNK", KXYZK, QUEEN , BISHOP, KNIGHT, 462*64*64*64);
				_KXYZK("KQRNK", KXYZK, QUEEN , ROOK  , KNIGHT, 462*64*64*64);

				_KXYZK("KQRBK", KXYZK, QUEEN , ROOK  , BISHOP, 462*64*64*64);
			}
		}


		{ // ENDGAMES WITH K+2 vs K+1

			{ // KXXKY endgames

				// IMPORTANT!!!  THERE IS A PROBLEM WITH THE KPPKP ENDGAME:
				// Some positions are won/lost in up to 127 => out of range!
				_KXYKZ("KPPKP", KPPKP, PAWN  , PAWN  , PAWN  , 1806*(47*48/2)*48);//!!!

				_KXYKZ("KNNKP", KXXKP, KNIGHT, KNIGHT, PAWN  , 1806*(63*64/2)*48);
				_KXYKZ("KBBKP", KXXKP, BISHOP, BISHOP, PAWN  , 1806*(63*64/2)*48);
				_KXYKZ("KRRKP", KXXKP, ROOK  , ROOK  , PAWN  , 1806*(63*64/2)*48);
				_KXYKZ("KQQKP", KXXKP, QUEEN , QUEEN , PAWN  , 1806*(63*64/2)*48);

				_KXYKZ("KPPKN", KPPKX, PAWN  , PAWN  , KNIGHT, 1806*(47*48/2)*64);
				_KXYKZ("KNNKN", KXXKY, KNIGHT, KNIGHT, KNIGHT, 462*(63*64/2)*64);
				_KXYKZ("KBBKN", KXXKY, BISHOP, BISHOP, KNIGHT, 462*(63*64/2)*64);
				_KXYKZ("KRRKN", KXXKY, ROOK  , ROOK  , KNIGHT, 462*(63*64/2)*64);
				_KXYKZ("KQQKN", KXXKY, QUEEN , QUEEN , KNIGHT, 462*(63*64/2)*64);

				_KXYKZ("KPPKB", KPPKX, PAWN  , PAWN  , BISHOP, 1806*(47*48/2)*64);
				_KXYKZ("KNNKB", KXXKY, KNIGHT, KNIGHT, BISHOP, 462*(63*64/2)*64);
				_KXYKZ("KBBKB", KXXKY, BISHOP, BISHOP, BISHOP, 462*(63*64/2)*64);
				_KXYKZ("KRRKB", KXXKY, ROOK  , ROOK  , BISHOP, 462*(63*64/2)*64);
				_KXYKZ("KQQKB", KXXKY, QUEEN , QUEEN , BISHOP, 462*(63*64/2)*64);

				_KXYKZ("KPPKR", KPPKX, PAWN  , PAWN  , ROOK  , 1806*(47*48/2)*64);
				_KXYKZ("KNNKR", KXXKY, KNIGHT, KNIGHT, ROOK  , 462*(63*64/2)*64);
				_KXYKZ("KBBKR", KXXKY, BISHOP, BISHOP, ROOK  , 462*(63*64/2)*64);
				_KXYKZ("KRRKR", KXXKY, ROOK  , ROOK  , ROOK  , 462*(63*64/2)*64);
				_KXYKZ("KQQKR", KXXKY, QUEEN , QUEEN , ROOK  , 462*(63*64/2)*64);

				_KXYKZ("KPPKQ", KPPKX, PAWN  , PAWN  , QUEEN , 1806*(47*48/2)*64);
				_KXYKZ("KNNKQ", KXXKY, KNIGHT, KNIGHT, QUEEN , 462*(63*64/2)*64);
				_KXYKZ("KBBKQ", KXXKY, BISHOP, BISHOP, QUEEN , 462*(63*64/2)*64);
				_KXYKZ("KRRKQ", KXXKY, ROOK  , ROOK  , QUEEN , 462*(63*64/2)*64);
				_KXYKZ("KQQKQ", KXXKY, QUEEN , QUEEN , QUEEN , 462*(63*64/2)*64);
			}

			{ // KXYKZ endgames
				_KXYKZ("KNPKP", KXPKY, KNIGHT, PAWN  , PAWN  , 1806*48*48*64);
				_KXYKZ("KBPKP", KXPKY, BISHOP, PAWN  , PAWN  , 1806*48*48*64);
				_KXYKZ("KRPKP", KXPKY, ROOK  , PAWN  , PAWN  , 1806*48*48*64);
				_KXYKZ("KQPKP", KXPKY, QUEEN , PAWN  , PAWN  , 1806*48*48*64);

				_KXYKZ("KNPKN", KXPKY, KNIGHT, PAWN  , KNIGHT, 1806*48*64*64);
				_KXYKZ("KBPKN", KXPKY, BISHOP, PAWN  , KNIGHT, 1806*48*64*64);
				_KXYKZ("KRPKN", KXPKY, ROOK  , PAWN  , KNIGHT, 1806*48*64*64);
				_KXYKZ("KQPKN", KXPKY, QUEEN , PAWN  , KNIGHT, 1806*48*64*64);

				_KXYKZ("KNPKB", KXPKY, KNIGHT, PAWN  , BISHOP, 1806*48*64*64);
				_KXYKZ("KBPKB", KXPKY, BISHOP, PAWN  , BISHOP, 1806*48*64*64);
				_KXYKZ("KRPKB", KXPKY, ROOK  , PAWN  , BISHOP, 1806*48*64*64);
				_KXYKZ("KQPKB", KXPKY, QUEEN , PAWN  , BISHOP, 1806*48*64*64);

				_KXYKZ("KNPKR", KXPKY, KNIGHT, PAWN  , ROOK  , 1806*48*64*64);
				_KXYKZ("KBPKR", KXPKY, BISHOP, PAWN  , ROOK  , 1806*48*64*64);
				_KXYKZ("KRPKR", KXPKY, ROOK  , PAWN  , ROOK  , 1806*48*64*64);
				_KXYKZ("KQPKR", KXPKY, QUEEN , PAWN  , ROOK  , 1806*48*64*64);

				_KXYKZ("KNPKQ", KXPKY, KNIGHT, PAWN  , QUEEN , 1806*48*64*64);
				_KXYKZ("KBPKQ", KXPKY, BISHOP, PAWN  , QUEEN , 1806*48*64*64);
				_KXYKZ("KRPKQ", KXPKY, ROOK  , PAWN  , QUEEN , 1806*48*64*64);
				_KXYKZ("KQPKQ", KXPKY, QUEEN , PAWN  , QUEEN , 1806*48*64*64);//124>=x>=-123 (close!)



				_KXYKZ("KBNKP", KXPKY, BISHOP, KNIGHT, PAWN  , 1806*64*48*64);
				_KXYKZ("KRNKP", KXPKY, ROOK  , KNIGHT, PAWN  , 1806*64*48*64);
				_KXYKZ("KQNKP", KXPKY, QUEEN , KNIGHT, PAWN  , 1806*64*48*64);

				_KXYKZ("KBNKN", KXYKZ, BISHOP, KNIGHT, KNIGHT, 462*64*64*64);
				_KXYKZ("KRNKN", KXYKZ, ROOK  , KNIGHT, KNIGHT, 462*64*64*64);
				_KXYKZ("KQNKN", KXYKZ, QUEEN , KNIGHT, KNIGHT, 462*64*64*64);

				_KXYKZ("KBNKB", KXYKZ, BISHOP, KNIGHT, BISHOP, 462*64*64*64);
				_KXYKZ("KRNKB", KXYKZ, ROOK  , KNIGHT, BISHOP, 462*64*64*64);
				_KXYKZ("KQNKB", KXYKZ, QUEEN , KNIGHT, BISHOP, 462*64*64*64);

				_KXYKZ("KBNKR", KXYKZ, BISHOP, KNIGHT, ROOK  , 462*64*64*64);
				_KXYKZ("KRNKR", KXYKZ, ROOK  , KNIGHT, ROOK  , 462*64*64*64);
				_KXYKZ("KQNKR", KXYKZ, QUEEN , KNIGHT, ROOK  , 462*64*64*64);

				_KXYKZ("KBNKQ", KXYKZ, BISHOP, KNIGHT, QUEEN , 462*64*64*64);
				_KXYKZ("KRNKQ", KXYKZ, ROOK  , KNIGHT, QUEEN , 462*64*64*64);
				_KXYKZ("KQNKQ", KXYKZ, QUEEN , KNIGHT, QUEEN , 462*64*64*64);



				_KXYKZ("KRBKP", KXYKP, ROOK  , BISHOP, PAWN  , 1806*64*48*64);
				_KXYKZ("KQBKP", KXYKP, QUEEN , BISHOP, PAWN  , 1806*64*48*64);

				_KXYKZ("KRBKN", KXYKZ, ROOK  , BISHOP, KNIGHT, 462*64*64*64);
				_KXYKZ("KQBKN", KXYKZ, QUEEN , BISHOP, KNIGHT, 462*64*64*64);

				_KXYKZ("KRBKB", KXYKZ, ROOK  , BISHOP, BISHOP, 462*64*64*64);
				_KXYKZ("KQBKB", KXYKZ, QUEEN , BISHOP, BISHOP, 462*64*64*64);

				_KXYKZ("KRBKR", KXYKZ, ROOK  , BISHOP, ROOK  , 462*64*64*64);
				_KXYKZ("KQBKR", KXYKZ, QUEEN , BISHOP, ROOK  , 462*64*64*64);

				_KXYKZ("KRBKQ", KXYKZ, ROOK  , BISHOP, QUEEN , 462*64*64*64);
				_KXYKZ("KQBKQ", KXYKZ, QUEEN , BISHOP, QUEEN , 462*64*64*64);



				_KXYKZ("KQRKP", KXYKP, QUEEN , ROOK  , PAWN  , 1806*64*48*64);

				_KXYKZ("KQRKN", KXYKZ, QUEEN , ROOK  , KNIGHT, 462*64*64*64);

				_KXYKZ("KQRKB", KXYKZ, QUEEN , ROOK  , BISHOP, 462*64*64*64);

				_KXYKZ("KQRKR", KXYKZ, QUEEN , ROOK  , ROOK  , 462*64*64*64);

				_KXYKZ("KQRKQ", KXYKZ, QUEEN , ROOK  , QUEEN , 462*64*64*64);
			}
		}

	}
#endif

	{ // Initialize hash_list
		for (int i=0; i<DB_ARRAY_LENGTH; i++)
			hash_list[i] = 0;
		typedef map<int, string>::const_iterator CI;
		for (CI i = weighted_piece_sum_to_endgame_name.begin(); i != weighted_piece_sum_to_endgame_name.end(); i++) {
			assert(!hash_list[i->first]);
			hash_list[i->first] = &(endgames[i->second]);
		}
	}

	init_cluster_functions();
}

void Endgames::destroy_tables() {
	typedef map<string, EndgameFunctionality>::iterator I;
	for (I i=endgames.begin(); i!=endgames.end(); i++)
		i->second.destroy_table();
}

void Endgames::destroy_bdd() {
	typedef map<string, EndgameFunctionality>::iterator I;
	for (I i=endgames.begin(); i!=endgames.end(); i++)
		i->second.destroy_bdd();
}

vector<int> Endgames::get_name_matches(string name_pattern) {
	vector<int> result;
	typedef map<string, EndgameFunctionality>::iterator I;
	for (I i=endgames.begin(); i!=endgames.end(); i++)
		if (i->second.name_match(name_pattern))
			result.push_back(i->second.calc_hash_value());
	if (result.size() == 0)
		cerr << "No endgame matches " << name_pattern << "\n";
	return result;
}

void Endgames::build_tables(string name_pattern) {
	cerr << "Deleting all endgames (maximizing free memory)\n";
	destroy();
	vector<int> m = get_name_matches(name_pattern);
	for (uint i=0; i<m.size(); i++) {
		hash_list[m[i]]->load_table(false, true);
		hash_list[m[i]]->print(cerr);
		hash_list[m[i]]->destroy();
	}
}
void Endgames::build_bdds(string name_pattern) {
	cerr << "Deleting all endgames (maximizing free memory)\n";
	destroy();
	vector<int> m = get_name_matches(name_pattern);
	for (uint i=0; i<m.size(); i++) {
		hash_list[m[i]]->load_bdd(false, true);
		hash_list[m[i]]->print(cerr);
		hash_list[m[i]]->destroy();
	}
}

void Endgames::load_tables(string name_pattern,
		bool restrict_to_stm,
		bool build_if_nescessary,
		int restricted_stm)
{ 
	vector<int> m = get_name_matches(name_pattern);
	for (uint i=0; i<m.size(); i++) {
		hash_list[m[i]]->load_table(restrict_to_stm, build_if_nescessary,
				restricted_stm);
		hash_list[m[i]]->print(cerr);
	}
}
void Endgames::load_bdds(string name_pattern,
		bool restrict_to_stm,
		bool build_from_tables_if_nescessary,
		bool build_tables_if_nescessary,
		int restricted_stm)
{
	vector<int> m = get_name_matches(name_pattern);
	for (uint i=0; i<m.size(); i++) {
		hash_list[m[i]]->load_bdd(restrict_to_stm, build_from_tables_if_nescessary,
				build_tables_if_nescessary, restricted_stm);
	}
}

void Endgames::print(ostream &os, string name_pattern) {
	os << "Endgames supported:\n";
	vector<int> m = get_name_matches(name_pattern);
	for (uint i=0; i<m.size(); i++)
		hash_list[m[i]]->print(os);
}

void Endgames::print_bdds(ostream &os, string name_pattern, bool print_bdds) {
	vector<int> m = get_name_matches(name_pattern);
	for (uint i=0; i<m.size(); i++)
		hash_list[m[i]]->print_bdd(os, print_bdds);
}

// inspect(cerr, "K##_Qe2_Kf7_Rf6") prints map
void Endgames::inspect(ostream &os, string s) {
	bool ok = true;
	int num_pieces = (s.size()+1)/4;
	ok &= 4*num_pieces-1 == (int)s.size();
	bool unknown_found = false;
	if (ok) {
		for (int i=0; i<num_pieces; i++) {
			if (s[4*i+1] == '#') {
				if (unknown_found  ||  s[4*i+2] != '#') {
					ok = false;
					break;
				}
				unknown_found = true;
			} else {
				if (s[4*i+1]<'a'  ||  'h'<s[4*i+1]  ||  s[4*i+2]<'1'  ||  '8'<s[4*i+2]) {
					ok = false;
					break;
				}
			}
		}
		if (!unknown_found) {
			os << "Endgames::inspect(..., " << s << "):\n"
					<< "No unknown position? All combinations tried!\n";
		}
	}
	if (ok) {
		string name(num_pieces, ' ');
		for (int i=0; i<num_pieces; i++)
			name[i] = s[4*i] & ~32;// Convert to lower case

		if (supported(name)) {
			vector<Position> positions(num_pieces);
			for (int i=0; i<num_pieces; i++)
				if (s[4*i+1] == '#') {
					positions[i] = ILLEGAL_POS;
				} else {
					positions[i] = CR_TO_POS[s[4*i+1]-'a'][s[4*i+2]-'1'];
				}

			if (unknown_found) {
				endgames[name].inspect(os, positions);
			} else {
				for (int i=0; i<num_pieces; i++) {
					int tmp = positions[i];
					positions[i] = ILLEGAL_POS;
					endgames[name].inspect(os, positions);
					positions[i] = tmp;
				}
			}

		} else {
			os << "Endgames::inspect(..., " << s << "):\n"
					<< "The piece combination " << name << " was not found.\n";
		}

	} else {
		os << "Endgames::inspect(..., " << s << "): wrong format!\n"
				<< "Examples of use: inspect(cerr, K## Ra2 Kg1), inspect(cerr, k## ra2 kg1)\n";
	}
}

bool clr_endgame_database(Board *board, ostream& os, vector<string> &p) {
	Board *_b = reinterpret_cast<Board *>(board);
	Board2 &b = *dynamic_cast<Board2plus *>(_b);

	if (dot_demand(p, 1, "help")) {
		os << "Endgame database, help:\n"
				<< "    print matches pattern  or  pm pattern\n"
				<< "      - A limited version of pattern matching.\n"
				<< "      - Examples \"pm K*K*\", \"pm K2K*\", \"pm KXKB\", not \"K1NK\"\n"
				<< "    load table pattern [r] [b] or  lt pattern [r] [b]\n"
				<< "      - b=t/f : build_if_nescessary, default is false.\n"
				<< "      - r=t/f : restrict_to_wtm, default is false.\n"
				<< "    build table pattern  or  bt pattern\n"
				<< "    load bdd pattern [b1] [b2] [r] or  lb pattern [b1] [b2] [r]\n"
				<< "      - b1=t/f : build_from_tables_if_nescessary, default is false.\n"
				<< "      - b2=t/f : build_tables_if_nescessary, default is false.\n"
				<< "      - r=t/f : restrict_to_wtm, default is false.\n"
				<< "    build bdd pattern  or  bb pattern\n"
				<< "    print bdd pattern  or  pb pattern\n"
				<< "      - (load) and print endgame bdd's or just x.\n"
				<< "    delete database  or  dd\n"
				<< "      - remove endgame tables from memory\n"
				<< "    inspect [p1 p2] [p3] [p4] [p5]\n"
				<< "      - Example: \"inspect K## Ra2 Kg1\"\n"
				<< "    verify table pattern  or  vt pattern\n"
				<< "      - Print number of positions with draw, mate in n, etc.\n"
				<< "    verify bdd pattern  or  vb pattern\n"
				<< "      - Verifies the bdd against the table (both must be loaded).\n"
				<< "    print settings  or  ps\n"
				<< "    set name value\n"
				<< "      - example \"set do_preprocessing false\"\n"
				<< "    print pattern or  p pattern\n"
				<< "    index database  or  id\n"
				<< "      - Find value of current position in endgame table.\n"
				<< "    show table index  or  sti\n"
				<< "      - Shows endgame table/bdd index of position and stm.\n"
				<< "    construct from table index name index stm  or  cfti name index stm\n"
				<< "      - Sets board according to name, index and stm.\n"
				<< "      - name is KK,KRK,...,KRKP,... index is 0..max, stm is w,b,0 or 1\n"
				<< "      - Example: \"cfti KRK 2132 w\"\n"
				<< "    examine unreachable positions name stm depth  or  eup name stm depth\n"
				<< "      - For each position in endgame \"name\" with stm (0 or 1), try to\n"
				<< "      - undo depth moves. If depth=0 use static unreachability test.\n"
				<< "      - if positions spelled w. cap. P,each unr.p. wr. to big_output.txt\n"
				<< "    rle pattern stm map_dont_cares\n"
				<< "      - Examples \"rle KQK 0 t\", \"rle KBBK 1 t\"\n"
				<< "    hrle pattern stm map_dont_cares\n"
				<< "      - huffman run length encode, archieves better performance.\n";


	} else if (dot_demand(p, 3, "hej", "hej", (uintptr_t)0)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		for (uint i=0; i<m.size(); i++)
			endgames[m[i]].reduce_information();

#ifdef ALLOW_5_MEN_ENDGAME
	} else if (dot_demand(p, 5, "a", "b", "c", (uintptr_t)1, (uintptr_t)0)) {
		cerr << "Doing secret stuff with KRRRK...\n";
		EndgameFunctionality &ef = endgames["KRRRK"];
		ef.load_bdd();
		int p = parse_result[0][0] == 'b'  ||  parse_result[0][0] == '1';
		int n = atoi(parse_result[1].c_str());
		for (int i=0; i<n; i++) {
			BDD_Index b;
			int r = rand();
			b[0] = r & 0x3F;
			b[1] = (r>>=6) & 0x3F;
			b[2] = (r>>=6) & 0x3F;
			b[3] = (r>>=6) & 0x3F;
			b[4] = (r>>=6) & 0xF;
			ef.direct_bdd_index(p, b);
		}
		cerr << "done\n";

	} else if (dot_demand(p, 5, "a", "a", "a", (uintptr_t)1, (uintptr_t)0)) {
		cerr << "Doing secret stuff with KNNNK...\n";
		EndgameFunctionality &ef = endgames["KNNNK"];
		ef.load_bdd();
		int p = parse_result[0][0] == 'b'  ||  parse_result[0][0] == '1';
		int n = atoi(parse_result[1].c_str());
		for (int i=0; i<n; i++) {
			BDD_Index b;
			int r = rand();
			b[0] = r & 0x3F;
			b[1] = (r>>=6) & 0x3F;
			b[2] = (r>>=6) & 0x3F;
			b[3] = (r>>=6) & 0x3F;
			b[4] = (r>>=6) & 0xF;
			ef.direct_bdd_index(p, b);
		}
		cerr << "done\n";

	} else if (dot_demand(p, 5, "c", "b", "a", 1, (uintptr_t)0)) {
		cerr << "Doing secret stuff with KQKR...\n";
		EndgameFunctionality &ef = endgames["KQKR"];
		ef.load_bdd();
		int p = parse_result[0][0] == 'b'  ||  parse_result[0][0] == '1';
		int n = atoi(parse_result[1].c_str());
		for (int i=0; i<n; i++) {
			BDD_Index b;
			int r = rand();
			b[0] = r & 0x3F;
			b[1] = (r>>=6) & 0x3F;
			b[2] = (r>>=6) & 0x3F;
			b[3] = (r>>=6) & 0xF;
			ef.direct_bdd_index(p, b);
		}
		cerr << "done\n";
#endif

	} else if (dot_demand(p, 3, "print", "matches", (uintptr_t)0)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		if (m.size()) {
			cerr << "Pattern " << parse_result[0] << " matches the following endgames:\n";
			for (uint i=0; i<m.size(); i++) {
				if (i) cerr << ", ";
				cerr << endgames[m[i]].get_name();
			}
			cerr << "\n";
		}

	} else if (dot_demand(p, 4, "latex", "print", "square", "permutations")) {
		for (int i=0; i<10; i++)
			print_latex_signed_map64(cerr, mappings[i], 2);

	} else if (dot_demand(p, 4, "rle", (uintptr_t)0, (uintptr_t)1, (uintptr_t)1)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		if (m.size()) {
			cerr << "Only win/draw/loss information in the run length encoded version.\n";
			int stm = (parse_result[1][0]=='b')  ||  (parse_result[1][0]=='1');
			bool map_dont_cares = parse_result[2][0]=='t';
			for (uint i=0; i<m.size(); i++)
				endgames[m[i]].run_length_encode(stm, 1, map_dont_cares);
		}

	} else if (dot_demand(p, 4, "hrle", (uintptr_t)0, (uintptr_t)1, (uintptr_t)1)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		if (m.size()) {
			cerr << "Only win/draw/loss information in the huffman run length encoded version.\n";
			int stm = (parse_result[1][0]=='b')  ||  (parse_result[1][0]=='1');
			bool map_dont_cares = parse_result[2][0]=='t';
			for (uint i=0; i<m.size(); i++)
				endgames[m[i]].run_length_encode(stm, 2, map_dont_cares);
		}

	} else if (dot_demand(p, 6, "examine", "unreachable", "positions", (uintptr_t)0, (uintptr_t)1, (uintptr_t)0)  ||
			dot_demand(p, 6, "examine", "unreachable", "Positions", (uintptr_t)0, (uintptr_t)1, (uintptr_t)0)) {
		int write_pos = dot_demand(p, 6, "examine", "unreachable", "Positions", (uintptr_t)0, (uintptr_t)1, (uintptr_t)0) ? 1 : 0;
		if (endgames.supported(parse_result[0])) {
			int stm = atoi(parse_result[1].c_str());
			int test_depth = atoi(parse_result[2].c_str());
			if ((stm==0 || stm==1)  &&  (0<=test_depth && test_depth<10)) {
				BitList bl;
				endgames[parse_result[0]].find_unreachable_positions(bl, stm, test_depth, write_pos+2+4);
			} else {
				cerr << "Stm or depth not in legal interval ([0..1] and [0..9])\n";
			}
		} else {
			os << "Unknown endgame table " << parse_result[0] << "\n";
		}

	} else if (dot_demand(p, 3, "build", "table", (uintptr_t)0)) {
		os << "Building endgame tables.\n";
		endgames.build_tables(parse_result[0]);

		// BEGIN load table
	} else if (dot_demand(p, 3, "load", "table", (uintptr_t)0)) {
		os << "Loading endgame tables.\n";
		endgames.load_tables(parse_result[0], false, false);
	} else if (dot_demand(p, 4, "load", "table", (uintptr_t)0, (uintptr_t)1)) {
		os << "Loading endgame tables.\n";
		bool buid_if_nescessary = parse_result[1][0] == 't';
		endgames.load_tables(parse_result[0], false, buid_if_nescessary);
	} else if (dot_demand(p, 5, "load", "table", (uintptr_t)0, (uintptr_t)1, (uintptr_t)1)) {
		os << "Loading endgame tables.\n";
		bool buid_if_nescessary = parse_result[1][0] == 't';
		bool restrict_to_wtm = parse_result[2][0] == 't';
		endgames.load_tables(parse_result[0], restrict_to_wtm, buid_if_nescessary);
		// END load tables

	} else if (dot_demand(p, 3, "build", "bdd", (uintptr_t)0)) {
		os << "Building endgame bdds.\n";
		endgames.build_bdds(parse_result[0]);

		// BEGIN load bdd
	} else if (dot_demand(p, 3, "load", "bdd", (uintptr_t)0)) {
		os << "Loading endgame bdds.\n";
		endgames.load_bdds(parse_result[0], false, false, false);
	} else if (dot_demand(p, 4, "load", "bdd", (uintptr_t)0, (uintptr_t)1)) {
		os << "Loading endgame bdds.\n";
		bool buid_from_tables_if_nescessary = parse_result[1][0] == 't';
		endgames.load_bdds(parse_result[0], false, buid_from_tables_if_nescessary, false);
	} else if (dot_demand(p, 5, "load", "bdd", (uintptr_t)0, (uintptr_t)1, (uintptr_t)1)) {
		os << "Loading endgame bdds.\n";
		bool buid_from_tables_if_nescessary = parse_result[1][0] == 't';
		bool buid_tables_if_nescessary = parse_result[2][0] == 't';
		endgames.load_bdds(parse_result[0], false,
				buid_from_tables_if_nescessary, buid_tables_if_nescessary);
	} else if (dot_demand(p, 6, "load", "bdd", (uintptr_t)0, (uintptr_t)1, (uintptr_t)1, (uintptr_t)1)) {
		os << "Loading endgame bdds.\n";
		bool buid_from_tables_if_nescessary = parse_result[1][0] == 't';
		bool buid_tables_if_nescessary = parse_result[2][0] == 't';
		bool restrict_to_wtm = parse_result[3][0] == 't';
		endgames.load_bdds(parse_result[0], restrict_to_wtm,
				buid_from_tables_if_nescessary, buid_tables_if_nescessary);
		// END load bdd

		// BEGIN verify stuff
	} else if (dot_demand(p, 3, "verify", "table", (uintptr_t)0)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		for (uint i=0; i<m.size(); i++) {
			endgames[m[i]].load_table();
			endgames[m[i]].print_table_verifier(os);
		}

	} else if (dot_demand(p, 3, "verify", "bdd", (uintptr_t)0)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		for (uint i=0; i<m.size(); i++) {
			endgames[m[i]].load_table();
			endgames[m[i]].load_bdd();
			endgames[m[i]].compare_bdd_with_table(WHITE);
			if (!endgames[m[i]].is_symmetric())
				endgames[m[i]].compare_bdd_with_table(BLACK);
		}
		// END verify stuff


	} else if (dot_demand(p, 3, "print", "bdd", (uintptr_t)0)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		for (uint i=0; i<m.size(); i++)
			if (endgames[m[i]].load_bdd())
				endgames[m[i]].print_bdd(os, true);

	} else if (dot_demand(p, 2, "delete", "database")) {
		os << "clearing endgame database from mem...\n";
		os.flush();
		endgames.destroy_tables();
		endgames.destroy_bdd();
		os << "done\n";

	} else if (dot_demand(p, 2, "inspect", (uintptr_t)0)) {
		endgames.inspect(os, parse_result[0]);

	} else if (dot_demand(p, 3, "inspect", (uintptr_t)3, (uintptr_t)3)) {
		string s = parse_result[0]+"_"+parse_result[1];
		endgames.inspect(os, s);

	} else if (dot_demand(p, 4, "inspect", (uintptr_t)3, (uintptr_t)3, (uintptr_t)3)) {
		string s = parse_result[0]+"_"+parse_result[1]+"_"+parse_result[2];
		endgames.inspect(os, s);

	} else if (dot_demand(p, 5, "inspect", (uintptr_t)3, (uintptr_t)3, (uintptr_t)3, (uintptr_t)3)) {
		string s = parse_result[0]+"_"+parse_result[1]+"_"+parse_result[2]+"_"+parse_result[3];
		endgames.inspect(os, s);

	} else if (dot_demand(p, 6, "inspect", (uintptr_t)3, (uintptr_t)3, (uintptr_t)3, (uintptr_t)3, (uintptr_t)3)) {
		string s = parse_result[0]+"_"+parse_result[1]+"_"+parse_result[2]+"_"+
				parse_result[3]+"_"+parse_result[4];
		endgames.inspect(os, s);

	} else if (dot_demand(p, 3, "set", (uintptr_t)0, (uintptr_t)0)) {
		// Todo: what if illegal name?
		endgame_settings->define(parse_result[0], parse_result[1]);
		os << "setting " << parse_result[0] << " set to " << parse_result[1] << "\n";

	} else if (dot_demand(p, 2, "print", "settings")) {
		endgame_settings->print(os);

	} else if (dot_demand(p, 1, "print")) {
		endgames.print(os);

	} else if (dot_demand(p, 2, "print", (uintptr_t)0)) {
		vector<int> m = endgames.get_name_matches(parse_result[0]);
		for (uint i=0; i<m.size(); i++)
			endgames[m[i]].print(os);

	} else if (dot_demand(p, 3, "print", "latex", "kingfs")) {
		latex_print_king_fs_indexes(os);

	} else if (dot_demand(p, 2, "index", "database")) {
		int value;
		if (endgame_table_index(b, value)) {
			os << "Found position in endgame table. value = "
					<< game_theoretical_value_to_string(value) << '\n';
		} else {
			os << "Did not find position in endgame table.\n";
		}
		int status = b.calc_game_status();
		if (status == GAME_OPEN) {
			Move move = b.moves();
			while (b.next_move(move)) {
				os << b.moveToSAN(move) << ": \t";
				Undo undo = b.execute_move(move);
				if (endgame_table_index(b, value)) {
					if (value) {
						// Adjust value to this player
						value = -value;
						if (value >= GUARANTEED_WIN) value--;
						if (value <= GUARANTEED_LOSS) value++;
					}
					os << game_theoretical_value_to_string(value);
				}
				else os << "?";
				os << "\n";
				b.undo_move(move, undo);
			}
		}

	} else if (dot_demand(p, 3, "show", "table", "index")) {
		triple<uint, uint, int> i;


		int hash_value = b.get_endgame_material().individual.endgame_hashing;
		if (b.get_num_pieces() <= MAX_MEN  &&  endgames.supported(hash_value)) {
			triple<uint, uint, int> i = endgames[hash_value].get_table_and_bdd_index_and_stm(b);

			os << "Endgame " << endgames.get_endgame_name(b) << ":\n"
					<< "(table index, bdd index, stm) = ("
					<< i.first << ", " << i.second << " ("
					<< toString(i.second, endgames[b.get_endgame_material().individual.endgame_hashing].calc_log_bdd_size(), 2)
					<< "b), " << i.third << ")\n";

			pair<int, int> p = endgames[hash_value].getModifiedOBDDIndexAndClusterValue(b);
			if (p.first != -1)
				cerr << "\t(Mod. bdd index, cluster) = (" << p.first << "," << p.second << ")\n";
		}

	} else if (dot_demand(p, 7, "construct", "from", "table", "index", (uintptr_t)0, (uintptr_t)0, (uintptr_t)1)) {
		string endgame_name = parse_result[0];
		uint table_index = atoi(parse_result[1].c_str());
		char stm = parse_result[2][0] | 32;
		int player;
		if (stm == 'w' || stm == '0') player = 0;
		else if (stm == 'b' || stm == '1') player = 1;
		else {
			os << "Side-to-move must be w or 0, or b or 1\n";
			return false;
		}

		if (endgames.construct_from_table_index(b, endgame_name, table_index, player))
			b.print_board(os);

	} else return false;
	return true;

}


string Endgames::get_endgame_name(const Board2 &board) {
	int hash_value = board.get_endgame_material().individual.endgame_hashing;
	if (board.get_num_pieces() <= MAX_MEN  &&  supported(hash_value)) {
		return hash_list[hash_value]->get_name();
	} else {
		return "";
	}
}



EndgameSettings *endgame_settings;
Endgames endgames;
