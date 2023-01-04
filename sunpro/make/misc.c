#ifndef lint
static char sccsid[]= "@(#)misc.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc	[Remotely from S5R2]
 */

#include "defs.h"
#include "funny.h"

/*
 *	Hash name strings to nameblocks
 */
Name
getname_fn(name, len, options)
	char			*name;
	register int		len;
	register int		options;
{
	register unsigned int	hashsum= 0;
	register int		length;
	register Name		*hashslot;
	register char		*cap= name;
	register Name		np;
	static struct Name	empty_Name;
	register char		*cbp;

	/* First figure out how long the string is. If the len argument is -1 */
	/* we count the chars here */
	if (len == FIND_LENGTH) {
		for (; *cap != NUL; hashsum <<= 1, hashsum += *cap++);
		length= cap - name;
	} else {
		length= len;
		for (; --len >= 0; hashsum <<= 1, hashsum += *cap++);
	};
	/* Run down the chain looking for the string */
	for (hashslot= &hashtab[hashsum % HASHSIZE], np= *hashslot;
	     (np != NULL) && ((len= np->hash.length) <= length);
	     hashslot= &(np->next), np= *hashslot) {
		if ((hashsum == np->hash.sum) && (length == len)) {
			compare_string(np->string, name, length, not_this_one, cap, cbp);
			/* We found it. Just return the Name */
			if (options & FILE_TYPE)
				np->stat.is_file= true;
			return(np);
		};
not_this_one:	;
	};
	if (options & DONT_ENTER)
		return(NULL);
	/* New name. Enter it */
	np= alloc(Name);
	*np= empty_Name;
	/* Get some memory for the namestring and copy it */
	np->string= Malloc(length + 1);
	copy_string(np->string, name, length, cap, cbp);
	/* Fill in the new Name */
	np->string[length]= NUL;
	np->hash.length= length;
	np->hash.sum= hashsum;
	np->stat.time= FILE_NO_TIME;
	if (*hashslot != NULL)
		np->next= *hashslot;
	*hashslot= np;
	/* Scan the namestring to classify it */
	for (cap= name, len= 0; --length >= 0;)
		len |= funny[*cap++];
	np->dollar= boolean((len & DOLLAR_FUNNY) != 0);
	np->meta= boolean((len & META_FUNNY) != 0);
	np->percent= boolean((len & PERCENT_FUNNY) != 0);
	np->wildcard= boolean((len & WILDCARD_FUNNY) != 0);
	np->stat.is_file= boolean(options & FILE_TYPE);
	return(np);
}

/*
 *	Cover funtion for free() to make it possible to insert advises
 */
void
Free(p)
	register char		*p;
{
	(void)free(p);
}

/*
 *	malloc() version that checks the returned value
 */
char                   *
Malloc(size)
	register int		size;
{
	extern char            *malloc();
	register char          *result= malloc((unsigned) size);

	if (result == NULL)
		fatal("Out of memory");
	return(result);
}

/*
 *	String manipulation
 */
/*
 *	expand_string() allocates more memory for strings that run out
 */
void
expand_string(string, length)
	register String		*string;
	register int		length;
{
	register char		*p;

	if (string->buffer.start == NULL) {
		/* For strings that have no memory allocated */
		string->buffer.start= string->text.p= string->text.end= Malloc(length);
		string->buffer.end= string->buffer.start + length;
		string->text.p[0]= NUL;
		string->free_after_use= true;
		return;
	};
	if (string->buffer.end - string->buffer.start >= length)
		/* If we really dont need more memory */
		return;
	/* Get more memory, copy the string and free the old buffer if it is was */
	/* malloc()ed */
	p= Malloc(length);
	(void)strcpy(p, string->buffer.start);
	string->text.p= p + (string->text.p - string->buffer.start);
	string->text.end= p + (string->text.end - string->buffer.start);
	string->buffer.end= p + length;
	if (is_true(string->free_after_use))
		Free(string->buffer.start);
	string->buffer.start= p;
	string->free_after_use= true;
}

/*
 *	Append a C string to a make string growing it if nessecary
 */
void
append_string(from, to, length)
	register char		*from;
	register String		*to;
	register int		length;
{
	register char		*cap;
	register char		*cbp;

	if (length == FIND_LENGTH) {
		length_string(from, length, cap);
	};
	if (to->buffer.start == NULL)
		expand_string(to, 32 + length);
	if (to->text.p + length >= to->buffer.end)
		expand_string(to, (to->buffer.end - to->buffer.start) * 2 + length);
	if (length > 0) {
		copy_string(to->text.p, from, length, cap, cbp);
		to->text.p += length;
	};
	*(to->text.p)= NUL;
}

/*
 *	Append one char to a make string growing it if nessecary
 */
void
append_char(from, to)
	char			from;
	register String		*to;
{
	if (to->buffer.start == NULL)
		expand_string(to, 32);
	if (to->text.p + 2 >= to->buffer.end)
		expand_string(to, to->buffer.end - to->buffer.start + 32);
	*(to->text.p)++= from;
	*(to->text.p)= NUL;
}

/*
 *	Nameblock property handling
 */
/*
 *	Create a new property and append it to the property list of a Name
 */
Property
append_prop(target, type)
	register Name		target;
	register Property_id	type;
{
	register Property	*insert= &target->prop;
	register Property	prop= *insert;
	register int		size;

	switch (type) {
	    case conditional_prop:
		size= sizeof(struct Conditional);
		break;
	    case line_prop:
		size= sizeof(struct Line);
		break;
	    case macro_prop:
		size= sizeof(struct Macro);
		break;
	    case makefile_prop:
		size= sizeof(struct Makefile);
		break;
	    case member_prop:
		size= sizeof(struct Member);
		break;
	    case recursive_prop:
		size= sizeof(struct Recursive);
		break;
	    case sccs_prop:
		size= sizeof(struct Sccs);
		break;
	    case suffix_prop:
		size= sizeof(struct Suffix);
		break;
	    case sym_link_to_prop:
		size= sizeof(struct Sym_link_to);
		break;
	    case target_prop:
		size= sizeof(struct Target);
		break;
	    case vpath_alias_prop:
		size= sizeof(struct Vpath_alias);
		break;
	    default:
		fatal("Internal error. Unknown prop type %d", type);
	};
	for (; prop != NULL; insert= &prop->next, prop= *insert);
	size += PROPERTY_HEAD_SIZE;
	*insert= prop= (struct Property *) Malloc(size);
	bzero((char *)prop, size);
	prop->type= type;
	prop->next= NULL;
	return(prop);
}

/*
 *	Scan the property list of a Name to find the next property of a given type
 */
Property
get_prop(start, type)
	register Property	start;
	register Property_id	type;
{
	for (; start != NULL; start= start->next)
		if (start->type == type)
			return(start);
	return(NULL);
}

/*
 *	Append a property to the Name if none of this type exists
 *	else return the one already there
 */
Property
maybe_append_prop(target, type)
	register Name		target;
	register Property_id	type;
{
	register Property	prop;

	if ((prop= get_prop(target->prop, type)) != NULL)
		return(prop);
	return(append_prop(target, type));
}

/*
 *	Print a message and die
 */
/*VARARGS1*/
void
fatal(message, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
	register char		*message;
{
	(void)fflush(stdout);
	(void)fprintf(stderr, "make: Fatal error: ");
	if (message)
		(void)fprintf(stderr, message,
			      a, b, c, d, e, f, g, h, i, j, k, l, m, n);
	(void)fprintf(stderr, "\n");
	if (is_true(flag.report_pwd)) {
		check_current_path();
		(void)fprintf(stderr, "Current working directory %s\n",
			current_path->string);
	};
	(void)fflush(stderr);
	exit(1);
}

/*
 *	Print a message but continue
 */
/*VARARGS1*/
void
warning(message, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
	register char		*message;
{
	(void)fflush(stdout);
	(void)fprintf(stderr, "make: Warning: ");
	if (message)
		(void)fprintf(stderr, message,
			      a, b, c, d, e, f, g, h, i, j, k, l, m, n);
	(void)fprintf(stderr, "\n");
	if (is_true(flag.report_pwd)) {
		check_current_path();
		(void)fprintf(stderr, "Current working directory %s\n",
			current_path->string);
	};
	(void)fflush(stderr);
}

/*
 *	Return the error message for a system call error
 */
char *
errmsg(errnum)
	int			errnum;
{
	extern int		sys_nerr;
	extern char 	       *sys_errlist[];
	static char		errbuf[6+1+11+1];

	if (errnum < 0 || errnum > sys_nerr) {
		(void)sprintf(errbuf, "Error %d", errnum);
		return (errbuf);
	} else
		return (sys_errlist[errnum]);
}

/*
 *	time_to_string() take a numeric time value and produces a proper string representation.
 */
char *
time_to_string(time)
	Timetype		time;
{
	char			*string;

	if (time == FILE_DOESNT_EXIST)
		return("File does not exist");
	if (time == FILE_MAX_TIME)
		return("Younger than any file");
	string= ctime(&time);
	string[strlen(string)-1]= NUL;
	return(strcpy(Malloc(strlen(string)+1), string));
}

/*
 *	Stuff pwd[] with the current path if it isnt there already
 */
void
check_current_path()
{
	char			pwd[MAXPATHLEN];

	if (current_path == NULL) {
		(void)getwd(pwd);
		if (pwd[0] == NUL) {
			pwd[0] == SLASH;
			pwd[1]= NUL;
		};
		current_path= getname(pwd, FIND_LENGTH);
	};
}

/*
 *	This is a set  of routines for dumping the internal make state
 *	Used for the -p option
 */
static void
print_value(value, daemon)
	register Name		value;
	enum daemon		daemon;
{
	Chain			cp;

	if (value == NULL)
		(void)printf(" =\n");
	else
		switch (daemon) {
		    case no_daemon:
			(void)printf("= %s\n", value->string);
			break;
		    case chain_daemon:
			for (cp= (Chain) value; cp != NULL; cp= cp->next)
				(void)printf(cp->next == NULL ? "%s" : "%s ",
					cp->name->string);
			(void)printf("\n");
			break;
		};
}

void
print_dependencies(target, line, go_recursive, print_makefiles)
	register Name		target;
	register Property	line;
	Boolean			go_recursive;
	Boolean			print_makefiles;
{
	register Dependency	dependencies;
	register Property	recursive;
	register Boolean	name_printed= false;

	if (is_true(target->dependency_printed))
		return;
	target->dependency_printed= true;
	if (is_true(target->has_recursive_dependency))
		for (recursive= get_prop(target->prop, recursive_prop);
		     recursive != NULL;
		     recursive= get_prop(recursive->next, recursive_prop)) {
			(void)printf("%s:\t%s %s %s",
				      target->string,
				      cached_names.recursive->string,
				  recursive->body.recursive.directory->string,
				    recursive->body.recursive.target->string);
			for (dependencies= recursive->body.recursive.makefiles;
			     dependencies != NULL;
			     dependencies= dependencies->next)
				(void)printf(" %s", dependencies->name->string);
			(void)printf("\n");
		};
	if (is_false(print_makefiles)) {
		(void)printf("%s:\t", target->string);
		name_printed= true;
	};
	if (print_makefiles && (line != NULL) && (line->body.line.dependencies != NULL)) {
		if (is_false(name_printed)) {
			(void)printf("%s:\t", target->string);
			name_printed= true;
		};
		for (dependencies= makefiles_used;
		     dependencies != NULL;
		     dependencies= dependencies->next)
			(void)printf("%s ", dependencies->name->string);
	};
	if (line != NULL) {
		if (is_false(name_printed) && (line->body.line.dependencies != NULL)) {
			(void)printf("%s:\t", target->string);
			name_printed= true;
		};
		for (dependencies= line->body.line.dependencies;
		     dependencies != NULL;
		     dependencies= dependencies->next)
			(void)printf("%s ", dependencies->name->string);
	};
	if (is_true(name_printed))
		(void)printf("\n");
	if (is_true(go_recursive) && (line != NULL))
		for (dependencies= line->body.line.dependencies;
		     dependencies != NULL;
		     dependencies= dependencies->next)
			print_dependencies(dependencies->name,
					   get_prop(dependencies->name->prop, line_prop),
					   go_recursive,
					   print_makefiles);
}

static void
print_rule(target)
	register Name		target;
{
	register Cmd_line	rule;
	register Property	line;

	if (((line= get_prop(target->prop, line_prop)) == NULL) ||
	    ((line->body.line.command_template == NULL) &&
	     (line->body.line.dependencies == NULL)))
		return;
	print_dependencies(target, line, false, false);
	for (rule= line->body.line.command_template; rule != NULL; rule= rule->next)
		(void)printf("\t%s\n", rule->command_line->string);
}

printdesc()
{
	register int		n;
	register Name		p;
	register Property	prop;
	register Dependency	dep;
	register Cmd_line	rule;
	Percent			percent;

	/* Default target */
	if (default_target_to_build != NULL) {
		print_rule(default_target_to_build);
		default_target_to_build->dependency_printed= true;
	};
	(void)printf("\n");

	/* .AR_REPLACE */
	if (ar_replace_rule != NULL) {
		(void)printf("%s:\n", cached_names.ar_replace->string);
		for (rule= ar_replace_rule; rule != NULL; rule= rule->next)
			(void)printf("\t%s\n", rule->command_line->string);
	};

	/* .DEFAULT */
	if (default_rule != NULL) {
		(void)printf("%s:\n", cached_names.default_rule->string);
		for (rule= default_rule; rule != NULL; rule= rule->next)
			(void)printf("\t%s\n", rule->command_line->string);
	};

	/* .IGNORE */
	if (is_true(flag.ignore_errors))
		(void)printf("%s:\n", cached_names.ignore->string);

	/* .KEEP_STATE: */
	if (is_true(flag.keep_state))
		(void)printf("%s:\n\n", cached_names.dot_keep_state->string);

	/* .PRECIOUS */
	(void)printf("%s: ", cached_names.precious->string);
	for (n= HASHSIZE - 1; n >= 0; n--)
		for (p= hashtab[n]; p != NULL; p= p->next)
			if (is_true(p->stat.is_precious))
				(void)printf("%s ", p->string);
	(void)printf("\n");

	/* .SCCS_GET */
	if (sccs_get_rule != NULL) {
		(void)printf("%s:\n", cached_names.sccs_get->string);
		for (rule= sccs_get_rule; rule != NULL; rule= rule->next)
			(void)printf("\t%s\n", rule->command_line->string);
	};

	/* .SILENT */
	if (is_true(flag.silent))
		(void)printf("%s:\n", cached_names.silent->string);

	/* .SUFFIXES: */
	(void)printf("%s: ", cached_names.suffixes->string);
	for (dep= suffixes; dep != NULL; dep= dep->next) {
		(void)printf("%s ", dep->name->string);
		build_suffix_list(dep->name);
	};
	(void)printf("\n\n");

	/* SYM_LINK_TO */
	for (n= HASHSIZE - 1; n >= 0; n--)
		for (p= hashtab[n]; p != NULL; p= p->next)
			if (is_true(p->stat.should_be_sym_link))
				(void)printf("%s:\t%s %s\n",
					     p->string,
					     cached_names.sym_link_to->string,
					     get_prop(p->prop, sym_link_to_prop)->
						body.sym_link_to.link_to->string);
	/* % rules */
	for (percent= percent_list; percent != NULL; percent= percent->next) {
		(void)printf("%s%%%s:\t%s%%%s\n",
			     percent->target_prefix->string,
			     percent->target_suffix->string,
			     percent->source_prefix->string,
			     percent->source_suffix->string);
		for (rule= percent->command_template; rule != NULL; rule= rule->next)
			(void)printf("\t%s\n", rule->command_line->string);
	};

	/* Suffix rules */
	for (n= HASHSIZE - 1; n >= 0; n--)
		for (p= hashtab[n]; p != NULL; p= p->next)
			if (is_false(p->dependency_printed) && (p->string[0] == PERIOD)) {
				print_rule(p);
				p->dependency_printed= true;
			};

	/* Macro assignments */
	for (n= HASHSIZE - 1; n >= 0; n--)
		for (p= hashtab[n]; p != NULL; p= p->next)
			if (((prop= get_prop(p->prop, macro_prop)) != NULL) &&
			    (prop->body.macro.value != NULL)) {
				(void)printf("%s", p->string);
				print_value(prop->body.macro.value,
					    prop->body.macro.daemon);
			};
	(void)printf("\n");

	/* Delays */
	for (n= HASHSIZE - 1; n >= 0; n--)
		for (p= hashtab[n]; p != NULL; p= p->next)
			for (prop= get_prop(p->prop, conditional_prop);
			     prop != NULL;
			     prop= get_prop(prop->next, conditional_prop)) {
				(void)printf("%s := %s",
					     p->string,
					     prop->body.conditional.name->string);
				print_value(prop->body.conditional.value, no_daemon);
			};
	(void)printf("\n");

	/* All other dependencies */
	for (n= HASHSIZE - 1; n >= 0; n--)
		for (p= hashtab[n]; p != NULL; p= p->next)
			if (is_false(p->dependency_printed) && (p->colons != no_colon))
				print_rule(p);
	(void)printf("\n");
	exit(0);
}
