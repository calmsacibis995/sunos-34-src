/*
 * egrep -- print lines containing (or not containing) a regular expression
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */
%token CHAR DOT CCL NCCL OR CAT STAR PLUS QUEST
%left OR
%left CHAR DOT CCL NCCL '('
%left CAT
%left STAR PLUS QUEST

%{
#ifndef lint
static	char sccsid[] = "@(#)egrep.y 1.1 86/09/24 SMI"; /* from S5R2 1.7 */
#endif

#include <stdio.h>
#include <ctype.h>

#define MAXLIN 350
#define MAXPOS 4000
#define NCHARS 128
#define NSTATES 128
#define FINAL -1
char gotofn[NSTATES][NCHARS];
int state[NSTATES];
char out[NSTATES];
int line = 1;
int name[MAXLIN];
int left[MAXLIN];
int right[MAXLIN];
int parent[MAXLIN];
int foll[MAXLIN];
int positions[MAXPOS];
char chars[MAXLIN];
int nxtpos;
int nxtchar = 0;
int tmpstat[MAXLIN];
int initstat[MAXLIN];
int xstate;
int count;
int icount;
char *input;

long	lnum;
int	bflag;
int	cflag;
int	iflag;
int	fflag;
int	lflag;
int	nflag;
int	vflag;
int	eflag;
int	nfile;
extern 	char *optarg;
extern	int optind;
FILE	*expfile;
FILE	*fptr;
int	blkno;
long	tln;
int	nsucc;

int	f;
char	*fname;
%}

%%
s:	t
		={ unary(FINAL, $1);
		  line--;
		}
	;
t:	b r
		={ $$ = node(CAT, $1, $2); }
	| OR b r OR
		={ $$ = node(CAT, $2, $3); }
	| OR b r
		={ $$ = node(CAT, $2, $3); }
	| b r OR
		={ $$ = node(CAT, $1, $2); }
	;
b:
		={ $$ = enter(DOT);
		   $$ = unary(STAR, $$); }
	;
r:	CHAR
		={ $$ = enter($1); }
	| DOT
		={ $$ = enter(DOT); }
	| CCL
		={ $$ = cclenter(CCL); }
	| NCCL
		={ $$ = cclenter(NCCL); }
	;

r:	r OR r
		={ $$ = node(OR, $1, $3); }
	| r r %prec CAT
		={ $$ = node(CAT, $1, $2); }
	| r STAR
		={ $$ = unary(STAR, $1); }
	| r PLUS
		={ $$ = unary(PLUS, $1); }
	| r QUEST
		={ $$ = unary(QUEST, $1); }
	| '(' r ')'
		={ $$ = $2; }
	| error 
	;

%%
yyerror(s) {
	fprintf(stderr, "egrep: %s\n", s);
	exit(2);
}

yylex() {
	extern int yylval;
	int cclcnt, x;
	register char c, d;
	switch(c = nextch()) {
		case '$':
		case '^': c = '\n';
			goto defchar;
		case '|': return (OR);
		case '*': return (STAR);
		case '+': return (PLUS);
		case '?': return (QUEST);
		case '(': return (c);
		case ')': return (c);
		case '.': return (DOT);
		case '\0': return (0);
		case '\n': return (OR);
		case '[': 
			x = CCL;
			cclcnt = 0;
			count = nxtchar++;
			if ((c = nextch()) == '^') {
				x = NCCL;
				c = nextch();
			}
			do {
				if (c == '\0') synerror();
				if (c == '-' && cclcnt > 0 && chars[nxtchar-1] != 0) {
					if ((d = nextch()) != 0) {
						c = chars[nxtchar-1];
						while (c < d) {
							if (nxtchar >= MAXLIN) overflo();
							chars[nxtchar++] = ++c;
							cclcnt++;
						}
						continue;
					}
				}
				if (nxtchar >= MAXLIN) overflo();
				chars[nxtchar++] = c;
				cclcnt++;
			} while ((c = nextch()) != ']');
			chars[count] = cclcnt;
			return (x);
		case '\\':
			if ((c = nextch()) == '\0') synerror();
		defchar:
		default: yylval = c; return (CHAR);
	}
}
nextch() {
	register int c;
	if (fflag) {
		if ((c = getc(expfile)) == EOF) {
			if (ferror(expfile)) {
				fprintf(stderr, "egrep: Read error on ");
				perror(fname);
				exit(1);
			}
			fclose(expfile);
			return(0);
		}
	}
	else c = *input++;
	return(c);
}

synerror() {
	fprintf(stderr, "egrep: syntax error\n");
	exit(2);
}

enter(x) int x; {
	if(line >= MAXLIN) overflo();
	name[line] = x;
	left[line] = 0;
	right[line] = 0;
	return(line++);
}

cclenter(x) int x; {
	register linno;
	linno = enter(x);
	right[linno] = count;
	return (linno);
}

node(x, l, r) {
	if(line >= MAXLIN) overflo();
	name[line] = x;
	left[line] = l;
	right[line] = r;
	parent[l] = line;
	parent[r] = line;
	return(line++);
}

unary(x, d) {
	if(line >= MAXLIN) overflo();
	name[line] = x;
	left[line] = d;
	right[line] = 0;
	parent[d] = line;
	return(line++);
}
overflo() {
	fprintf(stderr, "egrep: regular expression too long\n");
	exit(2);
}

cfoll(v) {
	register i;
	if (left[v] == 0) {
		count = 0;
		for (i=1; i<=line; i++) tmpstat[i] = 0;
		follow(v);
		add(foll, v);
	}
	else if (right[v] == 0) cfoll(left[v]);
	else {
		cfoll(left[v]);
		cfoll(right[v]);
	}
}
cgotofn() {
	register c, i, k;
	int n, s;
	char symbol[NCHARS];
	int j, nc, pc, pos;
	int curpos, num;
	int number, newpos;
	count = 0;
	for (n=3; n<=line; n++) tmpstat[n] = 0;
	if (cstate(line-1)==0) {
		tmpstat[line] = 1;
		count++;
		out[0] = 1;
	}
	for (n=3; n<=line; n++) initstat[n] = tmpstat[n];
	count--;		/*leave out position 1 */
	icount = count;
	tmpstat[1] = 0;
	add(state, 0);
	n = 0;
	for (s=0; s<=n; s++)  {
		if (out[s] == 1) continue;
		for (i=0; i<NCHARS; i++) symbol[i] = 0;
		num = positions[state[s]];
		count = icount;
		for (i=3; i<=line; i++) tmpstat[i] = initstat[i];
		pos = state[s] + 1;
		for (i=0; i<num; i++) {
			curpos = positions[pos];
			if ((c = name[curpos]) >= 0) {
				if (c < NCHARS) symbol[c] = 1;
				else if (c == DOT) {
					for (k=0; k<NCHARS; k++)
						if (k!='\n') symbol[k] = 1;
				}
				else if (c == CCL) {
					nc = chars[right[curpos]];
					pc = right[curpos] + 1;
					for (k=0; k<nc; k++) symbol[chars[pc++]] = 1;
				}
				else if (c == NCCL) {
					nc = chars[right[curpos]];
					for (j = 0; j < NCHARS; j++) {
						pc = right[curpos] + 1;
						for (k = 0; k < nc; k++)
							if (j==chars[pc++]) goto cont;
						if (j!='\n') symbol[j] = 1;
						cont:;
					}
				}
				else printf("something's funny\n");
			}
			pos++;
		}
		for (c=0; c<NCHARS; c++) {
			if (symbol[c] == 1) { /* nextstate(s,c) */
				count = icount;
				for (i=3; i <= line; i++) tmpstat[i] = initstat[i];
				pos = state[s] + 1;
				for (i=0; i<num; i++) {
					curpos = positions[pos];
					if ((k = name[curpos]) >= 0)
						if (
							(k == c)
							| (k == DOT)
							| (k == CCL && member(c, right[curpos], 1))
							| (k == NCCL && member(c, right[curpos], 0))
						) {
							number = positions[foll[curpos]];
							newpos = foll[curpos] + 1;
							for (k=0; k<number; k++) {
								if (tmpstat[positions[newpos]] != 1) {
									tmpstat[positions[newpos]] = 1;
									count++;
								}
								newpos++;
							}
						}
					pos++;
				} /* end nextstate */
				if (notin(n)) {
					if (n >= NSTATES) overflo();
					add(state, ++n);
					if (tmpstat[line] == 1) out[n] = 1;
					gotofn[s][c] = n;
				}
				else {
					gotofn[s][c] = xstate;
				}
			}
		}
	}
}

cstate(v) {
	register b;
	if (left[v] == 0) {
		if (tmpstat[v] != 1) {
			tmpstat[v] = 1;
			count++;
		}
		return(1);
	}
	else if (right[v] == 0) {
		if (cstate(left[v]) == 0) return (0);
		else if (name[v] == PLUS) return (1);
		else return (0);
	}
	else if (name[v] == CAT) {
		if (cstate(left[v]) == 0 && cstate(right[v]) == 0) return (0);
		else return (1);
	}
	else { /* name[v] == OR */
		b = cstate(right[v]);
		if (cstate(left[v]) == 0 || b == 0) return (0);
		else return (1);
	}
}


member(symb, set, torf) {
	register i, num, pos;
	num = chars[set];
	pos = set + 1;
	for (i=0; i<num; i++)
		if (symb == chars[pos++]) return (torf);
	return (!torf);
}

notin(n) {
	register i, j, pos;
	for (i=0; i<=n; i++) {
		if (positions[state[i]] == count) {
			pos = state[i] + 1;
			for (j=0; j < count; j++)
				if (tmpstat[positions[pos++]] != 1) goto nxt;
			xstate = i;
			return (0);
		}
		nxt: ;
	}
	return (1);
}

add(array, n) int *array; {
	register i;
	if (nxtpos + count > MAXPOS) overflo();
	array[n] = nxtpos;
	positions[nxtpos++] = count;
	for (i=3; i <= line; i++) {
		if (tmpstat[i] == 1) {
			positions[nxtpos++] = i;
		}
	}
}

follow(v) int v; {
	int p;
	if (v == line) return;
	p = parent[v];
	switch(name[p]) {
		case STAR:
		case PLUS:	cstate(v);
				follow(p);
				return;

		case OR:
		case QUEST:	follow(p);
				return;

		case CAT:	if (v == left[p]) {
					if (cstate(right[p]) == 0) {
						follow(p);
						return;
					}
				}
				else follow(p);
				return;
		case FINAL:	if (tmpstat[line] != 1) {
					tmpstat[line] = 1;
					count++;
				}
				return;
	}
}


main(argc, argv)
char **argv;
{
	register c;
	char *usage;
	register char *arg;
	int errflg = 0;
	usage = "[ -bcilnv ] [ -e exp ] [ -f file ] [ strings ] [ file ] ...";

	while(( c = getopt(argc, argv, "bcie:f:lnv")) != EOF)
		switch(c) {

		case 'b':
			bflag++;
			continue;

		case 'c':
			cflag++;
			continue;

		case 'i':
			iflag++;
			continue;

		case 'e':
			eflag++;
			input = optarg;
			continue;

		case 'f':
			if (fname != NULL) {
				fprintf(stderr, "egrep: Only one \"-f\" option allowed\n");
				exit(2);
			}
			fflag++;
			fname = optarg;
			expfile = fopen(fname,"r");
			if (expfile == NULL) {
				fprintf(stderr, "egrep: ");
				perror(fname);
				exit(2);
			}
			continue;

		case 'l':
			lflag++;
			continue;

		case 'n':
			nflag++;
			continue;

		case 'v':
			vflag++;
			continue;

		case '?':
			errflg++;
	}

	argc -= optind;
	if (errflg || ((argc <= 0) && !fflag)) {
		printf("usage: egrep %s\n",usage);
		exit(2);
	}
	if ( !eflag  && !fflag ) {
		input = argv[optind];
		optind++;
		argc--;
	}

	if (iflag) {
		for (arg = input; *arg != NULL; ++arg)
			*arg = (char)tolower((int)(*arg));
	}

	argv = &argv[optind];
	if(argc > 0)
		if (access(*argv, 0) != 0) {
			fprintf(stderr, "egrep: %s does not exist\n", *argv);
			exit(2);
		}
	yyparse();

	cfoll(line-1);
	cgotofn();
	nfile = argc;
	if (argc<=0) {
		if (lflag) exit(1);
		execute((char *)NULL);
	}
	else
		while ( --argc >= 0 ) {
			execute(*argv);
			argv++;
		}
	exit((nsucc == 2) ? 2 : (nsucc == 0));
}

execute(file)
char *file;
{
	register char *p;
	register cstat;
	register ccount;
	char buf[2*BUFSIZ];
	char *nlp;
	int istat;
	int in_line;
	if (file) {
		if ((fptr = fopen(file, "r")) == NULL) {
			fprintf(stderr, "egrep: ");
			perror(file);
			nsucc = 2;
			return;
		}
	}
	else
		fptr = stdin;
	ccount = 0;
	lnum = 1;
	tln = 0;
	in_line = 0;
	blkno = 0;
	p = buf;
	nlp = p;
	if ((ccount = fread(p, sizeof(char), BUFSIZ, fptr)) <= 0) goto done;
	in_line = 1;
	istat = cstat = gotofn[0]['\n'];
	if (out[cstat]) goto found;
	for (;;) {
		if (!iflag)
			cstat = gotofn[cstat][*p&0377];	/* all input chars made positive */
		else
			cstat = gotofn[cstat][tolower((int)*p&0377)]; /* for -i option*/
		if (out[cstat]) {
		found:	for(;;) {
				if (*p++ == '\n') {
					in_line = 0;
					if (vflag == 0) {
				succeed:	nsucc = (nsucc == 2) ? 2 : 1;
						if (cflag) tln++;
						else if (lflag) {
							printf("%s\n", file);
							fclose(fptr);
							return;
						}
						else {
							if (nfile > 1) printf("%s:", file);
							if (bflag) printf("%d:", blkno);
							if (nflag) printf("%ld:", lnum);
							if (p <= nlp) {
								while (nlp < &buf[2*BUFSIZ]) putchar(*nlp++);
								nlp = buf;
							}
							while (nlp < p) putchar(*nlp++);
						}
					}
					lnum++;
					nlp = p;
					if ((out[(cstat=istat)]) == 0) goto brk2;
				}
				cfound:
				if (--ccount <= 0) {
					if (p <= &buf[BUFSIZ]) {
						ccount = fread(p, sizeof(char), BUFSIZ, fptr);
					}
					else if (p == &buf[2*BUFSIZ]) {
						p = buf;
						ccount = fread(p, sizeof(char), BUFSIZ, fptr);
					}
					else {
						ccount = fread(p, sizeof(char), &buf[2*BUFSIZ] - p, fptr);
					}
					if (ccount <= 0) {
						if (ferror(fptr))
							goto done;
						if (in_line && (vflag == 0)) {
							in_line = 0;
							goto succeed;
						}
						goto done;
					}
					blkno++;
				}
				in_line = 1;
			}
		}
		if (*p++ == '\n') {
			in_line = 0;
			if (vflag) goto succeed;
			else {
				lnum++;
				nlp = p;
				if (out[(cstat=istat)]) goto cfound;
			}
		}
		brk2:
		if (--ccount <= 0) {
			if (p <= &buf[BUFSIZ]) {
				ccount = fread(p, sizeof(char), BUFSIZ, fptr);
			}
			else if (p == &buf[2*BUFSIZ]) {
				p = buf;
				ccount = fread(p, sizeof(char), BUFSIZ, fptr);
			}
			else {
				ccount = fread(p, sizeof(char), &buf[2*BUFSIZ] - p, fptr);
			}
			if (ccount <= 0) {
				if (ferror(fptr))
					goto done;
				if (in_line && vflag) {
					in_line = 0;
					goto succeed;
				}
				break;
			}
			blkno++;
		}
		in_line = 1;
	}
done:	if (ccount <= 0 && ferror(fptr)) {
		fprintf(stderr, "egrep: Read error on ");
		perror(file ? file : "standard input");
		nsucc = 2;
		fclose(fptr);
		return;
	}
	fclose(fptr);
	if (cflag) {
		if (nfile > 1)
			printf("%s:", file);
		printf("%ld\n", tln);
	}
}
