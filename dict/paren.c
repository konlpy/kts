/*----------------------------------------------*
 *                                              *
 * paren.c  : parenthesizing semantic markers   *
 *                                              *
 * JongJin Eun                                  *
 *                                              *
 *----------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

main(argc,argv)
int argc;
char *argv[];
{
	FILE *src, *des;
	char convert[81];
	int i, cnt, nomarker = 0, invalid = 0, linenum = 0;
	char *con[5], rest[81];
	char comma[2], rightparen[2];

	comma[0] = ',';			comma[1] = NULL;
	rightparen[0] = ')';	rightparen[1] = NULL;

	if(argc != 3) {
		printf("Usage : paren source destination \n");
		exit(1);
	}

	if((src = fopen(argv[1],"r")) == NULL) {
		printf("Error : can't open source file \"%s\"!!\n", argv[1]);
		exit(1);
	}

	if((des = fopen(argv[2],"w")) == NULL) {
		printf("Error : can't open destination file \"%s\"!!\n", argv[2]);
		exit(1);
	}

	while( fgets(convert,81,src) != NULL ) {
		linenum++;
		for(i = 0; i < 5; i++) {
			con[i] = (char *)malloc(81);
			con[i][0] = NULL;
		}
		sscanf(convert,"%s %s %s %s %s", 
			con[0], con[1], con[2], con[3], con[4]);

		if( con[0][0] == NULL ) 
			fprintf(des,"\n");
		else if( con[1][0] == NULL ) {
			invalid++;
			fprintf(des,"%s\n",con[0]);
		}
		else if( con[2][0] == NULL ) {
			nomarker++;
			fprintf(des,"%-5s %-16s ()\n",con[0], con[1]);
		}
		else {
			rest[0] = '('; 	rest[1] = NULL;
			strcat(rest,con[2]);
			for(i = 3; i < 5 && con[i][0] != NULL; i++) {
				strcat(rest,comma);
				strcat(rest,con[i]);
			}
			strcat(rest,rightparen);
			fprintf(des,"%-5s %-16s %s\n",con[0],con[1],rest);
		}
		for(i = 0; i < 5; i++) 
			free(con[i]);
	}	/* end of while */
	printf("The number of invalid line(entry) is %d\n", invalid);
	printf("The number of no marker line(entry) is %d\n", nomarker);
	fclose(src);
	fclose(des);
}
