#ifndef lint
static	char sccsid[] = "@(#)do_ir_archive.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cg_ir.h"
#include "opdescr.h"
#include <stdio.h>
#include <sys/file.h>

char *ir_file; 
BOOLEAN read_ir();

HEADER hdr;
BOOLEAN skyflag;
FILE *dotd_fp;
LIST *leaf_hash_tab[LEAF_HASH_SIZE];
extern int stmtprofflag;

do_ir_archive(fn, dump, sky)
char *fn;
int dump, sky;
{
char c;
int irfd;

	ir_file = fn;
	skyflag = (BOOLEAN) sky;
	irfd = open(ir_file,O_RDONLY,0);
	if (irfd == -1) {
		perror(ir_file);
		quit("can't open ir file");
	}
	while( read(irfd, &hdr, sizeof(HEADER)) == sizeof(HEADER) ) {
		free_ir_alloc_list();
		if(read_ir(irfd) == TRUE) {
			if(dump) {
				dump_segments();
				dump_blocks();
				dump_leaves();
				dump_triples();
			}
			map_to_pcc();
		}
	}
	if(stmtprofflag) {
		stmtprof_eof();
	}
}
