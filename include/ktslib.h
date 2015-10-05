
/*****************************************************
 * Korean Tagging System Library : KTS version 0.9   *
 *                                                   *
 *                                                   *
 * Sang Ho Lee                                       *
 * jhlee@csone.kaist.ac.kr , shlee2@adam.kaist.ac.kr *
 *                                                   *
 *****************************************************/

#ifndef __KTSLIB__
#define __KTSLIB__

extern void State2FilePrlg(); /* State-Based Multiple : PROLOG-Batch   */
extern void State2FilePcfg(); /* State-Based Multiple : PCFG Parser    */
extern int  GetMorphTag()   ; 
extern void DisplayEojeol() ; 

extern void DisplayPath()   ; /* Path-Based Best Candidate Only        */
extern void DisplayPathM()  ; /* Path-Based Multiple Candidates        */
extern void DisplayState()  ; /* State-Based Best & Multiple           */
extern void DisplayNBest()  ; /* Display N-Best Cutting                */

extern long OpenKTS()       ; /* open_kts/2 in sicstus prolog          */
extern long CloseKTS()      ; /* close_kts/2 in sicstus prolog         */
extern long MAT()           ; /* mat/3 in sicstus prolog               */

extern void PutAnalysis()   ; /* 간단한 태깅 분석 결과 출력            */ 

extern int idxOfPath[__NUMEOJEOLINSENT__] ; /* trellis의 각 path의 idx */
extern Trellis trellis                    ; /* trellis Header          */
extern Sentence senP[__SENTLENGTH__]      ; /* 태깅 결과 Pool          */
extern unsigned char _NCT_                ; /* 한글 Tag Set의 시작 Tag */
extern char posTags[__NUMOFTAG__][4]      ; /* 한글 Tag Set            */

#endif

