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

#ifndef KN_EOJ      /* Known Eojeol   : 형태소 분석이 된 어절    */
#define KN_EOJ 'o'
#endif

#ifndef UN_EOJ      /* Unknown Eojeol : 형태소 분석이 안 된 어절 */
#define UN_EOJ 'x'
#endif

#ifndef MARK        /* n개의 결과 중 threshold에 의해 결과에 포함 시킴 */
#define MARK   'o'
#endif

#ifndef UNMARK      /* n개의 결과 중 threshold에 의해 결과에 포함 안됨 */
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
 * 어절의 형태소 분석 결과의 형태소 갯수 - 1 
 *
 * 예) 나는  : 나/npp+는/jx <--- x 
 *     count :   0      1
 *           
 *     PATHLEN(x) = 1 
 */

#define PATHLEN(x)      ((x).pathlen)  


/*
 * 형태소 정보 Pool의 index를 return
 *
 * 예) 나는 : 나/npp+는/jx <--- x
 *           
 *     INFORINDEX(x,0) = index , senP[index] = 나,npp
 *     INFORINDEX(x,1) = index , senP[index] = 는,jx
 */

#define INFORINDEX(x,i) ((x).path[(i)]) 


/*
 * 형태소 정보 Pool의 `형태소'
 *
 * 예) 나는 : 나/npp+는/jx <--- x 
 *
 *     UCHAR __morph[MAXMORPH] ; 
 *
 *     GetMORPH(INFORINDEX(x,0),__morph) ;
 *
 *     __morph = "나" 
 *
 */

#define GetMORPH(x,__morph) \
        if (senP[x].pos >= _NCT_) kimmo2ks(senP[x].word,__morph); \
	else strcpy(__morph,senP[x].word)


/*
 * 형태소 정보 Pool의 `품사'
 *
 * 예) 나는 : 나/npp+는/jx
 *
 *     char __tag[5] ; 
 *
 *     GetPOS(INFORINDEX(x,0),__tag) ; 
 *
 *     __tag = "npp"
 */

#define GetPOS(x,__tag) strcpy(__tag,posTags[senP[(x)].pos-'0'])


/*
 * 형태소 다음에 Space가 필요한지에 대한 정보
 * 
 * 예) `이러지'는 `이렇게 하지'로 분석이 된다.
 *     이 경우, 하나의 어절이 space를 필요로 한다.
 *     `게'다음은 ' '를 갖고, '하'다음은 `+'를 갖는다.
 *
 *     MORPHSPACE(INFORINDEX(x,0)) = '+'
 *
 */

#define MORPHSPACE(x) ((senP[(x)].end_mark == 'o') ? ' ' : '+')


/* 
 * Prob(t_i|w_1..w_n)을 구하는 매크로
 *
 * 
 * 예) 나는  : 나/npp+는/jx <--- x 
 *     TAGPROB(x) = 0.9
 *
 */

#define TAGPROB(x) ((x).gammaprob)


#endif

/*********************************************/
/*********** End of Macros for KTS ***********/
/*********************************************/


