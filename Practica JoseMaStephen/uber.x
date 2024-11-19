
struct Posicion {
    int x;
    int y;
};

struct InfoAuto {
    int disponible;
    Posicion posicion;
    int tipoUber;
    int tarifa;
    string placa<7>;
};

struct InfoServicio {
    int numViajes;
    int numAutosLibres;
    int gananciaTotal;
};

struct ViajeInfo {
    Posicion posicion;
    int costoViaje;
    string placa<7>;
};

program APP_UBER {
    
    version UBER_DISPLAY {
        InfoAuto SolicitarViaje (struct Posicion) = 1;
        void TerminarViaje (struct ViajeInfo) = 2;
        InfoServicio EstadoServicio (void) = 3;        
    } = 1;

} = 0x20000004;
