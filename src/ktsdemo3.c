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
#include <stdlib.h>
#include <string.h>

#include "ktsdefs.h"
#include "kts.h"
#include "ktsds.h"
#include "ktslib.h"

void (*outproc)()           ; /* output function */
void DisplayTop10()         ; /* 태깅 결과의 top 10 display */

FILE *fptr1                 ; /* input file  */
FILE *fptr2                 ; /* output file */
FILE *tmpFile               ; /* tmpFile     */

main()
{
  Trellis *scanner  ; /* Trellis Pointer                 */ 
  int     idx, idx2 ; /* Index                           */
  int     backidx   ;
  int     num       ; 
  Path    tmpPath   ; /* 형태소 분석 결과 및 확률 값     */
  char    in[__BUFFERLENGTH__] ; 
  char    _mor[__BUFFERLENGTH__] ; /* Buffer Length      */
  char    _tag[5]   ; /* Tag                             */
  char    infile[80]  ; 
  char    outfile[80] ; 
  char    confirm[100] ; 
  char    ch      ; 
  int     counter ; 
  int     nth     ; 

  long oprtor       ; /* Tagging Operator                */
  double state_th   ; /* State-Based Threshold           */
  double path_th    ; /* Path-Based  Threshold           */
  long   num_pa     ; /* Path-Based Number of Candidates */


  (void) OpenKTS(0) ;        /* 0 is dummy for Prolog-Interface */

  printf("코퍼스로부터 수동 태깅하는 프로그램\n\n") ; 

  printf("사용 방법 : enter-key : 0번째 것을 넣는다.\n") ; 
  printf("            0 - 9     : n번째 것을 넣는다.\n") ; 
  printf("               t      : typing : 0 - 9번째 것을 무시하고 분석 결과를 입력한다.\n") ;
  printf("               q      : 본 프로그램은 끝낸다.\n") ; 


  printf ("Enter the input file name : ") ; 
  scanf("%s",infile) ; 
  printf ("Enter the output file name : ") ; 
  scanf("%s%*c",outfile) ; 

  if ((fptr1 = fopen(infile,"r")) == NULL) {
     fprintf(stderr,"Error : %s not Found\n",infile) ; 
     exit(1) ; 
  }
  if ((fptr2 = fopen(outfile,"w")) == NULL) {
     fprintf(stderr,"Error : can't make %s\n",outfile) ; 
     exit(1) ; 
  }

  /* 형태소 분석 및 Tagging */ 

  oprtor = STATE_MULT ; /* Tagging Mode is STATE_BEST            */
  state_th = 0.0      ; /* State_based tagging threshold is 0.0  */
  path_th = 0.0       ; /* Path_based tagging threshold is 0.0   */
  num_pa = 0          ; /* this is meaningful
			   when oprtor is PATH_MULT & STATE_PATH */

  outproc = DisplayState ; /* Display Format = DisplayState */

  while(fgets(in,__BUFFERLENGTH__ - 1 , fptr1) != NULL) {

    (void) MAT(in,oprtor,state_th,path_th,num_pa) ; 
   
    scanner = trellis.nextPtr ; 

    while (scanner != NULL) {

      DisplayTop10(scanner) ; /* 태깅 결과 Top 10을 Display */ 

      printf ("\n? ") ; 

      ch = getchar()  ; /* return , q , 0 - 9 , t */ 

      switch(ch) {

      case 'q'  : fclose(fptr1) ; 
                  fclose(fptr2) ; 
                  exit(1) ; 

      case 't'  : getchar()     ; /* get return-key */
                  printf("Enter : ") ; 
                  gets(confirm) ; 

                  fprintf(fptr2,"%s\n",confirm) ;  /* 문장을 넣는다. */ 

                  tmpFile = fopen("tmpFile","w") ; 
                  for (backidx = 0 , idx = 0 ; confirm[idx] != '\0' ; ++idx) { 
                    if (confirm[idx] == '+') {
                      confirm[idx] = '\0' ; 
                      for (idx2 = idx ; confirm[idx2] != '/' ; --idx2) ; 
                      fprintf(tmpFile,"%s ",&confirm[idx2+1]) ; 

                      confirm[idx2] = '\0' ; 
                      fprintf(tmpFile,"%s 1\n",&confirm[backidx]) ; 
         
                      backidx = idx + 1; 
                   }
                 }

                 for (idx2 = idx ; confirm[idx2] != '/' ; --idx2) ; 
                 fprintf(tmpFile,"%s ",&confirm[idx2+1]) ; 

                 confirm[idx2] = '\0' ; 
                 fprintf(tmpFile,"%s 1\n",&confirm[backidx]) ; 
                 fclose(tmpFile) ; 
                 system("mkdict tmpFile kts_dict") ; 
                 break ; 
     
     case '\n'  : nth = 0 ; /* 첫번째 것 */
     default    : if (ch != '\n') { nth = ch - '0' ; getchar() ; }
                  tmpPath = scanner->pathPool[scanner->sortedIdx[nth]] ;

                  for (idx2 = 0 ; idx2 < PATHLEN(tmpPath) ; ++idx2) {
                    GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
                    GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
                    fprintf (fptr2,"%s/%s%c",_mor,_tag,MORPHSPACE(INFORINDEX(tmpPath,idx2))) ; 
                  }
                  GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
                  GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
                  fprintf (fptr2,"%s/%s\n",_mor,_tag) ; 

                  if (scanner->known != KN_EOJ) { /* 미등록어 어절일 경우 */
                    GetMORPH(INFORINDEX(tmpPath,0),_mor) ; 
                    GetPOS(INFORINDEX(tmpPath,0),_tag) ; 
          
                    tmpFile = fopen("tmpFile","w") ; 
                    fprintf (tmpFile,"%s %s 1\n",_tag,_mor) ; 
                    fclose(tmpFile) ; 
                    system("mkdict tmpFile kts_dict") ; 
                  }   
                  break ; 

     } /* end of switch */

    scanner = scanner->nextPtr ; 

   } /* while scanner != NULL */

  }
      
  (void) CloseKTS(0) ; 

  return 0 ; 

}/*--------end of main--------*/


/*
 *
 *  태깅 결과를 최대 10개를 display
 *
 */
void DisplayTop10(scanner)
Trellis *scanner ;
{
  Path    tmpPath   ; /* 형태소 분석 결과 및 확률 값     */
  int     topnumber ; 
  int     counter   ; 
  int     idx2      ; 
  char    _mor[__BUFFERLENGTH__] ; /* Buffer Length      */
  char    _tag[5]   ; /* Tag                             */

  topnumber = (scanner->numPath >= 10) ? 10 : scanner->numPath ;  

  for(counter = 0 ; counter < topnumber ; ++counter) {

    tmpPath = scanner->pathPool[scanner->sortedIdx[counter]] ;

    printf ("%d %c ",counter,scanner->known) ; 
    for (idx2 = 0 ; idx2 < PATHLEN(tmpPath) ; ++idx2) {
        GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
        GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
        printf ("%s/%s%c",_mor,_tag,MORPHSPACE(INFORINDEX(tmpPath,idx2))) ; 
    }
    GetMORPH(INFORINDEX(tmpPath,idx2),_mor) ; 
    GetPOS(INFORINDEX(tmpPath,idx2),_tag) ; 
    printf ("%s/%s\n",_mor,_tag) ; 

  }
}/*---------------End of DisplayTop10------------------*/



