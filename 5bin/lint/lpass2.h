/*	@(#)lpass2.h 1.1 86/09/24 SMI; from S5R2 1.3	*/

typedef struct sty STYPE;
struct sty { ATYPE t; STYPE *next; };

typedef struct sym {
#ifdef FLEXNAMES
	char *name;
#else
	char name[LCHNM];
#endif
	char nargs;
	int decflag;
	int fline;
	STYPE symty;
	int fno;
	int mno;
	int use;
	short more;
	} STAB;
