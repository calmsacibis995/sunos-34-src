#ifndef lint
static	char sccsid[] = "@(#)goff.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*	 utility routine to turn off the g bit in relocatable files thereby hiding
	externals - used to build a onepass.o that lets the code generator 
	and f77pass1 can coexist within one executable
*/
# include <sys/types.h>
# include <stdio.h>
# include <a.out.h>
# include <stab.h>

#define YES 1
#define NO 0
char *exceptions[] =  {
		"_cg_init",
		"_cg_end",
		"_cg_proc"
};
int nex = sizeof(exceptions)/sizeof(char*);

static char *strp;
int dumpnames=0;

main(argc, argv)
char **argv;
{
char *fn, *scan_args();
FILE *fp;

	fn = scan_args(argc,argv);
	fp = fopen(fn,"r+");
	if (fp == NULL) {
		perror(fn);
		exit(1);
	}
	read_nlist(fp);
	return(0);
}

read_nlist(fp)
FILE *fp;
{
off_t offset, strsiz;
struct exec ex, *exp = &ex;
struct nlist sym;
register struct nlist *symp = &sym;
int i, n, here;

	fread(exp, sizeof(struct exec), 1, fp);
	n = exp->a_syms / sizeof(struct nlist);

	offset = N_STROFF(*exp);
	fseek(fp, offset, 0);
	fread(&strsiz, sizeof(strsiz), 1, fp);
	strp = (char*) malloc(strsiz);
	if(fread(strp+sizeof(strsiz), strsiz-sizeof(strsiz),1, fp) != 1) {
		quit("error reading string table");
	}

	offset = N_SYMOFF(*exp);
	fseek(fp, offset, 0);
	for(i=0; i<n; i++) {
		if(fread(symp, sizeof(struct nlist), 1, fp) != 1) {
			quit("error reading name list ");
		}
		if(change_sym(symp)) {
			offset=ftell(fp);
			fseek(fp, -sizeof(struct nlist), 1);
			if( fwrite(symp, sizeof(struct nlist), 1, fp) != 1) {
				quit("error writing  name list ");
			}
			fseek(fp, offset, 0);
		}
	}
}

change_sym(symp)
register struct nlist *symp;
{
register char c, *cp, **exp;

	c = symp->n_type;
	/* if its an external and text data or bss  look at it */
	if( ( c & N_EXT) && 
		((c & N_TYPE) & (N_TEXT | N_DATA | N_BSS)) 
		) {
		if (symp->n_un.n_strx) {
			symp->n_un.n_name = symp->n_un.n_strx + strp;
		} else {
			symp->n_un.n_name = "";
		}

		for(exp=exceptions;exp< &exceptions[nex];exp++) {
			if(strcmp(symp->n_un.n_name,*exp) == 0) {
				symp->n_un.n_strx = symp->n_un.n_name - strp;
				return NO;
			}
		}

		/* omit file name symbols, eg, "  t foo.o" */
		for (cp=symp->n_un.n_name; *cp; cp++) {
		  if (*cp == '.') {
			symp->n_un.n_strx = symp->n_un.n_name - strp;
			return NO;
			}
		}
		symp->n_type &= ~N_EXT;
		if(dumpnames) {
			psym(symp);
		}
		if(*(symp->n_un.n_name) == '\0') {
			symp->n_un.n_strx = 0;
		} else {
			symp->n_un.n_strx = symp->n_un.n_name - strp;
		}
		return YES;
	} else {
		return NO;
	}
}

static struct val_name_pair { int val; char * name; } names[] = {
	{N_UNDF, "N_UNDF"},
	{N_ABS , "N_ABS" },
	{N_TEXT, "N_TEXT"},
	{N_DATA , "N_DATA"},
	{N_BSS , "N_BSS"},
	{N_COMM , "N_COMM"},
	{N_FN , "N_FN"},
	{N_EXT , "N_EXT"},
};
#define NOFNAMES sizeof(names)/sizeof(struct val_name_pair)

psym(p)
register struct nlist *p;
{
register char *cp;
int c;
int i;

	c = p->n_type;
	c &= N_TYPE;
	for(i=0; i<NOFNAMES; i++) {
		if(c  == names[i].val) printf(" %s ",names[i].name);
	}
	printf(" %s\n", p->n_un.n_name);
}

quit(s){ fprintf(stderr,"%s\n",s); exit(1); }

char *
scan_args(argc, argv)
int argc;
char **argv;
{
	if (--argc>0 && argv[1][0]=='-' && argv[1][1]!=0) {
		argv++;
		while (*++*argv) switch (**argv) {
			case 'd':
				dumpnames++;
				break;
		}
	}
	if(argc == 0 ) {
		quit("usage:	goff [-d] stat.file");
	}
	return *(++argv);
}
