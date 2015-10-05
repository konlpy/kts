/*------------------------------------------------------------------*
 *                                                                  *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9         *
 *                                                                  *
 * SangHo Lee                                                       *
 *                                                                  *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr                *
 *                                                                  *
 * Computer Systems Lab.                                            * 
 * Computer Science , KAIST                                         *
 *                                                                  *
 * ktstagger.c ( Korean POS Tagging System : Tagger )               *
 *                                                                  *
 *------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <memory.h>
#include "ktsdefs.h"
#include "ktstbls.h"
#include "ktsds.h"
#include "kts.h"

EXTERN int ks2kimmo()      ; /* 한글을 kimmo code로 바꿈             */ 
EXTERN int PreMorphAn()    ; /* 형태소 해석 Module                   */
EXTERN UCHAR GetPos()      ; /* Get Part-Of-Speech                   */
EXTERN void AppendNode()   ; /* path-set에 path를 sorting하여 저장   */
EXTERN void RemoveTailNode() ; /* path-set에서 마지막 node를 제거    */
EXTERN void MakeSetEmtpy()   ; /* path-set을 clear                   */
EXTERN PathRep GetOneNode(); /* path-set에서 path를 얻는다           */

PUBLIC void ViterbiForwd() ; /* Viterbi Algorithm Forward            */
PUBLIC void ViterbiBckwd() ; /* Viterbi Algorithm Backward           */
PUBLIC void AlphaCmpttn()  ; /* HMM : Alpha Computation & P(O|lamda) */
PUBLIC void BetaCmpttn()   ; /* HMM : Beta , Gamma Computation       */
PUBLIC void MakeTrellis()  ; 
PUBLIC void InitTrellis()  ; 
PUBLIC void DeleteTrellis(); 
PUBLIC void AStar()        ; /* A*-Like Algorithm : Tree-Trellis    */
PUBLIC int AddTrellis()    ; /* Trellis를 첨가한다.                 */ 

#ifdef UNKNOWNMODEL3
EXTERN int IsInOpenTag()      ; /* Is Str In Open-Tag ?      */
EXTERN double ProbLexFtr()    ; /* Prob(Feature=Pattern|Tag) */
EXTERN double ProbInvLexFtr() ; /* Prob(Tag|Feature=Pattern) */
#endif

PRIVATE void MakeTrellis2()  ; /* 한글을 처리하기 위한 routine */
PRIVATE void MorphCopy()     ;
PRIVATE void PutIdx()        ; 
PRIVATE SINT Index()         ; 
PRIVATE int CollectIndices() ; 
int cmp()                    ; 
int idxcmp()                 ; 

EXTERN UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /*품사 전이 table*/
EXTERN Morph morphP[]  ; /* Morph Pool declared in ktsmoran.c */
EXTERN int morPtr      ; /* Morph Pool Top Ptr in ktsmoran.c  */ 

EXTERN UCHAR _INITI_ ; /* 문장,어절의 마지막을 뜻한다 */
EXTERN UCHAR _NCT_   ; /* 한글 Tag Set의 시작 Tag     */
EXTERN UCHAR _JC_    ; /* 어미, 조사 시작             */

#ifdef ANALYSIS
EXTERN int numkneojeol      ; /* known 어절 갯수               */
EXTERN int numunkneojeol    ; /* unknown 어절 갯수             */
EXTERN int totunknambi      ; /* unknown word의 candidate 갯수 */
EXTERN int totknambi        ; /* known word의 candidate 갯수   */
EXTERN double totProb       ; /* log_2 P(w1..n)                */
EXTERN double LogE2         ; /* log_e 2                       */
#endif

#ifdef UNKNOWNMODEL3
PUBLIC double unknownProb[__NUMOFOPENCLASS__][3] ; /* Known-Word , Unknown-Word */
PUBLIC UnkPttrn unknownLexm[__NUMOFOPENCLASS__]  ; /* Prob(Ending_i|Tag)를 위해 */
#endif

EXTERN double tranprob[__NUMOFTAG__][__NUMOFTAG__]     ; /* 형태소 전이 확률   */

EXTERN double pathThreshold              ; /* N-Best를 구하기 위한 Threshold   */
EXTERN int    tagfreq[__NUMOFTAG__]      ; /* 형태소 발생 Frequency            */
EXTERN double tagprob[__NUMOFTAG__]      ; 
EXTERN UCHAR buffer[__BUFFERLENGTH__]    ; 
EXTERN char buffer_tag[__BUFFERLENGTH__] ; 

Sentence senP[__SENTLENGTH__]     ; 
int senPtr         ; /* Sentence Pool에 들어간 형태소의 갯수                   */
int maxNumPath =0  ; /* 입력 문장에서 가능한 Path의 최대 갯수를 미리 계산한다  */
Trellis trellis    ; /* 하나의 어절을 하나의 Trellis에 넣는다 : header trellis */
Trellis *curTrls   ; /* 지금까지 만든 Trellis의 마지막을 가리킨다              */ 
PathRep pathNBest  ; /* N-Best Candidates Path를 저장하는 장소 : Linked-List   */
PathRep pathOpen   ; /* A*-Algorithm에서 Open-Node들을 저장하는 장소           */
double alphatotal  ; /* Sum of alphas in State-Based Tagging : P(O|model)      */
double tmpProb[__TEMPPROB__] ; /* gammaprob를 sorting하기 위한 Array           */


/*----------------------------------------------------------------*
 *                                                                *
 * ViterbiForwd  : Viterbi-Algorithm Forward                      *
 *                                                                *
 * Input    : NULL                                                *
 * Output   : NULL                                                *
 * Function : 하나의 어절에서 가능한 형태소 열을 갖고 있는        *
 *            Trellis에 대해서 Viterbi Algorithm을 수행하여       *
 *	      best path를 구한다.                                 *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void ViterbiForwd()
{
  Trellis *trellisPtr = &trellis ; /* Trellis를 따라가면서 확률을 계산 */ 
  Path    *befPath               ; /* 이전 Trellis의 가능한 Path       */
  Path    *nowPath               ; /* 현재 Trellis의 가능한 Path       */
  int senbef , sennow            ; 
  int befIdx , nowIdx            ;
  int numbef , numnow            ; 
  int nowTag                     ; 
  int maxIdx = -1                ; /* Back Pointer                     */
  double score                   ; /* 현재까지의 점수를 저장           */
  double maxscore = -1.0         ; /* max score = -Infinitive          */

  maxNumPath = 1 ;        /* 문장의 가능한 path 갯수를 계산하기 위해서 */ 

  while(trellisPtr != curTrls) { 
	
    befPath = trellisPtr->pathPool   ; /* 한 어절의 가능한 Path        */ 
    numbef = trellisPtr->numPath     ;

    trellisPtr = trellisPtr->nextPtr ; /* 처음에는 Header Trellis 다음 */	
    nowPath = trellisPtr->pathPool   ; 
    numnow = trellisPtr->numPath     ; 

    for(nowIdx = 0 ; nowIdx < numnow ; ++nowIdx) {
      maxscore = -1.0 ; 
      sennow = nowPath[nowIdx].path[0] ;  /* 다음 어절의 첫 형태소     */
      nowTag = senP[sennow].pos - '0' ;   /* 다음 어절의 첫 형태소 Tag */
      for(befIdx = 0 ; befIdx < numbef ; ++befIdx) { 
      senbef = befPath[befIdx].path[befPath[befIdx].pathlen] ; 
                                          /* 이전 어절의 마지막 형태소 */
      score = befPath[befIdx].maxprob *
              tranprob[senP[senbef].pos-'0'][nowTag] ; 

        if (score > maxscore) {
	  maxscore = score ; 
	  maxIdx = befIdx ; 
        }
      }
      nowPath[nowIdx].maxprob = maxscore * nowPath[nowIdx].lexprob ; 
      nowPath[nowIdx].backIdx = maxIdx ; /* For Viterbi */
    }
    maxNumPath *= numnow ; /* 한 문장에서 가능한 path 갯수 계산 */ 
    if (maxNumPath > 1000) maxNumPath = 1000 ; 
  }

}/*---------------End of ViterbiForwd---------------*/


/*----------------------------------------------------------------*
 *                                                                *
 * ViterbiBckwd  : Viterbi-Algorithm Backward                     *
 *                                                                *
 * Input    : NULL                                                *
 * Output   : NULL                                                *
 * Function : 하나의 어절에서 가능한 형태소 열을 갖고 있는        *
 *            Trellis에 대해서 Backtrack을 해서                   * 
 *	      best path를 구한다.                                 *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void ViterbiBckwd(path)
int path[] ; 
{
  Trellis *trellisPtr = curTrls ; /* 문장의 맨 마지막 어절 Trellis */
  int bestpath[__NUMEOJEOLINSENT__] ; /* BackTracking */
  int pathlen = 0 ; 
  int maxIdx  ;
  double maxScore = -1.0 ; 
  double score  ; 
  int idx ; 

  if (curTrls == &trellis) { /* 문장에 형태소가 하나도 없다 */
	path[0] = -1 ; 
	return ; 
  }

  for (idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
    score = trellisPtr->pathPool[idx].maxprob ; 
    if (score > maxScore) {
	  maxIdx = idx ; 
	  maxScore = score ; 
    }
  }

  /* maxIdx : 최적 품사열의 마지막 어절 Index */
  
  bestpath[pathlen]   = maxIdx ;     /* bestpath[0] = maxIdx */ 
  bestpath[++pathlen] = trellisPtr->pathPool[maxIdx].backIdx ; 

  while((trellisPtr = trellisPtr->backPtr) != &trellis) {
    bestpath[pathlen+1] = trellisPtr->pathPool[bestpath[pathlen]].backIdx ; 
    ++pathlen ; 
  }

  /* while문 이후 trellisPtr == &trellis 이다                    */
  /* bestpath[0] .. bestpath[pathlen - 1]에 Index가 들어가 있다  */
  /* 다시 문장 앞에서 뒤로 bestpath[..]에 해당하는 것을 출력한다 */  

  for(idx = pathlen - 1 ; idx >= 0 ; --idx)
    path[pathlen - 1 - idx] = bestpath[idx] ; 
  path[pathlen - 1 - idx] = -1 ;                  /* End of List */

}/*--------End of ViterbiBckwd---------*/


/*----------------------------------------------------------------*
 *                                                                *
 * AStar    : A*-Like Algorithm : Tree-Trellis Algorithm          *
 *                                                                *
 * Input    : opt = N : N-Best Candidates                         *
 * Output   : NULL                                                *
 * Function : Open-Node에 해당하는 Work-Area를 만들고 ,           *
 *            N-Best Candidates를 저장할 OutPut-Area를 만든다.    *
 *            A*-Like Algorithm으로 N-Best Candidates 구함.       * 
 *            Linked-List를 이용하여 N-Best 저장.                 * 
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void AStar(nbest)
int nbest ; 
{
  Trellis *trellisPtr = curTrls ; /* 문장의 맨 마지막 어절 Trellis */
  Path    path1 , path2 ; 
  PathRep onePath ; 
  PathRep tmpPath ; 
  double curProb  ; 
  int counter = 0 ;
  int senIdx1 , senIdx2 ; 
  int pathIdx   ; 
  int nowTag    ; 
  int idx       ; 

  double bestpathprob = 0.0 ; 
  double currpathprob = 0.0 ; 

  MakeSetEmpty(&pathOpen)   ; /* Open Node Set을 Clear   */
  MakeSetEmpty(&pathNBest)  ; /* N-Best Node Set을 Clear */ 

  pathNBest.nextPtr = pathOpen.nextPtr = NULL ; 
			   /* Open Node Set = Output Set = NULL */ 

  if (nbest > maxNumPath) nbest = maxNumPath ; /* 가능한 path 갯수를 넘을때 */

  if (curTrls == &trellis) return ;  /* 문장에 형태소가 하나도 없다 */

  onePath.epathlen = 0 ; 
  onePath.curTrPtr = trellisPtr ; 
  for (idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
   onePath.epath[0] = idx ; 
   onePath.fprob = trellisPtr->pathPool[idx].maxprob ; 
   onePath.gprob = 1.0 ;     /* HMM에서 Beta Computation하듯이 */
   AppendNode(&pathOpen,onePath) ; /* Open Node Set의 초기화를 한다  */  

#ifdef DEBUG2
   printf("******* Open-Node Set : Initialization *******\n") ; 
   DisplayPathRep(&pathOpen) ; 
#endif
  }

  while (counter < nbest && 
    currpathprob >= pathThreshold * bestpathprob ) { 
				      /* Output Set의 크기가 nbest가 아닐 동안 */
   tmpPath = GetOneNode(&pathOpen) ;  /* Open Node Set에서 maximum을 꺼낸다 */
   pathIdx = tmpPath.epath[tmpPath.epathlen] ; 
   path2 = tmpPath.curTrPtr->pathPool[pathIdx] ; 
   senIdx2 = path2.path[0] ;                         /* senP[]의 index */
   nowTag = senP[senIdx2].pos - '0' ; 

   tmpPath.curTrPtr = tmpPath.curTrPtr->backPtr ; 
   if (tmpPath.curTrPtr == &trellis) {
     tmpPath.fprob = tmpPath.gprob = tmpPath.gprob * 
                     path2.lexprob * tranprob[senP[0].pos-'0'][nowTag] ; 
     AppendNode(&pathNBest,tmpPath) ; 
     currpathprob = tmpPath.fprob ;   
     /* printf("Current counter=%d\n", counter); */
     if (++counter == 1) bestpathprob = tmpPath.fprob ; /* best-path prob를 할당한다. */

   } else {
    ++tmpPath.epathlen ; 
    curProb = tmpPath.gprob * path2.lexprob ; 
    for (idx = 0 ; idx < tmpPath.curTrPtr->numPath ; ++idx) {
     path1 = tmpPath.curTrPtr->pathPool[idx] ; 
     senIdx1 = path1.path[path1.pathlen]   ;          /* senP[]의 index */ 
     tmpPath.epath[tmpPath.epathlen] = idx ;
     tmpPath.gprob = curProb * tranprob[senP[senIdx1].pos-'0'][nowTag] ;
                                                      /* edge */ 
     tmpPath.fprob = tmpPath.gprob * path1.maxprob ;  /* f(n)=g(n)+h(n) */ 
     AppendNode(&pathOpen,tmpPath) ; 

#ifdef DEBUG2
     printf("******* Open-Node Set *******\n") ; 
     DisplayPathRep(&pathOpen) ; 
#endif

   }/*-end of for-*/
  }/*-end of else-*/
 }/*-end of while-*/

 /* 
  * while문을 나오면 아래 condition에 해당하는 것이 하나 더 추가된다.
  * 그러므로 pathNBest node list에서 마지막 하나를 제거한다. 
  */

 if (currpathprob < pathThreshold * bestpathprob ) RemoveTailNode(&pathNBest) ;  

#ifdef DEBUG2
  printf("******* N-Best Set *******\n") ; 
  DisplayPathRep(&pathNBest) ; 
#endif

}/*------------End of AStar-------------*/


/*---------------------------------------------------------------*
 *                                                               *
 * AlphaCmpttn : State-Based Tagging을 하기 위해 필요한          *
 *               Alpha-Computation을 한다.                       *
 *                                                               *
 *               HMM에서 사용하는 Forward Algorithm을 이용하여   *
 *		 전체 Trellis를 모두 계산한다.                   *
 *                                                               *
 *               Prob(O|lamda) 즉, 모델이 주어지고 전체          *
 *               Observation이 나올 확률을 구한다                *
 *                                                               *
 *---------------------------------------------------------------*/

PUBLIC void AlphaCmpttn()
{
  Trellis *trellisPtr = &trellis ; /* Trellis를 따라가면서 확률을 계산 */ 
  Path    *befPath               ; /* 이전 Trellis의 가능한 Path       */
  Path    *nowPath               ; /* 현재 Trellis의 가능한 Path       */
  int senbef , sennow            ; 
  int befIdx , nowIdx            ;
  int numbef , numnow            ; 
  int nowTag                     ; 
  double alpha                   ; /* Forward Computation              */ 

  while(trellisPtr != curTrls) { 
	
    befPath = trellisPtr->pathPool   ; /* 한 어절의 가능한 Path        */ 
    numbef = trellisPtr->numPath     ;

    trellisPtr = trellisPtr->nextPtr ; /* 처음에는 Header Trellis 다음 */	
    nowPath = trellisPtr->pathPool   ; 
    numnow = trellisPtr->numPath     ; 

    for(nowIdx = 0 ; nowIdx < numnow ; ++nowIdx) {
      sennow = nowPath[nowIdx].path[0] ;  /* 다음 어절의 첫 형태소     */
      nowTag = senP[sennow].pos - '0' ;   /* 다음 어절의 첫 형태소 Tag */
      alpha = 0.0 ; 
      for(befIdx = 0 ; befIdx < numbef ; ++befIdx) { 
        senbef = befPath[befIdx].path[befPath[befIdx].pathlen] ; 
			              /* 이전 어절의 마지막 형태소 */
	alpha += (befPath[befIdx].alphaprob *
		  tranprob[senP[senbef].pos-'0'][nowTag]) ; 
      }
      nowPath[nowIdx].alphaprob = alpha * nowPath[nowIdx].lexprob ; 
    }
  }

  /* 
   * P(O|lamda) = Sum of Alpha_T(i) 
   */

  alphatotal = 0.0 ; 
  nowPath = curTrls->pathPool ; 
  numnow = curTrls->numPath ; 
  for(nowIdx = 0 ; nowIdx < numnow ; ++nowIdx) 
     alphatotal += nowPath[nowIdx].alphaprob ; 

#ifdef ANALYSIS
  totProb += log(alphatotal)/LogE2 ; 
#endif

}/*---------End of AlphaCmpttn---------*/


/*---------------------------------------------------------------*
 *                                                               *
 * BetaCmpttn  : State-Based Tagging을 하기 위해 필요한          *
 *               Beta-Computation을 한다.                        *
 *                                                               *
 *               HMM에서 사용하는 Backward Algorithm을 이용하여  *
 *               전체 Trellis를 모두 계산한다.                   *
 *                                                               *
 *               gamma prob를 계산하고 gamma prob를 Sorting한다  * 
 *                                                               *
 *---------------------------------------------------------------*/

PUBLIC void BetaCmpttn()
{
  Trellis *trellisPtr = curTrls ; /* Trellis를 따라가면서 확률을 계산 */ 
  Path    *aftPath              ; /* 이전 Trellis의 가능한 Path       */
  Path    *nowPath              ; /* 현재 Trellis의 가능한 Path       */
  int senaft , sennow ; 
  int aftIdx , nowIdx ;
  int numaft , numnow ; 
  int nowTag          ; 
  int tmpIdx          ; 
  double beta         ; /* Backward Computation             */ 

  if (trellisPtr == &trellis) return ; /* 아무런 문장도 넣지 않았다 */

  aftPath = trellisPtr->pathPool ; 
  numaft = trellisPtr->numPath   ; 
  for(aftIdx = 0  ; aftIdx < numaft ; ++aftIdx) {
     aftPath[aftIdx].betaprob = 1.0 ; /* Beta_t(i) = 1.0 */
     aftPath[aftIdx].gammaprob = aftPath[aftIdx].alphaprob / alphatotal ; 
  }
  for(tmpIdx = 0 ; tmpIdx < numaft ; ++tmpIdx) {
     tmpProb[tmpIdx] = aftPath[tmpIdx].gammaprob ; 
     trellisPtr->sortedIdx[tmpIdx] = tmpIdx ; 
  } 
  qsort(trellisPtr->sortedIdx,trellisPtr->numPath,sizeof(int),idxcmp) ; 

  while(trellisPtr != trellis.nextPtr) { 
	
    aftPath = trellisPtr->pathPool   ; /* 한 어절의 가능한 Path */ 
    numaft = trellisPtr->numPath     ;

    trellisPtr = trellisPtr->backPtr ;
    nowPath = trellisPtr->pathPool   ; 
    numnow = trellisPtr->numPath     ; 

    for(nowIdx = 0 ; nowIdx < numnow ; ++nowIdx) {
     sennow = nowPath[nowIdx].path[nowPath[nowIdx].pathlen] ; 
	                               /* 현재 어절의 마지막  형태소    */
     nowTag = senP[sennow].pos - '0' ; /* 현재 어절의 마지막 형태소 Tag */
     beta = 0.0 ; 
     for(aftIdx = 0 ; aftIdx < numaft ; ++aftIdx) { 
        senaft = aftPath[aftIdx].path[0] ; /* 다음 어절의 첫 형태소     */
        beta += (aftPath[aftIdx].betaprob *
		 aftPath[aftIdx].lexprob  *
		 tranprob[nowTag][senP[senaft].pos-'0']) ; 
      }

      nowPath[nowIdx].betaprob = beta ; 
      nowPath[nowIdx].gammaprob = 
	   ( nowPath[nowIdx].alphaprob * beta ) / alphatotal ; 

    }/*-end of for-*/

    /* 
     * Sort gammaprobs into trellisPtr->sortedIdx
     */

    for(tmpIdx = 0 ; tmpIdx < numnow ; ++tmpIdx) {
    	tmpProb[tmpIdx] = nowPath[tmpIdx].gammaprob ; 
    	trellisPtr->sortedIdx[tmpIdx] = tmpIdx ; 
    } 
    qsort(trellisPtr->sortedIdx,trellisPtr->numPath,sizeof(int),idxcmp) ; 

  }/*-end of while-*/

}/*---------End of BetaCmpttn---------*/

/*-------------------------------------*
 * idxcmp : compare function for qsort *
 *-------------------------------------*/

int idxcmp(i,j)
int *i , *j ; 
{
   if(tmpProb[*j] - tmpProb[*i] > 0.0) return 1 ; 
   else if(tmpProb[*j] - tmpProb[*i] < 0.0) return -1 ; 
   return 0 ; 
}/*-End of idxcmp-*/


/*----------------------------------------------------------------*
 *                                                                *
 * MakeTrellis : 문장을 어절 단위로 만든다.                       *
 *                                                                *
 * Input    : NULL                                                *
 * Output   : NULL                                                *
 * Function : 문장을 형태소 Trellis구조로 만듦                    *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void MakeTrellis()
{
   int now_ptr = 0 ; 
   int bef_ptr = 0 ;

   curTrls = &trellis ;      /* Header Trellis('INI')를 가리킨다. */
   curTrls->nextPtr = NULL ; /* NULL로 초기화 */ 

   while (buffer_tag[now_ptr] != _EOL_) {

	 /* 어절 시작을 찾는다 */
     for (; buffer_tag[now_ptr] == _SPA_ ; ++now_ptr) ; 
     if (buffer_tag[now_ptr] == _EOL_) break ; /* 만약 space..\n이면 */
     bef_ptr = now_ptr ; 
      
     /* 어절 끝을 찾는다 */

     for (++now_ptr ; buffer_tag[now_ptr] != _SPA_ &&
		  buffer_tag[now_ptr] != _EOL_ ; ++now_ptr) ; 

     MakeTrellis2(bef_ptr,now_ptr) ; 

   }

}/*--------End of PutSent------------*/


/*------------------------------------------------------------------*
 *                                                                  *
 * MakeTrellis2 : 실제적으로 어절을 입력으로 받아 형태소 분석을     * 
 *                하고 그것을 가지고 trellis를 만들어 나간다.       *
 *                                                                  *
 * Input    : bef,now                                               *
 * Output   : NULL                                                  *
 * Function : 한글일 때 형태소 분석을 해서 그 결과를 senP에 넣는다. *
 *            이 때 index를 조절한다.                               *
 *                                                                  *
 *------------------------------------------------------------------*/

PRIVATE void MakeTrellis2(bef,now)
int bef ;                              /* 어절 시작 부분 */
int now ;                              /* 어절 끝   부분 */
{
  int tmp_value          ; 
  UCHAR tags[140]        ; /* tag를 저장 POS가 아니다       */
  UCHAR str[140]         ; /* 한 어절                       */
  UCHAR substr[140]      ; /* 어절 안의 동일 tag로 된 부분  */ 
  UCHAR kimmo[140]       ; /* 한글일 경우 이성진코드로 바꿈 */
  SINT indices[100]      ; /* Morph Pool의 바뀐 index       */
  SINT indShift[100][50] ; 
  char ender[100]        ; /* 어절의 마지막인가 ?           */
  SINT curr_ind          ;   
  SINT new_ind           ; 
  int ind_top            ; 
  int save_sen_ptr       ;
  int tmp , tmp2         ; 
  int now_ptr = 0        ; 
  int bef_ptr = 0        ; 
  int end_ptr            ;
  char temp_tag          ; 
  int known = 1          ; /* Default : Known Word          */

  save_sen_ptr = senPtr ; 
  end_ptr = now - bef ; 

  MorphCopy(str,&buffer[bef],end_ptr) ; 
  MorphCopy(tags,&buffer_tag[bef],end_ptr) ; 

  tags[end_ptr] = '^' ; /* while문 안의 tags[now_ptr] != temp_tag를 위해*/ 
  temp_tag = tags[0]  ; /* 제일 처음 시작 tag를 저장 */

  while(now_ptr < end_ptr) {

     /* 동일 Tag로 된 부분을 찾는다 */
     for (; tags[now_ptr] == temp_tag ; ++now_ptr) ; 
     /* 동일 Tag로 된 부분은 str[bef_ptr .. now_ptr-1]이다 */
	 
      if (temp_tag != _HAN_) { 

      senP[senPtr].end_mark    = 'x' ; /* 어절이라는 것 자체가 다음에 
       			        	 space가 있다는 것을 뜻한다 */
      senP[senPtr].sent_begin  = bef + bef_ptr ; 
      senP[senPtr].sent_end    = bef + now_ptr - 1 ;
      senP[senPtr].morph_begin = 0 ;
      senP[senPtr].morph_end   = 0 ; 
      senP[senPtr].prob = (double) 1.0 ; /* Prob(Word|Tag),P(T|W) */
      MorphCopy(senP[senPtr].word,&str[bef_ptr],now_ptr - bef_ptr); 
      senP[senPtr].pos = GetPos(temp_tag) ;
      senP[senPtr].nidx[0] = senPtr + 1   ;
      senP[senPtr].nidx[1]     = -1       ;
      ++senPtr ; 

     } else {
      
      MorphCopy(substr,&str[bef_ptr],now_ptr - bef_ptr) ; 
      if (!ks2kimmo(substr,kimmo)) {
    	fprintf(stderr,"Error : 한글코드가 이상합니다.\n") ; 
    	fprintf(stderr,"%s\n",buffer) ; 
      }
      known = PreMorphAn(kimmo) ; 
						
      /* morPtr : morphP의 Top을 가리킨다 */
      ind_top = CollectIndices(indices) ; /* 0 .. ind_top 까지 들어가 있다 */
      if (ind_top == -1) break  ; /* Error in Morph - Analysis */
      /* 0 .. ind_top 까지 index가 있다 (ind_top : 0) */

      for(tmp = 0 ; tmp < ind_top ; ++tmp) {
        curr_ind = indices[tmp] ; 
    	for(tmp2 = 0 ; morphP[curr_ind].nidx[tmp2] != -1 ; ++tmp2) 
    	  indShift[tmp][tmp2] = Index(indices,morphP[curr_ind].nidx[tmp2]) ; 
        ender[tmp] = morphP[curr_ind].end_mark ; 
        indShift[tmp][tmp2] = -1 ;  
      } 

      /* senPtr : 앞으로 넣을 곳 */

      for (tmp = 0 ; tmp < ind_top ; ++tmp) {  /* ind_top : 0이므로 제외 */
    	curr_ind = indices[tmp] ;  
    	new_ind = senPtr + tmp ; 
    	senP[new_ind].end_mark = ender[tmp] ; 
    	senP[new_ind].sent_begin = bef + bef_ptr ; 
    	senP[new_ind].sent_end = bef + now_ptr - 1 ;
        senP[new_ind].morph_begin = morphP[curr_ind].left ; 
        senP[new_ind].morph_end = morphP[curr_ind].right ; 
    	senP[new_ind].pos = morphP[curr_ind].pos ; 
        strcpy(senP[new_ind].word,morphP[curr_ind].word) ; 
    	senP[new_ind].prob = morphP[curr_ind].prob ; /* Prob(Word|Tag) */

    	for(tmp2 = 0 ; indShift[tmp][tmp2] != -1 ; ++tmp2)
    	  senP[new_ind].nidx[tmp2] = senPtr + indShift[tmp][tmp2] ;  
        senP[new_ind].nidx[tmp2] = -1 ; 
     
        /* index 가 0 일때 자동적으로 다음것을 가리치게 된다 */
     }
      /* 만약 앞에 있는 것이 한글이 아니고 지금 한글일 경우 :
         예 : english(영어) : '(' 다음 '영어'는 시작 형태소가 
         하나 이상이므로 '('가 시작 형태소를 가리키게 해야한다 */

      if (senP[senPtr-1].pos < _NCT_) {  /* 바로 위에 한글이 아니다 */
       for(tmp2 = 0 ; morphP[morPtr].nidx[tmp2] != -1 ; ++tmp2)
        senP[senPtr-1].nidx[tmp2] =
              senPtr + Index(indices,morphP[morPtr].nidx[tmp2]) ;
        senP[senPtr-1].nidx[tmp2] = -1 ;
      }

      senPtr += ind_top ; /* sentence Pool Top Pointer 수정 */
     }/*--end of else--*/

     bef_ptr  = now_ptr ; 
     temp_tag = tags[now_ptr] ; 
  }
 
  tmp_value = AddTrellis(str,save_sen_ptr,senPtr-1,known) ;
	                      /* UnknownWordModel 3   : Unk */ 
#ifdef ANALYSIS
  if (known) {
    ++numkneojeol            ; 
    totknambi += tmp_value   ; 
  } else {
    ++numunkneojeol          ; 
    totunknambi += tmp_value ;
  }
#endif 

}/*----------MakeTrellis2-------------*/


/*-------------------------------------------------------------------*
 *                                                                   *
 * index    : Function(x, {(value , index)} )                        *
 *           returns index of (x == value)                           *
 *                                                                   *
 * Input    : indices , ind                                          *
 * Output   : index                                                  *
 * Function : ind가 indices안에 있는 value와 동일할 때 그 index를    *
 *	      return	                                             *
 *                                                                   *
 *-------------------------------------------------------------------*/

PRIVATE SINT Index(indices,ind)
SINT indices[] ; 
SINT ind ; 
{
  short i ; 

  for(i = 0 ; indices[i] != 0 ; ++i)
	if (indices[i] == ind) return i ; 
  return i ; 
}/*-------End of Index--------*/


/*----------------------------------------------------------------*
 *                                                                *
 * CollectIndices : Collect usable morphP[].nidx[]                *
 *                                                                *
 * Input    : indices                                             *
 * Output   : total number of indices - 1                         *
 * Function : morphP[]에서 실제 의미 있는 record의 index를 모으고 *
 *            sorting한다.                                        *
 *                                                                *
 *----------------------------------------------------------------*/

PRIVATE int CollectIndices(indices)
SINT indices[] ; 
{
  int numind1 ; 
  int begin_ptr = -1 ; 
  int end_ptr = -1 ; 
  SINT morphIdx  ; 

  /* INI가 가르치는 index로 초기화 */

  for (numind1 = 0 ; morphP[morPtr].nidx[numind1] != -1 ; 
					 ++numind1)
    PutIdx(indices,&end_ptr,morphP[morPtr].nidx[numind1]) ;  
	
  if (end_ptr == -1) return -1 ; /* Error in Morph - Analysis */

  while (begin_ptr != end_ptr) {

    ++begin_ptr ; /* 제일 처음에는 0 */
    morphIdx = indices[begin_ptr] ; 

    for (numind1 = 0 ; morphP[morphIdx].nidx[numind1] != -1 ; 
					 ++numind1)
      PutIdx(indices,&end_ptr,morphP[morphIdx].nidx[numind1]) ; 
  }

  qsort(indices,end_ptr+1,sizeof(SINT),cmp) ; 

#ifdef DEBUG2

  { int i ; 
	for(i = 0 ; i <= end_ptr ; ++i) 
     printf("%d ", indices[i]) ; 
    putchar('\n') ; 
  }

#endif

  return end_ptr ; /* 0 .. end_ptr까지 값이 있다 */

}/*------------End of CollectIndices------------*/

/*--------------------------------------------------------*
 * cmp : compare the SINT - type value (descending order) *
 *--------------------------------------------------------*/

int cmp(i,j)
SINT *i,*j ; 
{
  return (int) *j - *i ; 
}


/*--------------------------------------------------------*
 *                                                        *
 * PutIdx   : Put Index into indices-array                *
 *                                                        *
 * Input    : indices , top-pointer , data                *
 * Output   : NULL                                        *
 * Function : indices[]에 data를 넣는다.                  *
 *	      이미 data가 indices[]에 있으면 넣지 않는다. *
 *                                                        *
 *--------------------------------------------------------*/

PRIVATE void PutIdx(indices , top , value)
SINT indices[] ; 
int *top ; 
SINT value ; 
{
  int tmp ; 

  for(tmp = 0 ; tmp <= *top ; ++tmp)
    if(indices[tmp] == value) return ; 

  *top = tmp ; 
  indices[*top] = value ; 
}/*----------End of PutIdx------*/


/*---------------------------*
 *                           *
 * MorphCopy : Morpheme Copy *
 *                           *
 *---------------------------*/

PRIVATE void MorphCopy(to,from,len)
UCHAR to[]   ; 
UCHAR from[] ; 
int len ; 
{
  int i ; 

  for (i=0 ; i < len ; ++i)
	to[i] = from[i] ; 
  to[i] = '\0' ; 
}/*------------End of MorphCopy----------*/

/*-------------------------------------------------*
 *                                                 *
 * InitTrellis : 하나의 문장은 Trellis로 구성된다. *
 *               Trellis *trellis는 문장의 처음을  *
 *	         뜻하기 위해 Header를 갖는다.      *
 *                                                 *
 *-------------------------------------------------*/

PUBLIC void InitTrellis()
{
  Sentence oneSent    ; 

  trellis.backPtr  = trellis.nextPtr = '\0' ; 
  trellis.startIdx = trellis.endIdx  = 0 ; 
  trellis.numPath = 1 ; 
  trellis.sortedIdx = (int *) malloc(sizeof(int)) ; /* Dummy */ 
  trellis.pathPool = (Path *) malloc(sizeof(Path)) ; 
  trellis.pathPool->pathlen   = 0   ; /* path[pathlen] = path[0]  */ 
  trellis.pathPool->path[0]   = 0   ; /* senP[0].word  = 'INI'    */
  trellis.pathPool->lexprob   = 1.0 ; /* Prob("INI"|INI)   = 1.0  */
  trellis.pathPool->maxprob   = 1.0 ; /* Partial Path Prob = 1.0  */ 
  trellis.pathPool->backIdx   = -1  ; /* backIdx는 없다           */
  trellis.pathPool->alphaprob = 1.0 ; /* alpha computation을 위해 */
  trellis.pathPool->ox[0]     = 
  trellis.pathPool->ox[1]     = 
  trellis.pathPool->ox[2]     = UNMARK ; /* 선택되지 않는 부분    */

  /* one Sentence Data 구조 : INI */

  oneSent.end_mark = 'x'      ; 
  oneSent.sent_begin = 0      ;
  oneSent.sent_end = 0        ; 
  oneSent.morph_begin = 0     ; 
  oneSent.morph_end = 0       ; 
  strcpy(oneSent.word,"INI")  ; 
  oneSent.pos = _INITI_       ; 
  oneSent.prob = (double) 1.0 ; /* Prob("INI"|INI) = 1.0     */ 
  oneSent.nidx[0] = 1         ; /* 그 다음을 가리킴 : 1      */ 
  oneSent.nidx[1] = -1        ; /* 그 다음을 가리킴 : E of L */ 
  senP[0] = oneSent           ; /* trellis.pathPool->path[0] */
  senPtr = 1                  ; /* senP[0] = 'INI'           */ 

}/*-----End of InitTrellis-----*/


/*-------------------------------------------------------------*
 *                                                             *
 * DeleteTrellis : 하나의 문장은 Trellis로 구성된다.           *
 *		   다음 문장을 위해 Trellis를 Deallocate한다.  *
 *                                                             *
 *-------------------------------------------------------------*/

PUBLIC void DeleteTrellis()
{
  Trellis *tmpPtr = trellis.nextPtr ; /* Trellis를 따라가며 지운다 */
  Trellis *tmpPtr2 ; 

  while (tmpPtr != NULL) {
   tmpPtr2 = tmpPtr ; 
   tmpPtr = tmpPtr->nextPtr ; 

   free(tmpPtr2->pathPool)  ; /* PathPool 내용을 Deallocate     */
   free(tmpPtr2->sortedIdx) ; 
   free(tmpPtr2)            ; /* Trellis를 Deallocate           */
  }
  trellis.nextPtr = NULL    ; /* header의 nextPtr = NULL을 한다 */

}/*--End of DeleteTrellis--*/


/*--------------------------------------------------------*
 *                                                        *
 * AddTrellis : senP[]의 start Index와 end Index를        *
 *	          받아 새로운 Trellis를 만들어            *
 *	          저장                                    *
 *                                                        *
 * 첫번째 품사가 openClass[]에 있는 것이면                *
 * P(w_i|t_i) = P(Known|Tag) * P^(w_i|t_i)                *
 * P(w_i|t_i) = P(Unknown|Tag) * P^(e_i|t_i)              *
 *                                                        *
 * path[0 .. pathlen]에 내용이 들어가 있다                *
 *                                                        *
 * IsInOpenTag(UCHAR in_tag) : 1 .. __UNKNNOFOPENCLS__    *
 * 그래서 IsInOpenTag(in_tag) - 1 == openClass[]의 idx    *
 *                                                        *
 * unknownProb[0 .. 11][0] = Prob(Known|Open_Class_Tag)   *
 * unknownProb[0 .. 11][1] = Prob(Unknown|Open_Class_Tag) *
 *                                                        *
 * unknownPttrn[0 .. 11].numofpttrn = number of patterns  *
 *                                    0 .. numofpttrn - 1 *
 *                                                        *
 * unknownPttrn[0 .. 11].pttrnPtr[ 0 .. numofpttrn - 1]에 *
 * pattern prob와 pattern이 들어가 있다                   *
 *                                                        *
 * unknownPttrn[0 .. 11].pttrnPtr[X].pttrnProb = P(E_i|T) *
 * unknownPttrn[0 .. 11].pttrnPtr[X].pttrn[..] = E_i      *
 *   tag sequence가 들어가 있고 '\0'로 terminate된다      *  
 **********************************************************
 *                                                        *
 * Unkown Model 4 : Lexical Level의 Feature를 참조한다.   *
 *                                                        *
 *                                                        *
 *                                                        *
 *--------------------------------------------------------*/

PUBLIC int AddTrellis(word,sIdx,eIdx,known)
unsigned char word[] ;       /* 입력 어절               */ 
int sIdx  ;                  /* senP[]의 start Index    */
int eIdx  ;                  /* senP[]의   end Index    */
int known ;                  /* Known Word인가 아닌가 ? */ 
{
  int beg_ptr      ; 
  int idx          ; 
  int tmpIdx       ; 
  int tmpPLen      ; /* tmp Path Len        */
  int openCIdx     ; /* open class word Idx */
  FLAG flag        ; /* flag                */
  UCHAR first_tag                ; /* tag sequence에서 첫번째 tag          */
  UCHAR tagseq[__MAXPATHLEN__]   ; /* unknown word 발견시 나머지 tag seq   */
  Trellis *trellisPtr            ; /* Trellis를 만든다                     */
  Path tmpPath[__TEMPPATHSIZE__] ; /* Path를 저장 : Circular Queue         */
  Path resPath[__TEMPPATHSIZE__] ; /* 최종 결과 Path를 저장                */ 
  int path1 = 0 , path2 = 0      ; /* path1 .. path2 에 path가 저장        */
  int stackTop = __TEMPPATHSIZE__; /* Data is stackTop .. __TEMPPSIZE__ -1 */ 
  int nextMor ;                    /* 다음 형태소를 가리키는 것            */
  int senIdx  ;  /* senIdx = tmpPath[path.].path[tmpPath[path.].pathlen]   */
  int senIdx2 ;  /* senIdx2 = senP[senIdx].nidx[]                          */ 

  trellisPtr = (Trellis *) malloc(sizeof(Trellis)) ; 

  beg_ptr = senP[sIdx].sent_begin ;  
  for(idx = sIdx ; idx <= eIdx && senP[idx].sent_begin == beg_ptr ; ++idx)
    if (senP[idx].morph_begin == 0 && senP[idx].pos < _JC_ ) {
      tmpPath[path2].pathlen = 0   ; /* path[]에서 마지막 node는 0에 있다 */  
      tmpPath[path2].path[0] = idx ; /* path의 시작 : senP[idx]           */ 
      tmpPath[path2++].lexprob = senP[idx].prob ;
    }

     /*
      * 예를 들어 `가라앉히는 게'라는 문장이 있을 때,
      * `게'는 `이/jcp+게/ef' 혹은 `게/jca'로 분석이 된다.
      * 이 때, 모두 `어미,조사'로 시작해서 결국 모두 무시된다.
      * 이러한 경우, batch로 실행시킬 때, 프로그램이 fail되므로,
      * 적어도 하나는 남겨두어야 한다. (맨 마지막 것을 갖고 계산)
      */

    if (path2 == 0) {
      tmpPath[path2].pathlen = 0      ; /* path[]에서 마지막 node는 0에 있다 */  
      tmpPath[path2].path[0] = idx -1 ; /* path의 시작 : 맨 마지막 것을 copy */ 
      tmpPath[path2++].lexprob = senP[idx-1].prob ; 
    }

     /*
      * P(어절|Tag Seq) =   P(word_1|tag_1) * P(tag_2|tag_1) 
      *                   * P(word_2|tag_2) * P(tag_3|tag_2)
      *                   *      ....       * P(tag_n|tag_n-1)
      *                   * P(word_n|tag_n)
      *
      * word_1이 unknown-word가 될 수 있다
      * 일단 계산된 값은 P(word_1|tag_1)이 1.0이므로 전체 
      * path가 결정된 후 eojeol이 unknown-word인가 비교한 후 
      * known-word일 경우   : P(Known|Tag) * P(Word|Tag) 
      * Unknown-word일 경우 : P(Unknown|Tag) * P(Ending|Tag) 
      * 
      * P(Known|Tag) , P(Unknown|Tag)를 corpus에서 
      * 맨 앞에 있는 것만을 갖고 계산했다.
      *
      * Circular - Queue를 이용하여 network을 푼다.
      *
      * Network로부터 여러개의 Path를 만든다. 
      *
      */

    while( path1 != path2 ) { /* [path1 .. path2 -1] : 내용이 waiting queue */

     senIdx = tmpPath[path1].path[tmpPath[path1].pathlen] ; 

     if (senP[senIdx].nidx[0] > eIdx) { /* boundary를 넘었으니까 더 이상 할 필요 없음 */

       --stackTop ; 
       resPath[stackTop] = tmpPath[path1] ;  /* Stack내용을 넣는다 */

     } else {

      for(flag = OFF, nextMor = 0 ; (senIdx2 = senP[senIdx].nidx[nextMor]) != -1 ; 
			  ++nextMor) {

       if (senP[senIdx].pos < _NCT_ ) 
          if (tranTable[senP[senIdx].pos - '0'][senP[senIdx2].pos - '0'] == '0') continue ;  

       tmpPath[path2] = tmpPath[path1] ; 
       ++(tmpPath[path2].pathlen) ; 
       tmpPath[path2].path[tmpPath[path2].pathlen] = senIdx2 ; 
       tmpPath[path2].lexprob *= tranprob[senP[senIdx].pos - '0'][senP[senIdx2].pos - '0']
                                 * senP[senIdx2].prob ; 
       flag = ON ; /* path에서 다음 형태소를 한번은 만들었다는 뜻 */

       /* lexprob = lexprob * P(t_n|t_n-1) * P(w|t_n) */ 

       if (++path2 == __TEMPPATHSIZE__) path2 = 0 ;
       if (path2 == path1) {

         /*
          *  fprintf(stderr,"Out of Bound : Path-Making-Area\n") ;
          *  fprintf(stderr,"__TEMPPATHSIZE__ needs to be bigger than now\n") ;
	  *  fprintf(stderr,"Input is %s\n",buffer) ; 
          *  exit(1) ;
          */

         /*
          *  Circular Queue인 path[path2=path1 .. path1]에 Partial 결과가 들어가 있다.
          *
          *  이 때는 Path를 만드는 것을 중지하고 다음 것을 만든다. 
          */ 

          break ;  

       }

      } /* end of for-loop */

      if (flag == OFF) { /* 한번도 expansion하지 못했다면.. 억지로라도 만든다. */
         senIdx2 = senP[senIdx].nidx[nextMor-1] ; /* 맨 마지막 것을 갖고 한다. */
         tmpPath[path2] = tmpPath[path1] ; 
         ++(tmpPath[path2].pathlen) ; 
         tmpPath[path2].path[tmpPath[path2].pathlen] = senIdx2 ; 
         tmpPath[path2].lexprob *= tranprob[senP[senIdx].pos - '0'][senP[senIdx2].pos - '0']
                                 * senP[senIdx2].prob ; 
         if (++path2 == __TEMPPATHSIZE__) path2 = 0 ;
      }        

    } /* end of else */

    if(++path1 == __TEMPPATHSIZE__) path1 = 0 ; 

   }/*--end of while--*/

   /* tmpPath[stackTop .. __TEMPPATHSIZE__ - 1]에 결과 저장되어 있다 */ 

   trellisPtr->pathPool = 
       (Path *) malloc(sizeof(Path) * ( __TEMPPATHSIZE__ - stackTop)) ; 

   memcpy(trellisPtr->pathPool,&resPath[stackTop],
		  sizeof(Path) * ( __TEMPPATHSIZE__ - stackTop)) ; 
   strcpy(trellisPtr->eojeol,word) ; 
   trellisPtr->startIdx = sIdx     ;  /* 필요할까 ? */
   trellisPtr->endIdx   = eIdx     ;  /* 필요할까 ? */
   trellisPtr->numPath  = __TEMPPATHSIZE__ - stackTop ; 
   trellisPtr->sortedIdx = (int *) malloc(sizeof(int) * trellisPtr->numPath) ; 
   trellisPtr->known = (known == 1) ? KN_EOJ : UN_EOJ ;  

   for(idx = 0 ; idx < trellisPtr->numPath ; ++idx)
     trellisPtr->pathPool[idx].ox[0] = 
     trellisPtr->pathPool[idx].ox[1] = 
     trellisPtr->pathPool[idx].ox[2] = UNMARK ;        /* initialization  */


#ifdef UNKNOWNMODEL3

   if (!known) {                                 /* if Unknown-Word */

    for(idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
     tmpPLen = trellisPtr->pathPool[idx].pathlen ;     /* path len을 저장 */
     first_tag = senP[trellisPtr->pathPool[idx].path[0]].pos ; /* 첫 품사 */ 
     tagseq[0] = '\0' ;                             /* Terminated by NULL */

     if (tmpPLen > 0) {
       tmpIdx = trellisPtr->pathPool[idx].path[1] ; 
       if (senP[tmpIdx].pos < _NCT_) strcpy(tagseq,senP[tmpIdx].word) ; 
       else  kimmo2ks(senP[tmpIdx].word,tagseq) ;  /* 완성형 한글로 바꿔서 비교 */
       strcat(tagseq,"/") ; 
       strcat(tagseq,posTags[senP[tmpIdx].pos-'0']) ; 
     }

#ifdef DEBUG3
     printf("lex - feature is %s\n",tagseq) ; 
#endif

     openCIdx = IsInOpenTag(first_tag) - 1 ; 

     /*
      * first_tag의 Open Class Tag의 Index를 구한다 :
      * unknown word이므로 openCIdx는 0 .. __NUMOFOPENCLASS__사이에 
      * 있어야 되나 , 앞의 것이 _NCT_이하의 것이면 (즉 심벌 등...)
      * openCIdx는 -1이 될 것이다. ( 예: TRI-팍스 )
      */

     /*
      * ~CASE1 : * Prob(t_i|w_i+1,t_i+1) 논문에서 사용한 것
      *  CASE1 : * Prob(w_i+1,t_i+1|t_i)을 사용할 수도 있다.
      *          하지만, 실험적으로 ~CASE1이 더 좋다.
      */

#ifndef CASE1

     if (openCIdx != -1)
         trellisPtr->pathPool[idx].lexprob *= unknownProb[openCIdx][1]
                    * ProbInvLexFtr(openCIdx,tagseq) ; 

#else

     if (openCIdx != -1)
         trellisPtr->pathPool[idx].lexprob *= unknownProb[openCIdx][1]
                 * ProbLexFtr(openCIdx,tagseq) ; 

#endif


     /* P(W_i|T_i) = P(Unknown|T_i) * P(T_i|Lexical-Feature) */ 

    }/*--end of for--*/

   } else {          /* if Known-Word */

     for(idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
       first_tag = senP[trellisPtr->pathPool[idx].path[0]].pos ;
       if((openCIdx = IsInOpenTag(first_tag)))
         trellisPtr->pathPool[idx].lexprob *= unknownProb[openCIdx - 1][0] ; 
     }

   }/*--end of else--*/

#endif

   curTrls->nextPtr = trellisPtr ; /* Trellis 연결                      */
   trellisPtr->backPtr = curTrls ; /* backPtr 연결 : Doubly Linked List */
   curTrls = curTrls->nextPtr    ; /* curTrls 조절                      */
   curTrls->nextPtr = NULL       ; /* 마지막 Trellis.nextPtr = NULL     */ 
   
   return trellisPtr->numPath ;  

}/*--End of AddTrellis--*/


/*****************************************************/
/****************End of ktstagger.c*******************/
/*****************************************************/


