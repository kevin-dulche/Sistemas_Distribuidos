#include <stdio.h>
#include <Tmk.h>
#define Coordinador 0

struct shared {
int sum;
}* shared;

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
   shared = (struct shared *) Tmk_malloc(sizeof(shared));
   if (shared== NULL)
     Tmk_exit(-1);
   shared->sum = 0;
   Tmk_distribute(&shared, sizeof(shared));
}
Tmk_barrier(0);
{
int id0=Tmk_proc_id;
int i,sumlocal=0;

printf("Soy %d, Empiezo la suma: \n",id0);
for(i=1;i<11;i++)
    sumlocal+=i;
printf("Mi suma local fue: %d\n",sumlocal);

Tmk_lock_acquire(0);
shared->sum+= sumlocal;
Tmk_lock_release(0);
}
 
Tmk_barrier(0);

if (Tmk_proc_id == 0)
{
  printf("Suma Total=  %d\n",shared->sum);
  Tmk_free(shared);
}

 Tmk_exit(0);
}



