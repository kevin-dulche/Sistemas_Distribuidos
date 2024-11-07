#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define ETHSIZE 400
#define PORT 8000
#define BUF_SIZE 5

int socket_fd;  // Hacer socket_fd global para poder acceder en la función de señal

// Manejar la señal SIGINT (Ctrl+C)
void manejar_signo(int sig)
{
    printf("\nRecibida señal de interrupción (Ctrl+C). Cerrando socket y liberando el puerto...\n");
    
    // Cerrar el socket
    close(socket_fd);
    
    // Finalizar el programa
    exit(0);
}

int main()
{
    char request[ETHSIZE];
    char datos_para_el_cliente[] = "Saludos desde el servidor TCP :) de Dulche";

    printf("\nSe creará el socket...\n");
    socket_fd = socket(AF_INET, SOCK_STREAM, 0); // Protocolo TCP
    if (socket_fd == -1)
    {
        perror("\nError, no se pudo crear el socket.\n");
        exit(-1);
    }
    else
        perror("\nSocket creado.\n");

    // Registrar la señal SIGINT para que ejecute la función manejar_signo
    signal(SIGINT, manejar_signo);

    printf("\nSe asignarán los atributos al socket...\n");

    struct sockaddr_in addr_serv;
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Error al establecer la opción SO_REUSEADDR");
        exit(-1);
    }

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

    int cupoGrupo = 30;
    int numSolictudes = 0;

    typedef struct
    {
        char nombre[100];
        char claveUEA[8];
        char grupo[6];
    } Alumno;

    Alumno alumnos[10];

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

        // Decodificar la petición
        char name[100];
        char claveUEA[8];
        char grupo[6];
        sscanf(request, "%[^#]#%[^#]#%s", name, claveUEA, grupo);

        printf("\nLa petición recibida fue: %s.\n", request);
        printf("El nombre del cliente es: %s.\n", name);
        printf("La clave de la UEA del cliente es: %s.\n", claveUEA);
        printf("El grupo del cliente es: %s.\n", grupo);
        // la ip del cliente
        printf("La dirección IP del cliente es: %s\n", inet_ntoa(addr_serv.sin_addr));

        strcpy(alumnos[numSolictudes].nombre, name);
        strcpy(alumnos[numSolictudes].claveUEA, claveUEA);
        strcpy(alumnos[numSolictudes].grupo, grupo);
        numSolictudes++;

        printf("\nSe han recibido %d solicitudes.\n", numSolictudes);
        // imprimir los datos de los alumnos
        for (int i = 0; i < numSolictudes; i++)
        {
            printf("\nNombre: %s\n", alumnos[i].nombre);
            printf("Clave de la UEA: %s\n", alumnos[i].claveUEA);
            printf("Grupo: %s\n", alumnos[i].grupo);
        }

        // Crear la cadena con los datos
        char datos_para_el_cliente[255];
        sprintf(datos_para_el_cliente, "Cupo total: %d, Solicitud recibida: %d", cupoGrupo, numSolictudes);

        // Enviar los datos al cliente
        send(newSocket_fd, datos_para_el_cliente, strlen(datos_para_el_cliente) + 1, 0);

        printf("\nYa envié el mensaje al cliente. Adiós.\n\n");
        close(newSocket_fd);
    }

    return 0;
}
