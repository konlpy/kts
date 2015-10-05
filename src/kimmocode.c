/*------------------------------------------------------------------------
 *
 *		Code Conversion Program
 *
 *
 *------------------------------------------------------------------------
 *
 * Modified by SangHo Lee 
 *
 *------------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "hantable.h"
#define __KIMMOCODE__
#include "kimmocode.h"

/*--------------------------------------------------
 *  C2S	: flag for Combination to Complete
 *  S2C : flag for Complete to Combination
 */
#define	C2S	0
#define S2C	1
#define KS_MASK 0x80

#define CHO_FILL	0x09
#define JUNG_FILL	0x10
#define JONG_FILL	0x01

#define ERROR	0
#define SUCC	1
#define MAX	200

#define get_cho(x)	(((x) & 0x7C00) >> 10)
#define get_jung(x)	(((x) & 0x03E0) >> 5)
#define get_jong(x)	((x) & 0x001F)

char Chos[][2] = {
    "", "", "", "", "", "", "", "", "", "", "g", "q", "n", "d", "f", "l",
    "m", "b", "r", "s", "v", "", "j", "z", "c", "k", "t", "p", "h", "", "", ""
};

char Jungs[][2] = {
    "", "", "a", "8", "", "ya", "y8", "e", "", "9", "ye", "y9", "", "o", "wa", "w8",
    "", "wi", "yo", "u", "", "we", "w9", "wu", "", "yu", "_", "yi", "", "i", "", ""
};

char Jongs[][2] = {
    "", "", "G", "Q", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM", "LB", "LS", "LP", "LT",
    "LH", "M", "B", "BS", "S", "V", "*", "J", "C", "K", "T", "P", "H", "", "", ""
};

char act_tbl[5][6] = {
    "05500",
    "13311",
    "88818",
    "99949",
    "66666"
};

int kimmo2ks(src,des)
unsigned char src[] ;
unsigned char des[] ; 
{
  unsigned short int first ; 
  if (strchr(Is_head,src[0])) { /* starting with (N|L|B|M) */
    if (!kimmo2ks2(src+1,des+2)) return ERROR ;  
    switch(src[0]) {
     case 'N' : des[0] = 0xa4 ; des[1] = 0xa4 ; break ;
     case 'L' : des[0] = 0xa4 ; des[1] = 0xa9 ; break ;
     case 'B' : des[0] = 0xa4 ; des[1] = 0xb2 ; break ; 
     case 'M' : des[0] = 0xa4 ; des[1] = 0xb1 ; break ; 
    }
    return SUCC ; 
  } 
  return kimmo2ks2(src,des) ; 
}/*---------end of kimmo2ks--------*/

int kimmo2ks2(src, des)
unsigned char *src;
unsigned char *des;
{
    register int i = 0, j = 0;
    unsigned int result, action;
    char tag[MAX];
    char cho, jung1, jung2, jong1, jong2;
    char one;
    int value;
    int make2c();
    unsigned char tmp[MAX];

    if (src[0] == '\0') { des[0] = '\0' ; return SUCC ; }

    while (src[i]) {
        tag[i] = whattype(src[i]);
        i++;
    }

    tag[i] = 4;
    i = 0;

    action = cho = jung1 = jung2 = jong1 = jong2 = 0;
    while (src[i]) {
        one = src[i];
        value = act_tbl[tag[i]][tag[i+1]];

        if (value == '1' || value == '8' || value == '3') {
            if (jung1)
                jung2 = one;
            else
                jung1 = one;
            if (value == '8' || (value == '3' && jung2))
                action = 2;
        }
        else if (value == '2' || value == '9' || value == '4') {
            if (jong1)
                jong2 = one;
            else
                jong1 = one;
            if (value == '9' || (value == '4' && jong2))
                action = 2;
        }
        else if (value == '5')
            cho = one;
        else if (value == '6') {
            tmp[j++] = one;
            action = 1;
        }
        else {
           /*
              printf(" error : kimmocode is %s\n",src);
              exit(0);
            */
            return ERROR ; 
        }
        if (action > 1) {
            value = make2c(cho, jung1, jung2, jong1, jong2);
            tmp[j++] = (unsigned char)(value >> 8);
            tmp[j++] = (unsigned char)(value);
        }
        if (action > 0)
            action = cho = jung1 = jung2 = jong1 = jong2 = 0;
        i++;
    }
    tmp[j] = '\0';
    ks(tmp, des, 0);
}

int make2c(cho, jung1, jung2, jong1, jong2)
char cho, jung1, jung2, jong1, jong2;
{
    register int i;
    char upper, middle, lower;



    for (i = 0; i < 32; i++)
        if (Chos[i][0] == cho) {
            upper = i;
            break;
        }
    for (i = 0; i < 32; i++)
        if (Jungs[i][0] == jung1 && Jungs[i][1] == jung2) {
            middle = i;
            break;
        }
    for (i = 0; i < 32; i++)
        if (Jongs[i][0] == jong1 && Jongs[i][1] == jong2) {
            lower = i;
            break;
        }
    if (upper == 0) {
        if (middle != 0)
            upper = 0x15;
        else {
            cho = tolower(jong1);
            for (i = 0; i < 32; i++)
                if (Chos[i][0] == cho) {
                    upper = i;
                    break;
                }
            middle = JUNG_FILL;
            lower = JONG_FILL;
        }
    }
    if (lower == 0)
        lower = JONG_FILL;
    return   (unsigned int)(((upper << 10)+(middle << 5)+(lower)) | 0x8000);
}


int whattype(ch)
char ch;
{
    if (strchr(Is_jung, ch))
        return 2;
    if (strchr(Is_cho, ch))
        return 0;
    if (strchr(Is_half, ch))
        return 1;
    if (strchr(Is_jong, ch))
        return 3;
    return 4;
}

int ks2kimmo(src, des)
unsigned char src[];
unsigned char des[];
{
    register int one, i = 0, j = 0, k = 0;
    register int flag = 1 ; 
    unsigned char tmp[MAX];
    char cho, jung, jong;

    if (src[0] == 0xa4) { /* 한글 영역 ㄴㄹㅂㅁ 특별처리 */
      switch(src[1]) {
       case 0xa4 : des[j++] = 'N' ; k = 2 ; break ; /* 니은 */
       case 0xa9 : des[j++] = 'L' ; k = 2 ; break ; /* 리을 */
       case 0xb2 : des[j++] = 'B' ; k = 2 ; break ; /* 비읍 */
       case 0xb1 : des[j++] = 'M' ; k = 2 ; break ; /* 미음 */
      }
    }
    if (ks(src+k, tmp, S2C) == ERROR) {
        printf(" code error \n");
        return ERROR;
    }

    while (one = tmp[i++]) {
        if (one & KS_MASK) {
            one <<= 8;
            one += (unsigned int)(tmp[i++]);

            cho = get_cho(one);
            jung = get_jung(one);
            jong = get_jong(one);

            if (jung == JUNG_FILL && jong == JONG_FILL)
                des[j++] = toupper(Chos[cho][0]);
            else {
                if (Chos[cho][0])
                    des[j++] = Chos[cho][0];
                if (Chos[cho][1])
                    des[j++] = Chos[cho][1];
                if (Jungs[jung][0])
                    des[j++] = Jungs[jung][0];
                if (Jungs[jung][1])
                    des[j++] = Jungs[jung][1];
                if (Jongs[jong][0])
                    des[j++] = Jongs[jong][0];
                if (Jongs[jong][1])
                    des[j++] = Jongs[jong][1];
            }
        }
        else
            des[j++] = (unsigned char) one;
    }
    des[j] = '\0';

    return SUCC;
}


int ks(src, des, type)
unsigned char *src;
unsigned char *des;
int type;
{
    register int i = 0;
    int upper, result;

    while (*src) {
        if ((upper = (unsigned int)(*src++)) & KS_MASK) {
            upper <<= 8;
            upper += (unsigned int)(*src++);
            if ((result = syllable(upper, type)) == ERROR)
                return ERROR;
            *des++ = (unsigned char)(result >> 8);
            *des++ = (unsigned char) result;
        }
        else
            *des++ = (unsigned char)upper;
    }
    *des = '\0';
    return SUCC;
}

/*--------------------------------------------------------------------------
 *  NAME
 *	syllable(src, type)
 *  ARGUMENTS
 *	int src		: source code in lower 2 byte.
 *	int type	: flag one of C2S and S2C
 *  DESCRIPTION
 *	convert a syllable
 *  RETURN VALUES
 *	0	if error
 *	value	if corresponding code exists.
 */
int syllable(src, type)
int src;
int type;
{
    register int value;

    if (value = binsrch(KSC2CS, KSC_SIZE, src, type))
        return value;
    // 테이블은 순서대로 되어 있으므로, 같은 테이블을 써서
    // 바이너리 서치할 수 있다.
    
    if (type == S2C) {
        if (value = binsrch(KSC2S_C, KEY_SIZE, src, 1)) // S2C == 1
            return value;
    }
    if (type == C2S) {
        if (value = binsrch(KSC2C_S, KEY_SIZE, src, 0)) // C2S == 0
            return value;
    }
    return ERROR;
}

int binsrch(code, n, key, type)
int code[][2];
int n;
int key;
int type;
{
    register int lower = 0;
    register int upper = n - 1;
    register int mid;

    while (lower <= upper) {
        mid = (lower + upper) >> 1;
        if (key > code[mid][type])
            lower = mid + 1;
        else if (key < code[mid][type])
            upper = mid - 1;
        else if (key == code[mid][type])
            return code[mid][1-type];
    }
    return ERROR;
}
