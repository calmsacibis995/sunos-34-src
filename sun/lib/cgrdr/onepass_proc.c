#ifndef lint
static	char sccsid[] = "@(#)onepass_proc.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "cg_ir.h"
#include "opdescr.h"
#include <stdio.h>
#include <sys/file.h>

/*	this module serves as an interface between f77pass1 and the part of
	the code generator that transforms iropt ir to pcc ir
	it knows neither f77pass1 nor pcc include files
*/
HEADER hdr;
LEAF *leaf_tab;
SEGMENT *seg_tab;
BLOCK *entry_block;
int nleaves;
BOOLEAN skyflag;

FILE *dotd_fp;
LIST *leaf_hash_tab[LEAF_HASH_SIZE];

cg_proc( hdr_f, nleaves_f, leaf_tab_f, seg_tab_f, entry_block_f, sky_f )
HEADER hdr_f;
int nleaves_f;
LEAF *leaf_tab_f;
SEGMENT *seg_tab_f;
BLOCK *entry_block_f;
BOOLEAN sky_f;
{
char c;
int irfd;
BOOLEAN have_ext_entry = FALSE;
register BLOCK *bp;

	hdr = hdr_f;
	leaf_tab = leaf_tab_f;
	seg_tab = seg_tab_f;
	entry_block = entry_block_f;
	nleaves = nleaves_f;
	skyflag = (BOOLEAN) sky_f;
	for(bp = entry_block; bp; bp = bp->next) {
		if(bp->is_ext_entry == TRUE) have_ext_entry = TRUE;
	}
	if(!have_ext_entry || nleaves == 0) return;

	free_ir_alloc_list();
	proc_init();
	map_to_pcc();
}
