
*** ediff는 sparc에 있던 풀그림

1. tagger를 작동시키면 (o , x) marker가 붙은 결과를 받는다.
   여기서 (o , x)를 없애고 기존에 수동 태깅된 것과 비교.

   diff 수동_태깅 태거_태깅 | ediff > dif.dif
   awk ' { print $1 } ' dif.dif > dif.dif2
   calcdiff dif.dif2
   형태소 level과 어절 level에서의 틀린 갯수 count  

2. 수동 태깅된 text에서 tagger가 태깅했을 때 known-word에 
   해당하는 어절들을 구하려면 ?

   1) ../splitdiff diffe_file k_f u_f e_f  
      이렇게 하면 u_f에 unknown_word(수동,태거)의 부분이 추출
      
   2) ../getdiff u_f tar_u_f
      이렇게 하면 unknown_word(수동)만 추출
      awk ' { print $2 } ' tar_u_f > tar_u_f2
      awk -F+ ' { SS = SS + NF } END { print SS } ' tar_u_f2
      이렇게 하면 unknown_word(수동)의 전체 형태소 갯수가 count

3. 이제 known-word에 해당하는 어절을 구하기 위해서는 ?

   1) 위에서 구한 unknown_word어절과 수동 태깅된 text와 비교.
      diff ../UnknownTest/un_test_drg.tagged.2 tar_u_f | ediff > kno_dif
      이렇게 하면 "차이"가 나는 부분만 kno_dif에 저장.

   2) grep "^o " kno_dif > known_part : known_part에 전부 저장됨.
      awk ' { print $2 } ' known_part > know_p 
 
   3) awk -F+ ' { SS = SS + NF } END { print SS } ' know_p
      known_word의 형태소 갯수 count 

4. known_word부분의 hit ratio를 구하기 위해
  
   1) kn_f를 vi를 이용해 /^o /를 없애고 awk로 첫번째 field것만 모으고,
      calcdiff를 한다.

5. Unknown_word부분의 hit ratio를 구하기 위해

   1) u_f를 vi를 이용해 /^o / , /^x / 를 없애고  calcdiff를 한다.





