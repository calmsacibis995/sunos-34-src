static char     sccsid[] = "@(#) vectors.h 1.1 86/09/25 Copyright Sun Micro";
 /*
  *  Trap locations the interrupt priorities
  *  Separated by Bill Nowicki March 1981
  */

# ifndef M68VECTORS
# define M68VECTORS

#define BusErrorVector  *(int*)0x08

#define IRQ1Vect        *(int*)0x64
#define IRQ2Vect        *(int*)0x68
#define IRQ3Vect        *(int*)0x6C
#define IRQ4Vect        *(int*)0x70
#define IRQ5Vect        *(int*)0x74
#define IRQ6Vect        *(int*)0x78
#define IRQ7Vect        *(int*)0x7C

# endif M68VECTORS
