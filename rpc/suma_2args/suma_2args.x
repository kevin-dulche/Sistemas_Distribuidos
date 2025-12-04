/*
 * Acá se pasarán por separado dos argumentos para la función remota.
 */

program CALC_PROG {				    /* número de programa */
	version CALC_VER {		        /* número de versión  */
		int suma(int, int) = 1;		/* función remota */
		int multiplicacion(int, int) = 2;
	} = 1;
} = 0x20000199;
