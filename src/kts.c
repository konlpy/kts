/*---------------------------------------------------------------*
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9      *
 *                                                               *
 * SangHo Lee                                                    *
 *                                                               *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr             *
 *                                                               *
 * Computer Systems Lab.                                         *
 * Computer Science KAIST                                        *
 *                                                               *
 *                                                               *
 * kts.c (Korean POS Tagging System Main Body)                   *
 *                                                               *
 * KTS is composed of -- ktsmain.c    Main-Body                  *
 *                    -- ktsmoran.c   Morphological Analyzer     *
 *                    -- ktsirr.c     Handling Irregulars        *
 *		      -- ktstagger.c  POS Tagger                 *
 *                    -- ktsunknown.c Handling Unknown Word      *
 *                    -- ktsformat.c  Output Formatting          *
 *		      -- ktsutil.c    Utilities                  *
 *                                                               *
 *---------------------------------------------------------------*/

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
#include <math.h>

#include "ktsdefs.h"
#include "ktsds.h"
#include "ktstbls.h"
#include "kts.h"

#define __TRANTBLFILE__  "TRANTBL"  /* ǰ�� ���� table file */
#define __TRANFREQFILE__ "TRANFREQ" /* ǰ�� ���� Freq  file */
#define __DICTFILE__     "kts_dict" /* dbm ����             */

#define __UNKNOWNFREQ__  "UNKNOWNFREQ" /* Freq(Known-Word|Tag) 
					  Freq(Unknown-Word|Tag)  */

#define __LEXFEATURE__   "LEXFEATURE"  /* Lexical Feature  */

/* External Functions */

EXTERN int ks2kimmo()       ; /* �ѱ��� kimmo code�� �ٲ�         */
EXTERN char CharTag()       ; /* character Tag(category) return   */
EXTERN void PreMorphAn()    ; /* ���¼� �ؼ� Module               */
EXTERN void LoadTranTbl()   ; /* transition table�� Loading       */
EXTERN void LoadTranFreq()  ; /* transition Freq�� Loading        */
EXTERN void LoadTagFreq()   ; /* Tag Frequency�� Loading          */
EXTERN void DisplayTag()    ; /* ������ character-tag�� ���δ�    */
EXTERN void MakeTrellis()   ; /* ������ Trellis ������ �����     */
EXTERN void MakeSetEmpty()  ; /* Open-Node , N-Best Set�� empty�� */ 
EXTERN void ViterbiForwd()  ; /* Viterbi Algorithm Forward        */
EXTERN void ViterbiBckwd()  ; /* Viterbi Algorithm Backward       */
EXTERN void AlphaCmpttn()   ; /* HMM : Alpha Computation & P(O|L) */
EXTERN void BetaCmpttn()    ; /* HMM : Beta , Gamma Computation   */
EXTERN void DisplayTrellis(); /* Trellis�� ������ ȭ�鿡 ���δ�   */
EXTERN void InitTrellis()   ; /* Trellis�� �����                 */
EXTERN void DeleteTrellis() ; /* ���� ������ ���� Trellis�� ����  */ 
EXTERN void InitOpenClass() ; /* Open Class Word�� �����Ѵ�       */
EXTERN void InitTag()       ; /* ���� ���̴� Tag Index�� ����     */
EXTERN void DictClose()     ; /* ������ �ݴ´�                    */
EXTERN DBM *DictOpen()      ; /* ������ ����                      */

#ifdef PROLOG
EXTERN void Put2PrologP()   ; /* Path-Based Best Candidate Only   */
EXTERN void Put2PrologPM()  ; /* Path-Based Multiple Candidates   */
EXTERN void Put2PrologS()   ; /* State-Based Best & Multiple      */
#else
EXTERN void ChooseNBest()   ; /* State-Based : N-Best Cutting     */
EXTERN void ChooseMPath()   ; 
#endif

#ifdef UNKNOWNMODEL3
EXTERN void LoadUnknownFreq() ; /* Prob(Unkn|Tag) , Prob(Known|Tag) */ 
EXTERN void LoadUnknownLexm() ; /* Prob(Ending_i|Tag)               */
EXTERN UnkPttrn unknownLexm[__NUMOFOPENCLASS__] ; 
#endif
 
EXTERN DBM *dictPtr         ; /* ���� dbm pointer                 */
EXTERN UCHAR buffer[]       ; /* �Է� ���� Buffer                 */
EXTERN char buffer_tag[]    ; /* ������ Tag Buffer                */
EXTERN Sentence senP[]      ; /* Sentence Pool                    */
EXTERN int senPtr           ; /* Sentence Pool Pointer            */
EXTERN int bufPtr           ; /* Buffer�� Pointer                 */
EXTERN PathRep pathNBest    ; /* N-Best Candidates Path�� ����    */
EXTERN PathRep pathOpen     ; /* A*-Algorithm :  Open-Node�� ���� */

PUBLIC long OpenKTS()       ; /* open_kts/2 in sicstus prolog     */
PUBLIC long CloseKTS()      ; /* close_kts/2 in sicstus prolog    */
PUBLIC long MAT()           ; /* mat/5 in sicstus prolog          */

PUBLIC double stateThreshold ; /* State-Based Multiple Threshold  */
PUBLIC double pathThreshold  ; /* Path-Based Multiple Threshold   */
PUBLIC int idxOfPath[__NUMEOJEOLINSENT__] ; /* trellis�� �� path�� idx */

#ifdef ANALYSIS
PUBLIC int numkneojeol      ; /* known ���� ����                  */
PUBLIC int numunkneojeol    ; /* unknown ���� ����                */
PUBLIC int totunknambi      ; /* unknown word�� candidate ����    */
PUBLIC int totknambi        ; /* known word�� candidate ����      */
PUBLIC double totProb = 0.0 ; /* log_2 P(w1..n)                   */
PUBLIC double LogE2         ; /* log_e 2                          */

PUBLIC void PutAnalysis()   ; /* ������ �м� ��� ��� */
#endif

/* Prolog�� Link�ϱ� ���ؼ��� main()�� �ʿ� ���� */

EXTERN double tranprob[__NUMOFTAG__][__NUMOFTAG__] ; 


/*-----------------------------------------------------------*
 *                                                           *
 *          Morphological Analysis and POS Tagging           *
 *                                                           *
 * Input : str       : �Է� ���� "���� ����."                *
 *                                                           *
 *         OPERATOR  : STATE_BEST : State-Based Tagging Best *
 *                   : STATE_MULT : State-Based Tagging Mult *
 *                   : STATE_PATH : State-Based & Path-Based * 
 *                   : PATH_MULT  :  Path-Based Tagging Mult *
 *                   : PATH_BEST  :  Path-Based Tagging Best *
 *                                                           *
 *         state-thr : 0 <= State-Based_Threshold <= 1.0     *
 *         path-thr  : 0 <= Path-Based_Threshold  <= 1.0     *
 *         num_path  : 0 <= Number of candidates             *
 *                                                           *
 *                                                           *
 * Comments : state-thr�� state-based tagging �����         *
 *            �ĺ��� Cutting�ϱ� ���� ����                   * 
 *            P(candidate_j|W) >= State-Th * P(Best|W)       *
 *                                                           *
 *          : path-thr�� path-based tagging �����           *
 *            �ĺ��� Cutting�ϱ� ���� ����                   * 
 *            num_path�� path-based tagging ��� ������ ���� * 
 *            P(candidate_j|W) >= Path-Th * P(Best|W)        *
 *            the number of candidates <= num_path           *
 *                                                           *
 *-----------------------------------------------------------*/

PUBLIC long MAT(str,operator,state_thr,path_thr,num_path)
char str[]       ;      /* Input Sentence : ���� ����.       */ 
long operator    ;      /* STATE_PATH, STATE_MULT, PATH_MULT */ 
                        /* PATH_BEST, STATE_BEST             */
double state_thr ;      /* State-Based Tagging Threshold     */
double path_thr  ;      /*  Path-Based Tagging Threshold     */
long num_path    ;      /* The number of tagging pathes      */
{

  bufPtr = 0 ;   /* �Է� ���� Buffer�� �ʱ�ȭ                */
  senPtr = 1 ;   /* Sentence Pool�� �ʱ�ȭ : senP[0] = 'INI' */

  strcpy(buffer,str) ; 

  DeleteTrellis()   ; /* Trellis�� Deallocate */

  for(; buffer[bufPtr] != '\n' && buffer[bufPtr] != '\0' ; ++bufPtr)
   if((buffer_tag[bufPtr] = CharTag(buffer[bufPtr])) == _HAN_) 
                                    buffer_tag[++bufPtr] = _HAN_ ; 

  buffer[bufPtr] = '\0' ; 
  buffer_tag[bufPtr] = _EOL_ ; 
  TagAdjust() ;            /* �´� Tag�� �����Ѵ�                   */

  if (buffer[0] != ';') {  /* �Է� ������ Comments�� �ƴϴ�         */

#ifdef DEBUG2
       DisplayTag()      ; /* ����� tag�� ���δ�                   */
#endif

       MakeTrellis()     ; /* ���¼��� Trellis������ ���Ѵ�         */
			   /* trellis�� ������ doubly linked list��
                              ����Ǿ��ִ�                          */
#ifdef DEBUG2
       DisplaySentence() ; 
#endif

       if (operator <= STATE_PATH) {   /* State-Based Tagging           */
	 stateThreshold = state_thr ;  /* State-Based Threshold Setting */
	 AlphaCmpttn()   ;             /* Alpha Computation in HMM      */
	 BetaCmpttn()    ;             /* Beta  Computation in HMM      */
       }

       if (operator >= STATE_PATH) {   /* Path-Based Tagging            */
	 pathThreshold = path_thr   ;  /* Path-Based Threshold Setting  */
         ViterbiForwd()  ;             /* Viterbi Forward Computation   */
         if (operator == PATH_BEST) ViterbiBckwd(idxOfPath) ; 
	 else AStar((int)num_path) ; 
       }


#ifdef DEBUG2
       if (operator <= STATE_PATH) DisplayTrellis('S') ;
					      /* State-Based Tagging */
       else DisplayTrellis('P') ;             /* Path-Based  Tagging */
#endif

#ifdef PROLOG

       if (operator == STATE_BEST) Put2PrologS(operator) ; /* State-Based Best */
       else if (operator == STATE_MULT) {                  /* State-Based Mult */
	  ChooseNBest()            ;                 /* cutting with threshold */
	  Put2PrologS(operator)    ;   
        } else if (operator == PATH_BEST) 
			     Put2PrologP(idxOfPath) ;    /* Path-Based Best */ 
       else if (operator == PATH_MULT) {                 /* Path-Based Mult */
	  ChooseMPath()            ; 
          Put2PrologPM()           ;   
       } else if (operator == STATE_PATH) {
	  ChooseNBest()            ;        /* Cutting with state-threshold */
	  ChooseMPath()            ;        /* Cutting with path-threshold  */
	  InterResult()            ;        /* Intersection of the results  */
          Put2PrologS(operator)    ; 
      } else return 0 ;                     /* Tagging Operator Error       */ 

#else

      if (operator == STATE_MULT)     ChooseNBest() ; /* State Cutting      */ 
      else if (operator == PATH_MULT) ChooseMPath() ; /* Path  Cutting      */
      else if (operator == STATE_PATH) {
	  ChooseNBest() ;       /* State-Based Cutting with state-threshold */
	  ChooseMPath() ;       /* Path-Based  Cutting with  path-threshold */
	  InterResult() ;       /* Intersection of the results              */
      } 

#endif

   }

  return 1 ; 

}/*-------------End of MAT-----------------*/



/*--------------------------------------*
 *                                      *
 * OpenKTS : Open Korean-Tagging-System *
 *                                      *
 *--------------------------------------*/

long OpenKTS(dummy)
long dummy ;                 /* dummy for Prolog-Interface */
{
  char * dict_path;
  char fname[256];

  dict_path = getenv("KDICT_PATH");
  fprintf(stderr,"KDICT_PATH=%s\n",dict_path);
  sprintf(fname,"%s/"__DICTFILE__,dict_path);
  dictPtr = DictOpen(fname) ; /* ���� open */
/*  dictPtr = DictOpen(dict_path __DICTFILE__) ; */ /* ���� open */
  
  InitTag() ; /* ���� ���̴� Tag�� �̸� ��� & P(`��'|tag=jcp) */
  InitOpenClass() ; /* Open Class Word�� ���� */

  sprintf(fname,"%s/"__TRANTBLFILE__,dict_path);
  LoadTranTbl(fname)   ; /* ǰ�� ���� table */

  sprintf(fname,"%s/"__TRANFREQFILE__,dict_path);
  LoadTranFreq(fname,tranprob) ; /* ǰ�� ���� FREQ */

  LoadTagFreq(dictPtr)   ; /* ǰ�� �߻� FREQ  */

#ifdef UNKNOWNMODEL3
  sprintf(fname,"%s/"__UNKNOWNFREQ__,dict_path);
  LoadUnknownFreq(fname) ; /* P(Unkn|Tag) , P(Known|Tag) */ 
  sprintf(fname,"%s/"__LEXFEATURE__,dict_path);
  LoadUnknownLexm(fname)  ; /* P(Tag_i|Word_i+1,Tag_i+1)  */
#endif

  InitTrellis()     ; /* Trellis�� Header�� �����             */

#ifdef ANALYSIS
  LogE2 = log((double)2.0) ;   
#endif

  return 1 ; 

}/*-------------End of OpenKTS------------------*/


/*----------------------------------------*
 *                                        *
 * CloseKTS : Close Korean-Tagging-System *
 *                                        *
 *----------------------------------------*/

long CloseKTS(dummy)
long dummy ;                 /* dummy for Prolog-Interface */
{

#ifdef UNKNOWNMODEL3   /* Lex Ȯ���� �̿��� unknown-word model */
  {
    int idx ; 
    for(idx = 0 ; idx < __NUMOFOPENCLASS__ ; ++idx) 
      free(unknownLexm[idx].pttrnPtr) ; 
  }
#endif

  DictClose(dictPtr) ; 

  return 1 ; 

}/*--------End of CloseKTS---------*/


#ifdef ANALYSIS

/*--------------------------------------*
 *                                      *
 * Put Analysis : ������ �м� ��� ��� *
 *                                      *
 *--------------------------------------*/

void PutAnalysis()
{ 
  int numeojeol = numkneojeol + numunkneojeol ;
  int totambi   = totknambi   + totunknambi   ; 
  double entropy    ; 
  double perplexity ; 

  entropy = ((double) (-1.0)/(double) numeojeol) * totProb ; 
  perplexity = pow((double)2.0,entropy) ; 

  printf("---------------Simple Statistics---------------\n\n")      ; 
  printf(" entropy    = -1/n * log_2 P(w1..n) : %f\n",entropy)       ; 
  printf(" Perplexity = 2^entropy             : %f\n",perplexity)    ; 
  printf("                       �� ���� ���� : %d\n",numeojeol)     ; 
  printf("                    Known ���� ���� : %d\n",numkneojeol)   ; 
  printf("                  Unknown ���� ���� : %d\n",numunkneojeol) ; 
  printf("                �� ���� ��ȣ�� ���� : %d\n",totambi)       ; 
  printf("             Known ���� ��ȣ�� ���� : %d\n",totknambi)     ; 
  printf("           Unknown ���� ��ȣ�� ���� : %d\n",totunknambi)   ; 
  if (numeojeol > 0)
    printf("              �� ������ ��ȣ�� ���� : %f\n",
			   (float)totambi/(float)numeojeol) ; 
  if (numkneojeol > 0)
    printf("           Known ������ ��ȣ�� ���� : %f\n",
		       (float)totknambi/(float)numkneojeol) ; 
  if (numunkneojeol > 0)
    printf("         Unknown ������ ��ȣ�� ���� : %f\n",
		   (float)totunknambi/(float)numunkneojeol) ; 

}/*----------End of PutAnalysis------------*/

#endif

/*-----------------------------------------------------------*
 *------------End of KTS(Korean Tagging System)--------------*
 *-----------------------------------------------------------*/




