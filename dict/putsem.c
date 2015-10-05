/*--------------------------------------------------------------*
 *                                                              *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9     *
 *                                                              *
 * SangHo Lee                                                   *
 *                                                              *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr            *
 *                                                              *
 * Computer Systems Lab.                                        *
 * Computer Science , KAIST                                     *
 *                                                              *
 * putsem.c ( Korean POS Tagging System : Putting Semantics )   *
 *                                                              *
 *--------------------------------------------------------------*/

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

datum inputData   ; /* key에 해당하는 것 : 단어        */
datum contentData ; /* tag 와 불규칙 code              */
datum isInData    ; /* 이미 있는지 조사하기 위한 datum */
DBM *database     ; /* DBM-Dictionary                  */

/* Defined in kimmocode.c */

EXTERN int ks2kimmo() ; 

/* Defined in ktsutil.c */

EXTERN int SplitStr()   ; 
EXTERN UCHAR TagIdx()   ; 
EXTERN UCHAR AkoIdx()   ; 
EXTERN UCHAR VkoIdx()   ; 
EXTERN int ModifyInfo() ;  
EXTERN void MergeInfo() ; 

int MakeSem()   ; 
int IsTagIn()   ; 
int SplitAVko() ; 

FILE *sem_ptr   ; 
char contents[300] ; 

main(argc,argv) 
int argc ; 
char *argv[] ; 
{

  char pos[10]        ; /* 품사              */
  char tmppos[10]     ; /* 품사              */
  char entry[50]      ; /* entry : 형태소    */
  char ako[50]        ; /* AKO(A Kind Of)    */ 
  char vko[50]        ; /* VKO(Verb Kind Of) */ 
  char akolist[8][20] ; /* AKO List          */
  char vkolist[8][20] ; /* VKO List          */
  int  numAko         ; /* number of AKO     */
  int  numVko         ; /* number of VKO     */
  char kimmocode[80]  ; /* kimmo code        */
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; 
  UCHAR semInfo[__ATTRSIZE__] ; /* Semantic Information */ 
  int numInfo                 ; /* entry에 대한 정보의 갯수 */
  int errornum   ; 
  int error1 = 0 ; 
  int error2 = 0 ; 

  if (argc != 3) {
	fprintf(stderr,"Usage : putsem semantic-dictionary DBM-Dictionary\n") ; 
	exit(1) ; 
  }

  if ((sem_ptr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : Semantic Dictionary %s needed\n",argv[1]) ; 
	exit(1) ; 
  }
  
  database = dbm_open(argv[2],O_RDWR,0644) ; /* open database */
  if (dbm_error(database)) {
	   perror("Here:1");
	   dbm_clearerr(database) ; 
  }

  while (fscanf(sem_ptr,"%s %s %s %s",tmppos,entry,ako,vko) != EOF) {
  
   if (!ks2kimmo(entry,kimmocode)) { /* convert entry into kimmo-code */ 
     fprintf(stderr,"Error : entry %s in converting into kimmo-code\n",
		     entry) ; 
     exit(1) ; 
   }

   inputData.dptr = kimmocode ; /* kimmo code로 바꿔서 dptr에 넣는다  */    
   inputData.dsize = strlen(kimmocode) + 1 ; 
   
   isInData = dbm_fetch(database,inputData) ;     /* 사전에 있나 본다 */ 
   if (dbm_error(database)) {
	   perror("Here:2");
	   dbm_clearerr(database) ; 
   }

   if (isInData.dptr == NULL) ++error1 ;     /* 사전에 그 단어가 없다 */    
   else  {  
     
     strcpy(pos,&tmppos[1])    ; 
     pos[strlen(pos)-1] = '\0' ;                   /* (pos) ==> pos   */ 

     numAko = SplitAVko(ako,akolist) ;             /* 0 .. numAko - 1 */
     numVko = SplitAVko(vko,vkolist) ;             /* 0 .. numVko - 1 */
     if(!MakeSem(semInfo,pos,numAko,akolist,numVko,vkolist)) {
       fprintf(stderr,"Error : semantic code가 틀림 : (%s) %s %s %s\n",
                      pos,entry,ako,vko) ; 
       continue ;   
     } 
 
     numInfo = SplitStr(info,isInData.dptr) ;      /* 정보를 나눈다   */ 
     if (IsTagIn(info,numInfo,semInfo[0])) {
       if ((errornum = ModifyInfo(info,numInfo,F_S,semInfo)) == -1) {
         fprintf(stderr,"Error : duplicate semantic code in (%s) %s %s %s\n",
                         pos,entry,ako,vko) ; 
         ++error2 ; /* duplicate semantic code error */ 
         continue ; 
       } else {
         numInfo = errornum ;  
         MergeInfo(contents,info,numInfo) ; /* 정보를 하나의 string으로 만든다 */

         contentData.dptr = contents ; 
         contentData.dsize = strlen(contents) + 1 ; /* '\0' character도 포함한다 */
         dbm_store(database,inputData,contentData,DBM_REPLACE) ; 
         if (dbm_error(database)) {
           perror("Here:3");
           dbm_clearerr(database) ; 
         }
       }
    }
   }
  }/* end of while */

  dbm_close(database) ; 
  fclose(sem_ptr); 
 
  printf("사전에 없는 entry 갯수        : %d\n",error1) ; 
  printf("duplicate semantic code error : %d\n",error2) ; 

}/*-----------End of Main-------------*/


/************************************
 *                                  *
 * SplitAVko : ako , vko를          *
 *             2-D Matrix로 넣는다. *
 *                                  *
 ************************************/

int SplitAVko(avko,avkolist)
char avko[]         ; 
char avkolist[][20] ; 
{
  int numavko = 0 ; 
  int scanner     ;  
  int idx = 0     ;  
  
  for(scanner = 1 ; avko[scanner] != '\0' ; ++scanner) {
    if (avko[scanner] >= '0' && avko[scanner] <= '9')
       avkolist[numavko][idx++] = avko[scanner] ;
    else {
      if (idx == 0) {
         avkolist[numavko][0] = '0'    ; 
         avkolist[numavko++][1] = '\0' ; 
      } else {
         avkolist[numavko++][idx] = '\0' ; 
         idx = 0 ; 
      }
    }

  }
  return numavko ; 

}/*------------End of SplitAVko------------*/


/********************************
 *                              *
 * MakeSem : Semantics Encoding *
 *                              *
 ********************************/

int MakeSem(semInfo,pos,numAko,akolist,numVko,vkolist)
UCHAR semInfo[]    ;                           /* to : semInfo[]  */
char pos[]         ;                           /* 품사            */
int numAko         ;                           /* 0 .. numAko - 1 */
char akolist[][20] ;                           /* akolist[][..]   */
int numVko         ;                           /* 0 .. numVko - 1 */
char vkolist[][20] ;                           /* vkolist[][..]   */
{
  int idx         ;  
  int flag = 1    ; 
  int scanner = 1 ; 

  semInfo[0] = TagIdx(pos) ; /* Format : 'Pos'  AKO & VKO */

  for(idx = 0 ; idx < numAko ; ++idx) 
    if((semInfo[scanner++] = AkoIdx(akolist[idx])) == 
                      __NUMOFAKO__ + '0') flag = 0 ;  
  semInfo[scanner++] = '&' ; /* separater */
  
  for(idx = 0 ; idx < numVko ; ++idx)
    if((semInfo[scanner++] = VkoIdx(vkolist[idx])) == 
                      __NUMOFVKO__ + '0') flag = 0 ; 
  semInfo[scanner] = '\0' ; 

  return flag ; 
}/*------------End of MakeSem-------------*/


/*******************************
 *                             *
 * IsTagIn : Tag가 있는가 조사 *
 *                             *
 *******************************/

int IsTagIn(info,numInfo,intag)
UCHAR info[][__ATTRSIZE__] ; 
int numInfo      ; 
UCHAR intag      ; 
{
  int idx ; 

  for(idx = 0 ; idx < numInfo ; ++idx)
    if (info[idx][0] == F_T && info[idx][1] == intag) return 1 ; 
  return 0 ; /* False */
}/*-------End of IsTagIn--------*/


