# Copyright 2015 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""The Python implementation of the GRPC helloworld.Greeter server."""

from concurrent import futures
import logging

import grpc
import helloworld_pb2
import helloworld_pb2_grpc

import datetime
import socket
import math

class Greeter(helloworld_pb2_grpc.GreeterServicer):
    # Implementacion de los metodos siguientesFecha y sumaDiasFecha
    def siguienteFecha(self, request, context):
        print(type(request))
        day = request.day
        month = request.month
        year = request.year
        date = datetime.date(year, month, day)
        next_date = date + datetime.timedelta(days=1)
        return helloworld_pb2.DateReply(day=next_date.day, month=next_date.month, year=next_date.year)
    def sumaDiasFecha(self, request, context):
        day = request.date.day
        month = request.date.month
        year = request.date.year
        date = datetime.date(year, month, day)
        next_date = date + datetime.timedelta(days=request.days)
        return helloworld_pb2.DateReply(day=next_date.day, month=next_date.month, year=next_date.year)
    
    # Implementacion de los metodos dimeCentroDiana, disparaCannon y mejorDisparo
    nombreMejorDisparo = "None"
    distanciaMejorDisparo = 0.0

    def dimeCentroDiana(self, request, context):
        centro = 128
        return helloworld_pb2.Distancia(distancia=centro)
    def disparaCannon(self, request, context):
        nombreUsuario = request.nombreUsuario
        anguloRadianes = math.radians(request.anguloRadianes)
        velocidadMtrsSeg = request.velocidadMtrsSeg
        g = 9.8
        distancia = velocidadMtrsSeg * velocidadMtrsSeg * math.sin(2 * anguloRadianes) / g
        distanciaEntreCentroYDiana = distancia - 128
        if self.nombreMejorDisparo == "None" or abs(distanciaEntreCentroYDiana) < self.distanciaMejorDisparo:
            self.nombreMejorDisparo = nombreUsuario
            self.distanciaMejorDisparo = abs(distanciaEntreCentroYDiana)
        # Si la distanica es negativa, la diana esta del lado derecho, si es positiva, la diana esta del lado izquierdo
        return helloworld_pb2.Distancia(distancia=distanciaEntreCentroYDiana)
    def mejorDisparo(self, request, context):
        return helloworld_pb2.mejorDisparoReturn(nombreMejorDisparo=self.nombreMejorDisparo, distancia=self.distanciaMejorDisparo)

def get_ip():
    try:
        # Intenta conectarte a una dirección externa y captura la IP usada para la conexión
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))  # Dirección pública de Google
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception as e:
        return f"Error obteniendo IP: {e}"

def serve():
    ip_local = get_ip()
    port = "50051"
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    helloworld_pb2_grpc.add_GreeterServicer_to_server(Greeter(), server)
    server.add_insecure_port(str(ip_local) + ":" + port)
    server.start()
    print(f"Server started, listening on {ip_local}:{port}...")
    try:
        server.wait_for_termination()
    except KeyboardInterrupt:
        print("Server stopped")
        server.stop(0)

if __name__ == "__main__":
    logging.basicConfig()
    serve()
