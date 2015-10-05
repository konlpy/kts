/*---------------------------------------------------

  Convert tag sequence to input

  programmed by yjkim, 94.5.28.

  ---------------------------------------------------*/

#include <stdio.h>

main(argc,argv)
int argc;
char *argv[];
{
	FILE	*fi, *fo;
	char	tag[5];
    int     flag = 1 ; 

	if (argc < 3) { 
	  printf("Usage : tag2in1 [in] [out]\n") ; 
	  exit(1) ; 
    }

	fi = fopen(argv[1], "r");
	fo = fopen(argv[2], "w");

	while( fscanf(fi,"%s",tag) != -1 )
	{
       if (flag) {
		 fprintf(fo,"INI ") ; 
		 flag = 0 ; 
       }

		fprintf(fo, "%s\n", tag);
		if (strcmp("s.",tag) && strcmp("sy",tag))
		  fprintf(fo, "%s ", tag);
		else flag = 1 ; 
	}

	fclose(fi);
	fclose(fo);
}/*--------------End of Main---------------------*/

