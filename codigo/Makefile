TMK_PATH=/home/ubuntu/Desktop/Sistemas_Distribuidos/Tmk

LIBDIR=$(TMK_PATH)/lib.i386_linux2
INCLUDE_DIR=$(TMK_PATH)/include

# Biblioteca para enlazar a sus programas de TreadMarks
LIB=Tmk_udp

# Banderas que usarán al compilar
FLAGS = -m32 -no-pie

# Ejecutables que pueden generar con este Makefile
OBJECTS = test_tmk hola_tmk suma_tmk


all: $(OBJECTS)

test_tmk: test_tmk.c
	gcc $(FLAGS) test_tmk.c -o test_tmk -L$(LIBDIR) -I$(INCLUDE_DIR) -l$(LIB)
	
hola_tmk: hola_tmk.c
	gcc $(FLAGS) hola_tmk.c -o hola_tmk -L$(LIBDIR) -I$(INCLUDE_DIR) -l$(LIB)
	
suma_tmk: suma_tmk.c
	gcc $(FLAGS) suma_tmk.c -o suma_tmk -L$(LIBDIR) -I$(INCLUDE_DIR) -l$(LIB)
	
clean:
	rm -f $(OBJECTS)


