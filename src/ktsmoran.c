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

WSM    wordSM[__NUMWSM__]        ; /* ���ο� ȯ�� Manage Pool */  
InputW inputWP[__EOJEOLLENGTH__] ; /* Input Word Pool         */  
Morph  morphP[__NUMMORPH__]      ; /* Morph Pool              */
int    displayP[__NUMMORPH__]    ; /* Display Pool for Mor-An */

int disPtr = -1 ; /* Display Top Pointer of displayP      */
int smPtr = -1  ; /* Stack Manager Top Pointer of wordSM  */
int morPtr = -1 ; /* Morph Pool Top Pointer of morphP     */

int kimmolen           ; /* �Է� string�� ����            */
SINT endNodeValue      ; /* kimmolen * __SKIPVALUE__      */
UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /* ���� tbl */
PUBLIC DBM* dictPtr    ; /* ������ ����Ű�� pointer       */

/* 
 * ���� ���̴� Tags
 */
EXTERN UCHAR _FINAL_          ; /* ���� , ���� ������              */ 
EXTERN UCHAR _INITI_          ; /* ���� , ���� ó��                */
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
PUBLIC UCHAR _INT_            ; /* �Ű� ���� '��'                  */
PUBLIC UCHAR jcp[] = "i"      ; /* ������ ���� '��'                */
PUBLIC UCHAR jcpInfo[1][__ATTRSIZE__] ; /* Category & irregular    */ 
PUBLIC double jcpProb =1.0    ; /* Prob(Word = '��'|Tag = jcp)     */
 
EXTERN int tagfreq[__NUMOFTAG__] ; /* ���¼� �߻� Frequency    */

EXTERN void (*irr_func[])()   ; /* �ұ�Ģ ó�� �Լ� Table          */
EXTERN void GetIrr()          ; /* �ұ�Ģ ó�� Position Finding    */
EXTERN int kimmo2ks()         ; /* �̼��� code --> KS5601          */
EXTERN void ReverseString()   ; /* string�� �Ųٷ� �����          */ 
EXTERN int SplitStr()         ; /* Encoding�� ������ Decode�Ѵ�    */  
EXTERN int DecodePreMor()     ; /* ���¼� ��м��� ������ Decode   */
EXTERN void PredictUnknown()  ; /* Unknown Word�� �����Ѵ�         */
EXTERN int MrgnFreq()         ; /* Marginal Frequency Word_i       */

PUBLIC void LoadTranTbl()     ; /* ǰ�� ���� table�� �����´�      */
PUBLIC int PreMorphAn()       ; /* Pre ���¼� �м�                 */
PUBLIC int MorphAn()          ; /* ���¼� �м�                     */ 
PUBLIC WSM MakeOneWSM()       ; /* Make one wsm record             */ 
PUBLIC void MakeWord()        ; /* Make one InputW record          */ 
PUBLIC void LookupDict()      ; /* ���� Lookup ( morphP���� ���� ) */
PUBLIC void PutEntry()        ; /* �������� ã�� ������ �ִ´�     */ 
PUBLIC int GetNextWordPos()   ; /* �������� ã�� ���¼Ҹ� ����     */ 
PUBLIC void WSMPush()         ; /* Word-Stack-Manager Push         */ 
PUBLIC void WSMPop()          ; /* Word-Stack-Manager Pop          */
PUBLIC int GetLNodeIdx()      ; /* ���¼��� ���� Node Idx return   */
PUBLIC UCHAR GetLeeOnRIdx()   ; /* Right Index�� �̼��� �ڵ�       */
PUBLIC void InitTag()         ; /* Global Tags Initialization      */ 
PUBLIC void MorPush()         ; /* Morph Push                      */ 
PUBLIC void ConTest()         ; /* Connectibility Test             */
PUBLIC Morph MakeMor()        ; /* �ϳ��� ���¼� record �����     */ 

#ifdef DEBUG
PUBLIC void DisplayWordPool() ; /* Word Pool Display               */
PUBLIC void DisplayMorph()    ; /* Morph Pool Display              */
PUBLIC void OutputMorph()     ; /* Display Pool�� KS5601�� Display */
PUBLIC void Out2Han()         ; /* ���¼Ҹ� KS5601�� Display       */
#endif

PRIVATE void GetSE()          ; /* ���۰� ���� ���ɼ��� ���´�     */  
PRIVATE void MakeWP()         ; /* Word Pool�� �����              */ 
PRIVATE void RealLookupDict() ; /* ������ �������� ���¼� ã��     */
PRIVATE void IdxCopy()        ; /* Morph Idx Copy until -1         */
PRIVATE SINT GetNBegin()      ; /* Get Next-Begin-Position(inputW) */ 
PRIVATE SINT GetNEnd()        ; /* Get Next-End-Position  (inputW) */
PRIVATE SINT GetFirstBegin()  ; /* Get First-Begin-Position        */ 
PRIVATE void GetPreProbs()    ; /* ���¼� ��м��� P(word|tag)     */



/*----------------------------------------------------------------*
 *                                                                *
 * PreMorphAn : �Է� ������ �޾� ���¼� �м��� �Ѵ�               *
 *                                                                *
 * Input   : kimmo                                                *
 * Output  : NULL                                                 *
 * Fuction : morphP[]�� ���¼� Trellis Structure�� ����� �ִ´�  *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC int PreMorphAn(kimmo)
UCHAR kimmo[]  ;                                 /* string : ���� */
{
  int   anal_return  ; 
  UCHAR starter[100] ;     /* ���¼� ������ �����Ѱ� ?    */ 
  UCHAR ender[100]   ;     /* ���¼� ���� �����Ѱ� ?      */
  UCHAR hanguel[140] ;     /* �̼��� �ڵ忡 �����ϴ� �ѱ� */ 
  
  disPtr = -1 ; 
  morPtr = -1 ;      
  smPtr = -1  ; 

  kimmolen = strlen(kimmo) ; 
  endNodeValue = (SINT) kimmolen  * __SKIPVALUE__ ; 

  GetSE(kimmo,kimmolen,starter,ender) ;          /* ���۰� ���� ���ɼ� */  
  MakeWP(kimmo,kimmolen,starter,ender) ;         /* Word Pool�� ����� */ 
  GetIrr() ;                                     /* �ұ�Ģ ���� ��´� */

  anal_return = MorphAn() ;  /* ���¼� �м� */

  if(!anal_return) {
#ifdef DEBUG
    kimmo2ks(kimmo,hanguel) ; 	
	fprintf(stderr,"Error : Unknown Word : %s\n",hanguel) ; 
#endif
  } 

#ifdef DEBUG
  DisplayMorph()      ; /* Morph Pool�� ����  */
  OutputMorph(morPtr) ; /* ���¼� �м� ���   */
#endif
  return anal_return  ; /* 
			 * 1 :   Known Word
                         * 0 : Unknown Word
			 */

}/*--------End of PreMorphAn------------*/

/*----------------------------------------------------------------*
 *                                                                *
 * GetSE   : �̼��� code�� �޾�                                   *
 *           ���¼� ���� , �� ���� Position Marking               *
 *                                                                *
 * Input   : kimmo   : �̼��� code                                *
 *           starter : ���¼� ���� Position Array                 *
 *           ender   : ���¼� �� Position Array                   *
 * Output  : NULL                                                 *
 * Fuction : starter �� ender�� Marking �Ѵ�.                     *
 *                                                                *
 *----------------------------------------------------------------*/

PRIVATE void GetSE(kimmo,strl,starter,ender)
char kimmo[]    ;                          /* kimmo code          */
int strl        ;                          /* kimmo code�� ����   */
UCHAR starter[] ;                          /* ������ �����ϸ� 'o' */
UCHAR ender[]   ;                          /* ���� �����ϸ� 'o'   */
{
  int i ; 

  memset(starter,(int) ' ' , strl) ; 
  memset(ender,(int) ' ' , strl) ; 

  for (i = 0 ; i < strl ; ++i) {
   if (strchr(Is_cho,kimmo[i])) {             /*    �ʼ�    */
      starter[i] = 'o' ; 
      ender[i] = 'x' ;                        /* not allowd */
   } else if (strchr(Is_half,kimmo[i])) {     /*  �� ����   */
      starter[i] = 'o' ; 
      ender[i] = 'x' ;                        /* not allowd */
   } else if (strchr(Is_head,kimmo[i])) {
      starter[i] = ender[i] = 'o' ;  
   } else if (strchr(Is_jong,kimmo[i])) {     /*    ����    */
      starter[i] = 'x' ; 
      ender[i] = 'o' ; 
   } else starter[i] = ender[i] = 'o' ;       /*    �߼�    */  
  }


}/*----------End of GetSE----------------*/

/*----------------------------------------------------------------*
 *                                                                *
 * MakeWP  : �̼��� code , ���� , �� ���� Position Mark Array��   *
 *           �޾� Word Pool�� �����.                             *
 *                                                                *
 * Input   : kimmo   : �̼��� code                                *
 *           starter : ���¼� ���� Position Array                 *
 *           ender   : ���¼� �� Position Array                   *
 * Output  : NULL                                                 *
 * Fuction : inputWP[]�� �̼��� code, starter , ender copy        *
 *                                                                *
 *----------------------------------------------------------------*/
 
PRIVATE void MakeWP(kimmo,strl,starter,ender)
char kimmo[]    ;                      /* input kimmo code */
int strl        ;                      /* kimmocode length */
UCHAR starter[] ;         /* ������ �� �ִ� ���� ���� ���� */
UCHAR ender[]   ;           /* ���� �� �ִ� ���� ���� ���� */
{
  SINT idx ;                          /* inputWP[]�� index */

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
 * LoadTranTbl  : ���¼��� Connectibility Matrix�� �����Ѵ�. *
 *	                                                     *
 * Input   : fname  : Connectibility Matrix File Name        *
 * Output  : NULL                                            *
 * Fuction : tranTable[][]�� Connectibility Matrix ����      *
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
 * MorphAn : ���¼� �м���                                          *
 *                                                                  *
 * Input   : NULL                                                   *
 * Output  : NULL                                                   *
 * Fuction : inputW�� ������ ���¼� �м��Ͽ� �� ���¼� �м� �����  *
 *           MorphP�� �ִ´�                                        *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC int MorphAn()
{
  int lidx            ;                /* ���� Node�� Pool Index   */
  int irrIdx          ;                /* �ұ�Ģ code�� indx       */
  SINT temp = -1      ; 
  UCHAR temp2 = '\0'  ; 
  SINT maxwright      ; 
  SINT connect[__NUMCONNECTION__] ;  /* connect �Ǵ� idx           */
  WSM oneWSM ;                       /* ���ο� ȯ�� �ϳ�           */ 
  Morph oneMor ;                     /* ���¼� �м��Ȱ� �ϳ�       */
  UCHAR entry[__MAXMORPH__] ;        /* �ѹ� �õ��� ���� entry     */  
  UCHAR info[__NUMATTR__][__ATTRSIZE__] ;
  int numInfo         ;              /* entry�� ���� ������ ����   */

  double probs[__NUMATTR__] ;          /* Ȯ�� : P(word|tag),P(tag,word)*/

  /* �ʱ�ȭ : WSM �� morphP */ 

  oneWSM = MakeOneWSM(
   GetFirstBegin(kimmolen) , /* begin : ù��° ���¼� ���۹�ȣ      */
   endNodeValue ,            /* end   : ���� ȯ���� ���¼� end node */ 
   (SINT ) kimmolen - 1 ,    /* top   : ���� Morph Pool�� topPosIdx */ 
   0 ,                       /* bottom: ���� Morph Pool�� bottomIdx */ 
   0 ,                       /* ini   : ���� environment�� ini node */
   endNodeValue ,            /* fin   : ���� environment�� fin node */
   'R'                       /* direct: pos .. pos + i : word ����  */
  ) ; 
  WSMPush(oneWSM) ;          /* ���ο� environment ���� */

  oneMor = MakeMor( 
   0            ,            /* ȯ�� : ���� ó��                     */
   'x'          ,            /* ������ space�� �ִ°�                */
   endNodeValue ,            /* ���¼� ���� node                     */
   endNodeValue ,            /* ���¼� �ϳ��� ������ node            */
   endNodeValue ,            /* ���� ���¼� �ؼ� ����� ������ node  */
   &temp ,                   /* ���� ���¼� ����Ű�� index : -1 : �� */
   &temp2 ,                  /*  NULL                                */
   _FINAL_ ,                 /* ���¼� index                         */
   '0' ,                     /* ���¼� �ұ�Ģ index                  */ 
   (double) 1.0              /* Ȯ�� : P(word|tag)                   */
  ) ; 
  MorPush(oneMor) ;          /* ������ Marker�� �ִ´� */

  do {
    MakeWord(entry) ;   

	   /*
	    *  ���� environment���� begin..end������ ���ڸ� ����� 
	    *  wordSM�� top ���� wordSM[0]�� bottom ����. 
	    */

    LookupDict(entry,info,probs,&numInfo) ; 

	   /* 
	    *  ���� lookup�� ���� �� �ܾ ã�� �͸��� 
	    *  ����� ����� �ǹǷ� wordSM�� ������ �� �ʿ�� ����.
	    *  morphP���� ã�� ���� ������ �������� �װ��� ã�´�.
            * 
	    *   info[0 .. numInfo-1]�� information ���� 
	    *  probs[0 .. numInfo-1]�� P(word|tag) ���� 
	    *   info[][0] == F_T : probs[]�� P(word|tag)�� ����Ǿ� �ִ�.
            */

    PutEntry(entry,info,probs,numInfo) ; 

           /* 
	    *  ������ morphP�� �ִ� ���� ��Ģ���� �Ѵ�.  
	    *  ���� numInfo == 0 �̶�� �ƹ��͵� ���ϰ� return.
	    *  numInfo ������� push�� �ϴµ� ���� pointer ���� , 
            *  �� wright,nidx���� handling�Ѵ�.
            */

           /* 
            *  �ұ�Ģ ó�� routine
            *  ������ �� ������ �������� �����Ѵ� 
            */

    if (wordSM[smPtr].end == endNodeValue) {
      lidx = GetLNodeIdx(wordSM[smPtr].begin) ; /* ���� index */
      for (irrIdx = 0 ; irrIdx < __NUMIRR__ ; ++irrIdx)
        if (inputWP[lidx].irr[irrIdx] == 'o') (*irr_func[irrIdx])(lidx) ; 
    }

           /* 
            *  �� ���� ������ begin..end�� morphP , wordSM , inputWP�� 
            *  �̿��Ͽ� ���Ѵ��� wordSM�� top�� ���´�.
            */

  } while(GetNextWordPos('G')) ; /* General Searching */

  /* ���⼭ INI �� �ִ´� */

  ConTest(morPtr,&maxwright,connect,(SINT) 0 , _INITI_ ) ;  

  oneMor = MakeMor(
   smPtr     , /* ȯ��                                         */
   'x'       , /* ���¼ҿ� ���¼��� ���� :        
		  �� : �̷��� : �̷��� �ϴ� (���Ⱑ �ʿ�)  */ 
   (SINT) 0  , /* INI �� ����   : 0                            */
   (SINT) 0  , /* INI �� ������ : 0                            */
   maxwright , /* ���� ���¼� �ؼ� ����� ������ node MAX      */
   connect   , /* ���� ���¼Ұ� �ִ� ���� ����Ű�� index:-1:�� */
   &temp2    , /* NULL                                         */
   _INITI_   , /* INI index                                    */
   '0'       , /* INI �ұ�Ģ code                              */
   (double) 1.0/* Ȯ�� : P(tag|word)                           */
  ) ; 
  MorPush(oneMor) ;               /* INI�� �ִ´� */

  if (maxwright == endNodeValue) return 1 ;  

                              /* Unknown Word ó�� */
  PredictUnknown() ; 
                              /*
                               * morP[]�� Unknown word ó���� �ؼ�  
                               * morP[]������ �����Ѵ�.
                               * ������ ��� ���¼Ҹ� �����Ͽ� �����Ѵ�
                               */
  return 0 ; 
              /*
               * 1 : ��� ���¼Ұ� ������ �ִ� ���̴� 
               * 0 : Unknown word ó���� �ؼ� ���� ����̴� 
               */

}/*----------End of MorphAn-----------*/

/*---------------------------------------------------------------*
 *                                                               *
 * MakeWord : word stack manager�� direct�� ���Ͽ� �̼��� code�� *
 *            �����Ͽ� �������� ã�� �� ���¼Ҹ� ����� ����.    *
 *                                                               *
 * Input    : entry                                              *
 * Output   : entry                                              *
 * Function : direct�� ���Ͽ� begin..end������ ���ڸ� �����.    *
 *            ���ڸ� ���� �� inputWP[i] .. inputWP[0]����        *
 *                                                               *
 *---------------------------------------------------------------*/

PUBLIC void MakeWord(entry)
UCHAR entry[] ; 
{
  SINT morStarter = wordSM[smPtr].begin ;
  SINT nextMorStart = wordSM[smPtr].end ; 
  SINT top = wordSM[smPtr].top ; 
  SINT inter ;                                  /* �߰� node ��ȣ */
  int temp  ; 
  int temp2 = 0  ; 

  if (wordSM[smPtr].direct == 'L') {
	inter = nextMorStart ;              /* ���� ó������ �� ������ node */ 
    while(inter != morStarter) {
	  for(temp = top ; temp >= 0 ; --temp)
		if (inputWP[temp].right == inter) {
		  entry[temp2++] = inputWP[temp].lee ; 
		  inter = inputWP[temp].left ;              /* ���� ���� ���Ѵ� */
          break ;  
        } 
    }
    entry[temp2] = '\0' ; 
    ReverseString(entry,temp2) ;              /* string�� �Ųٷ� ����� */ 
  } else { /* wordSM[smPtr].direct == 'R' */
    inter = morStarter ;                  /* ���� ó������ �� ó�� node */ 
    while(inter != nextMorStart) {
	  for(temp = top ; temp >= 0 ; --temp)
		if (inputWP[temp].left == inter) {
		  entry[temp2++] = inputWP[temp].lee ; 
		  inter = inputWP[temp].right ;           /* ������ ���� ���Ѵ� */
          break ; 
        }
    }
	entry[temp2] = '\0' ; 
  }
}/*-------End of MakeWord------------*/


/*------------------------------------------------------------------*
 *                                                                  *
 * LookupDict : ����(dictionary)���� ���¼Ұ� �ִ°� ���� ���¼Ұ�  *
 *		������ ǰ�� , Ȯ�� ���� ������ �´�.                *
 *                                                                  * 
 *   Input    : entry , info , probs , numInfo                      *
 *   Output   : info , probs , numInfo                              * 
 *   Function : entry�� ������ ���� morphP[0] ����                  * 
 *              morphP[morPtr]���̿� �߰��� �Ǹ� ������ ã�� �ʰ�   *
 *              �װ��� �ִ� ������ ���� list�� �޴´�.              *
 *              ������ ���� ����(dictionary)���� ã�´�.            *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC void LookupDict(entry,info,probs,numInfo)
UCHAR entry[]  ;                      /* �������� ã�ƾ� �� entry */  
UCHAR info[][__ATTRSIZE__] ;          /* ������ Lookup������ info */
double probs[] ;                      /* P(word|tag)              */ 
int *numInfo ;                        /* ambiguity ����           */ 
{
  int idx  ; 
  int idx2 ; 

  *numInfo = 0 ;

  for(idx = 0 ; idx <= morPtr ; ++idx)
	if (!strcmp(morphP[idx].word,entry) && morphP[idx].env == 0) {
	 for(idx2 = idx ; idx2 <= morPtr && morphP[idx2].env == 0 &&
			  !strcmp(morphP[idx2].word,entry) ; ++idx2) {

	   info[*numInfo][0] = F_T ;              /* Attribute : Tag�� ���� */
	   info[*numInfo][1] = morphP[idx2].pos ; /* POS ����               */ 
	   info[*numInfo][2] = morphP[idx2].irr ; /* �ұ�Ģ ����            */ 
	   probs[*numInfo] = morphP[idx2].prob  ; /* P(word|tag),P(tag|word)*/
	   ++*numInfo ; 
     }                         /* morphP[idx].env �� 0�϶� ���� �� �ִ� */

	 return ; 
   }

  RealLookupDict(entry,info,probs,numInfo) ; 

}/*-----------End of LookupDict-------------*/


/*-------------------------------------------------------------*
 *                                                             *
 * RealLookupDict : ���� ����(dictionary)���� ���¼Ҹ� ã�´�. *
 *                                                             *
 * Input    : entry , info , probs , numInfo                   *
 * Output   : info , probs , numInfo                           *
 * Function : entry�� key�� �Ͽ� ���� ����(dictionary)����     *
 *            ã�ƺ���.                                        *
 *            (tag,irr-information) --> info-array             *
 *            ambiguity             --> numInfo                *
 *            Prob(word|tag)        --> probs-array            *
 *            Prob(tag|word)        --> probs-array            *
 *                                                             *
 *-------------------------------------------------------------*/

PRIVATE void RealLookupDict(entry,info,probs,numInfo)
UCHAR entry[]  ;                          /* �������� ã�ƾ� �� entry */  
UCHAR info[][__ATTRSIZE__] ;              /* ������ Lookup������ info */
double probs[] ;                          /* P(word|tag)              */ 
int *numInfo ;                            /* ambiguity ����           */ 
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
    *numInfo = SplitStr(info,content.dptr)   ;       /* ������ Decode  */

    for(idx = 0 ; idx < *numInfo ; ++idx) {
      switch(info[idx][0]) {
       case F_T : 
		  freq = atoi(&info[idx][3]) ;       /* Freq(tag,word) */
                  probs[idx] =
       		(double) freq / (double) tagfreq[info[idx][1]-'0'] ; 
                  break ; 
       case F_P : break ;    /* ���¼� ��м��� PutEntry���� �ذ��Ѵ�. */
       case F_F : break ;    /* ���¼� �м� PreFerence�� PutEntry���� �ذ� */
       case F_L : break ;    /* F_L : Lookahead Characters like Trie   */ 
      }
    }

  }


}/*---End of RealLookupDict---*/

/*------------------------------------------------------------------*
 *                                                                  *
 *   PutEntry : ���¼ҿ� �׿� ���� �������� morphP[]�� �����Ѵ�.    *
 *              ������ �����ϴ� ���¼ҵ�� ���� �����Ѱ� ������ ��  *
 *              ������ �� �� ���¼Ҹ� ����Ű���� index�� �����Ѵ�.  *
 *                                                                  *
 *   Input    : entry , info , probs , numInfo                      *
 *   Output   : NULL                                                *
 *   Function : ���� numInfo == 0 �̸� return                       *
 *              �Է� ���¼��� ������ node ��ȣ�� ������ ���¼ҵ���  *
 *              ���� node ��ȣ�� ��ġ�ϰ� ���Ӱ����ϸ�              *
 *              �� ���¼Ұ� �ִ� ���� ����Ų��.                     *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC void PutEntry(entry,info,probs,numInfo)
UCHAR entry[]  ;                          /* �������� ã�ƾ� �� entry */  
UCHAR info[][__ATTRSIZE__] ;              /* ������ Lookup������ info */
double probs[] ;                          /* P(word|tag)              */ 
int  numInfo ;                            /* ambiguity ����           */ 
{
  JoinMark joinmark  ;  /* Pre-Analyzed�� �¿� context   : +: ���� �� */
  JoinMark tmpmark   ;  /* ���� ã�� �ܾ��� �¿� context : $: ���� �� */ 
  SINT morBegin      ; 
  SINT morEnd        ;
  int    idx , idx2  ;                  /* information Index         */ 
  Morph oneMor       ; 
  double preprobs[__NUMPREMORANAL__]  ; /* ���¼� ��м��� ����      */
  SINT connect[__NUMCONNECTION__]     ; /* connect �Ǵ� idx          */
  PreMor premor[__NUMPREMORANAL__]    ; /* ���¼� ��м��� ���� ���� */
  int numpremor      ; /* ��м��� ������ ���¼� ����                */
  SINT maxwright ; 
  int mtPtr = morPtr ;                  /* ���� morPtr�� �����Ѵ�    */

  morBegin = wordSM[smPtr].begin ;      /* ���¼� ���� node          */
  morEnd = wordSM[smPtr].end ;          /* ���¼� �� node            */

  if (!numInfo) return ;         /* numInfo�� 0�̸� ������ return    */

  for(idx = 0 ; idx < numInfo ; ++idx) {
    switch(info[idx][0]) {

     case F_T : /* Format : Tag */
                ConTest(mtPtr,&maxwright,connect,morEnd,info[idx][1]) ;  
                oneMor = MakeMor(
                  smPtr        , /* ȯ��                                     */
                  'x'          , /* (�ظ�-����)���� ���Ⱑ �ʿ�ġ �ʴ� */
                  morBegin     , /* ���¼� ���� node                         */
                  morEnd       , /* ���¼� �ϳ��� ������ node                */
                  maxwright    , /* ���� ���¼� �ؼ� ����� ������ node MAX  */
                  connect      , /* ���� ���¼Ұ� �ִ°� ����Ű��index:-1:�� */
                  entry        , /* �ϳ��� ���¼�                            */
                  info[idx][1] , /* ���¼� ǰ�� index                        */
                  info[idx][2] , /* ���¼� �ұ�Ģ code                       */
                  probs[idx]     /* Ȯ�� : P(word|tag)                       */
                ) ; 
                MorPush(oneMor) ;    /* ������ ���¼Ҵ� �ִ´�  */
                break ; 

     case F_P : /* Format  : Pre ���¼� ��м�              */
		/* example : �̷��� : �̷���/a*��/pv+��/ecx 
                 *           ��/ecx�� ���ؼ� ���� �Ͱ� �����Ѵ�.
	         *           �ڿ��� ������ MorPush() �Ѵ�.
                 *    ���� : ���¼� ��м��� ���� ���� ���¼��� ���� ��ȣ�� 
		 *	     ���� ������ ���¼��� ������ ��ȣ�� 
		 *	     �̹� �м��� �ٸ� ���¼ҵ�� Sync�� ���߰� 
		 *	     ��м� ���¼��� ��ȣ���� -1(Don't Care)�� �ִ´�
		 *
		 * morBegin�� morEnd�� ���� lookup�� �ܾ��� �¿� �� ��ȣ��.
		 * ������ �¿� �� ��ȣ�� 0�� endNodeValue�̴�.
		 *
                 */ 

                numpremor = DecodePreMor(premor, &info[idx][1], &joinmark) ;  

                /*
		 * joinmark.lmark�� joinmark.rmark�� ���Ƽ� 
		 * ���� ���� +, ���� �¿� ���� $ǥ���̹Ƿ�
		 * ���� ã�� �ܾ��� �¿� context�� �´��� ���Ѵ�.
		 */
                
		tmpmark.lmark = (morBegin == 0) ? '$' : '+' ; 
		tmpmark.rmark = (morEnd == endNodeValue) ? '$' : '+' ; 
                
                if (tmpmark.lmark != joinmark.lmark ||
		    tmpmark.rmark != joinmark.rmark) break ;
		                                 /* �¿� context�� ���� ������ */

                GetPreProbs(preprobs,premor,numpremor) ; /* P(word|tag)�� ���� */
		ConTest(mtPtr,&maxwright,connect,morEnd,
		        premor[numpremor-1].pos) ;   /* ��м��� ������ ���¼� */
                oneMor = MakeMor(
                  1          , /* ȯ�� = 1 : Original ������ �ƴ�          */
       	          'x'        , /* (�ظ�-����)���� ���Ⱑ �ʿ�ġ �ʴ� */
                  ((numpremor == 1) ? morBegin : -1) , 
			       /* ���¼� ����node : morBegin Ȥ�� Don'tCare*/
                  morEnd     , /* ���¼� �ϳ��� ������ node                */
                  maxwright  , /* ���� ���¼� �ؼ� ����� ������ node MAX  */
                  connect    , /* ���� ���¼Ұ� �ִ°� ����Ű�� index:-1:��*/
		  premor[numpremor-1].word , /* ������ ���¼�              */
		  premor[numpremor-1].pos  , /* ���¼� ǰ�� index          */
                  '0'        , /* ���¼� �ұ�Ģ code                       */
                  preprobs[numpremor-1] /* Ȯ�� : P(word|tag)              */
                ) ; 
                MorPush(oneMor) ;  

                for (idx2 = numpremor - 2 ; idx2 >= 0 ; --idx2) {
                  connect[0] = morPtr ; /* ������� Push�� ���� ����Ų�� */
                  connect[1] = -1     ; 
                  oneMor = MakeMor(1,((premor[idx2].mark == '*') ? 'o' : 'x'), 
			((idx2 == 0) ? morBegin : -1) , -1 , maxwright , connect ,
			premor[idx2].word,premor[idx2].pos,'0',preprobs[idx2]) ; 
                  MorPush(oneMor) ; 
                }
                break ; 

     case F_F : /* Format  : Pre ���¼� ��м�              */
                /* example : �̷��� : �̷���/a*��/pv+��/ecx
                 *           ��/ecx�� ���ؼ� ���� �Ͱ� �����Ѵ�.
                 *           �ڿ��� ������ MorPush() �Ѵ�.
                 *    ���� : ���¼� ��м��� ���� ���� ���¼��� ���� ��ȣ��
                 *           ���� ������ ���¼��� ������ ��ȣ��
                 *           �̹� �м��� �ٸ� ���¼ҵ�� Sync�� ���߰�
                 *           ��м� ���¼��� ��ȣ���� -1(Don't Care)�� �ִ´�
                 */

                numpremor = DecodePreMor(premor, &info[idx][1],&joinmark) ;

                /*
                 * joinmark.lmark�� joinmark.rmark�� ���Ƽ�
                 * ���� ���� +, ���� �¿� ���� $ǥ���̹Ƿ�
                 * ���� ã�� �ܾ��� �¿� context�� �´��� ���Ѵ�.
                 */

                tmpmark.lmark = (morBegin == 0) ? '$' : '+' ;
                tmpmark.rmark = (morEnd == endNodeValue) ? '$' : '+' ;

                if (tmpmark.lmark != joinmark.lmark ||
		    tmpmark.rmark != joinmark.rmark) break ;
		                                 /* �¿� context�� ���� ������ */

                GetPreProbs(preprobs,premor,numpremor) ; /* P(word|tag)�� ���� */

                /*
                 * Pop !! until morEnd is found
                 * It means deleting all other interpretation
                 *      ---> select this only
                 */

                while (morphP[morPtr].left != morEnd ||
                       morphP[morPtr].wright == -1) --morPtr ;
                mtPtr = morPtr ;

                ConTest(mtPtr,&maxwright,connect,morEnd,
                      premor[numpremor-1].pos) ; /* ��м��� ������ ���¼� */
                oneMor = MakeMor(
                  1          , /* ȯ�� = 1 : Original ������ �ƴ�          */
                  'x'        , /* (�ظ�-����)���� ���Ⱑ �ʿ�ġ �ʴ� */
                  ((numpremor == 1) ? morBegin : -1) ,
                               /* ���¼� ����node : morBegin Ȥ�� Don'tCare*/
                  morEnd     , /* ���¼� �ϳ��� ������ node                */
                  maxwright  , /* ���� ���¼� �ؼ� ����� ������ node MAX  */
                  connect    , /* ���� ���¼Ұ� �ִ°� ����Ű�� index:-1:��*/
                  premor[numpremor-1].word , /* ������ ���¼�              */
                  premor[numpremor-1].pos  , /* ���¼� ǰ�� index          */
                  '0'        , /* ���¼� �ұ�Ģ code                       */
                  preprobs[numpremor-1] /* Ȯ�� : P(word|tag)              */
                ) ;
                MorPush(oneMor) ;

                for (idx2 = numpremor - 2 ; idx2 >= 0 ; --idx2) {
                  connect[0] = morPtr ; /* ������� Push�� ���� ����Ų�� */
                  connect[1] = -1     ;
                  oneMor = MakeMor(1,((premor[idx2].mark == '*') ? 'o' : 'x'),
                        ((idx2 == 0) ? morBegin : -1) , -1 , maxwright , connect ,
                        premor[idx2].word,premor[idx2].pos,'0',preprobs[idx2]) ;
                  MorPush(oneMor) ;
                }
                return ; /* �� �̻� �ٸ� ������ �� �ʿ䰡 ����. */
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
 * Function : ��м� ���¼ҵ��� �Է����� �޾� preprobs[]�� ���� *
 *                                                              *
 *--------------------------------------------------------------*/

PRIVATE void GetPreProbs(preprobs,premor,numpremor)
double preprobs[]  ;                       /* ���¼� ��м��� P(word|tag) */
PreMor premor[]    ;                       /* ���¼� ��м��� ���� ����   */
int numpremor      ;                       /* ��м��� ������ ���¼� ���� */
{
  int idx , idx2 ; 
  int freq     ; 
  int bool     ;  /* ǥ�� ������ �� ǰ�簡 �ִ°� ? */
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
    bool = FALSE ; /* �� ǰ��� ������ ���� �ʴ� */

    if (content.dptr != NULL) {  /* ��м� ������ ������ ǥ�� ������ ���� �� */
                                 /* ǥ�� ������ Freq(Word,Tag) = 1 �� ����   */
      numInfo = SplitStr(info,content.dptr) ;  
      for(idx2 = 0 ; idx2 < numInfo ; ++idx2)
	if(info[idx2][0] == F_T && info[idx2][1] == premor[idx].pos) {
	   freq = atoi(&info[idx2][3]) ;  /* Freq(word,tag)               */
	   bool = TRUE ;                  /* ������ �� ǰ��� word�� �ִ� */
	   break ;
        }
     }

     preprobs[idx] = (bool == TRUE) ? (double) freq / posPr :
                                      (double) 1.0 ; 
  }

}/*---End of GetPreProbs---*/

/*--------------------------------------------------------------------*
 *                                                                    *
 * GetNextWordPos : ���¼Ҹ� ����� ����� ����                       *
 *                  ���� ���¼Ҹ� ����.                               *
 *                                                                    *
 * Input    : control                                                 *
 * Output   : 0 : ���̻� ���¼Ұ� ����.                               *
 *   	      1 : ���� ���¼Ҹ� ����� ������.                        *
 * Function : 'L' : �������θ� pointer�� �Ű� ���¼Ҹ� �����         *
 *            'R' : ���������θ� pointer�� �Ű� ���¼Ҹ� �����       *
 *            'G' : �Ϲ����� ��쿡 �ش��Ѵ�                          *
 *            �� ���� ������ begin..end�� morphP , wordSM , inputWP�� *
 *            �̿��Ͽ� ���Ѵ��� wordSM�� top�� ���´�.                *
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
  int findNext = 0 ;                            /* ���� ���� ã�ҳ� */

  if (nextBegin == mini && nextEnd == mfin) return 0 ;        /* �� */ 

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
                tempBegin = GetNBegin(mTop,nextBegin,mini) ; /* ���� begin */
	            tempEnd = GetNEnd(mTop,tempBegin,mfin) ;     /* ���� end   */
               } else {
                tempBegin = nextBegin ; 
	            tempEnd = GetNEnd(mTop,nextEnd,mfin) ;       /* ���� end   */
               }
			   break ; 
  }

  while (!findNext && tempEnd != mfin) { 
    for(morIdx = morPtr ; morIdx >= 0 ; --morIdx)
      if(morphP[morIdx].left == tempEnd &&
       morphP[morIdx].wright == endNodeValue) {
	   findNext = 1 ; /* ���������� fin�� �� �� �ִ� �� */ 
	   break ; 
     }
    if (!findNext) tempEnd = GetNEnd(mTop,tempEnd,mfin) ; 
  } ;
      /*  �־��� ��� : morphP[].left = wordSM[].fin�� �ȴ�        */
      /* ���� begin�� �ٲ� ���� ���� : �־����� end�� ���� ���� �� */
 
  wordSM[smPtr].begin = tempBegin ; 
  wordSM[smPtr].end = tempEnd ; 
  return 1 ;                       /* OK : ���� begin..end�� ã�Ҵ� */
}/*----------End of GetNextWordPos----------*/


/*--------------------------------------------------------------------*
 *                                                                    *
 * GetNBegin : Get Next Begin Position                                *
 *                                                                    *
 * Input    : mTop,preBegin,mini                                      *
 * Output   : Next Begin Position                                     *
 * Function : ���� ���¼�-���� ������ position(node-number)�� ã�´�. *
 *                                                                    *
 *--------------------------------------------------------------------*/

PRIVATE SINT GetNBegin(mTop,preBegin,mini) 
SINT mTop     ; 
SINT preBegin ; 
SINT mini     ;
{
  SINT wIdx  ;                                 /* wordP Index */
  SINT inter = preBegin ;                      /* �߰� node   */

  do {
   for (wIdx = mTop ; wIdx >= 0 ; --wIdx)
     if (inputWP[wIdx].right == inter) {
	   inter = inputWP[wIdx].left ; 
	   break ; 
     }
  } while (inputWP[wIdx].start != 'o' && inter != mini) ; 

  return inter ;                 /* �־��� ��� inter == mini */
}/*---------End of int GetNBegin----------*/


/*------------------------------------------------------------------*
 *                                                                  *
 * GetNEnd  : Get Next End Position                                 *
 *                                                                  *
 * Input    : mTop,preEnd,mfin                                      *
 * Output   : Next End Position                                     *
 * Function : ���� ���¼�-�� ������ position(node-number)�� ã�´�. *
 *                                                                  *
 *------------------------------------------------------------------*/

PRIVATE SINT GetNEnd(mTop,preEnd,mfin)
SINT mTop   ; 
SINT preEnd ; 
SINT mfin   ;
{
  SINT wIdx ;                                   /* wordP Index */
  SINT inter = preEnd ;                         /* �߰� node   */

  do {
   for (wIdx = mTop ; wIdx >= 0 ; --wIdx)
     if (inputWP[wIdx].left == inter) {
	   inter = inputWP[wIdx].right ; 
	   break ; 
     }
  } while (inputWP[wIdx].end != 'o' && inter != mfin) ; 

  return inter ;                /* �־��� ��� : inter == mfin */
}/*---------End of int GetNEnd----------*/


/*-----------------------------------------------------------------*
 *                                                                 *
 * ConTest  : Connectability Test :                                *
 *            Corpus���� ���� ���� ���� Matrix�� �����Ͽ�          *
 *            �� ���¼Ұ��� ���Ӱ��ɼ��� Test                      *
 *                                                                 *
 * Input    : mtPtr,maxwright,connect,morEnd,tag                   *
 * Output   : connect                                              *
 * Function : ���� ������ ���¼��� index�� connect array�� �ִ´�. *
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
      if (morphP[morIdx].pos == _INT_) {  /* �Ű� ���� ó�� `��' */
        if (strchr(Is_jong,GetLeeOnRIdx(morEnd))) { /* `��'�տ��� ������ ü�� */
          for (tempIdx = 0 ; morphP[morIdx].nidx[tempIdx] != -1 ; 
                                   ++tempIdx )
            connect[numofconnect++] = morphP[morIdx].nidx[tempIdx] ; 
          if (max < morphP[morIdx].wright) max = morphP[morIdx].wright ; 
        } /* ������ �ƴϸ� �ƹ��͵� ���� �ʴ´� */
      } else {
        connect[numofconnect++] = (SINT) morIdx ; 
        if (max < morphP[morIdx].wright) max = morphP[morIdx].wright ; 
      }
   }

  connect[numofconnect] = (SINT) -1 ; /* end of sequence */
  *maxwright = max ; 

    /*
     *  ���� ���� ���¼ҿ� �´� ���� �ϳ��� ���ٸ�
     *  connect[0] = -1 , *maxwright = -1 �̴� 
     */

}/*----------End of ConTest----------*/

/*-----------------------------------------------------------*
 *                                                           *
 * GetFirstBegin : Get First Begin Position                  *
 *                 ù��° ���� ��ġ�� ã�´�.                *
 *                                                           *
 * Input    : strl                                           *
 * Output   : begin position                                 *
 * Function : inputWP�� starter���� ���� �� �κп� �ִ� ���� *
 *            return                                         *
 *                                                           *
 *-----------------------------------------------------------*/

PRIVATE SINT GetFirstBegin(strl)
int strl ; 
{
  SINT i ; 
  for(i = strl - 1 ; i >= 0 ; --i)
    if (inputWP[i].start == 'o') 
	return inputWP[i].left ;      /* ������ �� �ִ� code�� ���� node */  
  
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
 * Function    :  ���� node ���� �޾� �װ��� index�� return *
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
 * Function    :  ������ node ���� �޾� �װ��� �̼��� �ڵ带 return *
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
 * Function : ���� ���Ǵ� tag���� index�� �̸� �����ϰ� *
 *            `������ ����'�� �󵵵� �����Ѵ�.            *
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
 * Fuction : Morph Pool�� �״�� ȭ�鿡 Display *
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
 * OutputMorph : morphP[index]������ morph�� ȭ�鿡       *
 *				 Display                                  *
 *                                                        *
 * Input   : index : ���� morphP[index]                   *
 * Output  : NULL                                         *
 * Fuction : Lattice ������ Morph Pool��                  *
 *           Linear ������ �ٲٰ� KS5601�� ȭ�鿡 Display *
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
 * Out2Han : �̼��� code�� �Ǿ��ִ� morphP[displayP[i]].word��       *
 *           KS5601�� ��ȯ�Ͽ� ȭ�鿡 Display                        *
 * Input   : NULL                                                    *
 * Output  : NULL                                                    *
 * Fuction : morphP[].word�� �ѱ� string���� ��ȯ�Ͽ� ȭ�鿡 Display *
 *                                                                   *
 *-------------------------------------------------------------------*/

PUBLIC void Out2Han()
{
  int i ; 
  UCHAR hanstr[__MAXMORPH__ * 2 ] ; /* ���¼� ���� */

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
 * Function         : inputWP[]�� �״�� ȭ�鿡 ��� *
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


