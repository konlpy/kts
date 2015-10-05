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
 * ktsirr.c ( Korean POS Tagging System : irregular-handling )      *
 *                                                                  *
 *------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "ktsdefs.h"
#include "ktstbls.h"
#include "ktsds.h"
#include "irrtbl.h"
#include "kimmocode.h"

EXTERN WSM wordSM[__NUMWSM__] ;   /* 새로운 환경을 Manage하기위한 Pool */  
EXTERN InputW inputWP[__EOJEOLLENGTH__] ; 
EXTERN Morph  morphP[__NUMMORPH__] ; 
EXTERN int smPtr  ; 
EXTERN int morPtr ; 
EXTERN int kimmolen  ; 
EXTERN SINT endNodeValue ; 
EXTERN UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /*품사 전이 table*/
EXTERN UCHAR _ECQ_     ;
EXTERN UCHAR _EF_      ;
EXTERN UCHAR _ECX_     ; 
EXTERN UCHAR _EFP_     ; 
EXTERN UCHAR _ECS_     ; 
EXTERN UCHAR _JCP_     ; 
EXTERN UCHAR _PV_      ; 
EXTERN UCHAR _PX_      ; 
EXTERN UCHAR _XPV_     ;
EXTERN UCHAR _XPA_     ;
EXTERN UCHAR jcp[]     ; /* 서술격 조사 `이'            */
EXTERN UCHAR jcpInfo[1][__ATTRSIZE__] ; 
EXTERN double jcpProb  ; /* Prob(Word = `이'|Tag = jcp) */

EXTERN WSM MakeOneWSM()     ; 
EXTERN void MakeWord()      ; 
EXTERN void LookupDict()    ; 
EXTERN void PutEntry()      ; 
EXTERN int GetNextWordPos() ; 
EXTERN void WSMPush()       ;
EXTERN void WSMPop()        ;

#ifdef DEBUG
EXTERN void DisplayWordPool() ; 
#endif

PRIVATE int IrrCheck_L()     ; 
PRIVATE int BinSearch()      ; 
PRIVATE int LeftCheck()      ; 
PRIVATE int LeftPeak()       ; 
PRIVATE InputW MakeOneWord() ; 
PRIVATE void SubMorA()       ; /* 불규칙 처리를 위한 M-A */ 
PRIVATE int CatCheck()       ;


PRIVATE void irr_eu()    ; /* 0  '으' 불규칙     */ 
PRIVATE void irr_l()     ; /* 1  'ㄹ' 탈락       */
PRIVATE void irr_s()     ; /* 2  'ㅅ' 탈락       */
PRIVATE void irr_d()     ; /* 3  'ㄷ' 불규칙     */
PRIVATE void irr_b()     ; /* 4  'ㅂ' 불규칙     */
PRIVATE void irr_leu()   ; /* 5  '르' 불규칙     */
PRIVATE void irr_leo()   ; /* 6  '러' 불규칙     */
PRIVATE void irr_u()     ; /* 7  '우' 불규칙     */
PRIVATE void irr_yeo()   ; /* 8  '여' 불규칙     */
PRIVATE void irr_geola() ; /* 9  '거라' 불규칙   */
PRIVATE void irr_neola() ; /* 10 '너라' 불규칙   */
PRIVATE void irr_h1()    ; /* 11 'ㅎ' 불규칙     */
PRIVATE void irr_h2()    ; /* 12 'ㅎ' 불규칙     */
PRIVATE void irr_ae()    ; /* 13 '애,에' 축약    */
PRIVATE void irr_hae()   ; /* 14 '해' 축약       */
PRIVATE void irr_ieo()   ; /* 15 '이+어=여' 체언 , 용언 */
PRIVATE void irr_wa_we() ; /* 16 '오+아=와' '우+어=워'  */
PRIVATE void irr_a_e()   ; /* 17 '아+아=아' '어+어+어'  */
PRIVATE void irr_w8()    ; /* 18 '외+어=왜'      */
PRIVATE void irr_i()     ; /* 19 '이' 탈락       */

void (*irr_func[])() = {
    irr_eu    ,
    irr_l     ,
    irr_s     ,
    irr_d     ,
    irr_b     ,
    irr_leu   ,
    irr_leo   , 
    irr_u     ,
    irr_yeo   , 
    irr_geola , 
    irr_neola , 
    irr_h1    , 
    irr_h2    , 
    irr_ae    , 
    irr_hae   , 
    irr_ieo   , 
    irr_wa_we , 
    irr_a_e   ,
    irr_w8    ,
    irr_i 
  } ;            /* 불규칙 처리 함수 table */



/*------------------------------------------------*
 *                                                *
 * GetIrr : Get Irr-Position                      *
 *                                                *
 * Input   : NULL                                 *
 * Output  : NULL                                 *
 * Fuction : kimmolen을 받아서 wordSM[]의 irr[]에 *
 *           불규칙이면 'o' 아니면 ' '를 채운다   *
 *                                                *
 *------------------------------------------------*/

PUBLIC void GetIrr()
{
   int start  ; 
   int wordlen ; 
   int lenIdx ; 
   int idx ; 
   int irrType ; 
   int find ;  
   int irrIdx[3] ; 
   char test[__NUMMORPH__ + __WSPARE__] ; 

   for(idx = 0 ; idx < kimmolen ; ++idx)
    memset(inputWP[idx].irr,(int) ' ',__NUMIRR__) ; 

   wordlen = 0 ; /* word length */ 

   do {
    for (start = 0 ; start < kimmolen - wordlen ; ++start) {
      for (lenIdx = 0 ; lenIdx <= wordlen ; ++lenIdx)
	   test[lenIdx] = inputWP[start + lenIdx].lee ; 
	  test[lenIdx] = '\0' ; /* 하나의 글자 pattern을 만든다 */
      find = 0 ; 
      find = BinSearch(irrTable[wordlen].irr_tbl,irrTable[wordlen].size,test,
                        irrIdx) ; /* pattern에 맞을 때 irrIdx에 넣어온다 */ 

    if (find) for (idx = 0 ; idx < 3 && irrIdx[idx] != -1 ; ++idx) {
	switch(irrIdx[idx]) {

	 case 1 : if (!IrrCheck_L(start-1,start+wordlen+1))
                   irrIdx[idx] = -1 ; /* ㄹ 불규칙이 아니다 */
                  break ; 

         case 19: if (!LeftPeak(start-1)) irrIdx[idx] = -1 ; 
	     	  break ; /* '이' 탈락 현상이 아니다 */

	 case 15: if (test[0] == 'y' && !LeftCheck(start-1)) {
	           irrIdx[idx] = -1 ; 
		       break ; 
              } /* 왼쪽이 초성이 오면 안된다 */ 

	 case 16: if (test[0] == 'w' && !LeftCheck(start-1)) {
		       irrIdx[idx] = -1 ; 
		       break ; 
              } /* 왼쪽이 초성이 오면 안된다 */

     case 0 :
	 case 17: if (!irrCheck_E(start+wordlen+1))
	           irrIdx[idx] = -1 ; /* 어미 축약이 아니다 */
               break ; 
     default : break ; 
    }

    if(irrIdx[idx] != -1) {
	  irrType = irrIdx[idx] ; 
	  inputWP[start+lenIdx-1+irrShift[irrType]].irr[irrType] = 'o' ; 
    }

   } /* end of for idx */
  } /* end of for start */ 
 } while( ++wordlen < 5 ) ; /* pattern 의 최대 길이 */ 


#ifdef DEBUG
  printf ("Irregular Trigger Position\n") ; 
  printf ("Char 01234567890123456789\n") ; 
  { int i,j ; 
    for (i = 0 ; i < kimmolen ; ++i) {
      printf("  %c  ",inputWP[i].lee) ; 
      for (j = 0 ; j < __NUMIRR__ ; ++j)
        putchar(inputWP[i].irr[j]) ; 
    putchar('\n') ; 
    }
  }
#endif 

}/*--------End of GetIrr---------------*/


/*-----------------------------------------------------------------*
 *                                                                 *
 * BinSearch : Binary Search                                       *
 *                                                                 *
 * Input   : irr_t , arrsize , test , irrIdx                       *
 * Output  : irrIdx                                                *
 * Fuction : 불규칙 code table에서 불규칙이 될 수 있는 것을 찾는다 *
 *                                                                 *
 *-----------------------------------------------------------------*/

PRIVATE int BinSearch(irr_t,arrsize,test,irrIdx)
IRR irr_t[]  ;                 /* 불규칙 pattern을 갖고 있는 table */  
int arrsize  ;                 /* 불규칙 table의 크기              */  
char test[]  ;                 /* 비교할 pattern                   */
int irrIdx[] ;                 /* 찾았을 때 그것의 불규칙 code Idx */
{
  int mid ;
  int lower = 0 ; 
  int upper = arrsize - 1 ;  
  int which ; 

  while (lower <= upper) {
	mid = (lower + upper) >> 1 ; 
	which = strcmp(test,irr_t[mid].pattern) ; 
    if (which < 0 ) upper = mid - 1 ; 
	else if (which > 0) lower = mid + 1 ; 
	else {
	  irrIdx[0] = irr_t[mid].irrhandle[0] ; 
	  irrIdx[1] = irr_t[mid].irrhandle[1] ;
	  irrIdx[2] = irr_t[mid].irrhandle[2] ; 
      return 1 ;
    }
  }
  return 0 ; 
}/*------------End of BinSearch----------------*/


/*---------------------------------------------------------------*
 *                                                               *
 * IrrCheck_L : Irr-Checking : 'L'-Irr                           *
 *                                                               *
 * Input      : wordL,wordR                                      *
 * Output     : 0 : Fail                                         *
 *              1 : Success                                      *
 * Function   : 'ㄹ'탈락 현상인가를 checking하기 위한 module     *
 *              오른쪽에 { N | n | B | si | o | s9 } 중 하나인가 *
 *              왼쪽에 character가 있을 때 초성이 나오면 안된다  *
 *                                                               *
 *---------------------------------------------------------------*/

PRIVATE int IrrCheck_L(wordL,wordR)
int wordL    ;                       /* 문자 열의 왼쪽   position */
int wordR    ;                       /* 문자 열의 오른쪽 position */ 
{
  int lcheck = 1 ;                   /* 왼쪽 check                */
  int rcheck = 0 ;                   /* 오른쪽 check              */

  if (wordR >= kimmolen) return 0 ;  /* 이 부분은 BinSearch를 미리*/
									 /* 안해도 되는 부분          */ 
  if (strchr("NnBoL",inputWP[wordR].lee)) rcheck = 1 ; 
  if (wordR < kimmolen - 1 && inputWP[wordR].lee == 's')
	if (inputWP[wordR+1].lee == 'i' || inputWP[wordR+1].lee == 'y' ||
		inputWP[wordR+1].lee == '9') rcheck = 1 ; /* 시 , 셔 , 세 */
  
  if (wordL >= 0) 
	if (strchr(Is_cho,inputWP[wordL].lee)) lcheck = 0 ; 

  return lcheck && rcheck ; 
}/*-------------End of IrrCheck_L------------------*/


/*-----------------------------------------------*
 *                                               *
 * LeftCheck : Left Checking                     *
 *                                               *
 * Input      : wordL                            *
 * Output     : 0 : 왼쪽에 초성이 오면           *
 *              1 : NULL 혹은 초성이 아닐 때     *
 * Function   : 왼쪽이 NULL --> Yes              *
 *              왼쪽에 초성이 오면 No , else Yes *
 *                                               *
 *-----------------------------------------------*/

PRIVATE int LeftCheck(wordL)
int wordL ; 
{
  if (wordL >= 0)
    if (strchr(Is_cho,inputWP[wordL].lee)) return 0 ; 
  return 1 ; 
}/*-------------End of LeftCheck-------------------*/


/*-------------------------------------------------------------*
 *                                                             *
 * LeftPeak : Left Peak                                        *
 *                                                             *
 * Input      : wordL                                          *
 * Output     : 0 : 왼쪽이 중성이 아닐 때                      *
 *              1 : default                                    *
 * Function   : 무종성 체언인가를 조사                         *
 *              왼쪽이 NULL --> 체언이 없다는 뜻 : No          *
 *              왼쪽에 중성이 아니면 No : 무종성이 아니라는 뜻 *
 *                                                             *
 *-------------------------------------------------------------*/

PRIVATE int LeftPeak(wordL)
int wordL ; 
{

/* 32야 : 32 + 이 + 야  

  if (wordL < 0) return 0 ; 
*/

  if (wordL < 0) return 1 ; 
  else if (!strchr(Is_jung,inputWP[wordL].lee)) return 0 ; 
  return 1 ; 

}/*-------------End of LeftPeak--------------------*/


/*------------------------------------------------------------*
 *                                                            *
 * irrCheck_E : Irr Checking : `E' `어'인가 조사              *
 *                                                            *
 * Input      : wordR                                         *
 * Output     : 0 : default                                   *
 *              1 : { 어절 끝 | 초성 | `ㅆ' | `야' |          *
 *                       `여' | `오' | `이' | `와' }          *
 * Function   : 어미 '아/어'로 시작하는가 checking하는 module *
 *                                                            *
 *------------------------------------------------------------*/

int irrCheck_E(wordR)
int wordR    ;                       /* 문자 열의 오른쪽 position */
{
  if (wordR >= kimmolen) return 1 ;  /* 어절 끝  : OK             */ 
  if (strchr("wioghqndlmbrsvfjzcktpVy",inputWP[wordR].lee)) return 1 ;        
        /* 초성 혹은 'ㅆ' , `야' , `여' , `오' , `이' , `와' OK   */
  return 0 ; 
}/*------------End of irrCheck_E------------------*/


/*----------------------------------------------------------------*
 *----------------------------------------------------------------*
 *                    불규칙 처리 Routines                        *
 *----------------------------------------------------------------*
 *----------------------------------------------------------------*/


/*-----------*
 *           *
 * `으' 탈락 * 
 *           *
 *-----------*/

PRIVATE void irr_eu(pos) 
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* 첫 초성 copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '으' 삽입     */
  inputWP[kimmolen+1] = MakeOneWord(mnode,rnode,'x','o','_') ; 
  
  oneWSM = MakeOneWSM( lnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 

#ifdef DEBUG
  printf("EU-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif

  SubMorA('p','0','L',oneWSM) ; /* 용언 , 불규칙X , 왼쪽 */

}/*-----------End of irr_eu-----------*/


/*-----------*
 *           *
 * 'ㄹ' 탈락 *
 *           *
 *-----------*/

PRIVATE void irr_l(pos)
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* 첫 중성 copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* 'ㄹ' 삽입     */
  inputWP[kimmolen+1] = MakeOneWord(mnode,rnode,'x','o','L') ; 
  
  oneWSM = MakeOneWSM( lnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 

#ifdef DEBUG
  printf("L-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_L-----------*/

/*-------------*
 *             *
 * 'ㅅ' 불규칙 *
 *             *
 *-------------*/

PRIVATE void irr_s(pos) 
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* 첫 중성 copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* 'ㅅ' 삽입     */
  inputWP[kimmolen+1] = MakeOneWord(mnode,rnode,'x','o','S') ; 
  
  oneWSM = MakeOneWSM( lnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 

#ifdef DEBUG
  printf("S-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif
  
  SubMorA('p','s','L',oneWSM) ; 

}/*-----------End of irr_S-----------*/

/*-------------*
 *             *
 * 'ㄷ' 불규칙 * 
 *             *
 *-------------*/

PRIVATE void irr_d(pos)   
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* 'ㄹ' --> 'ㄷ' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'x','o','D') ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 

#ifdef DEBUG
  printf("D-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','d','L',oneWSM) ; 

}/*-----------End of irr_S-----------*/

/*-------------*
 *             *
 * `ㅂ' 불규칙 *
 *             *
 *-------------*/

PRIVATE void irr_b(pos)  
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* '오/우' --> 'ㅂ' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'x','o','B') ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 

#ifdef DEBUG
  printf("B-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif

  SubMorA('p','b','L',oneWSM) ; 

}/*-----------End of irr_B-----------*/

/*-------------*
 *             *
 * '르' 불규칙 *
 *             *
 *-------------*/

PRIVATE void irr_leu(pos)  
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;
  mnode = inputWP[pos].left              ;

  /* 종성 'ㄹ' --> 초성 'ㄹ' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'x','x','l') ;  

  /* 초성 'ㄹ' --> 중성 '으' */
  inputWP[kimmolen+1] = MakeOneWord(mnode,rnode,'x','o','_') ; 
  
  oneWSM = MakeOneWSM( lnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 

#ifdef DEBUG
  printf("LEU-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif
  
  SubMorA('p','l','L',oneWSM) ; 

}/*-----------End of irr_L-----------*/

/*-------------*
 *             *
 * '러' 불규칙 *
 *             *
 *-------------*/

PRIVATE void irr_leo(pos) 
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;

  /* '으ㄹ' --> '으' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'x','o','_') ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("LEO-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','L','L',oneWSM) ; 

}/*-----------End of irr_leo-----------*/

/*---------------------------*
 *                           *
 * '우' 변칙 '퍼' --> '푸어' *
 *                           *
 *---------------------------*/

PRIVATE void irr_u(pos)  
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* 첫 초성 copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '우' 삽입     */
  inputWP[kimmolen+1] = MakeOneWord(mnode,rnode,'x','o','u') ; 
  
  oneWSM = MakeOneWSM( lnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("U-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif

  SubMorA('p','0','L',oneWSM) ; 
  
}/*-----------End of irr_eu-----------*/

/*-------------*
 *             *
 * '여' 불규칙 *
 *             *
 *-------------*/

PRIVATE void irr_yeo(pos)   
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;

  /* '아이' --> '아' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'x','o','a') ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("YEO-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_leo-----------*/

/*---------------*
 *               *
 * '거라' 불규칙 *
 *               *
 *---------------*/

PRIVATE void irr_geola(pos)
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT llnode ; /* left left node */
  WSM    oneWSM  ; 

  llnode = inputWP[pos-2].left           ; 
  lnode = inputWP[pos-1].left            ; 
  rnode = inputWP[pos].right             ;

  /* 'Xㄱ' --> 'X' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'x','o',inputWP[pos-1].lee) ;  

  oneWSM = MakeOneWSM( llnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("GEOLA-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_leo-----------*/

/*---------------*
 *               *
 * '너라' 불규칙 *
 *               *
 *---------------*/

PRIVATE void irr_neola(pos) 
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;

  /* '오ㄴ' --> '오' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'o','o','o') ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("NEOLA-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_leo-----------*/

/*---------------*
 *               *
 * 'ㅎ' 불규칙 1 *
 *               *
 *---------------*/

PRIVATE void irr_h1(pos)    
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT llnode ; 
  SINT lnode  ; 
  SINT rnode  ; 
  SINT mnode  ; 
  WSM    oneWSM  ; 

  llnode = inputWP[pos-1].left           ; 
  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* 중성 copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'x','x',inputWP[pos].lee) ;  

  /* 'ㅎ' 삽입 */
  inputWP[kimmolen+1] = MakeOneWord(mnode,rnode,'x','o','H') ; 
  
  oneWSM = MakeOneWSM( llnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("H 1-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif
  
  SubMorA('p','h','L',oneWSM) ; 

}/*-----------End of irr_H 1-----------*/

/*---------------*
 *               *
 * 'ㅎ' 불규칙 2 *
 *               *
 *---------------*/

PRIVATE void irr_h2(pos)   
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT llnode  ; 
  SINT lnode   ; 
  SINT rnode   ; 
  SINT mnode1  ; 
  SINT mnode2  ; 
  UCHAR in  ;
  int plusminus = 0 ;        /* 이전 모음이 양성/음성 */
  int temp          ; 
  WSM    oneWSM  ; 

  llnode = inputWP[pos-1].left          ;
  lnode  = inputWP[pos].left            ; 
  rnode  = inputWP[pos].right           ;
  mnode1 = lnode + 2                    ;
  mnode2 = lnode + 4                    ; 

  for (temp = pos - 2 ; temp >= 0 ; --temp) {
   if (strchr("ao",inputWP[temp].lee)) {
	 plusminus = 1 ; 
	 break ; 
   } else if (strchr("eu89_i",inputWP[temp].lee)) break ; 
  }

  /* 중성 '아/어' 삽입 */
  in = (plusminus == 1) ? 'a' : 'e' ; 
  inputWP[kimmolen] = MakeOneWord(mnode2,rnode,'o','o',in) ; 
  
  oneWSM = MakeOneWSM( mnode2 , rnode , kimmolen , 
                       kimmolen , mnode2 , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("H 2-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; /* 어미 */

  /* 중성 '아/어' 삽입 */
  in = (plusminus == 1) ? 'a' : 'e' ;
  inputWP[kimmolen] = MakeOneWord(lnode,mnode1,'x','x',in) ; 

  /* 종성 'ㅎ' 삽입 */ 
  inputWP[kimmolen+1] = MakeOneWord(mnode1,mnode2,'x','o','H') ; 
  
  oneWSM = MakeOneWSM( llnode , mnode2 , kimmolen + 1 , 
                       kimmolen , 0 , mnode2 , 'L') ; 
#ifdef DEBUG
  printf("H 2-Irr2\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif
  
  SubMorA('p','h','L',oneWSM) ; 

}/*-----------End of irr_H 2-----------*/

/*--------------*
 *              *
 * '애/에' 축약 *
 *              *
 *--------------*/

PRIVATE void irr_ae(pos)  
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT llnode  ; 
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  llnode = inputWP[pos-1].left           ;
  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* '어' 삽입 */
  inputWP[kimmolen] = MakeOneWord(mnode,rnode,'o','o','e') ;  
  
  oneWSM = MakeOneWSM( mnode , rnode , kimmolen , 
                kimmolen , mnode , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("AE-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; 

  /* '애/에' Copy */
  inputWP[kimmolen] = 
	  MakeOneWord(lnode,mnode,'x','o',inputWP[pos].lee) ; 
  
  oneWSM = MakeOneWSM( llnode , mnode , kimmolen , 
                       kimmolen , 0 , mnode , 'L') ; 
#ifdef DEBUG
  printf("AE-Irr2\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

 /* 가세요 : 가 + 시 + 어요 */

  if (inputWP[pos-1].lee == 's' &&
      inputWP[pos].lee == '9' &&
      inputWP[pos+1].lee == 'y') { /* 세요 = 시 + 어요 */
    inputWP[kimmolen] =
       MakeOneWord(lnode,mnode,'x','o','i') ; /* '이' */

    oneWSM = MakeOneWSM ( llnode , mnode , kimmolen ,
                          kimmolen , 0 , mnode , 'L') ;

    SubMorA('e','0','L',oneWSM) ;
  }

}/*-----------End of irr_ae-----------*/

/*-------------*
 *             *
 * '해' 불규칙 *
 *             *
 *-------------*/

PRIVATE void irr_hae(pos)
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT llnode ; 
  SINT lnode  ; 
  SINT rnode  ; 
  SINT mnode  ; 
  WSM oneWSM  ; 

  llnode = inputWP[pos-1].left           ;
  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* '어' 삽입 */
  inputWP[kimmolen] = MakeOneWord(mnode,rnode,'o','o','e') ;  
  
  oneWSM = MakeOneWSM( mnode , rnode , kimmolen , 
                kimmolen , mnode , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("AE-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; 

  /* '아' 삽입 */
  inputWP[kimmolen] = 
	  MakeOneWord(lnode,mnode,'x','o','a') ; 
  
  oneWSM = MakeOneWSM(llnode , mnode , kimmolen , 
                       kimmolen , 0 , mnode , 'L') ; 
#ifdef DEBUG
  printf("AE-Irr2\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_hae-----------*/

/*-----------------*
 *                 *
 * '이+어=여' 축약 *
 *                 *
 *-----------------*/

PRIVATE void irr_ieo(pos)  
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* 반모음 'y' --> 모음 'i' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'o','o','i') ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("I+EO=YEO-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('d','0','L',oneWSM) ; /* 'd' : 조사 , 선어말 어미 , 용언 */

}/*-----------End of irr_ieo-----------*/

/*--------------------------*
 *                          *
 * `오+아=와/우+어=워' 축약 *
 *                          *
 *--------------------------*/

PRIVATE void irr_wa_we(pos)  
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  UCHAR in ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* '와/워' --> '오/우' */
  in = (inputWP[pos+1].lee == 'a') ? 'o' : 'u' ; 

  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'o','o', in) ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("O/U-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_wa_we-----------*/

/*--------------------------*
 *                          *
 * `아/어+아/어=아/어' 축약 *
 *                          *
 *--------------------------*/

PRIVATE void irr_a_e(pos) 
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* 초성 copy    */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '아/어' 삽입 */
  inputWP[kimmolen+1] = 
	 MakeOneWord(mnode,rnode,'x','o', inputWP[pos+1].lee) ; 
  
  oneWSM = MakeOneWSM( lnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("A/EO-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_A/EO-----------*/

/*-----------------*
 *                 *
 * `외+어=왜' 축약 *
 *                 *
 *-----------------*/

PRIVATE void irr_w8(pos)  
int pos ;                    /* 불규칙 추정 position idx */
{
  SINT llnode ; 
  SINT lnode  ; 
  SINT rnode  ; 
  SINT mnode  ; 
  WSM oneWSM  ; 

  llnode = inputWP[pos-1].left           ;
  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* '어' 삽입 */
  inputWP[kimmolen] = MakeOneWord(mnode,rnode,'o','o','e') ;  
  
  oneWSM = MakeOneWSM( mnode , rnode , kimmolen , 
                kimmolen , mnode , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("WAE-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; 

  /* '이' 삽입 */
  inputWP[kimmolen] = MakeOneWord(lnode,mnode,'x','o','i') ; 
  
  oneWSM = MakeOneWSM(llnode , mnode , kimmolen , 
                       kimmolen , 0 , mnode , 'L') ; 
#ifdef DEBUG
  printf("WAE-Irr2\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

}/*-----------End of irr_ae-----------*/

/*----------------------------------------------------------------*
 *                                                                *
 *            '이' 축약 : 무종성 체언 + (이) + 어미               *
 *                                                                *
 *  예 : 사과라면 --> 사과 + 이 + 라면                            *
 *                                                                *
 *		(`라면'어미를 찾아 넣고 `이'를 넣는다)            *
 *                                                                *
 *----------------------------------------------------------------*/

PRIVATE void irr_i(pos) 
int pos ;                    /* 불규칙 추정 position idx */
{
  UCHAR entry[__MAXMORPH__] ;
  UCHAR info[__NUMATTR__][__ATTRSIZE__] ; 
  double probs[__NUMATTR__] ; 
  int numInfo     ;
  SINT lnode      ; 
  SINT rnode      ; 
  SINT mnode      ; 
  WSM  oneWSM     ; 
  int idx         ; 

  lnode = inputWP[pos].left  ;
  rnode = inputWP[pos].right ;
  mnode = lnode + 1          ;

  /* 초성 copy    */
  inputWP[kimmolen] = 
	 MakeOneWord(mnode,rnode,'o','x',inputWP[pos].lee) ;  

  oneWSM = MakeOneWSM(mnode , rnode,kimmolen ,
		      kimmolen , mnode , endNodeValue , 'R') ; 
		       
  WSMPush(oneWSM) ; 

#ifdef DEBUG
  printf("I-Deletion\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  do {
	MakeWord(entry) ; 
	LookupDict(entry,info,probs,&numInfo) ; 

    for(idx = 0 ; idx < numInfo ; ++idx) {
      if ((info[idx][0] == F_T) && (info[idx][1] == _ECQ_ || 
		   info[idx][1] == _ECS_ || info[idx][1] == _ECX_ || 
		   info[idx][1] == _EF_  || info[idx][1] == _EFP_))
	     PutEntry(entry,&info[idx][0],&probs[idx],1 ) ; 
    } 

  } while(GetNextWordPos('R')) ;        /* Dir Searching */

  WSMPop() ; 
  
  oneWSM = MakeOneWSM(lnode , mnode,kimmolen ,
		      kimmolen , lnode , mnode , 'L') ; 

		      /* lnode .. mnode까지라는 것을 위해서 */

  WSMPush(oneWSM) ; 
  PutEntry(jcp,jcpInfo,&jcpProb,1) ; /* 서술격 조사 '이' 넣는다 */
  WSMPop() ; 

}/*-----------End of irr_i-----------*/

/*------------------------------------------------------------------*
 *                                                                  *
 *  SubMorA  : Sub-Morphological-Analysis                           *
 *                                                                  *
 *  Input    : cat : category                                       *
 *             irr : irregular                                      *
 *		       dir : direction                                      *
 *             one : one word stack manager                         *
 *  Output   : NULL                                                 *
 *                                                                  *
 *  Function : direction에 따라 word를 만들며 (cat,irr)을 만족하는  *
 *	           형태소가 발견되면 morphP[]에 넣는다.                 *
 *                                                                  *
 *------------------------------------------------------------------*/

PRIVATE void SubMorA(cat,irr,dir,one) 
UCHAR cat ;                         /* 찾고있는 category       */
UCHAR irr ;                         /* 그것의 irregular        */ 
char dir  ;                         /* 형태소를 찾는 방향      */
WSM one   ;                         /* One Word Stack Mangager */
{
  UCHAR entry[__MAXMORPH__] ;
  UCHAR info[__NUMATTR__][__ATTRSIZE__] ; 
  double probs[__NUMATTR__] ; 
  int numInfo               ;

  int idx ; 

  WSMPush(one) ;             /* 새로운 environment 시작   */

  do {
	MakeWord(entry) ; 
	LookupDict(entry,info,probs,&numInfo) ; 

    for(idx = 0 ; idx < numInfo ; ++idx) {
     if (info[idx][0] == F_T) {
	   if (CatCheck(info[idx][1],cat) &&
	    info[idx][2] == irr)
        PutEntry(entry,&info[idx][0],&probs[idx],1) ; 
     } else if (info[idx][0] == F_P)
	    PutEntry(entry,&info[idx][0],&probs[idx],1) ; 
    } 
  } while(GetNextWordPos(dir)) ;        /* Dir Searching */

  WSMPop() ; 

}/*-----------End of SubMorA-----------*/

/*----------------------------------------------------------*
 *                                                          *
 * Category Checking : Category가 cat에 포함되는가 ?        *
 *                                                          *
 * Function :  cat : 'e' : 어미                             *
 *                   'd' : 서술격 조사 , 선어말 어미 , 용언 *
 *                   'p' : 용언                             *
 *                                                          *
 *             category가 옳은가 ?                          *
 *                                                          *
 *----------------------------------------------------------*/

PRIVATE int CatCheck(category,cat)
UCHAR category ; 
UCHAR cat ; 
{
  switch(cat) {

   case 'e' : if(category >= _ECQ_ &&
	           category <= _EF_) return 1 ; 
              return 0 ; 
     	      break ; 

   case 'd' : if ( category == _JCP_ || 
				   category == _EFP_ ) return 1 ; 

   case 'p' : if(category >= _PV_ &&
	             category <= _PX_) return 1 ; 
 			  if(category == _XPV_ ||
				 category == _XPA_) return 1 ; 
              return 0 ; 
              break ;
  }
  return 0 ; /* 잘못된 input */

}/*-----------End of CatCheck----------*/

/*--------------------------------------------------*
 *                                                  *
 * MakeOneWord : InputWP의 top에 넣을 Word를 만든다 *
 *                                                  *
 *--------------------------------------------------*/

PRIVATE InputW MakeOneWord(lef,righ,star,en,le)
SINT lef  ; 
SINT righ ; 
UCHAR star ; 
UCHAR en  ; 
UCHAR le  ; 
{
  InputW oneWord       ;

  oneWord.left  = lef  ; 
  oneWord.right = righ ; 
  oneWord.start = star ;  /* character copy   */
  oneWord.end   = en   ;
  oneWord.lee   = le   ;
                          /* 불규칙 정보 없음 */
  return oneWord       ;
}/*----------End of MakeOneWord-----------*/



