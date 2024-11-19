#include <stdio.h>
#include <math.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "uber.h"

#define NUM_AUTOS 8 

#define UBER_PLANET 1 
#define UBER_XL 2       
#define UBER_BLACK 3 

const int UberPlanet = 10;
const int UberXL = 15;
const int UberBlack = 25;

struct InfoAuto autos[NUM_AUTOS];
static int viajesRealizados = 0;
static int ganancia_total = 0;
bool isFirstTime = true;

int posicionesAleatorias () { 
    return ( 0 + (rand() % 51) );
}

void inicializarAutos () {
    srand (time (NULL));
    printf ("\n\n\t\tReady, UBER Cars? Go!\n");

    for ( int i = 0; i < NUM_AUTOS; i++ ) {
        autos[i].disponible = 1; 
        autos[i].posicion.x = posicionesAleatorias ();
        autos[i].posicion.y = posicionesAleatorias ();
        autos[i].tipoUber = UBER_PLANET + (rand () % UBER_BLACK);
        autos[i].tarifa = 0;

        char plate[7];
        
        sprintf (plate, "%d%d%c%c%c", i, rand() % 1000,
                'A' + rand() % 26, 'A' + rand() % 26, 'A' + rand() % 26);

        autos[i].placa = strdup (plate);
    } 
} 

double calcularDistancia (Posicion origen, Posicion destino) {
    return ( sqrt ( pow ((destino.x - origen.x), 2) + pow ((destino.y - origen.y), 2) ) );
}

int calcularTarifa (int tipoUBER, int kilometro) {
    switch ( tipoUBER ) {
        case 1: return (UberPlanet * kilometro);
            break;
        case 2: return (UberXL * kilometro);
            break;
        case 3: return (UberBlack * kilometro);
            break;
    }
} 

InfoAuto *solicitarviaje_1_svc(struct Posicion *posicionPasajero, struct svc_req *rqstp) {
    system ("clear");
    printf ("\n\t\t--------------- Serious UBER Application --------------- \n");
    printf ("\n\nLocalized Position! Passenger is in (%d, %d) Coordinates.", posicionPasajero->x, posicionPasajero->y);

    if ( isFirstTime == true ) {
        inicializarAutos ();
        isFirstTime = false;
    }

	static InfoAuto autoDisponible; 
    short int available = 0; 
    bool isAvailable = false;
    double distanciaMinima = INFINITY;

    for ( int i = 0; i < NUM_AUTOS; i++ ) {
        if ( autos[i].disponible == 1 ) {
            double distancia = calcularDistancia (autos[i].posicion, *posicionPasajero);
            if (distancia < distanciaMinima) {
                distanciaMinima = distancia;
                available = i; 
                isAvailable = true;
                break; 
            }
        }
        isAvailable = false;
    }

    if ( isAvailable == true) {
        double distancia = calcularDistancia (autos[available].posicion, *posicionPasajero);
        int tarifa = calcularTarifa (autos[available].tipoUber, (int)distancia);

        autos[available].tarifa = tarifa; 
        autoDisponible = autos[available];
        autos[available].disponible = 0;
        viajesRealizados++;

    } else { 
        autoDisponible.disponible = 0; 
    }
    
    return &autoDisponible;
}

void * terminarviaje_1_svc(struct ViajeInfo *ElPasajero, struct svc_req *rqstp) {
	static char * result;

    for ( int i = 0; i < NUM_AUTOS; i++ ) {
        if ( strcmp ( autos[i].placa, ElPasajero->placa) == 0 ) {
            autos[i].disponible = 1;
            autos[i].posicion.x = ElPasajero->posicion.x;
            autos[i].posicion.y = ElPasajero->posicion.y;
            ganancia_total += ElPasajero->costoViaje;
            printf ("\n\n\t>>El Auto con las Placas %s es Ahora DISPONIBLE<<\n", ElPasajero->placa);
            break;
        }
    }

	return (void *) &result;
}

InfoServicio * estadoservicio_1_svc(void *argp, struct svc_req *rqstp) {
	static InfoServicio  estado;
    int autosLibres = 0;

    estado.numViajes = viajesRealizados;
    estado.gananciaTotal = ganancia_total;

    for ( int i = 0; i < NUM_AUTOS; i++ ) {
        if ( autos[i].disponible == 1 ) {
            autosLibres++;
        }
    }

    estado.numAutosLibres = autosLibres;

	return &estado;
}
