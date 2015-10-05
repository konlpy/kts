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
 * ktsdemo.c (Korean POS Tagging System Demo Program)            *
 *                                                               *
 * KTS is composed of -- ktsmain.c    Main-Body                  *
 *                    -- ktsmoran.c   Morphological Analyzer     *
 *                    -- ktsirr.c     Handling Irregulars        *
 *		      -- ktstagger.c  POS Tagger                 *
 *                    -- ktsunknown.c Handling Unknown Word      *
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
#include "kts.h"
#include "ktsds.h"
#include "ktslib.h"


PUBLIC int GetUserChoice() ; /* 사용자가 원하는 Tagging 방법을   */ 

FILE *fptr           ; /* Input File  */
FILE *fptr2          ; /* Output File */

void (*outproc)()           ; /* output function */

main()
{
  int option      ; 
  char fname[50]  ; /* Input  file */ 
  char fname2[50] ; /* Output file */
  long oprtor     ; /* Tagging Operator */
  double state_th=0.0 ; /* State-Based Threshold */
  double path_th=0.0  ; /* Path-Based  Threshold */
  long   num_pa=0   ; /* Path-Based Number of Candidates */

  char in[__BUFFERLENGTH__] ; 

  (void) OpenKTS(0) ;        /* 0 is dummy for Prolog-Interface */

  /* 형태소 분석 및 Tagging */ 

#define ONLINE
#ifdef ONLINE
  fptr  = stdin  ; 
  fptr2 = stdout ; 
#else

  option = GetUserChoice(&oprtor,&state_th,&path_th,&num_pa) ; 

  printf ("Enter the input file : ") ; 
  scanf("%s",fname) ; 
  printf ("Enter the output file : ") ; 
  scanf("%s",fname2) ;   

  if ((fptr = fopen(fname,"r")) == NULL) {
     fprintf(stderr,"Error : %s not Found\n",fname) ; 
     exit(1) ; 
  }
  if ((fptr2 = fopen(fname2,"w")) == NULL) {
     fprintf(stderr,"Error : can't make %s\n",fname2) ; 
     exit(1) ; 
  }

#endif

//  option = GetUserChoice(&oprtor,&state_th,&path_th,&num_pa) ; 
//  printf ("%d: %f: %f: %d\n",oprtor,state_th,path_th,num_pa);
//  printf ("%d\n",option);

  oprtor=STATE_BEST; 
  outproc = DisplayState;
  option=1;
  while(1) {

#ifdef ONLINE
    if (!option) break ; 
//    printf ("Input : ") ; 
#endif

    if(fgets(in,__BUFFERLENGTH__ - 1 ,fptr) == NULL) break ;  
    (void) MAT(in,oprtor,state_th,path_th,num_pa) ;
    printf("\t = %s",in);
    
    if (oprtor == PATH_BEST) DisplayPath(fptr2,idxOfPath) ; /* PATH_BEST     */ 
    else if (oprtor == PATH_MULT) DisplayPathM(fptr2)     ; /* PATH_MULT     */
    else (*outproc)(fptr2,oprtor) ;  /* STATE_BEST , STATE_MULT , STATE_PATH */

  }

  (void) CloseKTS(0) ; /* 0 is dummy for Prolog-Interface */

#ifdef ANALYSIS
  PutAnalysis() ; 
#endif

#ifndef ONLINE
  fclose(fptr)  ; 
  fclose(fptr2) ; 
#endif

  return 0 ; 

}/*--------end of main--------*/


/*-----------------------*
 *                       *
 * Get the User's Choice *
 *                       *
 *-----------------------*/

PUBLIC int GetUserChoice(oprtor,state_thr,path_thr,num_path)
long   *oprtor    ;   /* Tagging Operator                 */
double *state_thr ;   /* State-Based Tagging의 threshold  */
double *path_thr  ;   /* Path-Based Tagging의 threshold   */
long   *num_path  ;   /* Path-Based일 경우 최대 후보 갯수 */
{
  int numcand ; 
  char inopt  ;
  long thres  ; 

  printf ("            Korean Tagging System\n\n") ; 
  printf ("===============================================\n") ; 
  printf ("                 Tagging Option                \n") ; 
  printf ("===============================================\n") ; 
  printf (" a . State-Based Tagging : Best Candidate Only \n") ; 
  printf (" b . State-Based Tagging : Multiple Candidates \n") ; 
  printf (" c . Path-Based  Tagging : Best Candidate Only \n") ; 
  printf (" d . Path-Based  Tagging : Multiple Candidates \n") ;
  printf (" e . State-Based & Path-Based Tagging          \n") ; 
  printf (" x . Exit                                      \n") ; 
  printf ("===============================================\n") ; 
  printf ("Select Mode : ") ; 
  scanf("%c%*c",&inopt) ; 

  switch(inopt) {
    case 'a' : *oprtor = STATE_BEST ; break ; 
    case 'b' : *oprtor = STATE_MULT ; break ; 
    case 'c' : *oprtor = PATH_BEST  ; break ; 
    case 'd' : *oprtor = PATH_MULT  ; break ; 
    case 'e' : *oprtor = STATE_PATH ; break ; 
    case 'x' : return 0 ; 
  }

  if (inopt == 'a' || inopt == 'b' || inopt == 'e') {
    printf ("=======================\n")  ; 
    printf ("==== Output Format ====\n")  ; 
    printf ("=======================\n")  ; 
    printf (" a . Prolog List Format \n") ; 
    printf (" b . PCFG Parser Format \n") ; 
    printf (" c . Standard Format   \n")  ; 
    printf ("=======================\n")  ; 
    printf ("Select Mode : ") ; 
    scanf("%c%*c",&inopt) ; 

    switch(inopt) {
      case 'a' : outproc = State2FilePrlg ; /* Prolog List Format */
		 break ; 
      case 'b' : outproc = State2FilePcfg ; /* PCFG Format        */
		 break ; 
      case 'c' : outproc = DisplayState   ; /* Standard Format    */
    }
  }

  switch(*oprtor) {
    case STATE_PATH :
    case STATE_MULT : printf ("Enter the State-Based threshold : ") ; 
                      scanf ("%lf%*c",&thres) ; 
                      *state_thr = thres ; 
                      if (*oprtor == STATE_MULT) break ; 
    case PATH_MULT  : printf ("Enter the Path-Based threshold : ") ; 
                      scanf ("%lf%*c",&thres) ; 
                      *path_thr = thres ; 
                        
                      printf ("\nMaximum candidates (1- ) : ") ; 
                      scanf ("%d%*c",&numcand) ; 
                      *num_path = (long) numcand ; 
  }

  return 1 ; 

}/*-----End of GetUserChoice-----*/



