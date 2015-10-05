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

datum inputData         ; /*  key에 해당하는 것 : 단어        */
datum contentData       ; /*  tag 와 불규칙 code              */
datum isInData          ; /*  이미 있는지 조사하기 위한 datum */
DBM *database           ; 
int tagfr[__NUMOFTAG__] ; /* Tag의 전체 frequency : default=0 */

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
  char contents[100] ; /* 사전의 내용을 쓸 contents      */
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; 
  UCHAR compInfo[__ATTRSIZE__] ; /* 비교할 Information   */
  UCHAR contTag      ; /* contents에 넣을 tag Index      */
  int numInput       ; /* 사전의 내용이 두개 혹은 세개 ? */
  int numInfo        ; /* entry에 대한 정보의 갯수       */

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
    else { /* 준말 , 본디말은 다른 사전을 읽어 처리 */
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
   inputData.dptr = kimmo ; /* kimmo code로 바꿔서 dptr에 넣는다  */    
   inputData.dsize = strlen(kimmo) + 1 ; 
   
   isInData = dbm_fetch(database,inputData) ;   /* 사전에 있나 본다 */ 
   if (dbm_error(database)) {
     perror("Here:2");
     dbm_clearerr(database) ; 
   }
   contTag = TagIdx(tag) ; /* tag index를 구한다    */

   if (isInData.dptr == NULL) {  /*           사전에 그 단어가 없다 */    
     contents[0] = F_T ;         /* Format : Tag , Pos , Irr , Freq */
     contents[1] = contTag ;                                /* Tag  */
     contents[2] = (numInput == 3) ? irr[0] : '0' ;         /* Irr  */
     strcpy(&contents[3],freq) ;  /*       Freq , Delimiter : Space */
   } else {                       /* 사전에 이미 그 단어가 존재한다 */
     numInfo = SplitStr(info,isInData.dptr) ;      /* 정보를 나눈다 */ 
     compInfo[0] = contTag     ;                       /* Tag       */
     compInfo[1] = (numInput == 3) ? irr[0] : '0' ;    /* Irr       */
     strcpy(&compInfo[2],freq) ;                       /* Frequency */

     numInfo = ModifyInfo(info,numInfo,F_T,compInfo) ;  /* Modify Infor Tag */
     MergeInfo(contents,info,numInfo) ; /* 정보를 하나의 string으로 만든다 */
   }

   /* Freq(Tag)를 저장하기 위해서 
    * 이전 Freq(Tag) += Freq(Curr Tag)
    * tagfr[0 .. __NUMOFTAG__]사이에 더해야 할 값이 있다.
    */

   tagfr[contTag-'0'] += atoi(freq) ;  

   contentData.dptr = contents ; 
   contentData.dsize = strlen(contents) + 1 ; /* '\0' character도 포함한다 */
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
 * Marginal Frequency를 더한다. *
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
   
   isInData = dbm_fetch(database,inputData) ;   /* 사전에 있나 본다 */ 
   if (dbm_error(database)) {
     perror("Error in searching `@Tag'");
     dbm_clearerr(database) ; 
   }

   if (isInData.dptr == NULL) {   /*          사전에 그 Tag가 없다 */    
     Myitoa(contfreq,tagfr[idx]) ; 
   } else {                       /* 사전에 이미 그 tag가 존재한다 */
     upfreq = atoi(isInData.dptr) + tagfr[idx] ; 
     Myitoa(contfreq,upfreq) ;    /* integer to Ascii              */
   }

   contentData.dptr  = (char *) contfreq ; 
   contentData.dsize = strlen(contfreq) + 1 ; /* '\0' character도 포함한다 */
   dbm_store(database,inputData,contentData,DBM_REPLACE) ; 
   if (dbm_error(database)) {
     perror("Error in writing Tag frequencies") ; 
     dbm_clearerr(database) ; 
   }
  }
}/*---------End of void ModifyTagFreq()------*/



