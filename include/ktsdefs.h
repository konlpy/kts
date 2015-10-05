/*-------------------------------------------------------------*
 *                                                             *
 * Korean Part-Of-Speech Tagging System ( kts ) Version 0.9    *
 *                                                             *
 * Sang Ho Lee                                                 *
 *                                                             *
 * jhlee@csone.kaist.ac.kr shlee2@adam.kaist.ac.kr             *
 *                                                             *
 * Computer Systems Lab.                                       *
 * Computer Science , KAIST                                    *
 *                                                             *
 * ktsdefs.h ( Korean Tagging System Defines Header File )     *
 *                                                             *
 *-------------------------------------------------------------*/

#ifndef __NEWTYPE__
#define __NEWTYPE__

typedef unsigned char UCHAR ; 
typedef short int     SINT  ; 
typedef short int     FLAG  ; 

#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

#ifndef PUBLIC
#define PUBLIC
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

#ifndef __NUMOFTAG__
#define __NUMOFTAG__ 55  /* contains INI , FIN */
#endif

#ifndef __TAGINITIALS__
#define __TAGINITIALS__
#define __INI__ "INI"
#define __FIN__ "FIN"
#endif

#ifndef __DICTFORMAT__
#define __DICTFORMAT__
#define F_T    'T'      /* Format : T : Tag                                   */
#define F_P    'P'      /* Format : P : Pre-Morph-Analyzed                    */
#define F_S    'S'      /* Format : S : Semantics                             */
#define F_M    'M'      /* Format : M : Marker                                */
#define F_L    'L'      /* Format : L : Lookahead like Trie : not implemented */
#define F_F    'F'      /* Format : F : Preference Morph-Analyzed             */
#endif


#ifndef __SENTAG__
#define __SENTAG__

#define __MAXMORPH__      51  /* 하나의 형태소의 최대 길이 ( 이성진 code )  */
#define __BUFFERLENGTH__ 1024 /* sentence buffer length                     */
#define __SENTLENGTH__   570 /* sentence에 있는 형태소 갯수                */
#define __NUMEOJEOLINSENT__ 60 /* 한 문장안에 있는 어절의 갯수            */
#define __EOJEOLLENGTH__ 100  /* 어절을 이성진 code로 변환할 때  최대 길이 */ 
#define __WSPARE__       100  /* input word의 변화를 위한 여유분           */
#define __SKIPVALUE__      6  /* node와의 간격 : 숫자가 __SKIPVALUE__ 증가 */
#define __NUMIRR__        20  /* 처리해 주는 불규칙 종류의 갯수            */
#define __NUMMORPH__     570  /* morph pool의 크기                         */
#define __NUMWSM__         3  /* 새로운 환경을 만들 가능성                 */
#define __NUMCONNECTION__ 80  /* 형태소가 다음 형태소와 붙을 가능성        */
#define __NUMATTR__       40  /* 형태소의 ambiguity                        */
#define __ATTRSIZE__      40  /* 형태소의 정보로 (Format,Value)의 크기     */
#define __NUMPREMORANAL__ 10  /* 형태소 기분석의 최대 형태소 갯수          */
#define __MAXPATHLEN__    50  /* 어절 하나에 대한 형태소의 최대 열         */  
#define __MAXNUMPATH__   900  /* 어절 하나에 대한 가능한 Path의 갯수       */
#define __MAXPTTRN__     300  /* 미등록어 추정을 위한 최대 tag pattern 수  */
#define __NUMOFOPENCLASS__ 12 /* 미등록어 추정되는 tag 갯수                */
#define __TEMPPATHSIZE__ __MAXNUMPATH__ /* path 처리를 위한 저장 장소      */
#define __TEMPPROB__     900 /* ML Tagging : gammaprob를 sorting하기 위해  */ 
#define __PTTRNTOT__     650 /* lexical pattern의 Max value                */ 

#define _HAN_ 'H'   /* Hanguel              */
#define _ENG_ 'E'   /* English              */
#define _COM_ ','   /* Symbol : ,           */
#define _PER_ '.'   /* Symbol : PERIOD .    */
#define _SEN_ '?'   /* Symbol : ? ! .       */
#define _OPE_ 'O'   /* Symbol : { ( [       */
#define _CLO_ 'C'   /* Symbol : } ) ]       */
#define _NUM_ 'N'   /* Symbol : 0 .. 9      */
#define _SPA_ 'S'   /* Symbol : Space , Tab */
#define _STA_ '$'   /* Symbol : Start       */
#define _EOL_ '#'   /* Symbol : End OF Line */
#define _CON_ '-'   /* Symbol : - --        */
#define _UNI_ '@'   /* Symbol : Unit %      */
#define _CUR_ '!'   /* Symbol : Currency $  */
#define _OTH_ '*'   /* Symbol : Others ...  */

#endif


