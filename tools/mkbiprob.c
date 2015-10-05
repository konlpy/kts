/*-------------------------------------------

  Get Word Ambiguity Probability

  S.H.Lee

  -------------------------------------------*/

#include <stdio.h>
#include <string.h>

FILE *fptr1 ; /* 전체 Tag의 갯수를 갖는 file */
FILE *fptr2 ; /* bigram의 갯수를 갖는 file   */

char word[100][5] ; 
int  freq[100]  ; 

int total_word  = 0 ;

char input_word[5] ; 
char to_word[5] ; 
int  input_freq ; 
float input_value ; 

main(argc,argv)
int argc     ; 
char *argv[] ; 
{
  int temp ; 

  if (argc != 3) {
	fprintf(stderr,"Usage : mkbiprob Left-Tag_i Bigram\n") ; 
	exit(1); 
  }

  if ((fptr1 = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : Total Tags %s not Found\n",argv[1]) ; 
	exit(1) ; 
  }
  if ((fptr2 = fopen(argv[2],"r")) == NULL) {
	fprintf(stderr,"Error : Bigram %s not Found\n",argv[2]) ; 
	exit(1); 
  }

  while(fscanf(fptr1,"%s %d",&word[total_word][0],&freq[total_word]) != EOF)
	  ++total_word ; 
  
  while(fscanf(fptr2,"%s %s %d",input_word,to_word,&input_freq) != EOF)
    for(temp = 0 ; temp < total_word ; ++temp)
	  if(!strcmp(input_word,word[temp]))
        printf ("%s %s %f\n",input_word,to_word,
		   (float) input_freq / (float) freq[temp]) ; 

  fclose(fptr1) ; 
  fclose(fptr2) ; 

}



