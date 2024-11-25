/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// Package main implements a client for Greeter service.
package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"math"
	"bufio"
	"strconv"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/protobuf/types/known/emptypb"

	pb "google.golang.org/grpc/examples/helloworld/helloworld" // Reemplaza con tu propio paquete generado
)

func main() {
	// Validar los argumentos
	if len(os.Args) != 3 {
		fmt.Println("Uso: go run practica3Client/main.go <server_ip> <server_port>")
		os.Exit(1)
	}

	// Leer la IP y el puerto de los argumentos
	ip := os.Args[1]
	port := os.Args[2]

	// Construir la dirección del servidor
	addr := fmt.Sprintf("%s:%s", ip, port)

	// Configurar la conexión al servidor gRPC
	conn, err := grpc.NewClient(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("No se pudo conectar: %v", err)
	}
	defer conn.Close()

	// Crear el cliente para el servicio gRPC
	client := pb.NewGreeterClient(conn)

	// Crear el contexto
	ctx := context.Background()

	// Llamar al método DimeCentroDiana
	r1, err := client.DimeCentroDiana(ctx, &emptypb.Empty{})
	if err != nil {
		log.Fatalf("Error en DimeCentroDiana: %v", err)
	}
	log.Printf("La diana se encuentra a %.2f metros del cannon\n", r1.GetDistancia())

	// Llamar al método DisparaCannon en un ciclo for de 5 iteraciones

	// Ingresa el nombre del usuario
	var nombre string
	reader := bufio.NewReader(os.Stdin)
	fmt.Print("Ingresa tu nombre: ")
	nombre, _ = reader.ReadString('\n')
	nombre = nombre[:len(nombre)-1] // Eliminar el salto de línea
	
	for i := 0; i < 5; i++ {
		// Ingresa el angulo del disparo
		fmt.Printf("Ingresa el angulo del disparo %d: ", i+1)
		input, _ := reader.ReadString('\n') // Leer toda la línea
		input = input[:len(input)-1]        // Eliminar el salto de línea

		// Convertir la entrada en un número
		angulo64, err := strconv.ParseInt(input, 10, 32)
		angulo := int32(angulo64)
		if err != nil {
			fmt.Println("Por favor ingresa un número válido.")
			i-- // Repetir el ciclo si la entrada es inválida
			continue
		}

		// Ingresa la velocidad del disparo en m/s
		fmt.Printf("Ingresa la velocidad del disparo %d: ", i+1)
		input, _ = reader.ReadString('\n') // Leer toda la línea
		input = input[:len(input)-1]        // Eliminar el salto de línea

		// Convertir la entrada en un número
		velocidad, err := strconv.ParseFloat(input, 64)
		if err != nil {
			fmt.Println("Por favor ingresa un número válido.")
			i-- // Repetir el ciclo si la entrada es inválida
			continue
		}

		// Llamar al método DisparaCannon
		r2, err := client.DisparaCannon(ctx, &pb.DisparaCannonArgs{NombreUsuario: nombre, AnguloRadianes: angulo, VelocidadMtrsSeg: velocidad})
		if err != nil {
			log.Fatalf("Error en DisparaCannon: %v", err)
		}
		log.Printf("Disparo %d: El disparo quedo a %f metros de distancia de la diana.\n", i+1, r2.GetDistancia())
		var distancia float64 = r2.GetDistancia()
		absDistancia := math.Abs(distancia)
		
		if absDistancia <= 1 {
			log.Printf("Felicidades, has dado en el blanco!\n")
			break
		}

		// Llamar al método MejorDisparo
		r3, err := client.MejorDisparo(ctx, &emptypb.Empty{})
		if err != nil {
			log.Fatalf("Error en MejorarDisparo: %v", err)
		}
		log.Printf("El mejor disparo hasta el momento fue de %s con una distancia de %.2f metros.\n", r3.GetNombreMejorDisparo(), r3.GetDistancia())
	}
}