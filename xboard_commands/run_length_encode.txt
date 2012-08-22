enter endgame database
system rm rle_test_results.txt

system echo "##################################################" >> rle_test_results.txt
system echo "#########   KR   ################################" >> rle_test_results.txt

load table KRK
>>rle_test_results.txt: rle KRK 0 f
>>rle_test_results.txt: rle KRK 0 t
>>rle_test_results.txt: hrle KRK 0 f
>>rle_test_results.txt: hrle KRK 0 t
>>rle_test_results.txt: rle KRK 1 f
>>rle_test_results.txt: rle KRK 1 t
>>rle_test_results.txt: hrle KRK 1 f
>>rle_test_results.txt: hrle KRK 1 t
delete database

system echo "##################################################" >> rle_test_results.txt
system echo "#########   KQK   ################################" >> rle_test_results.txt

load table KQK
>>rle_test_results.txt: rle KQK 0 f
>>rle_test_results.txt: rle KQK 0 t
>>rle_test_results.txt: hrle KQK 0 f
>>rle_test_results.txt: hrle KQK 0 t
>>rle_test_results.txt: rle KQK 1 f
>>rle_test_results.txt: rle KQK 1 t
>>rle_test_results.txt: hrle KQK 1 f
>>rle_test_results.txt: hrle KQK 1 t
delete database

system echo "##################################################" >> rle_test_results.txt
system echo "##########   KPK    ##############################" >> rle_test_results.txt

load table KPK
>>rle_test_results.txt: rle KPK 0 f
>>rle_test_results.txt: rle KPK 0 t
>>rle_test_results.txt: hrle KPK 0 f
>>rle_test_results.txt: hrle KPK 0 t
>>rle_test_results.txt: rle KPK 1 f
>>rle_test_results.txt: rle KPK 1 t
>>rle_test_results.txt: hrle KPK 1 f
>>rle_test_results.txt: hrle KPK 1 t
delete database

system echo "##################################################" >> rle_test_results.txt
system echo "##########     KRKP    ###########################" >> rle_test_results.txt

load table KRKP
>>rle_test_results.txt: rle KRKP 0 t
>>rle_test_results.txt: hrle KRKP 0 t
>>rle_test_results.txt: rle KRKP 1 t
>>rle_test_results.txt: hrle KRKP 1 t
delete database

system echo "##################################################" >> rle_test_results.txt
system echo "############   KQKR   ############################" >> rle_test_results.txt

load table KQKR
>>rle_test_results.txt: rle KQKR 0 t
>>rle_test_results.txt: hrle KQKR 0 t
>>rle_test_results.txt: rle KQKR 1 t
>>rle_test_results.txt: hrle KQKR 1 t
delete database
