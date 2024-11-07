#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ETHSIZE 400
#define PORT 8000
#define BUF_SIZE 5

int main()
{
    char request[ETHSIZE];
    char datos_para_el_cliente[] = "Saludos desde el servidor TCP :) de Dulche";

    printf("\nSe creará el socket...\n");
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0); // Protocolo TCP
    if (socket_fd == -1)
    {
        perror("\nerror, no se pudo crear el socket.\n");
        exit(-1);
    }
    else
        perror("\nSocket creado.\n");

    printf("\nSe asignarán los atributos al socket...\n");

    struct sockaddr_in addr_serv;
    memset(&addr_serv, sizeof addr_serv, 0);
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_serv.sin_port = htons(PORT);

    int idb = bind(socket_fd, (struct sockaddr *)&addr_serv, sizeof addr_serv);
    if (idb < 0)
        perror("\nNo se asignaron los atributos.\n");
    else
        perror("\nSi se asignaron los atributos.\n");

    if (listen(socket_fd, BUF_SIZE) < 0)
    {
        perror("\nError al tratar de escuchar.\n");
        exit(-1);
    }

    while (1)
    {
        printf("\nEsperando una conexión de un cliente en el puerto %d...\n", PORT);
        
        socklen_t addrLen;
        int newSocket_fd = accept(socket_fd, (struct sockaddr *)&addr_serv, &addrLen);
        if (newSocket_fd < 0)
        {
            perror("\nError al aceptar la conexión del cliente.\n");
            exit(-1);
        }

        int size_recv = read(newSocket_fd, request, ETHSIZE);
        printf("La petición recibida fue: %s.\n", request);
        // la ip del cliente
        printf("La dirección IP del cliente es: %s\n", inet_ntoa(addr_serv.sin_addr));

        /* Aqui enviamos los datos al cliente */
        send(newSocket_fd, datos_para_el_cliente, strlen(datos_para_el_cliente) + 1, 0);

        printf("\nYa envié el mensaje al cliente. Adiós.\n\n");
    }

    return 0;
}