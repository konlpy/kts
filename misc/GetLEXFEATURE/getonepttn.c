/*------------------------------------------------------------------

   Korean Part-Of-Speech Tagging System ( kts ) Version 1.0

   S.H.Lee

   Computer Systems Lab.
   Computer Science , KAIST

  getonepttn.c ( Korean POS Tagging System : get one pattern )

  -------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

main(argc,argv) 
int argc ; 
char *argv[] ; 
{
  FILE *fPtr ; 
  char in ; 

  if ((fPtr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : No Corpus\n") ; 
	exit(1) ; 
  }

  while(!feof(fPtr)) {
    in = getc(fPtr) ; 
    if (in == '/') {
      while ((in = getc(fPtr)) != '+' && in != '\n') 
	if (in != '/') putchar(in) ; 

      if (in == '\n') putchar('\n') ; 
      else {
       putchar(' ') ; 
       in = getc(fPtr) ;
       if (in == '+') {   /* +/sy */
        printf("+/sy\n") ; 
        while(getc(fPtr) != '\n') ; 
       } else {
	putchar(in) ; 
        while((in = getc(fPtr)) != '+' && in != '\n') putchar(in) ;
        if (in == '\n') putchar('\n') ; 
        else {
          while(getc(fPtr) != '\n') ; 
          putchar('\n') ; 
        }
       
       }
      }

    }
  }
  fclose(fPtr) ; 
}

