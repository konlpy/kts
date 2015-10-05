#include <stdio.h>

#define MAXLINE 11000

int o_num = 0 ; 
int x_num = 0 ; 

char token[400][200] ; /* token */
int numoftoken ; 

int column ; 

void main(argc, argv)
int argc ; 
char **argv ; 
{
    int i, ok, rp, ncomp, ototal, xtotal, o_hit, x_hit;

    char res[MAXLINE], tag[MAXLINE], word[MAXLINE];
    
    FILE *fpres, *fptag;

    ototal= xtotal= o_hit= x_hit= 0;

    if (argc< 3) {
        puts("usage : ncomp result_file tagged_cor [n]");
        exit(1);
    }
    if (argc== 3) ncomp= 2;
    else ncomp= atoi(argv[3]);

    fpres= fopen(argv[1], "r");
    fptag= fopen(argv[2], "r");

    if (fpres== NULL || fptag== NULL) exit(1);

    while(1) {
        if (fgets(res, MAXLINE, fpres)== NULL) break;
        if (fgets(tag, MAXLINE, fptag)== NULL) break;
        if (tag[strlen(tag)- 1]== '\n') tag[strlen(tag)- 1]= NULL; 
        if (res[strlen(res)- 1]== '\n') res[strlen(res)- 1]= NULL; 
        if (res[0]== 'o') ototal++;
        if (res[0]== 'x') xtotal++;

        numoftoken = 0 ; 
	column = 0 ; 

        for (i = 2 ; res[i] != '\0' ; ++i)
	  if (res[i] == ' ') {
	     token[numoftoken++][column] = '\0' ;  
	     column = 0 ; 
          } else {
             token[numoftoken][column++] = res[i] ; 
	  }
        token[numoftoken++][column] = '\0' ;
        
        for (i = 0 ; i < numoftoken ; ++i)
          if (!strcmp(token[i],tag)) {
	    if (res[0] == 'o') o_hit++ ; 
	    else x_hit++ ; 
          }

    }
   
    printf("o : total= %d, hit= %d\n", ototal, o_hit);
    printf("x : total= %d, hit= %d\n", xtotal, x_hit);
}

