/*----------------------------------------------------------------------*
 *                                                                      *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9             *
 *                                                                      *
 * SangHo Lee                                                           * 
 *                                                                      *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr                    *
 *                                                                      *
 * Computer Systems Lab.                                                *
 * Computer Science , KAIST                                             *
 *                                                                      *
 * ktsunknown.c ( Korean POS Tagging System : Unknown Word Prediction ) *
 *                                                                      *
 *----------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define DB_DBM_HSEARCH 1
#ifdef HAVE_DB_H
#  include <db.h>
#elif defined HAVE_DB4_DB_H
#  include <db4/db.h>
#elif defined HAVE_DB3_DB_H
#  include <db3/db.h>
#elif defined HAVE_NDBM_H
#  include <ndbm.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

#include "ktsdefs.h"
#include "ktsds.h"
#include "ktstbls.h"
#include "kimmocode.h"
#include "ktssyll.h"

#define FEATUREEPSILON   0.0000001 /* (1-Epsilon)P(Ending|Tag) + Epsilon  */
#define CHEEONENDIDX             4 /* openClass[]에서 체언의 마지막 Index */ 
#define YONGEONENDIDX            6 /* openClass[]에서 용언의 마지막 Index */ 
#define BUSAENDIDX               9 /* openClass[]에서 부사의 마지막 Index */
#define EXCLENDIDX              11 /* openClass[]에서 감탄사,관형사 Index */

#ifdef UNKNOWNMODEL3
PUBLIC double unknownProb[__NUMOFOPENCLASS__][3] ; /* Known-Word , Unknown-Word */
PUBLIC UnkPttrn unknownLexm[__NUMOFOPENCLASS__]  ; /* Prob(Ending_i|Tag)를 위해 */
PUBLIC PttrnTot patternTotal[__PTTRNTOT__]       ; /* pattern의 가장 큰 갯수    */ 
PUBLIC void LoadUnknownFreq() ; /* Prob(Unkn|Tag) , Prob(Known|Tag) */ 
PUBLIC void LoadUnknownLexm() ; /* Prob(Tag_Ending_i|Tag)           */ 
PRIVATE UCHAR FirstTag()      ; /* Tag string에서 첫번째 Tag        */
PUBLIC Pttrn Str2Lexm()       ; /* Str to Pattern                   */
PUBLIC int IsInOpenTag()      ; /* Is Str In Open-Tag ?             */
PUBLIC double ProbLexFtr()    ; /* Prob(Feature=Pattern|Tag)        */
int    numOfPtrns = 0         ; 
double numtotalptrn = 0.0     ; 
#endif

EXTERN InputW inputWP[__EOJEOLLENGTH__]            ; /* Input Word Pool        */
EXTERN UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /* 전이 tbl               */
EXTERN Morph  morphP[__NUMMORPH__]                 ; /* Morph Pool             */
EXTERN WSM    wordSM[__NUMWSM__]                   ; /* 새로운 환경 ManagePool */ 
EXTERN int tagfreq[__NUMOFTAG__]                   ; /* 형태소 발생 Frequency  */
EXTERN int morPtr ;                     /*   Morph Pool Top Pointer of morphP  */
EXTERN int smPtr  ;                     /* Stack Manager Top Pointer of wordSM */
EXTERN SINT endNodeValue  ; /* kimmolen * __SKIPVALUE__ defined in ktsmoran.c  */
EXTERN UCHAR _FINAL_          ; /* 어절 , 문장 마지막              */
EXTERN UCHAR _INITI_          ; /* 어절 , 문장 처음                */
EXTERN UCHAR _INT_            ; /* 매개 모음 `으'                  */
EXTERN UCHAR _EFP_            ; /* 선어말 어미                     */
EXTERN UCHAR _JC_             ; /* 격조사                          */
EXTERN UCHAR _JCM_            ; 
EXTERN UCHAR _JCA_            ; 
EXTERN UCHAR _JJ_             ; 
EXTERN UCHAR _XN_             ; /* 접사                            */
EXTERN UCHAR _EXM_            ; /* 관형형 전성어미                 */
EXTERN UCHAR _EXN_            ; /* 명사형 전성어미                 */
EXTERN UCHAR _ECQ_            ;
EXTERN UCHAR _ECX_            ; 

EXTERN UCHAR TagIdx()         ;
EXTERN void MorPush()         ; /* Morph Push                      */ 
EXTERN void ConTest()         ; /* Connectibility Test             */
EXTERN Morph MakeMor()        ; /* 하나의 형태소 record 만든다     */ 
EXTERN int kimmo2ks()         ; /* 이성진 코드를 ks완성형으로 바꿈 */ 

PUBLIC void PredictUnknown()      ; /* Unknown Word Prediction     */
PRIVATE Morph_Part GetPartMorph() ; /* Get Part of Morph Structure */
PRIVATE int GetUnknownWord()      ; /* Get Unknown Word            */
PRIVATE int CheckEndings()        ; /* X is in {조사,어미,접사} ?  */
PRIVATE int CheckSuffix()         ; /* 접사가 있는가 조사          */
PRIVATE double UnknownProb()      ; /* Unknown Word의 Lexical Prob */
PRIVATE int LingFltr()            ; /* 마지막 음절을 조사하는 모듈 */
PRIVATE int IsInTbl()             ;

/*
 * Open Class Word : nct : 시간성 보통 명사
 *                   nca : 동작성 보통 명사
 *                   ncs : 상태성 보통 명사
 *                   nc  : 보통 명사
 *                   nq  : 고유 명사
 *                   pv  : 동사
 *                   pa  : 형용사
 *                   m   : 관형사 
 *                   at  : 시간 부사
 *                   ajs : 문장 접속 부사
 *                   a   : 부사
 *                   i   : 감탄사
 *
 * Open Class Word는 INI 다음에 무조건 올 수 있다 
 *
 * Heuristics : {은 , 는 , 이 , 가 , 을 , 를 , 어미 , 접사} 
 *
 *              위의 형태소가 밑에서 위로 Searching할때  
 *              있으면 위에서 밑으로 형태소를 추정할때
 *              그 곳까지만 추정한다.
 *              즉 전체 어절을 하나의 형태소로 추정하지 않는다
 * 
 *  우선순위 :  접사 : xpv , xpa , xn  , xa 
 *              조사 : jc  , jcm , jcv , jca , jcp , jx  , jj
 *              어미 : efp , ecq , ecs , ecx , exm , exn , exa , ef
 * 
 *  Linguistic Filter : pv , pa , m , at , ajs , a , i
 */ 

UCHAR openClass[__NUMOFOPENCLASS__]  ; 

PUBLIC void InitOpenClass()
{
  openClass[0]  = TagIdx("nct") ; 
  openClass[1]  = TagIdx("nca") ; 
  openClass[2]  = TagIdx("ncs") ; 
  openClass[3]  = TagIdx("nc")  ;
  openClass[4]  = TagIdx("nq")  ;
  openClass[5]  = TagIdx("pv")  ;
  openClass[6]  = TagIdx("pa")  ; 
  openClass[7]  = TagIdx("at")  ;
  openClass[8]  = TagIdx("ajs") ; 
  openClass[9]  = TagIdx("a")   ; 
  openClass[10] = TagIdx("m")   ; 
  openClass[11] = TagIdx("i")   ; 
}/*--End of InitOpenClass--*/

/*--------------------------------------------------------------*
 *                                                              *
 * PredictUnknown : Unknown Word를 처리한다.                    * 
 *                  형태소 분석이 부분적으로 된 결과가          *
 *                  morP[]에 있는데 가능한 모든 형태소 추정을   *
 *		    한다                                        *
 *                                                              *
 * 주의 사항 : 추정할 형태소의 마지막 이성진 코드가             *
 *             초성 혹은 반모음이라면                           *
 *             그 부분 형태소에 대한 추정은 하지않는다.         *
 *                                                              *
 * 형태소 추정의 문제점                                         *
 *                                                              *
 * 1. 사실은 고유명사인데 그것이 다른 말로 형태소 분석되는것은  *
 *    어떻게 해야 하는가 (철수씨는 : 철수/nc+씨/xn+는/jx)       *
 *                                                              *
 * 2. 명사인데 다른 말로 형태소 분석이 되는 경우                *
 *    (원고를 : 원/nbu+고르/pv+ㄹ/exm)                          *
 *                                                              *
 * 3. 형태소 추정시 전체를 명사로 추정                          *
 *    (즉 FIN앞에 올 수 있는 것도 추정하여야 한다)              *
 *    (음절 : 음ㅈ + 얼/nc , 음저 + ㄹ/exm )                    *
 *            Fail           Success                            *
 *     ==> 음절/nc                                              *
 *                                                              *
 * 4. 체언은 2음절이 가장 많고 , 용언은 3음절이 가장 많다       *
 *    (Feature로 사용할 수 있는 요소이다)                       *
 *                                                              *
 * 5. 추정하는 것이 Open Class에 들어갈 경우에만 한다.          *
 *    Open Class : 체언 , 용언                                  * 
 *                                                              *
 * 방법 1. : Simplest Model                                     *
 *                                                              *
 *   1. (INI , 추정 형태소 , 부분 분석된 형태소들의 가장 왼쪽)  *
 *      ConTest(INI,추정 형태소) && ConTest(추정형태소,부분)    * 
 *      위의 조건을 만족하는 Tag로 추정하여 morP[]에 저장       *
 *                                                              *
 *   2. 'INI'를 morP[]에 저장                                   *
 *                                                              *
 *   3. Prob(Word|Tag) = ( Freq(Word,Tag) = 1 / Freq(Tag) + 1 ) *
 *                                                              *
 *   4. Prob(Word|Tag) = 1.0으로 간주 : 즉 , P(Tag_i|Tag_i-1)만 *
 *                                      고려하겠다는 뜻         *
 *                                                              *
 * 방법 2. : Statistical Model                                  *
 *                                                              *
 *  P(Un-Word|Tag) =   P(Unknown|Tag)                           *
 *                   * ((1-Epsilon) * P(Endings|Tag) + Epsilon) *
 *                                                              *
 *--------------------------------------------------------------*/

PUBLIC void PredictUnknown() 
{
  UCHAR tempCh = '\0'              ; /* NULL                               */
  UCHAR currTag                    ; 
  SINT  maxwright                  ; 
  int unIdx , idx                  ; 
  int start_idx , end_idx          ;
  int numNext                      ; /* 다음 형태소와 연결이 가능한 갯수   */
  int scanIdx                      ; /* 부분 형태소의 Scanning Index       */
  int unknLen                      ; /* 미등록어의 음절 갯수               */
  int tempIdx                      ; 
  int construct = 0                ;  
  Morph oneMor                     ; /* 형태소 분석된것 하나               */
  UCHAR unknWord[__MAXMORPH__]     ; 
  UCHAR unkn_hcode , unkn_lcode    ; /* 미등록어의 마지막 음절             */
  SINT nextIdx[__NUMCONNECTION__]  ; 
  UCHAR endings[250]               ; /* 형태소를 넣을 때 붙었던 형태소 tag */
  int numEndings = 0               ; /* endings[0 .. numEndings - 1]       */
  Morph_Part start_part , end_part ; 

 /* Stack Pop until morphP[morPtr].wright == -1
  * morphP[morPtr].left가 0보다 작으면 준말 사전에서 가져온 것
  */ 
 /* Case 1 : Pop until if morphP[morPtr].env != 1 */
 /* Case 2 : Pop until just if morphP[morPtr].wright != -1 */

  while(morphP[morPtr].wright == -1 || morphP[morPtr].left <= 0 ) --morPtr ;

  start_idx = end_idx = morPtr ; /* morphP[]을 위에서 아래서 scanning한다 */

  while(start_idx > 0) {

    start_part = GetPartMorph(start_idx) ;  
    end_part = GetPartMorph(--end_idx) ; 

    while (!memcmp(&start_part,&end_part,sizeof(Morph_Part))) 
			 end_part = GetPartMorph(--end_idx) ; 

#ifdef DEBUG2
    printf("PredictUnknown Module : start_idx : %d , end_idx : %d\n",
		 start_idx , end_idx) ; 
#endif

    /*
     * [start_part .. end_idx+1]에 있는 형태소에 대해 왼쪽을 복원
     * Unknown Word를 가져오고 그것의 마지막 이성진 코드가 
     * 초성이나 혹은 반 모음이면 만들지 않는다. 
     */

    if ((unknLen = GetUnknownWord(&unkn_hcode,&unkn_lcode,unknWord,start_part.left))) {
      for(unIdx = 0 ; unIdx < __NUMOFOPENCLASS__ ; ++unIdx) {
        numNext = 0 ; 
	currTag = openClass[unIdx] ; 
        for(scanIdx = start_idx ; scanIdx > end_idx ; --scanIdx)
          if (tranTable[currTag-'0'][morphP[scanIdx].pos-'0'] == '1' &&
               LingFltr(unknLen,unkn_hcode,unkn_lcode,unIdx)) { /* connectable */

            if (morphP[scanIdx].pos == _INT_) { /* `으' 처리 */
              if (strchr(Is_jong,inputWP[start_part.left/__SKIPVALUE__ -1].lee))
                                                                 /* 유종성 체언 */ 
                for (tempIdx = 0 ; morphP[scanIdx].nidx[tempIdx] != -1 ; ++tempIdx) {
                  nextIdx[numNext] = (SINT) morphP[scanIdx].nidx[tempIdx] ; 
                  endings[numEndings++] = morphP[nextIdx[numNext]].pos ; 
                  ++numNext ; 
                }
              /* 무종성 체언이면 아무것도 하지 않는다 */
            } else {
               nextIdx[numNext++] = (short int) scanIdx ; 
               endings[numEndings++] = morphP[scanIdx].pos ; 
            }

          }

        nextIdx[numNext] = (short int) -1 ; 
	if (numNext != 0) { /* 부분 형태소와 붙는 것이 있을 경우 */

         oneMor = MakeMor( (short int) 0 , 'x' , (short int) 0 , 
                     (short int) start_part.left ,
		     (short int) start_part.wright , nextIdx , unknWord ,
		     currTag , '0' , UnknownProb(currTag)) ; 

          MorPush(oneMor) ; 
	  construct = 1   ; /* 격자를 복구한 것이 있다 */

#ifdef DEBUG
	  printf("morPtr %d , start_idx %d , end_idx %d , nextIdx[0] = %d\n",
		  morPtr,start_idx,end_idx,nextIdx[0]) ; 
#endif

        }/*-End of If-*/
      }/*-End of For-*/
     
#ifdef BASELING1
       if (CheckSuffix(endings,numEndings)) end_idx = 0 ;  
#endif

    /*  접사가 있으면 더 이상 morphP[]을 찾지 않는다       */ 

   }/*-End Of If-*/

/* Case 2 */

   while(morphP[end_idx].wright == -1 || morphP[end_idx].left <= 0) --end_idx ; 
   start_idx = end_idx ;  

#ifdef DEBUG2
    printf("PredictUnknown Module : start_idx = end_idx : %d\n", start_idx) ; 
#endif

  }/*-End of While-*/

  /* 
   * 다음 If문은 하나의 형태소가 독립적으로 하나의 어절이 될 수 있는가 조사  
   * Connetability(추정 형태소 , FIN) 조사
   *
   * endings[0 .. numEndings - 1]에 있는 내용이 { 조사 , 어미 , 접사 } 중
   * 하나라도 있으면 전체 형태소가 독립적으로 하나의 어절이 된다고 가정하지
   * 않는다 : Heuristics
   *
   */

  if (!CheckEndings(endings,numEndings)) {

   if ((unknLen = GetUnknownWord(&unkn_hcode,&unkn_lcode,unknWord,endNodeValue))) {
    for(unIdx = 0 ; unIdx < __NUMOFOPENCLASS__ ; ++unIdx) {
      currTag = openClass[unIdx] ; 
      if (tranTable[currTag-'0'][_FINAL_-'0'] == '1' &&
          LingFltr(unknLen,unkn_hcode,unkn_lcode,unIdx)) { /* connectable */
	nextIdx[0] = (short int)  0 ; 
        nextIdx[1] = (short int) -1 ; 
        oneMor =  MakeMor( (short int) 0 , 'x' , (short int) 0 , 
	           (short int) start_part.left ,
	           (short int) start_part.wright , nextIdx , unknWord ,
	           currTag , '0' , UnknownProb(currTag)) ; 

        MorPush(oneMor) ; 
	construct = 1 ; 
      }/*-End of If-*/
    }/*-End of For-*/
   }/*-End Of If-*/
  }

  /* 여기서 INI 를 넣는다 */
  /* 전체에 대해 하나도 추정한 것이 없을 때는 명사로 추정한다. */

  for(idx = 0 ; idx < endNodeValue/__SKIPVALUE__ ; ++idx) 
     unknWord[idx] = inputWP[idx].lee ; 
  unknWord[idx] = '\0' ;  
  currTag = openClass[3] ; /* 보통명사로 추정 */

  if (!construct) {
     nextIdx[0] = (short int)  0 ; 
     nextIdx[1] = (short int) -1 ; 
     oneMor =  MakeMor( (short int) 0 , 'x' , (short int) 0 , 
	           (short int) start_part.left ,
	           (short int) start_part.wright , nextIdx , unknWord ,
	           currTag , '0' , UnknownProb(currTag)) ; 
     MorPush(oneMor) ; 
  }

  ConTest(morPtr,&maxwright,nextIdx,(SINT) 0 , _INITI_ ) ;

  oneMor = MakeMor(
   0         , /* 환경                                         */
   'x'       , /* 형태소와 형태소의 띄어쓰기 :
                  예 : 이러는 : 이렇게 하는 (띄어쓰기가 필요)  */
   (SINT) 0  , /* INI 의 왼쪽   : 0                            */
   (SINT) 0  , /* INI 의 오른쪽 : 0                            */
   maxwright , /* 최종 형태소 해석 결과의 마지막 node MAX      */
   nextIdx   , /* 다음 형태소가 있는 곳을 가리키는 index:-1:끝 */
   &tempCh   , /* NULL                                         */
   _INITI_   , /* INI index                                    */
   '0'       , /* INI 불규칙 code                              */
   (double) 1.0/* 확률 : P(tag|word)                           */
  ) ;
  MorPush(oneMor) ;               /* INI를 넣는다 */

}/*-------End of PredictUnknown-------*/ 


/*-------------------------------------------------*
 *                                                 *
 * CheckEndings : endings[]안의 요소들 중 하나라도 *
 *                _EFP_ 이면                       *
 *                된다면 return 1                  *
 *		  else   return 0                  *
 *                                                 *
 *-------------------------------------------------*/

PRIVATE int CheckEndings(endings,num)
UCHAR endings[] ;                    /* Unknown Word 다음 형태소 Tags */
int   num       ;
{
  int i ; 

/*
  for(i = 0 ; i < num ; ++i)
    if(endings[i] == _EFP_ || endings[i] == _JCM_ ||
       endings[i] == _JCA_ || endings[i] == _JJ_) return 1 ; 

*/

#ifdef BASELING1 

  for(i = 0 ; i < num ; ++i)
    if(endings[i] == _EFP_ || endings[i] == _JCM_ ||
       endings[i] == _JCA_ || endings[i] == _JJ_  ||
       endings[i] >= _XN_) return 1 ; 

#endif

  return 0 ; 
}/*----------End of CheckEndings-----------*/


/*------------------------------------------------*
 *                                                *
 * CheckSuffix : endings[]안의 요소들 중 하나라도 *
 *                { 접사 }의 원소가               *
 *                된다면 return 1                 *
 *		  else   return 0                 *
 *               (단 명사 접미사 : ㅁ , 기 제외)  *
 *------------------------------------------------*/

PRIVATE int CheckSuffix(endings,num)
UCHAR endings[] ;                    /* Unknown Word 다음 형태소 Tags */
int num         ;
{
  int i ;

  for(i = 0 ; i < num ; ++i)
    if(endings[i] >= _XN_) return 1 ; 

  return 0 ; 

}/*---------End of CheckSuffix---------*/


/*-----------------------------------------------------*
 *                                                     *
 * GetUnknownWord : Unknown Word를 inpuWP[]에서 얻고   * 
 *                 만약 마지막 이성진 코드가 초성 혹은 *
 *		   반모음이라면 Unknown Word가 아니다  *
 *                                                     *
 * 일단, ``해(여 변칙),되''는 자주 발생하므로          *
 * 그런 경우는 충분히 가능성을 제시해 주어야 한다.     * 
 *                                                     *
 * ``해'',``했''은 분명히 `하+어',`하+었'이니까        *
 * 불규칙 현상이 발생되었다고 해도 가능성 제시 필요    *
 *                                                     *
 *-----------------------------------------------------*/

PRIVATE int GetUnknownWord(high_code,low_code,unknWord,leftNode)
UCHAR *high_code ; 
UCHAR *low_code  ; 
UCHAR unknWord[] ; 
SINT  leftNode   ; 
{
  int idx        ; 
  int hanlen     ; /* ks code로 변환 후 음절 길이 */
  UCHAR tmph[50] ; 

  for(idx = 0 ; idx < leftNode/__SKIPVALUE__ ; ++idx)
    unknWord[idx] = inputWP[idx].lee ; 
  unknWord[idx] = '\0' ;  

  if (strchr("ghqndlmbrsvfjzcktp",unknWord[idx-1])) return 0 ; 
  if (unknWord[idx-1] == 'w') {   /* `와,워,왜'일 경우만 축약 */  
    if (inputWP[idx].lee == 'a') unknWord[idx-1] = 'o' ; 
    else if (inputWP[idx].lee == 'e') unknWord[idx-1] = 'u' ; 
    else return 0 ; 
  } 

/*
  if (unknWord[idx-2] == 'w' && unknWord[idx-1] == '8')
    unknWord[idx-1] = 'i' ; 
*/

  if (unknWord[idx-1] == 'y') {
    if (inputWP[idx].lee == 'e') unknWord[idx-1] = 'i' ; 
    else return 0 ; 
  }
 
 /* 
  * unknWord의 음절 길이를 return한다.
  */ 

  if(!kimmo2ks(unknWord,tmph)) {

 /*
  *   fprintf(stderr,"Error in ktsunknown.c::GetUnknownWord()\n") ; 
  *   exit(1) ; 
  */

    return 0 ; 
  }

  hanlen = strlen(tmph) ; /* 미등록어의 음절 갯수 * 2 */

  *high_code = tmph[hanlen-2] ; /* 마지막 음절의 high_code */
  *low_code  = tmph[hanlen-1] ; /* 마지막 음절의 low_code */

  hanlen /=2 ; 

#ifdef DEBUG3

  printf("%s\n",unknWord) ; 

#endif


  return hanlen ; 
}/*-----End of GetUnknownWord-----*/


/*----------------------------------------------*
 *                                              *
 * LingFltr : Lingustic Filter                  * 
 *            미등록어의 마지막 음절을 조사하여 *
 *            가능한 음절일 경우 1              *
 *            else 0 를 return                  *
 *                                              *
 *  openIdx : openClass[]의 Index               *
 *                                              *
 *----------------------------------------------*/

PRIVATE int LingFltr(len,hcode,lcode,openIdx)
int len     ;           /* 미등록어의 음절 길이 */ 
UCHAR hcode ;           /* 음절의 high code     */
UCHAR lcode ;           /* 음절의 low code      */
int openIdx ;           /* openClass[]의 Index  */
{

/*
 *   return 1 - IsInTbl(hcode,lcode,
 *              notmorphTbl[0].eumjeolset,notmorphTbl[0].size) ; 
 */

#ifdef BASELING2

   if (IsInTbl(hcode,lcode,
       notmorphTbl[0].eumjeolset,notmorphTbl[0].size)) return 0 ; 
   else if (openIdx > CHEEONENDIDX && openIdx <= YONGEONENDIDX) {
     len = (len >= 3) ? 2 : len - 1 ;  /* 0 <= len <= 2   */
     return IsInTbl(hcode,lcode,
		yongeonTbl[len].eumjeolset,yongeonTbl[len].size) ; 
   } else if (openIdx > YONGEONENDIDX && openIdx <= BUSAENDIDX) {
     len = (len >= 5) ? 4 : len - 1 ;  /* 0 <= len <= 4   */
     return IsInTbl(hcode,lcode,
		busaTbl[len].eumjeolset,busaTbl[len].size) ; 
   } else if (openIdx > BUSAENDIDX)    /* 감탄사 , 관형사 */
     return IsInTbl(hcode,lcode,
		exclTbl[0].eumjeolset,exclTbl[0].size) ; 

#endif

   return 1 ; 

}/*---------End of LingFltr---------*/


/*-----------------------------------------*
 *                                         *
 * IsInTbl  : Is In Table ?                * 
 *            Table내에 그 음절이 있는가 ? *
 *                                         *
 *            Linear Search                *
 *-----------------------------------------*/

PRIVATE int IsInTbl(hcode,lcode,set,size)
UCHAR hcode ;          /* 음절의 high code */
UCHAR lcode ;          /* 음절의 low code  */
UCHAR set[] ;          /* 음절의 집합      */
int    size ;          /* 집합의 크기      */ 
{
  int cnt ; 

  for(cnt = 0 ; cnt < size * 2 ; cnt += 2)
    if (set[cnt] == hcode) {
      if (set[cnt+1] == lcode) return 1 ; 
    }

  return 0 ; 
}/*----------End of IsInTbl-----------*/


/*----------------------------------------------------*
 *                                                    *
 * GetPartMorph : Morph Pool에서 비교하기 위한 Data만 *
 *		 구한다                               *
 *                                                    *
 *----------------------------------------------------*/

PRIVATE Morph_Part GetPartMorph(idx)
int idx ; 
{
  Morph_Part ret ; 

  ret.env    = morphP[idx].env    ; 
  ret.left   = morphP[idx].left   ;
  ret.right  = morphP[idx].right  ; 
  ret.wright = morphP[idx].wright ; 

  return ret ; 
}/*-----End of GetPartMorph-----*/


/*----------------------------------------------------*
 *                                                    *
 * UnknownProb : Unknown-Word의 Prob를 구한다.        *
 *                                                    *
 *   UNKNOWNMODEL1                                    *
 *                                                    *
 *           1. P(Word|Tag) = 1.0 / (Freq(Tag) + 1.0) *
 *                                                    *
 *   UNKNOWNMODEL2                                    *
 *                                                    *
 *           2. P(Word|Tag) = 1.0                     *
 *                                                    *
 *   UNKNOWNMODEL3                                    *
 *                                                    *
 *           3. P(Word|Tag) = Prob(t_i|w_i+1,t_i+1)   *
 *                                                    *
 *----------------------------------------------------*/

PRIVATE double UnknownProb(inTag)
UCHAR inTag ; 
{
  double lex_prob ; 

#ifdef UNKNOWNMODEL1
  lex_prob = (double) 1.0 / (double) (tagfreq[inTag-'0'] + 1) ; 
#else

  lex_prob = 1.0 ; /* 여기서는 확률을 1.0이라고 놓고 
		    * ktstagger.c::AddTrellis()에서 해결한다
		    */
#endif
  return lex_prob ; 

}/*---------End of UnknownProb-----------*/


/*******************************
 ******* UNKNOWN MODEL 3 *******
 *******************************/

#ifdef UNKNOWNMODEL3

/*----------------------------------------------------*
 *                                                    *
 * LoadUnknownFreq : Open Class Tag에 대하여          *
 *                   Freq(Known-Word,Tag) ,           *
 *                   Freq(Unknown-Word,Tag)를 구한다. *
 *                                                    *
 *----------------------------------------------------*/

PUBLIC void LoadUnknownFreq(fname)
char fname[] ;                /* fname == UNKNOWNFREQ */
{
  FILE *f1 ; 
  char in_tag[5] ; /* tag                                 */
  UCHAR tag_idx  ; /* TagIdx(tag)                         */
  int  tag_kn    ; /* freq(known,tag)                     */
  int  tag_ukn   ; /* freq(unknown,tag)                   */ 
  int  tag_sum   ; /* freq(known,tag) + freq(unknown,tag) */
  int  tagun_sum ; 
  int  idx       ;

  tagun_sum = 0  ;

  if ((f1 = fopen(fname,"r")) ==NULL) {
    fprintf(stderr,"Error : File Not Found : %s\n",fname) ; 
    exit(1) ; 
  }

  while (fscanf(f1,"%s%d%d",in_tag,&tag_kn,&tag_ukn) != EOF) {
    tag_idx = TagIdx(in_tag) ; 
    for(idx = 0 ; idx < __NUMOFOPENCLASS__ ; ++idx)  
      if(openClass[idx] == tag_idx) {
	tag_sum = tag_kn + tag_ukn ; 
	tagun_sum += tag_ukn ; 
	if (tag_sum == 0) tag_sum = tag_kn = 1 ; /* P(kn|tag) = 1.0 */
        unknownProb[idx][0] = (double) tag_kn / (double) tag_sum ; 
	unknownProb[idx][1] = (double) tag_ukn / (double) tag_sum ; 
        unknownProb[idx][2] = (double) tag_ukn ; 
      }
  }

  for(idx = 0 ; idx < __NUMOFOPENCLASS__ ; ++idx)  
      unknownProb[idx][2] /= (double) tagun_sum ; 


  fclose(f1) ; 

#ifdef DEBUG3
  {
    int i ; 
    printf("-----------Loading Unknown-Probabilities----------\n") ; 
    printf(" Open Class Tag    P(Known|Tag)    P(Unknown|Tag)\n") ;  
    for(i = 0 ; i < __NUMOFOPENCLASS__ ; ++i)
      printf("   %5s           %f       %f\n",posTags[openClass[i]-'0'],
                         unknownProb[i][0] , unknownProb[i][1]) ; 
    printf("\n") ; 
  }
#endif

}/*--------End of LoadUnknownFreq--------*/


/*--------------------------------------------------*
 *                                                  *
 * LoadUnknownLexm : Open Class Tag에 대하여        *
 *                   Lexical Feature를 Loading한다. * 
 *                                                  *
 *--------------------------------------------------*/

PUBLIC void LoadUnknownLexm(fname) 
char fname[] ;               /* fname == LEXFEATURE */
{
  FILE *f1 ; 
  UCHAR curr_tag               ; /* Current Tag                  */
  UCHAR cmp_tag                ; /* Comparison Tag               */
  char  inbuf[100]             ; /* pattrn & it's frequency      */
  Pttrn tmpPttrn[__MAXPTTRN__] ; /* Pttrn을 저장한 후 malloc     */
  double pttrn_total           ; /* pttrn_total = Sum Pttrn Prob */
  int    openIdx               ; /* OpenTagIdx                   */
  int    idx , idx2            ; 
  int    numPttrn = 0          ; /* 0 .. numPttrn - 1            */ 
  int    open_class_flag = 0   ; /* if 1 : open class tag pttrn  */ 

  cmp_tag = _INITI_ ; /* pattern의 제일 처음에 INI는 없다 */ 

  if ((f1 = fopen(fname,"r")) == NULL) {
    fprintf(stderr,"Error : File Not Found : %s\n",fname) ; 
    exit(1) ; 
  }

  while (fgets(inbuf,99,f1) != NULL) {
#ifdef DEBUG3
    printf("%s",inbuf) ; 
#endif

    curr_tag = FirstTag(inbuf) ; 
    if (curr_tag != cmp_tag) {
      open_class_flag = IsInOpenTag(curr_tag) ; 

      if (IsInOpenTag(cmp_tag)) { 

        for(idx = 0 ; idx < numPttrn ; ++idx) {
          for(idx2 = 0 ; idx2 < numOfPtrns ; ++idx2)
	    if (!strcmp(patternTotal[idx2].pttrn,tmpPttrn[idx].pttrn)) {
	      patternTotal[idx2].pttrntot += tmpPttrn[idx].pttrnProb ; 
              break ; 
            }

          if(idx2 == numOfPtrns) {
	    strcpy(patternTotal[numOfPtrns].pttrn,tmpPttrn[idx].pttrn) ; 
            patternTotal[numOfPtrns++].pttrntot = tmpPttrn[idx].pttrnProb ;   
          }
	}

        /* Write tmpPttrn[] into Patterns[] */

        pttrn_total = 0.0 ; 
        for (idx = 0 ; idx < numPttrn ; ++idx) 
          pttrn_total += tmpPttrn[idx].pttrnProb ; 

        openIdx = IsInOpenTag(cmp_tag) - 1 ; /* -1이 옳다 */ 
	unknownLexm[openIdx].totalprob  = pttrn_total ; 
        unknownLexm[openIdx].numofpttrn = numPttrn ; 
        unknownLexm[openIdx].pttrnPtr   = (Pttrn *) malloc(sizeof(Pttrn)*numPttrn) ; 
        memcpy(unknownLexm[openIdx].pttrnPtr,tmpPttrn,
			       sizeof(Pttrn)*numPttrn) ; 
      }

      cmp_tag = curr_tag ; 
      numPttrn = 0 ; 

/*    
 * FEATUREFREQ가 sort by ascii 되어있어서 다음 line은 없애야 된다. 
 * if (curr_tag > openClass[__NUMOFOPENCLASS__-1]) break ; 
 */

    }

    if (open_class_flag) tmpPttrn[numPttrn++] = Str2Lexm(inbuf); 

  }/* end of while */

  fclose(f1) ; 

  for(idx = 0 ; idx < numOfPtrns ; ++idx) 
    numtotalptrn += patternTotal[idx].pttrntot ; 

#ifdef DEBUG3

  {
    int i , j ; 
    for(i = 0 ; i < __NUMOFOPENCLASS__ ; ++i)
     for(j = 0 ; j < unknownLexm[i].numofpttrn ; ++j)
      printf("%s : %f : %s\n", posTags[openClass[i]-'0'] , 
                  unknownLexm[i].pttrnPtr[j].pttrnProb,
                  unknownLexm[i].pttrnPtr[j].pttrn) ; 
  }

#endif

}/*------------End of LoadUnknownPttrn-----------*/


/*--------------------------------------------------------*
 *                                                        *
 * FirstTag : Tag String에서 첫번째 Tag의 TagIdx를 return *
 *                                                        *
 *--------------------------------------------------------*/

PRIVATE UCHAR FirstTag(str)
char str[] ; 
{
  UCHAR ret_t  ; 
  int idx      ; 
  char tmpt[5] ; 

  for(idx = 0 ; str[idx] != '\0' ; ++idx) {
    tmpt[idx] = str[idx] ; 
    if (str[idx] == ' ') {
      tmpt[idx] = '\0' ; 
      ret_t = TagIdx(tmpt) ; 
      break ; 
    }
  }
  return ret_t ; 

}/*--------End of FirstTag---------*/


/*-----------------------------------*
 *                                   *
 * IsInOpenTag : Open Class Tag 인가 *
 *                                   * 
 *-----------------------------------*/

PUBLIC int IsInOpenTag(in_tag)
UCHAR in_tag ; 
{
  int idx ; 

  for(idx = 0 ; idx < __NUMOFOPENCLASS__ ; ++idx)
    if (openClass[idx] == in_tag) return idx + 1 ; /* OpenTagIdx + 1 */
  
  return 0 ; 
}/*------End of IsInOpenTag------*/


/*------------------------------------------------------*
 *                                                      *
 * ProbLexFtr  : Open Class Tag의 Index와 lex feature를 *
 *               받아 unknownPttrn[]에서 lookup하여     *
 *               P(lex-feature|tag)를 return한다        * 
 *                                                      *
 * P(lex-feature|tag) = P(w_i+1,t_i+1|t_i)              *
 *                                                      *
 *------------------------------------------------------*/

PUBLIC double ProbLexFtr(openIdx,in_pttrn)
int openIdx      ;                /* openClass[openIdx] */ 
UCHAR in_pttrn[] ;                /* Feature            */
{
  int idx       ; 
  Pttrn tmpPtrn ; /* tmp Pattern */

  for(idx = 0 ; idx < unknownLexm[openIdx].numofpttrn ; ++idx) {
    tmpPtrn = unknownLexm[openIdx].pttrnPtr[idx] ; 
    if(!strcmp(tmpPtrn.pttrn,in_pttrn)) 
	return tmpPtrn.pttrnProb / unknownLexm[openIdx].totalprob ; 
  }

 /*
  * 만약 오른쪽 (w_i+1,t_i+1)이 Feature-집합에 있지 않다면
  * FEATUREEPSILON을 return한다.
  * 
  */

  return FEATUREEPSILON ; 

}/*------------End of ProbLexFtr------------*/


/*--------------------------------------------------------*
 *                                                        *
 * ProbInvLexFtr : Open Class Tag의 Index와 lex feature를 *
 *                 받아 unknownPttrn[]에서 lookup하여     *
 *                 P(tag|lex-feature)를 return한다        * 
 *                                                        *
 * P(tag|lex-feature) = P(t_i|w_i+1,t_i+1)                *
 *                                                        *
 *--------------------------------------------------------*/

PUBLIC double ProbInvLexFtr(openIdx,in_pttrn)
int openIdx      ;  /* openClass[openIdx] */ 
UCHAR in_pttrn[] ;  /* Feature            */
{
  int idx         ; 
  double mrgprob  ; 
  Pttrn tmpPtrn   ; /* tmp Pattern */

  for(idx=0 ; idx < numOfPtrns ; ++idx)
    if(!strcmp(patternTotal[idx].pttrn,in_pttrn)) 
        mrgprob = patternTotal[idx].pttrntot ; 

  if (idx == numOfPtrns) /* 오른쪽 lex:pttrn이 없었다 */ 
         return FEATUREEPSILON ; 

  for(idx = 0 ; idx < unknownLexm[openIdx].numofpttrn ; ++idx) {
    tmpPtrn = unknownLexm[openIdx].pttrnPtr[idx] ; 
    if(!strcmp(tmpPtrn.pttrn,in_pttrn)) 
	return tmpPtrn.pttrnProb / mrgprob ; 
  }

  return FEATUREEPSILON ; 

}/*------------End of ProbInvLexFtr------------*/


/*--------------------------------------------*
 *                                            *
 * Str2Lexm : String을 Lexical Pttrn으로 바꿈 *
 *                                            * 
 *--------------------------------------------*/

PUBLIC Pttrn Str2Lexm(str)
char str[] ; 
{
  Pttrn ret_ptrn ;  

  char ntoken[3][__MAXPATHLEN__] ; /* lexical pattern tag list : 3 */
  int  numtoken = 0  ; 
  int  idx           ; 
  int  idx2 = 0      ; 

  for (idx = 0 ; str[idx] != '\0' ; ++idx)
    if (str[idx] == ' ') {
      ntoken[numtoken++][idx2] = '\0' ; 
      idx2 = 0 ; 
    } else ntoken[numtoken][idx2++] = str[idx] ; 

  ntoken[numtoken++][idx2] = '\0' ; 

  /* 0 .. numtoken - 1 */
  ret_ptrn.pttrnProb = (double) atoi(ntoken[numtoken - 1]) ; 

  if (numtoken == 2) ret_ptrn.pttrn[0] = '\0' ; 
  else strcpy(ret_ptrn.pttrn,ntoken[numtoken-2]) ; /* Lexical Feature */

  return ret_ptrn ; 

}/*-----------End of Str2Pttrn-----------*/

#endif


