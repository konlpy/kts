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

/* �ϳ��� ���ڴ� edge�� ǥ���ȴ� */

typedef struct __inputW__ {       /* Input Word Pool             */
  short int left                ; /* ������ ���� node            */
  short int right               ; /* ������ ������ node          */
  unsigned char start           ; /* �� ���ڷκ��� ���¼Ұ� ���� */
  unsigned char end             ; /* �� ���ڱ��� ���¼Ұ� ��     */
  unsigned char lee             ; /* �� ���� code                */
  unsigned char irr[__NUMIRR__] ; /* �ұ�Ģ ���ɼ�               */ 
} InputW ; 

typedef struct __morph__ {    /* Morph Pool */
  short int env             ; /* environment ��ȣ                             */
  char  end_mark            ; /* ���� ������ Space�� �ִ����� ���� Marker   */
  short int left            ; /* ���¼��� ���� node                           */
  short int right           ; /* ���¼� �ϳ��� ������ node                    */ 
  short int wright          ; /* ���� ���¼� �ؼ� ����� ������ node MAX      */
  short int nidx[__NUMCONNECTION__] ; /* ���� ���¼� ����Ű�� index:-1:��     */
  unsigned char word[__MAXMORPH__] ; /* �ϳ��� ���¼� ���̴� __MAXMORPH__     */ 
  double prob               ; /* ���¼��� P(word|tag) = C(word,tag)/C(tag)    */
  unsigned char pos         ; /* ���¼��� tag index                           */
  unsigned char irr         ; /* ���¼��� irregular code                      */
} Morph ; 

typedef struct __WSM__ {       /* Word Stack Manager */
  short int begin  ; /* ���� ȯ���� ���¼� begin node ��ȣ                    */ 
  short int end    ; /* ���� ȯ���� ���¼� end node ��ȣ                      */
  short int top    ; /* split Ȥ�� merge�� �Ǿ� ��ȭ�� �� ���� top            */ 
  short int bottom ; /* split Ȥ�� merge�� �Ǿ� ��ȭ�� �� ���� bott           */ 
  short int ini    ; /* ���� ȯ���� ini node ��ȣ : -1 : don't care condition */
  short int fin    ; /* ���� ȯ���� fin node ��ȣ : -1 : don't care condition */
  char direct      ; /* ���ڸ� ����� ���� : 'L' : ���� : 'R' : ������        */
} WSM ;

typedef struct __sent__ {
  char end_mark                    ; /* �� ������ ���� ��Ÿ���� Mark      */
  int sent_begin                   ; /* ���� �����ϴ� ��ȣ                */
  int sent_end                     ; /* ���� ������ ��ȣ                  */
  int morph_begin                  ; /* ���� ������ �����ϴ� ��ȣ         */
  int morph_end                    ; /* ���� ������ ������ ��ȣ           */
  double prob                      ; /* ���¼��� P(word|tag)              */
  unsigned char word[__MAXMORPH__] ; /* �ϳ��� ���¼� ���� : __MAXMORPH__ */
  unsigned char pos                ; /* ���¼� ǰ�� index                 */
  short int nidx[__NUMCONNECTION__] ; /* ���� ���¼� ����Ű�� next  index */
} Sentence ; 

typedef struct __m__ { /* morphP[]�� scanning�ϱ� ���� �ڷᱸ��           */
   short int env     ; /* environment ��ȣ                                */
   short int left    ; /* ���¼��� ���� node                              */
   short int right   ; /* ���¼� �ϳ��� ������ node                       */ 
   short int wright  ; /* ���� ���¼� �ؼ� ����� ������ node MAX         */
} Morph_Part ;


/*--------------------------------------------------------*/
/*-------------- Added From Version 0.9 ------------------*/ 
/*--------------------------------------------------------*/
/*-------- Data Structure for Multiple Tagging -----------*/
/*--------------------------------------------------------*/

typedef struct __pre__ {             /* Pre-Morphological Analyzed DS */ 
  unsigned char pos                ; /* ���¼��� ǰ�� index           */
  char mark                        ; /* '+' : �������� separater      */ 
                                     /* '*' : ������� separater      */
                                     /* '$' : ���¼� �� �м��� ��     */
  unsigned char word[__MAXMORPH__] ; /* �ϳ��� ���¼�                 */
} PreMor ;

typedef struct __lr__ {  /* Pre-Morph�� �¿� Context ������ ����  */ 
  unsigned char lmark ;  /* ��) +��$ ��/jx : ������ + �������� $  */ 
  unsigned char rmark ;  /*     + : ������     $ : ������ �¿� �� */
} JoinMark ; 

typedef struct __path__ {    /* path�� senP[]�� index���� ��Ÿ����   */ 
  int pathlen              ; /* ������ ���¼� path�� ���� in path[]  */ 
  int path[__MAXPATHLEN__] ; /* ������ ���¼� path�� �����Ѵ�        */
  char ox[3]               ; /* 
			      * ox[0] : State-Based�� ���   
			      * ox[1] : State-Based & Path-Based�� ���
			      * ox[2] : Path-Based�� ���
			      */ 

                             /* 
                              * For Viterbi Tagging : 
                              * Path-Based Tagging                   
		              */

  double lexprob           ; /* Prob(����|Tag Sequence)              */
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

typedef struct __trellis__ {     /* POS Tagging�� ���� �ڷᱸ��      */ 
  struct __trellis__ *backPtr ;  /* ���� ������ ����Ű�� Pointer     */
  struct __trellis__ *nextPtr ;  /* ���� ������ ����Ű�� Pointer     */
  unsigned char eojeol[__EOJEOLLENGTH__] ; /* ������ ���� char array */
  int startIdx   ;               /* senP[]�� ���� ���� Index         */
  int endIdx     ;               /* senP[]�� ���� ������ Index       */
  int numPath    ;               /* �ϳ��� �������� ����� Path ���� */
  Path *pathPool ;               /* ������ ������ ���¼� Path�� ���� */
                                 /* 
                                  * For Maximum Likelihood Tagging : 
				  * State-Based Tagging 
				  */
  int *sortedIdx ;            /* gammaprob�� sorting ���� �� indices */
  char known     ;            /* known word 'o' , Unknown word 'x'   */
} Trellis ; 

typedef struct __pathRep__ {  /* Path Representation */
  struct __pathRep__ *nextPtr ; 
  int epathlen              ; /* ��� ������ ������ �����ΰ� ?     */ 
  int epath[__NUMEOJEOLINSENT__] ; /* ������ ���� path�� �����Ѵ�    */
  Trellis *curTrPtr         ; /* Current Trellis Pointer             */
  double fprob              ; /* f(n) = g(n) + h*(n)                 */
  double gprob              ; /* tree�� ó������ ��������� cost     */
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

typedef struct __pttntot__ {     /* pattern�� ��ü pttrn�� ���� ���� */ 
  UCHAR  pttrn[__MAXPATHLEN__] ; 
  double pttrntot ; 
} PttrnTot ;

typedef struct __unknpttn__ {  /* Unknown Word�� ���� pattern           */
  int numofpttrn   ; /* Number of Pattern (Feature in PR) : 0 .. nop -1 */
  double totalprob ; /* \Sigma_{w_i+1,t_i+1} Freq(t_i,w_i+1,t_i+1)      */
  Pttrn *pttrnPtr  ; /* pttrn�� �����ϴ� ���                           */ 
} UnkPttrn ;

#endif

/*****************************************
 *************End of ktsds.h**************
 *****************************************/


