/*--------------------------------------------------------------*
 *                                                              *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9     *
 *                                                              *
 * Eun, Zong-Zin                                                *
 *                                                              *
 * Tel. (042)869-5551                                           *
 * E-mail : zzeun@adam, jjeun@csone.kaist.ac.kr                 *
 *                                                              *
 * Computer Systems Lab.                                        *
 * Computer Science , KAIST                                     *
 *                                                              *
 * putmarker.c                                                  *
 * ( Korean POS Tagging System : Putting Semantic Marker )      *
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
EXTERN int ModifyInfo() ;  
EXTERN void MergeInfo() ; 

void MakeMarker()  ;
int  IsTagIn()     ;

FILE *marker_ptr   ;
char contents[300] ; 

main(argc,argv)
int argc;
char *argv[];
{
  char pos[10]           ; /* 품사                      */
  char entry[50]         ; /* entry : 형태소            */
  char marker[50]        ; /* Semantic Marker           */
  char kimmocode[80]     ; /* kimmo code                */
  UCHAR info[__NUMATTR__][__ATTRSIZE__] ; 
  UCHAR markerInfo[__ATTRSIZE__] ; /* Semantic Marker Information */ 
  int numInfo                 	 ; /* entry에 대한 정보의 갯수 */
  int errornum   ; 
  int error1 = 0 ; 
  int error2 = 0 ; 

  if (argc != 3) {
	fprintf(stderr,"Usage : putmarker semantic-dictionary DBM-Dictionary\n") ; 
	exit(1) ; 
  }

  if ((marker_ptr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : Semantic Dictionary %s needed\n",argv[1]) ; 
	exit(1) ; 
  }

  database = dbm_open(argv[2],O_RDWR,0644) ; /* open database */
  if (dbm_error(database)) {
	   perror("Here:1");
	   dbm_clearerr(database) ; 
  }
	
  while (fscanf(marker_ptr,"%s %s %s",pos,entry,marker) != EOF) {

    if (!ks2kimmo(entry,kimmocode)) { /* convert entry into kimmo-code */ 
      fprintf(stderr,"Error : entry %s in converting into kimmo-code\n",entry); 
      exit(1) ; 
    }

    inputData.dptr = kimmocode ; /* kimmo code로 바꿔서 dptr에 넣는다  */    
    inputData.dsize = strlen(kimmocode) + 1 ; 
   
    isInData = dbm_fetch(database,inputData) ;     /* 사전에 있나 본다 */ 
    if (dbm_error(database)) {
      perror("Here:2");
      dbm_clearerr(database) ; 
    }

    if (isInData.dptr == NULL) {
      printf("Error : There's no %s\n",entry) ;  
      ++error1 ;     /* 사전에 그 단어가 없다 */    

    } else  {  
      MakeMarker(markerInfo,pos,marker) ;

      numInfo = SplitStr(info,isInData.dptr) ;      /* 정보를 나눈다   */ 
      if (IsTagIn(info,numInfo,markerInfo[0])) {
        if ((errornum = ModifyInfo(info,numInfo,F_M,markerInfo)) == -1) {
         fprintf(stderr,"Error : duplicate marker code in %s %s %s \n",
                          pos,entry,marker) ; 
         ++error2 ; /* duplicate marker code error */ 
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
  fclose(marker_ptr); 
 
  printf("사전에 없는 entry 갯수        : %d\n",error1) ; 
  printf("duplicate marker code error : %d\n",error2) ; 

}/*-----------End of Main-------------*/



/*****************************************
 *                                       *
 * MakeMarker : Semantic Marker Encoding *
 *                                       *
 *****************************************/

void MakeMarker(markerInfo,pos,marker)
UCHAR markerInfo[]    ;                    /* to : markerInfo[]  */
char pos[]            ;                    /*      품사          */
char marker[]         ;
{
  char tmpmarker[50] ;
  int stlen = strlen(marker) ; 

  markerInfo[0] = TagIdx(pos) ; /* Format : 'Pos'  Marker_List   */

  if (stlen == 2) {

     markerInfo[1] = '0'  ; 
     markerInfo[2] = '\0' ;  

  } else {

    markerInfo[1] = '\0'               ; /* make markerInfo[] end as NULL */
    strcpy(tmpmarker,&marker[1])       ;
    tmpmarker[stlen - 2] = '\0'        ; /* (marker) => marker */
    strcat(markerInfo,tmpmarker)       ;

  }

}/*------------End of MakeMarker-------------*/



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

