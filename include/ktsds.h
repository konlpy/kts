/*****************************************************
 * Korean Tagging System : KTS version 0.9           *
 *                                                   *
 * Header File for Data Structure                    *
 *                                                   *
 * Sang Ho Lee                                       *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr *
 *                                                   *
 *****************************************************/

#ifndef __KTSDS__
#define __KTSDS__

/* 하나의 글자는 edge로 표현된다 */

typedef struct __inputW__ {       /* Input Word Pool             */
  short int left                ; /* 글자의 왼쪽 node            */
  short int right               ; /* 글자의 오른쪽 node          */
  unsigned char start           ; /* 이 글자로부터 형태소가 시작 */
  unsigned char end             ; /* 이 글자까지 형태소가 끝     */
  unsigned char lee             ; /* 이 성진 code                */
  unsigned char irr[__NUMIRR__] ; /* 불규칙 가능성               */ 
} InputW ; 

typedef struct __morph__ {    /* Morph Pool */
  short int env             ; /* environment 번호                             */
  char  end_mark            ; /* 본디말 때문에 Space가 있는지에 대한 Marker   */
  short int left            ; /* 형태소의 왼쪽 node                           */
  short int right           ; /* 형태소 하나의 오른쪽 node                    */ 
  short int wright          ; /* 최종 형태소 해석 결과의 마지막 node MAX      */
  short int nidx[__NUMCONNECTION__] ; /* 다음 형태소 가리키는 index:-1:끝     */
  unsigned char word[__MAXMORPH__] ; /* 하나의 형태소 길이는 __MAXMORPH__     */ 
  double prob               ; /* 형태소의 P(word|tag) = C(word,tag)/C(tag)    */
  unsigned char pos         ; /* 형태소의 tag index                           */
  unsigned char irr         ; /* 형태소의 irregular code                      */
} Morph ; 

typedef struct __WSM__ {       /* Word Stack Manager */
  short int begin  ; /* 현재 환경의 형태소 begin node 번호                    */ 
  short int end    ; /* 현재 환경의 형태소 end node 번호                      */
  short int top    ; /* split 혹은 merge가 되어 변화를 한 곳의 top            */ 
  short int bottom ; /* split 혹은 merge가 되어 변화를 한 곳의 bott           */ 
  short int ini    ; /* 현재 환경의 ini node 번호 : -1 : don't care condition */
  short int fin    ; /* 현재 환경의 fin node 번호 : -1 : don't care condition */
  char direct      ; /* 문자를 만드는 방향 : 'L' : 왼쪽 : 'R' : 오른쪽        */
} WSM ;

typedef struct __sent__ {
  char end_mark                    ; /* 한 어절의 끝을 나타내는 Mark      */
  int sent_begin                   ; /* 문장 시작하는 번호                */
  int sent_end                     ; /* 문장 끝나는 번호                  */
  int morph_begin                  ; /* 어절 내에서 시작하는 번호         */
  int morph_end                    ; /* 어절 내에서 끝나는 번호           */
  double prob                      ; /* 형태소의 P(word|tag)              */
  unsigned char word[__MAXMORPH__] ; /* 하나의 형태소 길이 : __MAXMORPH__ */
  unsigned char pos                ; /* 형태소 품사 index                 */
  short int nidx[__NUMCONNECTION__] ; /* 다음 형태소 가리키는 next  index */
} Sentence ; 

typedef struct __m__ { /* morphP[]를 scanning하기 위한 자료구조           */
   short int env     ; /* environment 번호                                */
   short int left    ; /* 형태소의 왼쪽 node                              */
   short int right   ; /* 형태소 하나의 오른쪽 node                       */ 
   short int wright  ; /* 최종 형태소 해석 결과의 마지막 node MAX         */
} Morph_Part ;


/*--------------------------------------------------------*/
/*-------------- Added From Version 0.9 ------------------*/ 
/*--------------------------------------------------------*/
/*-------- Data Structure for Multiple Tagging -----------*/
/*--------------------------------------------------------*/

typedef struct __pre__ {             /* Pre-Morphological Analyzed DS */ 
  unsigned char pos                ; /* 형태소의 품사 index           */
  char mark                        ; /* '+' : 어절안의 separater      */ 
                                     /* '*' : 문장안의 separater      */
                                     /* '$' : 형태소 기 분석의 끝     */
  unsigned char word[__MAXMORPH__] ; /* 하나의 형태소                 */
} PreMor ;

typedef struct __lr__ {  /* Pre-Morph의 좌우 Context 정보를 위해  */ 
  unsigned char lmark ;  /* 예) +ㄴ$ 는/jx : 왼쪽은 + 오른쪽은 $  */ 
  unsigned char rmark ;  /*     + : 어절내     $ : 어절의 좌우 끝 */
} JoinMark ; 

typedef struct __path__ {    /* path를 senP[]의 index열로 나타낸다   */ 
  int pathlen              ; /* 어절의 형태소 path의 길이 in path[]  */ 
  int path[__MAXPATHLEN__] ; /* 어절의 형태소 path를 간직한다        */
  char ox[3]               ; /* 
			      * ox[0] : State-Based의 결과   
			      * ox[1] : State-Based & Path-Based의 결과
			      * ox[2] : Path-Based의 결과
			      */ 

                             /* 
                              * For Viterbi Tagging : 
                              * Path-Based Tagging                   
		              */

  double lexprob           ; /* Prob(어절|Tag Sequence)              */
  double maxprob           ; /* Partial Path Probability for Viterbi */
  int backIdx              ; /* back Pointer for Viterbi             */

                             /* 
			      * For Maximum Likelihood Tagging : 
			      * State-Based Tagging 
			      */

  double alphaprob         ; /* Forward Computation  : like HMM      */
  double betaprob          ; /* Backward Computation : like HMM      */ 
  double gammaprob         ; /* alphaprob * betaprob / Pr(O|lamda)   */ 
} Path ; 

typedef struct __trellis__ {     /* POS Tagging을 위한 자료구조      */ 
  struct __trellis__ *backPtr ;  /* 이전 어절을 가리키는 Pointer     */
  struct __trellis__ *nextPtr ;  /* 다음 어절을 가리키는 Pointer     */
  unsigned char eojeol[__EOJEOLLENGTH__] ; /* 어절을 갖는 char array */
  int startIdx   ;               /* senP[]의 어절 시작 Index         */
  int endIdx     ;               /* senP[]의 어절 마지막 Index       */
  int numPath    ;               /* 하나의 어절에서 생기는 Path 갯수 */
  Path *pathPool ;               /* 어절의 가능한 형태소 Path를 저장 */
                                 /* 
                                  * For Maximum Likelihood Tagging : 
				  * State-Based Tagging 
				  */
  int *sortedIdx ;            /* gammaprob를 sorting 했을 때 indices */
  char known     ;            /* known word 'o' , Unknown word 'x'   */
} Trellis ; 

typedef struct __pathRep__ {  /* Path Representation */
  struct __pathRep__ *nextPtr ; 
  int epathlen              ; /* 몇개의 어절을 저장한 상태인가 ?     */ 
  int epath[__NUMEOJEOLINSENT__] ; /* 문장의 어절 path를 저장한다    */
  Trellis *curTrPtr         ; /* Current Trellis Pointer             */
  double fprob              ; /* f(n) = g(n) + h*(n)                 */
  double gprob              ; /* tree의 처음에서 현재까지의 cost     */
} PathRep ;


/*----------------------------------------------------------------*/
/*---------- Data Structure for Handling Unknown Words -----------*/ 
/*----------------------------------------------------------------*/

typedef struct __pttn__ {        /* pattern : endings         */
  double pttrnProb             ; /* Freq(t_i,w_i+1,t_i+1)     */
  UCHAR  pttrn[__MAXPATHLEN__] ; /* pttrn[0] .. pttrn[n] \0    
				  * Tag_i .. Tag_j \0
				  */
} Pttrn ; 

typedef struct __pttntot__ {     /* pattern과 전체 pttrn이 나온 갯수 */ 
  UCHAR  pttrn[__MAXPATHLEN__] ; 
  double pttrntot ; 
} PttrnTot ;

typedef struct __unknpttn__ {  /* Unknown Word를 위한 pattern           */
  int numofpttrn   ; /* Number of Pattern (Feature in PR) : 0 .. nop -1 */
  double totalprob ; /* \Sigma_{w_i+1,t_i+1} Freq(t_i,w_i+1,t_i+1)      */
  Pttrn *pttrnPtr  ; /* pttrn을 저장하는 장소                           */ 
} UnkPttrn ;

#endif

/*****************************************
 *************End of ktsds.h**************
 *****************************************/


