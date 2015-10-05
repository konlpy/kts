#include <stdio.h>
#include <stdlib.h>
#include <string.h>

main(argc,argv)
int argc ; 
char *argv[] ; 
{
  FILE *fptr ; 
  char bef_str[256]  ; 
  char this_str[256] ; 

  bef_str[0] = '\0'  ;

  if (argc != 2) {
	fprintf(stderr,"Usage : plus file-name\n") ; 
	exit(1) ; 
  }

  if ((fptr = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr,"Error : can't open %s\n",argv[1]) ; 
    exit(1) ; 
  }

  while(fscanf(fptr,"%s",this_str) != EOF) {
    if(!strcmp(this_str,"./s.") || !strcmp(this_str,"!/s.") ||
	   !strcmp(this_str,"../sy") || !strcmp(this_str,".../sy") ||
	   !strcmp(this_str,"..../sy") || !strcmp(this_str,"...../sy") ||
	   !strcmp(this_str,"?/s.") || !strcmp(this_str,",/s,") ||
	   !strcmp(this_str,"-/s-"))
	{
	  strcat(bef_str,"+") ; 
	  strcat(bef_str,this_str) ; 
    } else {
      printf("%s\n",bef_str) ; 
      strcpy(bef_str,this_str) ; 
    }
  }
  fclose(fptr) ; 

}


