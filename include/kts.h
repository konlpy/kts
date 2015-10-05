/*****************************************************
 * Korean Tagging System : KTS version 0.9           *
 *                                                   *
 * Macros for Korean Tagging System                  *
 *                                                   *
 * Sang Ho Lee                                       *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr *
 *                                                   *
 *****************************************************/

#ifndef LEESKTS
#define LEESKTS

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL '\0'
#endif

#ifndef ON
#define ON 1
#endif

#ifndef OFF
#define OFF 0
#endif

#ifndef SPACE
#define SPACE ' '
#endif

#ifndef NEWLINE
#define NEWLINE '\n'
#endif

#ifndef KN_EOJ      /* Known Eojeol   : ���¼� �м��� �� ����    */
#define KN_EOJ 'o'
#endif

#ifndef UN_EOJ      /* Unknown Eojeol : ���¼� �м��� �� �� ���� */
#define UN_EOJ 'x'
#endif

#ifndef MARK        /* n���� ��� �� threshold�� ���� ����� ���� ��Ŵ */
#define MARK   'o'
#endif

#ifndef UNMARK      /* n���� ��� �� threshold�� ���� ����� ���� �ȵ� */
#define UNMARK 'x'
#endif


/*
 * TAGGING OPERATOR : 
 *
 *   STATE_BEST : State-Based Tagging Best Only
 *   STATE_MULT : State-Based Tagging Multiple with threshold
 *   STATE_PATH : State-Based & Path-Based Tagging Multiple with threshold
 *   PATH_BEST  : Path-Based Tagging Best Only
 *   PATH_MULT  : Path-Based Tagging Multiple with threshold
 */

#ifndef TAG_OPRTR
#define TAG_OPRTR
#define STATE_BEST -2L
#define STATE_MULT -1L
#define STATE_PATH  0L
#define PATH_MULT   1L
#define PATH_BEST   2L
#endif


/* 
 * ������ ���¼� �м� ����� ���¼� ���� - 1 
 *
 * ��) ����  : ��/npp+��/jx <--- x 
 *     count :   0      1
 *           
 *     PATHLEN(x) = 1 
 */

#define PATHLEN(x)      ((x).pathlen)  


/*
 * ���¼� ���� Pool�� index�� return
 *
 * ��) ���� : ��/npp+��/jx <--- x
 *           
 *     INFORINDEX(x,0) = index , senP[index] = ��,npp
 *     INFORINDEX(x,1) = index , senP[index] = ��,jx
 */

#define INFORINDEX(x,i) ((x).path[(i)]) 


/*
 * ���¼� ���� Pool�� `���¼�'
 *
 * ��) ���� : ��/npp+��/jx <--- x 
 *
 *     UCHAR __morph[MAXMORPH] ; 
 *
 *     GetMORPH(INFORINDEX(x,0),__morph) ;
 *
 *     __morph = "��" 
 *
 */

#define GetMORPH(x,__morph) \
        if (senP[x].pos >= _NCT_) kimmo2ks(senP[x].word,__morph); \
	else strcpy(__morph,senP[x].word)


/*
 * ���¼� ���� Pool�� `ǰ��'
 *
 * ��) ���� : ��/npp+��/jx
 *
 *     char __tag[5] ; 
 *
 *     GetPOS(INFORINDEX(x,0),__tag) ; 
 *
 *     __tag = "npp"
 */

#define GetPOS(x,__tag) strcpy(__tag,posTags[senP[(x)].pos-'0'])


/*
 * ���¼� ������ Space�� �ʿ������� ���� ����
 * 
 * ��) `�̷���'�� `�̷��� ����'�� �м��� �ȴ�.
 *     �� ���, �ϳ��� ������ space�� �ʿ�� �Ѵ�.
 *     `��'������ ' '�� ����, '��'������ `+'�� ���´�.
 *
 *     MORPHSPACE(INFORINDEX(x,0)) = '+'
 *
 */

#define MORPHSPACE(x) ((senP[(x)].end_mark == 'o') ? ' ' : '+')


/* 
 * Prob(t_i|w_1..w_n)�� ���ϴ� ��ũ��
 *
 * 
 * ��) ����  : ��/npp+��/jx <--- x 
 *     TAGPROB(x) = 0.9
 *
 */

#define TAGPROB(x) ((x).gammaprob)


#endif

/*********************************************/
/*********** End of Macros for KTS ***********/
/*********************************************/


