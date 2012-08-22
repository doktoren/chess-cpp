enter endgame database
system rm tmp_results.txt
destroy database

set clustering_method 0

set do_preprocessing_after_sifting false
set calc_sifting false
set do_preprocessing false

system rm ../endgames/*.bdd
load bdd KRK
load bdd KQK
load bdd KPK
load bdd KBBK
load bdd KQKR
system echo "clustering_method 0, do_preprocessing false, calc_sifting false, do_preprocessing_after_sifting false" >> tmp_results.txt
system ls -l ../endgames/*.bdd >> tmp_results.txt
system echo "" >> tmp_results.txt
destroy database

set do_preprocessing true

system rm ../endgames/*.bdd
load bdd KRK
load bdd KQK
load bdd KPK
load bdd KBBK
load bdd KQKR
system echo "clustering_method 0, do_preprocessing true, calc_sifting false, do_preprocessing_after_sifting false" >> tmp_results.txt
system ls -l ../endgames/*.bdd >> tmp_results.txt
system echo "" >> tmp_results.txt
destroy database

set calc_sifting true
set do_preprocessing false

system rm ../endgames/*.bdd
load bdd KRK
load bdd KQK
load bdd KPK
load bdd KBBK
load bdd KQKR
system echo "clustering_method 0, do_preprocessing false, calc_sifting true, do_preprocessing_after_sifting false" >> tmp_results.txt
system ls -l ../endgames/*.bdd >> tmp_results.txt
system echo "" >> tmp_results.txt
destroy database

set do_preprocessing true

system rm ../endgames/*.bdd
load bdd KRK
load bdd KQK
load bdd KPK
load bdd KBBK
load bdd KQKR
system echo "clustering_method 0, do_preprocessing true, calc_sifting true, do_preprocessing_after_sifting false" >> tmp_results.txt
system ls -l ../endgames/*.bdd >> tmp_results.txt
system echo "" >> tmp_results.txt
destroy database

set do_preprocessing_after_sifting true
set calc_sifting true
set do_preprocessing true

system rm ../endgames/*.bdd
load bdd KRK
load bdd KQK
load bdd KPK
load bdd KBBK
load bdd KQKR
system echo "clustering_method 0, do_preprocessing true, calc_sifting true, do_preprocessing_after_sifting true" >> tmp_results.txt
system ls -l ../endgames/*.bdd >> tmp_results.txt
system echo "" >> tmp_results.txt
destroy database
