#ifndef DEBUG_H
#define DEBUG_H

#define TRUE 1
#define FALSE 0

typedef int bool;


/* Interface to debugging routines. */


/* enable printing debug messages */
extern void DebugInit(char* flags);	 

/* is debug flag enabled? */
extern bool DebugIsEnabled(char flag); 	

/* print debug message if flag is enabled */
extern void DEBUG (char flag, char* format, ...);  	 
						

#endif /* DEBUG_H */
