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

char in[] = "�ǹ� �ȿ��� ���� �� �ٱ� ���� ���δ�." ; /* ���� */

void (*outproc)()           ; /* output function */

main()
{
  Trellis *scanner  ; /* Trellis Pointer                 */ 
  int     idx, idx2 ; /* Index                           */
  Path    tmpPath   ; /* ���¼� �м� ��� �� Ȯ�� ��     */
  char    _mor[__BUFFERLENGTH__] ; /* Buffer Length      */
  char    _tag[5]   ; /* Tag                             */

  long oprtor       ; /* Tagging Operator                */
  double state_th   ; /* State-Based Threshold           */
  double path_th    ; /* Path-Based  Threshold           */
  long   num_pa     ; /* Path-Based Number of Candidates */

  (void) OpenKTS(0) ;        /* 0 is dummy for Prolog-Interface */

  printf ("�Է� ���� : %s\n",in) ;  

  /* ���¼� �м� �� Tagging */ 

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

  printf ("�׹�° ������ ����\n") ; 

  scanner = trellis.nextPtr->nextPtr->nextPtr->nextPtr ; /* �׹�° ���� */

  printf ("���� : %s\n",scanner->eojeol) ; 
  printf ("���¼� �м� ���� : %d\n",scanner->numPath) ; 
  printf ("��Ͼ� Ȥ�� �̵�Ͼ� : %s\n",
	      (scanner->known == KN_EOJ) ? "��Ͼ�" : "�̵�Ͼ�") ; 

  for (idx = 0 ; idx < scanner->numPath ; ++idx) {
    tmpPath = scanner->pathPool[scanner->sortedIdx[idx]] ;
    printf ("\n%d��° �м� ����� ����\n",idx) ; 
    printf ("���¼� ���� : %d\n",PATHLEN(tmpPath) + 1) ; 

    for (idx2 = 0 ; idx2 < PATHLEN(tmpPath) ; ++idx2) {
      GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
      GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
      printf ("%s/%s%c",_mor,_tag,MORPHSPACE(INFORINDEX(tmpPath,idx2))) ; 
    }
    GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
    GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
    printf ("%s/%s\n",_mor,_tag) ; 
    printf ("Prob(���� �±�|%s) = %lf\n",in,TAGPROB(tmpPath)) ; 

  }


/*************************************/

  (void) CloseKTS(0) ; 

  return 0 ; 

}/*--------end of main--------*/




