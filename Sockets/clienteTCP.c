#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 400

/*
 *  sintaxis: cliente <direccion IP> <# puerto>
 */

int main(int argc, char *argv[])
{
    char request[] = "**************************************************";
    char buffer[BUF_SIZE];

    if (argc != 3)
    {
        puts("\nModo de uso: ./clienteTCP <direccion IP> <# puerto>");
        exit(-1);
    }

    printf("\nAbriendo el socket...\n");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        perror("\nError, no se pudo abrir el socket.\n\n");
    else
        perror("\nSocket abierto.\n");

    printf("\nAsignando atributos al socket...\n");
    struct sockaddr_in serv;
    memset(&serv, sizeof serv, 0);
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

    if (connect(fd, (struct sockaddr *)&serv, sizeof serv) < 0)
    {
        printf("\nNo se pudo realizar la conexión remota.\n");
        exit(-1);
    }

    int placas = 77;
    //sprintf(request, "Dulche :), %d", placas);
    sprintf(request, "Hola, soy la Laptop de Dulche");

    /*
     * Para enviar datos usar send()
     */
    send(fd, request, strlen(request), 0);

    printf("\nLa petición fue enviada al servidor.\n\n");

    /* para recibir datos usar recvfrom */
    read(fd, buffer, BUF_SIZE);

    printf("\nEstos son los datos enviados por el servidor:\n");
    printf("'%s'.\n\n", buffer);

    return 0;
}