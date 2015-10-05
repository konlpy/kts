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

EXTERN WSM wordSM[__NUMWSM__] ;   /* ���ο� ȯ���� Manage�ϱ����� Pool */  
EXTERN InputW inputWP[__EOJEOLLENGTH__] ; 
EXTERN Morph  morphP[__NUMMORPH__] ; 
EXTERN int smPtr  ; 
EXTERN int morPtr ; 
EXTERN int kimmolen  ; 
EXTERN SINT endNodeValue ; 
EXTERN UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /*ǰ�� ���� table*/
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
EXTERN UCHAR jcp[]     ; /* ������ ���� `��'            */
EXTERN UCHAR jcpInfo[1][__ATTRSIZE__] ; 
EXTERN double jcpProb  ; /* Prob(Word = `��'|Tag = jcp) */

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
PRIVATE void SubMorA()       ; /* �ұ�Ģ ó���� ���� M-A */ 
PRIVATE int CatCheck()       ;


PRIVATE void irr_eu()    ; /* 0  '��' �ұ�Ģ     */ 
PRIVATE void irr_l()     ; /* 1  '��' Ż��       */
PRIVATE void irr_s()     ; /* 2  '��' Ż��       */
PRIVATE void irr_d()     ; /* 3  '��' �ұ�Ģ     */
PRIVATE void irr_b()     ; /* 4  '��' �ұ�Ģ     */
PRIVATE void irr_leu()   ; /* 5  '��' �ұ�Ģ     */
PRIVATE void irr_leo()   ; /* 6  '��' �ұ�Ģ     */
PRIVATE void irr_u()     ; /* 7  '��' �ұ�Ģ     */
PRIVATE void irr_yeo()   ; /* 8  '��' �ұ�Ģ     */
PRIVATE void irr_geola() ; /* 9  '�Ŷ�' �ұ�Ģ   */
PRIVATE void irr_neola() ; /* 10 '�ʶ�' �ұ�Ģ   */
PRIVATE void irr_h1()    ; /* 11 '��' �ұ�Ģ     */
PRIVATE void irr_h2()    ; /* 12 '��' �ұ�Ģ     */
PRIVATE void irr_ae()    ; /* 13 '��,��' ���    */
PRIVATE void irr_hae()   ; /* 14 '��' ���       */
PRIVATE void irr_ieo()   ; /* 15 '��+��=��' ü�� , ��� */
PRIVATE void irr_wa_we() ; /* 16 '��+��=��' '��+��=��'  */
PRIVATE void irr_a_e()   ; /* 17 '��+��=��' '��+��+��'  */
PRIVATE void irr_w8()    ; /* 18 '��+��=��'      */
PRIVATE void irr_i()     ; /* 19 '��' Ż��       */

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
  } ;            /* �ұ�Ģ ó�� �Լ� table */



/*------------------------------------------------*
 *                                                *
 * GetIrr : Get Irr-Position                      *
 *                                                *
 * Input   : NULL                                 *
 * Output  : NULL                                 *
 * Fuction : kimmolen�� �޾Ƽ� wordSM[]�� irr[]�� *
 *           �ұ�Ģ�̸� 'o' �ƴϸ� ' '�� ä���   *
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
	  test[lenIdx] = '\0' ; /* �ϳ��� ���� pattern�� ����� */
      find = 0 ; 
      find = BinSearch(irrTable[wordlen].irr_tbl,irrTable[wordlen].size,test,
                        irrIdx) ; /* pattern�� ���� �� irrIdx�� �־�´� */ 

    if (find) for (idx = 0 ; idx < 3 && irrIdx[idx] != -1 ; ++idx) {
	switch(irrIdx[idx]) {

	 case 1 : if (!IrrCheck_L(start-1,start+wordlen+1))
                   irrIdx[idx] = -1 ; /* �� �ұ�Ģ�� �ƴϴ� */
                  break ; 

         case 19: if (!LeftPeak(start-1)) irrIdx[idx] = -1 ; 
	     	  break ; /* '��' Ż�� ������ �ƴϴ� */

	 case 15: if (test[0] == 'y' && !LeftCheck(start-1)) {
	           irrIdx[idx] = -1 ; 
		       break ; 
              } /* ������ �ʼ��� ���� �ȵȴ� */ 

	 case 16: if (test[0] == 'w' && !LeftCheck(start-1)) {
		       irrIdx[idx] = -1 ; 
		       break ; 
              } /* ������ �ʼ��� ���� �ȵȴ� */

     case 0 :
	 case 17: if (!irrCheck_E(start+wordlen+1))
	           irrIdx[idx] = -1 ; /* ��� ����� �ƴϴ� */
               break ; 
     default : break ; 
    }

    if(irrIdx[idx] != -1) {
	  irrType = irrIdx[idx] ; 
	  inputWP[start+lenIdx-1+irrShift[irrType]].irr[irrType] = 'o' ; 
    }

   } /* end of for idx */
  } /* end of for start */ 
 } while( ++wordlen < 5 ) ; /* pattern �� �ִ� ���� */ 


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
 * Fuction : �ұ�Ģ code table���� �ұ�Ģ�� �� �� �ִ� ���� ã�´� *
 *                                                                 *
 *-----------------------------------------------------------------*/

PRIVATE int BinSearch(irr_t,arrsize,test,irrIdx)
IRR irr_t[]  ;                 /* �ұ�Ģ pattern�� ���� �ִ� table */  
int arrsize  ;                 /* �ұ�Ģ table�� ũ��              */  
char test[]  ;                 /* ���� pattern                   */
int irrIdx[] ;                 /* ã���� �� �װ��� �ұ�Ģ code Idx */
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
 * Function   : '��'Ż�� �����ΰ��� checking�ϱ� ���� module     *
 *              �����ʿ� { N | n | B | si | o | s9 } �� �ϳ��ΰ� *
 *              ���ʿ� character�� ���� �� �ʼ��� ������ �ȵȴ�  *
 *                                                               *
 *---------------------------------------------------------------*/

PRIVATE int IrrCheck_L(wordL,wordR)
int wordL    ;                       /* ���� ���� ����   position */
int wordR    ;                       /* ���� ���� ������ position */ 
{
  int lcheck = 1 ;                   /* ���� check                */
  int rcheck = 0 ;                   /* ������ check              */

  if (wordR >= kimmolen) return 0 ;  /* �� �κ��� BinSearch�� �̸�*/
									 /* ���ص� �Ǵ� �κ�          */ 
  if (strchr("NnBoL",inputWP[wordR].lee)) rcheck = 1 ; 
  if (wordR < kimmolen - 1 && inputWP[wordR].lee == 's')
	if (inputWP[wordR+1].lee == 'i' || inputWP[wordR+1].lee == 'y' ||
		inputWP[wordR+1].lee == '9') rcheck = 1 ; /* �� , �� , �� */
  
  if (wordL >= 0) 
	if (strchr(Is_cho,inputWP[wordL].lee)) lcheck = 0 ; 

  return lcheck && rcheck ; 
}/*-------------End of IrrCheck_L------------------*/


/*-----------------------------------------------*
 *                                               *
 * LeftCheck : Left Checking                     *
 *                                               *
 * Input      : wordL                            *
 * Output     : 0 : ���ʿ� �ʼ��� ����           *
 *              1 : NULL Ȥ�� �ʼ��� �ƴ� ��     *
 * Function   : ������ NULL --> Yes              *
 *              ���ʿ� �ʼ��� ���� No , else Yes *
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
 * Output     : 0 : ������ �߼��� �ƴ� ��                      *
 *              1 : default                                    *
 * Function   : ������ ü���ΰ��� ����                         *
 *              ������ NULL --> ü���� ���ٴ� �� : No          *
 *              ���ʿ� �߼��� �ƴϸ� No : �������� �ƴ϶�� �� *
 *                                                             *
 *-------------------------------------------------------------*/

PRIVATE int LeftPeak(wordL)
int wordL ; 
{

/* 32�� : 32 + �� + ��  

  if (wordL < 0) return 0 ; 
*/

  if (wordL < 0) return 1 ; 
  else if (!strchr(Is_jung,inputWP[wordL].lee)) return 0 ; 
  return 1 ; 

}/*-------------End of LeftPeak--------------------*/


/*------------------------------------------------------------*
 *                                                            *
 * irrCheck_E : Irr Checking : `E' `��'�ΰ� ����              *
 *                                                            *
 * Input      : wordR                                         *
 * Output     : 0 : default                                   *
 *              1 : { ���� �� | �ʼ� | `��' | `��' |          *
 *                       `��' | `��' | `��' | `��' }          *
 * Function   : ��� '��/��'�� �����ϴ°� checking�ϴ� module *
 *                                                            *
 *------------------------------------------------------------*/

int irrCheck_E(wordR)
int wordR    ;                       /* ���� ���� ������ position */
{
  if (wordR >= kimmolen) return 1 ;  /* ���� ��  : OK             */ 
  if (strchr("wioghqndlmbrsvfjzcktpVy",inputWP[wordR].lee)) return 1 ;        
        /* �ʼ� Ȥ�� '��' , `��' , `��' , `��' , `��' , `��' OK   */
  return 0 ; 
}/*------------End of irrCheck_E------------------*/


/*----------------------------------------------------------------*
 *----------------------------------------------------------------*
 *                    �ұ�Ģ ó�� Routines                        *
 *----------------------------------------------------------------*
 *----------------------------------------------------------------*/


/*-----------*
 *           *
 * `��' Ż�� * 
 *           *
 *-----------*/

PRIVATE void irr_eu(pos) 
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* ù �ʼ� copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '��' ����     */
  inputWP[kimmolen+1] = MakeOneWord(mnode,rnode,'x','o','_') ; 
  
  oneWSM = MakeOneWSM( lnode , rnode , kimmolen + 1 , 
                       kimmolen , 0 , rnode , 'L') ; 

#ifdef DEBUG
  printf("EU-Irr\n") ; 
  DisplayWordPool(kimmolen+1) ; 
#endif

  SubMorA('p','0','L',oneWSM) ; /* ��� , �ұ�ĢX , ���� */

}/*-----------End of irr_eu-----------*/


/*-----------*
 *           *
 * '��' Ż�� *
 *           *
 *-----------*/

PRIVATE void irr_l(pos)
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* ù �߼� copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '��' ����     */
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
 * '��' �ұ�Ģ *
 *             *
 *-------------*/

PRIVATE void irr_s(pos) 
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* ù �߼� copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '��' ����     */
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
 * '��' �ұ�Ģ * 
 *             *
 *-------------*/

PRIVATE void irr_d(pos)   
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* '��' --> '��' */
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
 * `��' �ұ�Ģ *
 *             *
 *-------------*/

PRIVATE void irr_b(pos)  
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* '��/��' --> '��' */
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
 * '��' �ұ�Ģ *
 *             *
 *-------------*/

PRIVATE void irr_leu(pos)  
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;
  mnode = inputWP[pos].left              ;

  /* ���� '��' --> �ʼ� '��' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'x','x','l') ;  

  /* �ʼ� '��' --> �߼� '��' */
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
 * '��' �ұ�Ģ *
 *             *
 *-------------*/

PRIVATE void irr_leo(pos) 
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;

  /* '����' --> '��' */
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
 * '��' ��Ģ '��' --> 'Ǫ��' *
 *                           *
 *---------------------------*/

PRIVATE void irr_u(pos)  
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* ù �ʼ� copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '��' ����     */
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
 * '��' �ұ�Ģ *
 *             *
 *-------------*/

PRIVATE void irr_yeo(pos)   
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;

  /* '����' --> '��' */
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
 * '�Ŷ�' �ұ�Ģ *
 *               *
 *---------------*/

PRIVATE void irr_geola(pos)
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT llnode ; /* left left node */
  WSM    oneWSM  ; 

  llnode = inputWP[pos-2].left           ; 
  lnode = inputWP[pos-1].left            ; 
  rnode = inputWP[pos].right             ;

  /* 'X��' --> 'X' */
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
 * '�ʶ�' �ұ�Ģ *
 *               *
 *---------------*/

PRIVATE void irr_neola(pos) 
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ; 
  rnode = inputWP[++pos].right           ;

  /* '����' --> '��' */
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
 * '��' �ұ�Ģ 1 *
 *               *
 *---------------*/

PRIVATE void irr_h1(pos)    
int pos ;                    /* �ұ�Ģ ���� position idx */
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

  /* �߼� copy */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'x','x',inputWP[pos].lee) ;  

  /* '��' ���� */
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
 * '��' �ұ�Ģ 2 *
 *               *
 *---------------*/

PRIVATE void irr_h2(pos)   
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT llnode  ; 
  SINT lnode   ; 
  SINT rnode   ; 
  SINT mnode1  ; 
  SINT mnode2  ; 
  UCHAR in  ;
  int plusminus = 0 ;        /* ���� ������ �缺/���� */
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

  /* �߼� '��/��' ���� */
  in = (plusminus == 1) ? 'a' : 'e' ; 
  inputWP[kimmolen] = MakeOneWord(mnode2,rnode,'o','o',in) ; 
  
  oneWSM = MakeOneWSM( mnode2 , rnode , kimmolen , 
                       kimmolen , mnode2 , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("H 2-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; /* ��� */

  /* �߼� '��/��' ���� */
  in = (plusminus == 1) ? 'a' : 'e' ;
  inputWP[kimmolen] = MakeOneWord(lnode,mnode1,'x','x',in) ; 

  /* ���� '��' ���� */ 
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
 * '��/��' ��� *
 *              *
 *--------------*/

PRIVATE void irr_ae(pos)  
int pos ;                    /* �ұ�Ģ ���� position idx */
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

  /* '��' ���� */
  inputWP[kimmolen] = MakeOneWord(mnode,rnode,'o','o','e') ;  
  
  oneWSM = MakeOneWSM( mnode , rnode , kimmolen , 
                kimmolen , mnode , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("AE-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; 

  /* '��/��' Copy */
  inputWP[kimmolen] = 
	  MakeOneWord(lnode,mnode,'x','o',inputWP[pos].lee) ; 
  
  oneWSM = MakeOneWSM( llnode , mnode , kimmolen , 
                       kimmolen , 0 , mnode , 'L') ; 
#ifdef DEBUG
  printf("AE-Irr2\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('p','0','L',oneWSM) ; 

 /* ������ : �� + �� + ��� */

  if (inputWP[pos-1].lee == 's' &&
      inputWP[pos].lee == '9' &&
      inputWP[pos+1].lee == 'y') { /* ���� = �� + ��� */
    inputWP[kimmolen] =
       MakeOneWord(lnode,mnode,'x','o','i') ; /* '��' */

    oneWSM = MakeOneWSM ( llnode , mnode , kimmolen ,
                          kimmolen , 0 , mnode , 'L') ;

    SubMorA('e','0','L',oneWSM) ;
  }

}/*-----------End of irr_ae-----------*/

/*-------------*
 *             *
 * '��' �ұ�Ģ *
 *             *
 *-------------*/

PRIVATE void irr_hae(pos)
int pos ;                    /* �ұ�Ģ ���� position idx */
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

  /* '��' ���� */
  inputWP[kimmolen] = MakeOneWord(mnode,rnode,'o','o','e') ;  
  
  oneWSM = MakeOneWSM( mnode , rnode , kimmolen , 
                kimmolen , mnode , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("AE-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; 

  /* '��' ���� */
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
 * '��+��=��' ��� *
 *                 *
 *-----------------*/

PRIVATE void irr_ieo(pos)  
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* �ݸ��� 'y' --> ���� 'i' */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,rnode,'o','o','i') ;  

  oneWSM = MakeOneWSM( lnode , rnode , kimmolen , 
                       kimmolen , 0 , rnode , 'L') ; 
#ifdef DEBUG
  printf("I+EO=YEO-Irr\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('d','0','L',oneWSM) ; /* 'd' : ���� , ��� ��� , ��� */

}/*-----------End of irr_ieo-----------*/

/*--------------------------*
 *                          *
 * `��+��=��/��+��=��' ��� *
 *                          *
 *--------------------------*/

PRIVATE void irr_wa_we(pos)  
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  UCHAR in ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;

  /* '��/��' --> '��/��' */
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
 * `��/��+��/��=��/��' ��� *
 *                          *
 *--------------------------*/

PRIVATE void irr_a_e(pos) 
int pos ;                    /* �ұ�Ģ ���� position idx */
{
  SINT lnode ; 
  SINT rnode ; 
  SINT mnode ; 
  WSM    oneWSM  ; 

  lnode = inputWP[pos].left              ;
  rnode = inputWP[pos].right             ;
  mnode = (lnode + rnode)/2              ;

  /* �ʼ� copy    */
  inputWP[kimmolen] = 
	 MakeOneWord(lnode,mnode,'o','x',inputWP[pos].lee) ;  

  /* '��/��' ���� */
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
 * `��+��=��' ��� *
 *                 *
 *-----------------*/

PRIVATE void irr_w8(pos)  
int pos ;                    /* �ұ�Ģ ���� position idx */
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

  /* '��' ���� */
  inputWP[kimmolen] = MakeOneWord(mnode,rnode,'o','o','e') ;  
  
  oneWSM = MakeOneWSM( mnode , rnode , kimmolen , 
                kimmolen , mnode , endNodeValue , 'R') ; 
#ifdef DEBUG
  printf("WAE-Irr1\n") ; 
  DisplayWordPool(kimmolen) ; 
#endif
  
  SubMorA('e','0','R',oneWSM) ; 

  /* '��' ���� */
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
 *            '��' ��� : ������ ü�� + (��) + ���               *
 *                                                                *
 *  �� : ������ --> ��� + �� + ���                            *
 *                                                                *
 *		(`���'��̸� ã�� �ְ� `��'�� �ִ´�)            *
 *                                                                *
 *----------------------------------------------------------------*/

PRIVATE void irr_i(pos) 
int pos ;                    /* �ұ�Ģ ���� position idx */
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

  /* �ʼ� copy    */
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

		      /* lnode .. mnode������� ���� ���ؼ� */

  WSMPush(oneWSM) ; 
  PutEntry(jcp,jcpInfo,&jcpProb,1) ; /* ������ ���� '��' �ִ´� */
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
 *  Function : direction�� ���� word�� ����� (cat,irr)�� �����ϴ�  *
 *	           ���¼Ұ� �߰ߵǸ� morphP[]�� �ִ´�.                 *
 *                                                                  *
 *------------------------------------------------------------------*/

PRIVATE void SubMorA(cat,irr,dir,one) 
UCHAR cat ;                         /* ã���ִ� category       */
UCHAR irr ;                         /* �װ��� irregular        */ 
char dir  ;                         /* ���¼Ҹ� ã�� ����      */
WSM one   ;                         /* One Word Stack Mangager */
{
  UCHAR entry[__MAXMORPH__] ;
  UCHAR info[__NUMATTR__][__ATTRSIZE__] ; 
  double probs[__NUMATTR__] ; 
  int numInfo               ;

  int idx ; 

  WSMPush(one) ;             /* ���ο� environment ����   */

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
 * Category Checking : Category�� cat�� ���ԵǴ°� ?        *
 *                                                          *
 * Function :  cat : 'e' : ���                             *
 *                   'd' : ������ ���� , ��� ��� , ��� *
 *                   'p' : ���                             *
 *                                                          *
 *             category�� ������ ?                          *
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
  return 0 ; /* �߸��� input */

}/*-----------End of CatCheck----------*/

/*--------------------------------------------------*
 *                                                  *
 * MakeOneWord : InputWP�� top�� ���� Word�� ����� *
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
                          /* �ұ�Ģ ���� ���� */
  return oneWord       ;
}/*----------End of MakeOneWord-----------*/



