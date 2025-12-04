#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <Tmk.h>

#define Coordinador 0

int main(int argc, char **argv)
{
   
   int c;
           
   // Necesario para obtener parámetros que no usarán en el programa.
   extern char *optarg;
   while ( (c = getopt(argc,argv,"d:")) != -1);
   
   Tmk_startup(argc,argv);
   
   if (Tmk_proc_id == Coordinador) 
   {
      printf("Hola mundo, soy el coordinador!\n");
   }
   else 
   {
      printf("Hola mundo, soy el proceso %d!\n", Tmk_proc_id);
   }
   
   Tmk_barrier(0);
   
   if (Tmk_proc_id == 0) 
   {
      printf("Terminamos!\n");
   }
   
   Tmk_exit(0);
}



