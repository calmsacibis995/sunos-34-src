/*
 *		Micro-assembler global defines
 *		micro.h	1.0	85/06/06	
 */

typedef enum {False=0, True} Boolean;
typedef enum {NEITHER, NUMBER, ALPHA} SYMTYPE;
typedef enum {	Tnull, Pseudo, Branch1, Branch2, State, Latch, Loadctrl,
		Function, Mode, Csl, Csux, U, Woe, Datactrl,
		Recoe, Reclk, Ramwe, Ramcs, Pointer
} Reswd_type;

#define NINST   4096    /* max number of instruction set */
#define NNODE	16384	/* max number of instructions */
#define NSYM    2048	/* more than enough symbol-table space */
#define NSTRING 8*NSYM  /* really more than enough string space */
#define NHASH 137


/* instruction descriptor nodes */
typedef struct node {
	char	*filename;	/* name of file in which encountered	*/
	struct   sym  *symptr;  /* symbolic addr or value		*/
	unsigned short word1;	/* 1st word of microcode instruction	*/
	unsigned short word2;	/* 2nd word of microcode instruction	*/
	unsigned short word3;	/* 3rd word of microcode instruction	*/
	unsigned short word4;	/* 4th word of microcode instruction	*/
	unsigned short word5;   /* last byte of microcode instruction   */
	short	lineno;		/* line number on which encountered	*/
	short	addr;		/* address we assign this instruction	*/
	char    *line;		/* defining text line			*/
	Boolean sccsid:1;	/* part of the sccsid			*/
	Boolean filled:1;	/* instr. has been filled		*/
	Boolean org_pseudo:1;	/* it's a pseudo-op			*/
	Boolean routine:1;	/* it's a routine pseudo-op		*/
	Boolean jumpop:1; 	/* the addr field is used by jump op 	*/
} NODE;

typedef struct ramnode {
	char	name[20];
	short	entry;
	char	addr[12];
	short	load;
	short	func;
} RAMNODE;

/* user-defined symbol bucket */
typedef struct sym {
	char		*name;
	Boolean		defined:1;
	short		addr;
	struct node	*node;
	struct sym	*next_hash;
} SYMBOL;

/* reserved-word bucket */
typedef struct reswd {
	char	*name;
	Reswd_type
		type;
	short	value1,
		value2;
	struct reswd *next_hash;
} RESERVED;


extern Boolean dflag;
#define DEBUG if(dflag) printf

extern RESERVED * resw_lookup();
extern SYMBOL * enter();
extern SYMBOL   * lookup();
