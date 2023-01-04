/*	@(#)vax.h 1.1 86/09/25 SMI; from UCB 1.2 03/31/83	*/

    /*
     *	opcode of the `calls' instruction
     */
#define	CALLS	0xfb

    /*
     *	offset (in bytes) of the code from the entry address of a routine.
     *	(see asgnsamples for use and explanation.)
     */
#define OFFSET_OF_CODE	2
#define	UNITS_TO_CODE	(OFFSET_OF_CODE / sizeof(UNIT))

    /*
     *	register for pc relative addressing
     */
#define	PC	0xf

enum opermodes {
    literal, indexed, reg, regdef, autodec, autoinc, autoincdef, 
    bytedisp, bytedispdef, worddisp, worddispdef, longdisp, longdispdef,
    immediate, absolute, byterel, bytereldef, wordrel, wordreldef,
    longrel, longreldef
};
typedef enum opermodes	operandenum;

struct modebyte {
    unsigned int	regfield:4;
    unsigned int	modefield:4;
};

