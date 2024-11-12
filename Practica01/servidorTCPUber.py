import socket
import json
import random
import time

HOST = '127.0.0.1'
PORT = 8000
NUM_AUTOS = 2  # Puedes cambiar el número de autos

# Inicialización de los datos del servidor
autos_disponibles = [True] * NUM_AUTOS
viajes_realizados = 0
ganancia_total = 0

def obtener_estado():
    """Regresa el estado actual del servidor."""
    return json.dumps({"viajes_realizados": viajes_realizados, "ganancia_total": ganancia_total})

def solicitar_viaje():
    """Proceso de solicitud de viaje."""
    global viajes_realizados, ganancia_total
    for i in range(NUM_AUTOS):
        if autos_disponibles[i]:
            autos_disponibles[i] = False
            costo_viaje = random.randint(10, 100)
            viajes_realizados += 1
            ganancia_total += costo_viaje
            return json.dumps({"placas": i, "costo_viaje": costo_viaje})
    return json.dumps({"error": "No hay conductores"})

def terminar_viaje(placas):
    """Proceso para terminar un viaje y liberar un auto."""
    if 0 <= placas < NUM_AUTOS:
        autos_disponibles[placas] = True
        return "Viaje terminado"
    return "Placas inválidas"

def iniciar_servidor():
    """Función principal para iniciar el servidor."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        print("Servidor iniciado y en espera de conexiones...")

        while True:
            conn, addr = server_socket.accept()
            print("Conexión establecida con:", addr)  # IP y puerto del cliente
            with conn:
                while True:  # Mantener la conexión con el cliente abierta para múltiples mensajes
                    data = conn.recv(1024).decode()
                    
                    if not data:
                        print("Cliente desconectado.")
                        break  # Terminar la conexión con este cliente y esperar a otro
                    
                    solicitud = json.loads(data)
                    print("Solicitud recibida:", solicitud)
                    respuesta = ""

                    if solicitud["tipo"] == "estado":
                        respuesta = obtener_estado()
                    elif solicitud["tipo"] == "viaje":
                        respuesta = solicitar_viaje()
                    elif solicitud["tipo"] == "viaje_terminado":
                        placas = solicitud.get("placas", -1)
                        respuesta = terminar_viaje(placas)
                    
                    conn.sendall(respuesta.encode())


if __name__ == "__main__":
    iniciar_servidor()
