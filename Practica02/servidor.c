#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <time.h> // Para inicializar rand

#include "estructura.h"

#define N 5 // Cantidad de vehiculos

typedef struct {
    bool disponible;
    char placa[15];
    char tipo[15];
    int posicion_x;
    int posicion_y;
} vehiculo;

vehiculo vehiculos[N];
int ganancias = 0;
int viajes = 0;

bool vehiculos_inicializados = false;

void inicializar_vehiculos(vehiculo vehiculos[N]) {
    const char *tipos[] = {"UberPlanet", "UberXL", "UberBlack"};
    srand(time(NULL)); // Inicializa el generador de números aleatorios
    for (int i = 0; i < N; i++) {
        vehiculos[i].disponible = true;
        snprintf(vehiculos[i].placa, sizeof(vehiculos[i].placa), "%03d%c%c%c", rand() % 1000, 'A' + rand() % 26, 'A' + rand() % 26, 'A' + rand() % 26);
        strncpy(vehiculos[i].tipo, tipos[rand() % 3], sizeof(vehiculos[i].tipo) - 1);
        vehiculos[i].tipo[sizeof(vehiculos[i].tipo) - 1] = '\0'; // Asegurar terminación nula
        vehiculos[i].posicion_x = rand() % 100;
        vehiculos[i].posicion_y = rand() % 100;
    }
}

InfoAuto *solicitarviaje_1_svc(PosicionPasajero *PosicionPasajero, struct svc_req *rqstp) {
    
    if (!vehiculos_inicializados) {
        inicializar_vehiculos(vehiculos);
        vehiculos_inicializados = true;
    }
    printf("Solicitud de viaje recibida\n");
    printf("Posición del pasajero: (%d, %d)\n", PosicionPasajero->pos_x, PosicionPasajero->pos_y);
    // Verificar si hay vehículos disponibles
    int vehiculos_disponibles = 0;
    int vehiculos_disponibles_indices[N];

    for (int i = 0; i < N; i++) {
        if (vehiculos[i].disponible) {
            vehiculos_disponibles_indices[vehiculos_disponibles] = i;
            vehiculos_disponibles++;
        }
    }

    // Si no hay vehículos disponibles
    if (vehiculos_disponibles == 0) {
        return NULL;
    }

    printf("Vehículos disponibles: %d\n", vehiculos_disponibles);
    // Encontrar el vehículo más cercano
    int vehiculo_mas_cercano = 0;
    double distancia_minima = DBL_MAX;

    for (int i = 0; i < vehiculos_disponibles; i++) {
        double distancia = sqrt(pow(vehiculos[vehiculos_disponibles_indices[i]].posicion_x - PosicionPasajero->pos_x, 2) +
                                pow(vehiculos[vehiculos_disponibles_indices[i]].posicion_y - PosicionPasajero->pos_y, 2));
        if (distancia < distancia_minima) {
            distancia_minima = distancia;
            vehiculo_mas_cercano = vehiculos_disponibles_indices[i];
        }
    }

    printf("Vehículo más cercano: %s\n", vehiculos[vehiculo_mas_cercano].placa);
    // Preparar y retornar la información del viaje
    InfoAuto *info_auto = (InfoAuto *)malloc(sizeof(InfoAuto));
    printf("Vehículo asignado\n");

    info_auto->pos_x = vehiculos[vehiculo_mas_cercano].posicion_x;
    info_auto->pos_y = vehiculos[vehiculo_mas_cercano].posicion_y;

    printf("Tipo de vehículo: %s\n", vehiculos[vehiculo_mas_cercano].tipo);
    
    if (strcmp(vehiculos[vehiculo_mas_cercano].tipo, "UberPlanet") == 0) {
        info_auto->tarifa = 10;
    } else if (strcmp(vehiculos[vehiculo_mas_cercano].tipo, "UberXL") == 0) {
        info_auto->tarifa = 15;
    } else if (strcmp(vehiculos[vehiculo_mas_cercano].tipo, "UberBlack") == 0) {
        info_auto->tarifa = 25;
    }

    printf("Tarifa: %d\n", info_auto->tarifa);
    printf("vehiculo_mas_cercano: %d\n", vehiculo_mas_cercano);

    strcpy(info_auto->tipo, vehiculos[vehiculo_mas_cercano].tipo);

    strcpy(info_auto->placa, vehiculos[vehiculo_mas_cercano].placa);

    vehiculos[vehiculo_mas_cercano].disponible = false;

    printf("Vehículo asignado: %s\n", info_auto->placa);

    return info_auto;
}

void *terminarviaje_1_svc(TerminarViajeArgs *TerminarViajeArgs, struct svc_req *rqstp) {
    printf("Terminar viaje\n");
    printf("Placa: %s\n", TerminarViajeArgs->placa);
    printf("Posición final: (%d, %d)\n", TerminarViajeArgs->posicion_final.pos_x, TerminarViajeArgs->posicion_final.pos_y);
    printf("Costo del viaje: %d\n", TerminarViajeArgs->costo_viaje);

    double distancia_del_viaje;
    bool encontrado = false;
    for (int i = 0; i < N; i++) {
        if (strcmp(vehiculos[i].placa, TerminarViajeArgs->placa) == 0) {
            distancia_del_viaje = sqrt(pow(TerminarViajeArgs->posicion_final.pos_x - vehiculos[i].posicion_x, 2) +
                                            pow(TerminarViajeArgs->posicion_final.pos_y - vehiculos[i].posicion_y, 2));
            vehiculos[i].disponible = true;
            vehiculos[i].posicion_x = TerminarViajeArgs->posicion_final.pos_x;
            vehiculos[i].posicion_y = TerminarViajeArgs->posicion_final.pos_y;
            encontrado = true;
            break;
        }
    }

    if (!encontrado) {
        printf("Error: Vehículo con placa %s no encontrado.\n", TerminarViajeArgs->placa);
    }

    int costo_viaje = distancia_del_viaje * TerminarViajeArgs->costo_viaje;

    ganancias += costo_viaje;
    viajes++;
}

InfoServicio *estadoservicio_1_svc(struct svc_req *rqstp) {
    static InfoServicio info_servicio;
    info_servicio.ganancias = ganancias;
    info_servicio.viajes = viajes;
    info_servicio.vehiculos_disponibles = 0;

    for (int i = 0; i < N; i++) {
        if (vehiculos[i].disponible) {
            info_servicio.vehiculos_disponibles++;
        }
    }

    return &info_servicio;
}
