/*
 * calcdiff.c
 * 
 * difference를 계산하여 출력한다. : 형태소와 어절 단위에서
 *
 * S.H.Lee
 *
 * 
 * 사용법 : calcdiff diff_fname
 *
 *          diff referent_fname output_fname | ediff > temp.diff
 *          awk ' { print $1 } ' temp.diff > diff_fname
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX(X,Y)  (((X)>(Y)) ? (X) : (Y))

char buffer1[15][100] ; 
char buffer2[15][100] ; 

char buf1[30][70]  ; 
char buf2[30][70]  ; 

int dpmtrx[30][30] ; /* matrix for dynamic programming */
int sizeofbuf1     ; /* 형태소의 갯수 : buffer 1 */
int sizeofbuf2     ; /* 형태소의 갯수 : buffer 2 */

int numofeojeol1 = 0  ; /* 어절 갯수 */
int numofeojeol2 = 0  ; /* 어절 갯수 */

int dpvalue ;  /* dynamic programming : value */

int final_eojeol = 0 ; 
int final_morph  = 0 ; 

void caldiff() ; 
void tokenize() ; 

main(argc,argv)
int argc     ; 
char *argv[] ; 
{ FILE *fptr       ; 
  char buffer[100] ; 
  int state = 1    ; 
  int flag = 1     ; 

  if (argc < 2) {
	fprintf(stderr,"Usage : calcdiff diff_file\n") ; 
	exit(1) ; 
  }

  if ((fptr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : File Not Found : %s\n",argv[1]) ; 
	exit(1) ; 
  }

  while (fscanf(fptr,"%s",buffer) != EOF) {

#ifdef DEBUG
    printf("input  : %s\n",buffer) ; 
#endif 

      if (!strncmp(buffer,"--------",8)) {
        state = 1 - state ;         /* state = 0 , 1 Toggle */   
        if (numofeojeol1 != 0) 
          if (state == 0) {
	        tokenize() ; 
	        caldiff() ; 
	        numofeojeol1 = numofeojeol2 = 0 ; 
          }

      } else {

        if(state == 0) strcpy(buffer1[numofeojeol1++],buffer) ;
        if(state == 1) strcpy(buffer2[numofeojeol2++],buffer) ; 

     } 
  }

  tokenize() ; 
  caldiff() ; 

  printf("어절 단위로 볼 때 틀린 갯수 : %d\n",final_eojeol) ; 
  printf("형태소 단위로 볼 때 틀린 갯수 : %d\n",final_morph) ; 

}/*------------------------------------------------*/

void tokenize()
{
  int count1 ;  
  int count2 ;  
  int buidx1 ; 
  
  sizeofbuf1 = 0 ; 
  sizeofbuf2 = 0 ; 

  /* buffer1[0 .. numofeojeol1 - 1][]의 내용을 buf1[][]에 넣는다 */
  for (count1 = 0 ; count1 < numofeojeol1 ; ++count1) {
	buidx1 = 0 ; 
	for (count2 = 0 ; buffer1[count1][count2] != '\0' ; ++count2) {
	  buf1[sizeofbuf1][buidx1++] = buffer1[count1][count2] ; 
      if (buffer1[count1][count2] == '+') {
		 buf1[sizeofbuf1][buidx1-1] = '\0' ; 
         buidx1 = 0 ; 
         ++sizeofbuf1 ; 
      }
    } 
	buf1[sizeofbuf1++][buidx1++] = buffer1[count1][count2] ; 
  }

#ifdef DEBUG
  { int tt ; 
	for(tt = 0 ; tt < sizeofbuf1 ; ++tt)
	  printf("buf1[%d] : %s\n",tt,buf1[tt]) ; 
  }
#endif

  /* buffer2[0 .. numofeojeol2 - 1][]의 내용을 buf2[][]에 넣는다 */
  for (count1 = 0 ; count1 < numofeojeol2 ; ++count1) {
	buidx1 = 0 ; 
	for (count2 = 0 ; buffer2[count1][count2] != '\0' ; ++count2) {
	  buf2[sizeofbuf2][buidx1++] = buffer2[count1][count2] ; 
      if (buffer2[count1][count2] == '+') {
		 buf2[sizeofbuf2][buidx1-1] = '\0' ; 
         buidx1 = 0 ; 
         ++sizeofbuf2 ; 
      }
    } 
	buf2[sizeofbuf2++][buidx1++] = buffer2[count1][count2] ; 
  }

#ifdef DEBUG
  { int tt ; 
	for(tt = 0 ; tt < sizeofbuf2 ; ++tt)
	  printf("buf2[%d] : %s\n",tt,buf2[tt]) ; 
  }
#endif


}/*-----------------------------------*/

/*
 *
 */

void caldiff()
{
  int indx ; 
  int maxvalue ; 
  int x_axis ; 
  int y_axis ; 
  int tmpval ; 
  int strlen1 , strlen2 ; 
  int is_hit ; 

  /* buf1[][] , buf2[][]에 내용이 들어가 있다  
     0 .. sizeofbuf1 - 1 , 0 .. sizeofbuf2 - 1 */

  for(x_axis = 0 ; x_axis < sizeofbuf1 ; ++x_axis) {
	if (!strcmp(buf1[x_axis],buf2[0])) dpmtrx[x_axis][0] = 1 ; 
	else dpmtrx[x_axis][0] = 0 ; 
  }

  for(y_axis = 0 ; y_axis < sizeofbuf2 ; ++y_axis) {
	if (!strcmp(buf1[0],buf2[y_axis])) dpmtrx[0][y_axis] = 1 ; 
	else dpmtrx[0][y_axis] = 0 ; 
  }

  for(x_axis = 1 ; x_axis < sizeofbuf1 ; ++x_axis)
	for(y_axis = 1 ; y_axis < sizeofbuf2 ; ++y_axis) {
	  is_hit = 0 ; 
	  if (!strcmp(buf1[x_axis],buf2[y_axis])) is_hit = 1 ; 
	  else {
	    strlen1 = strlen(buf1[x_axis]) ; 
	    strlen2 = strlen(buf2[y_axis]) ; 
	    if (buf1[x_axis][strlen1 - 2] == 's' &&  /* 양쪽 symbol일 경우 */
			buf2[y_axis][strlen2 - 2] == 's') is_hit = 1 ; 
      }

	  if (is_hit) { /* Match */
		tmpval = dpmtrx[x_axis-1][y_axis-1] ; 
        dpmtrx[x_axis][y_axis] = (tmpval > 0) ? tmpval + 1 : 1 ;  
      } else { /* No Match */
        maxvalue = MAX(dpmtrx[x_axis-1][y_axis],dpmtrx[x_axis-1][y_axis-1]) ; 
        maxvalue = MAX(maxvalue,dpmtrx[x_axis][y_axis-1]) ; 
        dpmtrx[x_axis][y_axis] = maxvalue ; 
      }
    }
 
  dpvalue = dpmtrx[sizeofbuf1-1][sizeofbuf2-1] ; /* 최종 값     */

#ifdef DEBUG
  printf("(Matched Value) dpvalue = %d\n",dpvalue) ; 
#endif

  if (dpvalue != sizeofbuf1) {  /* 같지 않다면 */
    final_morph += (sizeofbuf1 - dpvalue) ; /* (형태소/품사)가 틀렸다 */ 
  }

  for(indx = 0 ; indx < numofeojeol1 ; ++indx) 
    if(strcmp(buffer1[indx],buffer2[indx])) ++final_eojeol ; 


}/*-----------------------------------------*/


/************************************************************************
 *
 * 입력 string의 tag와 형태소가 똑같은 경우가 있을 수 있다.
 * 특히 코퍼스에 잘못되어 ?? , ?..등은 심볼인데도 틀리게 나올 수 있다.
 * 그래서 , 심볼들의 비교는 무조건 같다고 놓는다.
 * 
 * 결과는 다음과 같이 나와야 한다.
 *
 * 1. tag 열은 같은데 형태소 분석이 잘못 된것 : 어절   ==> 틀림
 *       나/pv+는/exm                           형태소 ==> 1개 틀림 
 *       --------
 *       나/px+는/exm
 *
 * 2. tag 열의 크기 자체가 틀린 경우 : 어절   ==> 틀림
 *       움직임/nc+을/jc               형태소 ==> 2개 틀림
 *       --------
 *       움직이/pv+ㅁ/exn+을/jc
 *
 * 3. 아래와 같이 tag열의 크기가 다른데 
 *       같은 (pos,품사)가 있는 경우 : 어절   ==> 2개 틀림 
 *       달라지/pv+ㄴ/exm            : 형태소 ==> 1개 틀림 
 *       게/npd
 *       --------
 *       다르/pa+어/ecx+지/px+ㄴ/exm
 *       것/nb+이/jc
 *
 * A. 어절이 틀린것을 count :
 *       형태소 , 품사 중 하나라도 어절 단위로 Error를 Count
 *
 * B. 형태소 단위로 Error를 Count
 *    buffer2에 있는 tag갯수로 count : 위의 경우-2 : 3개 
 *
 * Dynamic Programming으로 푼다.
 *
 * X축,Y축에 비교할 두개의 List를 놓는다. 
 *
 * 만약 자신이 Hit하면 : 왼쪽,사선,밑 중 사선 부분의 값에 하나를 더해
 *                       자신의 값이 된다.
 *      자신이 Hit하지 않으면 : 왼쪽,사선,밑 중 최고 값을 갖는다.
 *
 * 평가 : Matrix의 마지막 값을 Z라고 할 때
 *        Z가 |X-List|와 같다고 하지 않을 때
 *             어절   : 어절 갯수 틀렸다.
 *             형태소 : |X-List| - Z개 틀렸다.
 *
 ************************************************************************/


