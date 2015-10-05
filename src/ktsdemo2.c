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
 * ktsdemo2.c (Korean POS Tagging System Demo Program - II)      *
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

#include "ktsdefs.h"
#include "kts.h"
#include "ktsds.h"
#include "ktslib.h"

char in[] = "건물 안에서 밝은 한 줄기 빛이 보인다." ; /* 예문 */

void (*outproc)()           ; /* output function */

main()
{
  Trellis *scanner  ; /* Trellis Pointer                 */ 
  int     idx, idx2 ; /* Index                           */
  Path    tmpPath   ; /* 형태소 분석 결과 및 확률 값     */
  char    _mor[__BUFFERLENGTH__] ; /* Buffer Length      */
  char    _tag[5]   ; /* Tag                             */

  long oprtor       ; /* Tagging Operator                */
  double state_th   ; /* State-Based Threshold           */
  double path_th    ; /* Path-Based  Threshold           */
  long   num_pa     ; /* Path-Based Number of Candidates */

  (void) OpenKTS(0) ;        /* 0 is dummy for Prolog-Interface */

  printf ("입력 문장 : %s\n",in) ;  

  /* 형태소 분석 및 Tagging */ 

  oprtor = STATE_BEST ; /* Tagging Mode is STATE_BEST            */
  state_th = 0.0      ; /* state_based tagging threshold is 0.0  */
  path_th = 0.0       ; /* path_based tagging threshold is 0.0   */
  num_pa = 0          ; /* this is meaningful
			   when oprtor is PATH_MULT & STATE_PATH */

  outproc = DisplayState ; /* Display Format = DisplayState */

  (void) MAT(in,oprtor,state_th,path_th,num_pa) ; 

  printf ("Tagging Mode is STATE_BEST\n") ;  

  if (oprtor == PATH_BEST) DisplayPath(stdout,idxOfPath) ; /* PATH_BEST     */ 
  else if (oprtor == PATH_MULT) DisplayPathM(stdout)     ; /* PATH_MULT     */
  else (*outproc)(stdout,oprtor) ;  /* STATE_BEST , STATE_MULT , STATE_PATH */
 

  oprtor = STATE_PATH ; /* Tagging Mode is STATE_BEST                */
  state_th = 0.00001  ; /* state_based tagging threshold is 0.00001  */
  path_th = 0.0001    ; /* path_based tagging threshold is 0.00001   */
  num_pa = 4          ; /* this is meaningful
			   when oprtor is PATH_MULT & STATE_PATH */

  outproc = State2FilePcfg ; /* Display Format = DisplayState */

  (void) MAT(in,oprtor,state_th,path_th,num_pa) ; 

  printf ("Tagging Mode is STATE_PATH\n") ;  

  if (oprtor == PATH_BEST) DisplayPath(stdout,idxOfPath) ; /* PATH_BEST     */ 
  else if (oprtor == PATH_MULT) DisplayPathM(stdout)     ; /* PATH_MULT     */
  else (*outproc)(stdout,oprtor) ;  /* STATE_BEST , STATE_MULT , STATE_PATH */


/*************************************/

  printf ("네번째 어절의 정보\n") ; 

  scanner = trellis.nextPtr->nextPtr->nextPtr->nextPtr ; /* 네번째 어절 */

  printf ("어절 : %s\n",scanner->eojeol) ; 
  printf ("형태소 분석 갯수 : %d\n",scanner->numPath) ; 
  printf ("등록어 혹은 미등록어 : %s\n",
	      (scanner->known == KN_EOJ) ? "등록어" : "미등록어") ; 

  for (idx = 0 ; idx < scanner->numPath ; ++idx) {
    tmpPath = scanner->pathPool[scanner->sortedIdx[idx]] ;
    printf ("\n%d번째 분석 결과의 정보\n",idx) ; 
    printf ("형태소 갯수 : %d\n",PATHLEN(tmpPath) + 1) ; 

    for (idx2 = 0 ; idx2 < PATHLEN(tmpPath) ; ++idx2) {
      GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
      GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
      printf ("%s/%s%c",_mor,_tag,MORPHSPACE(INFORINDEX(tmpPath,idx2))) ; 
    }
    GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
    GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
    printf ("%s/%s\n",_mor,_tag) ; 
    printf ("Prob(어절 태그|%s) = %lf\n",in,TAGPROB(tmpPath)) ; 

  }


/*************************************/

  (void) CloseKTS(0) ; 

  return 0 ; 

}/*--------end of main--------*/




