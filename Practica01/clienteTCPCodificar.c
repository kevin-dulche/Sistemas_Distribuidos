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

int fd;

// Manejar la señal SIGINT (Ctrl+C)
void manejar_signo(int sig)
{
    printf("\nRecibida señal de interrupción (Ctrl+C). Cerrando socket y liberando el puerto...\n");
    
    // Cerrar el socket
    close(fd);
    
    // Finalizar el programa
    exit(0);
}

int main(int argc, char *argv[])
{
    char request[255];
    char buffer[BUF_SIZE];

    if (argc != 3)
    {
        puts("\nModo de uso: ./clienteTCPCodificar <direccion IP> <# puerto>");
        exit(-1);
    }

    printf("\nAbriendo el socket...\n");
    fd = socket(AF_INET, SOCK_STREAM, 0);
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

    char name[100];
    char claveUEA[8];
    char grupo[6];

    printf("\nIngrese su nombre: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0'; // Eliminar el salto de línea de `name`

    printf("\nIngrese su clave de la UEA: ");
    scanf("%7s", claveUEA); // Limitar a 6 caracteres para `claveUEA`
    while (getchar() != '\n'); // Limpiar el buffer de entrada

    printf("\nIngrese su grupo: ");
    scanf("%5s", grupo); // Limitar a 4 caracteres para `grupo`
    while (getchar() != '\n'); // Limpiar el buffer de entrada

    // Codificar la petición
    sprintf(request, "%s#%s#%s", name, claveUEA, grupo);

    // Mostrar la cadena codificada para confirmar
    printf("Cadena codificada: %s\n", request);


    /*
     * Para enviar datos usar send()
     */
    send(fd, request, strlen(request), 0);

    printf("\nLa petición fue enviada al servidor.\n\n");

    // Variables para almacenar los datos recibidos
    int cupo_total;
    int solicitud_recibida;
    char datos_recibidos[255];

    // Recibir la cadena del servidor
    ssize_t bytes_recibidos = recv(fd, datos_recibidos, sizeof(datos_recibidos), 0);
    if (bytes_recibidos == -1)
    {
        perror("\nError al recibir los datos del servidor.\n");
        close(fd);
        exit(-1);
    }
    datos_recibidos[bytes_recibidos] = '\0'; // Asegurarse de que la cadena esté terminada en nulo

    // Mostrar los datos recibidos
    //printf("Datos recibidos: %s\n", datos_recibidos);

    // Usar sscanf para extraer los números de la cadena recibida
    if (sscanf(datos_recibidos, "Cupo total: %d, Solicitud recibida: %d", &cupo_total, &solicitud_recibida) == 2)
    {
        // Mostrar los valores extraídos
        printf("Cupo total del grupo: %d\n", cupo_total);
        printf("Número de solicitud recibida: %d\n", solicitud_recibida);
    }
    else
    {
        printf("Error al procesar los datos recibidos.\n");
    }

    close(fd);


    return 0;
}