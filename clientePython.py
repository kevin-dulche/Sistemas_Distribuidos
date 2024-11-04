import socket
import string
import sys

ETHSIZE =  400


def main():
    request = "Dame el clima para hoy - Python - Dulche :)"

    try:
        IPserver   = sys.argv[1]
        portServer = int(sys.argv[2])
    except:
        print("\n\nSintaxis: cliente <direccion IP> <# puerto>\n\n ")
        sys.exit(0)


    print("\nAbriendo el socket...\n")

    # Podemos quitar el último argumento, ya que por default se elige proto=0
    socketFD = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM, proto=0)

    print("\nAsignando atributos al socket...\n")

    # No es necesario convertir a formato de red.
    serv = (IPserver, 
            portServer)

    # Usamos el método sendto del objeto socket para enviar un mensaje
    try:
        socketFD.sendto(request.encode(), serv)
        print("\nLa petición fue enviado al servidor.\n")
    except:
        print("\nProblemas al enviar la petición.\n")
        sys.exit(0)

    # Para recibir datos usamos el método recvfrom del socket
    # el método regresa en una tupla el mensaje recibido y la dirección del emisor.
    (buffer, servAddr) = socketFD.recvfrom(ETHSIZE)

    print("\nEstos son los datos enviados por el servidor:\n")
    print("'{}'\n\n".format(buffer.decode()))



if __name__ == "__main__":
    main()