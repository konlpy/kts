/*-------------------------------------------------------------------*
 *                                                                   *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9          *
 *                                                                   *
 * SangHo Lee                                                        *
 *                                                                   *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr                 *
 *                                                                   *
 * Computer Systems Lab.                                             *
 * Computer Science , KAIST                                          *
 *                                                                   *
 * mkdict.c ( Korean POS Tagging System : Make Dictionary with Freq) *
 *                                                                   *
 *-------------------------------------------------------------------*/

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
#define __TAGSET__
#include "ktstbls.h"
#define __SEMANTICS__
#include "sem.h"

extern int ks2kimmo()   ; 
extern void Myitoa()    ; 
void ModifyTagFreq()    ; 

datum inputData         ; /*  key�� �ش��ϴ� �� : �ܾ�        */
datum contentData       ; /*  tag �� �ұ�Ģ code              */
datum isInData          ; /*  �̹� �ִ��� �����ϱ� ���� datum */
DBM *database           ; 
int tagfr[__NUMOFTAG__] ; /* Tag�� ��ü frequency : default=0 */

/* Defined in ktsutil.c */

EXTERN int SplitStr()   ; 
EXTERN UCHAR TagIdx()   ; 
EXTERN int NumSpace()   ; 
EXTERN int ModifyInfo() ;  
EXTERN void MergeInfo() ; 

main(argc,argv) 
int argc ; 
char *argv[] ; 
{
  FILE *dictPtr      ; 
  char readDict[100] ; /* input dict                     */
  char entry[80]     ; /* lexicon                        */
  char freq[20]      ; /* frequency                      */
  char tag[7]        ; /* tag                            */
  char irr[3]        ; /* irregular mark                 */
  char kimmo[80]     ; /* kimmo code                     */
  char contents[100] ; /* ������ ������ �� contents      */
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; 
  UCHAR compInfo[__ATTRSIZE__] ; /* ���� Information   */
  UCHAR contTag      ; /* contents�� ���� tag Index      */
  int numInput       ; /* ������ ������ �ΰ� Ȥ�� ���� ? */
  int numInfo        ; /* entry�� ���� ������ ����       */

  int counter = -1   ; 

  if (argc != 3) {
   fprintf(stderr,"Usage : mkdict Dictionary DBM-Dictionary\n") ; 
   exit(1) ; 
  }

  if ((dictPtr = fopen(argv[1],"r")) == NULL) {
   fprintf(stderr,"Error : dictionary %s needed\n",argv[1]) ; 
   exit(1) ; 
  }
  
  database = dbm_open(argv[2],O_RDWR|O_CREAT,0644) ; /* open database */
  if (dbm_error(database)) {
   perror("Here:1");
   dbm_clearerr(database) ; 
  }

  while (fgets(readDict,99,dictPtr) != NULL) {
    numInput = NumSpace(readDict) ; 
    if(numInput == 2) sscanf(readDict,"%s%s%s",tag,entry,freq) ; 
    else if(numInput == 3) sscanf(readDict,"%s%s%s%s",tag,entry,freq,irr) ; 
    else { /* �ظ� , ������ �ٸ� ������ �о� ó�� */
     fprintf(stderr,"Error : Format in Dictionary\n") ; 
     exit(1) ; 
    }

    if (!(++counter%1000)) {
        printf ("current input %d : %s %s %s\n",counter,entry,tag,freq) ; 
    }

   if (!ks2kimmo(entry,kimmo)) { /* convert entry into kimmo-code */ 
     fprintf(stderr,"Error : entry %s in Dic\n",entry) ; 
     exit(1) ; 
   }
   inputData.dptr = kimmo ; /* kimmo code�� �ٲ㼭 dptr�� �ִ´�  */    
   inputData.dsize = strlen(kimmo) + 1 ; 
   
   isInData = dbm_fetch(database,inputData) ;   /* ������ �ֳ� ���� */ 
   if (dbm_error(database)) {
     perror("Here:2");
     dbm_clearerr(database) ; 
   }
   contTag = TagIdx(tag) ; /* tag index�� ���Ѵ�    */

   if (isInData.dptr == NULL) {  /*           ������ �� �ܾ ���� */    
     contents[0] = F_T ;         /* Format : Tag , Pos , Irr , Freq */
     contents[1] = contTag ;                                /* Tag  */
     contents[2] = (numInput == 3) ? irr[0] : '0' ;         /* Irr  */
     strcpy(&contents[3],freq) ;  /*       Freq , Delimiter : Space */
   } else {                       /* ������ �̹� �� �ܾ �����Ѵ� */
     numInfo = SplitStr(info,isInData.dptr) ;      /* ������ ������ */ 
     compInfo[0] = contTag     ;                       /* Tag       */
     compInfo[1] = (numInput == 3) ? irr[0] : '0' ;    /* Irr       */
     strcpy(&compInfo[2],freq) ;                       /* Frequency */

     numInfo = ModifyInfo(info,numInfo,F_T,compInfo) ;  /* Modify Infor Tag */
     MergeInfo(contents,info,numInfo) ; /* ������ �ϳ��� string���� ����� */
   }

   /* Freq(Tag)�� �����ϱ� ���ؼ� 
    * ���� Freq(Tag) += Freq(Curr Tag)
    * tagfr[0 .. __NUMOFTAG__]���̿� ���ؾ� �� ���� �ִ�.
    */

   tagfr[contTag-'0'] += atoi(freq) ;  

   contentData.dptr = contents ; 
   contentData.dsize = strlen(contents) + 1 ; /* '\0' character�� �����Ѵ� */
   dbm_store(database,inputData,contentData,DBM_REPLACE) ; 
   if (dbm_error(database)) {
     perror("Here:3");
     dbm_clearerr(database) ; 
   }
  }
  
  ModifyTagFreq() ; /* Modify Marginal Tag Frequency */

  dbm_close(database) ; 
  fclose(dictPtr); 

  return 0 ; 
}/*-----------End of Main-------------*/

/********************************
 *                              *
 * Modify Tag Frequencies       *
 * Marginal Frequency�� ���Ѵ�. *
 *                              *
 ********************************/

void ModifyTagFreq()
{
  char  tagname[7]   ; /* extended tag name such as '@ecx' starting with '@' */
  UCHAR contfreq[10] ;
  int   upfreq       ; /* old frequency */
  int   idx          ; 

  tagname[0] = '@' ; 
  for(idx = 0 ; idx < __NUMOFTAG__ ; ++idx) {
   strcpy(&tagname[1],posTags[idx]) ; 
   inputData.dptr = tagname ; /* tagname : @INI , @a @ad .. */
   inputData.dsize = strlen(tagname) + 1 ; 
   
   isInData = dbm_fetch(database,inputData) ;   /* ������ �ֳ� ���� */ 
   if (dbm_error(database)) {
     perror("Error in searching `@Tag'");
     dbm_clearerr(database) ; 
   }

   if (isInData.dptr == NULL) {   /*          ������ �� Tag�� ���� */    
     Myitoa(contfreq,tagfr[idx]) ; 
   } else {                       /* ������ �̹� �� tag�� �����Ѵ� */
     upfreq = atoi(isInData.dptr) + tagfr[idx] ; 
     Myitoa(contfreq,upfreq) ;    /* integer to Ascii              */
   }

   contentData.dptr  = (char *) contfreq ; 
   contentData.dsize = strlen(contfreq) + 1 ; /* '\0' character�� �����Ѵ� */
   dbm_store(database,inputData,contentData,DBM_REPLACE) ; 
   if (dbm_error(database)) {
     perror("Error in writing Tag frequencies") ; 
     dbm_clearerr(database) ; 
   }
  }
}/*---------End of void ModifyTagFreq()------*/



