#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // Para sleep()
#include <rpc/rpc.h>
#include "estructura.h"

int main(int argc, char const *argv[])
{
    const char *host;
    
    if (argc < 3) {
        printf("usage: %s IP_ADDRESS_PORT pasajero/administrador\n", argv[0]);
        exit(1);
    }
    host = argv[1];

    if (strcmp(argv[2], "pasajero") == 0) {
        printf("Soy un pasajero\n");

        // Declarar variables
        InfoAuto *result_1;
        PosicionPasajero *solicitarviaje_1_arg;
        solicitarviaje_1_arg = (PosicionPasajero *)malloc(sizeof(PosicionPasajero));
        solicitarviaje_1_arg->pos_x = rand() % 100;
        solicitarviaje_1_arg->pos_y = rand() % 100;

        printf("Posición del pasajero: (%d, %d)\n", solicitarviaje_1_arg->pos_x, solicitarviaje_1_arg->pos_y);

        // Crear cliente RPC
        CLIENT *clnt;
        clnt = clnt_create(host, CALC_PROG, CALC_VER, "tcp");
        if (clnt == NULL) {
            clnt_pcreateerror(host);
            exit(1);
        }
        printf("Cliente creado\n");

        // Llamar a la función RPC para solicitar un viaje
        result_1 = solicitarviaje_1(&solicitarviaje_1_arg, clnt);
        if (result_1 == NULL) {
            clnt_perror(clnt, "call failed o ya no hay ubers disponibles");
        }

        sleep(5);  // Dormir por 5 segundos (simulación de viaje)

        //? Hasta aquí, el pasajero ya tiene un vehículo asignado

        printf("Vehículo asignado: %s\n", result_1->placa);

        // Terminar el viaje
        TerminarViajeArgs *terminarviaje_1_arg = (TerminarViajeArgs *)malloc(sizeof(TerminarViajeArgs));
        // Asignar memoria para la placa
        
        strcpy(terminarviaje_1_arg->placa, result_1->placa);

        terminarviaje_1_arg->posicion_final.pos_x = rand() % 100;
        terminarviaje_1_arg->posicion_final.pos_y = rand() % 100;

        // Calcular costo del viaje (suponiendo que se mide la distancia)
        terminarviaje_1_arg->costo_viaje = result_1->tarifa;

        printf("Costo del viaje: %d\n", terminarviaje_1_arg->costo_viaje);
        printf("Posición final: (%d, %d)\n", terminarviaje_1_arg->posicion_final.pos_x, terminarviaje_1_arg->posicion_final.pos_y);
        printf("Placa: %s\n", terminarviaje_1_arg->placa);

        
        // Realizar llamada para terminar el viaje
        terminarviaje_1(terminarviaje_1_arg, clnt);
        sleep(1);  // Dormir por 1 segundo

    } else if (strcmp(argv[2], "administrador") == 0) {
        printf("Soy un administrador\n");
    } else {
        printf("Tipo de usuario no reconocido\n");
        exit(1);
    }

    return 0;
}