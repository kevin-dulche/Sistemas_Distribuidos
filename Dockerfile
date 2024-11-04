# Usa una imagen base de Debian con las herramientas necesarias
FROM debian:latest

# Instala gcc y elimina los archivos de caché de instalación
RUN apt-get update && apt-get install -y gcc && rm -rf /var/lib/apt/lists/*

# Crea un directorio de trabajo en el contenedor
WORKDIR /usr/src/app

# Copia el archivo servidorTCP.c al contenedor
COPY servidorTCP.c .

# Compila el archivo servidorTCP.c
RUN gcc -o servidorTCP servidorTCP.c

# Expone el puerto 8000 para el servidor TCP
EXPOSE 8000

# Comando para ejecutar el servidor
CMD ["./servidorTCP"]