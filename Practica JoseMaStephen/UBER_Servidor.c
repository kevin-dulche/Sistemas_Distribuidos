
/*
    - Author: Stephen Luna Ramírez.
    - Subject: Sistemas Distribuidos.
    - Practice II. Llamadas a Procedimientos Remotos [RPC].
    - Corresponding Component: Servidor.

    - Description:
                Semejante al anterior práctica primera, el Servidor ofrecerá diversos autos UBER'S
                a la petición del Cliente (quien desea realizar un viaje), lo cual el mismo Servidor
                tendrá estas siguientes responsabilidades: 
                    
                    1. Verificación del Viaje (estado del auto).
                    2. Agregar posiciones aleatorias como también las del Cliente para cada auto asociado.
                    3. Ofrecer la disponibilidad del auto, una vez que el Cliente haya finalizado el viaje.
                    4. Otorgar el Estado del Servicio cuyo despliegue será visto sólo por el Cliente.
                
                De igual manera como la anterior, el Servidor terminará su respectiva ejecución cuando
                la llamada o interrupción haya sido con la siguiente tecla "CTRL + C", fuera de ella,
                seguirá ejecutando (esperando alguna petición por parte del Cliente) de manera infinita.


*/

#include <stdio.h>
#include <math.h> // Necesario para determinar o calcular la distancia entre dos puntos.
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "uber.h"
#include <string.h> // Para evitar errores a una cadena de caracteres, era necesario usar stdrup().

// Variables Globales y Constantes
#define NUM_AUTOS 8 // La cantidad o número de autos en total.

// Establecemos los tipos de UBER y sus correspondientes precios:
#define UBER_PLANET 1 
#define UBER_XL 2       
#define UBER_BLACK 3 

const int UberPlanet = 10;
const int UberXL = 15;
const int UberBlack = 25;

// Defino mis Variables Globales a ocupar
struct InfoAuto autos[NUM_AUTOS]; // El arreglo tipo 'struct' para los autos.
static int viajesRealizados = 0; // Un contador vivo (actualizable) para el total de los viajes realizados.
static int ganancia_total = 0; // Otro mismo para establecer (con el tiempo) la ganancia total a los viajes hechos.
bool isFirstTime = true; // Una variable tipo 'BOOL' para ejecutar por vez primera la B.D.


/*      /-/-/ Funciones SÓLO Para el Servidor /-/-/        */

// Genera, aleatoriamente, el número de las posiciones entre el 0 y 51. Es decir: [0,51]
int posicionesAleatorias () { return ( 0 + (rand() % 51) ); }

// Establece a todos los autos a su estado "libre", el número de sus posiciones, el tipo y las placas
void inicializarAutos () {
    srand (time (NULL)); // Generamos la semilla para establecer números aleatorios (las posiciones).
    printf ("\n\n\t\tReady, UBER Cars? Go!\n");

    for ( int i = 0; i < NUM_AUTOS; i++ ) {
        autos[i].disponible = 1; // Habilitamos disponibilidad para todos los autos

        // De Manera Aleatoria, Iniciamos a Generar las Posiciones (x,y) Para Cada Auto Correspondiente:
        autos[i].posicion.x = posicionesAleatorias ();
        autos[i].posicion.y = posicionesAleatorias ();

        autos[i].tipoUber = UBER_PLANET + (rand () % UBER_BLACK); // Generamos, igualmente, de forma aleatoria, los tipos de uber
        autos[i].tarifa = 0; // Como inicio, la tarifa para cada auto será de $0, esto para almacenar posteriormente sus precios correspondientes.

        // Generamos la Placa Aleatoria [3 dígitos + 3 letras]:
        char plate[7]; // Dado que no funcionaba 'snprintf' para autos[i].placa, 
                    // creamos una nueva cadena de caracteres para almacenar cada información para al objeto "autos".
        
        sprintf (plate, "%d%d%c%c%c", i, rand() % 1000,
                'A' + rand() % 26, 'A' + rand() % 26, 'A' + rand() % 26);

        autos[i].placa = strdup (plate); // Para evitar cualquier símbolo extraño, utilizamos dicha función 'strdup()'

    } // Fin del Ciclo FOR

} // Fin a la función inicializarAutos

// Determina la distancia entre dos puntos mediante la fórmula Euclidiana o Pitágorica
double calcularDistancia (Posicion origen, Posicion destino) {
    // Dado a la fórmula de Pitágoras Donde "distancia = RAÍZ ( (x2-x1)^2 + (y2-y1)^2)", Tenemos:
    return ( sqrt ( pow ((destino.x - origen.x), 2) + pow ((destino.y - origen.y), 2) ) );

} // Fin de la Función calcularDistancia

// Genera, a partir del tipo de UBER y el valor en kilómetros (las posiciones del Cliente), la tarifa para cada auto
int calcularTarifa (int tipoUBER, int kilometro) {
    // Dado al tipo de UBER, calculamos su precio por kilómetro (distancia) y generamos la tarifa correspondiente
    switch ( tipoUBER ) {
        case 1: return (UberPlanet * kilometro); // $10 x km
            break;
        case 2: return (UberXL * kilometro); // $15 X km
            break;
        case 3: return (UberBlack * kilometro); // $25 x km
            break;
    }

} // Fin de la Función calcularTarifa


/*      --- SERVICIOS [Funciones Para el Cliente] ---        */
    
    // Ofrece al Cliente Solicitud al Viaje (si lo hay)
InfoAuto *solicitarviaje_1_svc(struct Posicion *posicionPasajero, struct svc_req *rqstp) {
    system ("clear");
    printf ("\n\t\t--------------- Serious UBER Application --------------- \n");
    printf ("\n\nLocalized Position! Passenger is in (%d, %d) Coordinates.", posicionPasajero->x, posicionPasajero->y);

    // Si la invocación o llamado al método es la primera vez
    if ( isFirstTime == true ) {
        inicializarAutos (); // Genera la inicialización de la B.D. tipo autos.
        isFirstTime = false; // Dado que sólo es la primera vez, finalizamos su proceso de inicialización.
    }

    // Variable Locales del Propio Método
	static InfoAuto autoDisponible; // El objeto con sus respectivos atributos (campos).
    short int available = 0; // La posición del arreglo "autos" si alguno de ellos es libre
    bool isAvailable = false; // Simulación.
    double distanciaMinima = INFINITY; // Generamos un límite infinito para hallar distancias cercanas posibles.

    // El Servidor Verificará si Alguno de los Autos Todos es Dispobible
    for ( int i = 0; i < NUM_AUTOS; i++ ) {
            // Si el auto dado es realmente disponible
        if ( autos[i].disponible == 1 ) {
            // Calculamos, mediante la fórmula Euclidiana, la distancia cercana entre el UBER y el Cliente
            double distancia = calcularDistancia (autos[i].posicion, *posicionPasajero);
            
            // Si la distancia del Cliente es en verdad cercana al auto UBER
            if (distancia < distanciaMinima) {
                distanciaMinima = distancia; // Intercambiamos
                available = i; // Por tanto, el auto sí es disponible y, entretanto, guardamos su índice a la variable correspondiente.
                isAvailable = true;
                break; // Dado que ha encontrado uno 'libre', salimos al momento.
            }

        } // Fin del IF-ELSE externo
        isAvailable = false; // Caso adverso, nunca fue disponible el auto dado.

    } // Fin Ciclo FOR

        // Si el Auto Ofrecido sí fue Disponible
    if ( isAvailable == true) {
        // Calculamos Distancias Para Conseguir el Kilómetro Generado
        double distancia = calcularDistancia (autos[available].posicion, *posicionPasajero);
        
        // Finalmente, dado al tipo de UBER y el kilómetro generado, calculamos la tarifa total al Cliente
        int tarifa = calcularTarifa (autos[available].tipoUber, (int)distancia);

        autos[available].tarifa = tarifa; // Guardamos dicha información (precio total) al objeto respectivo.

        // Para concluir, guardamos toda la información (datos del objeto) al correspondiente y nuevo objeto del mismo 
        autoDisponible = autos[available];
        autos[available].disponible = 0; // Ahora el auto es ocupado para ese tiempo transcurrido.

        viajesRealizados++; // Incrementamos el número de viaje realizado (Hasta ese Momento, Claro).

    } else { // Si NO lo fue, esto es, NO hay Ningún Auto Disponible en ese Poco Lapso
        autoDisponible.disponible = 0; // Configuramos el resultado para indicar que el auto siempre no fue disponible.

    } // Fin Condición IF-ELSE
    
    return &autoDisponible; // Retornamos su valor (información) al Cliente correspondiente.

} // Fin al Servicio *solicitarviaje_1_svc

    // El Servidor mismo aceptará la respuesta del Cliente para habilitar la disponibilidad al auto antes otorgado
void * terminarviaje_1_svc(struct ViajeInfo *ElPasajero, struct svc_req *rqstp) {
	static char * result;

    // El Servidor Registrará, con la Correspondida Placa, el Estado del Auto (estado "libre").
    for ( int i = 0; i < NUM_AUTOS; i++ ) {
            // Si el Número de Placas es Semejante al del Auto Ofrecido
        if ( strcmp ( autos[i].placa, ElPasajero->placa) == 0 ) {
            autos[i].disponible = 1; // Deshabilitamos su estado "Ocupado" a su estado "Disponible".
            // El Servidor cambiará la posición anterior a otra nueva posición cuya misma es la del Cliente
            autos[i].posicion.x = ElPasajero->posicion.x;
            autos[i].posicion.y = ElPasajero->posicion.y;
            ganancia_total += ElPasajero->costoViaje; // Agregamos el costo del viaje a la suma evolutiva (i.e la Ganancia Total).
            // Enviamos respuesta al Cliente respectivo
            printf ("\n\n\t>>El Auto con las Placas %s es Ahora DISPONIBLE<<\n", ElPasajero->placa);
            
            break; // Dado que las placas y el auto cambió su estado (esto es, a estado "libre") salimos nuevamente del ciclo.

        } // Fin Condición IF-ELSE

    } // Fin del Ciclo FOR

	return (void *) &result;

} // Fin al Servicio * terminarviaje_1_svc

    // El Servidor ofrecerá el Estado del Servicio hasta ese momento
InfoServicio * estadoservicio_1_svc(void *argp, struct svc_req *rqstp) {
	static InfoServicio  estado;
    int autosLibres = 0; // El registro continuo a la suma cuyos autos son disponibles.

    // El Servidor Establecerá los Correspondientes Valores (Datos) Para la Vista del Cliente
    estado.numViajes = viajesRealizados;
    estado.gananciaTotal = ganancia_total;

    // El Servidor Propio Sumamará Quién de los autos es NO Ocupado
    for ( int i = 0; i < NUM_AUTOS; i++ ) {
            // Si entre todos los autos existen autos "libres"
        if ( autos[i].disponible == 1 ) {
            autosLibres++; // Registramos la suma a la variable local "autosLibres".
        }
        
    } // Fin del Ciclo FOR

    estado.numAutosLibres = autosLibres; // Finalmente, agregamos este valor (el total de autos "libres") al objeto.

	return &estado; // Enviamos al Cliente el correspondido Estado de Servicio (Interfaz del Usuario).
    
} // Fin al Servicio * estadoservicio_1_svc
