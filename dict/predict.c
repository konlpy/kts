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

datum inputData   ; /* key�� �ش��ϴ� �� : �ܾ�        */
datum contentData ; /* tag �� �ұ�Ģ code              */
datum isInData    ; /* �̹� �ִ��� �����ϱ� ���� datum */
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
  char org_entry[100] ; /* orig entry    : ���¼� �м� ���  */ 
  char red_kimmo[80]  ; /* reduced kimmo code                */
  char contents[100]  ; /* ������ ������ �� contents         */
  JoinMark joinmark   ; /* Prefer-Morph�� �¿� context       */
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; 
  UCHAR compInfo[__ATTRSIZE__] ; /* ���� Information       */
  int numInfo                  ; /* entry�� ���� ������ ���� */
  int errornum                 ; /* -1 : �̹� ��м��� �ִ�  */
  int inputsize                ; /* �Է��� ũ��              */

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
   joinmark.lmark = red_entry[0]           ; /* ��) +�Ͻ�$���� ����   + */
   joinmark.rmark = red_entry[inputsize-1] ; /* ��) +�Ͻ�$���� ������ $ */
   red_entry[inputsize-1] = '\0'           ;
  
   if (!ks2kimmo(&red_entry[1],red_kimmo)) { /* convert entry into kimmo-code */ 
     fprintf(stderr,"Error : entry %s%c in converting into kimmo-code\n",
					 red_entry,joinmark.rmark) ; 
     exit(1) ; 
   }

   inputData.dptr = red_kimmo ; /* kimmo code�� �ٲ㼭 dptr�� �ִ´�  */    
   inputData.dsize = strlen(red_kimmo) + 1 ; 
   
   isInData = dbm_fetch(database,inputData) ;   /* ������ �ֳ� ���� */ 
   if (dbm_error(database)) {
	   perror("Here:2");
	   dbm_clearerr(database) ; 
   }

   Tran2Info(compInfo,org_entry,joinmark) ; /* �̸� �м��� ����� Format�� �ٲ�  */ 

   if (isInData.dptr == NULL) {               /* ������ �� �ܾ ����  */    

     contents[0] = F_F ;             /* Format : PreFerence , �м� ��� */
     strcpy(&contents[1],compInfo) ; /* ���¼� �м��� EnCoded Data      */
   
   } else  {                         /* ������ �̹� �� �ܾ �����Ѵ�  */

     numInfo = SplitStr(info,isInData.dptr) ;         /* ������ ������  */ 
     if ((errornum = ModifyInfo(info,numInfo,F_F,compInfo)) == -1)
	   fprintf(stderr,"Error : ��м��� ������ : %s \n",
					   org_entry) ; 
     if (errornum != -1) numInfo = errornum ;  
     MergeInfo(contents,info,numInfo) ; /* ������ �ϳ��� string���� ����� */

   }

   contentData.dptr = contents ; 
   contentData.dsize = strlen(contents) + 1 ; /* '\0' character�� �����Ѵ� */
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



