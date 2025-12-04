 #
 # $Id: Makefile,v 1.1 1998/09/13 06:11:04 alc Exp $
 #
ARCH	= i386_linux2

CC	= gcc
CFLAGS	= -g -O2

MACHDEP_OBJS= getopt.o
MACHDEP_LIBS=

all: tsp.udp

include ../.common

getopt.o: $(TmkLIB)/support/getopt.c
	$(CC) $(CFLAGS) -c $(TmkLIB)/support/getopt.c
