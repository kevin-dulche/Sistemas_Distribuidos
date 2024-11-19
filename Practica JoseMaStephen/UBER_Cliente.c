#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "uber.h"
#include <math.h>
#include <time.h>


double calcularDistancia (Posicion origen, Posicion destino) {
    return ( sqrt ( pow ((destino.x - origen.x), 2) + pow ((destino.y - origen.y), 2) ) );

} 

void clientePasajero(char *host) {
    srand (time(NULL));

    CLIENT *client;
    InfoAuto *result;
    struct Posicion origen, destino; 
    char *UBER;

    if ((client = clnt_create(host, APP_UBER, UBER_DISPLAY, "tcp")) == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }

    origen.x = rand() % 51;
    origen.y = rand() % 51;
    destino.x = rand() % 51;
    destino.y = rand() % 51;

    system("clear");
    printf("Pasajero: Posición de origen: (%d, %d)\n", origen.x, origen.y);
    
    printf("Pasajero: Posición de destino: (%d, %d)\n", destino.x, destino.y);

    // Llamar al servicio SolicitarViaje
    result = solicitarviaje_1(&origen, client);

    if (result == NULL) {
        fprintf(stderr, "Error al solicitar viaje.\n");
        exit(1);
    }

    if (result->disponible == 0) {
        printf("Pasajero: No se encontraron autos disponibles. Fin del programa.\n");
        exit(0);
    }

    switch ( result->tipoUber ) {
        case 1: UBER = "UberPlanet";
            break;
        case 2: UBER = "UberXL";
            break;
        case 3: UBER = "UberBlack";
            break;
    }

    printf("Pasajero: Auto asignado - Placas: %s, Tipo Uber: %s, Tarifa: %d\n",
            result->placa, UBER, result->tarifa);

    double distancia = calcularDistancia(origen, result->posicion);
    sleep((unsigned int)distancia);

    // Llamar al servicio TerminarViaje
    ViajeInfo viajeInfo;

    viajeInfo.posicion.x = destino.x;
    viajeInfo.posicion.y = destino.y;
    viajeInfo.costoViaje = result->tarifa;
    viajeInfo.placa = result->placa;
    // viajeInfo.placa = strdup(result->placa); // Usar strdup para asignar una copia de la cadena

    terminarviaje_1(&viajeInfo, client);

    printf("Pasajero: Viaje terminado. Fin del programa.\n");
    
    free(viajeInfo.placa);

    clnt_destroy(client);
}

void clienteAdministrador(char *host) {
    CLIENT *client;
    InfoServicio *result;

    if ((client = clnt_create(host, APP_UBER, UBER_DISPLAY, "tcp")) == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }

    system ("clear");
    while (1) {
        // Llamar al servicio EstadoServicio cada 2 segundos
        result = estadoservicio_1(NULL, client);

        printf("\nAdministrador: Estado del servicio\n");
        printf("Número de viajes realizados: %d\n", result->numViajes);
        printf("Ganancia total: %d\n", result->gananciaTotal);
        printf("Número de autos libres: %d\n", result->numAutosLibres);

        sleep(2);
    }
    clnt_destroy(client);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <host> <rol: pasajero o administrador>\n", argv[0]);
        exit(1);
    }

    char *host = argv[1];
    char *rol = argv[2];

    if (strcmp(rol, "pasajero") == 0) {
        clientePasajero(host);
    } else if (strcmp(rol, "administrador") == 0) {
        clienteAdministrador(host);
    } else {
        fprintf(stderr, "Rol no reconocido. Use 'pasajero' o 'administrador'.\n");
        exit(1);
    }

    return 0;
}