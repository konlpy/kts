/*
 * getdiff.c
 * 
 * difference중 ----- 위에 있는 text만 모아서 저장 
 *
 * S.H.Lee
 *
 * 
 * 사용법 : getdiff diff_fname
 *          이렇게 하면 difference 부분만 모을 수 있어서...
 *          이것과 원래 옳은 tagging된 text와 difference를 
 *          구하면 common text 부분을 구할 수 있다.
 *          즉, added된 것으로 나올 것이다.
 *          added된 부분이 바로 common area이다. 
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void put_file() ; 

FILE *fptr     ; 
FILE *fptr2    ; 

int worksize ; 
char workarea[30][80] ; 

main(argc,argv)
int argc     ; 
char *argv[] ; 
{
  char buffer[100] ; 
  int flag = 0 ; 

  worksize = 0     ; 

  if (argc < 3) {
    fprintf(stderr,"Usage : getdiff diff_file target_diff_file\n") ; 
    fprintf(stderr,"        diff를 이용하여 나온 화일 중 윗 부분에 \n") ; 
    fprintf(stderr,"        해당하는 부분만 target_diff_file로 옮김\n") ; 
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

  while (fgets(buffer,99,fptr) != NULL) {

   if(!strncmp(buffer,"--------",8)) flag = 1 - flag ; 
   else {

    if (flag) fprintf(fptr2,"%s",buffer) ; 

   }

  }

  fclose(fptr)  ; 
  fclose(fptr2) ; 

}/*------------------------------------------------*/



