#include <stdio.h>
#include "Tmk.h"


#define SIZE 1000

struct shared {
  int sum;
  int turn;
  int *array;
};

struct shared *share1;
struct shared *share2;



void InitializeArray(void) {
  int start;
  int end;
  

  start = Tmk_proc_id * (SIZE / Tmk_nprocs);
  end = (Tmk_proc_id + 1) * (SIZE / Tmk_nprocs);

  for (;start<end;start++)
    shared->array[start]=start;
  
  
  return;
}


void PrintArray(void) {
  int i;
  
  for (i=0; i<SIZE; i++)
    printf("Element %d = %d \n",i,shared->array[i]);
  
  return;

}


void main(int argc, char **argv) {


  extern char *optarg;

  int c;
  

  int iterations;
  int M;
  int N;

  while ((c = getopt(argc, argv, "i:m:n:")) != -1)
    switch (c) {
    case 'i':
      iterations = atoi(optarg);
      break;
    case 'm':
      M = atoi(optarg);
      break;
    case 'n':
      N = atoi(optarg);
      break;
    }



  Tmk_startup(argc,argv);
  

  if (Tmk_proc_id==0) {
    share1 = (struct shared *)Tmk_malloc(sizeof(share1));
    share2 = (struct shared *)Tmk_malloc(sizeof(share2));
    
    if (shared==NULL)
      Tmk_exit(-1);
    Tmk_distribute(&shared,sizeof(shared));
    
    shared->array = (int *) Tmk_malloc(sizeof(int)*SIZE);
    if (shared->array==NULL)
      Tmk_exit(-1);

  }

  Tmk_barrier(0);

  InitializeArray();

  Tmk_barrier(1);

  if (Tmk_proc_id==0)
    PrintArray();


  Tmk_exit(0);
  
  return;
} 



