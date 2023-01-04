#ifndef lint
static	char sccsid[] = "@(#)misc.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cg_ir.h"
#include <stdio.h>

union leaf_location{
	ADDRESS *ap;
	struct constant *constp;
};

static char *ir_alloc_list = (char*) NULL;
char *ckalloc();

/*
**	binary constants are usually treated as byte strings - this is
**	for when it is necessary to deal with them as aligned data
*/
l_align(cp1)
register char *cp1;
{
union {
	char c[4];
	int w;
} word;
register char *cp2 = word.c;
register int i;

	for(i=0;i<4;i++){
		*cp2++ = *cp1++;
	}
	return(word.w);
}

char *
ckalloc(size)
{
register char *header, *start;

	header = (char*) malloc(size+sizeof(char*));
	*((char**)header) = ir_alloc_list;
	ir_alloc_list = header;
	start = header+sizeof(char*);
	bzero(start, size);
	return start;
}

free_ir_alloc_list()
{
register char *cp;

	if(ir_alloc_list) {
		for(cp=ir_alloc_list; cp; cp = *((char**)cp) ) {
			free(cp);
		}
		ir_alloc_list = (char*) NULL;
	}
}

quit(msg)
char *msg;
{
	fprintf(stderr,"%s\n",msg);
	exit(1);
}

LIST *
new_list()
{
LIST *lp;
	lp = (LIST*) ckalloc(sizeof(LIST));
	lp->next = lp;
}

type_align(type)
TYPE type;
{
static int type_alignment[NTYPES] = {
	
	/*UNDEF*/	0,
	/*FARG*/	4,
	/*CHAR*/	1,
	/*SHORT*/	2,
	/*INT*/		4,
	/*LONG*/	4,
	/*FLOAT*/	4,
	/*DOUBLE*/	4,
	/*STRTY*/	4,
	/*UNIONTY*/	4,
	/*ENUMTY*/	4,
	/*MOETY*/	0,
	/*UCHAR*/	1,
	/*USHORT*/	2,
	/*UNSIGNED*/	4,
	/*ULONG*/	4,
	
	/*BOOL*/	4,
	/*EXTENDEDF*/	4,
	/*COMPLEX*/	4,
	/*DCOMPLEX*/ 	4,
	/*STRING*/	1,
	/*VOID*/	0 
};
	if(ISPTR(type.tword)) {
		return(type_alignment[ULONG]);
	} else if (BTYPE(type.tword) != type.tword) {
		quita("type_align: alignment of constructed type >%X<",type.tword);
	} else {
		return(type_alignment[type.tword]);
	}
}

type_size(type)
TYPE type;
{
int size;
static int type_sizes[NTYPES] = {
	
	/*UNDEF*/	0,
	/*FARG*/	0,
	/*CHAR*/	1,
	/*SHORT*/	2,
	/*INT*/		4,
	/*LONG*/	4,
	/*FLOAT*/	4,
	/*DOUBLE*/	8,
	/*STRTY*/	0,
	/*UNIONTY*/	0,
	/*ENUMTY*/	0,
	/*MOETY*/	0,
	/*UCHAR*/	1,
	/*USHORT*/	2,
	/*UNSIGNED*/4,
	/*ULONG*/	4,
	
	/*BOOL*/	4,
	/*EXTENDEDF*/	0,
	/*COMPLEX*/	8,
	/*DCOMPLEX*/ 16,
	/*STRING*/	0,
	/*VOID*/	0 
};

	if(ISPTR(type.tword)) {
		size = type_sizes[ULONG];
	} else if (BTYPE(type.tword) != type.tword) {
		quita("type_size: size of constructed type >%X<",type.tword);
	} else {
		size = type_sizes[type.tword];
		if(size == 0) {
			if(type.tword == STRTY || type.tword == STRING ) {
				size = type.aux.size;
			} else {
				quita("type_size: don't know the size of type >%X<",type.tword);
			}
		}
	}
	return(size);
}

new_label()
{
static label_counter =0;
	return(++label_counter);
}
 
quita(str,arg)
char *str,*arg;
{
char buf[132];
	sprintf(buf,str,arg);
	quit(buf);
}

void
delete_list( tail, lp ) register LIST **tail, *lp;
{
	LIST *lp1_prev, *lp1;
	
	if( lp == LNULL )
		return;
	if( lp->next == lp) {
		*tail = LNULL;
		return;
	}

	/* else, search the *tail list for lp */
	lp1_prev = *tail;
	
	LFOR( lp1, (*tail)->next ) {
		if( lp1 == lp ) {
			lp1_prev->next = lp->next;
			if( *tail == lp ) { /* deleting the tail */
				*tail = lp1_prev;
			}
			return;
		} else {
			lp1_prev = lp1;
		}
	}
	/*  can NOT find lp in *tail list */
	quit("delete_list: can not find the item >%X< in the list", lp );
}

roundup(i1,i2)
{
	return (i2*((i1+i2-1)/i2));
}


LEAF *
ileaf(i)
int i;
{
struct constant const;
union const_u c;
TYPE t;

	t.tword = INT;
	t.aux.size = 0;
	const.isbinary = FALSE;
	const.c.i=i;
	return( leaf_lookup(CONST_LEAF,t,&const));
}

LEAF *
leaf_lookup(class,type,location)
LEAF_CLASS class;
TYPE type;
union leaf_location location;
{
register LEAF *leafp;
register LIST *hash_listp;
register SEGMENT *sp;
register ADDRESS *ap,*ap2;
register struct constant *constp; 
int index;
LIST *new_l, *lp;

	if (class == VAR_LEAF || class == ADDR_CONST_LEAF)  {
		ap = location.ap;
		sp = ap->seg;
	} else {
		constp = location.constp;
	}
	index = hash_leaf(class,location);

	LFOR(hash_listp,leaf_hash_tab[index]) {
		if(hash_listp && LCAST(hash_listp,LEAF)->class == class )  {
			leafp = (LEAF*)hash_listp->datap;
			if(same_irtype(leafp->type,type)==FALSE) continue;
			if(class == VAR_LEAF || class == ADDR_CONST_LEAF) {
				ap2 = &leafp->val.addr;
				if(ap2->seg == ap->seg &&
			   		ap2->offset == ap->offset) 
			   		return(leafp);
				else continue;
			} else {
				if ( B_ISINT(type.tword) || B_ISBOOL(type.tword)) {
					if(leafp->val.const.c.i == constp->c.i) return(leafp);
					continue;
				} else if ( B_ISCHAR(type.tword) ) {
					if(strcmp(leafp->val.const.c.cp,constp->c.cp) == 0) return(leafp);
					continue;
				}
				quita(" no literal pool for tword %X",type.tword);
				continue;
			}
		}
	}

	leafp = (LEAF*) ckalloc(sizeof(LEAF));
	leafp->tag = ISLEAF;
	leafp->leafno = nleaves++;
	leafp->type = type;
	leafp->class=class;
	if(leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF ) {
		leafp->val.addr = *ap;
		lp = new_list();
		(LEAF*) lp->datap = leafp;
		LAPPEND(sp->leaves,lp);
	} else {
		leafp->val.const = *constp;
	}
	new_l = new_list();
	(LEAF*) new_l->datap = leafp;
	LAPPEND(leaf_hash_tab[index], new_l);
	return(leafp);
}


int
hash_leaf(class,location)
LEAF_CLASS class;
union leaf_location location;
{
register unsigned key;
register char *cp;
register SEGMENT *sp;
register ADDRESS *ap;
register struct constant *constp;

key = 0;

if (class == VAR_LEAF || class == ADDR_CONST_LEAF)  {
	ap = location.ap;
	sp = ap->seg;
	if(sp->descr.builtin == BUILTIN_SEG) {
		key = ( ( (int) sp->descr.class << 16) | (ap->offset) );
	} else {
		for(cp=sp->name;*cp!='\0';cp++) key += *cp;
		key = ( (key << 16) | (ap->offset) );
	}
} else { /* class == CONST_LEAF */
	constp = location.constp;
	key = (unsigned) constp->c.i; 
}

key %= LEAF_HASH_SIZE;
return( (int) key);
}

BOOLEAN
same_irtype(p1,p2)
TYPE p1,p2;
{
	if(p1.tword == p2.tword ) {
		if(BTYPE(p1.tword) == STRTY ) {
			if(p1.aux.size != p2.aux.size) return(FALSE);
		} else if( ISFTN(p1.tword) ){
			if(p1.aux.func_descr != p2.aux.func_descr) return(FALSE);
		} else {
			return(TRUE);
		}
	}
	return(FALSE);
}

proc_init()
{
register LIST **lpp, **lptop;

	lptop = &leaf_hash_tab[LEAF_HASH_SIZE];
	for(lpp=leaf_hash_tab; lpp < lptop;) *lpp++ = LNULL;
}
