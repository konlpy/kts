/*---------------------------------------------------------------------*
 *                                                                     *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9            *
 *                                                                     *
 * SangHo Lee                                                          *
 *                                                                     *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr                   *
 *                                                                     *
 * Computer Systems Lab.                                               *
 * Computer Science , KAIST                                            *
 *                                                                     *
 * lookup.c ( Korean POS Tagging System : Lookup Dictionary )          *
 *                                                                     *
 *---------------------------------------------------------------------*/

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

#include "ktsdefs.h"
#include "ktsds.h"

#ifdef PROLOG
#include "ktstbls.h"
#include "sem.h"
#else
#define  __TAGSET__
#include "ktstbls.h"
#define  __SEMANTICS__
#include "sem.h"
#endif

#ifdef PROLOG
#include "runtime.h"             /* Prolog Interface */
PRIVATE void PutTag2Prlg() ; 
PRIVATE void PutSem2Prlg() ; 
PRIVATE void PutMarker2Prlg() ; 
#endif

#define DBM_DICT     "kts_dict"  /* DBM Dictionary   */

EXTERN int ks2kimmo()     ; 
EXTERN int kimmo2ks()     ; 
EXTERN int SplitStr()     ; /* ������ ������ */ 
EXTERN int MrgnFreq()     ; 
EXTERN int DecodePreMor() ; 
EXTERN void GetAVkoIdx()  ; 
EXTERN void GetAkoList()  ; 
EXTERN void GetVkoList()  ; 

datum inputData           ; /*  key�� �ش��ϴ� �� : �ܾ� */
datum isInData            ; /*  �̹� �ִ��� �����ϱ� ���� datum */
DBM *database             ; 

UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; /* Lookup�Ͽ��� �� */
PreMor premor[__NUMPREMORANAL__]  ; /* ���¼� ��м��� ���� */
int numpremor     ; /* ��м��� ������ ���¼� ����          */
int numInfo       ; /* entry�� ���� ������ ����             */
UCHAR akoIdx[8]   ; /* AKO-Index in AKO-List                */
UCHAR vkoIdx[8]   ; /* VKO-Index in VKO-List                */ 
int numako        ; /* number of AKO List                   */
int numvko        ; /* number of VKO List                   */

PUBLIC long OpenDict()  ; 
PUBLIC long LkupDict()  ;  
PUBLIC long CloseDict() ; 

/*********************************
 *                               *
 * OpenDict for Prolog Interface *
 *                               *
 *********************************/

PUBLIC long OpenDict(dummy)
long dummy ; 
{
  
  database = dbm_open(DBM_DICT,O_RDONLY,0644) ; /* open database */
  if (dbm_error(database)) {
	perror("Here:1") ; 
	dbm_clearerr(database) ; 
  }
  return 1 ; 

}/*---End of OpenDict---*/ 


/*******************************
 *                             *
 * LkupDict : str ( ���¼� )   *
 *                             *
 *******************************/

PUBLIC long LkupDict(str)
char str[] ;        /* prolog���� ������ string : key */
{
  UCHAR kimmo[80]  ; /* kimmo code                           */
  char ks[80]      ; /* �ѱ� code                            */
  int freq         ; /* Frequency  : Freq(Word,Tag)          */
  int mfreq        ; /* Marginal Frequency  : Freq(Word)     */
  int  idx,idx2    ; 
  char pos[5]      ; /* Part-Of-Speech */


  if (!ks2kimmo((UCHAR*)str,kimmo)) { /* convert entry into kimmo-code */ 
     fprintf(stderr,"Error : %s is not normal hanguel\n",str) ; 
     return 0 ; 
  }

#ifdef DEBUG
  printf ("Lookup String : %s#\n",str) ; 
  printf ("Kimmo Code    : %s#\n",kimmo) ; 
#endif

  inputData.dptr = (char *) kimmo ; /* kimmo code�� �ٲ㼭 dptr�� �ִ´�  */    
  inputData.dsize = strlen(kimmo) + 1 ; 

  isInData = dbm_fetch(database,inputData) ; /* ������ �ֳ� ���� */
  if (dbm_error(database)) {
	perror("Here:1") ; 
	dbm_clearerr(database) ; 
  }

  if (isInData.dptr == NULL) return 0 ; /* Lookup False */ 
  else { /* ������ �� �ܾ �����Ѵ� */

    numInfo = SplitStr(info,isInData.dptr) ; /* ������ ������ */ 

#ifdef DEBUG
    printf ("Encoded Data : %s\n",isInData.dptr) ; 
    printf ("number of Infor : %d\n",numInfo) ; 
#endif

    /* Freq(Word) = Sum Freq(Word,Tag) for All Tag */

#ifdef PROLOG

    for(idx = 0 ; idx < numInfo ; ++idx) {
       switch(info[idx][0]) {
        case F_T : PutTag2Prlg(&info[idx][1]) ; 
                   break ;  
        case F_S : PutSem2Prlg(&info[idx][1]) ; 
                   break ; 
        case F_M : PutMarker2Prlg(&info[idx][1]) ; 
                   break ; 
        case F_L : break ; 
       }
    }

#endif

  }/*--End of Else--*/

 return 1 ; 
}/*-----------End of LkupDict-------------*/


/**********************************
 *                                *
 * CloseDict for Prolog Interface *
 *                                *
 **********************************/

PUBLIC long CloseDict(dummy)
long dummy ; 
{
 dbm_close(database) ; 
 return 1 ; 
}/*---End of CloseDict---*/


#ifdef PROLOG

/*---------------------------------*
 *                                 *
 * PutTag2Prlg : Put Tag To Prolog *
 *                                 *
 * info[0] = POS , info[1] = Irr   *
 *                                 *
 *---------------------------------*/

PRIVATE void PutTag2Prlg(str)
UCHAR str[] ; 
{
  int tmpIdx         ; 
  SP_pred_ref pred   ; 
  SP_term inp        ; 
  SP_term sp_arg[3]  ; /* F_T , pos , irr */ 
  SP_term t1 , t2    ; /* For making List of morph/5         */

  pred = SP_predicate("safe_recordz",2,"") ; 
  SP_put_string(&inp,"dict")    ; /* recordz(dict,[F_T,POS,Irr],_) */
  SP_put_string(&sp_arg[0],"postag") ; /* F_T = attr "postag"      */
  SP_put_string(&sp_arg[1],posTags[str[0]-'0']) ;          /* POS */
  str[2] = '\0'          ; /* ǰ�� (POS)������ Frequency�� ���ش� */ 
  SP_put_string(&sp_arg[2],&str[1]) ; /* �ұ�Ģ �ڵ� ����         */ 

  SP_put_string(&t1,"[]") ; 
  SP_show_term(&t1) ; 

  for(tmpIdx = 2 ; tmpIdx >= 0 ; --tmpIdx) {
     SP_put_compound(&t2,".",2) ; 
     SP_put_arg(2,t2,t1) ; 
     t1 = sp_arg[tmpIdx] ; 
     SP_put_arg(1,t2,t1) ; 
     t1 = t2 ; 
  }
  SP_hide_term(&t1) ; 

  if(!SP_query(pred,&inp,&t1)) 
	  fprintf(stderr,"Error : Putting Record into Prolog DB\n") ; 

}/*------------End of PutTag2Prlg-------------*/


/*-----------------------------------------------------------*
 *                                                           *
 * PutMarker2Prlg : Put Semantic-Marker(english) To Prolog   *
 *                                                           *
 * info[0] = POS , info[1..n] = marker_1,marker2,..marker_m  *
 *                                                           *
 *-----------------------------------------------------------*/

PRIVATE void PutMarker2Prlg(str)
UCHAR str[] ; 
{
  int tmpIdx         ; 
  SP_pred_ref pred   ; 
  SP_term inp        ; 
  SP_term sp_arg[3]  ; /* F_M , pos , markers        */ 
  SP_term args[10]   ; /* Maximum number of args     */
  SP_term t1 , t2    ; /* For making List of morph/5 */
  int     numargs    ; /* the number of arguments    */
  int     strpos     ; /* string position            */

  pred = SP_predicate("safe_recordz",2,"") ; 
  SP_put_string(&inp,"dict")         ; /* recordz(dict,[F_M,POS,[markers|..]],_) */
  SP_put_string(&sp_arg[0],"marker") ; /* F_M = attr "marker"      */
  SP_put_string(&sp_arg[1],posTags[str[0]-'0']) ;           /* POS */

  for(strpos = strlen(str) - 1, 
    numargs = 0 ; strpos > 1 ; --strpos)  {
    if (str[strpos] == ',') {
      if (!strcmp(&str[strpos+1],"0")) 
         SP_put_string(&args[numargs++],"null") ; 
      else SP_put_string(&args[numargs++],&str[strpos+1]) ; 
      str[strpos] = '\0' ; 
    }
  }

  if (!strcmp(&str[strpos],"0")) 
      SP_put_string(&args[numargs++],"null") ; 
  else SP_put_string(&args[numargs++],&str[strpos]) ; 
 
  /* args[0 .. numargs - 1] in reverse order */

  /* put args[..] into sp_arg[2] */ 

  SP_put_string(&sp_arg[2],"[]") ; 
  SP_show_term(&sp_arg[2]) ; 

  for(tmpIdx = 0 ; tmpIdx < numargs ; ++tmpIdx) {
     SP_put_compound(&t2,".",2) ; 
     SP_put_arg(2,t2,sp_arg[2]) ; 
     sp_arg[2] = args[tmpIdx] ; 
     SP_put_arg(1,t2,sp_arg[2]) ; 
     sp_arg[2] = t2 ; 
  }
  SP_hide_term(&sp_arg[2]) ; 

  SP_put_string(&t1,"[]") ; 
  SP_show_term(&t1) ; 

  for(tmpIdx = 2 ; tmpIdx >= 0 ; --tmpIdx) {
     SP_put_compound(&t2,".",2) ; 
     SP_put_arg(2,t2,t1) ; 
     t1 = sp_arg[tmpIdx] ; 
     SP_put_arg(1,t2,t1) ; 
     t1 = t2 ; 
  }
  SP_hide_term(&t1) ; 

  if(!SP_query(pred,&inp,&t1)) 
	  fprintf(stderr,"Error : Putting Record into Prolog DB\n") ; 

}/*------------End of PutMarker2Prlg-------------*/


/*--------------------------------------------------*
 *                                                  *
 * PutSem2Prlg : Put Semantic-Marker(NEC) To Prolog *
 *                                                  *
 * info[0] = POS , (AKO-List,VKO-List)              *
 *                                                  *
 *--------------------------------------------------*/

PRIVATE void PutSem2Prlg(info)
UCHAR info[] ; 
{
  int tmpIdx             ; 
  SP_pred_ref pred       ; 
  SP_term inp            ; 
  SP_term sp_arg[3]      ; /* F_S , pos , [[AKO,,,],[VKO,,,]] */ 
  SP_term ako1,vko1,avko ; 
  SP_term ako2,vko2      ; 
  SP_term t1 , t2        ; /* For making List of morph/5      */

  pred = SP_predicate("safe_recordz",2,"") ; 
  SP_put_string(&inp,"dict")    ; /* recordz(dict,[F_T,POS,Irr],_) */
  SP_put_string(&sp_arg[0],"semtag") ; /* F_T = attr "postag"      */
  SP_put_string(&sp_arg[1],posTags[info[0]-'0']) ;          /* POS */

  GetAVkoIdx(&info[1],akoIdx,vkoIdx,&numako,&numvko) ; 

  SP_put_string(&ako1,"[]") ; 
  SP_show_term(&ako1) ; 
  for(tmpIdx = numako-1 ; tmpIdx >= 0 ; --tmpIdx) {
     SP_put_compound(&ako2,".",2) ; 
     SP_put_arg(2,ako2,ako1) ; 
     SP_put_string(&ako1,akoList[akoIdx[tmpIdx]-'0']) ; 
     SP_put_arg(1,ako2,ako1) ;
     ako1 = ako2 ; 
  }
  SP_hide_term(&ako1) ; 

  SP_put_string(&vko1,"[]") ; 
  SP_show_term(&vko1) ; 
  for(tmpIdx = numvko-1 ; tmpIdx >= 0 ; --tmpIdx) {
     SP_put_compound(&vko2,".",2) ; 
     SP_put_arg(2,vko2,vko1) ; 
     SP_put_string(&vko1,vkoList[vkoIdx[tmpIdx]-'0']) ; 
     SP_put_arg(1,vko2,vko1) ;
     vko1 = vko2 ; 
  }
  SP_hide_term(&vko1) ; 

  SP_put_string(&sp_arg[2],"[]") ; 
  SP_show_term(&sp_arg[2]) ; 
  SP_put_compound(&avko,".",2) ; 
  SP_put_arg(2,avko,sp_arg[2]) ; 
  SP_put_arg(1,avko,vko1) ; 
  sp_arg[2] = avko ; 
  SP_put_compound(&avko,".",2) ; 
  SP_put_arg(2,avko,sp_arg[2]) ; 
  SP_put_arg(1,avko,ako1) ; 
  sp_arg[2] = avko ; 
  SP_hide_term(&sp_arg[2]) ;  

  SP_put_string(&t1,"[]") ; 
  SP_show_term(&t1) ; 
  for(tmpIdx = 2 ; tmpIdx >= 0 ; --tmpIdx) {
     SP_put_compound(&t2,".",2) ; 
     SP_put_arg(2,t2,t1) ; 
     t1 = sp_arg[tmpIdx] ; 
     SP_put_arg(1,t2,t1) ; 
     t1 = t2 ; 
  }
  SP_hide_term(&t1) ; 

  if(!SP_query(pred,&inp,&t1)) 
	  fprintf(stderr,"Error : Putting Record into Prolog DB\n") ; 

}/*------------End of PutSem2Prlg-------------*/

#endif


