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
 * getdict.c ( Korean POS Tagging System : Get Dictionary from dbm ) *
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
#include "ktsds.h"
#define __TAGSET__
#include "ktstbls.h"
#define __SEMANTICS__
#include "sem.h"

datum key         ; /* key에 해당하는 것 : 단어 */
datum dbminfor    ; /* tag 와 불규칙 code       */
DBM *database     ; /* dbm - dictionary         */
FILE *dict_ptr1   ; /* core - dictionary        */
FILE *dict_ptr2   ; /* exception - dictionary   */
FILE *dict_ptr3   ; /* semantic  - dictionary   */
FILE *dict_ptr4   ; /* preference  - dictionary */
FILE *dict_ptr5   ; /* marker  - dictionary     */

void Put2File1()  ; 
void Put2File2()  ; 
void Put2File3()  ; 
void Put2File4()  ; 
void Put2File5()  ; 

/* Defined in ktsutil.c */

EXTERN int SplitStr()   ; 
EXTERN int kimmo2ks() ; 
EXTERN int DecodePreMor() ; 
EXTERN void GetAVkoIdx() ; 
EXTERN void GetAkoList() ; 
EXTERN void GetVkoList() ; 


main(argc,argv) 
int argc ; 
char *argv[] ; 
{
  UCHAR entry[80]    ; /* lexicon                        */
  UCHAR info[__NUMATTR__][__ATTRSIZE__]  ; 
  int numInfo        ; /* entry에 대한 정보의 갯수       */
  int infoIdx        ; /* 정보의 갯수                    */

  int counter = -1   ; 

  if (argc != 7) {
	fprintf(stderr,"Usage : getdict main-dict exc-dict sem-dict \
prefer-dict marker-dict DBM-Dictionary\n") ; 
	fprintf(stderr,"        get data from dbm-dictionary &\n") ; 
	fprintf(stderr,"        make human-readable dictionary\n") ; 
	exit(1) ; 
  }

  if ((dict_ptr1 = fopen(argv[1],"w")) == NULL) {
	fprintf(stderr,"Error : can't make %s\n",argv[1]) ; 
	exit(1) ; 
  }

  if ((dict_ptr2 = fopen(argv[2],"w")) == NULL) {
	fprintf(stderr,"Error : can't make %s\n",argv[2]) ; 
	exit(1) ; 
  }

  if ((dict_ptr3 = fopen(argv[3],"w")) == NULL) {
	fprintf(stderr,"Error : can't make %s\n",argv[3]) ; 
	exit(1) ; 
  }

  if ((dict_ptr4 = fopen(argv[4],"w")) == NULL) {
	fprintf(stderr,"Error : can't make %s\n",argv[4]) ; 
	exit(1) ; 
  }
 

  if ((dict_ptr5 = fopen(argv[5],"w")) == NULL) {
	fprintf(stderr,"Error : can't make %s\n",argv[5]) ; 
	exit(1) ; 
  }

  database = dbm_open(argv[6],O_RDWR|O_CREAT,0644) ; /* open database */
  if (dbm_error(database)) {
	   perror("Here:1");
	   dbm_clearerr(database) ; 
  }

  for (key = dbm_firstkey(database) ; key.dptr != NULL ; 
	                         key = dbm_nextkey(database) ) {

    if (key.dptr[0] == '@') continue ; /* 품사 */ 

    if(!kimmo2ks(key.dptr,entry)) {
      fprintf(stderr,"Error in kimmo-code : %s\n",key.dptr) ; 
      exit(1) ; 
    }
   
    dbminfor = dbm_fetch(database,key) ; /* 사전에 있나 본다 */ 
    if (dbm_error(database)) {
      perror("Here:2");
      dbm_clearerr(database) ; 
    }
    numInfo = SplitStr(info,dbminfor.dptr) ;    /* 정보를 나눈다 */ 
    for(infoIdx = 0 ; infoIdx < numInfo ; ++infoIdx) {
       if (info[infoIdx][0] == F_T) Put2File1(info[infoIdx],entry) ; 
       else if (info[infoIdx][0] == F_P) Put2File2(info[infoIdx],entry) ; 
       else if (info[infoIdx][0] == F_S) Put2File3(info[infoIdx],entry) ; 
       else if (info[infoIdx][0] == F_F) Put2File4(info[infoIdx],entry) ; 
       else if (info[infoIdx][0] == F_M) Put2File5(info[infoIdx],entry) ; 

    } 
  }

  dbm_close(database) ; 
  fclose(dict_ptr1); 
  fclose(dict_ptr2); 
  fclose(dict_ptr3); 
  fclose(dict_ptr4); 
  fclose(dict_ptr5); 

  return 0 ;

}/*-----------End of Main-------------*/


/********************************************
 * put2file1 : format = F_T :               *
 *                                          *
 * dictionary-format :                      *
 *            (F_T,Pos,Irr-Code,Frequency)  *
 *                                          *
 * human-readable-format :                  *
 *          (Pos,Entry,Frequency,Irr-Code)  *
 *                                          *
 ********************************************/

void Put2File1(infostr,entry)
UCHAR infostr[] ;                    /* 사전의 Format */
UCHAR entry[]   ;                    /* 형태소 : 한글 */ 
{
  char pos[5]   ; 
  char irr_code ; 
  char freq[10] ; 

  strcpy(pos,posTags[infostr[1] - '0']) ; /* 품사 복사 */
  irr_code = infostr[2] ;                 /* 불규칙    */
  strcpy(freq,&infostr[3]) ; 

  if (irr_code == '0') 
       fprintf(dict_ptr1,"%s %s %s\n",pos,entry,freq) ;
  else fprintf(dict_ptr1,"%s %s %s %c\n",pos,entry,freq,irr_code) ;

}/*------------End of put2file1-------------*/


/********************************************
 * put2file2 : format = F_P :               *
 *                                          *
 * dictionary-format :                      *
 *          (F_P,(Morpheme,Tag,(+|*|$))+ )  *
 *                                          *
 * human-readable-format :                  *
 *           (Entry,(Morpheme/Tag,(+|*))+)  *
 *                                          *
 ********************************************/

void Put2File2(infostr,entry)
UCHAR infostr[] ;                            /* 사전의 Format */
UCHAR entry[]   ;                            /* 형태소 : 한글 */ 
{
  PreMor premor[__NUMPREMORANAL__] ; /* 형태소 기분석된 내용 */
  JoinMark joinmark ; 
  char ks[100]  ; 
  char marker   ; /* + , * , \n */
  int idx       ; 
  int numpremor ; /* 형태소 기분석의 형태소 갯수 */

  numpremor = DecodePreMor(premor, &infostr[1],&joinmark) ; 

  fprintf(dict_ptr2,"%c%s%c     ",joinmark.lmark,entry,joinmark.rmark) ; 
  for (idx = 0 ; idx < numpremor ; ++idx) {
    kimmo2ks(premor[idx].word,ks) ; 
    fprintf (dict_ptr2,"%s/%s",ks,posTags[premor[idx].pos-'0']) ; 
	marker = premor[idx].mark ; 
    putc((marker == '$') ? '\n' : marker , dict_ptr2) ; 
  }

}/*------------End of put2file2-------------*/


/********************************************
 * put2file3 : format = F_S :               *
 *                                          *
 * dictionary-format :                      *
 *             (F_S,POS,AKO-list,VKO-list)  *
 *                                          *
 * human-readable-format :                  *
 *           (POS,Entry,AKO-list,VKO-list)  *
 *                                          *
 ********************************************/

void Put2File3(infostr,entry)
UCHAR infostr[] ;                     /* 사전의 Format */
UCHAR entry[]   ;                     /* 형태소 : 한글 */ 
{
  char pos[5]      ; 
  UCHAR akoIdx[8]  ;  
  UCHAR vkoIdx[8]  ;  
  char akolist[40] ; 
  char vkolist[40] ; 
  int numako       ; 
  int numvko       ; 

  strcpy(pos,posTags[infostr[1] - '0']) ; /* 품사 복사 */
 
  GetAVkoIdx(&infostr[2],akoIdx,vkoIdx,&numako,&numvko) ; 
  GetAkoList(akoIdx,numako,akolist) ; 
  GetVkoList(vkoIdx,numvko,vkolist) ; 

  fprintf(dict_ptr3,"(%s) %s (%s) (%s)\n",pos,entry,akolist,vkolist) ;

}/*------------End of put2file3-------------*/


/********************************************
 * put2file4 : format = F_F :               *
 *                                          *
 * dictionary-format :                      *
 *          (F_F,(Morpheme,Tag,(+|*|$))+ )  *
 *                                          *
 * human-readable-format :                  *
 *           (Entry,(Morpheme/Tag,(+|*))+)  *
 *                                          *
 ********************************************/

void Put2File4(infostr,entry)
UCHAR infostr[] ;                            /* 사전의 Format */
UCHAR entry[]   ;                            /* 형태소 : 한글 */ 
{
  PreMor premor[__NUMPREMORANAL__] ; /* 형태소 기분석된 내용 */
  JoinMark joinmark ; 
  char ks[100]  ; 
  char marker   ; /* + , * , \n */
  int idx       ; 
  int numpremor ; /* 형태소 기분석의 형태소 갯수 */

  numpremor = DecodePreMor(premor, &infostr[1],&joinmark) ; 

  fprintf(dict_ptr4,"%c%s%c     ",joinmark.lmark,entry,joinmark.rmark) ; 
  for (idx = 0 ; idx < numpremor ; ++idx) {
    kimmo2ks(premor[idx].word,ks) ; 
    fprintf (dict_ptr4,"%s/%s",ks,posTags[premor[idx].pos-'0']) ; 
	marker = premor[idx].mark ; 
    putc((marker == '$') ? '\n' : marker , dict_ptr4) ; 
  }

}/*------------End of put2file4-------------*/


/********************************************
 * put2file5 : format = F_M :               *
 *                                          *
 * dictionary-format :                      *
 *             (F_M,POS,Marker,Marker)      *
 *                                          *
 * human-readable-format :                  *
 *                                          *
 ********************************************/

void Put2File5(infostr,entry)
UCHAR infostr[] ;                     /* 사전의 Format */
UCHAR entry[]   ;                     /* 형태소 : 한글 */ 
{
  char pos[5]     ; 

  strcpy(pos,posTags[infostr[1] - '0']) ; 

  if (infostr[2] == '0')
       fprintf(dict_ptr5,"%-7s %-14s ()\n",pos,entry) ; 
  else fprintf(dict_ptr5,"%-7s %-14s (%s)\n",pos,entry,&infostr[2]) ; 

}/*------------End of put2file5-------------*/


