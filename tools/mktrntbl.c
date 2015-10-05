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
  FILE *inPtr  ; /* Input  File : 어절 tag sequence를 가진 화일 */
  FILE *outPtr ; /* Output File : 어절 tag transition table을 가지는 화일 */

  char instr[100] ; /* Input File로부터 읽는 string */
  char tags[15][5] ; /* 최대 15개의 tag가 한 어절안에 존재할 수 있다 */
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

    /* numTag : 현재 0..numTag까지 tag를 가지고 있다 */ 
    /* __INI__로부터 첫번째 것에 전이 될 수 있다 */ 

    transTag[TagIdx(__INI__)][TagIdx(tags[0])] = 0x01 ; /* 1 은 true */ 

    for(idx2 = 0 ; idx2 <= numTag - 1 ; ++idx2) 
      transTag[TagIdx(tags[idx2])][TagIdx(tags[idx2+1])] = 0x01 ;  

    /* __FIN__으로 마지막 것이 전이 될 수 있다 */

    transTag[TagIdx(tags[idx2])][TagIdx(__FIN__)] = 0x01 ; 
  }

 /* (jca,jcp) 와 (jx,jcp)는 붙지 못하게 만든다 */

  transTag[TagIdx("jca")][TagIdx("jcp")] = 0x00 ; 
  transTag[TagIdx("jx")][TagIdx("jcp")] = 0x00 ; 

 /* (ef,jx)는 붙게 만들어야 된다. : 사느냐 죽느냐는 중요하다. */

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



