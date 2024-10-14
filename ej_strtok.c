#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>

int main()
{
   char cadena[] = "ls test\ncat salida";
   char *ptrToken;
   char *saveptr;

   ptrToken = strtok_r( cadena, "\n", &saveptr);
   while ( ptrToken != NULL ) {
      printf( "%s\n", ptrToken );
      ptrToken = strtok_r( NULL, "\n", &saveptr );
   }
   return 0;
}
