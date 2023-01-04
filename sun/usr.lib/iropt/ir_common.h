/* @(#)ir_common.h 1.1 86/09/25 Copyr 1985 Sun Micro */
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
typedef enum { VAR_LEAF=1, ADDR_CONST_LEAF=2, CONST_LEAF=3 } LEAF_CLASS;
typedef enum { ISBLOCK=1, ISLEAF, ISTRIPLE } TAG;
typedef enum { INTR_FUNC, SUPPORT_FUNC, EXT_FUNC } FUNC_DESCR;
typedef enum { UNOPTIMIZED, OPTIMIZED } FILE_STATUS;
typedef unsigned long TWORD;


typedef enum { REG_SEG, STG_SEG } SEGCONTENT;
typedef enum { USER_SEG, BUILTIN_SEG } SEGBUILTIN;
typedef enum { LCLSTG_SEG, EXTSTG_SEG } SEGSTGTYPE;
typedef enum { ARG_SEG, BSS_SEG, DATA_SEG, AUTO_SEG, DREG_SEG, AREG_SEG, FREG_SEG,
			HEAP_SEG } SEGCLASS;

struct	segdescr_st	{
	SEGCLASS class:	8;			/* segment class */
	SEGCONTENT content: 8;		/* registers or storage */
	SEGBUILTIN builtin: 8;		/* built in or user defined */
	SEGSTGTYPE external: 8;		/* external or local storage */
};

typedef struct segment {
	char *name;
	struct segdescr_st descr;
	int base;
	int offset;
	int len;
	struct list *leaves;
} SEGMENT;

/* indexes into seg_tab for builtin segments : these segments are always present
** but may be empty
*/
# define ARG_SEGNO 0
# define BSS_SMALL_SEGNO 1
# define BSS_LARGE_SEGNO 2
# define DATA_SEGNO 3
# define AUTO_SEGNO 4
# define HEAP_SEGNO 5
# define DREG_SEGNO 6
# define AREG_SEGNO 7
# define FREG_SEGNO 8
# define N_BUILTIN_SEG (FREG_SEGNO + 1)

typedef struct address {
	struct segment * seg;
	int offset;
	int labelno;
	int length;
	short alignment;
} ADDRESS;
#define NOBASEREG -1

union	const_u {
		char *cp;		/* Character constants */
		long i;		/* Integer constants */
		char *fp[2];	/* Strings for float real and complex consts */
		char *bytep[2]; /* bytes for binary constants (init only) */
};

typedef struct list {
		struct list *next;
		union list_u *datap;
} LIST;

typedef struct irtype {
	TWORD	tword;		
	union {
		int size;
		FUNC_DESCR func_descr
	} aux;
} TYPE;

typedef struct block {
	TAG tag:8;
	BOOLEAN is_ext_entry : 8; /* is the block an external entry point */
	int blockno;
	struct triple *last_triple;
	char *entryname;
	int labelno;
	struct block *next;		/* allocation defined order */
	BOOLEAN visited;
	struct list *pred, *succ;
# ifdef IROPT
	struct block *dfonext, *dfoprev;	/* depth first search order */
	struct list *loops;
	int	loop_weight;
# endif
} BLOCK;

union leaf_value {
	struct address addr;
	struct constant {
		BOOLEAN isbinary;
		union const_u c
	} const;
};

typedef struct leaf {
	TAG tag:8;
	LEAF_CLASS class:8;
	int leafno;
	struct irtype	type;
	union leaf_value val;
	BOOLEAN	visited; /* used as a flag and/or pointer to auxiliary information*/
	char *pass1_id;
	struct leaf *next_leaf;
	struct list *overlap;
# ifdef IROPT
	struct triple *entry_define, *exit_use;
	BOOLEAN no_reg;	/* indicate this leaf can not be in the register */
	struct  var_ref *references;
	struct	var_ref *ref_tail;
	struct  list *dependent_exprs; /*list of expressions that dependend on this leaf*/
	struct list *kill_copy;	/* list of copies that will be killed if this leaf 
							**	has been redefined
							*/
# endif
} LEAF;

typedef struct triple {
	TAG tag:8;
	IR_OP	op:8;
	int tripleno;
	struct irtype	type;
	struct triple *tprev, *tnext;
	union node_u *left,*right;
	BOOLEAN	visited;	/* flag/ptr used repeatedly by various phases */
	struct list *can_access;/*	for ifetch or istore triples: the list 
							/*	of leaves that may be affected */
# ifdef IROPT
	struct expr *expr;
	struct list *reachdef1; /* list of sites that may define the var or expr */
	struct list *reachdef2; /*	used at this point */
	struct list *canreach;	/*	list of sites that may use the var or expr */
							/*	defined at this  point */
	struct var_ref *var_refs; /* the var_ref records created for a triple */
	struct list	*implicit_use;	/*implicit u records created for this triple */
	struct list	*implicit_def;
# endif
} TRIPLE;

typedef union node_u {
	struct leaf leaf;
	struct triple triple;
	struct { 
		TAG tag:8; 
		unsigned fill : 8;
		int number;	/* align with tripleno, leafno, blockno */
		struct irtype type;
	} operand;
	struct block block;
} NODE;

typedef struct header {
	int triple_offset, block_offset, leaf_offset, seg_offset, string_offset,
	list_offset, proc_size;
	int procno;
	char *procname;
	FILE_STATUS file_status;
	int regmask;
	int proc_type;
} HEADER;

typedef struct string_buff {
	struct string_buff *next;
	char *data, *top, *max;
} STRING_BUFF; 
# define STRING_BUFSIZE 1024

# define UN_OP (1<<0)		/* unary */
# define BIN_OP (1<<1)		/* binary */
# define VALUE_OP (1<<2)	/* computes a subexpression value */
# define BRANCH_OP (1<<3)	/* may alter control flow */
# define MOD_OP (1<<4)		/* may modify 1st operand */
# define USE1_OP (1<<5)		/* may use 1st operand */
# define USE2_OP (1<<6)		/* may use 2nd operand */
# define BOOL_OP (1<<7)		/* could be a boolean operation */
# define FLOAT_OP (1<<8)	/* could be a floating operation */
# define HOUSE_OP (1<<9)	/* housekeeping operator */
# define COMMUTE_OP (1<<10)	/* operator is commutative */
# define ROOT_OP (1<<11)	/* operator roots a pcc tree */
# define NTUPLE_OP (1<<12)	/* triple's right child holds additional triples */

# define ISOP(op,attribute) (op_descr[(int)(op)].flags & attribute)

extern struct opdescr_st {
	char name[20];
	int flags;
	struct {
		int d_weight, a_weight;
		} left, right;
	int f_weight;
} op_descr[];

#define LCAST(list,structname) ((structname*)((list)->datap))
#define LNULL   ((LIST*) NULL)
#define TNULL   ((TRIPLE*) NULL)
#define LFOR(list,head) for((list)=(head);(list);(list) = ((list)->next == (head) ? LNULL : (list)->next))
	
#define TFOR(triple,head) for((triple)=(head);(triple);(triple) = ((triple)->tnext == (head) ? TNULL : (triple)->tnext))
/*
	append an item to a circular list AND update the pointer to the list
*/
#define LAPPEND(after,item) { register LIST *tmp1;\
								if((after) != LNULL) {\
									tmp1 = (item)->next;\
									(item)->next = (after)->next; \
									(after)->next = tmp1;\
								} (after) = (item); }

#define TAPPEND(after,item) { register TRIPLE *tmp1, *tmp2;\
								if((after) != TNULL) {\
									tmp1 = (item)->tnext;\
									tmp2 = (after)->tnext;\
									(after)->tnext = tmp1;\
									(item)->tnext = tmp2; \
									tmp1->tprev = (after);\
									tmp2->tprev  = (item);\
								} (after) = (item); }

extern STRING_BUFF *string_buff, *first_string_buff;
extern BLOCK *first_block;
extern LEAF *first_leaf;
