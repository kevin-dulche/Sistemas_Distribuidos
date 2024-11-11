#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ETHSIZE 400
#define PORT 7780

struct sockaddr_in serv, cli;

int main()
{
    int fd, idb, cli_len, size_recv;
    char request[ETHSIZE];
    char datos_para_el_cliente[] = "El clima para hoy se anuncia nublado";

    printf("\nSe creará el socket...\n");
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
        printf("\nError, no se pudo crear el socket.\n");
    else
        printf("\nSocket creado.\n");

    printf("\nSe asignarán los atributos al socket...\n");
    memset(&serv, sizeof(serv), 0);
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(PORT);

    idb = bind(fd, (struct sockaddr *)&serv, sizeof(serv));

    if (idb < 0)
        printf("\nNo se asignaron los atributos.\n");
    else
        printf("\nSí se asignaron los atributos.\n");

    printf("\nEstoy escuchando en el puerto %d...\n", PORT);
    /* Aqui esperamos la peticion del cliente */
    cli_len = ETHSIZE;
    while (1)
    {
        size_recv = recvfrom(fd, (void *)request, ETHSIZE, 0,
                            (struct sockaddr *)&cli, (socklen_t *)&cli_len);

        if (size_recv < 0)
        {
            printf("\nHubo un problema con el recvfrom.\n");
            exit(-1);
        }
        else
        {
            printf("\nEl request recibido fue: '%s'\n\n", request);
        }

        /* Aqui enviamos los datos al cliente */
        sendto(fd, datos_para_el_cliente, strlen(datos_para_el_cliente), 0,
            (struct sockaddr *)&cli, (socklen_t)cli_len);
    }
    
    return 0;
}