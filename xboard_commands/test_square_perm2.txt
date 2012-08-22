enter endgame database
system rm test_square_perm2.txt

set clustering_method 1
set do_preprocessing_after_sifting true
set calc_sifting true
set do_preprocessing true

system echo "In all tests below these values will apply" >> test_square_perm2.txt
system echo "    clustering_method 2" >> test_square_perm2.txt
system echo "    do_preprocessing_after_sifting true" >> test_square_perm2.txt
system echo "    calc_sifting true" >> test_square_perm2.txt
system echo "    do_preprocessing true" >> test_square_perm2.txt

system echo "Testing square perm. for king and knight in endgame KBNK" >> test_square_perm2.txt
KRKP_btm.bdd
destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_black_pawn 0
set square_enum_king 3
system echo "square_enum_knight 0, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 1
set square_enum_king 3
system echo "square_enum_knight 1, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 2
set square_enum_king 3
system echo "square_enum_knight 2, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 3
set square_enum_king 3
system echo "square_enum_knight 3, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 4
set square_enum_king 3
system echo "square_enum_knight 4, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 5
set square_enum_king 3
system echo "square_enum_knight 5, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 6
set square_enum_king 3
system echo "square_enum_knight 6, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 7
set square_enum_king 3
system echo "square_enum_knight 7, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 8
set square_enum_king 3
system echo "square_enum_knight 8, square_enum_king 3" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt



destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 0
set square_enum_king 4
system echo "square_enum_knight 0, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 1
set square_enum_king 4
system echo "square_enum_knight 1, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 2
set square_enum_king 4
system echo "square_enum_knight 2, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 3
set square_enum_king 4
system echo "square_enum_knight 3, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 4
set square_enum_king 4
system echo "square_enum_knight 4, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 5
set square_enum_king 4
system echo "square_enum_knight 5, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 6
set square_enum_king 4
system echo "square_enum_knight 6, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 7
set square_enum_king 4
system echo "square_enum_knight 7, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 8
set square_enum_king 4
system echo "square_enum_knight 8, square_enum_king 4" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt



destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 0
set square_enum_king 8
system echo "square_enum_knight 0, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 1
set square_enum_king 8
system echo "square_enum_knight 1, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 2
set square_enum_king 8
system echo "square_enum_knight 2, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 3
set square_enum_king 8
system echo "square_enum_knight 3, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 4
set square_enum_king 8
system echo "square_enum_knight 4, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 5
set square_enum_king 8
system echo "square_enum_knight 5, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 6
set square_enum_king 8
system echo "square_enum_knight 6, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 7
set square_enum_king 8
system echo "square_enum_knight 7, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt

destroy database
system rm ../endgames/KRKP_?tm.bdd
set square_enum_knight 8
set square_enum_king 8
system echo "square_enum_knight 8, square_enum_king 8" >> test_square_perm2.txt
load bdd KRKP
system ls -l ../endgames/KRKP_?tm.bdd >> test_square_perm2.txt
