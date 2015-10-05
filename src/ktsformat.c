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
 * ktsoutput.c ( Korean POS Tagging System : Output )               *
 *                                                                  *
 *------------------------------------------------------------------*/
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include "ktsdefs.h"
#include "ktstbls.h"
#include "ktsds.h"
#include "kts.h"

#ifdef PROLOG
#include "runtime.h"
#endif

EXTERN int kimmo2ks()      ; /* kimmo code�� �ѱ۷� �ٲ�            */

PUBLIC void ChooseMPath()    ; 
PUBLIC void ChooseNBest()    ; 
PUBLIC void InterResult()    ;

#ifdef PROLOG
PUBLIC void Put2PrologP()  ; /* Path-Based Tagging Best Candidate   */
PUBLIC void Put2PrologPM() ; /* Path-Based Tagging Multi Candidates */
PUBLIC void Put2PrologS()  ; /* State-Based Tagging Best & Multiple */
PRIVATE void PutRecord()   ; /* recordz in prolog                   */
#else
PUBLIC void DisplayPath()  ; /* Path-Based Tagging Best Candidate   */
PUBLIC void DisplayPathM() ; /* Path-Based Tagging Multi Candidates */
PUBLIC void DisplayState() ; /* State-Based Tagging Best & Multiple */
PUBLIC int  GetMorphTag()    ; 
PUBLIC void DisplayEojeol()  ; /* �ϳ��� ���¼� ����� �����ش�       */
PUBLIC void State2FilePrlg() ; /* Prolog List Version   */
PUBLIC void PutEojeol()      ; 
PUBLIC void State2FilePcfg() ; /* PCFG Terminal Format  */
#endif

#ifdef DEBUG2
PUBLIC void DisplayTrellis()  ; 
PUBLIC void DisplaySentence() ; 
PUBLIC void DisplayTag()      ; /* ������ Meta-Tag���� ȭ�鿡 ���δ�  */
PUBLIC void DisplayPathRep()  ; /* N-Best Path�� ȭ�鿡 Display       */
#endif

EXTERN UCHAR _NCT_   ; /* �ѱ� Tag Set�� ���� Tag     */

EXTERN double stateThreshold             ; /* N-Best�� ���ϱ� ���� Threshold */
EXTERN UCHAR buffer[__BUFFERLENGTH__]    ; 
EXTERN char buffer_tag[__BUFFERLENGTH__] ; 
EXTERN Sentence senP[__SENTLENGTH__]     ; 
EXTERN int bufPtr    ; /* �Է� ���� Buffer�� pointer                           */
EXTERN int senPtr    ; /* Sentence Pool�� �� ���¼��� ����                 */

EXTERN Trellis trellis ;   /* �ϳ��� ������ �ϳ��� Trellis�� �ִ´� : header   */
EXTERN PathRep pathNBest ; /* N-Best Candidates Path�� ���� : Linked-List      */


/**************************************************/
/************ General Cutting Utility *************/
/**************************************************/


/*----------------------------------------------------------------*
 *                                                                *
 * ChooseNBest : Choose N-Best Candidates                         * 
 *                                                                *
 * Input    : path[] : �� Trellis�� �ִ� path�� �ϳ�              *
 * Output   : NULL                                                *
 * Function : Tagging�� ����� ȭ�鿡 ����Ѵ�.                   *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void ChooseNBest()
{
  Trellis *trellisPtr = &trellis ; 
  int select , idx ; 
  double bestprob ; 

  while((trellisPtr=trellisPtr->nextPtr) != NULL) {
      select = trellisPtr->sortedIdx[0] ; 
      bestprob = trellisPtr->pathPool[select].gammaprob ; 
      trellisPtr->pathPool[select].ox[0] = MARK ;     /* State-Based Mult */

      for(idx = 1 ; idx < trellisPtr->numPath ; ++idx) {
	select = trellisPtr->sortedIdx[idx] ; 
        if (bestprob * stateThreshold <= trellisPtr->pathPool[select].gammaprob)
	  trellisPtr->pathPool[select].ox[0] = MARK ; 
      }
  }
}/*-----------End of ChooseNBest------------*/


/*----------------------------------------------------------------*
 *                                                                *
 * ChooseMPath  : Choose Mulitple Path and Marking                *
 *                                                                *
 * Input    : stream                                              *
 * Output   : NULL                                                *
 * Function : Path-Based Tagging Multiple Candidates�� N�� ���   *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void ChooseMPath()
{
  int idx , idx2 ; 
  int pathlen ; 
  int mpath[__NUMEOJEOLINSENT__] ; /* ������ ���� path�� �����Ѵ� */
  PathRep *scanner = &pathNBest ; 
  Trellis *trellisPtr ; 

  while((scanner = scanner->nextPtr) != NULL) {
   pathlen = scanner->epathlen ;   /* �� ���� �׻� ���� ���̴�    */
   for(idx = pathlen ; idx >= 0 ; --idx)
    mpath[pathlen - idx] = scanner->epath[idx] ; 
   mpath[pathlen - idx] = -1 ;     /* End of List                 */

   trellisPtr = trellis.nextPtr ;  /* ù��° list��               */

   for(idx2 = 0 ; mpath[idx2] != -1 ; ++idx2) {

     /* Path-Based Multiple Cutting */

     trellisPtr->pathPool[mpath[idx2]].ox[2] = MARK ; 
     trellisPtr = trellisPtr->nextPtr ;
   }
  }

}/*--------End of ChooseMPath--------*/


/*-----------------------------------------------------------------*
 *                                                                 *
 * InterResult : Intersection of the results                       * 
 *                                                                 *
 * Input    : NULL                                                 *
 * Output   : NULL                                                 *
 * Function : ox[0]�� State-Based Tagging�� ����� �ְ�            *
 *            ox[2]�� Path-Based Tagging�� ����� �ִ�             *
 *            ox[1]�� �� ���� Tagging ����� ��� marking�Ǿ��� �� *
 *            marking��Ų��.                                       *
 *                                                                 *
 *-----------------------------------------------------------------*/

PUBLIC void InterResult()
{
  Trellis *trellisPtr = &trellis ; 
  int idx ; 

  while((trellisPtr=trellisPtr->nextPtr) != NULL) {
   for(idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
     if ( (trellisPtr->pathPool[idx].ox[0] == MARK) &&
	  (trellisPtr->pathPool[idx].ox[2] == MARK) )
           trellisPtr->pathPool[idx].ox[1] = MARK ; 
    
#ifdef DEBUG2
     printf ("%s : %d = %c %c %c\n",trellisPtr->eojeol,idx, 
	     trellisPtr->pathPool[idx].ox[0],
	     trellisPtr->pathPool[idx].ox[2],
             trellisPtr->pathPool[idx].ox[1]) ; 
#endif

   }

  }
   
}/*-----------End of InterResult------------*/


#ifdef PROLOG

/*--------------------------------------------------------------------*
 *                                                                    *
 * Put2PrologS  : Put To Prolog State : Best Only or Multiple         *
 *                                                                    *
 * Input    : state                                                   *
 * Output   : NULL                                                    *
 * Function : Tagging�� ����� Prolog Program�� �Ѱ��ش�.             *
 *                                                                    *
 *     Prolog ���� database�� ������ ���� ������� ����� �Ѱ��ش�.   *
 *                                                                    *
 * recordz(token,ejel(Index,Prob,[ [] , [] , [] ],_).                 *
 *                                                                    *
 * Index      : ������ �м� ��� starting 1                           *
 * [[],[],[]] : �ϳ��� ������ ������ ���¼� ���� �̷�����ִ�         *
 * []         : [Mark,Left-Node,Right-Node,Morpheme,POS]              *
 *                                                                    *
 * Mark       : ������ ���¼Ұ� �ϳ��� �������� ������ ���¼��ΰ� ?   *
 *	        �� , ������ space�� �ִ°� ? [ o | x ]                *
 * Left-Node  : Parsing�� �ϱ� ���� ���� node                         *
 * Right-Node : Parsing�� �ϱ� ���� ������ node                       *
 * Morpheme   : ���¼�                                                *
 * POS        : Part-Of-Speech : �Ļ�                                 *
 *                                                                    *
 *--------------------------------------------------------------------*/

PUBLIC void Put2PrologS(oprtr)
long oprtr  ;                                /* Tagging Operator */
{
  Trellis *trellisPtr = &trellis ; 
  Path *eojeolPath ; 
  int this , idx , idx2 ; 
  int lnode , rnode ; 

  rnode = idx = 0 ; 
  if (oprtr == STATE_BEST) {
    while((trellisPtr=trellisPtr->nextPtr) != NULL) {
      this = trellisPtr->sortedIdx[0] ;  
      eojeolPath = trellisPtr->pathPool ;  
      lnode = rnode ; 
      rnode = lnode + (eojeolPath[this].pathlen + 1) * 10 ;
      PutRecord(idx,lnode,rnode,eojeolPath,this) ; 
      ++idx ; 
    }
  } else {                           /* STATE_MULT , STATE_PATH */

    while((trellisPtr=trellisPtr->nextPtr) != NULL) {
     this = trellisPtr->sortedIdx[0] ; 
     eojeolPath = trellisPtr->pathPool ;  
     lnode = rnode ; 
     rnode = lnode + (eojeolPath[this].pathlen + 1) * 10 ;

     for(idx2 = 0 ; idx2 < trellisPtr->numPath ; ++idx2) {
       this = trellisPtr->sortedIdx[idx2] ; 

                            /* ox[]�� marking�� �Ǿ����� �� ��� */
       if (trellisPtr->pathPool[this].ox[oprtr+1] == MARK) 
                   PutRecord(idx,lnode,rnode,eojeolPath,this) ; 
    }
    ++idx ; 
   }
  }

}/*-----------End of Put2PrologS------------*/


/*--------------------------------------------------------------------*
 *                                                                    *
 * Put2PrologP  : Put To Prolog Path                                  *
 *                                                                    *
 * Input    : path                                                    *
 * Output   : NULL                                                    *
 * Function : Tagging�� ����� Prolog Program�� �Ѱ��ش�.             *
 *                                                                    *
 *     Prolog ���� database�� ������ ���� ������� ����� �Ѱ��ش�.   *
 *                                                                    *
 * recordz(token,ejel(Index,Prob,[ [] , [] , [] ],_).                 *
 *                                                                    *
 * Index      : ������ �м� ��� starting 1                           *
 * [[],[],[]] : �ϳ��� ������ ������ ���¼� ���� �̷�����ִ�         *
 * []         : [Mark,Left-Node,Right-Node,Morpheme,POS]              *
 *                                                                    *
 * Mark       : ������ ���¼Ұ� �ϳ��� �������� ������ ���¼��ΰ� ?   *
 *	        �� , ������ space�� �ִ°� ? [ o | x ]                *
 * Left-Node  : Parsing�� �ϱ� ���� ���� node                         *
 * Right-Node : Parsing�� �ϱ� ���� ������ node                       *
 * Morpheme   : ���¼�                                                *
 * POS        : Part-Of-Speech : �Ļ�                                 *
 *                                                                    *
 *--------------------------------------------------------------------*/

PUBLIC void Put2PrologP(path)
int path[] ; 
{
  Trellis *trellisPtr = trellis.nextPtr ; 
  Path *eojeolPath ; 
  int idx   ; 
  int this  ; 
  int lnode ; 
  int rnode = 0 ; 

  for(idx = 0 ; (this = path[idx]) != -1 ; ++idx) {
    eojeolPath = trellisPtr->pathPool ; 
    lnode = rnode ;
    rnode = lnode + (eojeolPath[this].pathlen + 1) * 10 ;
    PutRecord(idx,lnode,rnode,eojeolPath,this) ; 
	
    trellisPtr = trellisPtr->nextPtr ; 
  }
}/*-------------End of Put2PrologP-----------------*/


/*--------------------------------------------------------------------*
 *                                                                    *
 * Put2PrologPM : Put To Prolog Path Multiple Candidates              *
 *                                                                    *
 * Input    : NULL                                                    *
 * Output   : NULL                                                    *
 * Function : Tagging�� ����� Prolog Program�� �Ѱ��ش�.             *
 *            Put2PrologP()�� Call�ϰ� �װ��� Ȯ�� Prolog�� �ش�.     *
 *                                                                    *
 *            recorded(token,sentprob(P(O|lamda)),_)                  *
 *                                                                    *
 *--------------------------------------------------------------------*/

PUBLIC void Put2PrologPM()
{
  SP_pred_ref pred ;  /* prolog predicate : safe_recordz */
  SP_term inp      ; 
  SP_term prob     ; 
  SP_term comp     ;
  SP_term elem     ; 
  int idx  ; 
  int pathlen ; 
  int mpath[__NUMEOJEOLINSENT__] ; /* ������ ���� path�� �����Ѵ� */
  PathRep *scanner = &pathNBest ; 

  pred = SP_predicate("safe_recordz",2,"") ; 
  SP_put_string(&inp,"token")         ; /* recordz(token,sentprob(),_) */
  SP_put_compound(&comp,"sentprob",1) ; /* sentprob(prob)              */

  while((scanner = scanner->nextPtr) != NULL) {
   pathlen = scanner->epathlen ; /* �� ���� �׻� ���� ���̴� */
   for(idx = pathlen ; idx >= 0 ; --idx)
    mpath[pathlen - idx] = scanner->epath[idx] ; 
   mpath[pathlen - idx] = -1 ;   /* End of List              */

   Put2PrologP(mpath) ;          /* �ϳ��� path�� display    */

   SP_put_float(&prob,scanner->fprob) ; 
   SP_put_arg(1,comp,prob)   ; 

   if(!SP_query(pred,&inp,&comp)) 
	  fprintf(stderr,"Error : Putting Record into Prolog DB\n") ; 

  }/*-end of while-*/

}/*--------End of Put2PrologPM--------*/


/*--------------------------------------------------------------------*
 *                                                                    *
 * PutRecord  : Put Record into Prolog-interDB                        *
 *                                                                    *
 *--------------------------------------------------------------------*/

PRIVATE void PutRecord(idx,lnode,rnode,eojeolPath,pathIdx)
int idx           ;                  /* Index Of Path Pool  */
int lnode         ;                  /* Left-Node           */
int rnode         ;                  /* Right-Node          */
Path eojeolPath[] ;                  /* eojeolPath[pathIdx] */ 
int pathIdx       ; 
{
  int tmpIdx,tmpIdx2 ; 
  int senIdx         ; /* senP[senIdx].end_mark              */
  int skipvalue      ; /* lnode , lnode + skipvalue          */
  int numofmorph     ; /* number of morpheme in one eojeol   */
  int left , right   ; /* morph(mark,left,right,kimmo,pos)   */
  SP_pred_ref pred   ; 
  SP_term inp        ; 
  SP_term number     ; 
  SP_term prob       ; 
  SP_term sp_arg[5]  ; /* Mark , LNode , RNode , Morph , POS */ 
  SP_term t1 , t2    ; /* For making List of morph/5         */
  SP_term comp       ;
  SP_term elem       ; 

  numofmorph = eojeolPath[pathIdx].pathlen + 1 ; /* ���¼� ����        */
  skipvalue = (rnode - lnode) / numofmorph     ; /* skipvalue�� ���Ѵ� */
  left = rnode ; 

  pred = SP_predicate("safe_recordz",2,"") ; 
  SP_put_string(&inp,"token")      ; /* recordz(token,ejel(),_)          */
  SP_put_compound(&comp,"ejel",3)  ; /* ejel(num,prob,[morph(),morph()]) */
  SP_put_integer(&number,idx) ; 
  SP_put_float(&prob,eojeolPath[pathIdx].gammaprob) ; 
  SP_put_arg(1,comp,number) ;        /* ejel(number,prob,[      ])       */ 
  SP_put_arg(2,comp,prob)   ; 

  SP_put_string(&t1,"[]") ; 
  SP_show_term(&t1) ; 

  for(tmpIdx = numofmorph - 1 ; tmpIdx >= 0 ; --tmpIdx) {
   right = left ; 
   left = (tmpIdx == 0) ? lnode : left - skipvalue ; 
   senIdx = eojeolPath[pathIdx].path[tmpIdx] ; 

   if (tmpIdx == (numofmorph - 1)) SP_put_integer(&sp_arg[0],1) ; 
   else SP_put_integer(&sp_arg[0], (senP[senIdx].end_mark == 'o') ? 1 : 0) ; 
   SP_put_integer(&sp_arg[1], left)  ;            /*     Left  - Node Value */
   SP_put_integer(&sp_arg[2], right) ;            /*     Right - Node Value */
   SP_put_string(&sp_arg[3], senP[senIdx].word) ; /*     �̼��� code ���¼� */ 
   SP_put_string(&sp_arg[4], posTags[senP[senIdx].pos-'0']) ; /* POS        */
   SP_put_compound(&elem,"morph",5) ;             /* morph(m,l,r,kimmo,pos) */
   for(tmpIdx2 = 0 ; tmpIdx2 <= 4 ; ++tmpIdx2)
	  SP_put_arg(tmpIdx2+1,elem,sp_arg[tmpIdx2]) ; 

   SP_put_compound(&t2,".",2) ; 
   SP_put_arg(2,t2,t1) ; 
   t1 = elem ; 
   SP_put_arg(1,t2,t1) ; 
   t1 = t2 ; 
  }
  SP_hide_term(&t1) ; 
  SP_put_arg(3,comp,t1) ;  

  if(!SP_query(pred,&inp,&comp)) 
	  fprintf(stderr,"Error : Putting Record into Prolog DB\n") ; 

}/*------------End of PutRecord-------------*/

#else

/******************************************************************/
/***************** Routines For C Programmers *********************/
/******************************************************************/


/*---------------------------------------------------------*
 *                                                         *
 * GetMorphTag  : Get Morph & Tag                          *
 *                                                         *
 * Input    : One-Path , i-th                              * 
 * Output   : TRUE  : 0 <= i <= One-Path's Length          *
 *          : FALSE : i < 0 or i > One-Path's Length       *
 * Function : path.path[i]�� ���¼ҿ� �±׸� return        *
 *                                                         *
 * ���� ���� senP[sen_Idx].end_mark�� 'o'�̸� space�� �ǹ� *
 *                                                         *
 * ��) ��/npp+��/jx  = Path                                *
 *                                                         *
 *     Path.path[0]'Morph : ��    Path.path[1]'Morph : ��  *
 *     Path.path[0]'Tag   : npp   Path.path[1]'Tag   : jx  *
 *                                                         *
 *---------------------------------------------------------*/

PUBLIC int GetMorphTag(pathPool,i_th,_morph,_tag,eoj_space)
Path pathPool   ; /* ���¼� �м� ��� �ϳ�                            */
int i_th        ; /* pathPool.path[i] : i��° ���¼ҿ� ǰ��           */ 
UCHAR _morph[]  ; /* �м��� ���¼� �ϳ�                               */ 
char _tag[]     ; /* �м��� ���¼� �ϳ��� �±�                        */
char *eoj_space ; /* ���� ���� space : �̷��� : "�̷��� ����"�� �ȴ�. */ 
{

  int senIdx ; 

  if (i_th < 0 || i_th > PATHLEN(pathPool)) return FALSE ;  

  senIdx = INFORINDEX(pathPool,i_th) ; 

  GetMORPH(senIdx,_morph) ; /* ���¼Ҹ� _morph�� copy */
  GetPOS(senIdx,_tag)     ; /*   ǰ�縦   _tag�� copy */ 

  *eoj_space = MORPHSPACE(senIdx) ;  

  return TRUE ; 

}/*---------------End of GetMorphTag--------------------*/


/*-------------------------------------------------------*
 *                                                       *
 * DisplayEojeol  : Display Path                         *
 *                                                       *
 * Input    : Path to Stream                             * 
 * Output   : NULL                                       *
 * Function : pathPool[idx]�� �ش��ϴ� ������ ����Ѵ�   *
 *                                                       *
 *-------------------------------------------------------*/

PUBLIC void DisplayEojeol(stream,pathPool,separator)
FILE *stream   ;        /* ����ϴ� stream                */
Path pathPool  ;        /* ���¼� �м� ���               */ 
char separator ;        /* �м� ����� ����ϰ� ���� ���� */ 
{
  UCHAR hanstr[__MAXMORPH__] ; /* ���¼�              */
  char tagstr[5]             ; /* ǰ��                */
  char eoj_sp                ; /* ���¼� ������ space */ 
  int idx                    ; /* index��° ���¼�    */

  /*
   * stream�� "���¼�/tag"���� ����. 
   * eoj_sp�� �ϳ��� "���¼�/tag" ������ ���� ����. 
   * 
   * ��) `�̷���'�� `�̷��� ����'�� �м��� �ȴ�.
   *     �� ��, `�̷���'�� Trellis-Node�� 
   *     `��'������ space�� ���� �ȴ�.
   *    
   *     �̷�/pad+��/xa ��/pv+��/ef
   */

  for(idx = 0 ; idx < PATHLEN(pathPool) ; ++idx) {

    GetMorphTag(pathPool,idx,hanstr,tagstr,&eoj_sp) ; 
    fprintf(stream,"%s/%s%c",hanstr,tagstr,eoj_sp) ; 
      
  }

  /* path�� �� ������ ���¼Ҹ� ����Ѵ�. */

  GetMorphTag(pathPool,idx,hanstr,tagstr,&eoj_sp) ; 
  fprintf(stream,"%s/%s",hanstr,tagstr) ; 
  if (separator != (int)NULL) putc(separator,stream) ; 

  /* ������ ���¼� ���� ' ' Ȥ�� '\n' */

}/*---------------End of DisplayEojeol--------------------*/



/*----------------------------------------------------------------*
 *                                                                *
 * DisplayPath  : Display Path                                    *
 *                                                                *
 * Input    : stream , path[] : �� Trellis�� �ִ� path�� �ϳ�     *
 * Output   : NULL                                                *
 * Function : Path-Based Tagging�� ����� ȭ�鿡 ����Ѵ�.        *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void DisplayPath(stream,path)
FILE *stream ; 
int path[] ; 
{
  Trellis *trellisPtr = trellis.nextPtr ; 
  int idx ; 

  for(idx = 0 ; path[idx] != -1 ; ++idx) {
    fprintf(stream,"%c ",trellisPtr->known) ; 
    DisplayEojeol(stream,trellisPtr->pathPool[path[idx]],NEWLINE) ;
    trellisPtr = trellisPtr->nextPtr ;
  }

}/*--End of DisplayPath--*/


/*----------------------------------------------------------------*
 *                                                                *
 * DisplayPathM  : Display Path Multiple Candidates               *
 *                                                                *
 * Input    : stream                                              *
 * Output   : NULL                                                *
 * Function : Path-Based Tagging Multiple Candidates�� N�� ���   *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void DisplayPathM(stream)
FILE *stream ;
{
  int idx  ; 
  int pathlen ; 
  int mpath[__NUMEOJEOLINSENT__] ; /* ������ ���� path�� �����Ѵ� */
  PathRep *scanner = &pathNBest ; 

  while((scanner = scanner->nextPtr) != NULL) {
   pathlen = scanner->epathlen ;   /* �� ���� �׻� ���� ���̴�    */
   for(idx = pathlen ; idx >= 0 ; --idx)
    mpath[pathlen - idx] = scanner->epath[idx] ; 
   mpath[pathlen - idx] = -1 ;     /* End of List                 */

   DisplayPath(stream,mpath) ;     /* �ϳ��� path�� display       */

   fprintf(stream,"%e\n\n",scanner->fprob) ;      /* path Ȯ�� �� */

  }

}/*--------End of DisplayPathM--------*/


/*----------------------------------------------------------------*
 *                                                                *
 * DisplayState : Display State-Based Tagging Result              *
 *                                                                *
 *        oprtr : STATE_BEST : State-Based Best Candidate Only    *
 *                STATE_MULT , STATE_PATH : Multiple Candidates   *
 *                                                                *
 * Output   : NULL                                                *
 * Function : Tagging�� ����� ȭ�鿡 ����Ѵ�.                   *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void DisplayState(stream,oprtr)
FILE *stream ;
long oprtr   ;    /* oprtr = STATE_BEST , STATE_MULT , STATE_PATH */
{
  Trellis *trellisPtr = &trellis ; 
  int select , idx ; 

  if (oprtr == STATE_BEST) {
    while((trellisPtr=trellisPtr->nextPtr) != NULL) {
      fprintf(stream,"%c ",trellisPtr->known) ; 
      select = trellisPtr->sortedIdx[0] ;  
      DisplayEojeol(stream,trellisPtr->pathPool[select],NEWLINE) ; 

    }

  } else {        /* STATE_MULT , STATE_PATH */

    while((trellisPtr=trellisPtr->nextPtr) != NULL) {

      for(idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
         select = trellisPtr->sortedIdx[idx] ; 
         if (trellisPtr->pathPool[select].ox[oprtr+1] == MARK) {
            fprintf(stream,"%c ",trellisPtr->known) ; 
            DisplayEojeol(stream,trellisPtr->pathPool[select],SPACE) ; 
            fprintf(stream,"%f\n",trellisPtr->pathPool[select].gammaprob) ;
         }
      }
      putc(NEWLINE,stream) ; 
    }
  } 


}/*-----------End of DisplayState------------*/


/*-------------------------------------------------------------------*
 *                                                                   *
 * State2FilePrlg : Write State-Based Result To File (PROLOG)        *
 *                                                                   *
 *              For Prolog Interface Form (Batch Processing)         *
 *                                                                   *
 * Input    : Stream , Operator = STATE_MULT, STATE_BEST, STATE_PATH *
 * Output   : NULL                                                   *
 * Function : State-Based Tagging�� ��� File�� Write                *
 *                                                                   *
 *-------------------------------------------------------------------*/

PUBLIC void State2FilePrlg(stream,oprtr)
FILE *stream ;          /* File descriptor */
long oprtr   ;          /* Operator        */
{
  Trellis *trellisPtr = &trellis ; 
  int select , idx    ; 
  char tot_list[2000] ; /* Prolog Atom : ��ü String       */
  char eojeol[512]    ; 	
  int eojeollen       ; 
  int  tot_ptr = 1    ; 
  tot_list[0] = '['   ; 

  if (oprtr == STATE_BEST) {

    while((trellisPtr=trellisPtr->nextPtr) != NULL) {
      tot_list[tot_ptr++] = '[' ; 

	tot_list[tot_ptr++] = '[' ; 
        select = trellisPtr->sortedIdx[0] ; 
        PutEojeol(eojeol,select,trellisPtr->pathPool) ; 

        eojeollen = strlen(eojeol) ; 
	strcpy(&tot_list[tot_ptr],eojeol) ; 
	tot_ptr += eojeollen ; 
        tot_list[tot_ptr++] = ']' ; 

      tot_list[tot_ptr++] = ']' ; 
      tot_list[tot_ptr++] = ',' ; 
   }
   tot_list[--tot_ptr] = ']'  ; 
   tot_list[++tot_ptr] = '.'  ; 
   tot_list[++tot_ptr] = '\0' ;

  } else {                       /* STATE_MULT , STATE_PATH */

   while((trellisPtr=trellisPtr->nextPtr) != NULL) {
      tot_list[tot_ptr++] = '[' ; 
      for(idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
        select = trellisPtr->sortedIdx[idx] ; 
        if (trellisPtr->pathPool[select].ox[oprtr+1] == MARK) {
	   if (idx != 0) tot_list[tot_ptr++] = ',' ;  

           tot_list[tot_ptr++] = '[' ; 
           PutEojeol(eojeol,select,trellisPtr->pathPool) ; 

           eojeollen = strlen(eojeol) ; 
	   strcpy(&tot_list[tot_ptr],eojeol) ; 
	   tot_ptr += eojeollen ; 
           tot_list[tot_ptr++] = ']' ; 
		
        }
      }

      tot_list[tot_ptr++] = ']' ; 
      tot_list[tot_ptr++] = ',' ; 
   }
   tot_list[--tot_ptr] = ']'  ; 
   tot_list[++tot_ptr] = '.'  ; 
   tot_list[++tot_ptr] = '\0' ;
  
  }

  fprintf(stream,"%s\n",tot_list) ; 

}/*-----------End of State2FilePrlg------------*/


/*-------------------------------------------------------------------*
 *                                                                   *
 * State2FilePcfg : Write State-Based Result To File (PCFG)          *
 *                                                                   *
 *                  For Kim Hyung-Guen's PCFG Parser                 *
 *                                                                   *
 * Input    : Stream , Operator = STATE_MULT, STATE_BEST, STATE_PATH *
 * Output   : NULL                                                   *
 * Function : State-Based Tagging�� ��� File�� Write                *
 *                                                                   *
 *-------------------------------------------------------------------*/

PUBLIC void State2FilePcfg(stream,oprtr)
FILE *stream ;             /* File Descriptor */
long oprtr   ;             /* Operator        */ 
{
  Trellis *trellisPtr = &trellis ; 
  int select , idx    ; 
  int lnode , rnode   ; /* parser�� left-node , right-node */

  if (oprtr == STATE_BEST) { 
    
    rnode = 0 ; 
    while((trellisPtr=trellisPtr->nextPtr) != NULL) {
      lnode = rnode++ ; 
      select = trellisPtr->sortedIdx[0] ; 
      fprintf(stream,"%d %d %s ",lnode,rnode,trellisPtr->eojeol) ; 
      DisplayEojeol(stream,trellisPtr->pathPool[select],' ') ; 

      fprintf(stream,"%f\n",trellisPtr->pathPool[select].gammaprob) ;

    }

  } else {             /* STATE_MULT , STATE_PATH */
    
    rnode = 0 ; 
    while((trellisPtr=trellisPtr->nextPtr) != NULL) {
      lnode = rnode++ ; 
      for(idx = 0 ; idx < trellisPtr->numPath ; ++idx) {
        select = trellisPtr->sortedIdx[idx] ; 
        if (trellisPtr->pathPool[select].ox[oprtr+1] == MARK) {
       
          fprintf(stream,"%d %d %s ",lnode,rnode,trellisPtr->eojeol) ; 
          select = trellisPtr->sortedIdx[idx] ; 
          DisplayEojeol(stream,trellisPtr->pathPool[select],' ') ; 

          fprintf(stream,"%f\n",trellisPtr->pathPool[select].gammaprob) ;
        }
      } /* end of for */
    } /* end of while */

  } /* end of if */

  putc('\n',stream) ; 

}/*-----------End of State2FilePcfg------------*/


/*----------------------------------------------------------------*
 *                                                                *
 * PutEojeol : Put Eojeol into String :                           *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void PutEojeol(str,this,pathPool)
char str[] ; 
int this ; 
Path pathPool[] ; 
{
  UCHAR hanstr[__MAXMORPH__] ; 
  char tmpstr[256] ; 
  int str_ptr = 0 ; 
  int idx     ; 
  int senIdx  ; 

  sprintf(str,"%f,",pathPool[this].gammaprob) ;

  str_ptr = strlen(str) ; 

  for(idx = 0 ; idx <= pathPool[this].pathlen ; ++idx) {
    senIdx = pathPool[this].path[idx] ; 
    if(senP[senIdx].pos >= _NCT_) {
      kimmo2ks(senP[senIdx].word,hanstr) ; 
	  sprintf(tmpstr,"[\'%s\',%s]",hanstr,posTags[senP[senIdx].pos-'0']) ; 
    } else sprintf(tmpstr,"[\'%s\',%s]",senP[senIdx].word,
	                             posTags[senP[senIdx].pos-'0']) ; 
    strcpy(str+str_ptr,tmpstr) ; 
    str_ptr += strlen(tmpstr) ; 
    if (idx < pathPool[this].pathlen) str[str_ptr++] = ',' ; 
  }
  str[str_ptr] = '\0' ; 

}/*---------------End of PutEojeol--------------------*/


#endif


#ifdef DEBUG2
  
/*----------------------------------------------------------*
 *                                                          *
 * DisplayTrellis : Trellis�� ������ ȭ�鿡 ���δ�.         *
 *                  Trellis�� �����ϱ� ���� senP[]�� ������ *
 *          	    ���ƾ� �Ѵ�.                            *
 *                                                          *
 *----------------------------------------------------------*/

PUBLIC void DisplayTrellis(method)
char method ; 
{
  Trellis *trellisPtr = &trellis ; /* header���� �����Ͽ� ���� */
  Path *pathPtr ; 
  int idx1,idx2 ; 

  while((trellisPtr = trellisPtr->nextPtr) != NULL) {
    pathPtr = trellisPtr->pathPool ; 
	printf("    %8s  ���� : %s\n",
			(trellisPtr->known == 'o') ? "Known" : "Unknown" ,
            trellisPtr->eojeol) ; 
	printf("start idx , end idx : %d , %d\n",
			trellisPtr->startIdx , trellisPtr->endIdx) ; 
    if (method == 'S') {
      printf("State-Based Tagging : ") ; 
	    for(idx1 = 0 ; idx1 < trellisPtr->numPath ; ++idx1)
	      printf("%d ",trellisPtr->sortedIdx[idx1]) ; 
      putchar('\n') ; 
    }
    printf("Idx len   lexprob      maxprob   bk  alphaprob      betaprob    gammaprob  path\n") ; 
    for(idx1 = 0 ; idx1 < trellisPtr->numPath ; ++idx1) {
	   printf("%2d  %2d %e %e %2d %e %e %e ",idx1,
			  pathPtr[idx1].pathlen, pathPtr[idx1].lexprob,
			  pathPtr[idx1].maxprob, pathPtr[idx1].backIdx,
              pathPtr[idx1].alphaprob , pathPtr[idx1].betaprob,
			  pathPtr[idx1].gammaprob) ; 

       for(idx2 = 0 ; idx2 <= pathPtr[idx1].pathlen ; ++idx2)
		 printf("%d ",pathPtr[idx1].path[idx2]) ; 
       putchar('\n') ; 
    }
	putchar('\n') ; 
  }
}/*--End of DisplayTrellis--*/


/*------------------------------------------------------------------*
 *                                                                  *
 * DisplaySentence : �� ������ ���¼� Lattice-Structure�� �����ش�. *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC void DisplaySentence()
{
  int i , j ; 

  printf ("IDX mark Sbeg Send Mbegin Mend         word pos     prob    nidx\n") ; 
  for(i = 0 ; i < senPtr ; ++i) {
   printf("%3d %c   %3d  %3d    %3d  %3d     %10s %3s %e ",i,
	 senP[i].end_mark,senP[i].sent_begin,
	 senP[i].sent_end,senP[i].morph_begin,
	 senP[i].morph_end,senP[i].word,posTags[senP[i].pos-'0'],
	 senP[i].prob) ;

   for(j = 0 ; senP[i].nidx[j] != -1 ; ++j)
	 printf ("%d ",senP[i].nidx[j]) ; 
   putchar('\n') ; 
  }
}/*---------End of DisplaySentence----------*/


/*-----------------------------------------------------*
 *                                                     *
 * DisplayTag : �� ������ �� character-tag�� �����ش�. *
 *                                                     *
 *-----------------------------------------------------*/

PUBLIC void DisplayTag()
{
  int i ; 

  printf("%s\n",buffer) ; 
  for(i = 0 ; i <= bufPtr ; ++i)
	putchar(buffer_tag[i]) ; 
  putchar('\n') ; 
}


/*-----------------------------------------------------*
 *                                                     *
 * DisplayPathRep : PathRep List�� ������ Display      *
 *                                                     *
 *-----------------------------------------------------*/

PUBLIC void DisplayPathRep(pathset)
PathRep *pathset ; 
{
  PathRep *scanner = pathset ; 
  int idx ; 

  while((scanner = scanner->nextPtr) != NULL) {
    for(idx = 0 ; idx <= scanner->epathlen ; ++idx)
	  printf("%d ",scanner->epath[idx]) ; 
    printf("%e %s \n",scanner->fprob,scanner->curTrPtr->eojeol) ; 
  }
}/*---------End of DisplayPathRep---------*/




#endif


/*************************************************/
/****************End of ktsoutput.c***************/
/*************************************************/




