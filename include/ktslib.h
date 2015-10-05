
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

extern void PutAnalysis()   ; /* ������ �±� �м� ��� ���            */ 

extern int idxOfPath[__NUMEOJEOLINSENT__] ; /* trellis�� �� path�� idx */
extern Trellis trellis                    ; /* trellis Header          */
extern Sentence senP[__SENTLENGTH__]      ; /* �±� ��� Pool          */
extern unsigned char _NCT_                ; /* �ѱ� Tag Set�� ���� Tag */
extern char posTags[__NUMOFTAG__][4]      ; /* �ѱ� Tag Set            */

#endif

