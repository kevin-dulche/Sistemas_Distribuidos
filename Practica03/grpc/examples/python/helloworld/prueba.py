import socket

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

# Guarda la IP en una variable
local_ip = get_ip()
print(f"Tu IP en esta red es: {local_ip}")
