/*
 * calcdiff.c
 * 
 * difference�� ����Ͽ� ����Ѵ�. : ���¼ҿ� ���� ��������
 *
 * S.H.Lee
 *
 * 
 * ���� : calcdiff diff_fname
 *
 *          diff referent_fname output_fname | ediff > temp.diff
 *          awk ' { print $1 } ' temp.diff > diff_fname
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(X,Y)  (((X)>(Y)) ? (X) : (Y))

char buffer1[15][100]  ; 

char buffer2[15][100]  ; 

char buf1[30][70]  ; 
char buf2[30][70]  ; 

int dpmtrx[30][30] ; /* matrix for dynamic programming */
int sizeofbuf1     ; /* ���¼��� ���� : buffer 1 */
int sizeofbuf2     ; /* ���¼��� ���� : buffer 2 */

int numofeojeol1 = 0  ; /* ���� ���� */
int numofeojeol2 = 0  ; /* ���� ���� */

int dpvalue ;  /* dynamic programming : value */

int final_eojeol = 0 ; 
int final_morph  = 0 ; 
int final_posma  = 0 ; /* number of pos match */

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

  printf("���� ������ �� �� Ʋ�� ���� : %d\n",final_eojeol) ; 
  printf("���¼� ������ �� �� Ʋ�� ���� : %d\n",final_morph) ; 

}/*------------------------------------------------*/

void tokenize()
{
  int count1 ;  
  int count2 ;  
  int buidx1 ; 
  
  sizeofbuf1 = 0 ; 
  sizeofbuf2 = 0 ; 

  /* buffer1[0 .. numofeojeol1 - 1][]�� ������ buf1[][]�� �ִ´� */
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

  /* buffer2[0 .. numofeojeol2 - 1][]�� ������ buf2[][]�� �ִ´� */
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

  /* buf1[][] , buf2[][]�� ������ �� �ִ�  
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
	    if (buf1[x_axis][strlen1 - 2] == 's' &&  /* ���� symbol�� ��� */
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
 
  dpvalue = dpmtrx[sizeofbuf1-1][sizeofbuf2-1] ; /* ���� ��     */

#ifdef DEBUG
  printf("(Matched Value) dpvalue = %d\n",dpvalue) ; 
#endif

  if (dpvalue != sizeofbuf1) {  /* ���� �ʴٸ� */
    final_morph += (sizeofbuf1 - dpvalue) ; /* (���¼�/ǰ��)�� Ʋ�ȴ� */ 
  }

  for(indx = 0 ; indx < numofeojeol1 ; ++indx) 
    if(strcmp(buffer1[indx],buffer2[indx])) ++final_eojeol ; 

  /* Get Part-Of-Speech List */ 











}/*-----------------------------------------*/








/************************************************************************
 *
 * �Է� string�� tag�� ���¼Ұ� �Ȱ��� ��찡 ���� �� �ִ�.
 * Ư�� ���۽��� �߸��Ǿ� ?? , ?..���� �ɺ��ε��� Ʋ���� ���� �� �ִ�.
 * �׷��� , �ɺ����� �񱳴� ������ ���ٰ� ���´�.
 * 
 * ����� ������ ���� ���;� �Ѵ�.
 *
 * 1. tag ���� ������ ���¼� �м��� �߸� �Ȱ� : ����   ==> Ʋ��
 *       ��/pv+��/exm                           ���¼� ==> 1�� Ʋ�� 
 *       --------
 *       ��/px+��/exm
 *
 * 2. tag ���� ũ�� ��ü�� Ʋ�� ��� : ����   ==> Ʋ��
 *       ������/nc+��/jc               ���¼� ==> 2�� Ʋ��
 *       --------
 *       ������/pv+��/exn+��/jc
 *
 * 3. �Ʒ��� ���� tag���� ũ�Ⱑ �ٸ��� 
 *       ���� (pos,ǰ��)�� �ִ� ��� : ����   ==> 2�� Ʋ�� 
 *       �޶���/pv+��/exm            : ���¼� ==> 1�� Ʋ�� 
 *       ��/npd
 *       --------
 *       �ٸ�/pa+��/ecx+��/px+��/exm
 *       ��/nb+��/jc
 *
 * A. ������ Ʋ������ count :
 *       ���¼� , ǰ�� �� �ϳ��� ���� ������ Error�� Count
 *
 * B. ���¼� ������ Error�� Count
 *    buffer2�� �ִ� tag������ count : ���� ���-2 : 3�� 
 *
 * Dynamic Programming���� Ǭ��.
 *
 * X��,Y�࿡ ���� �ΰ��� List�� ���´�. 
 *
 * ���� �ڽ��� Hit�ϸ� : ����,�缱,�� �� �缱 �κ��� ���� �ϳ��� ����
 *                       �ڽ��� ���� �ȴ�.
 *      �ڽ��� Hit���� ������ : ����,�缱,�� �� �ְ� ���� ���´�.
 *
 * �� : Matrix�� ������ ���� Z��� �� ��
 *        Z�� |X-List|�� ���ٰ� ���� ���� ��
 *             ����   : ���� ���� Ʋ�ȴ�.
 *             ���¼� : |X-List| - Z�� Ʋ�ȴ�.
 *
 ************************************************************************/


