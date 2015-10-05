/*------------------------------------------------------------------
 *
 *  Korean Part-Of-Speech Tagging System ( KTS ) Version 0.9
 *
 *  S.H.Lee
 *
 *  Computer Systems Lab.
 * Computer Science , KAIST
 *
 *  mktrntbl.c ( Korean POS Tagging System : Make Transition-Table )
 * 
 *-------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#define __TAGSET__
#include "ktsdefs.h"
#include "ktstbls.h"

unsigned char transTag[__NUMOFTAG__][__NUMOFTAG__] ; 

int linecnt = 0 ; /* line counter */

main(argc,argv)
int argc ; 
char *argv[] ; 
{
  FILE *inPtr  ; /* Input  File : ���� tag sequence�� ���� ȭ�� */
  FILE *outPtr ; /* Output File : ���� tag transition table�� ������ ȭ�� */

  char instr[100] ; /* Input File�κ��� �д� string */
  char tags[15][5] ; /* �ִ� 15���� tag�� �� �����ȿ� ������ �� �ִ� */
  char inter ; 
  int  idx ;   
  int  idx2 ; 
  int  numTag ;
  int  inTagidx ; 

  if (argc != 3) {
	fprintf(stderr,"Usage : mktrntbl TagSeq Table\n") ; 
	exit(1) ; 
  }
  if ((inPtr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : %s not Found\n",argv[1]) ; 
	exit(1) ; 
  }
  if ((outPtr = fopen(argv[2],"w")) == NULL) {
	fprintf(stderr,"Error : Can't Make %s\n",argv[2]) ; 
	exit(1) ; 
  }

  while(fgets(instr,99,inPtr) != NULL) {
    ++linecnt ; 
    numTag = 0 ; 
    inTagidx = 0 ; 
    for(idx = 0 ; instr[idx] != '\n' && instr[idx] != '\0' ; ++idx) 
      if (instr[idx] == ' ') { 
	tags[numTag][inTagidx] = '\0' ; 
        ++numTag ; 
	inTagidx = 0 ; 
      } else tags[numTag][inTagidx++] = instr[idx] ; 
    tags[numTag][inTagidx] = '\0' ; 

    /* numTag : ���� 0..numTag���� tag�� ������ �ִ� */ 
    /* __INI__�κ��� ù��° �Ϳ� ���� �� �� �ִ� */ 

    transTag[TagIdx(__INI__)][TagIdx(tags[0])] = 0x01 ; /* 1 �� true */ 

    for(idx2 = 0 ; idx2 <= numTag - 1 ; ++idx2) 
      transTag[TagIdx(tags[idx2])][TagIdx(tags[idx2+1])] = 0x01 ;  

    /* __FIN__���� ������ ���� ���� �� �� �ִ� */

    transTag[TagIdx(tags[idx2])][TagIdx(__FIN__)] = 0x01 ; 
  }

 /* (jca,jcp) �� (jx,jcp)�� ���� ���ϰ� ����� */

  transTag[TagIdx("jca")][TagIdx("jcp")] = 0x00 ; 
  transTag[TagIdx("jx")][TagIdx("jcp")] = 0x00 ; 

 /* (ef,jx)�� �ٰ� ������ �ȴ�. : ����� �״��Ĵ� �߿��ϴ�. */

  transTag[TagIdx("ef")][TagIdx("jx")] = 0x01 ; 


  for(idx= 0 ; idx < __NUMOFTAG__ ; ++idx) {
   for(idx2= 0 ; idx2 < __NUMOFTAG__ ; ++idx2)
     fputc('0' + transTag[idx][idx2],outPtr) ; 
   fputc('\n',outPtr) ; 
  }

  fclose(inPtr) ; 
  fclose(outPtr) ; 
}


int TagIdx(in)
char in[] ; 
{
  int i ; 
  for (i=0 ; i < __NUMOFTAG__ ; ++i)
	if (!strcmp(in,posTags[i])) return i ; 
  printf ("Error : %d th line : There' no %s:\n",linecnt,in) ; 
}/*----end of TagIdx---*/



