
=======================================================

   K T S : Korean Tagging System       ver 0.9


   Sangho Lee      
   jhlee@csone.kaist.ac.kr    shlee2@adam.kaist.ac.kr

   Computer Systems Lab.
   KAIST.

=======================================================

KTS는 [이 상호, `미등록어를 고려한 한국어 품사 태깅 시스템 구현',
          한국과학기술원, 석사학위 논문, 1995]에서 구현된
시스템을 기초로 하였습니다. 

본 KTS는 한국어 품사 태깅 시스템으로 다음과 같은 
디렉토리로 구성되어 있습니다.


  ./       :

       KTS의 C-code, 사전에 관련된 Utility를 포함한다.

  ./manual :

       KTS의 사용 방법,  main.ps를 읽어보기 바랍니다.

  ./Tool/  :

       KTS가 필요로 하고 있는 사전, 확률 값들을 
       추출하기 위해 사용되는 Utility.
       이미 태깅된 화일로부터 정보를 추출한다.
             
       TRANTBL    : 형태소의 품사 접속표
       TRANFREQ   : 품사의 전이 확률
       total.dict : 사전

  ./Tool/Irregular/ :

       태깅된 화일로부터 사전을 구축할 때, 불규칙
       코드를 넣기 위해 수동으로 작성된 불규칙 사전. 
       (한국어 불규칙 용언이 모두 수록되어 있는 것은 아니다.)

  ./Tool2/GetUNKNOWNFREQ/ :

       - KTS에서 사용하는 미등록어 발생 확률을 구하는 Utility.
       - UNKNOWNFREQ는 임의의 코퍼스를 KTS로 태깅하고 수동 태깅된
         것과 비교하여 얻는 확률 값이다. KTS로 태깅할 경우,
         미등록어에 대해서는 앞에 `x'가 marking되어 있다.
       - KTS에서 사용하는 P(k_i^1|t_i^1).

  ./Tool2/GetLEXFEATURE/ :
 
       - KTS에서 사용하는 P(t_i^1|t_i^2,m_i^2)을 구하는 Utility.

  ./Tool2/Evaluation/ :
     
       KTS에 의해 태깅을 하고 그 태깅 정확률을 구하는 Utility.
       이것은 논문에서 사용한 Utility일 뿐입니다.
 
  ./Corpus/Tagged/ :

       - 수동 태깅된 코퍼스.
       - TotCrps.Tag가 전체 코퍼스의 태깅된 것임. 
                   (태깅이 잘못된 부분이 있음.)
       - raw 코퍼스와 일대일 matching이 되어 있지 않음.
       - 일대일 matching이 되는 부분은 newTest.Tag, newUnTrn.Tag입니다.
       - 그러므로 나머지 부분은 KTS가 필요로 하는 사전, 확률 값들을
         얻기 위해 사용되었습니다.

  ./Corpus/UnTagged/ :

       태깅에 사용된 raw 코퍼스.


=========================================

  KTS의 Makefile 

=========================================

  ./Makefile : KTS를 compile하고 libkts.a를 만든다.
               그 후, 사전을 구축한다.
               (libktspl.a는 Prolog를 위한 library인데
                이 부분은 compile하지 않는다.)

  ./kts.make : C 언어 사용자를 위한
               libkts.a를 만든다.(Makefile에서 사용함)

  ./ktspl.make : Prolog 사용자를 위한
               libktspl.a를 만든다.(Makefile에서 사용 안함)

  * Prolog 사용자는 libktspl.a를 만들기 위해서는

  environment variable SP_PATH에 Sicstus-Prolog Directory가
  assign되어야 한다. (.cshrc에 넣으면 된다.)

  예) setenv SP_PATH /user/local/lib/sicstus2.1

  make -f ktspl.make
 

==========================================

  KTS가 필요로 하는 파라미터를 얻는 방법.

==========================================

  corpus가 Trn.Tag(Trn.raw), unknTrn.Tag(unknTrn.raw), 
  Test.Tag(Test.raw)로 나누어질 때,

  1.  Trn.Tag로 TRANTBL, TRANFREQ, total.dict를 구한 후,
      사전 kts_dict을 구축한다.

  2.  KTS를 UNKNOWNMODEL2로 compile한 후,  
      LEXFEATURE를 Trn.Tag로부터 추출한다.
      (논문에서 요구하는 LEXFEATURE를 사용하기 위해서는
       UNKNOWNMODEL3로 compile해야 한다.) 
 
      KTS로 unknTrn.raw를 태깅하고 수동 태깅된 것과 
      비교하여 UNKNOWNFREQ를 구한다.


