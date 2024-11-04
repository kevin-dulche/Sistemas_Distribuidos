#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ETHSIZE 400

struct sockaddr_in serv;

/*
 *  sintaxis: cliente <direccion IP> <# puerto>
 */

int main(int argc, char *argv[])
{
    int fd, idb, result_sendto;
    char request[] = "Dame el clima para hoy - C - Dulche :)";
    char buffer[ETHSIZE];

    if (argc != 3)
    {
        printf("\n\nSintaxis: cliente <direccion IP> <# puerto>\n\n");
        exit(-1);
    }

    printf("\nAbriendo el socket...\n");
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
        printf("\nError, no se pudo abrir el socket.\n\n");
    else
        printf("\nSocket abierto.\n");

    printf("\nAsignando atributos al socket...\n");

    memset(&serv, sizeof(serv), 0);
    serv.sin_family = AF_INET;
    /* inet_addr convierte de cadena de caracteres (formato puntado
     * "128.112.123.1") a
     * octetos.
     */

    serv.sin_addr.s_addr = inet_addr(argv[1]);

    /* htons convierte un entero largo en octetos cuyo orden entienden
     * las funciones de los sockets
     */
    serv.sin_port = htons(atoi(argv[2]));

    /*
     * Para enviar datos usar sendto()
     */

    result_sendto = sendto(fd, request, 200, 0,
                           (const struct sockaddr *)&serv, sizeof(serv));
    if (result_sendto < 0)
    {
        printf("\nProblemas al enviar la peticion.\n\n");
        exit(-1);
    }
    else
    {
        printf("\nLa petición fue enviado al servidor.\n");
    }

    /* para recibir datos usar recvfrom
     */

    recvfrom(fd, (void *)buffer, ETHSIZE, 0,
             (struct sockaddr *)NULL, (socklen_t *)NULL);

    printf("\nEstos son los datos enviados por el servidor:\n");
    printf("'%s'\n\n", buffer);

    return 0;
}