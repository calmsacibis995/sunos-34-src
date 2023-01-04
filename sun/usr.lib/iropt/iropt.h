/*	@(#)iropt.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "page.h"
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

# define LEAF_HASH_SIZE 256
# define EXPR_HASH_SIZE 256
# define COPY_HASH_SIZE 256

extern struct expr  *expr_hash_tab[];
extern LIST *copy_hash_tab[];
extern LIST *leaf_hash_tab[];

#define LOCAL 	static
#define	ORD(x)	((int) x)
#define	BPW		32

typedef enum {  VAR_DEF, VAR_AVAIL_DEF, VAR_USE1, VAR_USE2, 
				VAR_EXP_USE1, VAR_EXP_USE2 } VAR_REFERENCE_TYPE;

typedef enum {  EXPR_GEN, EXPR_AVAIL_GEN, EXPR_USE1, EXPR_USE2, 
				EXPR_EXP_USE1, EXPR_EXP_USE2, EXPR_KILL, EXPR_AVAIL_KILL } EXPR_REFERENCE_TYPE;

extern BLOCK *entry_block, *exit_block, *first_block_dfo;
extern SEGMENT *seg_tab;
extern int nseg, nblocks, nleaves,ntriples, nexprs, 
				nvardefs, nexprdefs, ncopies;
LIST *copy_list(), *merge_lists();
extern char * base_type_names[] ;
extern LIST *labelno_consts;

#   define SHOWRAW debugflag[20]
#   define SHOWCOOKED debugflag[21]
#   define SHOWDF debugflag[22]
#define MAXDEBUGFLAG 22
extern BOOLEAN debugflag[];

#	define DO_IV optimflag[2]
#	define DO_LOOP optimflag[5]
#	define DO_CSE optimflag[8]
#	define DO_COPY_PPG optimflag[11]
#	define DO_REG_ALLOC optimflag[14]
#	define MAXOPTIMFLAG 14
extern BOOLEAN optimflag[];
extern BOOLEAN skyflag;
extern BOOLEAN stmtprofflag;

typedef struct expr {
	IR_OP op;
	TYPE type;
	struct expr *left, *right;
	int exprno;
	LEAF *save_temp;  /* tmp that saves the value */
	LIST *depends_on; /*list of leaves that this expression dependends on*/
	struct expr_ref *references; /* description of sites which reference this expression */
	struct expr_ref *ref_tail;
	struct expr *next_expr;
} EXPR;

typedef struct site{
	BLOCK *bp;
	TRIPLE *tp;
} SITE;

typedef struct copy {
	int copyno;
	BOOLEAN visited;
	struct leaf *left, *right;
	struct list *define_at;
} COPY;

/* one of these is built for every def/use of a variable */
typedef struct var_ref {
	VAR_REFERENCE_TYPE reftype:16;
	short defno;		/* index for this definition in data flow bit vectors */
	struct var_ref *next_vref; /* references to a given leaf are linked */
	int refno;
	SITE site;  		/* site where the deed occurs*/
	LEAF *leafp;        /* leaf used or defined */
	struct var_ref *next;	/* link for traversing all var refs */
} VAR_REF;

/*	and one of these is buit for every def/use of a value */

typedef struct expr_ref {
	EXPR_REFERENCE_TYPE reftype:16;
	short defno;
	struct expr_ref *next_eref; /* references to a given expr are linked */
	int refno;
	SITE site;
} EXPR_REF;

/*	masses of expr kill records are created for large routines - to
** obtain reasonble performance expr kill references are kept in compressed
** structs  - the reftype and next_eref fields are common to expr_ref
** and expr_kill ref. Expr kill records do not appear on d/u chains so
** the space used to store them is reclaimed after build_expr_sets
*/
typedef struct expr_kill_ref {
	EXPR_REFERENCE_TYPE reftype:16;
	short blockno;
	struct expr_ref *next_eref; /* references to a given expr are linked */
# ifdef DEBUG
	int refno;
# endif
} EXPR_KILLREF;

typedef struct copy_ref {
	SITE site;	/* where it defined */
} COPY_REF;

typedef union list_u {
	struct copy copy;
	struct leaf leaf;
	struct triple triple;
	struct block block;
	struct var_ref var_ref;
	struct expr_ref expr_ref;
	struct copy_ref copy_ref;
} LDATA;

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
# define ISFLOAT(x) ((x) == FLOAT )
# define ISDOUBLE(x) ((x) == DOUBLE )
# define ISEXTENDEDF(x) ((x) == EXTENDEDF )
# define ISREAL(x) ((x) == FLOAT || (x) == DOUBLE || (x) == EXTENDEDF)
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
#define ISREADL(x) ((x)&MSKREAL)

typedef struct set_description {
	int bit_rowsize;
	int word_rowsize;
	int nrows;
	unsigned long *bits;
} *SET_PTR; 

BOOLEAN bit_test();
typedef unsigned long * SET;
typedef unsigned long * BIT_INDEX;

/*
	#define roundup(a,b)    ( b * ( (a+b-1)/b) )
*/

#define AUTO_SET(set,n_rows,n_bits)  {\
	register int bit_rowsize, word_rowsize, byte_setsize;\
		bit_rowsize = roundup(n_bits,BPW); \
		word_rowsize = bit_rowsize / 32; \
		byte_setsize = word_rowsize * n_rows * sizeof(unsigned); \
		set = (SET_PTR) ckalloca(byte_setsize+sizeof(struct set_description));\
		set->nrows = n_rows; \
		set->bit_rowsize = bit_rowsize; \
		set->word_rowsize = word_rowsize; \
		set->bits = (unsigned *) ((char*)set + sizeof(struct set_description));\
	}

PAGE *getpage();
LISTQ *new_listq();
extern LISTQ *proc_lq;
LIST *NEWLIST();
