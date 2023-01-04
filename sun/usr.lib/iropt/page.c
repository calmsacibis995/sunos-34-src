#ifndef lint
static	char sccsid[] = "@(#)page.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "reg.h"
#include <stdio.h>

int npages[] = { 0, 0, 0, 0, 0, 0, 0, 0, };
#define NPAGETAGS sizeof(npages)/sizeof(int)
int npagetags = NPAGETAGS;
extern TRIPLE *free_triple_lifo;

extern int nvardefs, nexprdefs;
extern int nvarrefs, nexprrefs;
extern VAR_REF *var_ref_head;
extern LISTQ *cse_lq, *copy_lq, *reg_lq, *var_lq, *loop_lq, *proc_lq, *df_lq, *tmp_lq, *dependentexpr_lq;
extern BOOLEAN pflag;
extern char *heap_start;

static VAR_REF *last_var_ref = (VAR_REF*) NULL;
static PAGE *misc_page;
static long *misc_nextavail, *misc_top;
static PAGE *lastpg;

int pgsize, availchar_per_page;
PAGE *freepage_lifo = NULL;

heap_setup(irf_size)
int irf_size;
{
PAGE *pp, *here;
int pad, rest;
char *brk;

		if(pflag == FALSE) {
			pgsize = getpagesize();
			if(ntriples < 250) {
				pgsize = ( pgsize >=  4*1024 ? pgsize : 4*1024);
			} else if(ntriples < 500 ) {
				pgsize = ( pgsize >=  8*1024 ? pgsize : 8*1024);
			} else if(ntriples < 1000 ) {
				pgsize = ( pgsize >=  16*1024 ? pgsize : 16*1024);
			} else {
				pgsize = ( pgsize >=  32*1024 ? pgsize : 32*1024);
			}
		}
		irf_size = roundup(irf_size,sizeof(long));
		pad = roundup(irf_size,pgsize);

		here= (PAGE*) sbrk(0);
		pp = (PAGE*) sbrk(pad);
		if(pp  == (PAGE*) -1) {
			quita("alloc_setup: not enough space for %d bytes of ir file", pad);
		}
		bzero(here,pad);

		lastpg = (PAGE*) ( sbrk(0) -pgsize);
		if(irf_size < pad) { /* use space left in last page for ckalloc */
			misc_page = lastpg;
			misc_top = (long*) ((char*) misc_page + pgsize);
			misc_nextavail = (long*) ((char*) misc_top - (pad-irf_size));
		}

		availchar_per_page = pgsize - 2*sizeof(char*); 
}

LISTQ *
new_listq()
{
register LISTQ *listq;

	listq = (LISTQ*) ckalloc(1,sizeof(LISTQ));
	return(listq);
}

empty_listq(listqp)
LISTQ *listqp;
{
register PAGE *pp, *next;

	for(pp = listqp->head; pp; pp = next) {
		next = pp->next;
		pp->next = freepage_lifo;
		freepage_lifo = pp;
	}
	listqp->nextavail = LNULL;
	listqp->head = listqp->tail = (PAGE*) NULL;
}

refill_listq(listqp)
LISTQ *listqp;
{
register PAGE *pp;

	pp = getpage(LISTPAGE);
	if(listqp->tail) {
		listqp->tail->next = pp;
	}
	if(listqp->head == (PAGE*) NULL ) {
		listqp->head = pp;
	}
	listqp->tail = pp;
	listqp->nextavail = (LIST*) &pp->start;
	listqp->top = &pp->start + ( sizeof(LIST)*(availchar_per_page/sizeof(LIST)));
}

LIST *
NEWLIST(lqp) 
register LISTQ *lqp;
{
register LIST *tmp;

	if(lqp->tail && ( lqp->nextavail != (LIST*) lqp->top ) ){
	} else {
		refill_listq(lqp);
	}
	tmp =  lqp->nextavail;
	tmp->next = tmp;

	lqp->nextavail++;
	return tmp;
}
/*
	#define NEWLIST(lqp) ( (    (lqp)->tail && \
							 (LIST*) &((lqp)->tail->next) != (lqp)->nextavail \
							  ? NULL :  refill_listq(lqp) )\
							  , lqp->nextavail++)
*/

PAGE *
getpage(used_for)
int used_for;
{
int i= 0;
PAGE *page;

	if(freepage_lifo != NULL) {
		page = freepage_lifo;
		freepage_lifo = page->next;
	} else {
		page = (PAGE*) sbrk(pgsize);
		if(page == (PAGE*) -1) {
			perror("getpage");
			pagestats();
			quit("getpage: out of memory");
		}
		if(lastpg && page != (PAGE*) ((char*)lastpg+pgsize)) {
			quit("getpage: heap not contiguous \n");
		}
		lastpg = page;
		page->used_for = used_for;
		npages[used_for]++;
	}
	page->next = NULL;
	return page;
}

TRIPLE *
new_triple(flush)
BOOLEAN flush;
{
static PAGE *head, *tail;
static TRIPLE *nextavail, *top, *tmp;
PAGE *pp;

	if(flush == TRUE) {
		free_pages(head);
		nextavail = top = (TRIPLE*) NULL;
		head = tail = (PAGE*) NULL;
		return;
	}

	if( nextavail == top ) {
		pp = getpage(TRIPLEPAGE);
		if(tail) {
			tail->next = pp;
		}
		if(head == (PAGE*) NULL ) {
			head = pp;
		}
		tail = pp;
		nextavail = (TRIPLE*) &pp->start;
		top = (TRIPLE*) (&pp->start + ( sizeof(TRIPLE)*(availchar_per_page/sizeof(TRIPLE))));
	}
	tmp =  nextavail;
	nextavail++;
	return tmp;
}

free_pages(head)
register PAGE *head;
{
register PAGE *next,*pp;

	for(pp = head; pp; pp = next) {
		next = pp->next;
		pp->next = freepage_lifo;
		freepage_lifo = pp;
	}
}

ckalloc(n,size)
unsigned n,size;
{
PAGE *new_page;
register long *tmp, *tmp_x;
register unsigned byte_size, long_size, remainder;
register int i;
int page_multiple;

	byte_size = size * n;
	byte_size = roundup(byte_size, sizeof(long)); /* request in bytes */
	long_size = byte_size / sizeof(long); /* request in words */
	if(byte_size > pgsize ) {

			if(debugflag[18] == TRUE) {
				if(misc_page)
					printf("ckalloc %d wasted\n",(misc_top-misc_nextavail)*4);
			}
			page_multiple = roundup(byte_size, pgsize);
			new_page = (PAGE*) sbrk(page_multiple);
			if(new_page == (PAGE*) -1) {
				perror("ckalloc:");
				pagestats();
				quit("ckalloc: out of memory");
			}
			misc_top = (long*) ((char*)new_page + page_multiple);
			lastpg = misc_page = (PAGE*) ((char*) misc_top - pgsize);
			misc_nextavail = (long*) new_page + long_size;
			tmp = (long*) new_page;
	} else if(misc_page && misc_nextavail+long_size < misc_top) {
		/* there's room on this page */
		tmp = misc_nextavail;
		misc_nextavail += long_size;
	} else {
		new_page = getpage(MISCPAGE);
		if(misc_page && new_page == (PAGE*) ((char*)misc_page+pgsize)) {
			/* the next page is contiguous */
			tmp = misc_nextavail;
			remainder = long_size - (misc_top-misc_nextavail);
			misc_page = new_page;
			misc_top = (long*) ((char*)new_page + pgsize);
			misc_nextavail = (long*)new_page + remainder;
		} else {
			if(debugflag[18] == TRUE) {
				if(misc_page)
					printf("ckalloc %d wasted\n", (misc_top-misc_nextavail)*4);
			}
			misc_page = new_page;
			misc_top = (long*) ((char*)new_page + pgsize);
			tmp = (long*) misc_page;
			misc_nextavail = (long*)new_page + long_size;
		}
	}
	
	tmp_x = tmp;
	for(i=0;i<long_size;i++) {
		*tmp_x++ = 0;
	}
	return ((int) tmp);
}

EXPR *
new_expr(flush)
BOOLEAN flush;
{
static PAGE *head, *tail;
static EXPR *nextavail, *top, *tmp;
PAGE *pp;

	if(flush == TRUE) {
		free_pages(head);
		nextavail = top = (EXPR*) NULL;
		head = tail = (PAGE*) NULL;
		return;
	}

	if( nextavail != top ) {
	} else {
		pp = getpage(EXPRPAGE);
		if(tail) {
			tail->next = pp;
		}
		if(head == (PAGE*) NULL ) {
			head = pp;
		}
		tail = pp;
		nextavail = (EXPR*) &pp->start;
		top = (EXPR*) (&pp->start + ( sizeof(EXPR)*(availchar_per_page/sizeof(EXPR))));
	}
	tmp =  nextavail;
	nextavail++;
	return tmp;
}

VAR_REF *
new_var_ref(flush)
BOOLEAN flush;
{
static PAGE *head, *tail;
static VAR_REF *nextavail, *top, *rp;
PAGE *pp;

	if(flush == TRUE) {
		free_pages(head);
		var_ref_head  = last_var_ref = nextavail = top = (VAR_REF*) NULL;
		head = tail = (PAGE*) NULL;
		return;
	}

	if( nextavail != top ) {
	} else {
		pp = getpage(VARREFPAGE);
		if(tail) {
			tail->next = pp;
		}
		if(head == (PAGE*) NULL ) {
			head = pp;
		}
		tail = pp;
		nextavail = (VAR_REF*) &pp->start;
		top = (VAR_REF*) (&pp->start + 
					( sizeof(VAR_REF)*(availchar_per_page/sizeof(VAR_REF)) ));
		if(var_ref_head == NULL) var_ref_head = nextavail;
	}
	rp =  nextavail;
	nextavail++;

	rp->next_vref = rp->next = (VAR_REF*) NULL;
	rp->refno = nvarrefs++;
	if( last_var_ref ) {
		last_var_ref->next = rp;
	}
	last_var_ref = rp;
	return rp;
}

EXPR_REF *
new_expr_ref(flush)
BOOLEAN flush;
{
static PAGE *head, *tail;
static EXPR_REF *nextavail, *top;
register EXPR_REF *rp;
PAGE *pp;

	if(flush == TRUE) {
		free_pages(head);
		nextavail = top = (EXPR_REF*) NULL;
		head = tail = (PAGE*) NULL;
		return;
	}

	if( nextavail == top ) {
		pp = getpage(EXPRREFPAGE);
		if(tail) {
			tail->next = pp;
		}
		if(head == (PAGE*) NULL ) {
			head = pp;
		}
		tail = pp;
		nextavail = (EXPR_REF*) &pp->start;
		top = (EXPR_REF*) (&pp->start + 
					( sizeof(EXPR_REF)*(availchar_per_page/sizeof(EXPR_REF)) ));
	}
	rp =  nextavail;
	nextavail++;

	rp->next_eref = (EXPR_REF*) NULL;
	rp->refno = nexprrefs++;
	return rp;
}

EXPR_KILLREF *
new_expr_killref(flush)
BOOLEAN flush;
{
static PAGE *head, *tail;
static EXPR_KILLREF *nextavail, *top;
register EXPR_KILLREF *rp;
PAGE *pp;

	if(flush == TRUE) {
		free_pages(head);
		nextavail = top = (EXPR_KILLREF*) NULL;
		head = tail = (PAGE*) NULL;
		return;
	}

	if( nextavail == top ) {
		pp = getpage(EXPRKILLPAGE);
		if(tail) {
			tail->next = pp;
		}
		if(head == (PAGE*) NULL ) {
			head = pp;
		}
		tail = pp;
		nextavail = (EXPR_KILLREF*) &pp->start;
		top = (EXPR_KILLREF*) (&pp->start + 
					( sizeof(EXPR_KILLREF)*(availchar_per_page/sizeof(EXPR_KILLREF)) ));
	}
	rp =  nextavail;
	nextavail++;

# ifdef DEBUG
	rp->refno = nexprrefs++;
# endif
	rp->next_eref =  (EXPR_REF*) NULL;
	return rp;
}

WEB *
alloc_new_web(flush)
BOOLEAN flush;
{
static PAGE *head, *tail;
static WEB *nextavail, *top, *tmp;
PAGE *pp;

	if(flush == TRUE) {
		free_pages(head);
		nextavail = top = (WEB*) NULL;
		head = tail = (PAGE*) NULL;
		return;
	}

	if( nextavail != top ) {
	} else {
		pp = getpage(WEBPAGE);
		if(tail) {
			tail->next = pp;
		}
		if(head == (PAGE*) NULL ) {
			head = pp;
		}
		tail = pp;
		nextavail = (WEB*) &pp->start;
		top = (WEB*) (&pp->start + ( sizeof(WEB)*(availchar_per_page/sizeof(WEB))));
	}
	tmp =  nextavail;
	nextavail++;
	return tmp;
}

heap_reset()
{
char *heap_top;
register PAGE *pp, *nextp;
register int i;

	if(debugflag[18] == TRUE) {
		pagestats();
	}
	new_expr(TRUE);
	new_expr_ref(TRUE);
	new_expr_killref(TRUE);
	new_var_ref(TRUE);
	new_triple(TRUE);
	alloc_new_web(TRUE);
	dependentexpr_lq = cse_lq = copy_lq = reg_lq = var_lq = df_lq = proc_lq = tmp_lq = loop_lq = (LISTQ*) NULL;
	lastpg = misc_page = (PAGE*) NULL;
	misc_top = misc_nextavail = (long*) NULL;
	free_triple_lifo = TNULL;

	heap_top = (char*) sbrk(0);
	if( (heap_top - heap_start)%pgsize ) {
		quit("heap_reset : heap not pgsize multiple ");
	}
	if(debugflag[18] == TRUE) {
		printf("heap: %d pages of size %d\n",
			(heap_top-heap_start)/pgsize,pgsize);
	}
	freepage_lifo = (PAGE*) NULL;
	brk(heap_start);
}
