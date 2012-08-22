CFLAGS  = -O3 -Wall -pedantic -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors -DNDEBUG -ffast-math -fomit-frame-pointer
CFLAGS_DB = -ggdb3 -pg -Wall -pedantic -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors
CFLAGS_XB = -O3 -Wall -pedantic -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors -DNDEBUG -DXBOARD -ffast-math -fomit-frame-pointer
CFLAGS_DB_XB = -ggdb3 -pg -Wall -pedantic -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors -DXBOARD

CFLAGS_NALIMOV = -O3 -DNDEBUG -Wall -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors
CFLAGS_NALIMOV_DB = -Wall -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors

#TEST = -ggdb3 -O3 -Wall -pedantic -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors -ffast-math -fno-strict-aliasing

TEST = -ggdb3 -O3 -pg -Wall -pedantic -ansi -march=pentium4 -Woverloaded-virtual -felide-constructors

MODS = board endgame_table_bdd mapping_of_wildcards mapping_of_wildcards2 \
endgame_run_length_encoding run_length_encoding/bit_stream \
endgame_clustering_functions endgame_Nalimov board_2 endgame_castling \
endgame_piece_enumerations endgame_square_permutations \
binary_decision_diagram endgame_database \
 \
chess \
cpu_communication_module xboard_listener streams help_functions settings \
move_and_undo board_tables board_move_tables \
board_2_plus static_exchange_evaluation \
board_3 \
 \
cpu_evaluation_1 cpu_evaluation_2 cpu_evaluation_2_const cpu_evaluation_3 \
cpu_search_1 cpu_search_2 cpu_search_3 \
cpu_engines cpu_search engine \
 \
file_loader parser test_suite \
hash_value hash_table transposition_table_content opening_library \
game_phase piece_values unsigned_long_long \
 \
my_vector clustering_algorithm bdd_compression

all: chess

chess: ${MODS:=.o}
	g++ -g -ggdb3 ${MODS:=.o} -o $@

endgame_Nalimov.o:
	$(CXX) -c $(CFLAGS_NALIMOV) $< -o $@

%.o: %.cxx
	$(CXX) -c $(CFLAGS) $< -o $@

clean:
	rm -f test *~ *.d *.o .\#* \#* a.out chess core tmp.* .nfs* incoming.txt big_output.txt probe_Nalimov/*~ probe_Nalimov/test help_programs/*~ help_programs/test help_programs/a.out

%.d: %.cxx
	g++ -MM $< -o $@

dep: ${MODS:=.d}

include ${MODS:=.d}


#gprof chess -b --ignore-non-functions >profiling.txt
