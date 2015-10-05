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
#define CHEEONENDIDX             4 /* openClass[]���� ü���� ������ Index */ 
#define YONGEONENDIDX            6 /* openClass[]���� ����� ������ Index */ 
#define BUSAENDIDX               9 /* openClass[]���� �λ��� ������ Index */
#define EXCLENDIDX              11 /* openClass[]���� ��ź��,������ Index */

#ifdef UNKNOWNMODEL3
PUBLIC double unknownProb[__NUMOFOPENCLASS__][3] ; /* Known-Word , Unknown-Word */
PUBLIC UnkPttrn unknownLexm[__NUMOFOPENCLASS__]  ; /* Prob(Ending_i|Tag)�� ���� */
PUBLIC PttrnTot patternTotal[__PTTRNTOT__]       ; /* pattern�� ���� ū ����    */ 
PUBLIC void LoadUnknownFreq() ; /* Prob(Unkn|Tag) , Prob(Known|Tag) */ 
PUBLIC void LoadUnknownLexm() ; /* Prob(Tag_Ending_i|Tag)           */ 
PRIVATE UCHAR FirstTag()      ; /* Tag string���� ù��° Tag        */
PUBLIC Pttrn Str2Lexm()       ; /* Str to Pattern                   */
PUBLIC int IsInOpenTag()      ; /* Is Str In Open-Tag ?             */
PUBLIC double ProbLexFtr()    ; /* Prob(Feature=Pattern|Tag)        */
int    numOfPtrns = 0         ; 
double numtotalptrn = 0.0     ; 
#endif

EXTERN InputW inputWP[__EOJEOLLENGTH__]            ; /* Input Word Pool        */
EXTERN UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /* ���� tbl               */
EXTERN Morph  morphP[__NUMMORPH__]                 ; /* Morph Pool             */
EXTERN WSM    wordSM[__NUMWSM__]                   ; /* ���ο� ȯ�� ManagePool */ 
EXTERN int tagfreq[__NUMOFTAG__]                   ; /* ���¼� �߻� Frequency  */
EXTERN int morPtr ;                     /*   Morph Pool Top Pointer of morphP  */
EXTERN int smPtr  ;                     /* Stack Manager Top Pointer of wordSM */
EXTERN SINT endNodeValue  ; /* kimmolen * __SKIPVALUE__ defined in ktsmoran.c  */
EXTERN UCHAR _FINAL_          ; /* ���� , ���� ������              */
EXTERN UCHAR _INITI_          ; /* ���� , ���� ó��                */
EXTERN UCHAR _INT_            ; /* �Ű� ���� `��'                  */
EXTERN UCHAR _EFP_            ; /* ��� ���                     */
EXTERN UCHAR _JC_             ; /* ������                          */
EXTERN UCHAR _JCM_            ; 
EXTERN UCHAR _JCA_            ; 
EXTERN UCHAR _JJ_             ; 
EXTERN UCHAR _XN_             ; /* ����                            */
EXTERN UCHAR _EXM_            ; /* ������ �������                 */
EXTERN UCHAR _EXN_            ; /* ����� �������                 */
EXTERN UCHAR _ECQ_            ;
EXTERN UCHAR _ECX_            ; 

EXTERN UCHAR TagIdx()         ;
EXTERN void MorPush()         ; /* Morph Push                      */ 
EXTERN void ConTest()         ; /* Connectibility Test             */
EXTERN Morph MakeMor()        ; /* �ϳ��� ���¼� record �����     */ 
EXTERN int kimmo2ks()         ; /* �̼��� �ڵ带 ks�ϼ������� �ٲ� */ 

PUBLIC void PredictUnknown()      ; /* Unknown Word Prediction     */
PRIVATE Morph_Part GetPartMorph() ; /* Get Part of Morph Structure */
PRIVATE int GetUnknownWord()      ; /* Get Unknown Word            */
PRIVATE int CheckEndings()        ; /* X is in {����,���,����} ?  */
PRIVATE int CheckSuffix()         ; /* ���簡 �ִ°� ����          */
PRIVATE double UnknownProb()      ; /* Unknown Word�� Lexical Prob */
PRIVATE int LingFltr()            ; /* ������ ������ �����ϴ� ��� */
PRIVATE int IsInTbl()             ;

/*
 * Open Class Word : nct : �ð��� ���� ���
 *                   nca : ���ۼ� ���� ���
 *                   ncs : ���¼� ���� ���
 *                   nc  : ���� ���
 *                   nq  : ���� ���
 *                   pv  : ����
 *                   pa  : �����
 *                   m   : ������ 
 *                   at  : �ð� �λ�
 *                   ajs : ���� ���� �λ�
 *                   a   : �λ�
 *                   i   : ��ź��
 *
 * Open Class Word�� INI ������ ������ �� �� �ִ� 
 *
 * Heuristics : {�� , �� , �� , �� , �� , �� , ��� , ����} 
 *
 *              ���� ���¼Ұ� �ؿ��� ���� Searching�Ҷ�  
 *              ������ ������ ������ ���¼Ҹ� �����Ҷ�
 *              �� �������� �����Ѵ�.
 *              �� ��ü ������ �ϳ��� ���¼ҷ� �������� �ʴ´�
 * 
 *  �켱���� :  ���� : xpv , xpa , xn  , xa 
 *              ���� : jc  , jcm , jcv , jca , jcp , jx  , jj
 *              ��� : efp , ecq , ecs , ecx , exm , exn , exa , ef
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
 * PredictUnknown : Unknown Word�� ó���Ѵ�.                    * 
 *                  ���¼� �м��� �κ������� �� �����          *
 *                  morP[]�� �ִµ� ������ ��� ���¼� ������   *
 *		    �Ѵ�                                        *
 *                                                              *
 * ���� ���� : ������ ���¼��� ������ �̼��� �ڵ尡             *
 *             �ʼ� Ȥ�� �ݸ����̶��                           *
 *             �� �κ� ���¼ҿ� ���� ������ �����ʴ´�.         *
 *                                                              *
 * ���¼� ������ ������                                         *
 *                                                              *
 * 1. ����� ��������ε� �װ��� �ٸ� ���� ���¼� �м��Ǵ°���  *
 *    ��� �ؾ� �ϴ°� (ö������ : ö��/nc+��/xn+��/jx)       *
 *                                                              *
 * 2. ����ε� �ٸ� ���� ���¼� �м��� �Ǵ� ���                *
 *    (���� : ��/nbu+��/pv+��/exm)                          *
 *                                                              *
 * 3. ���¼� ������ ��ü�� ���� ����                          *
 *    (�� FIN�տ� �� �� �ִ� �͵� �����Ͽ��� �Ѵ�)              *
 *    (���� : ���� + ��/nc , ���� + ��/exm )                    *
 *            Fail           Success                            *
 *     ==> ����/nc                                              *
 *                                                              *
 * 4. ü���� 2������ ���� ���� , ����� 3������ ���� ����       *
 *    (Feature�� ����� �� �ִ� ����̴�)                       *
 *                                                              *
 * 5. �����ϴ� ���� Open Class�� �� ��쿡�� �Ѵ�.          *
 *    Open Class : ü�� , ���                                  * 
 *                                                              *
 * ��� 1. : Simplest Model                                     *
 *                                                              *
 *   1. (INI , ���� ���¼� , �κ� �м��� ���¼ҵ��� ���� ����)  *
 *      ConTest(INI,���� ���¼�) && ConTest(�������¼�,�κ�)    * 
 *      ���� ������ �����ϴ� Tag�� �����Ͽ� morP[]�� ����       *
 *                                                              *
 *   2. 'INI'�� morP[]�� ����                                   *
 *                                                              *
 *   3. Prob(Word|Tag) = ( Freq(Word,Tag) = 1 / Freq(Tag) + 1 ) *
 *                                                              *
 *   4. Prob(Word|Tag) = 1.0���� ���� : �� , P(Tag_i|Tag_i-1)�� *
 *                                      ����ϰڴٴ� ��         *
 *                                                              *
 * ��� 2. : Statistical Model                                  *
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
  int numNext                      ; /* ���� ���¼ҿ� ������ ������ ����   */
  int scanIdx                      ; /* �κ� ���¼��� Scanning Index       */
  int unknLen                      ; /* �̵�Ͼ��� ���� ����               */
  int tempIdx                      ; 
  int construct = 0                ;  
  Morph oneMor                     ; /* ���¼� �м��Ȱ� �ϳ�               */
  UCHAR unknWord[__MAXMORPH__]     ; 
  UCHAR unkn_hcode , unkn_lcode    ; /* �̵�Ͼ��� ������ ����             */
  SINT nextIdx[__NUMCONNECTION__]  ; 
  UCHAR endings[250]               ; /* ���¼Ҹ� ���� �� �پ��� ���¼� tag */
  int numEndings = 0               ; /* endings[0 .. numEndings - 1]       */
  Morph_Part start_part , end_part ; 

 /* Stack Pop until morphP[morPtr].wright == -1
  * morphP[morPtr].left�� 0���� ������ �ظ� �������� ������ ��
  */ 
 /* Case 1 : Pop until if morphP[morPtr].env != 1 */
 /* Case 2 : Pop until just if morphP[morPtr].wright != -1 */

  while(morphP[morPtr].wright == -1 || morphP[morPtr].left <= 0 ) --morPtr ;

  start_idx = end_idx = morPtr ; /* morphP[]�� ������ �Ʒ��� scanning�Ѵ� */

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
     * [start_part .. end_idx+1]�� �ִ� ���¼ҿ� ���� ������ ����
     * Unknown Word�� �������� �װ��� ������ �̼��� �ڵ尡 
     * �ʼ��̳� Ȥ�� �� �����̸� ������ �ʴ´�. 
     */

    if ((unknLen = GetUnknownWord(&unkn_hcode,&unkn_lcode,unknWord,start_part.left))) {
      for(unIdx = 0 ; unIdx < __NUMOFOPENCLASS__ ; ++unIdx) {
        numNext = 0 ; 
	currTag = openClass[unIdx] ; 
        for(scanIdx = start_idx ; scanIdx > end_idx ; --scanIdx)
          if (tranTable[currTag-'0'][morphP[scanIdx].pos-'0'] == '1' &&
               LingFltr(unknLen,unkn_hcode,unkn_lcode,unIdx)) { /* connectable */

            if (morphP[scanIdx].pos == _INT_) { /* `��' ó�� */
              if (strchr(Is_jong,inputWP[start_part.left/__SKIPVALUE__ -1].lee))
                                                                 /* ������ ü�� */ 
                for (tempIdx = 0 ; morphP[scanIdx].nidx[tempIdx] != -1 ; ++tempIdx) {
                  nextIdx[numNext] = (SINT) morphP[scanIdx].nidx[tempIdx] ; 
                  endings[numEndings++] = morphP[nextIdx[numNext]].pos ; 
                  ++numNext ; 
                }
              /* ������ ü���̸� �ƹ��͵� ���� �ʴ´� */
            } else {
               nextIdx[numNext++] = (short int) scanIdx ; 
               endings[numEndings++] = morphP[scanIdx].pos ; 
            }

          }

        nextIdx[numNext] = (short int) -1 ; 
	if (numNext != 0) { /* �κ� ���¼ҿ� �ٴ� ���� ���� ��� */

         oneMor = MakeMor( (short int) 0 , 'x' , (short int) 0 , 
                     (short int) start_part.left ,
		     (short int) start_part.wright , nextIdx , unknWord ,
		     currTag , '0' , UnknownProb(currTag)) ; 

          MorPush(oneMor) ; 
	  construct = 1   ; /* ���ڸ� ������ ���� �ִ� */

#ifdef DEBUG
	  printf("morPtr %d , start_idx %d , end_idx %d , nextIdx[0] = %d\n",
		  morPtr,start_idx,end_idx,nextIdx[0]) ; 
#endif

        }/*-End of If-*/
      }/*-End of For-*/
     
#ifdef BASELING1
       if (CheckSuffix(endings,numEndings)) end_idx = 0 ;  
#endif

    /*  ���簡 ������ �� �̻� morphP[]�� ã�� �ʴ´�       */ 

   }/*-End Of If-*/

/* Case 2 */

   while(morphP[end_idx].wright == -1 || morphP[end_idx].left <= 0) --end_idx ; 
   start_idx = end_idx ;  

#ifdef DEBUG2
    printf("PredictUnknown Module : start_idx = end_idx : %d\n", start_idx) ; 
#endif

  }/*-End of While-*/

  /* 
   * ���� If���� �ϳ��� ���¼Ұ� ���������� �ϳ��� ������ �� �� �ִ°� ����  
   * Connetability(���� ���¼� , FIN) ����
   *
   * endings[0 .. numEndings - 1]�� �ִ� ������ { ���� , ��� , ���� } ��
   * �ϳ��� ������ ��ü ���¼Ұ� ���������� �ϳ��� ������ �ȴٰ� ��������
   * �ʴ´� : Heuristics
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

  /* ���⼭ INI �� �ִ´� */
  /* ��ü�� ���� �ϳ��� ������ ���� ���� ���� ���� �����Ѵ�. */

  for(idx = 0 ; idx < endNodeValue/__SKIPVALUE__ ; ++idx) 
     unknWord[idx] = inputWP[idx].lee ; 
  unknWord[idx] = '\0' ;  
  currTag = openClass[3] ; /* ������� ���� */

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
   0         , /* ȯ��                                         */
   'x'       , /* ���¼ҿ� ���¼��� ���� :
                  �� : �̷��� : �̷��� �ϴ� (���Ⱑ �ʿ�)  */
   (SINT) 0  , /* INI �� ����   : 0                            */
   (SINT) 0  , /* INI �� ������ : 0                            */
   maxwright , /* ���� ���¼� �ؼ� ����� ������ node MAX      */
   nextIdx   , /* ���� ���¼Ұ� �ִ� ���� ����Ű�� index:-1:�� */
   &tempCh   , /* NULL                                         */
   _INITI_   , /* INI index                                    */
   '0'       , /* INI �ұ�Ģ code                              */
   (double) 1.0/* Ȯ�� : P(tag|word)                           */
  ) ;
  MorPush(oneMor) ;               /* INI�� �ִ´� */

}/*-------End of PredictUnknown-------*/ 


/*-------------------------------------------------*
 *                                                 *
 * CheckEndings : endings[]���� ��ҵ� �� �ϳ��� *
 *                _EFP_ �̸�                       *
 *                �ȴٸ� return 1                  *
 *		  else   return 0                  *
 *                                                 *
 *-------------------------------------------------*/

PRIVATE int CheckEndings(endings,num)
UCHAR endings[] ;                    /* Unknown Word ���� ���¼� Tags */
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
 * CheckSuffix : endings[]���� ��ҵ� �� �ϳ��� *
 *                { ���� }�� ���Ұ�               *
 *                �ȴٸ� return 1                 *
 *		  else   return 0                 *
 *               (�� ��� ���̻� : �� , �� ����)  *
 *------------------------------------------------*/

PRIVATE int CheckSuffix(endings,num)
UCHAR endings[] ;                    /* Unknown Word ���� ���¼� Tags */
int num         ;
{
  int i ;

  for(i = 0 ; i < num ; ++i)
    if(endings[i] >= _XN_) return 1 ; 

  return 0 ; 

}/*---------End of CheckSuffix---------*/


/*-----------------------------------------------------*
 *                                                     *
 * GetUnknownWord : Unknown Word�� inpuWP[]���� ���   * 
 *                 ���� ������ �̼��� �ڵ尡 �ʼ� Ȥ�� *
 *		   �ݸ����̶�� Unknown Word�� �ƴϴ�  *
 *                                                     *
 * �ϴ�, ``��(�� ��Ģ),��''�� ���� �߻��ϹǷ�          *
 * �׷� ���� ����� ���ɼ��� ������ �־�� �Ѵ�.     * 
 *                                                     *
 * ``��'',``��''�� �и��� `��+��',`��+��'�̴ϱ�        *
 * �ұ�Ģ ������ �߻��Ǿ��ٰ� �ص� ���ɼ� ���� �ʿ�    *
 *                                                     *
 *-----------------------------------------------------*/

PRIVATE int GetUnknownWord(high_code,low_code,unknWord,leftNode)
UCHAR *high_code ; 
UCHAR *low_code  ; 
UCHAR unknWord[] ; 
SINT  leftNode   ; 
{
  int idx        ; 
  int hanlen     ; /* ks code�� ��ȯ �� ���� ���� */
  UCHAR tmph[50] ; 

  for(idx = 0 ; idx < leftNode/__SKIPVALUE__ ; ++idx)
    unknWord[idx] = inputWP[idx].lee ; 
  unknWord[idx] = '\0' ;  

  if (strchr("ghqndlmbrsvfjzcktp",unknWord[idx-1])) return 0 ; 
  if (unknWord[idx-1] == 'w') {   /* `��,��,��'�� ��츸 ��� */  
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
  * unknWord�� ���� ���̸� return�Ѵ�.
  */ 

  if(!kimmo2ks(unknWord,tmph)) {

 /*
  *   fprintf(stderr,"Error in ktsunknown.c::GetUnknownWord()\n") ; 
  *   exit(1) ; 
  */

    return 0 ; 
  }

  hanlen = strlen(tmph) ; /* �̵�Ͼ��� ���� ���� * 2 */

  *high_code = tmph[hanlen-2] ; /* ������ ������ high_code */
  *low_code  = tmph[hanlen-1] ; /* ������ ������ low_code */

  hanlen /=2 ; 

#ifdef DEBUG3

  printf("%s\n",unknWord) ; 

#endif


  return hanlen ; 
}/*-----End of GetUnknownWord-----*/


/*----------------------------------------------*
 *                                              *
 * LingFltr : Lingustic Filter                  * 
 *            �̵�Ͼ��� ������ ������ �����Ͽ� *
 *            ������ ������ ��� 1              *
 *            else 0 �� return                  *
 *                                              *
 *  openIdx : openClass[]�� Index               *
 *                                              *
 *----------------------------------------------*/

PRIVATE int LingFltr(len,hcode,lcode,openIdx)
int len     ;           /* �̵�Ͼ��� ���� ���� */ 
UCHAR hcode ;           /* ������ high code     */
UCHAR lcode ;           /* ������ low code      */
int openIdx ;           /* openClass[]�� Index  */
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
   } else if (openIdx > BUSAENDIDX)    /* ��ź�� , ������ */
     return IsInTbl(hcode,lcode,
		exclTbl[0].eumjeolset,exclTbl[0].size) ; 

#endif

   return 1 ; 

}/*---------End of LingFltr---------*/


/*-----------------------------------------*
 *                                         *
 * IsInTbl  : Is In Table ?                * 
 *            Table���� �� ������ �ִ°� ? *
 *                                         *
 *            Linear Search                *
 *-----------------------------------------*/

PRIVATE int IsInTbl(hcode,lcode,set,size)
UCHAR hcode ;          /* ������ high code */
UCHAR lcode ;          /* ������ low code  */
UCHAR set[] ;          /* ������ ����      */
int    size ;          /* ������ ũ��      */ 
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
 * GetPartMorph : Morph Pool���� ���ϱ� ���� Data�� *
 *		 ���Ѵ�                               *
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
 * UnknownProb : Unknown-Word�� Prob�� ���Ѵ�.        *
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

  lex_prob = 1.0 ; /* ���⼭�� Ȯ���� 1.0�̶�� ���� 
		    * ktstagger.c::AddTrellis()���� �ذ��Ѵ�
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
 * LoadUnknownFreq : Open Class Tag�� ���Ͽ�          *
 *                   Freq(Known-Word,Tag) ,           *
 *                   Freq(Unknown-Word,Tag)�� ���Ѵ�. *
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
 * LoadUnknownLexm : Open Class Tag�� ���Ͽ�        *
 *                   Lexical Feature�� Loading�Ѵ�. * 
 *                                                  *
 *--------------------------------------------------*/

PUBLIC void LoadUnknownLexm(fname) 
char fname[] ;               /* fname == LEXFEATURE */
{
  FILE *f1 ; 
  UCHAR curr_tag               ; /* Current Tag                  */
  UCHAR cmp_tag                ; /* Comparison Tag               */
  char  inbuf[100]             ; /* pattrn & it's frequency      */
  Pttrn tmpPttrn[__MAXPTTRN__] ; /* Pttrn�� ������ �� malloc     */
  double pttrn_total           ; /* pttrn_total = Sum Pttrn Prob */
  int    openIdx               ; /* OpenTagIdx                   */
  int    idx , idx2            ; 
  int    numPttrn = 0          ; /* 0 .. numPttrn - 1            */ 
  int    open_class_flag = 0   ; /* if 1 : open class tag pttrn  */ 

  cmp_tag = _INITI_ ; /* pattern�� ���� ó���� INI�� ���� */ 

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

        openIdx = IsInOpenTag(cmp_tag) - 1 ; /* -1�� �Ǵ� */ 
	unknownLexm[openIdx].totalprob  = pttrn_total ; 
        unknownLexm[openIdx].numofpttrn = numPttrn ; 
        unknownLexm[openIdx].pttrnPtr   = (Pttrn *) malloc(sizeof(Pttrn)*numPttrn) ; 
        memcpy(unknownLexm[openIdx].pttrnPtr,tmpPttrn,
			       sizeof(Pttrn)*numPttrn) ; 
      }

      cmp_tag = curr_tag ; 
      numPttrn = 0 ; 

/*    
 * FEATUREFREQ�� sort by ascii �Ǿ��־ ���� line�� ���־� �ȴ�. 
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
 * FirstTag : Tag String���� ù��° Tag�� TagIdx�� return *
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
 * IsInOpenTag : Open Class Tag �ΰ� *
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
 * ProbLexFtr  : Open Class Tag�� Index�� lex feature�� *
 *               �޾� unknownPttrn[]���� lookup�Ͽ�     *
 *               P(lex-feature|tag)�� return�Ѵ�        * 
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
  * ���� ������ (w_i+1,t_i+1)�� Feature-���տ� ���� �ʴٸ�
  * FEATUREEPSILON�� return�Ѵ�.
  * 
  */

  return FEATUREEPSILON ; 

}/*------------End of ProbLexFtr------------*/


/*--------------------------------------------------------*
 *                                                        *
 * ProbInvLexFtr : Open Class Tag�� Index�� lex feature�� *
 *                 �޾� unknownPttrn[]���� lookup�Ͽ�     *
 *                 P(tag|lex-feature)�� return�Ѵ�        * 
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

  if (idx == numOfPtrns) /* ������ lex:pttrn�� ������ */ 
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
 * Str2Lexm : String�� Lexical Pttrn���� �ٲ� *
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


