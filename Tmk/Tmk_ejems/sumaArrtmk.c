#include <stdio.h>
#include <Tmk.h>
#define Coordinador 0

struct shared {
int sum;
int turn;
int *array;
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
   shared->array = (int *) Tmk_malloc(arrayDim *sizeof(int));
   if (shared->array == NULL)
     Tmk_exit(-1);
   for (i=0;i< arrayDim;i++)
      shared->array[i]= i+1;
   shared->turn =0;
   shared->sum = 0;
   Tmk_distribute(&shared, sizeof(shared));
}
Tmk_barrier(0);
{
int id0=Tmk_proc_id, id1= Tmk_proc_id +1;
int perProc = arrayDim/Tmk_nprocs;
int leftOver = arrayDim % Tmk_nprocs;

printf("Empiezo la suma: %d\n",id0);
start= id0 *perProc +id0 * leftOver /Tmk_nprocs;
end= id1 * perProc + id1 *leftOver / Tmk_nprocs;
}

Tmk_barrier(0);
{
int mySum=0;
for(i=start;i<end;i++)
  mySum+=shared->array[i];

Tmk_lock_acquire(0);
shared->sum+= mySum;
Tmk_lock_release(0);
}
 
Tmk_barrier(0);

if (Tmk_proc_id == 0)
{
  printf("Suma =  %d\n",shared->sum);
  Tmk_free(shared->array);
  Tmk_free(shared);
}

 Tmk_exit(0);
}



