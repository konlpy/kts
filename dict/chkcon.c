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
 * chkcon.c ( Korean POS Tagging System : Check POS Connectability ) *
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
#include <string.h>
#include "ktsdefs.h"       /* Definitions                 */
#define __TAGSET__
#include "ktstbls.h"       /* Korean Tag Tables           */

UCHAR tranTable[__NUMOFTAG__][__NUMOFTAG__] ; /* 전이 tbl */
FILE *fPtr                ; /* TRANTBL file pointer */
PUBLIC int TagIdx()       ; 
PUBLIC void LoadTranTbl() ; 
PUBLIC void PutTranTbl()  ;

main(argc,argv)
int argc     ; 
char *argv[] ; 
{
  int lefttag , righttag ; 
  int left1 , left2      ; 
  int right1 , right2    ; 
  int counter = 0        ; 

  if (argc != 5) {
	fprintf(stderr,"Usage : 1. chkcon lookup TRANTBL left-tag right-tag\n") ; 
	fprintf(stderr,"        2. chkcon lookup TRANTBL    all   right-tag\n") ; 
	fprintf(stderr,"        3. chkcon lookup TRANTBL left-tag     all  \n") ; 
	fprintf(stderr,"        4. chkcon lookup TRANTBL    all       all  \n") ; 
        fprintf(stderr,"        5. chkcon set    TRANTBL left-tag right-tag\n") ; 
        fprintf(stderr,"        6. chkcon delete TRANTBL left-tag right-tag\n") ; 
	fprintf(stderr,"\nResult : if connectable : Pos Pos +\n") ;
	fprintf(stderr,"Result : if not         : Pos Pos .\n") ; 
	exit(1) ; 
  }

  LoadTranTbl(argv[2]) ; 

  lefttag = TagIdx(argv[3]) ; 
  righttag = TagIdx(argv[4]) ; 

#ifdef DEBUG
  printf("leftpos  : %d : left-symbol  : %s\n",lefttag,argv[3]) ; 
  printf("rightpos : %d : right-symbol : %s\n",righttag,argv[4]) ; 
#endif 

  if (!strcmp("lookup",argv[1])) {
    left1  = (lefttag  == __NUMOFTAG__) ?       0         : lefttag  ; 
    left2  = (lefttag  == __NUMOFTAG__) ? __NUMOFTAG__ -1 : lefttag  ; 
    right1 = (righttag == __NUMOFTAG__) ?       0         : righttag ; 
    right2 = (righttag == __NUMOFTAG__) ? __NUMOFTAG__ -1 : righttag ; 

    printf("Morpheme Connnectability Matrix\n\n") ; 

    for(lefttag = left1 ; lefttag <= left2 ; ++lefttag)
  	for(righttag = right1 ; righttag <= right2 ; ++righttag) {
        printf ("%3s %3s : %c   ",posTags[lefttag],posTags[righttag],
			  (tranTable[lefttag][righttag] == '1') ? '+' : '.') ; 
        if (!(++counter % 3)) putchar('\n') ; 
  	}
    putchar('\n') ; 

  } else if (!strcmp("set",argv[1])) {

#ifdef DEBUG
  printf("leftpos  : %d : left-symbol  : %s\n",lefttag,argv[3]) ; 
  printf("rightpos : %d : right-symbol : %s\n",righttag,argv[4]) ; 
#endif 

    tranTable[lefttag][righttag] = (UCHAR) '1' ; 
    PutTranTbl() ; 

  } else if (!strcmp("delete",argv[1])) {

#ifdef DEBUG
  printf("leftpos  : %d : left-symbol  : %s\n",lefttag,argv[3]) ; 
  printf("rightpos : %d : right-symbol : %s\n",righttag,argv[4]) ; 
#endif 

    tranTable[lefttag][righttag] = (UCHAR) '0' ; 
    PutTranTbl() ; 

  } else {
    fprintf(stderr,"Error : %s is not an operator\n",argv[1]) ; 
    fclose(fPtr) ; 
    exit(1) ; 
  }

  fclose(fPtr) ; 

}/*--------End of Main--------*/


/*----------------------------------------------------------------*
 *                                                                *
 * LoadTranTbl  : 형태소의 Connectibility Matrix를 구성한다.      *
 *                                                                *
 * Input   : fname  : Connectibility Matrix File Name             *
 * Output  : NULL                                                 *
 * Fuction : tranTable[][]에 Connectibility Matrix 구성           *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void LoadTranTbl(fname)
char *fname ; 
{
  char in[60] ; 
  int i ; 

  if ((fPtr = fopen(fname,"r+")) == NULL) {
	fprintf(stderr,"Error : %s not Found\n",fname) ; 
	return ; 
  }
  for (i = 0 ; i < __NUMOFTAG__ ; ++i) {
	fgets(in,59,fPtr) ; 
    memcpy(tranTable[i],in,__NUMOFTAG__) ; 
  }

}/*---------End of LoadTranTbl-----------*/


/*----------------------------------------------------------------*
 *                                                                *
 * PutTranTbl  : 형태소의 Connectibility Matrix를 File에 쓴다     *
 *                                                                *
 * Input   :                                                      *
 * Output  : NULL                                                 *
 * Fuction : tranTable[][]에 Connectibility Matrix Writing        *
 *                                                                *
 *----------------------------------------------------------------*/

PUBLIC void PutTranTbl()
{
  int idx,idx2 ; 

  rewind(fPtr) ; /* rewind */

  for(idx= 0 ; idx < __NUMOFTAG__ ; ++idx) {
   for(idx2= 0 ; idx2 < __NUMOFTAG__ ; ++idx2)
     fputc(tranTable[idx][idx2],fPtr) ; 
   fputc('\n',fPtr) ; 
  }

}/*---------End of PutTranTbl-----------*/


/*---------------------------------*
 *                                 *
 *  TagIdx : return Index of Tag   *
 *                                 *
 *---------------------------------*/

PUBLIC int TagIdx(in)
char in[] ;
{
  int i ;
  for (i = 0 ; i < __NUMOFTAG__ ; ++i)
        if (!strcmp(in,posTags[i])) return i ;
  return __NUMOFTAG__ ;                     /* Tag에 없으면 */

}/*----end of TagIdx---*/


