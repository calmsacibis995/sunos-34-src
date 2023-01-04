/* @(#)rpc_util.h 1.1 86/09/25 (C) 1986 SMI */

/*
 * rpc_util.h, Useful definitions for the RPC protocol compiler
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

extern char *malloc();
extern char *sprintf();
#define alloc(size)		malloc((unsigned)(size))
#define ALLOC(object)   (object *) malloc(sizeof(object)) 

#define OFF 0
#define ON 1

struct list {
	char *val;
	struct list *next;
};
typedef struct list list;

/*
 * Global variables
 */
#define MAXLINESIZE 1024
extern char curline[MAXLINESIZE];
extern char *where;
extern int linenum;

extern char *outfile;
extern char *outfile2;
extern char *infile;
extern FILE *fout;
extern FILE *fin;

extern list *printed;
extern list *defined;


/*
 * rpc_util routines
 */
void storeval();
#define STOREVAL(list,item)	\
	storeval(list,(char *)item)

char *findval();
#define FINDVAL(list,item,finder) \
	findval(list, (char *) item, finder)

int streq();
void error();
void expected1();
void expected2();
void expected3();

/*
 * rpc_cout routines
 */
void cprint();
void emit();

/*
 * rpc_hout routines
 */
void print_datadef();
void print_funcdefs();

/*
 * rpc_svcout routines
 */
void write_most();
void write_register();
void write_rest();

