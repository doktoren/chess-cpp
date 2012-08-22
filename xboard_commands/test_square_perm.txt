enter endgame database
system rm test_square_perm.txt

set clustering_method 1
set do_preprocessing_after_sifting true
set calc_sifting true
set do_preprocessing true

system echo "In all tests below these values will apply" >> test_square_perm.txt
system echo "    clustering_method 2" >> test_square_perm.txt
system echo "    do_preprocessing_after_sifting true" >> test_square_perm.txt
system echo "    calc_sifting true" >> test_square_perm.txt
system echo "    do_preprocessing true" >> test_square_perm.txt

system echo "Testing square perm. for king and bishop in endgame KBBK" >> test_square_perm.txt

destroy database
system rm ../endgames/KRK_?tm.bdd
set square_enum_rook 8
set square_enum_king 8
system echo "square_enum_bishop 8, square_enum_king 8" >> test_square_perm.txt
load bdd KRK
system ls -l ../endgames/KRK_?tm.bdd >> test_square_perm.txt
>>test_square_perm.txt: print KRK
