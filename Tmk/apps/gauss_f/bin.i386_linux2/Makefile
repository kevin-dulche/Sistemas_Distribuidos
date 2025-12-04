 #
 # $Id: Makefile,v 1.3 1999/08/03 04:41:42 alc Exp $
 #
ARCH	= i386_linux2

CC	= gcc
CFLAGS	= -O2

FC	= g77
FFLAGS  = -O2

MACHDEP_OBJS= common_symbol.o getopt.o
MACHDEP_LIBS=

all: Gauss.udp

include ../.common

common_symbol.o: $(TmkLIB)/support/common_symbol.s
	$(AS) -o $@ $(TmkLIB)/support/common_symbol.s

getopt.o: $(TmkLIB)/support/getopt.c
	$(CC) $(CFLAGS) -c $(TmkLIB)/support/getopt.c
