/*	@(#)cg_ir.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

typedef enum {
LEAFOP=0,  ENTRYDEF, EXITUSE, IMPLICITDEF, IMPLICITUSE,
PLUS, MINUS,MULT,DIV,REMAINDER, AND, OR, XOR, NOT, LSHIFT, RSHIFT, SCALL, FCALL,
EQ, NE, LE, LT, GE, GT, CONV, COMPL, NEG, ADDROF, IFETCH, ISTORE, GOTO,
CBRANCH, SWITCH, REPEAT, ASSIGN, PASS, STMT, LABELDEF, 
INDIRGOTO, FVAL, LABELREF, PARAM
} IR_OP;

typedef enum {FALSE,TRUE} BOOLEAN;

/* 
**	the above typedefs differ in f77pass1 and iropt
*/

#include "ir_common.h"

extern HEADER hdr;

#define LOCAL 	static
#define	ORD(x)	((int) x)
#define	BPW		32

extern BLOCK *entry_block;
extern SEGMENT *seg_tab;
extern TRIPLE *triple_tab;
extern LEAF* leaf_tab;
extern int nseg, nblocks, nleaves,ntriples, nlists, strtabsize;
extern char * base_type_names[] ;

extern LIST *leaf_hash_tab[];
# define LEAF_HASH_SIZE 256

extern BOOLEAN skyflag;

typedef union list_u {
	struct leaf leaf;
	struct triple triple;
	struct block block;
} LDATA;
LIST *new_list();
LEAF *leaf_lookup();
BOOLEAN same_irtype();

/*
**	pcc base types 
**	these  two won't work in the ir: use explicit modifier of UNDEFINED instead
**	# define TNULL PTR    	 * pointer to UNDEF * 
**	# define TVOID FTN		 * function returning UNDEF (for void)  *
**
*/
# define UNDEF 0
# define FARG 1
# define CHAR 2
# define SHORT 3
# define INT 4
# define LONG 5
# define FLOAT 6
# define DOUBLE 7
# define STRTY 8
# define UNIONTY 9
# define ENUMTY 10
# define MOETY 11
# define UCHAR 12
# define USHORT 13
# define UNSIGNED 14
# define ULONG 15
/*		
**	additional ir base types
*/
# define BOOL 16
# define EXTENDEDF 17
# define COMPLEX 18
# define DCOMPLEX 19
# define STRING 20
# define LABELNO 21
# define VOID 22

# define NTYPES VOID+1
/* type modifiers */

# define PTR  0100
# define FTN  0200
# define ARY  0300

/*
**  type packing constants :
**	the pcc's type packing is modified to allow 6 bits for base and 13 
**	qualifiers
*/

# define TMASK 0300
# define BTMASK 077
# define BTSHIFT 6
# define TSHIFT 2
/*	macros	*/

# define MODTYPE(x,y) x = ( (x)&(~BTMASK))|(y)  /* set basic type of x to y */
# define BTYPE(x)  ( (x)&BTMASK)   /* basic type of x */
# define ISUNSIGNED(x) ((x)<=ULONG&&(x)>=UCHAR)
# define UNSIGNABLE(x) ((x)<=LONG&&(x)>=CHAR)
# define ENUNSIGN(x) ((x)+(UNSIGNED-INT))
# define DEUNSIGN(x) ((x)+(INT-UNSIGNED))
# define ISPTR(x) (( (x)&TMASK)==PTR)
# define ISFTN(x)  (( (x)&TMASK)==FTN)  /* is x a function type */
# define ISARY(x)   (( (x)&TMASK)==ARY)   /* is x an array type */
# define INCREF(x) ((( (x)&~BTMASK)<<TSHIFT)|PTR|( (x)&BTMASK))
# define DECREF(x) ((( (x)>>TSHIFT)&~BTMASK)|( (x)&BTMASK))

#define M(x) 	(1<< ((int)(x)))
#define MSKINT M(SHORT)  | M(INT) | M(LONG)
#define MSKREAL M(FLOAT) | M(DOUBLE) | M(EXTENDEDF)
#define MSKCOMPLEX M(COMPLEX) | M(DCOMPLEX)
#define MSKCHAR M(CHAR) | M(STRING)
#define ONEOF(x,y) (M(x) & (y) )

#define B_ISINT(z) ONEOF((BTYPE(z)) , MSKINT)
#define B_ISREAL(z) ONEOF((BTYPE(z)), MSKREAL)
#define B_ISCHAR(z) ONEOF((BTYPE(z)) , MSKCHAR)
#define B_ISBOOL(z)  ( (BTYPE(z)) == BOOL )
#define B_ISCOMPLEX(z) ONEOF((BTYPE(z)) ,MSKCOMPLEX)

#define	MSKUSE	M(VAR_USE1) | M( VAR_USE2)| M( VAR_EXP_USE1) | M( VAR_EXP_USE2)
#define ISUSE(z) ONEOF(z,MSKUSE)

#define	MSKEUSE	M(EXPR_USE1) | M( EXPR_USE2)| M( EXPR_EXP_USE1) | M( EXPR_EXP_USE2)
#define ISEUSE(z) ONEOF(z,MSKEUSE)
#define ISCONST(leafp) ((int)((LEAF*)leafp)->class & 2 )

extern struct opdescr_st op_descr[];
typedef struct opaque { int filler; } *CNODE;

#   define ARGFPOFFSET 8
#   define  AREGOFFSET  8
#   define STACK_REG 6 + AREGOFFSET
#   define BSS_REG  5 + AREGOFFSET
#   define FREGOFFSET 8 + AREGOFFSET
#   define  MAXDISPL 32768
#   define  HIGH_DREG 7
#   define  N_DREGS 6
