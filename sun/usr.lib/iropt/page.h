/*	@(#)page.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

extern int availchar_per_page;
/*
	Memory is managed in chunks called "pages" which are actually a
	multiple of the system pagesize.  A page contains structures of
	a single type except for MISC pages which hold sundry
	structures. Reuse of storage within a procedure is done at the
	page level - eg at the end of each transformation all pages
	that hold list blocks for the canreach/reachdef chains are
	added to the free page list.  At the start of a new procedure
	ALL heap storage is reclaimed. Pages which may be reused within a
	procedure  have a header with a next pointer and a "used for"
	field whereas for pages which are held until "heap_reset" the
	header space is used for storage.
*/

#define LISTPAGE 0
#define TRIPLEPAGE 1
#define EXPRPAGE 2
#define EXPRREFPAGE 3
#define VARREFPAGE 4
#define WEBPAGE 5
#define EXPRKILLPAGE 6 /*for kill expr_ref records - there are a lot of these*/
#define MISCPAGE 7
#define NPAGETAGS sizeof(npages)/sizeof(int)

typedef struct page {
	int used_for;
	struct page *next;
	char start; /* actually char start[availchar_per_page] */
} PAGE;

typedef struct listq {
	PAGE *head, *tail;
	struct list *nextavail;
	char *top;
} LISTQ;

extern PAGE *exprrefpage_fifo;
extern PAGE *freepage_lifo;

