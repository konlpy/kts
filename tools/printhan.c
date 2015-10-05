/*------------------------------------------------------------------

   Korean Part-Of-Speech Tagging System ( kts ) Version 1.0

   S.H.Lee

   Computer Systems Lab.
   Computer Science , KAIST

  printhan.c ( Korean POS Tagging System : get tag sequence )

  -------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

main() 
{
  char buff[1024] ;

  while(scanf("%s",buff) != EOF) 
    if (buff[0] & 0x80) printf("%s\n",buff) ; 

  return 0 ; 
}

