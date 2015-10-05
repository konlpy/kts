/*---------------------------------------------------------------------*
 *                                                                     *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9            *
 *                                                                     *
 * SangHo Lee                                                          *
 *                                                                     *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr                   *
 *                                                                     *
 * Computer Systems Lab.                                               *
 * Computer Science , KAIST                                            *
 *                                                                     *
 * lkdict.c ( Korean POS Tagging System : Lookup Dictionary with Freq) *
 *                                                                     *
 *---------------------------------------------------------------------*/

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
#include <fcntl.h>
#include <string.h>
#include "ktsdefs.h"
#include "ktsds.h"
#define __TAGSET__
#include "ktstbls.h"
#define __SEMANTICS__
#include "sem.h"

EXTERN int ks2kimmo()     ; 
EXTERN int kimmo2ks()     ; 
EXTERN int SplitStr()     ; /* 정보를 나눈다 */ 
EXTERN int MrgnFreq()     ; 
EXTERN int DecodePreMor() ; 
EXTERN void GetAVkoIdx()  ; 
EXTERN void GetAkoList()  ; 
EXTERN void GetVkoList()  ; 
EXTERN void GetMarkerList()  ; 

EXTERN int tagfreq[] ; 

datum inputData   ;   /*  key에 해당하는 것 : 단어 */
datum isInData    ;   /*  이미 있는지 조사하기 위한 datum */
DBM *database ; 

main(argc,argv) 
int argc ; 
char *argv[] ; 
{
  JoinMark joinmark ; 
  char kimmo[80]   ; /* kimmo code                           */
  char ks[80]      ; /* 한글 code                            */
  int freq         ; /* Frequency  : Freq(Word,Tag)          */
  int mfreq        ; /* Marginal Frequency  : Freq(Word)     */
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; 
  PreMor premor[__NUMPREMORANAL__] ; /* 형태소 기분석된 내용 */
  int numpremor    ; /* 기분석된 내용의 형태소 갯수          */
  int numInfo      ; /* entry에 대한 정보의 갯수             */
  int  idx,idx2    ; 
  char pos[5]      ; /* Part-Of-Speech          */
  UCHAR akoIdx[8]  ; /* AKO-Index in AKO-List   */
  UCHAR vkoIdx[8]  ; /* VKO-Index in VKO-List   */ 
  char akolist[40] ; /* Human-Readable AKO List */ 
  char vkolist[40] ; /* Human-Readable VKO List */ 
  int numako       ; /* number of AKO List      */
  int numvko       ; /* number of VKO List      */

  if (argc != 3) {
   fprintf(stderr,"Usage : lkdict DBM-Dict keyword\n") ; 
   exit(1) ; 
  }
  
  database = dbm_open(argv[1],O_RDONLY,0644) ; /* open database */
  if (dbm_error(database)) {
    perror("Here:1") ; 
    dbm_clearerr(database) ; 
  }

  if (argv[2][0] == '@') {
    LoadTagFreq(database) ;         /* Tag Frequencies를 Load한다. */
    printf(" %s's frequency is %d\n",argv[2],tagfreq[TagIdx(&argv[2][1])-'0']) ; 
    exit(0) ; 
  }

  if (!ks2kimmo(argv[2],kimmo)) { /* convert entry into kimmo-code */ 
     fprintf(stderr,"Error : %s is not normal hanguel\n",argv[1]) ; 
     exit(1) ; 
  }
  inputData.dptr = kimmo ; /* kimmo code로 바꿔서 dptr에 넣는다  */    
  printf ("Kimmo Code : %s\n",kimmo) ; 
  inputData.dsize = strlen(kimmo) + 1 ; 

  isInData = dbm_fetch(database,inputData) ; /* 사전에 있나 보자 */
  if (dbm_error(database)) {
	perror("Here:1") ; 
	dbm_clearerr(database) ; 
  }

  if (isInData.dptr == NULL) { /* 사전에 그 단어가 없다 */    
     printf("No %s in Dictionary\n",argv[2]) ; 
  } else { /* 사전에 그 단어가 존재한다 */

  numInfo = SplitStr(info,isInData.dptr) ; /* 정보를 나눈다 */ 

#ifdef DEBUG
  printf ("Encoded Data : %s#\n",isInData.dptr) ; 
  printf ("number of Infor : %d\n",numInfo) ; 
#endif

  mfreq = MrgnFreq(info,numInfo) ; 

  /* Freq(Word) = Sum Freq(Word,Tag) for All Tag */

  for(idx = 0 ; idx < numInfo ; ++idx) {
   switch(info[idx][0]) {
    case F_T : 
           freq = atoi(&info[idx][3]) ; 
           printf(" Pos : %3s  Irr : %c  P(T|W) : %f\n",
	          posTags[info[idx][1]-'0'],
	          info[idx][2],(float) freq / (float) mfreq) ; 
           break ; 
    case F_P : 
           printf("====== Pre-Analyzed Data ======\n ") ; 
           numpremor = DecodePreMor(premor, &info[idx][1],&joinmark) ; 
	   printf("%c%s%c : ",joinmark.lmark,argv[2],joinmark.rmark) ;  

           for (idx2 = 0 ; idx2 < numpremor ; ++idx2) {
             kimmo2ks(premor[idx2].word,ks) ; 
	     printf ("%s/%s",ks,posTags[premor[idx2].pos-'0']) ; 
	     switch( premor[idx2].mark ) {
	      case '+' : putchar('+') ; break ; 
	      case '*' : putchar(' ') ; break ; 
	      case '$' : putchar('\n') ; break ;
             }
           }
	   break ; 
    case F_F : 
           printf("====== Preference Morph-Analyzed Data ======\n ") ; 
           numpremor = DecodePreMor(premor, &info[idx][1],&joinmark) ; 
	   printf("%c%s%c : ",joinmark.lmark,argv[2],joinmark.rmark) ;  

           for (idx2 = 0 ; idx2 < numpremor ; ++idx2) {
             kimmo2ks(premor[idx2].word,ks) ; 
	     printf ("%s/%s",ks,posTags[premor[idx2].pos-'0']) ; 
	     switch( premor[idx2].mark ) {
	      case '+' : putchar('+') ; break ; 
	      case '*' : putchar(' ') ; break ; 
	      case '$' : putchar('\n') ; break ;
             }
           }
	   break ; 
    case F_S :
           strcpy(pos,posTags[info[idx][1] - '0']) ; /* 품사 복사 */
           GetAVkoIdx(&info[idx][2],akoIdx,vkoIdx,&numako,&numvko) ; 
           GetAkoList(akoIdx,numako,akolist) ; 
           GetVkoList(vkoIdx,numvko,vkolist) ; 
           printf("%s (%s) (%s)\n",pos,akolist,vkolist) ;
           break ; 
    case F_M :
           strcpy(pos,posTags[info[idx][1] - '0']) ; /* 품사 복사 */
           printf("%s (%s)\n",pos,&info[idx][2]) ;
           break ; 
    case F_L : break ; 
   }
  }
  printf ("Freq(%s) in Corpus : %d\n",argv[2],mfreq) ; 

  }

 dbm_close(database) ; 

 return 0 ; 
}/*-----------End of Main-------------*/


