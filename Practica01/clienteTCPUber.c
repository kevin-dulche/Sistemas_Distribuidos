#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

char datos_recibidos[1024];

void recepcion_datos(int fd) {
    ssize_t bytes_recibidos = recv(fd, datos_recibidos, sizeof(datos_recibidos) - 1, 0);
    if (bytes_recibidos == -1) {
        perror("\nError al recibir los datos del servidor.\n");
        close(fd);
        exit(-1);
    }
    datos_recibidos[bytes_recibidos] = '\0';
    printf("Datos recibidos: %s\n", datos_recibidos);
}

int main(int argc, char *argv[]) {
    char request[255];

    if (argc != 3) {
        puts("\nModo de uso: ./clienteTCPUber <direccion IP> <# puerto>");
        exit(-1);
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("\nError, no se pudo abrir el socket.\n\n");
        exit(-1);
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(argv[1]);
    serv.sin_port = htons(atoi(argv[2]));

    if (connect(fd, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("\nNo se pudo realizar la conexión remota.\n");
        close(fd);
        exit(-1);
    }

    int opcion, placas;
    do {
        printf("\nSelecciona una opción:\n");
        printf("1. Solicitar viaje\n");
        printf("2. Solicitar estado\n");
        printf("3. Salir\n");
        printf("Opción: ");
        scanf("%d", &opcion);
        getchar();

        if (opcion == 1) {
            snprintf(request, sizeof(request), "{\"tipo\": \"viaje\"}");
            send(fd, request, strlen(request), 0);
            recepcion_datos(fd);

            // if (strcmp(datos_recibidos, "{\"tipo\": \"viaje_rechazado\"}") == 0) {
            if (strcmp(datos_recibidos, "{\"error\": \"No hay conductores\"}") == 0) {
                printf("\nNo hay conductores. Saliendo...\n");
                close(fd);
                exit(0);
            }
            
            // Simulación del viaje
            printf("\nSimulando viaje...\n");
            sleep(3);  // Tiempo de viaje simulado
            printf("\nViaje completado. Solicitando fin del viaje.\n");
            
            snprintf(request, sizeof(request), "{\"tipo\": \"viaje_terminado\", \"placas\": %d}", placas);
            printf("Placas del auto: %d\n", placas);
            send(fd, request, strlen(request), 0);
            recepcion_datos(fd);
            close(fd);
            exit(0);

        } else if (opcion == 2) {
            snprintf(request, sizeof(request), "{\"tipo\": \"estado\"}");
            send(fd, request, strlen(request), 0);
            recepcion_datos(fd);
            close(fd);
            exit(0);

        } else if (opcion == 3) {
            printf("Saliendo...\n");
        } else {
            printf("Opción no válida\n");
        }
        
    } while (opcion != 3);

    close(fd);
    return 0;
}
