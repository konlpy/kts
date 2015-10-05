/*
 * freqmrg.c
 * 
 * freq(Known-Word , tag)�� freq(Unknown-Word , tag)�� �����Ѵ�.
 *
 * S.H.Lee
 *
 * 
 * ���� : freqmrge known.1 unknown.1 UNKFREQ
 *
 */

#include <stdio.h>
#include <string.h>
#define __TAGSET__
#include "ktstbls.h"

int TagIdx() ; 

FILE *fptr  ; 
FILE *fptr2 ; 
FILE *fptr3 ; 

int mrgfreq[__NUMOFTAG__][2] ; /* Known , Unknown */

char buffer[10] ; 
int freq ; 

main(argc,argv)
int argc     ; 
char *argv[] ; 
{

  int idx ; 

  if (argc < 4) {
    fprintf(stderr,"Usage : getdiff known_freq unknown_freq UNKFREQ\n") ; 
    fprintf(stderr,"        Frequency�� �����Ͽ� �ϳ��� UNKFREQ�� ����\n") ; 
    exit(1) ; 
  }

  if ((fptr = fopen(argv[1],"r")) == NULL) {
    fprintf(stderr,"Error : File Not Found : %s\n",argv[1]) ; 
    exit(1) ; 
  }

  if ((fptr2 = fopen(argv[2],"r")) == NULL) {
    fprintf(stderr,"Error : File Not Found : %s\n",argv[2]) ; 
    exit(1) ; 
  }

  if ((fptr3 = fopen(argv[3],"w")) == NULL) {
    fprintf(stderr,"Error : File Not Found : %s\n",argv[3]) ; 
    exit(1) ; 
  }

  while (fscanf(fptr,"%s%d",buffer,&freq) != EOF) {
    mrgfreq[TagIdx(buffer)][0] = freq ;    

  }

  while (fscanf(fptr2,"%s%d",buffer,&freq) != EOF) {
    mrgfreq[TagIdx(buffer)][1] = freq ;  

  }

  for(idx = 0 ; idx < __NUMOFTAG__ ; ++idx)
    fprintf(fptr3,"%s %d %d\n",posTags[idx],mrgfreq[idx][0],
                               mrgfreq[idx][1]) ; 

  fclose(fptr)  ; 
  fclose(fptr2) ; 
  fclose(fptr3) ; 

}/*------------------------------------------------*/


/*------------------------------*
 *                              *
 * TagIdx : return Index of Tag *
 *                              *
 *------------------------------*/

int TagIdx(in)
char in[] ;
{
  int i ;

  for (i = 0 ; i < __NUMOFTAG__ ; ++i)
    if (in[0] == posTags[i][0]) {
        if (!strcmp(in,posTags[i])) return i ;
    }
  return __NUMOFTAG__  ; /* Tag�� ������ */

}/*----end of TagIdx---*/


