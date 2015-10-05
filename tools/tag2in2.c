/*---------------------------------------------------

  tag2in2 : �̰��� input-corpus�� inter.prob�� intra.prob��
	    ���ϴ� program�Դϴ�.

      1   : inter.prob���� "INI" ���� tag �� freq��
            ������ ���� ó���� tag�� õ���ϴ�
            Ȯ���̰�,

      0   : intra.prob���� "INI" ���� tag �� freq��
            �� ���� ó�� tag�� õ���ϴ�
            Ȯ���Դϴ�.

  ---------------------------------------------------*/

#include <stdio.h>
#include <string.h>

char buf[100]     ;  
char token[15][5] ; /* token���� ������ */

FILE *fi, *fo     ;

int gettoken()    ; 
void writetags()  ; 

main(argc,argv)
int argc;
char *argv[];
{
    char tag[5]    ;
    int  numtoken  ; 
    char pretag[5] ; /* Pre-Tag  */
    char nxttag[5] ; /* Next-Tag */

    if (argc < 3) { 
      printf("Usage : tag2in2 [in] [out]\n") ; 
      exit(1) ; 
    }

    strcpy(pretag,"INI") ; /* Pre-Tag parameter = INI */

    fi = fopen(argv[1], "r");
    fo = fopen(argv[2], "w");

    while(fgets(buf,99,fi) != NULL) {

      numtoken = gettoken(buf) ; /* get tokens from string */
      
      writetags(nxttag,pretag,numtoken) ; 

      strcpy(pretag,nxttag) ;  

    }

    fclose(fi);
    fclose(fo);
}/*--------------End of Main---------------------*/


int gettoken(str)
char str[] ; 
{
  int i       ; 
  int ret = 0 ; 
  int cnt = 0 ; 

  for(i = 0 ; str[i] != '\n' && str[i] != '\0' ; ++i)
    if (str[i] != ' ') token[ret][cnt++] = str[i] ;     
    else {
      token[ret++][cnt] = '\0' ; 
      cnt = 0 ;      
    } 

  token[ret++][cnt] = '\0' ; 

  return ret ; 
}/*------End of gettoken------*/


void writetags(nxttag,pretag,num)
char nxttag[] ; 
char pretag[] ; 
int num ; 
{
  int idx ; 

  fprintf(fo,"1 %s %s\n",pretag,token[0]) ; 

  fprintf(fo,"0 INI %s\n",token[0]) ; 

  for(idx = 1 ; idx < num ; ++idx) 
    fprintf(fo,"0 %s %s\n",token[idx-1],token[idx]) ; 

  strcpy(nxttag,token[num-1]) ; 
  if (!strcmp(nxttag,"s.") || !strcmp(nxttag,"sy")) 
       strcpy(nxttag,"INI") ; 

}/*---------End of writetags-----------*/





