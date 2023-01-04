#ifndef lint
static	char sccsid[] = "@(#)iropt.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "loop.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "opdescr.h"

char *ir_file; 
char *heap_start;
FILE *outfp;
# define DEFAULT_PCC_FILE "a.pcc"

BOOLEAN debugflag[MAXDEBUGFLAG+1] = {FALSE};
BOOLEAN optimflag[MAXOPTIMFLAG+1] = {FALSE};
BOOLEAN skyflag = {FALSE};
BOOLEAN use68881 = {FALSE};
BOOLEAN use68020 = {FALSE};
BOOLEAN usefpa = {FALSE};
BOOLEAN pflag = {FALSE};
BOOLEAN stmtprofflag = {FALSE};
BOOLEAN read_ir();
BOOLEAN partial_opt = {FALSE};		       /* if partial_opt then do
					        *  copy_ppg and reg_allo for
					        *  local variables.
						*/
int regmask;

EXPR *expr_hash_tab[EXPR_HASH_SIZE];
LIST *leaf_hash_tab[LEAF_HASH_SIZE];
LIST *copy_hash_tab[COPY_HASH_SIZE];

char *scan_args();

/*
** note that though indirect is a unary op it's right child may be an access list
*/

char * base_type_names[] = {
"undef", "farg", "char", "short", "int", "long", "float", "double",
"strty", "unionty", "enumty", "moety", "uchar", "ushort", "unsigned",
"ulong", "bool", "extendedf", "complex", "dcomplex", "string", "labelno", "void"
};


extern int pgsize;
extern VAR_REF *free_var, *var_ref_head, *alloc_new_varref();
extern EXPR_REF *free_expr, *alloc_new_exprref();
extern EXPR *expr_head, *expr_tail;
LISTQ *var_lq, *df_lq, *proc_lq, *tmp_lq, *loop_lq;
HEADER hdr;
extern int n_label_defs, preloop_ntriples, preloop_nleaves;
extern LIST *labelno_consts, *label_list, *indirgoto_targets;

static char *pcc_file;
static char stdout_buffer[BUFSIZ];
static char outfp_buffer[BUFSIZ];


main(argc,argv) 
int argc;
char *argv[];
{
char c;
int irfd;

	iropt_init();
	if( (ir_file=scan_args(argc,argv)) != NULL ) {
		irfd = open(ir_file,O_RDONLY,0);
		if (irfd == -1) {
			perror(ir_file);
			quit("can't open ir file");
		}
		if(pcc_file == (char*) NULL) {
			pcc_file = DEFAULT_PCC_FILE;
		}
		outfp=fopen(pcc_file,"w");
		if(outfp == NULL) {
			perror("iropt:");
			quit("can't open pcc file");
		}
		setbuf(outfp,outfp_buffer);
		while( read(irfd, &hdr, sizeof(HEADER)) == sizeof(HEADER) ) {
			if(read_ir(irfd) == TRUE) {
				connect_labelrefs();
				form_bb();
				if(SHOWRAW==TRUE) {
					dump_segments();
					dump_blocks();
					dump_leaves();
					dump_triples();
				}
				if(optimflag[0] == TRUE) {
					insert_implicit();
					/* FIXME
						cleanup_cf();
					*/
					find_dfo();
					find_loops(); 
					make_loop_tree();
					if(DO_LOOP == TRUE && ! partial_opt ) {
						preloop_ntriples = ntriples;
						preloop_nleaves = nleaves;
						compute_df(TRUE,FALSE,TRUE);
						delete_implicit();
						/* want 1 temp per expr so build expr_hash_tab */
						entable_exprs();
						free_depexpr_lists();
						if(debugflag[4]==TRUE) dump_cooked(4);
						scan_loop_tree(loop_tree_root,FALSE);
						if(debugflag[6]==TRUE) dump_cooked(6);
						free_exprs();
						empty_listq(df_lq);
					}
					if(DO_IV == TRUE && ! partial_opt ) {
						preloop_ntriples = ntriples;
						preloop_nleaves = nleaves;
						compute_df(TRUE,FALSE,TRUE);
						entable_exprs();
						if(debugflag[1]==TRUE) dump_cooked(1);
						scan_loop_tree(loop_tree_root,TRUE);
						if(debugflag[3]==TRUE) dump_cooked(3);
						empty_listq(df_lq);
						free_depexpr_lists();
						free_exprs();
						empty_listq(loop_lq);
					}
					if(DO_CSE == TRUE && ! partial_opt ) {
						cse_init();
						compute_df(FALSE,TRUE,FALSE);
						if(debugflag[7]==TRUE) dump_cooked(7);
						do_local_cse();
						free_exprs();
						empty_listq(df_lq);
						compute_df(TRUE,TRUE,TRUE);
						if(debugflag[8]==TRUE) dump_cooked(8);
						do_global_cse();
						if(debugflag[9]==TRUE) dump_cooked(9);
						free_exprs();
						empty_listq(df_lq);
					}
					if(DO_COPY_PPG == TRUE) {
						compute_df(TRUE,FALSE,FALSE);
						if(debugflag[10]==TRUE) dump_cooked(10);
						do_local_ppg();
						empty_listq(df_lq);
						compute_df(TRUE,FALSE,TRUE);
						if( partial_opt ) {
							delete_implicit();
						}
						if(debugflag[11]==TRUE) dump_cooked(11);
						do_global_ppg();
						if(debugflag[12]==TRUE) dump_cooked(12);
						empty_listq(df_lq);
					}
					if(DO_REG_ALLOC == TRUE ) {

					/* put the triples in a basic block */
					/* in order to make the short_intf's */
					/* life easier */
						resequence_triples();	
						compute_df(TRUE,FALSE,TRUE);
						if(debugflag[13]==TRUE) dump_cooked(13);
						do_register_alloc();
						if(debugflag[15]==TRUE) dump_cooked(15);
						empty_listq(df_lq);
					}
				}
				ir_prewrite();
				write_irfile(hdr.procno, hdr.procname, hdr.proc_type, outfp);
			}
			heap_reset();
		}
	}
}

char *
scan_args(argc,argv)
int argc;
register char *argv[];
{
register char *s;
int i;

--argc;
++argv;

optimflag[0] = debugflag[0] = FALSE;
pcc_file = (char*) NULL;


while(argc > 0 &&  *argv[0] =='-' ) {
		switch(*(argv[0]+1)) {
			case 'o':
				--argc;
				++argv;
				if(argc == 0) {
					quit("scanargs: no argument given for -o");
				}
				pcc_file = *argv;
				goto contin;
		}

		for(s=argv[0]+1;*s; ++s) {
			switch(*s) {
		
				case 'P':
					partial_opt = TRUE;
					/* fall through */
					
				case 'O':
					for(i=0;i< MAXOPTIMFLAG+1;i++) {
						optimflag[i] =  TRUE;
					}
					optimflag[0]=TRUE;
					/*
					**	test whether individual optimizations should be 
					**	suppressed
					*/
					while(*s == 'O' || *s == 'P' || *s == ',') {
						register int k=0;
						while(isdigit(*++s) ) k=10*k + (*s - '0');
						if(k > MAXOPTIMFLAG) {
							quita("bad optimization flag: %d",k);
						} else {
							if(k>0) optimflag[k] = FALSE;
						}
					}
					--s;
					break;
		
				case 'd':
					debugflag[0]=TRUE;
					while(*s == 'd' || *s == ',') {
						register int k=0;
						while(isdigit(*++s) ) k=10*k + (*s - '0');
						if(k > MAXDEBUGFLAG) quita("bad debug flag: %d",k);
						else debugflag[k] = TRUE;
					}
					--s;
					break;
		
				case 'F':
					skyflag = TRUE;
					break;

				case 'm':
					use68881 = TRUE;
					break;
				
				case 'f':
					usefpa = TRUE;
					break;
					
				case 'c':
					use68020 = TRUE;
					break;
					
				case 'a':
					stmtprofflag = TRUE;
					break;

				case 'p':
					{ register int k=0;

						pflag = TRUE;
						while(isdigit(*++s) ) k=10*k + (*s - '0');
						if(k < 1 || k > 32) quita("bad pgsize multiple %d",k);
						pgsize *=  k;
					}
					--s;
					break;

				default:
					quita("invalid flag %c",*s);
			}
		}
	contin: 
		--argc;
		++argv;
	}
if(argc < 1) {
	quit(" no ir file specified ");
}
return(argv[0]);
}

iropt_init()
{
struct rlimit ri, *rip = &ri;
char *tmp;

	/* make sure the stdio buffers don't interfere with heap management */
	setbuf(stdout,stdout_buffer);
	setbuffer(stderr, NULL, 0);

	pgsize = getpagesize();
	/* initialize heap_start to the first page boundary after edata
	** from this point the heap will grow in pgsize chunks
	*/
	heap_start=(char*) sbrk(0);
	tmp = (char*) roundup((int)heap_start,pgsize);
	if(tmp != heap_start) {
		sbrk(tmp-heap_start);
		heap_start = tmp;
	}
	getrlimit(RLIMIT_STACK, rip);
	rip->rlim_cur = rip->rlim_max;
	setrlimit(RLIMIT_STACK, rip);
}

proc_init()
{
register LIST **lpp, **lptop;
register EXPR **epp, **eptop;

	fflush(stdout);
	lptop = &leaf_hash_tab[LEAF_HASH_SIZE];
	for(lpp=leaf_hash_tab; lpp < lptop;) *lpp++ = LNULL;

	eptop = &expr_hash_tab[EXPR_HASH_SIZE];
	for(epp=expr_hash_tab; epp < eptop;) *epp++ = (EXPR *)NULL;

	entry_block = exit_block = (BLOCK*) NULL;
	loop_tree_root = ( LOOP_TREE*) NULL;
	indirgoto_targets = labelno_consts = label_list = LNULL;
	n_label_defs = 0;

	if(proc_lq == NULL) proc_lq = new_listq();
	empty_listq(proc_lq);
	if(var_lq == NULL) var_lq = new_listq();
	if(df_lq == NULL) df_lq = new_listq();
	if(tmp_lq == NULL) tmp_lq = new_listq();
	regmask = 0x757;		       /* all registers are avaliable
						* except A7 and A6 */
}

/* before rewriting an ir file  make sure labelefs point to leaves so
** f1 doesn't have to distinguish optimized from unoptimzed input; also
** zero out some fields that will not be used by f1
*/
ir_prewrite()
{
register BLOCK *bp, *block;
register TRIPLE *tp, *labelref;

	for(bp=entry_block; bp; bp=bp->next) {
		TFOR(tp, bp->last_triple) {
			if(ISOP(tp->op,NTUPLE_OP)) {
				TFOR(labelref, (TRIPLE*)tp->right) {
					if( labelref->op == LABELREF ) {
						if((block=(BLOCK*)labelref->left) && block->tag ==ISBLOCK){
							labelref->left = (NODE*) ileaf(block->labelno);
						} else {
							labelref->left = (NODE*) ileaf(-1);
						}
					}
				}
			} else if(tp->op == LABELDEF) {
				if((block=(BLOCK*)tp->left) && block->tag ==ISBLOCK) {
					tp->left = (NODE*) ileaf(block->labelno);
				}
			}
			tp->can_access = LNULL;
		}
		bp->pred = bp->succ = LNULL;
	}
}
