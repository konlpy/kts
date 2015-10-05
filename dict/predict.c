/*------------------------------------------------------------------*
 *                                                                  *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9         *
 *                                                                  *
 * SangHo Lee                                                       *
 *                                                                  *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr                *
 *                                                                  *
 * Computer Systems Lab.                                            *
 * Computer Science , KAIST                                         *
 *                                                                  *
 * predict.c ( Korean POS Tagging System : PreFerence Dictionary )  *
 *                                                                  *
 *------------------------------------------------------------------*/

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

datum inputData   ; /* key에 해당하는 것 : 단어        */
datum contentData ; /* tag 와 불규칙 code              */
datum isInData    ; /* 이미 있는지 조사하기 위한 datum */
DBM *database     ; /* DBM-Dictionary                  */

/* Defined in kimmocode.c */

EXTERN int ks2kimmo() ; 

/* Defined in ktsutil.c */

EXTERN int SplitStr()   ; 
EXTERN UCHAR TagIdx()   ; 
EXTERN int ModifyInfo() ;  
EXTERN void MergeInfo() ; 
EXTERN void Tran2Info() ; 

main(argc,argv) 
int argc ; 
char *argv[] ; 
{

  FILE *dictPtr ; 
  char red_entry[100] ; /* reduced entry : Key               */
  char org_entry[100] ; /* orig entry    : 형태소 분석 결과  */ 
  char red_kimmo[80]  ; /* reduced kimmo code                */
  char contents[100]  ; /* 사전의 내용을 쓸 contents         */
  JoinMark joinmark   ; /* Prefer-Morph의 좌우 context       */
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; 
  UCHAR compInfo[__ATTRSIZE__] ; /* 비교할 Information       */
  int numInfo                  ; /* entry에 대한 정보의 갯수 */
  int errornum                 ; /* -1 : 이미 기분석이 있다  */
  int inputsize                ; /* 입력의 크기              */

  if (argc != 3) {
	fprintf(stderr,"Usage : predict Preference-Dictionary DBM-Dictionary\n") ; 
	exit(1) ; 
  }

  if ((dictPtr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : Dictionary %s needed\n",argv[1]) ; 
	exit(1) ; 
  }
  
  database = dbm_open(argv[2],O_RDWR,0644) ; /* open database */
  if (dbm_error(database)) {
	   perror("Here:1");
	   dbm_clearerr(database) ; 
  }

  while (fscanf(dictPtr,"%s%s",red_entry,org_entry) != EOF) {

   inputsize = strlen(red_entry)           ;
   joinmark.lmark = red_entry[0]           ; /* 예) +일실$에서 왼쪽   + */
   joinmark.rmark = red_entry[inputsize-1] ; /* 예) +일실$에서 오른쪽 $ */
   red_entry[inputsize-1] = '\0'           ;
  
   if (!ks2kimmo(&red_entry[1],red_kimmo)) { /* convert entry into kimmo-code */ 
     fprintf(stderr,"Error : entry %s%c in converting into kimmo-code\n",
					 red_entry,joinmark.rmark) ; 
     exit(1) ; 
   }

   inputData.dptr = red_kimmo ; /* kimmo code로 바꿔서 dptr에 넣는다  */    
   inputData.dsize = strlen(red_kimmo) + 1 ; 
   
   isInData = dbm_fetch(database,inputData) ;   /* 사전에 있나 본다 */ 
   if (dbm_error(database)) {
	   perror("Here:2");
	   dbm_clearerr(database) ; 
   }

   Tran2Info(compInfo,org_entry,joinmark) ; /* 미리 분석된 결과의 Format을 바꿈  */ 

   if (isInData.dptr == NULL) {               /* 사전에 그 단어가 없다  */    

     contents[0] = F_F ;             /* Format : PreFerence , 분석 결과 */
     strcpy(&contents[1],compInfo) ; /* 형태소 분석의 EnCoded Data      */
   
   } else  {                         /* 사전에 이미 그 단어가 존재한다  */

     numInfo = SplitStr(info,isInData.dptr) ;         /* 정보를 나눈다  */ 
     if ((errornum = ModifyInfo(info,numInfo,F_F,compInfo)) == -1)
	   fprintf(stderr,"Error : 기분석이 동일함 : %s \n",
					   org_entry) ; 
     if (errornum != -1) numInfo = errornum ;  
     MergeInfo(contents,info,numInfo) ; /* 정보를 하나의 string으로 만든다 */

   }

   contentData.dptr = contents ; 
   contentData.dsize = strlen(contents) + 1 ; /* '\0' character도 포함한다 */
   dbm_store(database,inputData,contentData,DBM_REPLACE) ; 
   if (dbm_error(database)) {
    perror("Here:3");
    dbm_clearerr(database) ; 
   }

  }

  dbm_close(database) ; 
  fclose(dictPtr); 

  return 0 ; 
}/*-----------End of Main-------------*/



