#include <stdio.h>

main()
{
   int dot = 0 ; 
   char ch ; 

   while ((ch = getchar()) != EOF) {

	 if (ch == '.') dot = 1 ; 
     if (dot == 1 && ch != '.') {
	   if (ch != ',') {
	     putchar(' ') ; 
	     dot = 0 ; 
	   }
     }
	 putchar(ch) ; 
   }
}



