#ifndef lint
static char sccsid[]= "@(#)read.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc	[Remotely from S5R2]
 */

#include "defs.h"
#include <ctype.h>
#include <fcntl.h>

#define goto_state(new_state) { state= (new_state); \
				goto enter_state;}
#define set_state(new_state) state= (new_state)

#define enter_name(start, end, flag, string, names) { \
	names= enter_name_fn(start, end, flag, string, names, &extra_names, &target_group_seen); \
	if (extra_names == NULL) \
		extra_names= (Name_vector *)alloca(sizeof(Name_vector));}

typedef enum {
	no_state,
	scan_name_state,
	scan_command_state,
	enter_dependencies_state,
	enter_conditional_state,
	enter_equal_state,
	illegal_eoln_state,
	poorly_formed_macro_state,
	exit_state
}                       Reader_state;
typedef enum separator  Separator;
typedef struct Name_vector {
	Name                    names[64];
	Chain                   target_group[64];
	short                   used;
	struct Name_vector     *next;
} Name_vector;

static char *		file_being_read;
static int		line_number;
static int		max_include_depth= 20;

/*VARARGS1*/
void
fatal_reader(string, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
	register char		*string;
{
	char			message[1000];

	if (temp_file_name != NULL) {
		(void)fprintf(stderr, "make: Temp-file %s not removed\n",
				temp_file_name->string);
		temp_file_name= NULL;
	};
	if (file_being_read != NULL) {
		if (line_number != 0)
			(void)sprintf(message,
			      "%s, line %d: %s",
			       file_being_read,
			       line_number,
			       string);
		else
			(void)sprintf(message,
			      "%s: %s",
			       file_being_read,
			       string);
		fatal(message, a, b, c, d, e, f, g, h, i, j, k, l, m, n);
	} else
		fatal(string, a, b, c, d, e, f, g, h, i, j, k, l, m, n);
}

/*
 *	get_next_block() will get the next block of text to read either
 *	by popping one source block of the stack of Sources
 *	or by reading some more from the makefile.
 */
Source
get_next_block_fn(source)
	register Source		source;
{
	register int		length;
	register int		to_read;

	if (source == NULL)
		return(source);
	if ((source->fd < 0) || (source->bytes_left_in_file <= 0)) {
		/* We cant read from the makefile so we pop the source block */
		if (source->fd > 2) {
			(void)close(source->fd);
			if (lock_file_name != NULL) {
				(void)unlink(lock_file_name);
				lock_file_name = NULL;
			};
		};
		if (is_true(source->string.free_after_use) &&
		    (source->string.buffer.start != NULL)) {
			Free(source->string.buffer.start);
			source->string.buffer.start= NULL;
		};
		return(source->previous);
	};
	/* Read some more from the makefile. Hopefully the kernel managed to */
	/* prefetch the stuff */
	to_read= 8 * 1024;
	if (to_read > source->bytes_left_in_file)
		to_read= source->bytes_left_in_file;
	if ((length= read(source->fd,
		       source->string.buffer.end - source->bytes_left_in_file,
			   to_read)) != to_read) {
		line_number= 0;
		if (length == 0)
			fatal("Error reading `%s': Premature EOF",
				file_being_read);
		else
			fatal("Error reading `%s': %s", file_being_read,
				errmsg(errno));
	}
	source->bytes_left_in_file -= length;
	source->string.text.end += length;
	return(source);
}

/*
 *	Take a namestring and remove redundant ../, // and ./ constructs
 */
Name
normalize_name(start, length)
	register char		*start;
	register int		length;
{
	register char		*string= alloca(length + 1);
	register char		*cdp;
	char			*current_component;
	Name			name;
	register int		count;

	/* Copy string removing ./ and // */
	/* First strip leading ./ */
	while ((length > 1) && (start[0] == PERIOD) && (start[1] == SLASH)) {
		start += 2;
		length -= 2;
		while ((length > 0) && (start[0] == SLASH)) {
			start++;
			length--;
		};
	};
	/* Then copy the rest of the string removing /./ & // */
	cdp= string;
	while (length > 0) {
		if (((length > 2) &&
		     (start[0] == SLASH) &&
		     (start[1] == PERIOD) &&
		     (start[2] == SLASH)) ||
		    ((length == 2) &&
		     (start[0] == SLASH) &&
		     (start[1] == PERIOD))) {
			start += 2;
			length -= 2;
			continue;
		};
		if ((length > 1) && (start[0] == SLASH) && (start[1] == SLASH)) {
			start++;
			length--;
			continue;
		};
		*cdp++= *start++;
		length--;
	};
	*cdp= NUL;
	/* Now scan for <name>/../ and remove such combinations iff <name> is */
	/* not a symlink or another .. */
	/* Each time something is removed the whole process is restarted */
removed_one:
	start= string;
	current_component= cdp= string= alloca((length= strlen(start)) + 1);
	while (length > 0) {
		if (((length > 3) &&
		     (start[0] == SLASH) &&
		     (start[1] == PERIOD) &&
		     (start[2] == PERIOD) &&
		     (start[3] == SLASH)) ||
		    ((length == 3) &&
		     (start[0] == SLASH) &&
		     (start[1] == PERIOD) &&
		     (start[2] == PERIOD)))
			/* Positioned on the / that starts a /.. sequence */
			if (((count = cdp - current_component) != 0) &&
			    (exists(name= getname(current_component, count)) > 0) &&
			    is_false(name->stat.is_sym_link) &&
			    (name != cached_names.dotdot)) {
				cdp= current_component;
				start += 3;
				length -= 3;
				if (length > 0) {
					start++;	/* skip slash */
					length--;
					while (length > 0) {
						*cdp++= *start++;
						length--;
					};
				};
				*cdp= NUL;
				goto removed_one;
			};
		if ((*cdp++= *start++) == SLASH)
			current_component= cdp;
		length--;
	};
	*cdp= NUL;
	if (string[0] == NUL)
		return(cached_names.dot);
	return(getname(string, FIND_LENGTH));
}

/*
 *	A string has been found to contain member names.
 *	(The "lib.a[members]" and "lib.a(members)" notation)
 *	Handle it pretty much as enter_name_fn() does for simple names.
 */
static Name_vector *
enter_member_name(lib_start, member_start, string_end, current_names, extra_names)
	register char		*lib_start;
	register char		*member_start;
	register char		*string_end;
	Name_vector		*current_names;
	Name_vector		**extra_names;
{
	register Boolean	entry= false;
	char			buffer[STRING_BUFFER_LENGTH];
	Name			lib;
	Name			member;
	Name			name;
	Property		prop;
	char			*memberp;
	register int		paren_count;
	register Boolean	has_dollar;
	register char		*cq;
	char			closing_paren= BRACERIGHT;

	if (*member_start != BRACELEFT)
		closing_paren= PARENRIGHT;
	/* Internalize the name of the library */
	lib= getname(lib_start, member_start - lib_start);
	lib->member_class= closing_paren == BRACERIGHT ? new_member:old_member;
	member_start++;
	if ((*member_start == BRACELEFT) || (*member_start == PARENLEFT)) {
		/* This is really the "lib.a[[entries]]" format */
		entry= true;
		member_start++;
	};
	/* Move the library name to the buffer where we intend to build the */
	/* "lib.a[member]" for each member */
	(void)strncpy(buffer, lib_start, member_start-lib_start);
	memberp= buffer + (member_start-lib_start);
more:
	/* Find the end of the member name. Allow nested (). Detect $ */
	for (cq= memberp, has_dollar= false, paren_count= 0;
	     (member_start < string_end) &&
		((*member_start != PARENRIGHT) || (paren_count > 0)) &&
		(*member_start != BRACERIGHT) &&
		!isspace(*member_start);
	     *cq++= *member_start++)
		switch (*member_start) {
		    case PARENLEFT:
			paren_count++;
			break;
		    case PARENRIGHT:
			paren_count--;
			break;
		    case DOLLAR:
			has_dollar= true;
		};
	/* Internalize the member name */
	member= getname(memberp, cq - memberp);
	if ((cq - memberp > AR_MEMBER_NAME_LEN) && is_false(has_dollar))
		cq= memberp + AR_MEMBER_NAME_LEN;
	*cq++= closing_paren;
	if (is_true(entry))
		*cq++= closing_paren;
	/* Internalize the "lib.a[member]" notation for this member */
	name= getname(buffer, cq - buffer);
	name->member_class= lib->member_class;
	/* And add the member prop */
	prop= append_prop(name, member_prop);
	prop->body.member.library= lib;
	if (is_true(entry)) {
		/* "lib.a[[entry]" notation */
		prop->body.member.entry= member;
		prop->body.member.member= NULL;
	} else {
		/* "lib.a[member]" Notation */
		prop->body.member.entry= NULL;
		prop->body.member.member= member;
	};
	/* Handle overflow of current_names */
	if (current_names->used ==
		(sizeof(current_names->names) / sizeof(current_names->names[0]))) {
		if (current_names->next != NULL)
			current_names= current_names->next;
		else {
			if (*extra_names == NULL)
				current_names= current_names->next=
					(Name_vector *) Malloc(sizeof(Name_vector));
			else {
				current_names= current_names->next= *extra_names;
				*extra_names= NULL;
			};
			current_names->used= 0;
			current_names->next= NULL;
		};
	};
	current_names->target_group[current_names->used]= NULL;
	current_names->names[current_names->used++]= name;
	while (isspace(*member_start))
		member_start++;
	/* Check if there are more members */
	if ((*member_start == PARENRIGHT) || (*member_start == BRACERIGHT) || (member_start >= string_end))
		return(current_names);
	goto more;
}

/*
 *	Take one string and enter it as a name. The string is passed in twp parts.
 *	A make string and possibly a C string to append to it.
 *	The result is stuffed in the vector current_names.
 *	extra_names points to a vector that is used if current_names overflows.
 *	This is allocad in the calling routine.
 *	Here we handle the "lib.a[members]" notation.
 */
static Name_vector *
enter_name_fn(string_start, string_end, tail_present, string,
	      current_names, extra_names, target_group_seen)
	register char		*string_start;
	register char		*string_end;
	Boolean			tail_present;
	String			*string;
	Boolean			*target_group_seen;
	Name_vector		*current_names;
	Name_vector		**extra_names;
{
	Name			name;
	register char		*cp;
	char			ch;

	/* If we were passed a separate tail of the name we append it to the */
	/* make string with the rest of it */
	if (is_true(tail_present)) {
		append_string(string_start, string, string_end - string_start);
		string_start= string->buffer.start;
		string_end= string->text.p;
	};
	/* Strip "./" from the head of the name */
	if ((string_start[0] == PERIOD) && (string_start[1] == SLASH))
		string_start += 2;
	ch= *string_end;
	*string_end= NUL;
	/* Check if there are any parens that are not prefixed with "$" */
	/* or any "[" */
	/* If there are we have to deal with the "lib.a[members]" format */
	for (cp= index(string_start, PARENLEFT); cp != NULL; cp= index(cp + 1, PARENLEFT))
		if (*(cp - 1) != DOLLAR) {
			*string_end= ch;
			return(enter_member_name(string_start, cp, string_end, current_names, extra_names));
		};
	if ((cp= index(string_start, BRACELEFT)) != NULL) {
		*string_end= ch;
		return(enter_member_name(string_start, cp, string_end, current_names, extra_names));
	};
	*string_end= ch;
	/* If the current_names vector is full we patch in the one from */
	/* extra_names */
	if (current_names->used ==
	     (sizeof(current_names->names) / sizeof(current_names->names[0]))) {
		if (current_names->next != NULL)
			current_names= current_names->next;
		else {
			current_names->next= *extra_names;
			*extra_names= NULL;
			current_names= current_names->next;
			current_names->used= 0;
			current_names->next= NULL;
		};
	};
	current_names->target_group[current_names->used]= NULL;
	/* Remove extra ../ constructs if we are reading from a report file */
	if (makefile_type == reading_cpp_file)
		name= normalize_name(string_start, string_end - string_start);
	else
		name= getname(string_start, string_end - string_start);
	/* Internalize the name. Detect the name "+" (target group here) */
	if ((current_names->names[current_names->used++]= name) == cached_names.plus)
		*target_group_seen= true;
	if (is_true(tail_present) && is_true(string->free_after_use))
		Free(string->buffer.start);
	return(current_names);
}

/*
 *	If a "+" was seen when the target list was scanned we need to extract the groups
 *	Each target in the name vector that is a member of a group gets a pointer
 *	to a chain of all the members stuffed in its target_group vector slot
 */
static void
find_target_groups(target)
	register Name_vector	*target;
{
	register Chain		target_group= NULL;
	register Chain		tail_target_group= NULL;
	register Name		*next;
	register Boolean	clear_target_group= false;
	register int		i;

	/* Scan the list of targets */
	for (; target != NULL; target= target->next)
		for (i= 0; i < target->used; i++)
			if (target->names[i] != NULL) {
				/* If the previous target terminated a group we */
				/* flush the pointer to that member chain */
				if (is_true(clear_target_group)) {
					clear_target_group= false;
					target_group= NULL;
				};
				/* Pick up a pointer to the cell with the next */
				/* target */
				if (i + 1 != target->used)
					next= &target->names[i + 1];
				else
					next= (target->next != NULL) ?
						&target->next->names[0] : NULL;
/* We have four states here */
/*	0:	No target group started and next element is not a "+" */
/*		This is not interesting */
/*	1:	A target group is being built and the next element is not a "+" */
/*		This terminates the group */
/*	2:	No target group started and the next member is "+" */
/*		This is the first target in a group */
/*	3:	A target group started and the next member is a "+" */
/*		The group continues */
				switch ((target_group ? 1 : 0) +
					(next && (*next == cached_names.plus) ? 2 : 0)) {
				    case 0:	/* Not target_group */
					break;
				    case 1:	/* Last group member */
					/* We need to keep this pointer so we */
					/* can stuff it for the last member */
					clear_target_group= true;
					/* fall into */
				    case 3:	/* Middle group member */
					/* Add this target to the current chain */
					tail_target_group->next= alloc(Chain);
					tail_target_group= tail_target_group->next;
					tail_target_group->next= NULL;
					tail_target_group->name= target->names[i];
					break;
				    case 2:	/* First group member */
					/* Start a new chain */
					target_group= tail_target_group= alloc(Chain);
					target_group->next= NULL;
					target_group->name= target->names[i];
					break;
				};
				/* Stuff the current chain, if any, in the */
				/* targets group slot */
				target->target_group[i]= target_group;
				if ((next != NULL) && (*next == cached_names.plus))
					*next= NULL;
			};
}

/*
 *	Used when tracing the reading of rules
 */
static void
print_rule(command)
	register Cmd_line	command;
{
	for (; command != NULL; command= command->next)
		(void)printf("\t%s\n", command->command_line->string);
}

/*
 *	Read the pseudo target make knows about
 */
static void
special_reader(target, depes, command)
	Name			target;
	register Name_vector	*depes;
	Cmd_line		command;
{
	register Dependency	dp;
	register Dependency	*insert_dep;
	register Name		np;
	Name			np2;
	register int		n;
	register Boolean	first= true;

	switch (target->special_reader) {
	    case ar_replace_special:
		if (depes->used != 0)
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		ar_replace_rule= command;
		if (is_true(flag.trace_reader)) {
			(void)printf("%s:\n", cached_names.ar_replace->string);
			print_rule(command);
		};
		break;
	    case built_last_make_run_special:
		built_last_make_run_seen= true;
		break;
	    case default_special:
		if (depes->used != 0)
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		default_rule= command;
		if (is_true(flag.trace_reader)) {
			(void)printf("%s:\n", cached_names.default_rule->string);
			print_rule(command);
		};
		break;
	    case ignore_special:
		if (depes->used != 0)
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		flag.ignore_errors= true;
		if (is_true(flag.trace_reader))
			(void)printf("%s:\n", cached_names.ignore->string);
		break;
	    case keep_state_special:
		if (depes->used != 0)
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		flag.keep_state= true;
		if (is_true(flag.trace_reader))
			(void)printf("%s:\n", cached_names.dot_keep_state->string);
		break;
	    case make_version_special:
		if (depes->used != 1)
			fatal_reader("Illegal dependency list for target `%s'",
				     target->string);
		if (depes->names[0] != cached_names.current_make_version)
			warning("Version mismatch between current version `%s' and `%s'",
				      cached_names.current_make_version->string,
				      depes->names[0]->string);
		break;
	    case precious_special:
		/* Set the precious bit for all the targets on the dependency list */
		for (; depes != NULL; depes= depes->next)
			for (n= 0; n < depes->used; n++) {
				if (is_true(flag.trace_reader))
					(void)printf("%s:\t%s\n",
						     cached_names.precious->string,
						     depes->names[n]->string);
				depes->names[n]->stat.is_precious= true;
			};
		break;
	    case sccs_get_special:
		if (depes->used != 0)
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		sccs_get_rule= command;
		if (is_true(flag.trace_reader)) {
			(void)printf("%s:\n", cached_names.sccs_get->string);
			print_rule(command);
		};
		break;
	    case silent_special:
		if (depes->used != 0)
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		flag.silent= true;
		if (is_true(flag.trace_reader))
			(void)printf("%s:\n", cached_names.silent->string);
		break;
	    case suffixes_special:
		if (depes->used == 0) {
			/* .SUFFIXES with no dependency list clears the suffixes list */
			for (n= HASHSIZE - 1; n >= 0; n--)
				for (np= hashtab[n]; np != NULL; np= np->next)
					np->with_squiggle= np->without_squiggle= false;
			suffixes= NULL;
			if (is_true(flag.trace_reader))
				(void)printf("%s:\n", cached_names.suffixes->string);
			return;
		};
		/* Otherwise we append to the list */
		for (; depes != NULL; depes= depes->next)
		    for (n= 0; n < depes->used; n++) {
			np= depes->names[n];
			/* Find the end of the list and check if the */
			/* suffix already has been entered */
			for (insert_dep= &suffixes, dp= *insert_dep;
			     dp != NULL;
			     insert_dep= &dp->next, dp= *insert_dep)
				if (dp->name == np)
					goto duplicate_suffix;
			if (is_true(flag.trace_reader)) {
				if (is_true(first)) {
					(void)printf("%s:\t",
						     cached_names.suffixes->string);
					first= false;
				};
				(void)printf("%s ", depes->names[n]->string);
			};
			/* If the suffix is suffixed with "~" we strip */
			/* that and mark the suffix nameblock */
			if (np->string[np->hash.length - 1] == TILDE) {
				np2= getname(np->string, (int)(np->hash.length - 1));
				np2->with_squiggle= true;
				if (is_true(np2->without_squiggle))
					continue;
				np= np2;
			};
			np->without_squiggle= true;
			/* Add the suffix to the list */
			dp= *insert_dep= alloc(Dependency);
			insert_dep= &dp->next;
			dp->next= NULL;
			dp->name= np;
	    duplicate_suffix:;
		    };
		if (is_true(flag.trace_reader))
			(void)printf("\n");
		break;
	    case sym_link_to_special:
		if (depes->used != 0)
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		sym_link_to_rule= command;
		if (is_true(flag.trace_reader)) {
			(void)printf("%s:\n", cached_names.sym_link_to->string);
			print_rule(command);
		};
		break;
	    default:
		fatal_reader("Internal error: Unknown special reader");
	};
}

/*
 *	Enter "x%y : a%b" type lines
 *	% patterns are stored in four parts head and tail for target and source
 */
static void
enter_percent(target, source, command)
	register Name		target;
	register Name		source;
	Cmd_line		command;
{
	register Percent	result= alloc(Percent);
	Percent			p;
	Percent			*insert;
	register char		*cp;

	result->next= NULL;
	result->command_template= command;

	/* Find the % in the target string and split it */
	if ((cp= index(target->string, PERCENT)) != NULL) {
		result->target_prefix= getname(target->string, cp - target->string);
		result->target_suffix=
		    getname(cp + 1, (int)(target->hash.length - 1 - (cp - target->string)));
	} else
		fatal_reader("Illegal pattern with %% in source but not in target");

	/* Same thing for the source pattern */
	if ((cp= index(source->string, PERCENT)) != NULL) {
		result->source_prefix= getname(source->string, cp - source->string);
		result->source_suffix=
		    getname(cp + 1, (int)(source->hash.length - 1 - (cp - source->string)));
		result->source_percent= true;
	} else {
		result->source_prefix= source;
		result->source_suffix= cached_names.empty_suffix_name;
		result->source_percent= false;
	};
	if ((cached_names.empty_suffix_name == result->target_prefix) &&
	    (cached_names.empty_suffix_name == result->target_suffix) &&
	    (cached_names.empty_suffix_name == result->source_prefix) &&
	    (cached_names.empty_suffix_name == result->source_suffix)) {
		fatal_reader("Illegal pattern: `%%:%%'");
	};
	/* Find the end of the percent list and append the new pattern */
	for (insert= &percent_list, p= *insert;
	     p != NULL;
	     insert= &p->next, p= *insert);
	*insert= result;

	if (is_true(flag.trace_reader)) {
		(void)printf("%s%%%s:\t%s%s%s\n",
			     result->target_prefix->string,
			     result->target_suffix->string,
			     result->source_prefix->string,
			     is_true(result->source_percent) ? "%":"",
			     result->source_suffix->string);
		print_rule(command);
	};
}

/*
 *	Enter one dependency. Do not enter duplicates.
 */
void
enter_dependency(line, depe, automatic)
	Property		line;
	register Name		depe;
	Boolean			automatic;
{
	register Dependency	dp;
	register Dependency	*insert;

	if (is_true(flag.trace_reader))
		(void)printf("%s ", depe->string);
	/* Find the end of the list and check for duplicates */
	for (insert= &line->body.line.dependencies, dp= *insert;
	     dp != NULL;
	     insert= &dp->next, dp= *insert)
		if (dp->name == depe) {
			if (is_true(dp->automatic))
				dp->automatic= automatic;
			dp->stale= false;
			return;
		};
	/* Insert the new dependency since we couldnt find it */
	dp= *insert= alloc(Dependency);
	dp->name= depe;
	dp->next= NULL;
	dp->automatic= automatic;
	dp->stale= false;
	depe->stat.is_file= true;
}

/*
 * Given a directory and another file name, both fully-qualified,
 * compose a relative path name from the first to the second.
 */
static void
make_relative(from, to, result)
	char			*from;
	char			*to;
	char			*result;
{
	int			ncomps;
	int			i;
	int			len;
	char			*tocomp;
	char			*cp;

	/* Check if the path already is relative */
	if (to[0] != SLASH) {
		(void)strcpy(result, to);
		return;
	};

	/* Find the number of components in the from name. ncomp= number of */
	/* slashes + 1. */
	ncomps= 1;
	for (cp= from; *cp != NUL; cp++) {
		if (*cp == SLASH) {
			ncomps++;
		}
	}

	/* See how many components match to determine how many, if any, ".."s */
	/* will be needed. */
	result[0]= NUL;
	tocomp= to;
	while (*from != NUL && *from == *to) {
		if (*from == SLASH) {
			ncomps--;
			tocomp= &to[1];
		}
		from++;
		to++;
	}

	/* Now for some special cases. Check for exact matches and for either */
	/* name terminating exactly. */
	if (*from == NUL) {
		if (*to == NUL) {
			(void)strcpy(result, ".");
			return;
		}
		if (*to == SLASH) {
			ncomps--;
			tocomp= &to[1];
		}
	} else if (*from == SLASH && *to == NUL) {
		ncomps--;
		tocomp= to;
	}
	/* Add on the ".."s. */
	for (i= 0; i < ncomps; i++) {
		(void)strcat(result, "../");
	}

	/* Add on the remainder, if any, of the to name. */
	if (*tocomp == NUL) {
		len= strlen(result);
		result[len - 1]= NUL;
	} else {
		(void)strcat(result, tocomp);
	}
	return;
}

/*
 *	Take one target and a list of dependencies and process the whole thing.
 *	The target might be special in some sense in which case that is handled.
 */
static void
enter_dependencies(target, target_group, depes, command, separator)
	register Name		target;
	Chain			target_group;
	register Name_vector	*depes;
	register Cmd_line	command;
	register enum Separator	separator;
{
	register int		i;
	register Property	line;
	Name			name;
	Name			directory;
	char			*namep;
	Dependency		dp;
	Dependency		*dpp;
	Property		line2;
	char			relative[MAXPATHLEN];
	register int		recursive_state;

	/* If we saw it in the makefile it must be a file */
	target->stat.is_file= true;
	/* If the target is special we delegate the processing */
	if (target->special_reader != no_special) {
		special_reader(target, depes, command);
		return;
	};
	/* Check if this is a "a%b : x%y" type rule */
	if ((depes->used == 1) &&
	    (is_true(target->percent) || is_true(depes->names[0]->percent))) {
		enter_percent(target, depes->names[0], command);
		return;
	};
	/* Check if this is a .RECURSIVE line */
	if ((depes->used >= 3) && (depes->names[0] == cached_names.recursive)) {
		target->has_recursive_dependency= true;
		depes->names[0]= NULL;
		check_current_path();
		recursive_state= 0;
		dp= NULL;
		dpp= &dp;
		/* Read the dependencies. They are "<directory> <target-made> */
		/* <makefile>*" */
		for (; depes != NULL; depes= depes->next)
		    for (i= 0; i < depes->used; i++)
			if (depes->names[i] != NULL)
			    switch (recursive_state++) {
			      case 0:	/* Directory */
				make_relative(current_path->string,
					      depes->names[i]->string, relative);
				directory= getname(relative, FIND_LENGTH);
				break;
			      case 1:	/* Target */
				name= depes->names[i];
				break;
			      default:	/* Makefiles */
				*dpp= alloc(Dependency);
				(*dpp)->next= NULL;
				(*dpp)->name= depes->names[i];
				(*dpp)->automatic= false;
				(*dpp)->stale= false;
				dpp= &((*dpp)->next);
				break;
			    };
		/* Check if this recursion already has been reported else enter */
		/* the recursive prop for the target */
		for (line= get_prop(target->prop, recursive_prop);
		     line != NULL;
		     line= get_prop(line->next, recursive_prop))
			if ((line->body.recursive.directory == directory) &&
			    (line->body.recursive.target == name)) {
				line->body.recursive.makefiles= dp;
				return;
			};
		line2= append_prop(target, recursive_prop);
		line2->body.recursive.directory= directory;
		line2->body.recursive.target= name;
		line2->body.recursive.makefiles= dp;
		return;
	};
	/* If this is the first target that doesnt start with a "." in the */
	/* makefile we remember that */
	if ((makefile_type == reading_makefile) && (default_target_to_build == NULL) &&
	    ((target->string[0] != PERIOD) || index(target->string, SLASH)))
		default_target_to_build= target;
	/* Handle .SYM_LINK_TO lines */
	if (depes->names[0] == cached_names.sym_link_to) {
		if (depes->used > 2)
			warning("Extra dependencies for `%s: %s' line ignored",
				      target->string, cached_names.sym_link_to->string);
		if (command != NULL)
			warning("Rule for `%s: %s' line ignored",
				      target->string, cached_names.sym_link_to->string);
		/* Enter it as a regular dependency */
		enter_dependency(maybe_append_prop(target, line_prop),
				 depes->names[1],
				 false);
		/* Also enter a special prop that forces doname() to create the */
		/* link before chasing the dependency */
		line= maybe_append_prop(target, sym_link_to_prop);
		line->body.sym_link_to.link_to= depes->names[1];
		target->stat.should_be_sym_link= true;
		if (is_true(flag.trace_reader))
			(void)printf("%s:\t%s %s\n",
				     target->string,
				     cached_names.sym_link_to->string,
				     depes->names[1]->string);
		return;
	};
	/* Check if the line is ":" or "::" */
	if (makefile_type == reading_makefile) {
	    if (target->colons == no_colon)
		target->colons= separator;
	    else
		if (target->colons != separator)
			fatal_reader(":/:: conflict for target `%s'",
				     target->string);
	    if (target->colons == two_colon) {
		if (depes->used == 0) {
			/* If this is a "::" type line with no dependencies we */
			/* add one "FRC" type dependency for free */
			depes->used= 1; /* Force :: targets with no depes to
					  * always run */
			depes->names[0]= cached_names.force;
		};
		/* Do not delete "::" type targets when interrupted */
		target->stat.is_precious= true;
		/* Build a synthetic target "<number>%target" for "target" */
		namep= Malloc((int)(target->hash.length + 10));
		(void)sprintf(namep, "%d@%s", target->colon_splits++, target->string);
		name= getname(namep, FIND_LENGTH);
		if (is_true(flag.trace_reader))
			(void)printf("%s:\t", target->string);
		/* Make "target" depend on "<number>%target */
		enter_dependency(line2= maybe_append_prop(target, line_prop), name, true);
		line2->body.line.target= target;
		/* Put a prop on "<number>%target that makes appear as "target" */
		/* when it is processed */
		maybe_append_prop(name, target_prop)->body.target.target= target;
		name->has_target_prop= true;
		if (is_true(flag.trace_reader))
			(void)printf("\n");
		(target= name)->stat.is_file= true;
	    };
	};
	/* This really is a regular dependency line. Just enter it */
	line= maybe_append_prop(target, line_prop);
	line->body.line.target= target;
	/* Depending on what kind of makefile we are reading we have to treat */
	/* things differently */
	switch (makefile_type) {
	    case reading_makefile:
		/* Reading regular makefile. Just notice whether this redefines */
		/* the rule for the  target */
		if (command != NULL) {
			if (line->body.line.command_template != NULL) {
				line->body.line.command_template_redefined= true;
				if (target->string[0] == PERIOD)
					line->body.line.command_template= command;
			} else
				line->body.line.command_template= command;
		};
		break;
	    case rereading_statefile:
		/* Rereading the statefile. We only enter thing that changed */
		/* since the previous time we read it */
		if (is_false(built_last_make_run_seen))
			return;
		built_last_make_run_seen= false;
		command_changed= true;
		target->has_built= true;
	    case reading_statefile:
		/* Reading the statefile for the first time. Enter the rules as */
		/* "Commands used" not "templates to use" */
		if (command != NULL)
			line->body.line.command_used= command;
		break;
	    case reading_cpp_file:
		/* Reading report file from programs that reports dependencies */
		/* If this is the first time the target is read from this */
		/* reportfile we clear all old automatic depes */
		if (target->temp_file_number == temp_file_number)
			break;
		target->temp_file_number= temp_file_number;
		command_changed= true;
		/* We also clear all old recursive reports */
		for (line2= get_prop(target->prop, recursive_prop);
		     line2 != NULL;
		     line2= get_prop(line2->next, recursive_prop))
			line2->type= no_prop;
		if (line != NULL)
			for (dpp= &line->body.line.dependencies, dp= *dpp;
			     dp != NULL;
			     dp= dp->next)
				if (is_true(dp->automatic))
					*dpp= dp->next;
				else
					dpp= &dp->next;
		break;
	    default:
		fatal_reader("Internal error. Unknown makefile type %d", makefile_type);
	};
	/* A target may only be involved in one target group */
	if (line->body.line.target_group != NULL) {
		if (target_group != NULL)
			fatal_reader("Too many target groups for target `%s'",
				     target->string);
	} else
		line->body.line.target_group= target_group;

	if (is_true(flag.trace_reader))
		(void)printf("%s:\t", target->string);
	/* Enter the dependencies */
	for (; depes != NULL; depes= depes->next)
		for (i= 0; i < depes->used; i++)
			enter_dependency(line,
					 depes->names[i],
					 boolean(makefile_type != reading_makefile));
	if (is_true(flag.trace_reader)) {
		(void)printf("\n");
		print_rule(command);
	};
}

/*
 *	Enter "target := MACRO= value" constructs
 */
static void
enter_conditional(target, name, value, append)
	register Name		target;
	register Name		name;
	register Name		value;
	register Boolean	append;
{
	register Property	conditional;

	/* Count how many conditionals we must activate before building the */
	/* target */
	target->conditional_cnt++;
	maybe_append_prop(name, macro_prop)->body.macro.is_conditional= true;
	/* Add the property for the target */
	conditional= append_prop(target, conditional_prop);
	conditional->body.conditional.name= name;
	conditional->body.conditional.value= value;
	conditional->body.conditional.append= append;
	if (is_true(flag.trace_reader))
	    if (value == NULL)
		(void)printf("%s := %s %c=\n", target->string, name->string,
			is_true(append) ? PLUS : SPACE);
	    else
		(void)printf("%s := %s %c= %s\n", target->string, name->string,
			      is_true(append) ? PLUS : SPACE,
			      value->string);
}

/*
 *	Enter "MACRO= value" constructs
 */
void
enter_equal(name, value, append)
	register Name		name;
	register Name		value;
	register Boolean	append;
{
	(void)setvar(name, value, append);
	if (is_true(flag.trace_reader))
	    if (value == NULL)
		(void)printf("%s %c=\n", name->string, is_true(append) ? PLUS : SPACE);
	    else
		(void)printf("%s %c= %s\n", name->string, is_true(append) ? PLUS : SPACE,
				value->string);
}

/*
 *	Macro and function that evaluates one macro
 *	and makes the reader read from the value of it
 */
#define push_macro_value(offs) \
		source_p++; \
		uncache_source(); \
		{ Source t= (Source)alloca(sizeof(struct Source)); \
			source= push_macro_value_fn(t, buffer, sizeof(buffer), source); };\
		cache_source(offs)

static Source
push_macro_value_fn(bp, buffer, size, source)
	register Source         bp;
	register char          *buffer;
	int                     size;
	register Source         source;
{
	bp->string.buffer.start= bp->string.text.p= buffer;
	bp->string.text.end= NULL;
	bp->string.buffer.end= buffer + size;
	bp->string.free_after_use= false;
	expand_macro(source, &bp->string);
	bp->string.text.p= bp->string.buffer.start;
	bp->fd= -1;
	bp->already_expanded= true;
	bp->previous= source;
	return(bp);
}

/*
 *	The main reader routine for makefiles
 */
/*
 *	Strings are read from Sources.
 *	When macros are found their value is represented by a Source that is pushed on a stack.
 *	At end of string (that is returned from get_char() as 0) the block is popped.
 */
static void
parse_makefile(source)
	register Source         source;
{
	register char          *source_p;
	register char          *source_end;
	register char          *string_start;
	char                   *string_end;
	register Boolean        macro_seen_in_string;
	Boolean                 append;
	String                  name_string;
	char                    name_buffer[STRING_BUFFER_LENGTH];
	register int            distance;
	register int            paren_count;
	int			brace_count;
	Cmd_line                command;
	Cmd_line                command_tail;
	Name                    macro_value;

	Name_vector             target;
	Name_vector             depes;
	Name_vector             extra_name_vector;
	Name_vector            *current_names;
	Name_vector            *extra_names= &extra_name_vector;
	Name_vector            *nvp;
	Boolean                 target_group_seen;
	int                     i;

	register Reader_state   state;
	register Reader_state   on_eoln_state;
	register enum Separator separator;

	char                    buffer[4 * STRING_BUFFER_LENGTH];
	struct Source          *extrap;

	Boolean                 do_not_exec_rule= flag.do_not_exec_rule;
	Name                    makefile_name;

	target.next= depes.next= NULL;
	/* Move some values from their struct to register declared locals */
	cache_source(0);

start_new_line:
	/* Read whitespace on old line. Leave pointer on first char on next */
	/* line. */
	on_eoln_state= exit_state;
	for (; 1; source_p++)
	    switch (get_char()) {
	      case NUL:
		/* End of this string. Pop it and return to the previous one */
		get_next_block(source);
		source_p--;
		if (source == NULL)
			goto_state(on_eoln_state);
		break;
	      case NEWLINE:
	end_of_line:
		source_p++;
		if (source->fd >= 0)
			line_number++;
		switch (get_char()) {
		    case NUL:
			get_next_block(source);
			source_p--;
			if (source == NULL)
				goto_state(on_eoln_state);
			/* Go back to the top of this loop */
			goto start_new_line;
		    case NEWLINE:
		    case NUMBERSIGN:
		    case DOLLAR:
		    case SPACE:
		    case TAB:
			/* Go back to the top of this loop since the */
			/* new line does not start with a regular char */
			goto start_new_line;
		    default:
			/* We found the first proper char on the new line */
			goto start_new_line_no_skip;
		};
	      case TAB:
	      case SPACE:
		/* Whitespace. Just keep going in this loop */
		break;
	      case NUMBERSIGN:
		/* Comment. Skip over it */
		for (; 1; source_p++)
			switch (get_char()) {
			    case NUL:
				get_next_block(source);
				source_p--;
				if (source == NULL)
					goto_state(on_eoln_state);
				break;
			    case BACKSLASH:
				/* Comments can be continued */
				if (*++source_p == NUL) {
					get_next_block(source);
					if (source == NULL)
						goto_state(on_eoln_state);
				};
				break;
			    case NEWLINE:
				/* After we skip the comment we go to */
				/* the end of line handler since end of */
				/* line terminates comments */
				goto end_of_line;
			};
	      case DOLLAR:
		/* Macro reference */
		if (is_true(source->already_expanded))
			/* If we are reading from the expansion of a */
			/* macro we already expanded everything enough */
			goto start_new_line_no_skip;
		/* Expand the value and push the Source on the stack of */
		/* things being read */
		push_macro_value(1);
		break;
	      default:
		/* We found the first proper char on the new line */
		goto start_new_line_no_skip;
	    };

	/* We found the first normal char (one that starts an identifier) on the newline */
start_new_line_no_skip:
	/* Inspect that first char to see if it maybe is special anyway */
	switch (get_char()) {
	    case NUL:
		get_next_block(source);
		source_p--;
		if (source == NULL)
			goto_state(on_eoln_state);
		goto start_new_line_no_skip;
	    case NEWLINE:
		/* Just in case */
		goto start_new_line;
	    case EXCLAM:
		/* Evaluate the line before it is read */
		string_start= source_p + 1;
		macro_seen_in_string= false;
		/* Stuff the line in a string so we can eval it */
		for (; 1; source_p++)
		    switch (get_char()) {
		      case NEWLINE:
				goto eoln_1;
		      case NUL:
			if (source->fd > 0) {
				if (is_false(macro_seen_in_string)) {
					macro_seen_in_string= true;
					init_string_from_stack(name_string, name_buffer);
				};
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				get_next_block(source);
				string_start= source_p;
				source_p--;
				break;
			};
	eoln_1:
			if (is_false(macro_seen_in_string))
				init_string_from_stack(name_string, name_buffer);
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			extrap= (Source) alloca(sizeof(struct Source));
			extrap->string.buffer.start= NULL;
			if (*source_p == NUL)
				source_p++;
			/* Eval the macro */
			expand_value(getname(name_string.buffer.start, FIND_LENGTH),
				     &extrap->string);
			if (is_true(name_string.free_after_use))
				Free(name_string.buffer.start);
			uncache_source();
			extrap->string.text.p= extrap->string.buffer.start;
			extrap->fd= -1;
			/* And push the value */
			extrap->previous= source;
			source= extrap;
			cache_source(0);
			goto line_evald;
		    };
	    default:
		goto line_evald;
	};

	/* We now have a line we can start reading */
line_evald:
	if (source == NULL)
		goto_state(exit_state);
	/* Check if this an include command */
	if ((makefile_type == reading_makefile) &&
	    is_false(source->already_expanded) &&
	    (is_equaln(source_p, "include", 7))) {
		source_p += 7;
		if (isspace(*source_p)) {
			/* Yes this is an include. Skip spaces to get to the */
			/* filename */
			while ((get_char() != NUL) && isspace(get_char()))
				source_p++;
			string_start= source_p;
			/* Find the end of the filename */
			while ((get_char() != NUL) && (!isspace(get_char())))
				source_p++;
			source->string.text.p= source_p;
			/* Even when we run -n we want to create makefiles */
			flag.do_not_exec_rule= false;
			recursion_level= 0;
			makefile_name= getname(string_start, source_p - string_start);
			if (makefile_name->dollar) {
				String			destination;
				char			buffer[STRING_BUFFER_LENGTH];
				char			*p;
				char			*q;

				init_string_from_stack(destination, buffer);
				expand_value(makefile_name, &destination);
				for (p= destination.buffer.start; (*p != NUL) && isspace(*p); p++);
				for (q= p; (*q != NUL) && !isspace(*q); q++);
				makefile_name= getname(p, q-p);
				if (is_true(destination.free_after_use))
					Free(destination.buffer.start);
			};
			source_p++;
			uncache_source();
			if (read_trace_level > 0)
				(void)printf("Reading included makefile %s\n",
					     makefile_name->string);
			/* Read the file */
			if (read_simple_file(makefile_name, true, true, true, false, true, false) == failed)
				fatal_reader("Read of include file `%s' failed",
					     makefile_name->string);
			if (read_trace_level > 0)
				(void)printf("End of included makefile %s\n",
					     makefile_name->string);
			flag.do_not_exec_rule= do_not_exec_rule;
			cache_source(0);
			goto start_new_line;
		}
		else
			source_p -= 7;
	};

	/* Reset the status in preparation for the new line */
	for (nvp= &target; nvp != NULL; nvp= nvp->next)
		nvp->used= 0;
	for (nvp= &depes; nvp != NULL; nvp= nvp->next)
		nvp->used= 0;
	target_group_seen= false;
	command= command_tail= NULL;
	macro_value= NULL;
	append= false;
	current_names= &target;
	set_state(scan_name_state);
	on_eoln_state= illegal_eoln_state;
	separator= none_seen;

	/* The state machine starts here */
enter_state:
	while (1) switch (state) {

/****************************************************************
 *	Scan name state
  */
case scan_name_state:
	/* Scan an identifier. We skip over chars until we find a break char */

	/* First skip white space. */
	for (; 1; source_p++)
	    switch (get_char()) {
	      case NUL:
		get_next_block(source);
		source_p--;
		if (source == NULL)
			goto_state(on_eoln_state);
		break;
	      case NEWLINE:
		/* We found the end of the line. Do postprocessing or return error */
		source_p++;
		if (source->fd >= 0)
			line_number++;
		goto_state(on_eoln_state);
	      case BACKSLASH:
		/* Continuation */
		if (*++source_p == NUL) {
			get_next_block(source);
			if (source == NULL)
				goto_state(on_eoln_state);
		};
		/* Skip over any number of newlines */
		while (*source_p == NEWLINE) {
			if (source->fd >= 0)
				line_number++;
			if (*++source_p == NUL) {
				get_next_block(source);
				if (source == NULL)
					goto_state(on_eoln_state);
			};
		};
		source_p--;
		break;
	      case TAB:
	      case SPACE:
		/* Whitespace is skipped */
		break;
	      case NUMBERSIGN:
		/* Comment. Skip over it */
		for (; 1; source_p++)
			switch (get_char()) {
			    case NUL:
				get_next_block(source);
				source_p--;
				if (source == NULL)
					goto_state(on_eoln_state);
				break;
			    case BACKSLASH:
				if (*++source_p == NUL) {
					get_next_block(source);
					if (source == NULL)
						goto_state(on_eoln_state);
				};
				break;
			    case NEWLINE:
				source_p++;
				if (source->fd >= 0)
					line_number++;
				goto_state(on_eoln_state);
			};
	      case DOLLAR:
		/* Macro reference. Expand and push */
		/* value */
		if (is_true(source->already_expanded))
			goto scan_name;
		push_macro_value(1);
		break;
	      default:
		/* End of white space */
		goto scan_name;
	    };

	/* First proper identifier character */
	scan_name:
	string_start= source_p;
	paren_count= brace_count= 0;
	macro_seen_in_string= false;
	resume_name_scan:
	for (; 1; source_p++)
	   switch (get_char()) {
	      case NUL:
		/* Save what we have seen so far of the identifier */
		if (is_false(macro_seen_in_string))
			init_string_from_stack(name_string, name_buffer);
		append_string(string_start, &name_string, source_p - string_start);
		macro_seen_in_string= true;
		/* Get more text to read */
		get_next_block(source);
		string_start= source_p;
		source_p--;
		if (source == NULL)
			goto_state(on_eoln_state);
		break;
	      case NEWLINE:
		if (paren_count > 0)
			fatal_reader("Unmatched `(' on line");
		if (brace_count > 0)
			fatal_reader("Unmatched `{' on line");
		source_p++;
		/* Enter name */
		enter_name(string_start,
			   source_p - 1,
			   macro_seen_in_string,
			   &name_string,
			   current_names);
		/* Do postprocessing or return error */
		if (source->fd >= 0)
			line_number++;
		goto_state(on_eoln_state);
	      case BACKSLASH:
		if (paren_count+brace_count > 0)
			break;
		/* Enter name, skip any number of newlines and continue reading names */
		enter_name(string_start,
			   source_p,
			   macro_seen_in_string,
			   &name_string,
			   current_names);
		if (*++source_p == NUL) {
			get_next_block(source);
			if (source == NULL)
				goto_state(on_eoln_state);
		};
		while (*source_p == NEWLINE) {
			if (source->fd >= 0)
				line_number++;
			if (*++source_p == NUL) {
				get_next_block(source);
				if (source == NULL)
					goto_state(on_eoln_state);
			};
		};
		goto enter_state;
	      case NUMBERSIGN:
		if (paren_count+brace_count > 0)
			break;
		fatal_reader("Unexpected comment seen");
	      case DOLLAR:
		if (is_true(source->already_expanded))
			break;
		/* Save the identifier so far */
		if (is_false(macro_seen_in_string))
			init_string_from_stack(name_string, name_buffer);
		append_string(string_start, &name_string, source_p - string_start);
		macro_seen_in_string= true;
		/* Eval and push the macro */
		push_macro_value(1);
		string_start= source_p + 1;
		break;
	      case PARENLEFT:
		paren_count++;
		break;
	      case PARENRIGHT:
		if (--paren_count < 0)
			fatal_reader("Unmatched `)' on line");
		break;
	      case BRACELEFT:
		brace_count++;
		break;
	      case BRACERIGHT:
		if (--brace_count < 0)
			fatal_reader("Unmatched `}' on line");
		break;
	      case AMPERSAND:
	      case GREATER:
	      case BAR:
		if (paren_count+brace_count == 0)
			source_p++;
		/* Fall into */
	      case TAB:
	      case SPACE:
		if (paren_count+brace_count > 0)
			break;
		enter_name(string_start,
			   source_p,
			   macro_seen_in_string,
			   &name_string,
			   current_names);
		goto enter_state;
	      case COLON:
		if (paren_count+brace_count > 0)
			break;
		/* End of the target list. We now start reading dependencies or a */
		/* conditional assignment */
		if (separator != none_seen)
			fatal_reader("Extra `:', `::', or `:=' on dependency line");
		/* Enter the last target */
		if ((string_start != source_p) || is_true(macro_seen_in_string))
			enter_name(string_start,
				   source_p,
				   macro_seen_in_string,
				   &name_string,
				   current_names);
		/* Check if it is ":" "::" or ":=" */
scan_colon_label:
		switch (*++source_p) {
		    case NUL:
			get_next_block(source);
			source_p--;
			if (source == NULL)
				goto_state(enter_dependencies_state);
			goto scan_colon_label;
		    case EQUAL:
			separator= conditional_seen;
			source_p++;
			current_names= &depes;
			goto_state(scan_name_state);
		    case COLON:
			separator= two_colon;
			source_p++;
			break;
		    default:
			separator= one_colon;
		};
		current_names= &depes;
		on_eoln_state= enter_dependencies_state;
		goto_state(scan_name_state);
	      case SEMICOLON:
		if (paren_count+brace_count > 0)
			break;
		/* End of reading names. Start reading */
		/* the rule */
		if ((separator != one_colon) && (separator != two_colon))
			fatal_reader("Unexpected command seen");
		/* Enter the last dependency */
		if ((string_start != source_p) || is_true(macro_seen_in_string))
			enter_name(string_start,
				   source_p,
				   macro_seen_in_string,
				   &name_string,
				   current_names);
		source_p++;
		/* Make sure to enter a rule even if the is no text here */
		if (*source_p == NEWLINE) {
			command= command_tail= alloc(Cmd_line);
			command->next= NULL;
			command->command_line= cached_names.empty_suffix_name;
			command->make_refd= false;
			command->ignore_command_dependency= false;
			command->assign= false;
			command->ignore_error= false;
			command->silent= false;
		};
		goto_state(scan_command_state);
	      case PLUS:
		if (paren_count+brace_count > 0)
			break;
		/* We found "+=" construct */
		if (source_p != string_start) {
			/* "+" is not a break char. */
			/* Ignore it if it is part of an identifier */
			source_p++;
			goto resume_name_scan;
		};
		/* Make sure the "+" is followed by a "=" */
scan_append:
		switch (*++source_p) {
		    case NUL:
			get_next_block(source);
			source_p--;
			if (source == NULL)
				goto_state(illegal_eoln_state);
			goto scan_append;
		    case EQUAL:
			append= true;
			break;
		    default:
			/* The "+" just starts a regular name. Start reading that name */
			goto resume_name_scan;
		};
		/* Fall into */
	      case EQUAL:
		if (paren_count+brace_count > 0)
			break;
		/* We found macro assignment. Check if it is legal and if it is appending */
		switch (separator) {
		    case none_seen:
			separator= equal_seen;
			on_eoln_state= enter_equal_state;
			break;
		    case conditional_seen:
			on_eoln_state= enter_conditional_state;
			break;
		    default:
			fatal_reader("Macro assignment on dependency line");
		};
		if (is_true(append))
			source_p--;
		/* Enter the macro name */
		if ((string_start != source_p) || is_true(macro_seen_in_string))
			enter_name(string_start,
				   source_p,
				   macro_seen_in_string,
				   &name_string,
				   current_names);
		if (is_true(append))
			source_p++;
		macro_value= NULL;
		source_p++;
		distance= 0;
		/* Skip whitespace to the start of the value */
		for (; 1; source_p++)
			switch (get_char()) {
			    case NUL:
				get_next_block(source);
				source_p--;
				if (source == NULL)
					goto_state(on_eoln_state);
				break;
			    case BACKSLASH:
				if (*++source_p == NUL) {
					get_next_block(source);
					if (source == NULL)
						goto_state(on_eoln_state);
				};
				break;
			    case NEWLINE:
			    case NUMBERSIGN:
				string_start= source_p;
				goto macro_value_end;
			    case TAB:
			    case SPACE:
				break;
			    default:
				goto macro_value_start;
			};
macro_value_start:
		macro_seen_in_string= false;
		string_start= source_p;
		/* Find the end of the value */
		for (; 1; source_p++) {
		   if (distance != 0)
			*source_p= *(source_p + distance);
		   switch (get_char()) {
		      case NUL:
			if (is_false(macro_seen_in_string)) {
				macro_seen_in_string= true;
				init_string_from_stack(name_string, name_buffer);
			};
			append_string(string_start, &name_string, source_p - string_start);
			get_next_block(source);
			string_start= source_p;
			source_p--;
			if (source == NULL)
				goto_state(on_eoln_state);
			break;
		      case BACKSLASH:
			source_p++;
			if (distance != 0)
				*source_p= *(source_p + distance);
			if (*source_p == NUL) {
				if (is_false(macro_seen_in_string)) {
					macro_seen_in_string= true;
					init_string_from_stack(name_string, name_buffer);
				};
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				get_next_block(source);
				string_start= source_p;
				if (source == NULL)
					goto_state(on_eoln_state);
			};
			if (*source_p == NEWLINE) {
				source_p--;
				distance++;
				*source_p= SPACE;
				while (*(source_p + distance + 1) == NEWLINE)
					distance++;
				while ((*(source_p + distance + 1) == TAB) ||
				       (*(source_p + distance + 1) == SPACE))
					distance++;
			};
			break;
		      case NEWLINE:
		      case NUMBERSIGN:
			goto macro_value_end;
			};
		};
macro_value_end:
		/* Complete the value in the string */
		if (is_false(macro_seen_in_string)) {
			macro_seen_in_string= true;
			init_string_from_stack(name_string, name_buffer);
		};
		append_string(string_start, &name_string, source_p - string_start);
		if (name_string.buffer.start != name_string.text.p)
			macro_value= getname(name_string.buffer.start, FIND_LENGTH);
		if (is_true(name_string.free_after_use))
			Free(name_string.buffer.start);
		for (; distance > 0; distance--)
			*source_p++= SPACE;
		goto_state(on_eoln_state);
	};

/****************************************************************
 *	enter dependencies state
 */
case enter_dependencies_state:
	enter_dependencies_label:
/* Expects pointer on first non whitespace char after last dependency. (On next line.) */
/* We end up here after having read a "targets : dependencies" line */
/* The state checks if there is a rule to read and if so dispatches to scan_command_state */
/* scan_command_state reads one rule line and the returns here */

	/* First check if the first char on the next line is special */
	switch (get_char()) {
	    case NUL:
		get_next_block(source);
		if (source == NULL)
			break;
		goto enter_dependencies_label;
	    case EXCLAM:
		/* The line should be evaluate before it is read */
		macro_seen_in_string= false;
		string_start= source_p + 1;
		for (; 1; source_p++)
		   switch (get_char()) {
		      case NEWLINE:
			goto eoln_2;
		      case NUL:
			if (source->fd > 0) {
				if (is_false(macro_seen_in_string)) {
					macro_seen_in_string= true;
					init_string_from_stack(name_string, name_buffer);
				};
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				get_next_block(source);
				string_start= source_p;
				source_p--;
				break;
			};
	eoln_2:
			if (is_false(macro_seen_in_string))
				init_string_from_stack(name_string, name_buffer);
			append_string(string_start, &name_string, source_p - string_start);
			extrap= (Source) alloca(sizeof(struct Source));
			extrap->string.buffer.start= NULL;
			expand_value(getname(name_string.buffer.start, FIND_LENGTH),
				     &extrap->string);
			if (is_true(name_string.free_after_use))
				Free(name_string.buffer.start);
			uncache_source();
			extrap->string.text.p= extrap->string.buffer.start;
			extrap->fd= -1;
			extrap->previous= source;
			source= extrap;
			cache_source(0);
			goto enter_dependencies_label;
		   };
	    case DOLLAR:
		if (is_true(source->already_expanded))
			break;
		push_macro_value(0);
		goto enter_dependencies_label;
	    case NUMBERSIGN:
		if (makefile_type != reading_makefile) {
			source_p++;
			goto_state(scan_command_state);
		};
		for (; 1; source_p++)
			switch (get_char()) {
			    case NUL:
				get_next_block(source);
				source_p--;
				if (source == NULL)
					goto_state(on_eoln_state);
				break;
			    case BACKSLASH:
				if (*++source_p == NUL) {
					get_next_block(source);
					if (source == NULL)
						goto_state(on_eoln_state);
				};
				break;
			    case NEWLINE:
				source_p++;
				if (source->fd >= 0)
					line_number++;
				goto enter_dependencies_label;
			};
	    case TAB:
		goto_state(scan_command_state);
	};

	/* We read all the command lines for the target/dependency line. Enter the stuff */
	if (is_true(target_group_seen))
		find_target_groups(&target);
	for (nvp= &target; nvp != NULL; nvp= nvp->next)
		for (i= 0; i < nvp->used; i++)
			if (nvp->names[i] != NULL)
				enter_dependencies(nvp->names[i],
						   nvp->target_group[i],
						   &depes,
						   command,
						   separator);
	goto start_new_line;

/****************************************************************
 *	scan command state
 */
case scan_command_state:
	/* We need to read one rule line. Do that and return to */
	/* the enter dependencies state */
	string_start= source_p;
	macro_seen_in_string= false;
	for (; 1; source_p++)
	   switch (get_char()) {
	      case BACKSLASH:
		if (is_false(macro_seen_in_string))
			init_string_from_stack(name_string, name_buffer);
		append_string(string_start, &name_string, source_p - string_start);
		macro_seen_in_string= true;
		if (*++source_p == NUL) {
			get_next_block(source);
			if (source == NULL)
				goto_state(on_eoln_state);
		};
		append_char(BACKSLASH, &name_string);
		append_char(*source_p, &name_string);
		if (*source_p == NEWLINE) {
			if (source->fd >= 0)
				line_number++;
			if (*++source_p == NUL) {
				get_next_block(source);
				if (source == NULL)
					goto_state(on_eoln_state);
			};
			if (*source_p == TAB)
				source_p++;
		} else {
			if (*++source_p == NUL) {
				get_next_block(source);
				if (source == NULL)
					goto_state(on_eoln_state);
			};
		};
		string_start= source_p;
		break;
	      case NEWLINE:
		if ((string_start != source_p) || is_true(macro_seen_in_string)) {
			if (macro_seen_in_string) {
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				string_start= name_string.buffer.start;
				string_end= name_string.text.p;
			} else
				string_end= source_p;
			while ((*string_start != NEWLINE) && isspace(*string_start))
				string_start++;
			if ((string_end > string_start) || (makefile_type == reading_statefile)) {
				if (command_tail == NULL)
					command= command_tail= alloc(Cmd_line);
				else {
					command_tail->next= alloc(Cmd_line);
					command_tail= command_tail->next;
				}
				command_tail->next= NULL;
				command_tail->command_line =
					getname(string_start, string_end - string_start);
				if (is_true(macro_seen_in_string) &&
				    is_true(name_string.free_after_use))
					Free(name_string.buffer.start);
			};
		};
		do {
			if (source->fd >= 0)
				line_number++;
			if (*++source_p == NUL) {
				get_next_block(source);
				if (source == NULL)
					goto_state(on_eoln_state);
			};
		} while (*source_p == NEWLINE);

		goto_state(enter_dependencies_state);
	      case NUL:
		if (is_false(macro_seen_in_string))
			init_string_from_stack(name_string, name_buffer);
		append_string(string_start, &name_string, source_p - string_start);
		macro_seen_in_string= true;
		get_next_block(source);
		string_start= source_p;
		source_p--;
		if (source == NULL)
			goto_state(enter_dependencies_state);
		break;
	   };

/****************************************************************
 *	enter equal state
 */
case enter_equal_state:
	if (target.used != 1)
		goto_state(poorly_formed_macro_state);
	enter_equal(target.names[0], macro_value, append);
	goto start_new_line;

/****************************************************************
 *	enter conditional state
 */
case enter_conditional_state:
	if (depes.used != 1)
		goto_state(poorly_formed_macro_state);
	for (nvp= &target; nvp != NULL; nvp= nvp->next)
		for (i= 0; i < nvp->used; i++)
			enter_conditional(nvp->names[i],
					  depes.names[0],
					  macro_value,
					  append);
	goto start_new_line;

/****************************************************************
 *	Error states
 */
case illegal_eoln_state:
	fatal_reader("Unexpected end of line seen");
case poorly_formed_macro_state:
	fatal_reader("Badly formed macro assignment");
case exit_state:
	return;
default:
	fatal_reader("Internal error. Unknown reader state");
};
}

/*
 *	Make the makefile and setup to read it. Actually read it if it is stdio.
 */
Boolean
read_simple_file(makefile_name, chase_path, doname_it, complain, must_exist, report_file, lock_makefile)
	register Name           makefile_name;
	register Boolean        chase_path;
	register Boolean        doname_it;
	Boolean			complain;
	Boolean			must_exist;
	Boolean                 report_file;
	Boolean			lock_makefile;
{
	register Source         source= (Source) Malloc(sizeof(struct Source));
	register Property       makefile= maybe_append_prop(makefile_name, makefile_prop);
	Property                orig_makefile= makefile;
	register char          *p;
	register int            length;
	register int            n;
	Dependency              dp;
	Dependency             *dpp;
	char                   *path;
	char			*previous_file_being_read= file_being_read;
	int			previous_line_number= line_number;

	if ((makefile_name->hash.length != 1) || (makefile_name->string[0] != HYPHEN)) {
		if ((makefile->body.makefile.contents == NULL) && (is_true(doname_it))) {
			if (makefile_path == NULL) {
				add_dir_to_path(".", &makefile_path, -1);
				add_dir_to_path("/usr/include/make", &makefile_path, -1);
			};
			if (doname(makefile_name, true, false) == build_dont_know) {
				n= access_vroot(makefile_name->string,
						4,
						is_true(chase_path) ?
							makefile_path : NULL, VROOT_DEFAULT);
				if (n == 0) {
					get_vroot_path((char **) NULL, &path, (char **)NULL);
					if ((path[0] == PERIOD) && (path[1] == SLASH))
						path += 2;
					makefile_name= getname(path, FIND_LENGTH);
				};
			};
		};
		source->string.free_after_use= false;
		source->previous= NULL;
		source->already_expanded= false;
		if (max_include_depth-- <= 0)
			fatal("Too many nested include statements");
		if (makefile->body.makefile.contents == NULL) {
			if (is_true(doname_it) &&
			    (doname(makefile_name, true, false) == build_failed)) {
				if (is_true(complain))
					(void)fprintf(stderr, "make: Couldn't make `%s'\n",
						makefile_name->string);
				return(failed);
			}
			if (exists(makefile_name) == FILE_DOESNT_EXIST) {
				if (is_true(complain) || makefile_name->stat.errno != ENOENT)
					if (is_true(must_exist))
						fatal("Can't find `%s': %s",
							makefile_name->string,
							errmsg(makefile_name->stat.errno));
					else
						warning("Can't find `%s': %s",
							makefile_name->string,
							errmsg(makefile_name->stat.errno));
				return(failed);
			}
			orig_makefile->body.makefile.size= makefile->body.makefile.size =
				source->bytes_left_in_file= makefile_name->stat.size;
			if (is_true(report_file)) {
				for (dpp= &makefiles_used;
				     *dpp != NULL;
				     dpp= &(*dpp)->next);
				dp= alloc(Dependency);
				dp->next= NULL;
				dp->name= makefile_name;
				dp->automatic= false;
				dp->stale= false;
				*dpp= dp;
			};
			source->fd= open_vroot(makefile_name->string, O_RDONLY, 0, NULL, VROOT_DEFAULT);
			if (source->fd < 0) {
				if (is_true(complain) || errno != ENOENT)
					if (is_true(must_exist))
						fatal("Can't open `%s': %s",
							makefile_name->string,
							errmsg(errno));
					else
						warning("Can't open `%s': %s",
							makefile_name->string,
							errmsg(errno));
				return(failed);
			}
			(void)fcntl(source->fd, F_SETFD, 1);
			/* Lock the file for read */
			if (is_true(lock_makefile)) {
				lock_file(makefile_name->string);
			};
			orig_makefile->body.makefile.contents=
				makefile->body.makefile.contents =
				source->string.text.p= source->string.buffer.start =
					Malloc((int)(makefile_name->stat.size + 1));
			source->string.text.end= source->string.text.p;
			source->string.buffer.end=
				source->string.text.p + makefile_name->stat.size;
		} else {
			source->fd= -1;
			source->string.text.p=
				source->string.buffer.start=
					makefile->body.makefile.contents;
			source->string.text.end=
				source->string.text.p + makefile->body.makefile.size;
			source->bytes_left_in_file= makefile->body.makefile.size;
			source->string.buffer.end=
				source->string.text.p + makefile->body.makefile.size;
		};
		file_being_read= makefile_name->string;
	} else {
		makefile_name= cached_names.standard_in;
		source->string.free_after_use= false;
		source->previous= NULL;
		source->bytes_left_in_file= 0;
		source->already_expanded= false;
		source->fd= -1;
		source->string.buffer.start= source->string.text.p= Malloc(length= 1024);
		source->string.buffer.end= source->string.text.p + length;
		file_being_read= "standard input";
		line_number= 0;
		while ((n= read(fileno(stdin), source->string.text.p, length)) > 0) {
		    length -= n;
		    source->string.text.p += n;
		    if (length == 0) {
			p= Malloc(length= 1024 +
				   (source->string.buffer.end - source->string.buffer.start));
			(void)strncpy(p,
				      source->string.buffer.start,
				      source->string.buffer.end - source->string.buffer.start);
			Free(source->string.buffer.start);
			source->string.text.p= p +
				(source->string.buffer.end - source->string.buffer.start);
			source->string.buffer.start= p;
			source->string.buffer.end= source->string.buffer.start + length;
			length= 1024;
		    }
		};
		if (n < 0)
			fatal("Error reading standard input: %s",
			    errmsg(errno));
		source->string.text.p= source->string.buffer.start;
		source->string.text.end= source->string.buffer.end - length;
	};
	line_number= 1;
	parse_makefile(source);
	file_being_read= previous_file_being_read;
	line_number= previous_line_number;
	max_include_depth++;
	return(succeeded);
}
