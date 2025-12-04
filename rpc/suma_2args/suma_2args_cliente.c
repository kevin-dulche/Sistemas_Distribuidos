
#include "suma_2args.h"


int main (int argc, char *argv[])
{
    char *host;

    if (argc < 4) {
        printf ("usage: %s server_host num1 num2\n", argv[0]);
        exit (1);
    }
    host = argv[1];

    int primero, segundo;
    sscanf(argv[2], "%d", &(primero));
    sscanf(argv[3], "%d", &(segundo));

    CLIENT *clnt;
    int  *result_1;

    clnt = clnt_create (host, CALC_PROG, CALC_VER, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror (host);
        exit (1);
    }

    /* Acá se hace la invocación REMOTA */	
    result_1 = suma_1(primero, segundo, clnt);

    if (result_1 == (int *) NULL) {
        clnt_perror (clnt, "call failed");
    }
    else {
        printf("Operación: %d + %d = %d\n", 
                primero, segundo, *result_1); 
    }

    result_1 = multiplicacion_1(primero, segundo, clnt);

    if (result_1 == (int *) NULL) {
        clnt_perror (clnt, "call failed");
    }
    else {
        printf("Operación: %d * %d = %d\n", 
                primero, segundo, *result_1); 
    }

    clnt_destroy (clnt);

    exit (0);
}