#ifndef lint 
static char sccsid[] = "@(#)rpc_main.c 1.1 86/09/25 (C) 1986 SMI";
#endif
 
/*
 * rpc_main.c, Top level of the RPC protocol compiler.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <strings.h>
#include <sys/file.h>
#include "rpc_util.h"
#include "rpc_parse.h"
#include "rpc_scan.h"

static cflag;
static hflag;
static sflag;
static char *cmdname;

main(argc,argv)
    int argc;
    char *argv[];
{

	cmdname = argv[0];
	if (! parseargs(argc,argv)) {
		fprintf(stderr,
			"usage: %s infile\n",cmdname);
		fprintf(stderr,
			"       %s [-c | -h] [-o outfile] [infile]\n",cmdname);
		fprintf(stderr,
			"       %s [-s udp|tcp]* [-o outfile] [infile]\n",
			cmdname);
		exit(1);
	}
	if (cflag) {
		open_input(infile);
		open_output(outfile,(char*)NULL);
		c_output();
	} else if (hflag) {
		open_input(infile);
		open_output(outfile,(char*)NULL);
		h_output();
	} else if (sflag) {
		open_input(infile);
		open_output(outfile,(char*)NULL);
		s_output(argc,argv);
	} else {
		open_input(infile);
		open_output(infile,".c");
		c_output();	
		outfile2 = outfile;
		reinitialize();
		open_input(infile);	
		open_output(infile,".h");
		h_output();	
	}
}

char *
extend(file,ext)
	char *file;
	char *ext;
{
	char *res;
	char *p;

	res = alloc(strlen(file) + strlen(ext) + 1);
	if (res == NULL) {
		abort();	
	}
	p = rindex(file,'.');
	if (p == NULL) {
		return(NULL);
	}
	strcpy(res,file);
	strcpy(res + (p - file),ext);
	return(res);
}

	
open_output(file,ext)
	char *file;
	char *ext;
{

	if (file == NULL) {
		fout = stdout;
		return;
	}
	if (ext != NULL) {
		if (! (outfile = extend(file, ext))) {
			fprintf(stderr,"%s: %s has no extension\n",cmdname,file);
			crash();	
		}
	} else {
		outfile = file;
	}
	if (infile != NULL && streq(outfile,infile)) {
		fprintf(stderr,"%s: output would overwrite %s\n",cmdname,infile);
		outfile = NULL;
		crash();
	}
	fout = fopen(outfile,"w");	
	if (fout == NULL) {
		fprintf(stderr,"%s: unable to open ",cmdname);
		perror(outfile);
		crash();	
	}
}

open_input(file)
	char *file;
{
	if (file == NULL) {
		fin = stdin;
		return;
	}
	infile = file;
	fin = fopen(infile,"r");
	if (fin == NULL) {
		fprintf(stderr,"%s: unable to open ",cmdname);
		perror(infile);
		crash();	
	}
}


c_output()
{
	definition *def;
	char *include;	

	fprintf(fout,"#include <rpc/rpc.h>\n");
	if (infile && (include = extend(infile,".h"))) {
		fprintf(fout,"#include \"%s\"\n",include);
		free(include);
	}
	scanprint(OFF);
	while (def = get_definition()) {
		emit(def);
	}
}

h_output()
{
	definition *def;

	scanprint(ON);	
	while (def = get_definition()) {
		print_datadef(def);
	}
	print_funcdefs();
}

s_output(argc,argv)
	int argc;
	char *argv[];
{
	char *include;	

	scanprint(OFF);
	fprintf(fout,"#include <stdio.h>\n");
	fprintf(fout,"#include <rpc/rpc.h>\n");
	if (infile && (include = extend(infile,".h"))) {
		fprintf(fout,"#include \"%s\"\n",include);
		free(include);
	}
	while (get_definition()) 
		;
	write_most();
	do_registers(argc,argv);	
	write_rest();
}


do_registers(argc,argv)
	int argc;
	char *argv[];
{
	int i;

	for (i = 1; i < argc; i++) {
		if (streq(argv[i],"-s")) {
			write_register(argv[i+1]);
			i++;
		}
	}
}

static
parseargs(argc,argv)
	int argc;
	char *argv[];
{
	int i;
	int j;
	char c;
	char flag[(1 << 8*sizeof(char))];

	if (argc < 2) {
		return(0);
	}

	flag['c'] = 0;
	flag['h'] = 0;
	flag['s'] = 0;
	flag['o'] = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (infile) {
				return(0);
			}
			infile = argv[i];
		} else {
			for (j = 1; argv[i][j] != 0; j++) {
				switch (c = argv[i][j]) {
				case 'c':	
				case 'h':
					if (flag[c]) {
						return(0);
					}
					flag[c] = 1;	
					break;
				case 'o':	
				case 's':
					if (argv[i][j-1] != '-' || argv[i][j+1] != 0) {
						return(0);
					}
					flag[c] = 1;
					if (++i == argc) {
						return(0);
					}
					if (c == 's') {
						if (! streq(argv[i],"udp") &&
								! streq(argv[i],"tcp")) {
							return(0);
						}
					} else if (c == 'o') {
						if (outfile) {
							return(0);
						}
						outfile = argv[i];
					}
					goto nextarg;	
	
				default:
					return(0);
				}
			}
		nextarg:
			;
		}
	}
	cflag = flag['c'];
	hflag = flag['h'];
	sflag = flag['s'];
	if (! cflag && ! hflag && !sflag && (infile == NULL || outfile != NULL)) {
		return(0);
	}
	return(1);
}
		
