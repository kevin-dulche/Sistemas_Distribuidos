#include<stdio.h>
#include<Tmk.h>

#define coordinador 0
#define N 4

typedef struct shared{
        int X;
        } info;

info * shared;

int * V;

int soycoor();

int main(int argc, char**argv){

        int sumaLocal=0,i,j,k,c;//vaariables locales a la maquina
        int arrayDim=100;

        { extern char * optarg;
                while((c=getopt(argc,argv,"d:"))!=-1)//Limpia el buffer
                switch(c){
                        case 'd':arrayDim=atoi(optarg);
                        break;
                }
        }
        Tmk_startup(argc,argv);

        if(Tmk_proc_id== coordinador){
                shared = (info *) Tmk_malloc(sizeof(info));
                if (shared== NULL)
                        Tmk_exit(-1);
                Tmk_distribute(&shared, sizeof(info));

        //        Tmk_lock_release(0);
        //        Tmk_lock_acquire(N-1);
          //      printf("La suma es: %d\n",shared->X);
           //     for( i=0; i< 3 ;i++)
             //     Tmk_lock_acquire(i);
               }
       Tmk_lock_acquire(Tmk_proc_id);
  //printf("suma es: %d\n",shared->X);

        Tmk_barrier(1);

        if(Tmk_proc_id == 0)
        {       shared->X =5;
                (shared->X) +=1;
                Tmk_lock_release(0);
        }
        else
           if(Tmk_proc_id<(N-1)){
                Tmk_lock_acquire(Tmk_proc_id-1);
                (shared->X) +=1;
                Tmk_lock_release(Tmk_proc_id);
                }
           else
                {
                   Tmk_lock_acquire(N-2);
                   (shared->X)+=1;
                 //  Tmk_lock_release(3);
                }


// printf("suma es: %d\n",shared->X);

        Tmk_barrier(1);
        if(soycoor())
            printf("suma es: %d\n",shared->X);

        Tmk_exit(0);
}


int soycoor(){
        if(Tmk_proc_id==coordinador)
                return 1;
        else
                return 0;
}
