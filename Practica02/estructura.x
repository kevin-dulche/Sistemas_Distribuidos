/* Archivo corregido */
struct PosicionPasajero {
    int pos_x;
    int pos_y;
};

struct PosicionFinal {
    int pos_x;
    int pos_y;
};

struct InfoAuto {
    int pos_x;
    int pos_y;
    char tipo[15];  /* Máximo 15 caracteres */
    int tarifa;
    char placa[15]; /* Máximo 15 caracteres */
};

struct InfoServicio {
    int ganancias;
    int viajes;
    int vehiculos_disponibles;
};

struct TerminarViajeArgs {
    PosicionFinal posicion_final;
    int costo_viaje;
    char placa[15]; /* Máximo 15 caracteres */
};

program CALC_PROG { /* Número de programa */
    version CALC_VER { /* Número de versión */
        InfoAuto solicitarViaje(PosicionPasajero) = 1;  /* Versión 1 */
        void terminarViaje(TerminarViajeArgs) = 2;      /* Versión 2 */
        InfoServicio estadoServicio(void) = 3;         /* Versión 3 */
    } = 1;
} = 0x20000199;