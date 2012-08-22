enter endgame database
system rm tmp_results.txt
destroy database

set clustering_method 2

set do_preprocessing_after_sifting true
set calc_sifting true
set do_preprocessing true

system rm ../endgames/*.bdd
load bdd KRKP
load bdd KQQK
load bdd KRRK
