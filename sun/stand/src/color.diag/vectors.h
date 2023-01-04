 /*
  *  Trap locations the interrupt priorities
  *  Separated by Bill Nowicki March 1981
  */

#define BusErrorVector  *(int*)0x08
#define BusEVect	*((int*)0x8)

#define IRQ1Vect        *(int*)0x64
#define IRQ2Vect        *(int*)0x68
#define IRQ3Vect        *(int*)0x6C
#define IRQ4Vect        *(int*)0x70
#define IRQ5Vect        *(int*)0x74
#define IRQ6Vect        *(int*)0x78
#define IRQ7Vect        *(int*)0x7C

