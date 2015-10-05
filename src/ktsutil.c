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
 * ktsutil.c ( Korean POS Tagging System : Utility )                *
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
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "ktsdefs.h"
#include "ktsds.h"
#include "ktstbls.h"
#include "sem.h"
  
PUBLIC DBM* DictOpen()      ; /* Dictionary Open                     */ 
PUBLIC void DictClose()     ; /* Dictionary Close                    */
PUBLIC int numSpace()       ; /* String�� space ������ ����          */
PUBLIC void ReverseString() ; /* String�� Reverse�Ѵ�                */
PUBLIC void Myitoa()        ; /* Integer To Ascii                    */
PUBLIC UCHAR TagIdx()       ; /* �Է� Tag�� ���� Index�� return�Ѵ�  */
PUBLIC UCHAR AkoIdx()       ; /* �Է� AKO�� ���� Index�� return�Ѵ�  */
PUBLIC UCHAR VkoIdx()       ; /* �Է� VKO�� ���� Index�� return�Ѵ�  */
PUBLIC int SplitStr()       ; /* String�� space(delimiter)�� ������  */
PUBLIC int ModifyInfo()     ; /* Modify Info String                  */
PUBLIC void MergeInfo()     ; /* Info�� Merge�Ͽ� �ϳ��� String����  */
PUBLIC void Tran2Info()     ; /* String�� Infor Format���� Enconding */
PUBLIC void AppendNode()    ; /* path-set�� path�� sorting�Ͽ� ����  */
PUBLIC void RemoveTailNode(); /* path-set���� ������ node�� ����     */ 
PUBLIC PathRep GetOneNode() ; /* path-set���� path�� ��´�          */
PUBLIC void LoadTranFreq()  ; /* Load Transition Frequencies         */ 
PUBLIC void LoadTagFreq()   ; /* Load Tag Frequencies                */
PUBLIC char CharTag()       ; /* Return Meta-Tag                     */
PUBLIC void TagAdjust()     ; /* Adjust Meta-Tag in Sentence         */
PUBLIC void MakeSetEmpty()  ; /* Make Open-Node,Result Set Empty     */
PUBLIC UCHAR GetPos()       ; /* Get Part-Of-Speech                  */
PUBLIC void GetAVkoIdx()    ; /* Get AKO,VKO Index of AVKO-List      */
PUBLIC void GetAkoList()    ; /* Get Human-Readable AKO-List         */
PUBLIC void GetVkoList()    ; /* Get Human-Readable VKO-List         */

EXTERN ks2kimmo()           ; /* �ѱ� �ϼ����� kimmo code�� �ٲ۴�   */ 
EXTERN void TagAbstract()   ; /* Tag-Sequence�� Abstract Tag�� �ٲ۴�*/

PUBLIC UCHAR _FINAL_ ; /* ����,������ ó���� ���Ѵ�   */
PUBLIC UCHAR _INITI_ ; /* ����,������ �������� ���Ѵ� */
PUBLIC UCHAR _NCT_   ; /* �ѱ� Tag Set�� ���� Tag     */

PUBLIC UCHAR _F_   ; /* Foreign word             : f   */
PUBLIC UCHAR _SCO_ ; /* Symbol : Sentence Comma  : s,  */
PUBLIC UCHAR _SCL_ ; /* Symbol : Sentence Closer : s.  */
PUBLIC UCHAR _SLQ_ ; /* Symbol : Left Quotation  : s`  */
PUBLIC UCHAR _SRQ_ ; /* Symbol : Right Quotation : s'  */
PUBLIC UCHAR _SCN_ ; /* Symbol : Connection Mark : s-  */
PUBLIC UCHAR _SNN_ ; /* Number                   : nnn */
PUBLIC UCHAR _SSU_ ; /* Symbol : Unit            : su  */
PUBLIC UCHAR _SSW_ ; /* Symbol : Currency        : sw  */
PUBLIC UCHAR _SSY_ ; /* Symbol : Other Symbols   : sy  */

double tranprob[__NUMOFTAG__][__NUMOFTAG__] ; /* ���¼� ���� Ȯ�� */

int tagfreq[__NUMOFTAG__]         ; /* ���¼� �߻� Frequency */
double tagprob[__NUMOFTAG__]      ; /* P(���¼� tag)         */

UCHAR buffer[__BUFFERLENGTH__]    ; 
char buffer_tag[__BUFFERLENGTH__] ; 
int bufPtr         ; /* �Է� ���� Buffer�� pointer            */


/*---------------------------------*
 *                                 *
 *  DictOpen : Dictionary Open     *
 *                                 *
 *---------------------------------*/

PUBLIC DBM* DictOpen(fname)
char fname[] ;
{
  DBM* dict ;
  dict = dbm_open(fname,O_RDONLY,0644) ;   /* ���� open */
  if (!dict) {
    fprintf(stderr,"Dictionary not found - set 'KDICT_PATH' properly\n") ; 
    exit(1);
  }
  if (dbm_error(dict)) {
    perror("Error in Dictionary Opening : ") ;
    dbm_clearerr(dict) ;
  }
  return dict ;
}/*--------End of DictOpen------*/


/*---------------------------------*
 *                                 *
 *  DictClose : Dictionary Close   *
 *                                 *
 *---------------------------------*/

PUBLIC void DictClose(dict)
DBM* dict ;
{
  dbm_close(dict) ;
  if (dbm_error(dict)) {
    perror("Error in Dictionay Closing : ") ;
    dbm_clearerr(dict) ;
  }
}/*------End of DictClose-------------*/

/*--------------------------------------*
 *                                      *
 * NumSpace : Count the Space in String *
 *                                      *
 *--------------------------------------*/

PUBLIC int NumSpace(in)
char in[] ; 
{
  int i ; 
  int re = 0 ; 
  for (i = 0 ; in[i] != '\0' && in[i] != '\n' ; ++i)
	if (in[i] == ' ') ++re ; 
  return re ; 
}/*---------------End of numSpace-----------------*/

/*---------------------------------------------*
 *                                             *
 * ReverseString : string entry�� reverse�Ѵ�. *
 *                                             *
 * Input    : entry , len                      *
 * Output   : entry                            *
 * Function : ex : abc\0 --> cba\0             *
 *                                             *
 *---------------------------------------------*/

PUBLIC void ReverseString(entry,len)
UCHAR entry[] ;
int len ;
{
  int i ;
  UCHAR temp[__MAXMORPH__] ;    /* Inverse String�� �ִ� ũ�� */
  for (i = 0 ; i < len ; ++i)
    temp[i] = entry[len-1-i] ;
  temp[len] = '\0' ;
  strcpy(entry,temp) ;
}/*------------End of ReverseString----------*/

/*----------------------*
 *                      *
 * Myitoa : Int 2 Ascii *
 *                      *
 *----------------------*/

PUBLIC void Myitoa(out,in)
UCHAR out[] ; 
int in ; 
{
  int counter = 0 ; 

  out[counter++] = in%10 + '0' ;
  in /= 10 ;  
  for( ; in != 0 ; in /= 10) 
    out[counter++] = in%10 + '0' ;
  out[counter] = '\0' ; 

  ReverseString(out,counter) ; 
}/*----------End of Myitoa----------*/

/*------------------------------*
 *                              *
 * TagIdx : return Index of Tag *
 *                              *
 *------------------------------*/

PUBLIC UCHAR TagIdx(in)
char in[] ;
{
  UCHAR i ;
  for (i = 0 ; i < __NUMOFTAG__ ; ++i)
    if (in[0] == posTags[i][0]) {
        if (!strcmp(in,posTags[i])) return i + '0' ;
    }
  return __NUMOFTAG__ + '0' ;  /* Tag�� ������ */
}/*----end of TagIdx---*/

/*------------------------------*
 *                              *
 * AkoIdx : return Index of Ako *
 *                              *
 *------------------------------*/

PUBLIC UCHAR AkoIdx(in)
char in[] ;
{
  UCHAR i ;
  for (i = 0 ; i < __NUMOFAKO__ ; ++i)
    if (in[0] == akoList[i][0]) {
        if (!strcmp(in,akoList[i])) return i + '0' ;
    }
  return __NUMOFAKO__ + '0' ;     /* AKO�� ������ */
}/*----end of AkoIdx---*/


/*------------------------------*
 *                              *
 * VkoIdx : return Index of Vko *
 *                              *
 *------------------------------*/

PUBLIC UCHAR VkoIdx(in)
char in[] ;
{
  UCHAR i ;
  for (i = 0 ; i < __NUMOFVKO__ ; ++i)
    if (in[0] == vkoList[i][0]) {
        if (!strcmp(in,vkoList[i])) return i + '0' ;
    }
  return __NUMOFVKO__ + '0' ;     /* VKO�� ������ */
}/*----end of VkoIdx---*/


/*-------------------------*
 *                         *
 * SplitStr : Split String *
 *                         *
 *-------------------------*/

PUBLIC int SplitStr(to,from)
char to[][__ATTRSIZE__] ; 
char from[] ; 
{
  int toRow , toCol ;
  int fromIdx ; 

  toRow = 0 ; toCol = 0 ;  
  for(fromIdx = 0 ; from[fromIdx] != '\0' ; ++fromIdx)
	if(from[fromIdx] == ' ') {
	   to[toRow++][toCol] = '\0' ; 
	   toCol = 0 ; 
    } else to[toRow][toCol++] = from[fromIdx] ;  

  to[toRow++][toCol] = '\0' ;  

  return toRow ; 
}/*-----------------End of SplitStr-----------------*/


/*---------------------------------*
 *                                 *
 * ModifyInfo : Modify Information *
 *                                 *
 *---------------------------------*/

PUBLIC int ModifyInfo(info,numInfo,attr,compInfo) 
UCHAR info[][__ATTRSIZE__] ;                     /* (Attr,Value) Array  */
int numInfo  ;                                   /* (Attr,Value) ����   */
UCHAR attr   ;                                   /* attr = ( F_T , F_P) */
UCHAR compInfo[]  ;                              /* Freq(Word,Tag)      */
{
  int idx ; 

  int freq1 ; /* info[][__ATTRSIZE__]�� �ִ� Frequency */
  int freq2 ; /* compInfo[]�� �ִ� Frequency           */
  
  for(idx = 0 ; idx < numInfo ; ++idx) {
    switch(attr) {
      case F_T : 
		if (info[idx][0] == F_T && compInfo[0] == info[idx][1] &&
                    compInfo[1] == info[idx][2]) {
		 freq1 = atoi(&info[idx][3]) ; /* ������ �ִ� Freq   */ 
                 freq2 = atoi(&compInfo[2])  ; /* ���� ��     Freq   */ 
                 Myitoa(&info[idx][3],freq1+freq2) ; /* Int to Ascii */
		 return numInfo ;         /* numInfo�� ������ �ʾҴ� */ 
                }
		break ; 
     case F_P : 
                if (info[idx][0] == F_P && 
			!strcmp(&info[idx][1],compInfo)) return -1 ; /* �̹� �ִ� */ 
		break ;                  
     case F_S :
                if (info[idx][0] == F_S &&
                        !strcmp(&info[idx][1],compInfo)) return -1 ; /* �̹� �ִ� */
                break ; 
     case F_M :
                if (info[idx][0] == F_M &&
                        !strcmp(&info[idx][1],compInfo)) return -1 ; /* �̹� �ִ� */
                break ;
     case F_F :
                if (info[idx][0] == F_F &&
                        !strcmp(&info[idx][1],compInfo)) return -1 ; /* �̹� �ִ� */
                break ;
     case F_L : return numInfo ; 
		break ;                          /* Not Implemented */
    } /*-end of switch-*/
  }

  /* info[][]�� ���� �ʾƼ� append�Ѵ�. */

  switch(attr) {
   case F_T : info[numInfo][0] = F_T ;        /* Attr = F_T ( Tag ) */
              strcpy(&info[numInfo][1],compInfo) ; 
              break ; 
   case F_P : info[numInfo][0] = F_P ;        /* Attr = F_P ( Pre ) */ 
              strcpy(&info[numInfo][1],compInfo) ; 
              break ; 
   case F_S : info[numInfo][0] = F_S ;        /* Attr = F_S ( Sem ) */ 
              strcpy(&info[numInfo][1],compInfo) ; 
              break ; 
   case F_M : info[numInfo][0] = F_M ;        /* Attr = F_M ( Marker )     */
              strcpy(&info[numInfo][1],compInfo) ;
              break ;
   case F_F : info[numInfo][0] = F_F ;        /* Attr = F_F ( PreFerence ) */
              strcpy(&info[numInfo][1],compInfo) ;
              break ;
   case F_L : break ; 
  } /*-end of switch-*/

  return ++numInfo ; 

}/*----end of ModifyInfo-----*/

/*-----------------------------------------------*
 *                                               *
 * MergeInfo : Merge Information into One String *
 *                                               *
 *-----------------------------------------------*/

PUBLIC void MergeInfo(str,info,numInfo)
char str[]                 ;                     /* Target String   */ 
UCHAR info[][__ATTRSIZE__] ;                     /* information     */
int numInfo                ;                     /* number of Infor */
{
  int infoRow , infoCol ; 
  int strIdx = 0 ; 
  
  for(infoRow = 0 ; infoRow < numInfo ; ++infoRow) {
    for(infoCol = 0 ; (str[strIdx] = info[infoRow][infoCol]) != '\0' ; 
				  ++infoCol , ++strIdx) ; 
    str[strIdx++] = ' ' ; /* '\0'�� ' '�� �ٲ۴� */
  }
  str[--strIdx] = '\0' ; 

}/*--------End of MergeInfo-------*/


/*--------------------------------------------------------*
 *                                                        *
 * MrgnFreq : Get Marginal Frequency                      *
 *                                                        *
 *            Freq(Word) = Sum Freq(Word,Tag) for All Tag *
 *                                                        *
 *--------------------------------------------------------*/

PUBLIC int MrgnFreq(info,numInfo)
UCHAR info[][__ATTRSIZE__] ; 
int numInfo ; 
{
  int idx ; 
  int mfreq = 0 ; 

  for(idx = 0 ; idx < numInfo ; ++idx)
	if (info[idx][0] == F_T) mfreq += atoi(&info[idx][3]) ; 

  return mfreq ; 
}/*-----End of MrgnFreq-----*/


/*----------------------------------------------------------*
 *                                                          *
 * Tran2Info : Transform to Information-Format              *
 *                                                          *
 * Annotate : Marker : '*' : ���ο� ���� ����               *
 *                     '+' : ���� �� ���ο� ���¼� ����     *
 *                     '$' : ���¼� ��м��� �������� �ǹ�  *
 *                                                          *
 *          joinmark : Pre-Morph�� �¿� context ����        *
 *                                                          *
 *                                                          *
 *  ��) $��$     : ������ ó��($)���� ������($)   ��ȯ ���� *
 *      +��$     : ������ �߰�(+)���� ������($)   ��ȯ ���� *
 *      $�ؾ߰�+ : ������ ó��($)���� �߰�����(+) ��ȯ ���� *
 *      +��߰�+ : ������ �߰�(+)���� �߰�����(+) ��ȯ ���� *
 *                                                          *
 *----------------------------------------------------------*/

PUBLIC void Tran2Info(compInfo,entry,joinmark) 
UCHAR compInfo[]     ; /* ���� ���·� �ٲ۴�           */
char entry[]         ; /* �Է� string                  */
JoinMark joinmark    ; /* �¿� context                 */
{
  UCHAR temp[10][20] ; /* entry�� ���¼� ������ ������ */
  UCHAR buf[15]      ; /* ���¼Ҹ� ��� ����           */
  UCHAR pos          ; /* ǰ�� : POS                   */
  UCHAR marker       ; /* Marker = ( '*' | '+' | '$' ) */
  int entryPtr ; 
  int compPtr = 2 ; 
  int strl ; 
  int tempRow , tempCol ;
  int tempIdx , tempCIdx1 , tempCIdx2 ; 

  tempRow = tempCol = 0 ; 
  
  compInfo[0] = joinmark.lmark ; 
  compInfo[1] = joinmark.rmark ; 

  for(entryPtr=0 ; entry[entryPtr] != '\0' ; ++entryPtr) {
    temp[tempRow][tempCol++] = entry[entryPtr] ; 
    if (entry[entryPtr] == '*' || entry[entryPtr] == '+') {
      temp[tempRow++][tempCol] = '\0' ; 
      tempCol = 0 ; 
    }
  }
  temp[tempRow][tempCol++] = '$'  ; 
  temp[tempRow++][tempCol] = '\0' ; 

  /* ���¼� ������ temp[][]�� �� �ִ� */

  tempCIdx2 = 0 ; 
  for(tempIdx = 0 ; tempIdx < tempRow ; ++tempIdx) {
    for(tempCIdx1 = tempCIdx2 ; temp[tempIdx][tempCIdx2] != '/' ;
							++tempCIdx2) ; 
    temp[tempIdx][tempCIdx2++] = '\0' ;               /* '/' --> '\0' */ 
    (void) ks2kimmo(&temp[tempIdx][tempCIdx1],buf)  ; /*  Kimmo Code  */
    for(tempCIdx1 = tempCIdx2 ; temp[tempIdx][tempCIdx2] != '\0' ; 
							++tempCIdx2) ; 
    marker = temp[tempIdx][tempCIdx2-1] ;
    temp[tempIdx][tempCIdx2-1] = '\0' ; 
    tempCIdx2 = 0 ; 
    pos = TagIdx(&temp[tempIdx][tempCIdx1]) ;  
	strl = strlen(buf) ;  
    buf[strl++] = pos ; buf[strl++] = marker ; buf[strl] = '\0' ; 
    strcpy(&compInfo[compPtr],buf) ; /* ��� buf�� Coding�Ѵ� (Append) */
    compPtr += strl ; 
  }

}/*---End of Tran2Info---*/


/*---------------------------------------------------------*
 *                                                         *
 * DecodePreMor : ���¼� ��м� ������ PreMor Format����   *
 *	          Decode�Ѵ�.                              *
 *                                                         *
 * Annotate : Marker : '*' : ���ο� ���� ����              *
 *                     '+' : ���� �� ���ο� ���¼� ����    *
 *                     '$' : ���¼� ��м��� �������� �ǹ� *
 *                                                         *
 *            Format : { (POS , Marker , Word) }*          *
 *                                                         *
 *---------------------------------------------------------*/

PUBLIC int DecodePreMor(premor,str,joinmark)
PreMor premor[]    ; /* ���¼� ��м� format : decoded */
char str[]         ; /* �Է� string          : encoded */
JoinMark *joinmark ; 
{
  int stridx ;                      /* �Է� string�� index */
  int numpremor = 0 ; 
  int premoridx     ; 
  JoinMark tmpmark  ; 
 
  tmpmark.lmark = (UCHAR) str[0] ; 
  tmpmark.rmark = (UCHAR) str[1] ; 

  *joinmark = tmpmark ; 

  premoridx = 0 ;
  for(stridx = 2 ; str[stridx] != '\0' ; ++stridx) {
    if (str[stridx] == '+' || str[stridx] == '*' ||
		str[stridx] == '$') {
      premor[numpremor].mark = str[stridx]  ; 
      premor[numpremor].pos = str[stridx-1] ; 
      premor[numpremor++].word[premoridx-1] = '\0' ; 
      premoridx = 0 ; 
    } else premor[numpremor].word[premoridx++] = str[stridx] ; 
  }

  return numpremor ; 

}/*---End of DecodePreMor---*/


/*-----------------------------------------------------*
 *                                                     *
 * AppendNode  : Append Node into parameter-Set        *
 *                                                     *
 * Input       : &pathset , one_path                   *
 * Output      : NULL                                  *
 * Function    : pathset�� sorting�� ������ one_path�� *
 *               �����Ѵ�                              *
 *                                                     *
 *-----------------------------------------------------*/

PUBLIC void AppendNode(pathset,onepath)  
PathRep *pathset ;               /* &path-set          */
PathRep onepath  ;               /* path-set�� element */
{
  PathRep *elePtr   ; /* element Pointer */
  PathRep *scanner  ; /* sorting ������ �ֱ� ���� scanner �ʿ� */

  elePtr = (PathRep *) malloc (sizeof(PathRep)) ; 

  if (elePtr == NULL) { 
    printf("Error in Memory Allocation : ktsutil.c::AppendNode\n") ; 
    exit(1) ; 
  }

  *elePtr = onepath ; 
  elePtr->nextPtr = NULL ; 
  scanner = pathset ; 

  while(1) {
    if (scanner->nextPtr == NULL) {
	 scanner->nextPtr = elePtr ; 
	 elePtr->nextPtr = NULL ; 
	 break ; 
    }
    if (scanner->nextPtr->fprob <= elePtr->fprob) {
	 elePtr->nextPtr = scanner->nextPtr ; 
	 scanner->nextPtr = elePtr ; 
	 break ; 
    }
   scanner = scanner->nextPtr ; 
  }

}/*------------End Of AppendNode------------*/


/*-----------------------------------------------------*
 *                                                     *
 * RemoveTailNode : Node List���� ������ node�� ����   *
 *                                                     *
 * Input       : &pathset                              *
 * Output      : NULL                                  *
 *                                                     *
 *-----------------------------------------------------*/

PUBLIC void RemoveTailNode(pathset)  
PathRep *pathset ;    /* &path-set                             */
{
  PathRep *scanner  ; /* sorting ������ �ֱ� ���� scanner �ʿ� */
  int cntr = 0 ; 

  scanner = pathset ; 

  while(scanner->nextPtr->nextPtr != NULL) {
    ++cntr ; 
    scanner = scanner->nextPtr ; 
  } 

  if (cntr > 0) { /* �ּ��� best-path�� ���� �־�� �ȴ�. */
    free(scanner->nextPtr) ; 
    scanner->nextPtr = NULL ; 
  }

}/*------------End Of RemoveTailNode------------*/


/*-----------------------------------------------------*
 *                                                     *
 * GetOneNode  : Get One Node From parameter-Set       *
 *                                                     *
 * Input       : &pathset                              *
 * Output      : one_path                              *
 * Function    : pathset�� prob�� max�� one_path��     *
 *               return�Ѵ�                            *
 *                                                     *
 *-----------------------------------------------------*/

PUBLIC PathRep GetOneNode(pathset)
PathRep *pathset ;                       /* &path-set  */
{
  PathRep onepath ; 
  PathRep *tmppath ; 

  if (pathset->nextPtr == NULL) {
     printf("There's no nodes in open node set : ktsutil.c::GetOneNode()\n");
     exit(1);
  }
  onepath = *(pathset->nextPtr) ;
  tmppath = pathset->nextPtr ; 

  pathset->nextPtr = pathset->nextPtr->nextPtr ; 
  free(tmppath) ; 
  return onepath ; 

}/*-----------End of GetOneNode-----------*/

/*-----------------------------------------------------*
 *                                                     *
 * MakeSetEmpty : parameter-Set�� ������ ��� ���ش�   *
 *                                                     *
 * Input       : &pathset                              *
 * Output      : NULL                                  *
 * Function    : pathset�� Linked-List�� ���󰡸�      *
 *               Node�� ���ش�                         *
 *                                                     *
 *-----------------------------------------------------*/

PUBLIC void MakeSetEmpty(pathset)
PathRep *pathset ;                        /* &path-set */
{
  PathRep *scanner = pathset->nextPtr ; 
  PathRep *tmp ; 

  while (scanner != NULL) {
	tmp = scanner ; 
	scanner = scanner->nextPtr ; 
	free(tmp) ; 
  } 
  pathset->nextPtr = NULL ; /* header-node�� nextPtr = NULL */

}/*--------End of MakeSetEmpty--------*/

/*------------------------------------------------------------------*
 *                                                                  *
 * LoadTranFreq : Loading Transition-Frequency                      *
 *                                                                  *
 * Input    : transition-Frequency�� ���� �ִ� file name            *
 * Output   : NULL                                                  *
 * Function : Freq(tag_i , tag_(i-1))�� loading                     *
 *            Prob(tag_i | tag_(i-1)) =                             *
 *                                                                  *
 *	     	               Freq(tag_(i-1) , tag_i)              *
 *	    	            ----------------------------            *
 *                          ---                                     *
 *                          \     Freq(tag_(i-1),tag_j)             *
 *                          /                                       *
 *                          ---                                     *
 *                           j                                      *
 *                                                                  *
 *                                                                  *
 *  * ���ο� �ܾ ÷���� ��� : Simplest Model                    *
 *                                                                  *
 *       Freq(Word,Tag)�� ������ ÷���ϰ� �ȴ�.                     *
 *	 �̶� Prob(Word|Tag)�� Freq(Word,Tag) = 1 / Freq(Tag)�̴�.  *
 *	 �� Freq(Tag)�� �����ȴ�.                                   *
 *	 Freq(Tag_i | Tag_i-1)���� ������ ��ġ�� �ʾƼ�             *
 *	 Marginal Frequency(Tag)�� ���� �����ؾ� �ȴ�.              *
 *                                                                  *
 *------------------------------------------------------------------*/

PUBLIC void LoadTranFreq(fname,tran)
char *fname                 ; 
double tran[][__NUMOFTAG__] ; /* ���¼� ���� Ȯ�� */

{
  FILE *fptr ; 

  char bef_tag[10] ; /* tag_(i-1)                           */ 
  char aft_tag[10] ; /* tag_(i)                             */
  double sum       ; /* Sum of tag(i,j) for all j           */
  int freq         ; 
  int bef_tagIdx   ;
  int aft_tagIdx   ; 

  if ((fptr = fopen(fname,"r")) == NULL) {
    fprintf(stderr,"Error : %s not Found\n",fname) ; 
    return ; 
  }

  while (fscanf(fptr,"%s%s%d",bef_tag,aft_tag,&freq) != EOF) {
   bef_tagIdx = (int) TagIdx(bef_tag) - '0' ;  
   aft_tagIdx = (int) TagIdx(aft_tag) - '0' ; 
   tran[bef_tagIdx][aft_tagIdx] = (double) freq ; 
  }

  fclose(fptr) ; 

  /* 
   *
   * __NUMOFTAG__ - 2 = 53 , tag�� xa������ ����Ѵ�.
   * __NUMOFTAG__ - 1 = TagIdx("FIN") - '0' ; 
   * FIN , int�� ��꿡�� ���ܵȴ�.
   * Freq(FIN|any tag) = Constant = 1.0���� ����(���Ǹ� ����) 
   *                     A* algorithm���� Backtrack�� �� �� 
   *                     f = g + h ���� g = 1.0���� ����Ѵ�.
   * Freq(int|any tag) = 0
   *
   * ��� Freq(t_i,t_j)�� ���Ͽ� ++Freq(t_i,t_j)�� �Ѵ�. 
   *
   */

#ifdef DEBUG2
  { int i , j ; 

    j = (int) _INITI_ - '0' ;  
    for(i = 0 ; i < __NUMOFTAG__ - 2 ; ++i)
      printf("%d ",(int) tran[j][i]) ; 
    putchar('\n') ; 
  }
#endif

  for(bef_tagIdx = 0 ; bef_tagIdx < __NUMOFTAG__ - 2 ; ++bef_tagIdx) {
      sum = 0.0 ; 
    for(aft_tagIdx = 0 ; aft_tagIdx < __NUMOFTAG__ - 2 ; ++aft_tagIdx) {
      tran[bef_tagIdx][aft_tagIdx] += (double) 1.0 ; 
      sum += tran[bef_tagIdx][aft_tagIdx] ; 
    }
    for(aft_tagIdx = 0 ; aft_tagIdx < __NUMOFTAG__ - 2 ; ++aft_tagIdx)
      tran[bef_tagIdx][aft_tagIdx] /= sum ; 
    tran[bef_tagIdx][aft_tagIdx] = 1.0 ; /* P(FIN|any_tag) = 1.0 */
  }

#ifdef DEBUG2
  { int i , j ; 
	printf ("Load Probability(next_tag|before_tag)\n") ; 
	printf ("Ex : P(tag|INI) as followings...\n") ; 
    
	j = (int) _INITI_ - '0' ;  

	for(i = 0 ; i < __NUMOFTAG__ - 2 ; ++i)
	  printf("%f ",tran[j][i]) ;  
    putchar('\n') ; 
  }
#endif

}/*----------------End of LoadTranFreq-----------------*/


/*---------------------------------------------------*
 *                                                   *
 * LoadTagFreq : Loading Tag Frequency               *
 *                                                   *
 * Input       : NULL                                *
 * Output      : NULL                                *
 * Function    : tagfreq[]�� �ִ´�                  *
 *                                                   *
 *---------------------------------------------------*/

PUBLIC void LoadTagFreq(dict_dbm)
DBM *dict_dbm      ; 
{
  datum inputData  ; /*  key�� �ش��ϴ� �� : �ܾ� */
  datum isInData   ; /*  �̹� �ִ��� �����ϱ� ���� datum */
  
  char tagname[10] ; /* search tag : @INI , @a */
  int idx ; 
  double total_prob = 0.0 ; 

  tagname[0] = '@' ; 
  for(idx = 0 ; idx < __NUMOFTAG__ ; ++idx) {
   strcpy(&tagname[1],posTags[idx]) ;    
   inputData.dptr = tagname ; 
   inputData.dsize = strlen(tagname) + 1 ; 
   isInData = dbm_fetch(dict_dbm,inputData) ; 

   tagfreq[idx] = atoi(isInData.dptr) ; 
   tagprob[idx] = (double) tagfreq[idx] ; 
   total_prob += tagprob[idx] ; 
  }

  /* Prob(tag) */

  for(idx = 0 ; idx < __NUMOFTAG__ ; ++idx)
    tagprob[idx] /= total_prob ; 

}/*--End of LoadTagFreq--*/


/*------------------------------------------*
 *                                          *
 * CharTag  : Tag of character              *
 *                                          *
 * Input    : in                            *
 * Output   : it's tag                      *
 * Function : character�� category�� return *
 *                                          *
 *------------------------------------------*/

PUBLIC char CharTag(in)
char in ; 
{
  if (in & 0x80)        return _HAN_ ; /* �̰� �߸� �� ���� �ִ�. */
  else if (in == '\n')  return _EOL_ ; 
  else if (isdigit(in)) return _NUM_ ; 
  else if (isalpha(in)) return _ENG_ ; 
  else if (isspace(in)) return _SPA_ ; 
  else if (strchr(",:/",in)) return _COM_ ; 
  else if (strchr("!?",in)) return _SEN_ ; 
  else if (in == '.') return _PER_ ; 
  else if (strchr("`({[",in)) return _OPE_ ; 
  else if (strchr("'\")}]",in)) return _CLO_ ; 
  else if (strchr("-~",in)) return _CON_ ; 
  else if (in == '%') return _UNI_ ; 
  else if (in == '$') return _CUR_ ; 
  else return _OTH_ ; 
}/*----------------End of CharTag-----------------*/

/*--------------------------------------------------*
 *                                                  *
 * TagAdjust : Tag Adjust                           *
 *                                                  *
 * Input     : NULL                                 *
 * Output    : NULL                                 *
 * Function  : if (number)*,(number)* ==> ',' _NUM_ *
 *             if (number)*.(number)* ==> '.' _NUM_ *
 *             if (.)*                ==> '.' _OTH_ *
 *                                                  *
 *--------------------------------------------------*/

PUBLIC void TagAdjust()
{
  int i ; 

  for (i = 1 ; i < bufPtr ; ++i) 
   switch(buffer_tag[i]) {
    case _PER_ : if (buffer_tag[i-1] == _NUM_ &&
			 buffer_tag[i+1] == _NUM_) {
		   buffer_tag[i] = _NUM_ ; 
		   break ; 
                 }
		 if (buffer_tag[i-1] == _OTH_) {
		   buffer_tag[i] = _OTH_ ; 
		   break ; 
                 }
                 if (buffer_tag[i-1] == _PER_) 
		   buffer_tag[i-1]=buffer_tag[i]= _OTH_ ; 
		 break ; 
    case _COM_ : if (buffer_tag[i-1] == _NUM_ &&
	                 buffer_tag[i+1] == _NUM_)
	           buffer_tag[i] = _NUM_ ; 
                 break ; 
   }
}/*-------------End of TagAdjust-------------*/

/*-----------------------------*
 *                             *
 * GetPos : Get Part-Of-Speech *
 *                             *
 *-----------------------------*/

PUBLIC UCHAR GetPos(ch)
char ch ; 
{

  switch(ch) {
   case _ENG_ : return _F_   ; 
   case _COM_ : return _SCO_ ; 
   case _SEN_ :
   case _PER_ : return _SCL_ ; 
   case _OPE_ : return _SLQ_ ; 
   case _CLO_ : return _SRQ_ ; 
   case _NUM_ : return _SNN_ ; 
   case _CON_ : return _SCN_ ; 
   case _UNI_ : return _SSU_ ; 
   case _CUR_ : return _SSW_ ; 
   case _OTH_ : return _SSY_ ; 
   case _STA_ : return _INITI_ ; 
  }

 return _FINAL_ ; /* Error */

}/*-----------End of GetPos-----------*/


/*--------------------------------*
 *                                *
 * GetAVkoIdx : Get AKO,VKO Index *
 *                                *
 *--------------------------------*/

PUBLIC void GetAVkoIdx(str,akoIdx,vkoIdx,numako,numvko)
UCHAR str[]    ; 
UCHAR akoIdx[] ; 
UCHAR vkoIdx[] ; 
int *numako    ; 
int *numvko    ; 
{
  int idx      ; 
  int idx2 = 0 ; 

  for(idx = 0 ; str[idx] != '&' ; ++idx)
    akoIdx[idx] = str[idx] ; 
  akoIdx[idx] = '\0' ; 
  *numako = idx ; 

  for(++idx ; str[idx] != '\0' ; ++idx)
    vkoIdx[idx2++] = str[idx] ; 
  vkoIdx[idx2] = '\0' ; 
  *numvko = idx2 ; 

}/*---------End of GetAVkoIdx----------*/


/*------------------------------------------*
 *                                          *
 * GetAkoList : Get Human-Readable AKO-List *
 *                                          *
 *------------------------------------------*/

PUBLIC void GetAkoList(akoIdx,numako,akolist)
UCHAR akoIdx[] ; 
int numako     ; 
char akolist[] ; 
{
  int idx ; 
  akolist[0] = '\0' ; 
  if (numako == 1 && akoIdx[0] == '0') return ; 

  for(idx = 0 ; idx < numako ; ++idx) {
    strcat(akolist,akoList[akoIdx[idx] - '0']) ; 
    if (idx < numako - 1) strcat(akolist,",") ; 
  }
}/*-------End of GetAkoList--------*/


/*------------------------------------------*
 *                                          *
 * GetVkoList : Get Human-Readable VKO-List *
 *                                          *
 *------------------------------------------*/

PUBLIC void GetVkoList(vkoIdx,numvko,vkolist)
UCHAR vkoIdx[] ; 
int numvko     ; 
char vkolist[] ; 
{
  int idx ; 
  vkolist[0] = '\0' ; 
  if (numvko == 1 && vkoIdx[0] == '0') return ; 

  for(idx = 0 ; idx < numvko ; ++idx) {
    strcat(vkolist,vkoList[vkoIdx[idx] - '0']) ; 
    if (idx < numvko - 1) strcat(vkolist,",") ; 
  }
}/*-------End of GetVkoList--------*/



