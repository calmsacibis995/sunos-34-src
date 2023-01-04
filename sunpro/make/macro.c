#ifndef lint
static char sccsid[]= "@(#)macro.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc	[Remotely from S5R2]
 */


#include "defs.h"
#include <ctype.h>

static char *		current_string;

/*
 *	Return expanded value of macro.
 */
Name
getvar(name)
	register Name		name;
{
	String			destination;
	char			buffer[STRING_BUFFER_LENGTH];
	register Name		result;

	init_string_from_stack(destination, buffer);
	expand_value(maybe_append_prop(name, macro_prop)->body.macro.value, &destination);
	result= getname(destination.buffer.start, FIND_LENGTH);
	if (is_true(destination.free_after_use))
		Free(destination.buffer.start);
	return(result);
}

/*
 *	Set a macro value, possibly supplying a daemon to be used when referencing the value
 */
Property
setvar_daemon(name, value, append, daemon)
	register Name		name;
	register Name		value;
	Boolean			append;
	enum daemon		daemon;
{
	register Property	macro= maybe_append_prop(name, macro_prop);
	String			destination;
	char			buffer[STRING_BUFFER_LENGTH];
	register Chain		chain;

	if ((makefile_type != reading_nothing) && is_true(macro->body.macro.read_only))
		return(macro);
	if (is_true(append)) {
		/* If we are appending we just tack the new value after the old */
		/* one with a space in between */
		init_string_from_stack(destination, buffer);
		if ((macro != NULL) && (macro->body.macro.value != NULL)) {
			append_string(macro->body.macro.value->string,
				      &destination,
				      (int)macro->body.macro.value->hash.length);
			if (value != NULL)
				append_char(SPACE, &destination);
		};
		if (value != NULL)
			append_string(value->string,
				      &destination,
				      (int)value->hash.length);
		value= getname(destination.buffer.start, FIND_LENGTH);
		if (is_true(destination.free_after_use))
			Free(destination.buffer.start);
	};
	/* Debugging trace */
	if (debug_level > 1) {
		if (value != NULL)
			switch (daemon) {
			    case chain_daemon:
				(void)printf("%s =", name->string);
				for (chain= (Chain) value;
				     chain != NULL;
				     chain= chain->next)
					(void)printf(" %s", chain->name->string);
				(void)printf("\n");
				break;
			    case no_daemon:
				(void)printf("%s= %s\n", name->string, value->string);
				break;
			}
		else
			(void)printf("%s =\n", name->string);
	};
	/* Set the new values in the macro property block */
	macro->body.macro.value= value;
	macro->body.macro.daemon= daemon;
	/* If the user changes the VIRTUAL_ROOT we need to flush the vroot package cache */
	if (name == cached_names.path)
		flush_path_cache();
	if (name == cached_names.virtual_root)
		flush_vroot_cache();
	/* If this sets the VPATH we remember that */
	if ((name == cached_names.vpath) && (value != NULL) && (value->hash.length > 0))
		vpath_defined= true;
	/* For environment variables we also set the environment value each time */
	if (is_true(macro->body.macro.exported)) {
		if (value == NULL)
			(void)setenv(name->string, "");
		else {
			init_string_from_stack(destination, buffer);
			expand_value(value, &destination);
			(void)setenv(name->string, destination.buffer.start);
			if (is_true(destination.free_after_use))
				Free(destination.buffer.start);
		};
	};
	return(macro);
}

/*
 *	This is the daemon that handles $? type values
 */
static void
expand_chainblock_list(chain, destination)
	register Chain		chain;
	register String		*destination;
{
	/* Just loop thru the list and append each string to the string we are */
	/* building */
	for (; chain != NULL; chain= chain->next) {
		append_string(chain->name->string,
			      destination,
			      (int)chain->name->hash.length);
		if (chain->next != NULL)
			append_char(SPACE, destination);
	};
}

/*
 *	expand_value() recursively expands all macros in the string value.
 *	destination is where the expanded value should be appended.
 */
void
expand_value(value, destination)
	Name			value;
	register String		*destination;
{
	struct	Source           sourceb;
	register struct Source	*source= &sourceb;
	register char		*source_p;
	register char		*source_end;
	char			*block_start;
	char		        *previous_string= current_string;

	if (value == NULL) {
		/* Make sure to get a string allocated even if it will be empty */
		append_string("", destination, FIND_LENGTH);
		destination->text.end= destination->text.p;
		return;
	};
	if (is_false(value->dollar)) {
		/* If the value we are expanding does not contain any $ we dont */
		/* have to parse it */
		append_string(value->string, destination, (int)value->hash.length);
		destination->text.end= destination->text.p;
		return;
	};

	if (is_true(value->being_expanded))
		fatal_reader("Loop detected when expanding macro value `%s'", value->string);
	value->being_expanded= true;
	/* Setup the structure we read from */
	sourceb.string.text.p= sourceb.string.buffer.start= value->string;
	sourceb.string.text.end= sourceb.string.buffer.end=
				sourceb.string.text.p + value->hash.length;
	sourceb.string.free_after_use= false;
	sourceb.previous= NULL;
	sourceb.fd= -1;
	current_string= value->string;
	/* Lift some pointers from the struct to local register variables */
	cache_source(0);
/* We parse the string in segments */
/* We read chars until we find a $, then we append what we have read so far */
/* (since last $ processing) to the destination. When we find a $ we call */
/* expand_macro() and let it expand that particular $ reference into dest */
	block_start= source_p;
	for (; 1; source_p++)
		switch (get_char()) {
		    case DOLLAR:
			/* Save the plain string we found since start of string */
			/* or previous $ */
			append_string(block_start, destination, source_p - block_start);
			source->string.text.p= ++source_p;
			uncache_source();
			/* Go expand the macro reference */
			expand_macro(source, destination);
			cache_source(1);
			block_start= source_p + 1;
			break;
		    case NUL:
			/* The string ran out. Get some more */
			append_string(block_start, destination, source_p - block_start);
			get_next_block(source);
			if (source == NULL) {
				destination->text.end= destination->text.p;
				value->being_expanded= false;
				return;
			};
			block_start= source_p;
			source_p--;
			break;
		};
	current_string= previous_string;
}

/*
 *	expand_value_with_daemon() checks for daemons and then maybe calls expand_value()
 */
static void
expand_value_with_daemon(macro, destination)
	register Property	macro;
	register String		*destination;
{
	switch (macro->body.macro.daemon) {
	    case no_daemon:
		expand_value(macro->body.macro.value, destination);
		return;
	    case chain_daemon:
		/* If this is a $? value we call the daemon to translate the */
		/* list of names to a string */
		expand_chainblock_list((Chain) macro->body.macro.value, destination);
		return;
	};
}

/*
 *	expand_macro() should be called with source->string.text.p pointing to the first
 *	char after the $ that starts a macro reference.
 *	source->string.text.p is returned pointing to the first char after the macro name.
 *	It will read the macro name, expanding any macros in it,
 *	and get the value. The value is then expanded.
 *	destination is a String that is filled in with the expanded macro.
 *	It may be passed in referencing a buffer to expand the macro into.
 */

void
expand_macro(source, destination)
	register Source		source;
	register String		*destination;
{
	register char		*source_p= source->string.text.p;
	register char		*source_end= source->string.text.end;
	Property		macro= NULL;
	Name			name;
	char			*block_start;
	char			*colon;
	char			*eq= NULL;
	char			*percent;
	char			*p;
	register int		closer;
	register int		closer_level= 1;
	String			string;
	char			buffer[STRING_BUFFER_LENGTH];
	String			extracted;
	char			extracted_string[MAXPATHLEN];
	char			*left_head;
	char			*left_tail;
	char			*right_head;
	char			*right_tail;
	int			left_head_len;
	int			left_tail_len;
	int			right_head_len;
	int			right_tail_len;
	enum {
		no_replace,
		suffix_replace,
		pattern_replace
	}			replacement= no_replace;
	enum {
		no_extract,
		dir_extract,
		file_extract
	}                       extraction= no_extract;

	/* First copy the (macro-expanded) macro name into string */
	init_string_from_stack(string, buffer);
recheck_first_char:
	/* Check the first char of the macro name to figure out what to do */
	switch (get_char()) {
	    case NUL:
		get_next_block(source);
		if (source == NULL)
			fatal_reader("'$' at end of string `%s'", current_string);
		goto recheck_first_char;
	    case PARENLEFT:
		/* Multi char name */
		closer= PARENRIGHT;
		break;
	    case BRACELEFT:
		/* Multi char name */
		closer= BRACERIGHT;
		break;
	    case NEWLINE:
		fatal_reader("'$' at end of line");
	    default:
		/* Single char macro name. Just suck it up */
		append_char(*source_p, &string);
		source->string.text.p= source_p + 1;
		goto get_macro_value;
	};

	/* Handle multi-char macro names */
	block_start= ++source_p;
	for (; 1; source_p++)
		switch (get_char()) {
		    case NUL:
			append_string(block_start, &string, source_p - block_start);
			get_next_block(source);
			if (source == NULL) {
				if (current_string != NULL)
					fatal_reader("Unmatched `%c' in string `%s'",
						closer == BRACERIGHT? '{' : '(',
						current_string);
				else
					fatal_reader("Premature EOF");
			}
			block_start= source_p;
			source_p--;
			break;
		    case NEWLINE:
			fatal_reader("Unmatched `%c' on line",
				closer == BRACERIGHT? '{' : '(');
		    case DOLLAR:
			/* Macro names may reference macros. This expands the */
			/* value of such macros into the macro name string */
			append_string(block_start, &string, source_p - block_start);
			source->string.text.p= ++source_p;
			uncache_source();
			expand_macro(source, &string);
			cache_source(0);
			block_start= source_p;
			source_p--;
			break;
		    case PARENLEFT:
			/* Allow nested pairs of () in the macro name */
			if (closer == PARENRIGHT)
				closer_level++;
			break;
		    case BRACELEFT:
			/* Allow nested pairs of {} in the macro name */
			if (closer == BRACERIGHT)
				closer_level++;
			break;
		    case PARENRIGHT:
		    case BRACERIGHT:

			/* End of the name. Save the string in the macro name */
			/* string */
			if ((*source_p == closer) && (--closer_level <= 0)) {
				source->string.text.p= source_p + 1;
				append_string(block_start,
					      &string,
					      source_p - block_start);
				goto get_macro_value;
			};
			break;
		};
	/* We got the macro name. We now inspect it to see if it specifies any */
	/* translations of the value */
get_macro_value:
	name= NULL;
	/* First check if we have a $(@D) type translation */
	if ((funny[string.buffer.start[0]] & SPECIAL_MACRO_FUNNY) &&
	    (string.text.p - string.buffer.start >= 2) &&
	    ((string.buffer.start[1] == 'D') || (string.buffer.start[1] == 'F'))) {
		switch (string.buffer.start[1]) {
		    case 'D':
			extraction= dir_extract;
			break;
		    case 'F':
			extraction= file_extract;
			break;
		    default:
			fatal_reader("Illegal macro reference `%s'", string.buffer.start);
		};
		/* Internalize the macro name using the first char only */
		name= getname(string.buffer.start, 1);
		(void)strcpy(string.buffer.start, string.buffer.start+2);
	};
	/* Check for other kinds of translations */
	if ((colon= index(string.buffer.start, COLON)) != NULL) {
		/* We have a $(FOO:.c=.o) type translation. Get the */
		/* name of the macro proper */
		if (name == NULL)
			name= getname(string.buffer.start, colon - string.buffer.start);
		/* Pickup all the translations */
		if ((percent= index(colon + 1, PERCENT)) == NULL)
		    while (colon != NULL) {
			if ((eq= index(colon + 1, EQUAL)) == NULL)
				fatal("= missing from := transformation");
			left_tail_len= eq - colon - 1;
			left_tail= alloca(left_tail_len + 1);
			(void)strncpy(left_tail, colon + 1, eq - colon - 1);
			left_tail[eq - colon - 1]= NUL;
			replacement= suffix_replace;
			if ((colon= index(eq + 1, COLON)) != NULL) {
				right_tail= alloca(colon - eq);
				(void)strncpy(right_tail, eq + 1, colon - eq - 1);
				right_tail[colon - eq - 1]= NUL;
			} else {
				right_tail= alloca(strlen(eq) + 1);
				(void)strcpy(right_tail, eq + 1);
			};
		    } else {
			if ((eq= index(colon + 1, EQUAL)) == NULL)
				fatal("= missing from := transformation");
			if ((percent= index(colon + 1, PERCENT)) == NULL)
				fatal("%% missing from := transformation");
			if (eq < percent)
				fatal("%% missing from pattern of := transformation");

			if (percent > colon+1) {
				left_head= alloca(percent-colon);
				(void)strncpy(left_head, colon+1, percent-colon-1);
				left_head[percent-colon-1]= NUL;
				left_head_len= percent-colon-1;
			} else {
				left_head= NULL;
				left_head_len= 0;
			};

			if (eq > percent+1) {
				left_tail= alloca(eq-percent);
				(void)strncpy(left_tail, percent+1, eq-percent-1);
				left_tail[eq-percent-1]= NUL;
				left_tail_len= eq-percent-1;
			} else {
				left_tail= NULL;
				left_tail_len= 0;
			};

			if ((percent= index(eq, PERCENT)) == NULL) {
				right_head_len= strlen(eq)-1;
				right_head= alloca(right_head_len+1);
				(void)strcpy(right_head, eq+1);
				right_tail= NULL;
				right_tail_len= 0;
			} else {
				right_head= alloca(percent-eq);
				(void)strncpy(right_head, eq+1, percent-eq-1);
				right_head[percent-eq-1]= NUL;
				right_head_len= percent-eq-1;

				right_tail_len= strlen(percent)-1;
				right_tail= alloca(right_tail_len+1);
				(void)strcpy(right_tail, percent+1);
			};
			replacement= pattern_replace;
		    };
	};
	if (name == NULL)
		/* No translations found. Use the whole string as the macro name */
		name= getname(string.buffer.start,
			      string.text.p - string.buffer.start);
	if (is_true(string.free_after_use))
		Free(string.buffer.start);
	if (name == cached_names.make)
		make_word_mentioned= true;
	if (name == cached_names.query)
		query_mentioned= true;
	/* Get the macro value */
	macro= get_prop(name->prop, macro_prop);
	if ((macro != NULL) && is_true(macro->body.macro.is_conditional)) {
		is_conditional= true;
		if (makefile_type == reading_makefile)
			warning("Possibly conditional macro `%s' referenced when reading makefile",
					name->string);
		};
	/* Macro name read and parsed. Expand the value. */
	if ((macro == NULL) || (macro->body.macro.value == NULL))
		/* If the value is empty we just get out of here */
		goto exit;
	if ((replacement != no_replace) || (extraction != no_extract)) {
		/* If there were any transforms specified in the macro name we */
		/* deal with them here */
		init_string_from_stack(string, buffer);
		/* Expand the value into a local string buffer */
		expand_value_with_daemon(macro, &string);
		/* Scan the expanded string */
		p= string.buffer.start;
		while (*p != NUL) {
			/* First skip over any white space and append that to */
			/* the destination string */
			block_start= p;
			while ((*p != NUL) && isspace(*p))
				p++;
			append_string(block_start, destination, p - block_start);
			/* Then find the end of the next word */
			block_start= p;
			while ((*p != NUL) && !isspace(*p))
				p++;
			/* If we cant find another word we are done */
			if (block_start == p)
				break;
			/* Then apply the transforms to the word */
			init_string_from_stack(extracted, extracted_string);
			switch (extraction) {
			      case dir_extract:
				/* $(@D) type transform. Extract the */
				/* path from the word. Deliver "." if */
				/* none is found */
				eq= rindex(block_start, SLASH);
				if ((eq == NULL) || (eq > p)) {
					append_string(".", &extracted, 1);
				} else {
					append_string(block_start,
						      &extracted,
						      eq - block_start);
				};
				break;
			      case file_extract:
				/* $(@F) type transform. Remove the path from the word if any */
				eq= rindex(block_start, SLASH);
				if ((eq == NULL) || (eq > p)) {
					append_string(block_start, &extracted, p - block_start);
				} else {
					append_string(eq + 1, &extracted, p - eq - 1);
				};
				break;
			      case no_extract:
				append_string(block_start, &extracted, p - block_start);
				break;
			};
			switch (replacement) {
			      case suffix_replace:
				/* $(FOO:.o=.c) type transform. Maybe */
				/* replace the tail of the word */
				if (((extracted.text.p - extracted.buffer.start) >= left_tail_len) &&
				    is_equaln(extracted.text.p - left_tail_len, left_tail, left_tail_len)) {
					append_string(extracted.buffer.start,
						      destination,
						      (extracted.text.p - extracted.buffer.start) - left_tail_len);
					append_string(right_tail, destination, FIND_LENGTH);
				} else {
					append_string(extracted.buffer.start,
						      destination,
						      FIND_LENGTH);
				};
				break;
			      case pattern_replace:
				/* $(X:a%b=c%d) type transform */
				if (((extracted.text.p - extracted.buffer.start) >= left_head_len+left_tail_len) &&
				    is_equaln(left_head, extracted.buffer.start, left_head_len) &&
				    is_equaln(left_tail, extracted.text.p - left_tail_len, left_tail_len)) {
					if (right_tail == NULL)
						append_string(right_head, destination, right_head_len);
					else {
						append_string(right_head, destination, right_head_len);
						append_string(extracted.buffer.start + left_head_len,
							      destination,
							      (extracted.text.p - extracted.buffer.start)-left_head_len-left_tail_len);
						append_string(right_tail, destination, right_tail_len);
					};
				} else
					append_string(extracted.buffer.start, destination, FIND_LENGTH);
				break;
			      case no_replace:
				append_string(extracted.buffer.start, destination, FIND_LENGTH);
				break;
			    };
		};
		if (is_true(string.free_after_use))
			Free(string.buffer.start);
	} else
		/* This is for the case when the macro name did not specify transforms */
		expand_value_with_daemon(macro, destination);
exit:
	*destination->text.p= NUL;
	destination->text.end= destination->text.p;
}
