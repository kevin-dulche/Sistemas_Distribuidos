 #
 # $Id: Makefile,v 1.1 1997/12/08 03:25:40 alc Exp $
 #
ARCH	= i386_linux2

CC	= gcc
CFLAGS	= -g -O2

MACHDEP_OBJS= getopt.o
MACHDEP_LIBS= -lm

all: raytrace.udp

include	../.common

getopt.o: $(TmkLIB)/support/getopt.c
	$(CC) $(CFLAGS) -c $(TmkLIB)/support/getopt.c
