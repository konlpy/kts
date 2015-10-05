/* 
 빈도수를 세기 위한 프로그램이다.
 
 사용번 : freq < in_sorted > out_with_frequency
 유닉스 명령어 uniq -c로 대체할 수 있다
*/

#include <stdio.h>
#include <string.h>
main()
{
	char categ[BUFSIZ];
	char precat[BUFSIZ];
	int freq;

	gets(precat); 
	freq = 1;
	while(gets(categ) != NULL) {
		if (! strcmp(precat, categ)) {
			freq++;
		} else {
			printf("%s %d\n",precat, freq); 
			freq = 1;
			strcpy(precat, categ);
		}
	}
	if (! strcmp(precat, categ)) {
		printf("%s %d\n",precat, freq); 
	} else {
		printf("%s %d\n",precat, freq); 
		printf("%s %d\n",categ, 1); 
	}
}

