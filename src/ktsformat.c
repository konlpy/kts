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

EXTERN int kimmo2ks()      ; /* kimmo code를 한글로 바꿈            */

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
PUBLIC void DisplayEojeol()  ; /* 하나의 형태소 결과를 보여준다       */
PUBLIC void State2FilePrlg() ; /* Prolog List Version   */
PUBLIC void PutEojeol()      ; 
PUBLIC void State2FilePcfg() ; /* PCFG Terminal Format  */
#endif

#ifdef DEBUG2
PUBLIC void DisplayTrellis()  ; 
PUBLIC void DisplaySentence() ; 
PUBLIC void DisplayTag()      ; /* 문장의 Meta-Tag열을 화면에 보인다  */
PUBLIC void DisplayPathRep()  ; /* N-Best Path를 화면에 Display       */
#endif

EXTERN UCHAR _NCT_   ; /* 한글 Tag Set의 시작 Tag     */

EXTERN double stateThreshold             ; /* N-Best를 구하기 위한 Threshold */
EXTERN UCHAR buffer[__BUFFERLENGTH__]    ; 
EXTERN char buffer_tag[__BUFFERLENGTH__] ; 
EXTERN Sentence senP[__SENTLENGTH__]     ; 
EXTERN int bufPtr    ; /* 입력 문장 Buffer의 pointer                           */
EXTERN int senPtr    ; /* Sentence Pool에 들어간 형태소의 갯수                 */

EXTERN Trellis trellis ;   /* 하나의 어절을 하나의 Trellis에 넣는다 : header   */
EXTERN PathRep pathNBest ; /* N-Best Candidates Path를 저장 : Linked-List      */


/**************************************************/
/************ General Cutting Utility *************/
/**************************************************/


/*----------------------------------------------------------------*
 *                                                                *
 * ChooseNBest : Choose N-Best Candidates                         * 
 *                                                                *
 * Input    : path[] : 각 Trellis에 있는 path중 하나              *
 * Output   : NULL                                                *
 * Function : Tagging된 결과를 화면에 출력한다.                   *
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
 * Function : Path-Based Tagging Multiple Candidates를 N개 출력   *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void ChooseMPath()
{
  int idx , idx2 ; 
  int pathlen ; 
  int mpath[__NUMEOJEOLINSENT__] ; /* 문장의 어절 path를 저장한다 */
  PathRep *scanner = &pathNBest ; 
  Trellis *trellisPtr ; 

  while((scanner = scanner->nextPtr) != NULL) {
   pathlen = scanner->epathlen ;   /* 이 값은 항상 같을 것이다    */
   for(idx = pathlen ; idx >= 0 ; --idx)
    mpath[pathlen - idx] = scanner->epath[idx] ; 
   mpath[pathlen - idx] = -1 ;     /* End of List                 */

   trellisPtr = trellis.nextPtr ;  /* 첫번째 list로               */

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
 * Function : ox[0]에 State-Based Tagging의 결과가 있고            *
 *            ox[2]에 Path-Based Tagging의 결과가 있다             *
 *            ox[1]에 두 가지 Tagging 결과가 모두 marking되었을 때 *
 *            marking시킨다.                                       *
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
 * Function : Tagging된 결과를 Prolog Program에 넘겨준다.             *
 *                                                                    *
 *     Prolog 내부 database에 다음과 같은 방법으로 결과를 넘겨준다.   *
 *                                                                    *
 * recordz(token,ejel(Index,Prob,[ [] , [] , [] ],_).                 *
 *                                                                    *
 * Index      : 어절의 분석 결과 starting 1                           *
 * [[],[],[]] : 하나의 어절이 세개의 형태소 열로 이루어져있다         *
 * []         : [Mark,Left-Node,Right-Node,Morpheme,POS]              *
 *                                                                    *
 * Mark       : 현재의 형태소가 하나의 어절에서 마지막 형태소인가 ?   *
 *	        즉 , 다음에 space가 있는가 ? [ o | x ]                *
 * Left-Node  : Parsing을 하기 위한 왼쪽 node                         *
 * Right-Node : Parsing을 하기 위한 오른쪽 node                       *
 * Morpheme   : 형태소                                                *
 * POS        : Part-Of-Speech : 픔사                                 *
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

                            /* ox[]에 marking이 되어있을 때 출력 */
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
 * Function : Tagging된 결과를 Prolog Program에 넘겨준다.             *
 *                                                                    *
 *     Prolog 내부 database에 다음과 같은 방법으로 결과를 넘겨준다.   *
 *                                                                    *
 * recordz(token,ejel(Index,Prob,[ [] , [] , [] ],_).                 *
 *                                                                    *
 * Index      : 어절의 분석 결과 starting 1                           *
 * [[],[],[]] : 하나의 어절이 세개의 형태소 열로 이루어져있다         *
 * []         : [Mark,Left-Node,Right-Node,Morpheme,POS]              *
 *                                                                    *
 * Mark       : 현재의 형태소가 하나의 어절에서 마지막 형태소인가 ?   *
 *	        즉 , 다음에 space가 있는가 ? [ o | x ]                *
 * Left-Node  : Parsing을 하기 위한 왼쪽 node                         *
 * Right-Node : Parsing을 하기 위한 오른쪽 node                       *
 * Morpheme   : 형태소                                                *
 * POS        : Part-Of-Speech : 픔사                                 *
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
 * Function : Tagging된 결과를 Prolog Program에 넘겨준다.             *
 *            Put2PrologP()를 Call하고 그것의 확률 Prolog에 준다.     *
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
  int mpath[__NUMEOJEOLINSENT__] ; /* 문장의 어절 path를 저장한다 */
  PathRep *scanner = &pathNBest ; 

  pred = SP_predicate("safe_recordz",2,"") ; 
  SP_put_string(&inp,"token")         ; /* recordz(token,sentprob(),_) */
  SP_put_compound(&comp,"sentprob",1) ; /* sentprob(prob)              */

  while((scanner = scanner->nextPtr) != NULL) {
   pathlen = scanner->epathlen ; /* 이 값은 항상 같을 것이다 */
   for(idx = pathlen ; idx >= 0 ; --idx)
    mpath[pathlen - idx] = scanner->epath[idx] ; 
   mpath[pathlen - idx] = -1 ;   /* End of List              */

   Put2PrologP(mpath) ;          /* 하나의 path를 display    */

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

  numofmorph = eojeolPath[pathIdx].pathlen + 1 ; /* 형태소 갯수        */
  skipvalue = (rnode - lnode) / numofmorph     ; /* skipvalue를 구한다 */
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
   SP_put_string(&sp_arg[3], senP[senIdx].word) ; /*     이성진 code 형태소 */ 
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
 * Function : path.path[i]의 형태소와 태그를 return        *
 *                                                         *
 * 어절 안의 senP[sen_Idx].end_mark가 'o'이면 space를 의미 *
 *                                                         *
 * 예) 나/npp+는/jx  = Path                                *
 *                                                         *
 *     Path.path[0]'Morph : 나    Path.path[1]'Morph : 는  *
 *     Path.path[0]'Tag   : npp   Path.path[1]'Tag   : jx  *
 *                                                         *
 *---------------------------------------------------------*/

PUBLIC int GetMorphTag(pathPool,i_th,_morph,_tag,eoj_space)
Path pathPool   ; /* 형태소 분석 결과 하나                            */
int i_th        ; /* pathPool.path[i] : i번째 형태소와 품사           */ 
UCHAR _morph[]  ; /* 분석된 형태소 하나                               */ 
char _tag[]     ; /* 분석된 형태소 하나의 태그                        */
char *eoj_space ; /* 어절 안의 space : 이러지 : "이렇게 하지"가 된다. */ 
{

  int senIdx ; 

  if (i_th < 0 || i_th > PATHLEN(pathPool)) return FALSE ;  

  senIdx = INFORINDEX(pathPool,i_th) ; 

  GetMORPH(senIdx,_morph) ; /* 형태소를 _morph로 copy */
  GetPOS(senIdx,_tag)     ; /*   품사를   _tag로 copy */ 

  *eoj_space = MORPHSPACE(senIdx) ;  

  return TRUE ; 

}/*---------------End of GetMorphTag--------------------*/


/*-------------------------------------------------------*
 *                                                       *
 * DisplayEojeol  : Display Path                         *
 *                                                       *
 * Input    : Path to Stream                             * 
 * Output   : NULL                                       *
 * Function : pathPool[idx]에 해당하는 어절을 출력한다   *
 *                                                       *
 *-------------------------------------------------------*/

PUBLIC void DisplayEojeol(stream,pathPool,separator)
FILE *stream   ;        /* 출력하는 stream                */
Path pathPool  ;        /* 형태소 분석 결과               */ 
char separator ;        /* 분석 결과를 출력하고 쓰는 글자 */ 
{
  UCHAR hanstr[__MAXMORPH__] ; /* 형태소              */
  char tagstr[5]             ; /* 품사                */
  char eoj_sp                ; /* 형태소 다음의 space */ 
  int idx                    ; /* index번째 형태소    */

  /*
   * stream에 "형태소/tag"열을 쓴다. 
   * eoj_sp는 하나의 "형태소/tag" 다음에 쓰는 글자. 
   * 
   * 예) `이러지'는 `이렇게 하지'로 분석이 된다.
   *     이 때, `이러지'의 Trellis-Node는 
   *     `게'다음에 space를 갖게 된다.
   *    
   *     이렇/pad+게/xa 하/pv+지/ef
   */

  for(idx = 0 ; idx < PATHLEN(pathPool) ; ++idx) {

    GetMorphTag(pathPool,idx,hanstr,tagstr,&eoj_sp) ; 
    fprintf(stream,"%s/%s%c",hanstr,tagstr,eoj_sp) ; 
      
  }

  /* path의 맨 마지막 형태소를 출력한다. */

  GetMorphTag(pathPool,idx,hanstr,tagstr,&eoj_sp) ; 
  fprintf(stream,"%s/%s",hanstr,tagstr) ; 
  if (separator != (int)NULL) putc(separator,stream) ; 

  /* 마지막 형태소 다음 ' ' 혹은 '\n' */

}/*---------------End of DisplayEojeol--------------------*/



/*----------------------------------------------------------------*
 *                                                                *
 * DisplayPath  : Display Path                                    *
 *                                                                *
 * Input    : stream , path[] : 각 Trellis에 있는 path중 하나     *
 * Output   : NULL                                                *
 * Function : Path-Based Tagging된 결과를 화면에 출력한다.        *
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
 * Function : Path-Based Tagging Multiple Candidates를 N개 출력   *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void DisplayPathM(stream)
FILE *stream ;
{
  int idx  ; 
  int pathlen ; 
  int mpath[__NUMEOJEOLINSENT__] ; /* 문장의 어절 path를 저장한다 */
  PathRep *scanner = &pathNBest ; 

  while((scanner = scanner->nextPtr) != NULL) {
   pathlen = scanner->epathlen ;   /* 이 값은 항상 같을 것이다    */
   for(idx = pathlen ; idx >= 0 ; --idx)
    mpath[pathlen - idx] = scanner->epath[idx] ; 
   mpath[pathlen - idx] = -1 ;     /* End of List                 */

   DisplayPath(stream,mpath) ;     /* 하나의 path를 display       */

   fprintf(stream,"%e\n\n",scanner->fprob) ;      /* path 확률 값 */

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
 * Function : Tagging된 결과를 화면에 출력한다.                   *
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
 * Function : State-Based Tagging된 결과 File에 Write                *
 *                                                                   *
 *-------------------------------------------------------------------*/

PUBLIC void State2FilePrlg(stream,oprtr)
FILE *stream ;          /* File descriptor */
long oprtr   ;          /* Operator        */
{
  Trellis *trellisPtr = &trellis ; 
  int select , idx    ; 
  char tot_list[2000] ; /* Prolog Atom : 전체 String       */
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
 * Function : State-Based Tagging된 결과 File에 Write                *
 *                                                                   *
 *-------------------------------------------------------------------*/

PUBLIC void State2FilePcfg(stream,oprtr)
FILE *stream ;             /* File Descriptor */
long oprtr   ;             /* Operator        */ 
{
  Trellis *trellisPtr = &trellis ; 
  int select , idx    ; 
  int lnode , rnode   ; /* parser의 left-node , right-node */

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
 * DisplayTrellis : Trellis의 내용을 화면에 보인다.         *
 *                  Trellis를 이해하기 위해 senP[]의 내용을 *
 *          	    보아야 한다.                            *
 *                                                          *
 *----------------------------------------------------------*/

PUBLIC void DisplayTrellis(method)
char method ; 
{
  Trellis *trellisPtr = &trellis ; /* header부터 시작하여 진행 */
  Path *pathPtr ; 
  int idx1,idx2 ; 

  while((trellisPtr = trellisPtr->nextPtr) != NULL) {
    pathPtr = trellisPtr->pathPool ; 
	printf("    %8s  어절 : %s\n",
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
 * DisplaySentence : 한 문장의 형태소 Lattice-Structure를 보여준다. *
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
 * DisplayTag : 한 문장의 각 character-tag를 보여준다. *
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
 * DisplayPathRep : PathRep List의 내용을 Display      *
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




