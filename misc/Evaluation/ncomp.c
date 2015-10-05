
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 9000

char res[MAXLINE]  ; 
char tag[MAXLINE]  ; 
char word[MAXLINE] ;

FILE *fpres, *fptag;

int o_num = 0 ; 
int x_num = 0 ; 

int main(argc , argv)
int argc     ; 
char *argv[] ; 
{
    int i, ok, rp, ncomp, ototal, xtotal, o_dif, x_dif;
    

    ototal= xtotal= o_dif= x_dif= 0;

    if (argc< 3) {
        puts("usage : ncomp result_file tagged_cor [n]");
        exit(1);
    }
    if (argc== 3) ncomp= 100;
    else ncomp= atoi(argv[3]);

    fpres = fopen(argv[1], "r");
    fptag = fopen(argv[2], "r");

    if (fpres== NULL || fptag== NULL) exit(1);

    while(1) {
        if (fgets(res, MAXLINE - 1 , fpres)== NULL) break;
        if (fgets(tag, MAXLINE - 1 , fptag)== NULL) break;

        if (tag[strlen(tag)- 1]== '\n') tag[strlen(tag)- 1]= (char)NULL; 
        if (res[strlen(res)- 1]== '\n') res[strlen(res)- 1]= (char)NULL; 
        if (res[0]== 'o') ototal++;
        if (res[0]== 'x') xtotal++;
        rp= 2;
        ok= 0;
        for (i= 0; i< ncomp; i++) {
            sscanf(res+ rp, "%s", word);
            if (!strcmp(word, tag)) {
                ok= 1;
                break;
            }
            rp+= strlen(word)+ 1;
            if (rp> strlen(res)) break;
        }
        if (!ok)  {
            if (res[0]== 'o') o_dif++;
            else x_dif++;
        }
    }
   
    printf("o : total= %d, diff= %d\n", ototal, o_dif);
    printf("x : total= %d, diff= %d\n", xtotal, x_dif);
    return 0;
}

