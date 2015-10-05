/*-------------------------------------------------------------------*
 *                                                                   *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9          *
 *                                                                   *
 * SangHo Lee                                                        *
 *                                                                   *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr                 *
 *                                                                   *
 * Computer Systems Lab.                                             *
 * Computer Science , KAIST                                          *
 *                                                                   *
 * ktsmoran.c ( Korean POS Tagging System : Morphological Analyzer ) *
 *                                                                   *
 *-------------------------------------------------------------------*/
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
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>

#include "ktsdefs.h"      /* Definitions                 */
#define __TAGSET__
#include "ktstbls.h"      /* Korean Tag Tables           */
#include "ktsds.h"        /* Korean Tag Data Structure   */
#define __SEMANTICS__
#include "sem.h"
#include "kimmocode.h"    /* Lee Sung-Jin Code Converter */
#include "kts.h"

WSM    wordSM[__NUMWSM__]        ; /* 새로운 환경 Manage Pool */  
InputW inputWP[__EOJEOLLENGTH__] ; /* Input Word Pool         */  
Morph  morphP[__NUMMORPH__]      ; /* Morph Pool              */
int    displayP[__NUMMORPH__]    ; /* Display Pool for Mor-An */

int disPtr = -1 ; /* Display Top Pointer of displayP      */
int smPtr = -1  ; /* Stack Manager Top Pointer of wordSM  */
int morPtr = -1 ; /* Morph Pool Top Pointer of morphP     */

int kimmolen           ; /* 입력 string의 길이            */
SINT endNodeValue      ; /* kimmolen * __SKIPVALUE__      */
UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /* 전이 tbl */
PUBLIC DBM* dictPtr    ; /* 사전을 가리키는 pointer       */

/* 
 * 자주 쓰이는 Tags
 */
EXTERN UCHAR _FINAL_          ; /* 어절 , 문장 마지막              */ 
EXTERN UCHAR _INITI_          ; /* 어절 , 문장 처음                */
EXTERN UCHAR _NCT_            ; 
EXTERN UCHAR _F_              ; /* Foreign word             : f   */
EXTERN UCHAR _SCO_            ; /* Symbol : Sentence Comma  : s,  */
EXTERN UCHAR _SCL_            ; /* Symbol : Sentence Closer : s.  */
EXTERN UCHAR _SLQ_            ; /* Symbol : Left Quotation  : s`  */
EXTERN UCHAR _SRQ_            ; /* Symbol : Right Quotation : s'  */
EXTERN UCHAR _SCN_            ; /* Symbol : Connection Mark : s-  */
EXTERN UCHAR _SNN_            ; /* Number                   : nnn */
EXTERN UCHAR _SSU_            ; /* Symbol : Unit            : su  */ 
EXTERN UCHAR _SSW_            ; /* Symbol : Currency        : sw  */
EXTERN UCHAR _SSY_            ; /* Symbol : Other Symbols   : sy  */

PUBLIC UCHAR _NN_  ; 
PUBLIC UCHAR _NNN_ ; 
PUBLIC UCHAR _PAD_ ;
PUBLIC UCHAR _NBU_ ;
PUBLIC UCHAR _NB_  ;
PUBLIC UCHAR _NPD_ ;
PUBLIC UCHAR _NPP_ ;
PUBLIC UCHAR _MD_  ;
PUBLIC UCHAR _MN_  ;
PUBLIC UCHAR _M_   ;
PUBLIC UCHAR _NC_  ; 

PUBLIC UCHAR _JC_             ;
PUBLIC UCHAR _JCM_            ;
PUBLIC UCHAR _JCA_            ;
PUBLIC UCHAR _JJ_             ;
PUBLIC UCHAR _ECQ_            ; 
PUBLIC UCHAR _EF_             ; 
PUBLIC UCHAR _EXM_            ;
PUBLIC UCHAR _ECS_            ; 
PUBLIC UCHAR _EFP_            ;
PUBLIC UCHAR _ECX_            ; 
PUBLIC UCHAR _JCP_            ; 
PUBLIC UCHAR _PV_             ; 
PUBLIC UCHAR _PA_             ; 
PUBLIC UCHAR _PX_             ; 
PUBLIC UCHAR _XN_             ; 
PUBLIC UCHAR _XPV_            ;
PUBLIC UCHAR _XPA_            ; 
PUBLIC UCHAR _XA_             ; 
PUBLIC UCHAR _A_              ; 
PUBLIC UCHAR _AT_             ; 
PUBLIC UCHAR _JX_             ;
PUBLIC UCHAR _NCA_            ; 
PUBLIC UCHAR _NCS_            ; 
PUBLIC UCHAR _INT_            ; /* 매개 모음 '으'                  */
PUBLIC UCHAR jcp[] = "i"      ; /* 서술격 조사 '이'                */
PUBLIC UCHAR jcpInfo[1][__ATTRSIZE__] ; /* Category & irregular    */ 
PUBLIC double jcpProb =1.0    ; /* Prob(Word = '이'|Tag = jcp)     */
 
EXTERN int tagfreq[__NUMOFTAG__] ; /* 형태소 발생 Frequency    */

EXTERN void (*irr_func[])()   ; /* 불규칙 처리 함수 Table          */
EXTERN void GetIrr()          ; /* 불규칙 처리 Position Finding    */
EXTERN int kimmo2ks()         ; /* 이성진 code --> KS5601          */
EXTERN void ReverseString()   ; /* string을 거꾸로 만든다          */ 
EXTERN int SplitStr()         ; /* Encoding된 내용을 Decode한다    */  
EXTERN int DecodePreMor()     ; /* 형태소 기분석의 내용을 Decode   */
EXTERN void PredictUnknown()  ; /* Unknown Word를 추정한다         */
EXTERN int MrgnFreq()         ; /* Marginal Frequency Word_i       */

PUBLIC void LoadTranTbl()     ; /* 품사 전이 table을 가져온다      */
PUBLIC int PreMorphAn()       ; /* Pre 형태소 분석                 */
PUBLIC int MorphAn()          ; /* 형태소 분석                     */ 
PUBLIC WSM MakeOneWSM()       ; /* Make one wsm record             */ 
PUBLIC void MakeWord()        ; /* Make one InputW record          */ 
PUBLIC void LookupDict()      ; /* 사전 Lookup ( morphP에서 먼저 ) */
PUBLIC void PutEntry()        ; /* 사전에서 찾은 정보를 넣는다     */ 
PUBLIC int GetNextWordPos()   ; /* 사전에서 찾을 형태소를 만듦     */ 
PUBLIC void WSMPush()         ; /* Word-Stack-Manager Push         */ 
PUBLIC void WSMPop()          ; /* Word-Stack-Manager Pop          */
PUBLIC int GetLNodeIdx()      ; /* 형태소의 왼쪽 Node Idx return   */
PUBLIC UCHAR GetLeeOnRIdx()   ; /* Right Index의 이성진 코드       */
PUBLIC void InitTag()         ; /* Global Tags Initialization      */ 
PUBLIC void MorPush()         ; /* Morph Push                      */ 
PUBLIC void ConTest()         ; /* Connectibility Test             */
PUBLIC Morph MakeMor()        ; /* 하나의 형태소 record 만든다     */ 

#ifdef DEBUG
PUBLIC void DisplayWordPool() ; /* Word Pool Display               */
PUBLIC void DisplayMorph()    ; /* Morph Pool Display              */
PUBLIC void OutputMorph()     ; /* Display Pool을 KS5601로 Display */
PUBLIC void Out2Han()         ; /* 형태소를 KS5601로 Display       */
#endif

PRIVATE void GetSE()          ; /* 시작과 끝의 가능성을 적는다     */  
PRIVATE void MakeWP()         ; /* Word Pool을 만든다              */ 
PRIVATE void RealLookupDict() ; /* 실제로 사전에서 형태소 찾음     */
PRIVATE void IdxCopy()        ; /* Morph Idx Copy until -1         */
PRIVATE SINT GetNBegin()      ; /* Get Next-Begin-Position(inputW) */ 
PRIVATE SINT GetNEnd()        ; /* Get Next-End-Position  (inputW) */
PRIVATE SINT GetFirstBegin()  ; /* Get First-Begin-Position        */ 
PRIVATE void GetPreProbs()    ; /* 형태소 기분석의 P(word|tag)     */



/*----------------------------------------------------------------*
 *                                                                *
 * PreMorphAn : 입력 어절을 받아 형태소 분석을 한다               *
 *                                                                *
 * Input   : kimmo                                                *
 * Output  : NULL                                                 *
 * Fuction : morphP[]에 형태소 Trellis Structure를 만들어 넣는다  *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC int PreMorphAn(kimmo)
UCHAR kimmo[]  ;                                 /* string : 어절 */
{
  int   anal_return  ; 
  UCHAR starter[100] ;     /* 형태소 시작이 가능한가 ?    */ 
  UCHAR ender[100]   ;     /* 형태소 끝이 가능한가 ?      */
  UCHAR hanguel[140] ;     /* 이성진 코드에 대응하는 한글 */ 
  
  disPtr = -1 ; 
  morPtr = -1 ;      
  smPtr = -1  ; 

  kimmolen = strlen(kimmo) ; 
  endNodeValue = (SINT) kimmolen  * __SKIPVALUE__ ; 

  GetSE(kimmo,kimmolen,starter,ender) ;          /* 시작과 끝의 가능성 */  
  MakeWP(kimmo,kimmolen,starter,ender) ;         /* Word Pool을 만든다 */ 
  GetIrr() ;                                     /* 불규칙 정보 얻는다 */

  anal_return = MorphAn() ;  /* 형태소 분석 */

  if(!anal_return) {
#ifdef DEBUG
    kimmo2ks(kimmo,hanguel) ; 	
	fprintf(stderr,"Error : Unknown Word : %s\n",hanguel) ; 
#endif
  } 

#ifdef DEBUG
  DisplayMorph()      ; /* Morph Pool을 보임  */
  OutputMorph(morPtr) ; /* 형태소 분석 결과   */
#endif
  return anal_return  ; /* 
			 * 1 :   Known Word
                         * 0 : Unknown Word
			 */

}/*--------End of PreMorphAn------------*/

/*----------------------------------------------------------------*
 *                                                                *
 * GetSE   : 이성진 code를 받아                                   *
 *           형태소 시작 , 끝 가능 Position Marking               *
 *                                                                *
 * Input   : kimmo   : 이성진 code                                *
 *           starter : 형태소 시작 Position Array                 *
 *           ender   : 형태소 끝 Position Array                   *
 * Output  : NULL                                                 *
 * Fuction : starter 와 ender에 Marking 한다.                     *
 *                                                                *
 *----------------------------------------------------------------*/

PRIVATE void GetSE(kimmo,strl,starter,ender)
char kimmo[]    ;                          /* kimmo code          */
int strl        ;                          /* kimmo code의 길이   */
UCHAR starter[] ;                          /* 시작이 가능하면 'o' */
UCHAR ender[]   ;                          /* 끝이 가능하면 'o'   */
{
  int i ; 

  memset(starter,(int) ' ' , strl) ; 
  memset(ender,(int) ' ' , strl) ; 

  for (i = 0 ; i < strl ; ++i) {
   if (strchr(Is_cho,kimmo[i])) {             /*    초성    */
      starter[i] = 'o' ; 
      ender[i] = 'x' ;                        /* not allowd */
   } else if (strchr(Is_half,kimmo[i])) {     /*  반 모음   */
      starter[i] = 'o' ; 
      ender[i] = 'x' ;                        /* not allowd */
   } else if (strchr(Is_head,kimmo[i])) {
      starter[i] = ender[i] = 'o' ;  
   } else if (strchr(Is_jong,kimmo[i])) {     /*    종성    */
      starter[i] = 'x' ; 
      ender[i] = 'o' ; 
   } else starter[i] = ender[i] = 'o' ;       /*    중성    */  
  }


}/*----------End of GetSE----------------*/

/*----------------------------------------------------------------*
 *                                                                *
 * MakeWP  : 이성진 code , 시작 , 끝 가능 Position Mark Array를   *
 *           받아 Word Pool을 만든다.                             *
 *                                                                *
 * Input   : kimmo   : 이성진 code                                *
 *           starter : 형태소 시작 Position Array                 *
 *           ender   : 형태소 끝 Position Array                   *
 * Output  : NULL                                                 *
 * Fuction : inputWP[]에 이성진 code, starter , ender copy        *
 *                                                                *
 *----------------------------------------------------------------*/
 
PRIVATE void MakeWP(kimmo,strl,starter,ender)
char kimmo[]    ;                      /* input kimmo code */
int strl        ;                      /* kimmocode length */
UCHAR starter[] ;         /* 시작할 수 있는 곳에 대한 정보 */
UCHAR ender[]   ;           /* 끝날 수 있는 곳에 대한 정보 */
{
  SINT idx ;                          /* inputWP[]의 index */

  for(idx = 0 ; idx < strl ; ++idx) {
    inputWP[idx].left  = idx * __SKIPVALUE__ ; 
    inputWP[idx].right = inputWP[idx].left + __SKIPVALUE__ ; 
    inputWP[idx].start = starter[idx] ; 
    inputWP[idx].end   = ender[idx] ; 
    inputWP[idx].lee   = kimmo[idx] ; 
  }
#ifdef DEBUG
  printf ("Module : MakeWP\n") ; 
  DisplayWordPool(strl-1) ; 
#endif

}/*----------End of MakeWP-----------*/


/*-----------------------------------------------------------*
 *                                                           *
 * LoadTranTbl  : 형태소의 Connectibility Matrix를 구성한다. *
 *	                                                     *
 * Input   : fname  : Connectibility Matrix File Name        *
 * Output  : NULL                                            *
 * Fuction : tranTable[][]에 Connectibility Matrix 구성      *
 *	                                                     *
 *-----------------------------------------------------------*/

PUBLIC void LoadTranTbl(fname)
char *fname ; 
{
  FILE *fPtr  ; 
  char in[60] ; 
  int i ; 

  if ((fPtr = fopen(fname,"r")) == NULL) {
	fprintf(stderr,"Error : %s not Found\n",fname) ; 
	return ; 
  }
  for (i = 0 ; i < __NUMOFTAG__ ; ++i) {
	fgets(in,59,fPtr) ; 
    memcpy(tranTable[i],in,__NUMOFTAG__) ; 
  }

#ifdef DEBUG
   printf ("Module : LoadTranTbl\n") ; 
   printf ("       : tranTable[0] as followings..\n") ; 
   { int i , j ; 
	 for(i = 0 ; i < __NUMOFTAG__ ; ++i) {
	   for (j = 0 ; j < __NUMOFTAG__ ; ++j)
	     putchar(tranTable[i][j]) ; 
       putchar('\n') ; 
     }
   }
#endif
}/*---------End of LoadTranTbl-----------*/

/*------------------------------------------------------------------*
 *                                                                  *
 * MorphAn : 형태소 분석기                                          *
 *                                                                  *
 * Input   : NULL                                                   *
 * Output  : NULL                                                   *
 * Fuction : inputW의 내용을 형태소 분석하여 각 형태소 분석 결과를  *
 *           MorphP에 넣는다                                        *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC int MorphAn()
{
  int lidx            ;                /* 왼쪽 Node의 Pool Index   */
  int irrIdx          ;                /* 불규칙 code의 indx       */
  SINT temp = -1      ; 
  UCHAR temp2 = '\0'  ; 
  SINT maxwright      ; 
  SINT connect[__NUMCONNECTION__] ;  /* connect 되는 idx           */
  WSM oneWSM ;                       /* 새로운 환경 하나           */ 
  Morph oneMor ;                     /* 형태소 분석된것 하나       */
  UCHAR entry[__MAXMORPH__] ;        /* 한번 시도해 보는 entry     */  
  UCHAR info[__NUMATTR__][__ATTRSIZE__] ;
  int numInfo         ;              /* entry에 대한 정보의 갯수   */

  double probs[__NUMATTR__] ;          /* 확률 : P(word|tag),P(tag,word)*/

  /* 초기화 : WSM 과 morphP */ 

  oneWSM = MakeOneWSM(
   GetFirstBegin(kimmolen) , /* begin : 첫번째 형태소 시작번호      */
   endNodeValue ,            /* end   : 현재 환경의 형태소 end node */ 
   (SINT ) kimmolen - 1 ,    /* top   : 현재 Morph Pool의 topPosIdx */ 
   0 ,                       /* bottom: 현재 Morph Pool의 bottomIdx */ 
   0 ,                       /* ini   : 현재 environment의 ini node */
   endNodeValue ,            /* fin   : 현재 environment의 fin node */
   'R'                       /* direct: pos .. pos + i : word 만듦  */
  ) ; 
  WSMPush(oneWSM) ;          /* 새로운 environment 시작 */

  oneMor = MakeMor( 
   0            ,            /* 환경 : 제일 처음                     */
   'x'          ,            /* 다음에 space가 있는가                */
   endNodeValue ,            /* 형태소 왼쪽 node                     */
   endNodeValue ,            /* 형태소 하나의 오른쪽 node            */
   endNodeValue ,            /* 최종 형태소 해석 결과의 마지막 node  */
   &temp ,                   /* 다음 형태소 가리키는 index : -1 : 끝 */
   &temp2 ,                  /*  NULL                                */
   _FINAL_ ,                 /* 형태소 index                         */
   '0' ,                     /* 형태소 불규칙 index                  */ 
   (double) 1.0              /* 확률 : P(word|tag)                   */
  ) ; 
  MorPush(oneMor) ;          /* 마지막 Marker를 넣는다 */

  do {
    MakeWord(entry) ;   

	   /*
	    *  현재 environment에서 begin..end사이의 글자를 만든다 
	    *  wordSM의 top 부터 wordSM[0]의 bottom 까지. 
	    */

    LookupDict(entry,info,probs,&numInfo) ; 

	   /* 
	    *  사전 lookup은 현재 그 단어를 찾는 것만이 
	    *  고려의 대상이 되므로 wordSM을 가지고 갈 필요는 없다.
	    *  morphP에서 찾고 나서 없으면 사전에서 그것을 찾는다.
            * 
	    *   info[0 .. numInfo-1]에 information 저장 
	    *  probs[0 .. numInfo-1]에 P(word|tag) 저장 
	    *   info[][0] == F_T : probs[]에 P(word|tag)가 저장되어 있다.
            */

    PutEntry(entry,info,probs,numInfo) ; 

           /* 
	    *  무조건 morphP에 넣는 것을 원칙으로 한다.  
	    *  물론 numInfo == 0 이라면 아무것도 안하고 return.
	    *  numInfo 갯수대로 push를 하는데 단지 pointer 조작 , 
            *  즉 wright,nidx만을 handling한다.
            */

           /* 
            *  불규칙 처리 routine
            *  오른쪽 맨 끝까지 보았으면 시작한다 
            */

    if (wordSM[smPtr].end == endNodeValue) {
      lidx = GetLNodeIdx(wordSM[smPtr].begin) ; /* 왼쪽 index */
      for (irrIdx = 0 ; irrIdx < __NUMIRR__ ; ++irrIdx)
        if (inputWP[lidx].irr[irrIdx] == 'o') (*irr_func[irrIdx])(lidx) ; 
    }

           /* 
            *  그 다음 가능한 begin..end를 morphP , wordSM , inputWP를 
            *  이용하여 구한다음 wordSM의 top에 적는다.
            */

  } while(GetNextWordPos('G')) ; /* General Searching */

  /* 여기서 INI 를 넣는다 */

  ConTest(morPtr,&maxwright,connect,(SINT) 0 , _INITI_ ) ;  

  oneMor = MakeMor(
   smPtr     , /* 환경                                         */
   'x'       , /* 형태소와 형태소의 띄어쓰기 :        
		  예 : 이러는 : 이렇게 하는 (띄어쓰기가 필요)  */ 
   (SINT) 0  , /* INI 의 왼쪽   : 0                            */
   (SINT) 0  , /* INI 의 오른쪽 : 0                            */
   maxwright , /* 최종 형태소 해석 결과의 마지막 node MAX      */
   connect   , /* 다음 형태소가 있는 곳을 가리키는 index:-1:끝 */
   &temp2    , /* NULL                                         */
   _INITI_   , /* INI index                                    */
   '0'       , /* INI 불규칙 code                              */
   (double) 1.0/* 확률 : P(tag|word)                           */
  ) ; 
  MorPush(oneMor) ;               /* INI를 넣는다 */

  if (maxwright == endNodeValue) return 1 ;  

                              /* Unknown Word 처리 */
  PredictUnknown() ; 
                              /*
                               * morP[]에 Unknown word 처리를 해서  
                               * morP[]내용을 보완한다.
                               * 가능한 모든 형태소를 추정하여 선택한다
                               */
  return 0 ; 
              /*
               * 1 : 모든 형태소가 사전에 있는 것이다 
               * 0 : Unknown word 처리를 해서 나온 결과이다 
               */

}/*----------End of MorphAn-----------*/

/*---------------------------------------------------------------*
 *                                                               *
 * MakeWord : word stack manager의 direct에 의하여 이성진 code를 *
 *            연결하여 사전에서 찾아 볼 형태소를 만들어 본다.    *
 *                                                               *
 * Input    : entry                                              *
 * Output   : entry                                              *
 * Function : direct에 의하여 begin..end사이의 글자를 만든다.    *
 *            글자를 만들 때 inputWP[i] .. inputWP[0]까지        *
 *                                                               *
 *---------------------------------------------------------------*/

PUBLIC void MakeWord(entry)
UCHAR entry[] ; 
{
  SINT morStarter = wordSM[smPtr].begin ;
  SINT nextMorStart = wordSM[smPtr].end ; 
  SINT top = wordSM[smPtr].top ; 
  SINT inter ;                                  /* 중간 node 번호 */
  int temp  ; 
  int temp2 = 0  ; 

  if (wordSM[smPtr].direct == 'L') {
	inter = nextMorStart ;              /* 제일 처음에는 맨 마지막 node */ 
    while(inter != morStarter) {
	  for(temp = top ; temp >= 0 ; --temp)
		if (inputWP[temp].right == inter) {
		  entry[temp2++] = inputWP[temp].lee ; 
		  inter = inputWP[temp].left ;              /* 왼쪽 것을 구한다 */
          break ;  
        } 
    }
    entry[temp2] = '\0' ; 
    ReverseString(entry,temp2) ;              /* string을 거꾸로 만든다 */ 
  } else { /* wordSM[smPtr].direct == 'R' */
    inter = morStarter ;                  /* 제일 처음에는 맨 처음 node */ 
    while(inter != nextMorStart) {
	  for(temp = top ; temp >= 0 ; --temp)
		if (inputWP[temp].left == inter) {
		  entry[temp2++] = inputWP[temp].lee ; 
		  inter = inputWP[temp].right ;           /* 오른쪽 것을 구한다 */
          break ; 
        }
    }
	entry[temp2] = '\0' ; 
  }
}/*-------End of MakeWord------------*/


/*------------------------------------------------------------------*
 *                                                                  *
 * LookupDict : 사전(dictionary)에서 형태소가 있는가 보고 형태소가  *
 *		있으면 품사 , 확률 등을 가지고 온다.                *
 *                                                                  * 
 *   Input    : entry , info , probs , numInfo                      *
 *   Output   : info , probs , numInfo                              * 
 *   Function : entry와 동일한 것이 morphP[0] 부터                  * 
 *              morphP[morPtr]사이에 발견이 되면 사전을 찾지 않고   *
 *              그곳에 있던 정보를 전부 list로 받는다.              *
 *              없으면 실제 사전(dictionary)에서 찾는다.            *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC void LookupDict(entry,info,probs,numInfo)
UCHAR entry[]  ;                      /* 사전에서 찾아야 될 entry */  
UCHAR info[][__ATTRSIZE__] ;          /* 사전을 Lookup했을때 info */
double probs[] ;                      /* P(word|tag)              */ 
int *numInfo ;                        /* ambiguity 갯수           */ 
{
  int idx  ; 
  int idx2 ; 

  *numInfo = 0 ;

  for(idx = 0 ; idx <= morPtr ; ++idx)
	if (!strcmp(morphP[idx].word,entry) && morphP[idx].env == 0) {
	 for(idx2 = idx ; idx2 <= morPtr && morphP[idx2].env == 0 &&
			  !strcmp(morphP[idx2].word,entry) ; ++idx2) {

	   info[*numInfo][0] = F_T ;              /* Attribute : Tag를 저장 */
	   info[*numInfo][1] = morphP[idx2].pos ; /* POS 저장               */ 
	   info[*numInfo][2] = morphP[idx2].irr ; /* 불규칙 저장            */ 
	   probs[*numInfo] = morphP[idx2].prob  ; /* P(word|tag),P(tag|word)*/
	   ++*numInfo ; 
     }                         /* morphP[idx].env 가 0일때 믿을 수 있다 */

	 return ; 
   }

  RealLookupDict(entry,info,probs,numInfo) ; 

}/*-----------End of LookupDict-------------*/


/*-------------------------------------------------------------*
 *                                                             *
 * RealLookupDict : 실제 사전(dictionary)에서 형태소를 찾는다. *
 *                                                             *
 * Input    : entry , info , probs , numInfo                   *
 * Output   : info , probs , numInfo                           *
 * Function : entry를 key로 하여 실제 사전(dictionary)에서     *
 *            찾아본다.                                        *
 *            (tag,irr-information) --> info-array             *
 *            ambiguity             --> numInfo                *
 *            Prob(word|tag)        --> probs-array            *
 *            Prob(tag|word)        --> probs-array            *
 *                                                             *
 *-------------------------------------------------------------*/

PRIVATE void RealLookupDict(entry,info,probs,numInfo)
UCHAR entry[]  ;                          /* 사전에서 찾아야 될 entry */  
UCHAR info[][__ATTRSIZE__] ;              /* 사전을 Lookup했을때 info */
double probs[] ;                          /* P(word|tag)              */ 
int *numInfo ;                            /* ambiguity 갯수           */ 
{
  datum key        ; 
  datum content    ; 
  int idx          ; 
  int freq         ; 

  key.dptr = (char *) entry ; 
  key.dsize = strlen(entry) + 1 ; 

  content = dbm_fetch(dictPtr,key) ; 

  if (content.dptr == NULL) *numInfo = 0 ; 
  else {
    *numInfo = SplitStr(info,content.dptr)   ;       /* 정보를 Decode  */

    for(idx = 0 ; idx < *numInfo ; ++idx) {
      switch(info[idx][0]) {
       case F_T : 
		  freq = atoi(&info[idx][3]) ;       /* Freq(tag,word) */
                  probs[idx] =
       		(double) freq / (double) tagfreq[info[idx][1]-'0'] ; 
                  break ; 
       case F_P : break ;    /* 형태소 기분석은 PutEntry에서 해결한다. */
       case F_F : break ;    /* 형태소 분석 PreFerence는 PutEntry에서 해결 */
       case F_L : break ;    /* F_L : Lookahead Characters like Trie   */ 
      }
    }

  }


}/*---End of RealLookupDict---*/

/*------------------------------------------------------------------*
 *                                                                  *
 *   PutEntry : 형태소와 그에 따른 정보들을 morphP[]에 저장한다.    *
 *              기존에 존재하는 형태소들과 접속 가능한가 조사한 후  *
 *              가능할 때 그 형태소를 가르키도록 index를 조절한다.  *
 *                                                                  *
 *   Input    : entry , info , probs , numInfo                      *
 *   Output   : NULL                                                *
 *   Function : 만약 numInfo == 0 이면 return                       *
 *              입력 형태소의 오른쪽 node 번호와 기존의 형태소들중  *
 *              왼쪽 node 번호와 일치하고 접속가능하면              *
 *              그 형태소가 있는 곳을 가리킨다.                     *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC void PutEntry(entry,info,probs,numInfo)
UCHAR entry[]  ;                          /* 사전에서 찾아야 될 entry */  
UCHAR info[][__ATTRSIZE__] ;              /* 사전을 Lookup했을때 info */
double probs[] ;                          /* P(word|tag)              */ 
int  numInfo ;                            /* ambiguity 갯수           */ 
{
  JoinMark joinmark  ;  /* Pre-Analyzed의 좌우 context   : +: 어절 안 */
  JoinMark tmpmark   ;  /* 현재 찾은 단어의 좌우 context : $: 어절 끝 */ 
  SINT morBegin      ; 
  SINT morEnd        ;
  int    idx , idx2  ;                  /* information Index         */ 
  Morph oneMor       ; 
  double preprobs[__NUMPREMORANAL__]  ; /* 형태소 기분석의 갯수      */
  SINT connect[__NUMCONNECTION__]     ; /* connect 되는 idx          */
  PreMor premor[__NUMPREMORANAL__]    ; /* 형태소 기분석된 내용 보관 */
  int numpremor      ; /* 기분석된 내용의 형태소 갯수                */
  SINT maxwright ; 
  int mtPtr = morPtr ;                  /* 이전 morPtr을 저장한다    */

  morBegin = wordSM[smPtr].begin ;      /* 형태소 시작 node          */
  morEnd = wordSM[smPtr].end ;          /* 형태소 끝 node            */

  if (!numInfo) return ;         /* numInfo가 0이면 무조건 return    */

  for(idx = 0 ; idx < numInfo ; ++idx) {
    switch(info[idx][0]) {

     case F_T : /* Format : Tag */
                ConTest(mtPtr,&maxwright,connect,morEnd,info[idx][1]) ;  
                oneMor = MakeMor(
                  smPtr        , /* 환경                                     */
                  'x'          , /* (준말-본디말)에서 띄어쓰기가 필요치 않다 */
                  morBegin     , /* 형태소 왼쪽 node                         */
                  morEnd       , /* 형태소 하나의 오른쪽 node                */
                  maxwright    , /* 최종 형태소 해석 결과의 마지막 node MAX  */
                  connect      , /* 다음 형태소가 있는곳 가리키는index:-1:끝 */
                  entry        , /* 하나의 형태소                            */
                  info[idx][1] , /* 형태소 품사 index                        */
                  info[idx][2] , /* 형태소 불규칙 code                       */
                  probs[idx]     /* 확률 : P(word|tag)                       */
                ) ; 
                MorPush(oneMor) ;    /* 무조건 형태소는 넣는다  */
                break ; 

     case F_P : /* Format  : Pre 형태소 기분석              */
		/* example : 이러지 : 이렇게/a*하/pv+지/ecx 
                 *           지/ecx에 대해서 이전 것과 연결한다.
	         *           뒤에서 앞으로 MorPush() 한다.
                 *    주의 : 형태소 기분석의 가장 왼쪽 형태소의 왼쪽 번호와 
		 *	     가장 오른쪽 형태소의 오른쪽 번호를 
		 *	     이미 분석된 다른 형태소들과 Sync를 맞추고 
		 *	     기분석 형태소의 번호에는 -1(Don't Care)를 넣는다
		 *
		 * morBegin과 morEnd는 현재 lookup한 단어의 좌우 끝 번호다.
		 * 어절의 좌우 끝 번호는 0과 endNodeValue이다.
		 *
                 */ 

                numpremor = DecodePreMor(premor, &info[idx][1], &joinmark) ;  

                /*
		 * joinmark.lmark와 joinmark.rmark를 보아서 
		 * 어절 내는 +, 어절 좌우 끝은 $표시이므로
		 * 현재 찾은 단어의 좌우 context와 맞는지 비교한다.
		 */
                
		tmpmark.lmark = (morBegin == 0) ? '$' : '+' ; 
		tmpmark.rmark = (morEnd == endNodeValue) ? '$' : '+' ; 
                
                if (tmpmark.lmark != joinmark.lmark ||
		    tmpmark.rmark != joinmark.rmark) break ;
		                                 /* 좌우 context가 같지 않으면 */

                GetPreProbs(preprobs,premor,numpremor) ; /* P(word|tag)를 구함 */
		ConTest(mtPtr,&maxwright,connect,morEnd,
		        premor[numpremor-1].pos) ;   /* 기분석의 마지막 형태소 */
                oneMor = MakeMor(
                  1          , /* 환경 = 1 : Original 어절이 아님          */
       	          'x'        , /* (준말-본디말)에서 띄어쓰기가 필요치 않다 */
                  ((numpremor == 1) ? morBegin : -1) , 
			       /* 형태소 왼쪽node : morBegin 혹은 Don'tCare*/
                  morEnd     , /* 형태소 하나의 오른쪽 node                */
                  maxwright  , /* 최종 형태소 해석 결과의 마지막 node MAX  */
                  connect    , /* 다음 형태소가 있는곳 가리키는 index:-1:끝*/
		  premor[numpremor-1].word , /* 마지막 형태소              */
		  premor[numpremor-1].pos  , /* 형태소 품사 index          */
                  '0'        , /* 형태소 불규칙 code                       */
                  preprobs[numpremor-1] /* 확률 : P(word|tag)              */
                ) ; 
                MorPush(oneMor) ;  

                for (idx2 = numpremor - 2 ; idx2 >= 0 ; --idx2) {
                  connect[0] = morPtr ; /* 방금전에 Push한 것을 가리킨다 */
                  connect[1] = -1     ; 
                  oneMor = MakeMor(1,((premor[idx2].mark == '*') ? 'o' : 'x'), 
			((idx2 == 0) ? morBegin : -1) , -1 , maxwright , connect ,
			premor[idx2].word,premor[idx2].pos,'0',preprobs[idx2]) ; 
                  MorPush(oneMor) ; 
                }
                break ; 

     case F_F : /* Format  : Pre 형태소 기분석              */
                /* example : 이러지 : 이렇게/a*하/pv+지/ecx
                 *           지/ecx에 대해서 이전 것과 연결한다.
                 *           뒤에서 앞으로 MorPush() 한다.
                 *    주의 : 형태소 기분석의 가장 왼쪽 형태소의 왼쪽 번호와
                 *           가장 오른쪽 형태소의 오른쪽 번호를
                 *           이미 분석된 다른 형태소들과 Sync를 맞추고
                 *           기분석 형태소의 번호에는 -1(Don't Care)를 넣는다
                 */

                numpremor = DecodePreMor(premor, &info[idx][1],&joinmark) ;

                /*
                 * joinmark.lmark와 joinmark.rmark를 보아서
                 * 어절 내는 +, 어절 좌우 끝은 $표시이므로
                 * 현재 찾은 단어의 좌우 context와 맞는지 비교한다.
                 */

                tmpmark.lmark = (morBegin == 0) ? '$' : '+' ;
                tmpmark.rmark = (morEnd == endNodeValue) ? '$' : '+' ;

                if (tmpmark.lmark != joinmark.lmark ||
		    tmpmark.rmark != joinmark.rmark) break ;
		                                 /* 좌우 context가 같지 않으면 */

                GetPreProbs(preprobs,premor,numpremor) ; /* P(word|tag)를 구함 */

                /*
                 * Pop !! until morEnd is found
                 * It means deleting all other interpretation
                 *      ---> select this only
                 */

                while (morphP[morPtr].left != morEnd ||
                       morphP[morPtr].wright == -1) --morPtr ;
                mtPtr = morPtr ;

                ConTest(mtPtr,&maxwright,connect,morEnd,
                      premor[numpremor-1].pos) ; /* 기분석의 마지막 형태소 */
                oneMor = MakeMor(
                  1          , /* 환경 = 1 : Original 어절이 아님          */
                  'x'        , /* (준말-본디말)에서 띄어쓰기가 필요치 않다 */
                  ((numpremor == 1) ? morBegin : -1) ,
                               /* 형태소 왼쪽node : morBegin 혹은 Don'tCare*/
                  morEnd     , /* 형태소 하나의 오른쪽 node                */
                  maxwright  , /* 최종 형태소 해석 결과의 마지막 node MAX  */
                  connect    , /* 다음 형태소가 있는곳 가리키는 index:-1:끝*/
                  premor[numpremor-1].word , /* 마지막 형태소              */
                  premor[numpremor-1].pos  , /* 형태소 품사 index          */
                  '0'        , /* 형태소 불규칙 code                       */
                  preprobs[numpremor-1] /* 확률 : P(word|tag)              */
                ) ;
                MorPush(oneMor) ;

                for (idx2 = numpremor - 2 ; idx2 >= 0 ; --idx2) {
                  connect[0] = morPtr ; /* 방금전에 Push한 것을 가리킨다 */
                  connect[1] = -1     ;
                  oneMor = MakeMor(1,((premor[idx2].mark == '*') ? 'o' : 'x'),
                        ((idx2 == 0) ? morBegin : -1) , -1 , maxwright , connect ,
                        premor[idx2].word,premor[idx2].pos,'0',preprobs[idx2]) ;
                  MorPush(oneMor) ;
                }
                return ; /* 더 이상 다른 정보를 볼 필요가 없다. */
     case F_L : break ;  /* Lookahead like Trie : not implemented */
    }
  }/*--end of for--*/

}/*---------End of PutEntry-----------*/


/*--------------------------------------------------------------*
 *                                                              *
 * GetPreProbs : Get Pre-Morph-Analyzed Probabilities           *
 *                                                              *
 * Input    : premor , numpremor                                *
 * Output   : preprobs                                          *
 * Function : 기분석 형태소들을 입력으로 받아 preprobs[]에 저장 *
 *                                                              *
 *--------------------------------------------------------------*/

PRIVATE void GetPreProbs(preprobs,premor,numpremor)
double preprobs[]  ;                       /* 형태소 기분석의 P(word|tag) */
PreMor premor[]    ;                       /* 형태소 기분석된 내용 보관   */
int numpremor      ;                       /* 기분석된 내용의 형태소 갯수 */
{
  int idx , idx2 ; 
  int freq     ; 
  int bool     ;  /* 표준 사전에 그 품사가 있는가 ? */
  int numInfo  ; 
  double posPr ; 
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ;
  datum key ; 
  datum content ; 

  for(idx = 0 ; idx < numpremor ; ++idx) {
    key.dptr = (char *) premor[idx].word ; 
    key.dsize = strlen(premor[idx].word) + 1 ; 
    content = dbm_fetch(dictPtr,key) ; 
    posPr = (double) tagfreq[premor[idx].pos-'0'] ; 
    bool = FALSE ; /* 그 품사로 사전에 있지 않다 */

    if (content.dptr != NULL) {  /* 기분석 사전에 있으나 표준 사전에 없을 때 */
                                 /* 표준 사전에 Freq(Word,Tag) = 1 로 가정   */
      numInfo = SplitStr(info,content.dptr) ;  
      for(idx2 = 0 ; idx2 < numInfo ; ++idx2)
	if(info[idx2][0] == F_T && info[idx2][1] == premor[idx].pos) {
	   freq = atoi(&info[idx2][3]) ;  /* Freq(word,tag)               */
	   bool = TRUE ;                  /* 사전에 이 품사로 word가 있다 */
	   break ;
        }
     }

     preprobs[idx] = (bool == TRUE) ? (double) freq / posPr :
                                      (double) 1.0 ; 
  }

}/*---End of GetPreProbs---*/

/*--------------------------------------------------------------------*
 *                                                                    *
 * GetNextWordPos : 형태소를 만드는 방법에 따라                       *
 *                  다음 형태소를 만듦.                               *
 *                                                                    *
 * Input    : control                                                 *
 * Output   : 0 : 더이상 형태소가 없음.                               *
 *   	      1 : 다음 형태소를 만들어 보았음.                        *
 * Function : 'L' : 왼쪽으로만 pointer를 옮겨 형태소를 만든다         *
 *            'R' : 오른쪽으로만 pointer를 옮겨 형태소를 만든다       *
 *            'G' : 일반적인 경우에 해당한다                          *
 *            그 다음 가능한 begin..end를 morphP , wordSM , inputWP를 *
 *            이용하여 구한다음 wordSM의 top에 적는다.                *
 *                                                                    *
 *--------------------------------------------------------------------*/

PUBLIC int GetNextWordPos(control)
char control ; 
{
  SINT nextBegin = wordSM[smPtr].begin ; 
  SINT nextEnd = wordSM[smPtr].end ; 
  SINT mTop = wordSM[smPtr].top ; 
  SINT mini = wordSM[smPtr].ini ; 
  SINT mfin = wordSM[smPtr].fin ; 
  SINT tempBegin ; 
  SINT tempEnd ; 
  SINT morIdx ; 
  int findNext = 0 ;                            /* 다음 것을 찾았나 */

  if (nextBegin == mini && nextEnd == mfin) return 0 ;        /* 끝 */ 

  switch(control) {
	case 'L' : tempBegin = GetNBegin(mTop,nextBegin,mini) ; 
			   wordSM[smPtr].begin = tempBegin ; 
			   wordSM[smPtr].end = nextEnd ;
			   return 1 ; 
			   break ; 
    case 'R' : tempBegin = nextBegin ; 
			   tempEnd = GetNEnd(mTop,nextEnd,mfin) ; 
			   break ; 
    case 'G' : if (nextEnd == mfin) {
                tempBegin = GetNBegin(mTop,nextBegin,mini) ; /* 다음 begin */
	            tempEnd = GetNEnd(mTop,tempBegin,mfin) ;     /* 다음 end   */
               } else {
                tempBegin = nextBegin ; 
	            tempEnd = GetNEnd(mTop,nextEnd,mfin) ;       /* 다음 end   */
               }
			   break ; 
  }

  while (!findNext && tempEnd != mfin) { 
    for(morIdx = morPtr ; morIdx >= 0 ; --morIdx)
      if(morphP[morIdx].left == tempEnd &&
       morphP[morIdx].wright == endNodeValue) {
	   findNext = 1 ; /* 최종적으로 fin에 갈 수 있는 것 */ 
	   break ; 
     }
    if (!findNext) tempEnd = GetNEnd(mTop,tempEnd,mfin) ; 
  } ;
      /*  최악의 경우 : morphP[].left = wordSM[].fin이 된다        */
      /* 절대 begin을 바꿀 일은 없다 : 최악으로 end가 끝에 가면 됨 */
 
  wordSM[smPtr].begin = tempBegin ; 
  wordSM[smPtr].end = tempEnd ; 
  return 1 ;                       /* OK : 다음 begin..end를 찾았다 */
}/*----------End of GetNextWordPos----------*/


/*--------------------------------------------------------------------*
 *                                                                    *
 * GetNBegin : Get Next Begin Position                                *
 *                                                                    *
 * Input    : mTop,preBegin,mini                                      *
 * Output   : Next Begin Position                                     *
 * Function : 다음 형태소-시작 가능한 position(node-number)를 찾는다. *
 *                                                                    *
 *--------------------------------------------------------------------*/

PRIVATE SINT GetNBegin(mTop,preBegin,mini) 
SINT mTop     ; 
SINT preBegin ; 
SINT mini     ;
{
  SINT wIdx  ;                                 /* wordP Index */
  SINT inter = preBegin ;                      /* 중간 node   */

  do {
   for (wIdx = mTop ; wIdx >= 0 ; --wIdx)
     if (inputWP[wIdx].right == inter) {
	   inter = inputWP[wIdx].left ; 
	   break ; 
     }
  } while (inputWP[wIdx].start != 'o' && inter != mini) ; 

  return inter ;                 /* 최악의 경우 inter == mini */
}/*---------End of int GetNBegin----------*/


/*------------------------------------------------------------------*
 *                                                                  *
 * GetNEnd  : Get Next End Position                                 *
 *                                                                  *
 * Input    : mTop,preEnd,mfin                                      *
 * Output   : Next End Position                                     *
 * Function : 다음 형태소-끝 가능한 position(node-number)를 찾는다. *
 *                                                                  *
 *------------------------------------------------------------------*/

PRIVATE SINT GetNEnd(mTop,preEnd,mfin)
SINT mTop   ; 
SINT preEnd ; 
SINT mfin   ;
{
  SINT wIdx ;                                   /* wordP Index */
  SINT inter = preEnd ;                         /* 중간 node   */

  do {
   for (wIdx = mTop ; wIdx >= 0 ; --wIdx)
     if (inputWP[wIdx].left == inter) {
	   inter = inputWP[wIdx].right ; 
	   break ; 
     }
  } while (inputWP[wIdx].end != 'o' && inter != mfin) ; 

  return inter ;                /* 최악의 경우 : inter == mfin */
}/*---------End of int GetNEnd----------*/


/*-----------------------------------------------------------------*
 *                                                                 *
 * ConTest  : Connectability Test :                                *
 *            Corpus에서 구한 접속 가능 Matrix를 참조하여          *
 *            두 형태소간의 접속가능성을 Test                      *
 *                                                                 *
 * Input    : mtPtr,maxwright,connect,morEnd,tag                   *
 * Output   : connect                                              *
 * Function : 접속 가능한 형태소의 index를 connect array에 넣는다. *
 *                                                                 *
 *-----------------------------------------------------------------*/

PUBLIC void ConTest(mtPtr,maxwright,connect,morEnd,tag)
int mtPtr       ; 
SINT *maxwright ; 
SINT connect[]  ; 
SINT morEnd     ; 
UCHAR tag       ;
{
  int numofconnect = 0 ; 
  int morIdx  ;
  int tempIdx ; 
  SINT max = -1 ;            

  for(morIdx = mtPtr ; morIdx >= 0 ; --morIdx) 
   if (morEnd == morphP[morIdx].left &&
      tranTable[tag - '0'][morphP[morIdx].pos - '0'] == '1' &&
      morphP[morIdx].wright != -1) {     
      if (morphP[morIdx].pos == _INT_) {  /* 매개 모음 처리 `으' */
        if (strchr(Is_jong,GetLeeOnRIdx(morEnd))) { /* `으'앞에는 유종성 체언 */
          for (tempIdx = 0 ; morphP[morIdx].nidx[tempIdx] != -1 ; 
                                   ++tempIdx )
            connect[numofconnect++] = morphP[morIdx].nidx[tempIdx] ; 
          if (max < morphP[morIdx].wright) max = morphP[morIdx].wright ; 
        } /* 종성이 아니면 아무것도 하지 않는다 */
      } else {
        connect[numofconnect++] = (SINT) morIdx ; 
        if (max < morphP[morIdx].wright) max = morphP[morIdx].wright ; 
      }
   }

  connect[numofconnect] = (SINT) -1 ; /* end of sequence */
  *maxwright = max ; 

    /*
     *  만약 앞의 형태소와 맞는 것이 하나도 없다면
     *  connect[0] = -1 , *maxwright = -1 이다 
     */

}/*----------End of ConTest----------*/

/*-----------------------------------------------------------*
 *                                                           *
 * GetFirstBegin : Get First Begin Position                  *
 *                 첫번째 시작 위치를 찾는다.                *
 *                                                           *
 * Input    : strl                                           *
 * Output   : begin position                                 *
 * Function : inputWP의 starter에서 가장 끝 부분에 있는 것을 *
 *            return                                         *
 *                                                           *
 *-----------------------------------------------------------*/

PRIVATE SINT GetFirstBegin(strl)
int strl ; 
{
  SINT i ; 
  for(i = strl - 1 ; i >= 0 ; --i)
    if (inputWP[i].start == 'o') 
	return inputWP[i].left ;      /* 시작할 수 있는 code의 왼쪽 node */  
  
  fprintf(stderr,"Error : There's no start position in Word\n") ; 
  return 0 ; 
}/*---------End of GetFirstBegin------*/


/*---------------------------------*
 *                                 *
 * WSMPop : Word Stack Manager Pop *
 *                                 *
 *---------------------------------*/

PUBLIC void WSMPop()
{
  if (--smPtr < 0) {
	fprintf(stderr,"Error : UnderFlow in WSM\n") ; 
    exit(1) ; 
  }

}/*------------End of WSMPop------------*/


/*-----------------------------------*
 *                                   *
 * WSMPush : Word Stack Manager Push *
 *                                   *
 *-----------------------------------*/

PUBLIC void WSMPush(oneWSM) 
WSM oneWSM ; 
{
  if (smPtr >= __NUMWSM__ - 1) {
      fprintf (stderr,"Error : OverFlow in Word Stack Manager : %d \n",smPtr) ; 
  }
  wordSM[++smPtr] = oneWSM ; 

}/*-----------End of WSMPush------------*/


/*-------------------------------------------------*
 *                                                 *
 * MakeOneWSM : Make One Word Stack Manager Record *
 *                                                 *
 *-------------------------------------------------*/

PUBLIC WSM MakeOneWSM(beg,en,to,bot,in,fi,direc) 
SINT beg ; 
SINT en  ; 
SINT to  ; 
SINT bot ; 
SINT in ; 
SINT fi ; 
char direc ; 
{
  WSM ret ; 
  ret.begin = beg ; 
  ret.end = en ; 
  ret.top = to ; 
  ret.bottom = bot ; 
  ret.ini = in ; 
  ret.fin = fi ; 
  ret.direct = direc ; 
  return ret ; 
}/*---------End of MakeOneWSM--------*/

/*----------------------*
 *                      *
 * MakeMor : Make Morph * 
 *                      *
 *----------------------*/

PUBLIC Morph MakeMor(en,ox,lef,righ,wrigh,nid,wor,po,ir,pro)
SINT en  ; 
char ox  ; 
SINT lef ; 
SINT righ ; 
SINT wrigh ; 
SINT nid[] ; 
UCHAR wor[] ; 
UCHAR po ; 
UCHAR ir ; 
double pro ; 
{
  Morph ret ; 
  ret.env = en ; 
  ret.end_mark = ox ; 
  ret.left = lef ; 
  ret.right = righ ; 
  ret.wright = wrigh ; 
  IdxCopy(ret.nidx,nid) ; /* copy until -1 */
  strcpy(ret.word,wor) ; 
  ret.pos = po ; 
  ret.irr = ir ; 
  ret.prob = pro ; 
  return ret ; 
}/*------------End of MakeMor-------------*/ 


/*----------------------*
 *                      *
 * IdxCopy : Index Copy *
 *                      *
 *----------------------*/

PRIVATE void IdxCopy(nidx,nid)
SINT nidx[] ; 
SINT nid[] ; 
{
  int i ; 
  for(i = 0 ; (nidx[i] = nid[i]) != -1 ; ++i) ; 
}/*------------End of IdxCopy---------------*/


/*----------------------*
 *                      *
 * MorPush : Morph Push *
 *                      *
 *----------------------*/

PUBLIC void MorPush(oneMor) 
Morph oneMor ; 
{
  if (morPtr >= __NUMMORPH__ - 1) {
	fprintf (stderr,"Error : OverFlow in Morph Pool\n",morPtr) ; 
	exit(1) ; 
  }
  morphP[++morPtr] = oneMor ; 
}/*-----------End of MorPush------------*/


/*----------------------------------------------------------*
 *                                                          *
 * GetLNodeIdx : Get Left Node Index                        *
 *                                                          *
 * Function    :  왼쪽 node 값을 받아 그것의 index를 return *
 *                                                          *
 *----------------------------------------------------------*/

PUBLIC int GetLNodeIdx(lnode)
SINT lnode ; 
{
  int i ; 

  for (i = wordSM[smPtr].top ; i >= 0 ; --i)
	if (inputWP[i].left == lnode) return i ;
  return kimmolen ; 
}/*-----------End of GetLNodeIdx--------*/


/*------------------------------------------------------------------*
 *                                                                  *
 * GetLeeOnRIdx : Get Lee-Code on Right Node Index                  *
 *                                                                  *
 * Function    :  오른쪽 node 값을 받아 그것의 이성진 코드를 return *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC UCHAR GetLeeOnRIdx(morRight)
SINT morRight ; 
{
  int i ; 

  for (i = wordSM[smPtr].top ; i >= 0 ; --i)
     if (inputWP[i].right == morRight) return inputWP[i].lee ; 
  return 'x' ;    /* Error */
}/*---------End of GetLeeOnRIdx-----------*/


/*--------------------------------------------------------*
 *                                                        *
 * InitTag  : Initialize Tags                             *
 *                                                        *
 * Function : 자주 사용되는 tag들의 index를 미리 저장하고 *
 *            `서술격 조사'의 빈도도 저장한다.            *
 *                                                        *
 *--------------------------------------------------------*/

PUBLIC void InitTag()
{
   _INITI_ = TagIdx(__INI__) ; /* INI                            */
   _FINAL_ = TagIdx(__FIN__) ; /* FIN                            */
   _F_     = TagIdx("f")     ; /* Foreign word             : f   */
   _SCO_   = TagIdx("s,")    ; /* Symbol : Sentence Comma  : s,  */
   _SCL_   = TagIdx("s.")    ; /* Symbol : Sentence Closer : s.  */
   _SLQ_   = TagIdx("s`")    ; /* Symbol : Left Quotation  : s`  */
   _SRQ_   = TagIdx("s'")    ; /* Symbol : Right Quotation : s'  */
   _SCN_   = TagIdx("s-")    ; /* Symbol : Connection Mark : s-  */
   _SNN_   = TagIdx("nnn")   ; /* Number                   : nnn */
   _SSU_   = TagIdx("su")    ; /* Symbol : Unit            : su  */ 
   _SSW_   = TagIdx("sw")    ; /* Symbol : Currency        : sw  */
   _SSY_   = TagIdx("sy")    ; /* Symbol : Other Symbols   : sy  */
   _NNN_   = TagIdx("nnn")   ; 
   _NN_    = TagIdx("nn")    ; 
   _JC_    = TagIdx("jc")    ;
   _JCM_   = TagIdx("jcm")   ;
   _JCA_   = TagIdx("jca")   ;
   _JJ_    = TagIdx("jj")    ;
   _ECQ_   = TagIdx("ecq")   ; 
   _EF_    = TagIdx("ef")    ; 
   _EFP_   = TagIdx("efp")   ; 
   _EXM_   = TagIdx("exm")   ; 
   _ECS_   = TagIdx("ecs")   ; 
   _ECX_   = TagIdx("ecx")   ; 
   _JCP_   = TagIdx("jcp")   ; 
   _PV_    = TagIdx("pv")    ; 
   _PA_    = TagIdx("pa")    ; 
   _NCT_   = TagIdx("nct")   ; 
   _PX_    = TagIdx("px")    ; 
   _XN_    = TagIdx("xn")    ; 
   _XPV_   = TagIdx("xpv")   ;
   _XPA_   = TagIdx("xpa")   ; 
   _INT_   = TagIdx("int")   ; 
   _NCS_   = TagIdx("ncs")   ; 
   _NCA_   = TagIdx("nca")   ; 
   _XA_    = TagIdx("xa")    ; 
   _A_     = TagIdx("a")     ;
   _AT_    = TagIdx("at")    ; 
   _JX_    = TagIdx("jx")    ; 
   _PAD_   = TagIdx("pad")   ; 
   _NBU_   = TagIdx("nbu")   ; 
   _NB_    = TagIdx("nb")    ; 
   _NPD_   = TagIdx("npd")   ; 
   _NPP_   = TagIdx("npp")   ; 
   _MD_    = TagIdx("md")    ; 
   _MN_    = TagIdx("mn")    ; 
   _M_     = TagIdx("m")     ; 
   _NC_    = TagIdx("nc")    ; 

   jcpInfo[0][0] = F_T           ; /* Attribute = Tag */
   jcpInfo[0][1] = TagIdx("jcp") ; /* Tag = jcp       */
   jcpInfo[0][2] = '0'           ; /* irr = '0'       */

}/*--------End of InitTag---------*/



/*---------------------------------------------------------------*/
/*-------------------CODES FOR DEBUGGING-------------------------*/
/*---------------------------------------------------------------*/

#ifdef DEBUG

/*----------------------------------------------*
 *                                              *
 * DisplayMorph : Morph Pool Display            *
 *                                              *
 * Input   : NULL                               *
 * Output  : NULL                               *
 * Fuction : Morph Pool을 그대로 화면에 Display *
 *                                              *
 *----------------------------------------------*/

PUBLIC void DisplayMorph()
{
  int index  ; 
  int index2 ; 

  printf("Idx env mark left right wright word           pos irr      prob nidex\n") ; 

  for(index = morPtr ; index >= 0 ; --index) {
    printf("%2d  %3d  %c  %4d %5d %6d %15s %3s  %c  %f ",index,morphP[index].env,
      morphP[index].end_mark,morphP[index].left,morphP[index].right,
      morphP[index].wright,morphP[index].word,
      posTags[morphP[index].pos-'0'],morphP[index].irr,morphP[index].prob) ; 
    for(index2 = 0 ; morphP[index].nidx[index2] != -1 ; ++index2)
	  printf("%d ",morphP[index].nidx[index2]) ; 
    putchar('\n') ; 
  }
}/*-----------End of DisplayMorph----------*/


/*--------------------------------------------------------*
 *                                                        *
 * OutputMorph : morphP[index]이후의 morph를 화면에       *
 *				 Display                                  *
 *                                                        *
 * Input   : index : 현재 morphP[index]                   *
 * Output  : NULL                                         *
 * Fuction : Lattice 구조인 Morph Pool을                  *
 *           Linear 구조로 바꾸고 KS5601로 화면에 Display *
 *                                                        *
 *--------------------------------------------------------*/

PUBLIC void OutputMorph(index)
SINT index ; 
{
  int i ; 
  
  if (index == 0) {
    Out2Han() ; 
    return ; 
  }
  
  displayP[++disPtr] = index ;  

  for (i = 0 ; morphP[index].nidx[i] != -1 ; ++i) {
    OutputMorph(morphP[index].nidx[i]) ; 
  }
  --disPtr ; 

}/*-----------End of OutputMorph-----------*/


/*-------------------------------------------------------------------*
 *                                                                   *
 * Out2Han : 이성진 code로 되어있는 morphP[displayP[i]].word을       *
 *           KS5601로 변환하여 화면에 Display                        *
 * Input   : NULL                                                    *
 * Output  : NULL                                                    *
 * Fuction : morphP[].word를 한글 string으로 변환하여 화면에 Display *
 *                                                                   *
 *-------------------------------------------------------------------*/

PUBLIC void Out2Han()
{
  int i ; 
  UCHAR hanstr[__MAXMORPH__ * 2 ] ; /* 형태소 길이 */

  for(i = 1 ; i < disPtr ; ++i) {
    kimmo2ks(morphP[displayP[i]].word,hanstr) ; 
    printf("%s/%s+",hanstr,posTags[morphP[displayP[i]].pos-'0']) ; 
  }

  kimmo2ks(morphP[displayP[i]].word,hanstr) ; 
  printf("%s/%s\n",hanstr,posTags[morphP[displayP[i]].pos-'0']) ; 

}/*-----------End of Out2Han--------------*/


/*---------------------------------------------------*
 *                                                   *
 * DisplayWordPool  : Display wordP ( Word Pool )    *
 *                                                   *
 * Function         : inputWP[]을 그대로 화면에 출력 *
 *                                                   *
 *---------------------------------------------------*/

PUBLIC void DisplayWordPool(len)
int len ; 
{
   int i ; 
   printf ("Left Right Starter Ender Lee\n") ; 
   for(i = len ; i >= 0 ; --i) 
	  printf("%4d %5d     %c     %c    %c\n",
	  inputWP[i].left,inputWP[i].right,
	  inputWP[i].start,inputWP[i].end,inputWP[i].lee) ; 
}/*--------------------End of DisplayWordPool------------------*/


#endif

/***************************************************/
/*************** End of Main Prog ******************/
/***************************************************/


