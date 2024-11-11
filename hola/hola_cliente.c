#include <stdio.h>
#include "hello.h"

int main(int argc, char *argv[]) {
   CLIENT *client;
   int *return_value, filler;
   char *server;
   
   if (argc != 2) {
      fprintf(stderr, "Usage: %s host_name\n", *argv);
      exit(1);
   }
   
   server = argv[1];
   
   if ((client=clnt_create(server, DISPLAY_PRG, DISPLAY_VER, "tcp")) == (CLIENT *) NULL) 
   {
      clnt_pcreateerror(server);
      exit(2);
   }
   
   
   printf("client : Calling function.\n");
   return_value = print_hello_1((void *) &filler, client);
   if (*return_value)
      printf("\nClient : Mission accomplished.\n");
   else
      printf("\nClient : Unable to display message.\n");
   return 0;
}   

