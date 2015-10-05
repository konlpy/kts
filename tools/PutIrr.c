/*-------------------------------------------------------

   PutIrr : 불규칙 code를 사전에 넣는다. 
 
   Usage : PutIrr [bdslLi] Dict Irr > Object-file
 
   S.H.Lee

  -------------------------------------------------------*/


#include <stdio.h>
#include <string.h>

FILE *fPtr ;  /* input file */
FILE *fPtr2 ; /* reference file */
main(argc,argv) 
int argc ; 
char *argv[] ;
{
  unsigned char in[120] ; 
  unsigned char in2[120] ; 
  unsigned char in3[120] ; 
  unsigned char in4[120] ; 
  unsigned char in5[40] ; 
  unsigned char out[120] ;
  unsigned char refer[120] ; 
  int flag ; 
  char irr_char ; 

  if (argc != 4) {
	fprintf(stderr,"Usage : PutIrr [bdslLi] Dict-with-Prob Irr-File\n") ; 
    exit(1) ; 
  }
  if ((fPtr = fopen(argv[2],"r+")) == NULL) {
	fprintf(stderr,"Error : No Dict-with-Prob\n") ; 
	exit(1) ; 
  }
  if ((fPtr2 = fopen(argv[3],"r+")) == NULL) {
	fprintf (stderr,"Error : No Irr-File \n") ; 
	exit(1) ; 
  }
  irr_char = argv[1][0] ; 

/*
  while(!feof(fPtr)) {
   fgets(in3,118,fPtr) ;
*/
  while(fgets(in3,118,fPtr) != NULL) {

   if (numspace(in3) == 3) {
     printf("%s",in3) ; /* 이미 있는 것을 쓴다 */

	 sscanf(in3,"%s %s %s %s",in,in2,in4,in5) ; 
     rewind(fPtr2) ; 
     flag = 0 ; 
     while (!feof(fPtr2)) {
       fscanf(fPtr2,"%s",refer) ; 
         if (!strcmp(in2,refer)) {
	      flag = 1 ; 
    	  break ; 
         } 
       }

     if (flag) printf("%s %s %s %c\n",in,in2,in4,irr_char) ; 
    
   } else { 
	 sscanf(in3,"%s %s %s",in,in2,in4) ; 
     rewind(fPtr2) ; 
     flag = 0 ; 
     while (!feof(fPtr2)) {
       fscanf(fPtr2,"%s",refer) ; 
         if (!strcmp(in2,refer)) {
	      flag = 1 ; 
    	  break ; 
         } 
       }

     if (flag) printf("%s %s %s %c\n",in,in2,in4,irr_char) ; 
     else printf("%s %s %s\n",in,in2,in4) ; 
   }
 }

 fclose(fPtr) ; 
 fclose(fPtr2) ; 

}/*-------------------End of Main--------------------*/


int numspace(s)
char s[] ; 
{
  int i ; 
  int ret = 0 ; 
  for (i = 0 ; s[i] != '\0' ; ++i) 
    if (s[i] == ' ') ++ret ; 

  return ret ; 
}

