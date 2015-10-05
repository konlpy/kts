/*
 * calcdiff.c
 * 
 * difference를 계산하여 출력한다. : 문장단위에서
 *
 * S.H.Lee
 *
 * 
 * 사용법 : calcdiff2 diff_fname
 *
 *          diff referent_fname output_fname | ediff > temp.diff
 *          awk ' { print $1 } temp.diff > diff_fname
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void calc()    ; 
char typestr() ; 

char workarea[40] ; 
int worksize ; 
int hit_stmt ; 

main(argc,argv)
int argc     ; 
char *argv[] ; 
{
  FILE *fptr       ; 
  char buffer[100] ; 
  worksize = 0     ; 

  if (argc < 2) {
	fprintf(stderr,"Usage : calcdiff diff_file\n") ; 
	exit(1) ; 
  }

  if ((fptr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : File Not Found : %s\n",argv[1]) ; 
	exit(1) ; 
  }

  while (fscanf(fptr,"%s",buffer) != EOF) {

#ifdef DEBUG
    printf("input  : %s\n",buffer) ; 
#endif 

      if (!strcmp(buffer,"EOS")) {
        calc() ; 
		worksize = 0 ; 
      } else workarea[worksize++] = typestr(buffer) ; 
  }

  printf("문장 단위로 볼 때 맞은 갯수 : %d\n",hit_stmt) ; 

}/*------------------------------------------------*/


void calc()
{
  int idx ; 
  int bool = 1 ; 

  if (worksize == 1) {
	if (workarea[0] == 'A') ++hit_stmt ; 
  } else {
    if (workarea[0] == 'A' && workarea[worksize-1] == 'A') {
	  for(idx = 1 ; idx < worksize - 1 ; ++idx) {
		if (workarea[idx] != 'C') {
		  bool = 0 ;  
          break ; 
        } 
      }
      if(bool == 1) ++hit_stmt ; 
    }
  }
}



char typestr(str)
char str[] ; 
{
  if (!strcmp(str,"--------")) return 'A' ; 
  else if (!strcmp(str,"EOS")) return 'B' ; 
  else return 'C' ; 

}/*--------------------*/



