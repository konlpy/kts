/*
 * splitdiff.c
 * 
 * difference중 unknown word가 있는 부분과 
 *              없는 부분을 나누어 저장한다. 
 *
 * S.H.Lee
 *
 * 
 * 사용법 : splitdiff diff_fname
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void put_file() ; 

FILE *fptr     ; 
FILE *fptr2    ; 
FILE *fptr3    ; 
FILE *fptr4    ; /* 아주 이상한 부분을 쓰는 file */

int worksize ; 
char workarea[100][80] ; 

main(argc,argv)
int argc     ; 
char *argv[] ; 
{
  char buffer[100] ; 

  worksize = 0     ; 

  if (argc < 5) {
    fprintf(stderr,"Usage : splitdiff diff_file known_file unknown_file etc_file\n") ; 
    exit(1) ; 
  }

  if ((fptr = fopen(argv[1],"r")) == NULL) {
    fprintf(stderr,"Error : File Not Found : %s\n",argv[1]) ; 
    exit(1) ; 
  }

  if ((fptr2 = fopen(argv[2],"w")) == NULL) {
    fprintf(stderr,"Error : Can't Make File : %s\n",argv[2]) ; 
    exit(1) ; 
  } 

  if ((fptr3 = fopen(argv[3],"w")) == NULL) {
    fprintf(stderr,"Error : Can't Make File : %s\n",argv[3]) ; 
    exit(1) ; 
  } 

  if ((fptr4 = fopen(argv[4],"w")) == NULL) {
    fprintf(stderr,"Error : Can't Make File : %s\n",argv[4]) ; 
    exit(1) ; 
  } 

  fgets(buffer,99,fptr) ; /* Get new_line */
  strcpy(workarea[worksize++],"\n") ; 

  while (fgets(buffer,99,fptr) != NULL) {

#ifdef DEBUG
    printf("input  : %s",buffer) ; 
#endif 

    if (buffer[0] == '\n') {
      put_file() ; 
      worksize = 0 ; 
    } 

    strcpy(workarea[worksize++],buffer) ; 

  }

  put_file() ;  

  fclose(fptr)  ; 
  fclose(fptr2) ; 
  fclose(fptr3) ; 
  fclose(fptr4) ; 

}/*------------------------------------------------*/

void put_file()
{
  int flag = 0  ; 
  int idx       ; 
  int upline    ; /* up-line     */
  int midline   ; /* middle-line */ 
  int up_file   ; 
  int down_file ; 

  for(idx = 0 ; idx < worksize ; ++idx)
    if (workarea[idx][0] == 'x') ++flag ; /* counter */ 

  for(upline = 0 ; strncmp(workarea[upline],"--------",8) ; 
                    ++upline) ; 
  for(midline = upline+1 ; strncmp(workarea[midline],"--------",8) ; 
                    ++midline) ; 
  
  up_file = midline - upline     ;
  down_file = worksize - midline ; 

  if (up_file != down_file) { /* 아니면 다른 화일에 쓴다 : 수작업 */
    for(idx = 0 ; idx < worksize ; ++idx)
      fprintf(fptr4,"%s",workarea[idx]) ;     
  } else {
  
   if (flag) { /* 중간에 unknown word가 있다는 뜻 */

    if (up_file - 1 > flag) {
     
     fprintf(fptr2,"\n") ; 
     fprintf(fptr2,"--------\n") ; 
    }

     fprintf(fptr3,"\n") ; 
     fprintf(fptr3,"--------\n") ; 
     for(idx = midline+1 ; idx < worksize ; ++idx) {
       if (workarea[idx][0] == 'o') {
         fprintf(fptr2,"%s",workarea[idx-midline+1]) ; 
       } else fprintf(fptr3,"%s",workarea[idx-midline+1]) ; 
     }
   
    if (up_file - 1 > flag) {

     fprintf(fptr2,"--------\n") ; 
    }

     fprintf(fptr3,"--------\n") ; 
     for(idx = midline+1 ; idx < worksize ; ++idx) {
       if (workarea[idx][0] == 'o') {
         fprintf(fptr2,"%s",workarea[idx]) ; 
       } else fprintf(fptr3,"%s",workarea[idx]) ; 
     }
   } else { /* 전부 'o'일 경우 */
    for(idx = 0 ; idx < worksize ; ++idx)
      fprintf(fptr2,"%s",workarea[idx]) ;     

   }

  }

}/*-----End of put_file-----*/


