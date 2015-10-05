/*
 * getdiff.c
 * 
 * difference�� ----- ���� �ִ� text�� ��Ƽ� ���� 
 *
 * S.H.Lee
 *
 * 
 * ���� : getdiff diff_fname
 *          �̷��� �ϸ� difference �κи� ���� �� �־...
 *          �̰Ͱ� ���� ���� tagging�� text�� difference�� 
 *          ���ϸ� common text �κ��� ���� �� �ִ�.
 *          ��, added�� ������ ���� ���̴�.
 *          added�� �κ��� �ٷ� common area�̴�. 
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
    fprintf(stderr,"        diff�� �̿��Ͽ� ���� ȭ�� �� �� �κп� \n") ; 
    fprintf(stderr,"        �ش��ϴ� �κи� target_diff_file�� �ű�\n") ; 
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



