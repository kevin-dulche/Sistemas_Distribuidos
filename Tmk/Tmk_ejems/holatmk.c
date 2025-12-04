#include <stdio.h>
#include <Tmk.h>
#define Coordinador 0


main(int argc, char **argv)
{
   
   int start, end, i,c, p, arrayDim  = 100;
   
   {
      extern char *optarg;
      while ((c= getopt(argc,argv,"d:"))!=-1)
         switch(c) {
            case 'd': arrayDim = atoi(optarg);
            break;
         } 
   }
   
   Tmk_startup(argc,argv);
   if (Tmk_proc_id == Coordinador)
   {
      printf("Hola soy el coordinador!\n");
   }
   //Tmk_barrier(0);
   else{
      printf("Hola Soy : %d!\n",Tmk_proc_id);
   }
   
   Tmk_barrier(0);
   
   if (Tmk_proc_id == 0)
   {
      printf("Terminamos!\n");
   }
   
   Tmk_exit(0);
}



