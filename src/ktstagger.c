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

EXTERN int ks2kimmo()      ; /* �ѱ��� kimmo code�� �ٲ�             */ 
EXTERN int PreMorphAn()    ; /* ���¼� �ؼ� Module                   */
EXTERN UCHAR GetPos()      ; /* Get Part-Of-Speech                   */
EXTERN void AppendNode()   ; /* path-set�� path�� sorting�Ͽ� ����   */
EXTERN void RemoveTailNode() ; /* path-set���� ������ node�� ����    */
EXTERN void MakeSetEmtpy()   ; /* path-set�� clear                   */
EXTERN PathRep GetOneNode(); /* path-set���� path�� ��´�           */

PUBLIC void ViterbiForwd() ; /* Viterbi Algorithm Forward            */
PUBLIC void ViterbiBckwd() ; /* Viterbi Algorithm Backward           */
PUBLIC void AlphaCmpttn()  ; /* HMM : Alpha Computation & P(O|lamda) */
PUBLIC void BetaCmpttn()   ; /* HMM : Beta , Gamma Computation       */
PUBLIC void MakeTrellis()  ; 
PUBLIC void InitTrellis()  ; 
PUBLIC void DeleteTrellis(); 
PUBLIC void AStar()        ; /* A*-Like Algorithm : Tree-Trellis    */
PUBLIC int AddTrellis()    ; /* Trellis�� ÷���Ѵ�.                 */ 

#ifdef UNKNOWNMODEL3
EXTERN int IsInOpenTag()      ; /* Is Str In Open-Tag ?      */
EXTERN double ProbLexFtr()    ; /* Prob(Feature=Pattern|Tag) */
EXTERN double ProbInvLexFtr() ; /* Prob(Tag|Feature=Pattern) */
#endif

PRIVATE void MakeTrellis2()  ; /* �ѱ��� ó���ϱ� ���� routine */
PRIVATE void MorphCopy()     ;
PRIVATE void PutIdx()        ; 
PRIVATE SINT Index()         ; 
PRIVATE int CollectIndices() ; 
int cmp()                    ; 
int idxcmp()                 ; 

EXTERN UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /*ǰ�� ���� table*/
EXTERN Morph morphP[]  ; /* Morph Pool declared in ktsmoran.c */
EXTERN int morPtr      ; /* Morph Pool Top Ptr in ktsmoran.c  */ 

EXTERN UCHAR _INITI_ ; /* ����,������ �������� ���Ѵ� */
EXTERN UCHAR _NCT_   ; /* �ѱ� Tag Set�� ���� Tag     */
EXTERN UCHAR _JC_    ; /* ���, ���� ����             */

#ifdef ANALYSIS
EXTERN int numkneojeol      ; /* known ���� ����               */
EXTERN int numunkneojeol    ; /* unknown ���� ����             */
EXTERN int totunknambi      ; /* unknown word�� candidate ���� */
EXTERN int totknambi        ; /* known word�� candidate ����   */
EXTERN double totProb       ; /* log_2 P(w1..n)                */
EXTERN double LogE2         ; /* log_e 2                       */
#endif

#ifdef UNKNOWNMODEL3
PUBLIC double unknownProb[__NUMOFOPENCLASS__][3] ; /* Known-Word , Unknown-Word */
PUBLIC UnkPttrn unknownLexm[__NUMOFOPENCLASS__]  ; /* Prob(Ending_i|Tag)�� ���� */
#endif

EXTERN double tranprob[__NUMOFTAG__][__NUMOFTAG__]     ; /* ���¼� ���� Ȯ��   */

EXTERN double pathThreshold              ; /* N-Best�� ���ϱ� ���� Threshold   */
EXTERN int    tagfreq[__NUMOFTAG__]      ; /* ���¼� �߻� Frequency            */
EXTERN double tagprob[__NUMOFTAG__]      ; 
EXTERN UCHAR buffer[__BUFFERLENGTH__]    ; 
EXTERN char buffer_tag[__BUFFERLENGTH__] ; 

Sentence senP[__SENTLENGTH__]     ; 
int senPtr         ; /* Sentence Pool�� �� ���¼��� ����                   */
int maxNumPath =0  ; /* �Է� ���忡�� ������ Path�� �ִ� ������ �̸� ����Ѵ�  */
Trellis trellis    ; /* �ϳ��� ������ �ϳ��� Trellis�� �ִ´� : header trellis */
Trellis *curTrls   ; /* ���ݱ��� ���� Trellis�� �������� ����Ų��              */ 
PathRep pathNBest  ; /* N-Best Candidates Path�� �����ϴ� ��� : Linked-List   */
PathRep pathOpen   ; /* A*-Algorithm���� Open-Node���� �����ϴ� ���           */
double alphatotal  ; /* Sum of alphas in State-Based Tagging : P(O|model)      */
double tmpProb[__TEMPPROB__] ; /* gammaprob�� sorting�ϱ� ���� Array           */


/*----------------------------------------------------------------*
 *                                                                *
 * ViterbiForwd  : Viterbi-Algorithm Forward                      *
 *                                                                *
 * Input    : NULL                                                *
 * Output   : NULL                                                *
 * Function : �ϳ��� �������� ������ ���¼� ���� ���� �ִ�        *
 *            Trellis�� ���ؼ� Viterbi Algorithm�� �����Ͽ�       *
 *	      best path�� ���Ѵ�.                                 *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void ViterbiForwd()
{
  Trellis *trellisPtr = &trellis ; /* Trellis�� ���󰡸鼭 Ȯ���� ��� */ 
  Path    *befPath               ; /* ���� Trellis�� ������ Path       */
  Path    *nowPath               ; /* ���� Trellis�� ������ Path       */
  int senbef , sennow            ; 
  int befIdx , nowIdx            ;
  int numbef , numnow            ; 
  int nowTag                     ; 
  int maxIdx = -1                ; /* Back Pointer                     */
  double score                   ; /* ��������� ������ ����           */
  double maxscore = -1.0         ; /* max score = -Infinitive          */

  maxNumPath = 1 ;        /* ������ ������ path ������ ����ϱ� ���ؼ� */ 

  while(trellisPtr != curTrls) { 
	
    befPath = trellisPtr->pathPool   ; /* �� ������ ������ Path        */ 
    numbef = trellisPtr->numPath     ;

    trellisPtr = trellisPtr->nextPtr ; /* ó������ Header Trellis ���� */	
    nowPath = trellisPtr->pathPool   ; 
    numnow = trellisPtr->numPath     ; 

    for(nowIdx = 0 ; nowIdx < numnow ; ++nowIdx) {
      maxscore = -1.0 ; 
      sennow = nowPath[nowIdx].path[0] ;  /* ���� ������ ù ���¼�     */
      nowTag = senP[sennow].pos - '0' ;   /* ���� ������ ù ���¼� Tag */
      for(befIdx = 0 ; befIdx < numbef ; ++befIdx) { 
      senbef = befPath[befIdx].path[befPath[befIdx].pathlen] ; 
                                          /* ���� ������ ������ ���¼� */
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
    maxNumPath *= numnow ; /* �� ���忡�� ������ path ���� ��� */ 
    if (maxNumPath > 1000) maxNumPath = 1000 ; 
  }

}/*---------------End of ViterbiForwd---------------*/


/*----------------------------------------------------------------*
 *                                                                *
 * ViterbiBckwd  : Viterbi-Algorithm Backward                     *
 *                                                                *
 * Input    : NULL                                                *
 * Output   : NULL                                                *
 * Function : �ϳ��� �������� ������ ���¼� ���� ���� �ִ�        *
 *            Trellis�� ���ؼ� Backtrack�� �ؼ�                   * 
 *	      best path�� ���Ѵ�.                                 *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void ViterbiBckwd(path)
int path[] ; 
{
  Trellis *trellisPtr = curTrls ; /* ������ �� ������ ���� Trellis */
  int bestpath[__NUMEOJEOLINSENT__] ; /* BackTracking */
  int pathlen = 0 ; 
  int maxIdx  ;
  double maxScore = -1.0 ; 
  double score  ; 
  int idx ; 

  if (curTrls == &trellis) { /* ���忡 ���¼Ұ� �ϳ��� ���� */
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

  /* maxIdx : ���� ǰ�翭�� ������ ���� Index */
  
  bestpath[pathlen]   = maxIdx ;     /* bestpath[0] = maxIdx */ 
  bestpath[++pathlen] = trellisPtr->pathPool[maxIdx].backIdx ; 

  while((trellisPtr = trellisPtr->backPtr) != &trellis) {
    bestpath[pathlen+1] = trellisPtr->pathPool[bestpath[pathlen]].backIdx ; 
    ++pathlen ; 
  }

  /* while�� ���� trellisPtr == &trellis �̴�                    */
  /* bestpath[0] .. bestpath[pathlen - 1]�� Index�� �� �ִ�  */
  /* �ٽ� ���� �տ��� �ڷ� bestpath[..]�� �ش��ϴ� ���� ����Ѵ� */  

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
 * Function : Open-Node�� �ش��ϴ� Work-Area�� ����� ,           *
 *            N-Best Candidates�� ������ OutPut-Area�� �����.    *
 *            A*-Like Algorithm���� N-Best Candidates ����.       * 
 *            Linked-List�� �̿��Ͽ� N-Best ����.                 * 
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void AStar(nbest)
int nbest ; 
{
  Trellis *trellisPtr = curTrls ; /* ������ �� ������ ���� Trellis */
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

  MakeSetEmpty(&pathOpen)   ; /* Open Node Set�� Clear   */
  MakeSetEmpty(&pathNBest)  ; /* N-Best Node Set�� Clear */ 

  pathNBest.nextPtr = pathOpen.nextPtr = NULL ; 
			   /* Open Node Set = Output Set = NULL */ 

  if (nbest > maxNumPath) nbest = maxNumPath ; /* ������ path ������ ������ */

  if (curTrls == &trellis) return ;  /* ���忡 ���¼Ұ� �ϳ��� ���� */

  onePath.epathlen = 0 ; 
  onePath.curTrPtr = trellisPtr ; 
  for (idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
   onePath.epath[0] = idx ; 
   onePath.fprob = trellisPtr->pathPool[idx].maxprob ; 
   onePath.gprob = 1.0 ;     /* HMM���� Beta Computation�ϵ��� */
   AppendNode(&pathOpen,onePath) ; /* Open Node Set�� �ʱ�ȭ�� �Ѵ�  */  

#ifdef DEBUG2
   printf("******* Open-Node Set : Initialization *******\n") ; 
   DisplayPathRep(&pathOpen) ; 
#endif
  }

  while (counter < nbest && 
    currpathprob >= pathThreshold * bestpathprob ) { 
				      /* Output Set�� ũ�Ⱑ nbest�� �ƴ� ���� */
   tmpPath = GetOneNode(&pathOpen) ;  /* Open Node Set���� maximum�� ������ */
   pathIdx = tmpPath.epath[tmpPath.epathlen] ; 
   path2 = tmpPath.curTrPtr->pathPool[pathIdx] ; 
   senIdx2 = path2.path[0] ;                         /* senP[]�� index */
   nowTag = senP[senIdx2].pos - '0' ; 

   tmpPath.curTrPtr = tmpPath.curTrPtr->backPtr ; 
   if (tmpPath.curTrPtr == &trellis) {
     tmpPath.fprob = tmpPath.gprob = tmpPath.gprob * 
                     path2.lexprob * tranprob[senP[0].pos-'0'][nowTag] ; 
     AppendNode(&pathNBest,tmpPath) ; 
     currpathprob = tmpPath.fprob ;   
     /* printf("Current counter=%d\n", counter); */
     if (++counter == 1) bestpathprob = tmpPath.fprob ; /* best-path prob�� �Ҵ��Ѵ�. */

   } else {
    ++tmpPath.epathlen ; 
    curProb = tmpPath.gprob * path2.lexprob ; 
    for (idx = 0 ; idx < tmpPath.curTrPtr->numPath ; ++idx) {
     path1 = tmpPath.curTrPtr->pathPool[idx] ; 
     senIdx1 = path1.path[path1.pathlen]   ;          /* senP[]�� index */ 
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
  * while���� ������ �Ʒ� condition�� �ش��ϴ� ���� �ϳ� �� �߰��ȴ�.
  * �׷��Ƿ� pathNBest node list���� ������ �ϳ��� �����Ѵ�. 
  */

 if (currpathprob < pathThreshold * bestpathprob ) RemoveTailNode(&pathNBest) ;  

#ifdef DEBUG2
  printf("******* N-Best Set *******\n") ; 
  DisplayPathRep(&pathNBest) ; 
#endif

}/*------------End of AStar-------------*/


/*---------------------------------------------------------------*
 *                                                               *
 * AlphaCmpttn : State-Based Tagging�� �ϱ� ���� �ʿ���          *
 *               Alpha-Computation�� �Ѵ�.                       *
 *                                                               *
 *               HMM���� ����ϴ� Forward Algorithm�� �̿��Ͽ�   *
 *		 ��ü Trellis�� ��� ����Ѵ�.                   *
 *                                                               *
 *               Prob(O|lamda) ��, ���� �־����� ��ü          *
 *               Observation�� ���� Ȯ���� ���Ѵ�                *
 *                                                               *
 *---------------------------------------------------------------*/

PUBLIC void AlphaCmpttn()
{
  Trellis *trellisPtr = &trellis ; /* Trellis�� ���󰡸鼭 Ȯ���� ��� */ 
  Path    *befPath               ; /* ���� Trellis�� ������ Path       */
  Path    *nowPath               ; /* ���� Trellis�� ������ Path       */
  int senbef , sennow            ; 
  int befIdx , nowIdx            ;
  int numbef , numnow            ; 
  int nowTag                     ; 
  double alpha                   ; /* Forward Computation              */ 

  while(trellisPtr != curTrls) { 
	
    befPath = trellisPtr->pathPool   ; /* �� ������ ������ Path        */ 
    numbef = trellisPtr->numPath     ;

    trellisPtr = trellisPtr->nextPtr ; /* ó������ Header Trellis ���� */	
    nowPath = trellisPtr->pathPool   ; 
    numnow = trellisPtr->numPath     ; 

    for(nowIdx = 0 ; nowIdx < numnow ; ++nowIdx) {
      sennow = nowPath[nowIdx].path[0] ;  /* ���� ������ ù ���¼�     */
      nowTag = senP[sennow].pos - '0' ;   /* ���� ������ ù ���¼� Tag */
      alpha = 0.0 ; 
      for(befIdx = 0 ; befIdx < numbef ; ++befIdx) { 
        senbef = befPath[befIdx].path[befPath[befIdx].pathlen] ; 
			              /* ���� ������ ������ ���¼� */
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
 * BetaCmpttn  : State-Based Tagging�� �ϱ� ���� �ʿ���          *
 *               Beta-Computation�� �Ѵ�.                        *
 *                                                               *
 *               HMM���� ����ϴ� Backward Algorithm�� �̿��Ͽ�  *
 *               ��ü Trellis�� ��� ����Ѵ�.                   *
 *                                                               *
 *               gamma prob�� ����ϰ� gamma prob�� Sorting�Ѵ�  * 
 *                                                               *
 *---------------------------------------------------------------*/

PUBLIC void BetaCmpttn()
{
  Trellis *trellisPtr = curTrls ; /* Trellis�� ���󰡸鼭 Ȯ���� ��� */ 
  Path    *aftPath              ; /* ���� Trellis�� ������ Path       */
  Path    *nowPath              ; /* ���� Trellis�� ������ Path       */
  int senaft , sennow ; 
  int aftIdx , nowIdx ;
  int numaft , numnow ; 
  int nowTag          ; 
  int tmpIdx          ; 
  double beta         ; /* Backward Computation             */ 

  if (trellisPtr == &trellis) return ; /* �ƹ��� ���嵵 ���� �ʾҴ� */

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
	
    aftPath = trellisPtr->pathPool   ; /* �� ������ ������ Path */ 
    numaft = trellisPtr->numPath     ;

    trellisPtr = trellisPtr->backPtr ;
    nowPath = trellisPtr->pathPool   ; 
    numnow = trellisPtr->numPath     ; 

    for(nowIdx = 0 ; nowIdx < numnow ; ++nowIdx) {
     sennow = nowPath[nowIdx].path[nowPath[nowIdx].pathlen] ; 
	                               /* ���� ������ ������  ���¼�    */
     nowTag = senP[sennow].pos - '0' ; /* ���� ������ ������ ���¼� Tag */
     beta = 0.0 ; 
     for(aftIdx = 0 ; aftIdx < numaft ; ++aftIdx) { 
        senaft = aftPath[aftIdx].path[0] ; /* ���� ������ ù ���¼�     */
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
 * MakeTrellis : ������ ���� ������ �����.                       *
 *                                                                *
 * Input    : NULL                                                *
 * Output   : NULL                                                *
 * Function : ������ ���¼� Trellis������ ����                    *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void MakeTrellis()
{
   int now_ptr = 0 ; 
   int bef_ptr = 0 ;

   curTrls = &trellis ;      /* Header Trellis('INI')�� ����Ų��. */
   curTrls->nextPtr = NULL ; /* NULL�� �ʱ�ȭ */ 

   while (buffer_tag[now_ptr] != _EOL_) {

	 /* ���� ������ ã�´� */
     for (; buffer_tag[now_ptr] == _SPA_ ; ++now_ptr) ; 
     if (buffer_tag[now_ptr] == _EOL_) break ; /* ���� space..\n�̸� */
     bef_ptr = now_ptr ; 
      
     /* ���� ���� ã�´� */

     for (++now_ptr ; buffer_tag[now_ptr] != _SPA_ &&
		  buffer_tag[now_ptr] != _EOL_ ; ++now_ptr) ; 

     MakeTrellis2(bef_ptr,now_ptr) ; 

   }

}/*--------End of PutSent------------*/


/*------------------------------------------------------------------*
 *                                                                  *
 * MakeTrellis2 : ���������� ������ �Է����� �޾� ���¼� �м���     * 
 *                �ϰ� �װ��� ������ trellis�� ����� ������.       *
 *                                                                  *
 * Input    : bef,now                                               *
 * Output   : NULL                                                  *
 * Function : �ѱ��� �� ���¼� �м��� �ؼ� �� ����� senP�� �ִ´�. *
 *            �� �� index�� �����Ѵ�.                               *
 *                                                                  *
 *------------------------------------------------------------------*/

PRIVATE void MakeTrellis2(bef,now)
int bef ;                              /* ���� ���� �κ� */
int now ;                              /* ���� ��   �κ� */
{
  int tmp_value          ; 
  UCHAR tags[140]        ; /* tag�� ���� POS�� �ƴϴ�       */
  UCHAR str[140]         ; /* �� ����                       */
  UCHAR substr[140]      ; /* ���� ���� ���� tag�� �� �κ�  */ 
  UCHAR kimmo[140]       ; /* �ѱ��� ��� �̼����ڵ�� �ٲ� */
  SINT indices[100]      ; /* Morph Pool�� �ٲ� index       */
  SINT indShift[100][50] ; 
  char ender[100]        ; /* ������ �������ΰ� ?           */
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

  tags[end_ptr] = '^' ; /* while�� ���� tags[now_ptr] != temp_tag�� ����*/ 
  temp_tag = tags[0]  ; /* ���� ó�� ���� tag�� ���� */

  while(now_ptr < end_ptr) {

     /* ���� Tag�� �� �κ��� ã�´� */
     for (; tags[now_ptr] == temp_tag ; ++now_ptr) ; 
     /* ���� Tag�� �� �κ��� str[bef_ptr .. now_ptr-1]�̴� */
	 
      if (temp_tag != _HAN_) { 

      senP[senPtr].end_mark    = 'x' ; /* �����̶�� �� ��ü�� ������ 
       			        	 space�� �ִٴ� ���� ���Ѵ� */
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
    	fprintf(stderr,"Error : �ѱ��ڵ尡 �̻��մϴ�.\n") ; 
    	fprintf(stderr,"%s\n",buffer) ; 
      }
      known = PreMorphAn(kimmo) ; 
						
      /* morPtr : morphP�� Top�� ����Ų�� */
      ind_top = CollectIndices(indices) ; /* 0 .. ind_top ���� �� �ִ� */
      if (ind_top == -1) break  ; /* Error in Morph - Analysis */
      /* 0 .. ind_top ���� index�� �ִ� (ind_top : 0) */

      for(tmp = 0 ; tmp < ind_top ; ++tmp) {
        curr_ind = indices[tmp] ; 
    	for(tmp2 = 0 ; morphP[curr_ind].nidx[tmp2] != -1 ; ++tmp2) 
    	  indShift[tmp][tmp2] = Index(indices,morphP[curr_ind].nidx[tmp2]) ; 
        ender[tmp] = morphP[curr_ind].end_mark ; 
        indShift[tmp][tmp2] = -1 ;  
      } 

      /* senPtr : ������ ���� �� */

      for (tmp = 0 ; tmp < ind_top ; ++tmp) {  /* ind_top : 0�̹Ƿ� ���� */
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
     
        /* index �� 0 �϶� �ڵ������� �������� ����ġ�� �ȴ� */
     }
      /* ���� �տ� �ִ� ���� �ѱ��� �ƴϰ� ���� �ѱ��� ��� :
         �� : english(����) : '(' ���� '����'�� ���� ���¼Ұ� 
         �ϳ� �̻��̹Ƿ� '('�� ���� ���¼Ҹ� ����Ű�� �ؾ��Ѵ� */

      if (senP[senPtr-1].pos < _NCT_) {  /* �ٷ� ���� �ѱ��� �ƴϴ� */
       for(tmp2 = 0 ; morphP[morPtr].nidx[tmp2] != -1 ; ++tmp2)
        senP[senPtr-1].nidx[tmp2] =
              senPtr + Index(indices,morphP[morPtr].nidx[tmp2]) ;
        senP[senPtr-1].nidx[tmp2] = -1 ;
      }

      senPtr += ind_top ; /* sentence Pool Top Pointer ���� */
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
 * Function : ind�� indices�ȿ� �ִ� value�� ������ �� �� index��    *
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
 * Function : morphP[]���� ���� �ǹ� �ִ� record�� index�� ������ *
 *            sorting�Ѵ�.                                        *
 *                                                                *
 *----------------------------------------------------------------*/

PRIVATE int CollectIndices(indices)
SINT indices[] ; 
{
  int numind1 ; 
  int begin_ptr = -1 ; 
  int end_ptr = -1 ; 
  SINT morphIdx  ; 

  /* INI�� ����ġ�� index�� �ʱ�ȭ */

  for (numind1 = 0 ; morphP[morPtr].nidx[numind1] != -1 ; 
					 ++numind1)
    PutIdx(indices,&end_ptr,morphP[morPtr].nidx[numind1]) ;  
	
  if (end_ptr == -1) return -1 ; /* Error in Morph - Analysis */

  while (begin_ptr != end_ptr) {

    ++begin_ptr ; /* ���� ó������ 0 */
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

  return end_ptr ; /* 0 .. end_ptr���� ���� �ִ� */

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
 * Function : indices[]�� data�� �ִ´�.                  *
 *	      �̹� data�� indices[]�� ������ ���� �ʴ´�. *
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
 * InitTrellis : �ϳ��� ������ Trellis�� �����ȴ�. *
 *               Trellis *trellis�� ������ ó����  *
 *	         ���ϱ� ���� Header�� ���´�.      *
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
  trellis.pathPool->backIdx   = -1  ; /* backIdx�� ����           */
  trellis.pathPool->alphaprob = 1.0 ; /* alpha computation�� ���� */
  trellis.pathPool->ox[0]     = 
  trellis.pathPool->ox[1]     = 
  trellis.pathPool->ox[2]     = UNMARK ; /* ���õ��� �ʴ� �κ�    */

  /* one Sentence Data ���� : INI */

  oneSent.end_mark = 'x'      ; 
  oneSent.sent_begin = 0      ;
  oneSent.sent_end = 0        ; 
  oneSent.morph_begin = 0     ; 
  oneSent.morph_end = 0       ; 
  strcpy(oneSent.word,"INI")  ; 
  oneSent.pos = _INITI_       ; 
  oneSent.prob = (double) 1.0 ; /* Prob("INI"|INI) = 1.0     */ 
  oneSent.nidx[0] = 1         ; /* �� ������ ����Ŵ : 1      */ 
  oneSent.nidx[1] = -1        ; /* �� ������ ����Ŵ : E of L */ 
  senP[0] = oneSent           ; /* trellis.pathPool->path[0] */
  senPtr = 1                  ; /* senP[0] = 'INI'           */ 

}/*-----End of InitTrellis-----*/


/*-------------------------------------------------------------*
 *                                                             *
 * DeleteTrellis : �ϳ��� ������ Trellis�� �����ȴ�.           *
 *		   ���� ������ ���� Trellis�� Deallocate�Ѵ�.  *
 *                                                             *
 *-------------------------------------------------------------*/

PUBLIC void DeleteTrellis()
{
  Trellis *tmpPtr = trellis.nextPtr ; /* Trellis�� ���󰡸� ����� */
  Trellis *tmpPtr2 ; 

  while (tmpPtr != NULL) {
   tmpPtr2 = tmpPtr ; 
   tmpPtr = tmpPtr->nextPtr ; 

   free(tmpPtr2->pathPool)  ; /* PathPool ������ Deallocate     */
   free(tmpPtr2->sortedIdx) ; 
   free(tmpPtr2)            ; /* Trellis�� Deallocate           */
  }
  trellis.nextPtr = NULL    ; /* header�� nextPtr = NULL�� �Ѵ� */

}/*--End of DeleteTrellis--*/


/*--------------------------------------------------------*
 *                                                        *
 * AddTrellis : senP[]�� start Index�� end Index��        *
 *	          �޾� ���ο� Trellis�� �����            *
 *	          ����                                    *
 *                                                        *
 * ù��° ǰ�簡 openClass[]�� �ִ� ���̸�                *
 * P(w_i|t_i) = P(Known|Tag) * P^(w_i|t_i)                *
 * P(w_i|t_i) = P(Unknown|Tag) * P^(e_i|t_i)              *
 *                                                        *
 * path[0 .. pathlen]�� ������ �� �ִ�                *
 *                                                        *
 * IsInOpenTag(UCHAR in_tag) : 1 .. __UNKNNOFOPENCLS__    *
 * �׷��� IsInOpenTag(in_tag) - 1 == openClass[]�� idx    *
 *                                                        *
 * unknownProb[0 .. 11][0] = Prob(Known|Open_Class_Tag)   *
 * unknownProb[0 .. 11][1] = Prob(Unknown|Open_Class_Tag) *
 *                                                        *
 * unknownPttrn[0 .. 11].numofpttrn = number of patterns  *
 *                                    0 .. numofpttrn - 1 *
 *                                                        *
 * unknownPttrn[0 .. 11].pttrnPtr[ 0 .. numofpttrn - 1]�� *
 * pattern prob�� pattern�� �� �ִ�                   *
 *                                                        *
 * unknownPttrn[0 .. 11].pttrnPtr[X].pttrnProb = P(E_i|T) *
 * unknownPttrn[0 .. 11].pttrnPtr[X].pttrn[..] = E_i      *
 *   tag sequence�� �� �ְ� '\0'�� terminate�ȴ�      *  
 **********************************************************
 *                                                        *
 * Unkown Model 4 : Lexical Level�� Feature�� �����Ѵ�.   *
 *                                                        *
 *                                                        *
 *                                                        *
 *--------------------------------------------------------*/

PUBLIC int AddTrellis(word,sIdx,eIdx,known)
unsigned char word[] ;       /* �Է� ����               */ 
int sIdx  ;                  /* senP[]�� start Index    */
int eIdx  ;                  /* senP[]��   end Index    */
int known ;                  /* Known Word�ΰ� �ƴѰ� ? */ 
{
  int beg_ptr      ; 
  int idx          ; 
  int tmpIdx       ; 
  int tmpPLen      ; /* tmp Path Len        */
  int openCIdx     ; /* open class word Idx */
  FLAG flag        ; /* flag                */
  UCHAR first_tag                ; /* tag sequence���� ù��° tag          */
  UCHAR tagseq[__MAXPATHLEN__]   ; /* unknown word �߽߰� ������ tag seq   */
  Trellis *trellisPtr            ; /* Trellis�� �����                     */
  Path tmpPath[__TEMPPATHSIZE__] ; /* Path�� ���� : Circular Queue         */
  Path resPath[__TEMPPATHSIZE__] ; /* ���� ��� Path�� ����                */ 
  int path1 = 0 , path2 = 0      ; /* path1 .. path2 �� path�� ����        */
  int stackTop = __TEMPPATHSIZE__; /* Data is stackTop .. __TEMPPSIZE__ -1 */ 
  int nextMor ;                    /* ���� ���¼Ҹ� ����Ű�� ��            */
  int senIdx  ;  /* senIdx = tmpPath[path.].path[tmpPath[path.].pathlen]   */
  int senIdx2 ;  /* senIdx2 = senP[senIdx].nidx[]                          */ 

  trellisPtr = (Trellis *) malloc(sizeof(Trellis)) ; 

  beg_ptr = senP[sIdx].sent_begin ;  
  for(idx = sIdx ; idx <= eIdx && senP[idx].sent_begin == beg_ptr ; ++idx)
    if (senP[idx].morph_begin == 0 && senP[idx].pos < _JC_ ) {
      tmpPath[path2].pathlen = 0   ; /* path[]���� ������ node�� 0�� �ִ� */  
      tmpPath[path2].path[0] = idx ; /* path�� ���� : senP[idx]           */ 
      tmpPath[path2++].lexprob = senP[idx].prob ;
    }

     /*
      * ���� ��� `��������� ��'��� ������ ���� ��,
      * `��'�� `��/jcp+��/ef' Ȥ�� `��/jca'�� �м��� �ȴ�.
      * �� ��, ��� `���,����'�� �����ؼ� �ᱹ ��� ���õȴ�.
      * �̷��� ���, batch�� �����ų ��, ���α׷��� fail�ǹǷ�,
      * ��� �ϳ��� ���ܵξ�� �Ѵ�. (�� ������ ���� ���� ���)
      */

    if (path2 == 0) {
      tmpPath[path2].pathlen = 0      ; /* path[]���� ������ node�� 0�� �ִ� */  
      tmpPath[path2].path[0] = idx -1 ; /* path�� ���� : �� ������ ���� copy */ 
      tmpPath[path2++].lexprob = senP[idx-1].prob ; 
    }

     /*
      * P(����|Tag Seq) =   P(word_1|tag_1) * P(tag_2|tag_1) 
      *                   * P(word_2|tag_2) * P(tag_3|tag_2)
      *                   *      ....       * P(tag_n|tag_n-1)
      *                   * P(word_n|tag_n)
      *
      * word_1�� unknown-word�� �� �� �ִ�
      * �ϴ� ���� ���� P(word_1|tag_1)�� 1.0�̹Ƿ� ��ü 
      * path�� ������ �� eojeol�� unknown-word�ΰ� ���� �� 
      * known-word�� ���   : P(Known|Tag) * P(Word|Tag) 
      * Unknown-word�� ��� : P(Unknown|Tag) * P(Ending|Tag) 
      * 
      * P(Known|Tag) , P(Unknown|Tag)�� corpus���� 
      * �� �տ� �ִ� �͸��� ���� ����ߴ�.
      *
      * Circular - Queue�� �̿��Ͽ� network�� Ǭ��.
      *
      * Network�κ��� �������� Path�� �����. 
      *
      */

    while( path1 != path2 ) { /* [path1 .. path2 -1] : ������ waiting queue */

     senIdx = tmpPath[path1].path[tmpPath[path1].pathlen] ; 

     if (senP[senIdx].nidx[0] > eIdx) { /* boundary�� �Ѿ����ϱ� �� �̻� �� �ʿ� ���� */

       --stackTop ; 
       resPath[stackTop] = tmpPath[path1] ;  /* Stack������ �ִ´� */

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
       flag = ON ; /* path���� ���� ���¼Ҹ� �ѹ��� ������ٴ� �� */

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
          *  Circular Queue�� path[path2=path1 .. path1]�� Partial ����� �� �ִ�.
          *
          *  �� ���� Path�� ����� ���� �����ϰ� ���� ���� �����. 
          */ 

          break ;  

       }

      } /* end of for-loop */

      if (flag == OFF) { /* �ѹ��� expansion���� ���ߴٸ�.. �����ζ� �����. */
         senIdx2 = senP[senIdx].nidx[nextMor-1] ; /* �� ������ ���� ���� �Ѵ�. */
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

   /* tmpPath[stackTop .. __TEMPPATHSIZE__ - 1]�� ��� ����Ǿ� �ִ� */ 

   trellisPtr->pathPool = 
       (Path *) malloc(sizeof(Path) * ( __TEMPPATHSIZE__ - stackTop)) ; 

   memcpy(trellisPtr->pathPool,&resPath[stackTop],
		  sizeof(Path) * ( __TEMPPATHSIZE__ - stackTop)) ; 
   strcpy(trellisPtr->eojeol,word) ; 
   trellisPtr->startIdx = sIdx     ;  /* �ʿ��ұ� ? */
   trellisPtr->endIdx   = eIdx     ;  /* �ʿ��ұ� ? */
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
     tmpPLen = trellisPtr->pathPool[idx].pathlen ;     /* path len�� ���� */
     first_tag = senP[trellisPtr->pathPool[idx].path[0]].pos ; /* ù ǰ�� */ 
     tagseq[0] = '\0' ;                             /* Terminated by NULL */

     if (tmpPLen > 0) {
       tmpIdx = trellisPtr->pathPool[idx].path[1] ; 
       if (senP[tmpIdx].pos < _NCT_) strcpy(tagseq,senP[tmpIdx].word) ; 
       else  kimmo2ks(senP[tmpIdx].word,tagseq) ;  /* �ϼ��� �ѱ۷� �ٲ㼭 �� */
       strcat(tagseq,"/") ; 
       strcat(tagseq,posTags[senP[tmpIdx].pos-'0']) ; 
     }

#ifdef DEBUG3
     printf("lex - feature is %s\n",tagseq) ; 
#endif

     openCIdx = IsInOpenTag(first_tag) - 1 ; 

     /*
      * first_tag�� Open Class Tag�� Index�� ���Ѵ� :
      * unknown word�̹Ƿ� openCIdx�� 0 .. __NUMOFOPENCLASS__���̿� 
      * �־�� �ǳ� , ���� ���� _NCT_������ ���̸� (�� �ɹ� ��...)
      * openCIdx�� -1�� �� ���̴�. ( ��: TRI-�Ž� )
      */

     /*
      * ~CASE1 : * Prob(t_i|w_i+1,t_i+1) ������ ����� ��
      *  CASE1 : * Prob(w_i+1,t_i+1|t_i)�� ����� ���� �ִ�.
      *          ������, ���������� ~CASE1�� �� ����.
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

   curTrls->nextPtr = trellisPtr ; /* Trellis ����                      */
   trellisPtr->backPtr = curTrls ; /* backPtr ���� : Doubly Linked List */
   curTrls = curTrls->nextPtr    ; /* curTrls ����                      */
   curTrls->nextPtr = NULL       ; /* ������ Trellis.nextPtr = NULL     */ 
   
   return trellisPtr->numPath ;  

}/*--End of AddTrellis--*/


/*****************************************************/
/****************End of ktstagger.c*******************/
/*****************************************************/


