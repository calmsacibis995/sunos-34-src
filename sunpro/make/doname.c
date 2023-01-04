#ifndef lint
static char sccsid[]= "@(#)doname.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc	[Remotely from S5R2]
 */

#include "defs.h"
#include "funny.h"
#include <sys/file.h>
#include <ctype.h>

/*
 *	dynamic_dependencies() checks if any dependency contains a macro ref
 *	If so, it replaces the dependency with the expanded version.
 *	Here, "$@" gets translated to target->string. That is
 *	the current name on the left of the colon in the
 *	makefile.  Thus,
 *		xyz:	s.$@.c
 *	translates into
 *		xyz:	s.xyz.c
 *
 *	Also, "$(@F)" translates to the same thing without a preceeding
 *	directory path (if one exists).
 *	Note, to enter "$@" on a dependency line in a makefile
 *	"$$@" must be typed. This is because make expands
 *	macros in dependency lists upon reading them.
 *	dynamic_dependencies() also expands file wildcards.
 *	If there are any Shell meta characters in the name,
 *	search the directory, and replace the dependency
 *	with the set of files the pattern matches
 */
static void
dynamic_dependencies(target)
	Name			target;
{
	char			pattern[MAXPATHLEN];
	register char		*p;
	Property		line;
	register Dependency	dependency;
	register Dependency	*remove;
	String			string;
	char			buffer[MAXPATHLEN];
	register Boolean	set_at= false;
	register char		*start;
	Dependency		new_depe;
	register Boolean	reuse_cell;
	Dependency		first_member;
	Name			directory;
	Name			lib;
	Name			member;
	Property		prop;
	Name			true_target= target;
	char			parenright;
	char			parenleft;
	char			*library;

	target->has_depe_list_expanded= true;
	if ((line= get_prop(target->prop, line_prop)) == NULL)
		return;
	/* If the target is constructed from a "::" target we consider that */
	if (target->has_target_prop)
		true_target= get_prop(target->prop, target_prop)->body.target.target;
	/* Scan all dependencies and process the ones that contain "$" chars */
	for (dependency= line->body.line.dependencies;
	     dependency != NULL;
	     dependency= dependency->next) {
		if (is_false(dependency->name->dollar))
			continue;
		/* The make macro $@ is bound to the target name once per */
		/* invocation of dynamic_dependencies() */
		if (is_false(set_at)) {
			(void)setvar(cached_names.c_at, true_target, false);
			set_at= true;
		};
		/* Expand this dependency string */
		init_string_from_stack(string, buffer);
		expand_value(dependency->name, &string);
		/* Scan the expanded string. It could contain whitespace */
		/* which mean it expands to several dependencies */
		start= string.buffer.start;
		first_member= NULL;
		/* We use the original dependency cell for the first */
		/* dependency from the expansion */
		reuse_cell= true;
		/* We also have to deal with dependencies that expand to */
		/* lib.a(members) notation */
		for (p= start; *p != NUL; p++)
		    if ((*p == PARENLEFT) || (*p == BRACELEFT)) {
			lib= getname(start, p - start);
			if (*p == BRACELEFT) {
				lib->member_class= new_member;
				parenleft= BRACELEFT;
				parenright= BRACERIGHT;
			} else {
				lib->member_class= old_member;
				parenleft= PARENLEFT;
				parenright= PARENRIGHT;
			};
			first_member= dependency;
			start= p + 1;
			break;
		    };
		do {
		    /* First skip whitespace */
		    for (p= start; *p != NUL; p++)
			if ((*p == NUL) || isspace(*p) || (*p == PARENRIGHT) || (*p == BRACERIGHT))
				break;
		    /* Enter dependency from expansion if there is one */
		    if (p != start) {
			/* Create new dependency cell if this is not */
			/* the first dependency picked from the */
			/* expansion */
			if (is_false(reuse_cell)) {
				new_depe= alloc(Dependency);
				new_depe->next= dependency->next;
				new_depe->automatic= false;
				new_depe->stale= false;
				dependency->next= new_depe;
				dependency= new_depe;
			};
			reuse_cell= false;
			/* Internalize the dependency name */
			dependency->name= getname(start, p - start);
			if ((debug_level > 0) && (first_member == NULL))
				(void)printf("%*sDynamic dependency `%s' for target `%s'\n",
					     BLANKS,
					     dependency->name->string,
					     true_target->string);
			for (start= p; isspace(*start); start++);
			p= start;
		    };
		} while ((*p != NUL) && (*p != PARENRIGHT) && (*p != BRACERIGHT));
		/* If the expansion was of lib.a(members) format we now enter */
		/* the proper member cells */
		if (first_member != NULL)
		    /* Scan the new dependencies and transform them from */
		    /* "foo" to "lib.a(foo)" */
		    for (; 1; first_member= first_member->next) {
			/* Build "lib.a(foo)" name */
			init_string_from_stack(string, buffer);
			append_string(lib->string, &string, (int)lib->hash.length);
			append_char(parenleft, &string);
			append_string(first_member->name->string, &string, FIND_LENGTH);
			append_char(parenright, &string);
			member= first_member->name;
			/* Replace "foo" with "lib.a(foo)" */
			first_member->name= getname(string.buffer.start, FIND_LENGTH);
			if (is_true(string.free_after_use))
				Free(string.buffer.start);
			if (debug_level > 0)
				(void)printf("%*sDynamic dependency `%s' for target `%s'\n",
					     BLANKS,
					     first_member->name->string,
					     true_target->string);
			first_member->name->member_class= lib->member_class;
			/* Add member property to member */
			prop= maybe_append_prop(first_member->name, member_prop);
			prop->body.member.library= lib;
			prop->body.member.entry= NULL;
			prop->body.member.member= member;
			if (first_member == dependency)
				break;
		    };
	};
	/* Then scan all the dependencies again. This time we want to expand */
	/* shell file wildcards */
	for (remove= &line->body.line.dependencies, dependency= *remove;
	     dependency != NULL;
	     dependency= *remove)
	    /* If dependency name string contains shell wildcards replace */
	    /* the name with the expansion */
	    if (is_true(dependency->name->wildcard)) {
		if ((start= index(dependency->name->string, PARENLEFT)) != NULL) {
			/* lib(*) type pattern */
			library= buffer;
			(void)strncpy(buffer, dependency->name->string, start-dependency->name->string);
			buffer[start-dependency->name->string]= NUL;
			(void)strncpy(pattern,
				      start+1,
				      (int)(dependency->name->hash.length-(start-dependency->name->string)-2));
			pattern[dependency->name->hash.length-(start-dependency->name->string)-2]= NUL;
		} else {
			library= NULL;
			(void)strncpy(pattern,
				      dependency->name->string,
				      (int)dependency->name->hash.length);
			pattern[dependency->name->hash.length]= NUL;
		};
		if ((start= rindex(pattern, SLASH)) == NULL) {
			directory= cached_names.dot;
			p= pattern;
		} else {
			directory= getname(pattern, start-pattern);
			p= start+1;
		};
		/* The expansion is handled by the read_dir() routine */
		read_dir(directory, p, line, library);
		*remove= (*remove)->next;
	    } else
		remove= &dependency->next;

	/* Then unbind $@ */
	(void)setvar(cached_names.c_at, (Name) NULL, false);
}

/*
 *	find_percent_rule() tries to find a rule from the list of wildcard
 *	matched rules.
 *	It scans the list attempting to match the target.
 *	For each target match it checks if the corresponding source exists.
 *	If it does the match is returned.
 *	The percent_list is built at makefile read time.
 *	Each percent rule get one entry on the list.
 */
static Doname
find_percent_rule(target, command)
	register Name		target;
	Property		*command;
{
	register Percent	pat_list;
	String			source_string;
	char			buffer[STRING_BUFFER_LENGTH];
	register Name		source;
	register Property	line;
	Name			true_target= target;
	register int		prefix_length;
	register int		suffix_length;

	/* If the target is constructed for a "::" target we consider that */
	if (target->has_target_prop)
		true_target= get_prop(target->prop, target_prop)->body.target.target;
	if (debug_level > 1)
		(void)printf("%*sLooking for %% rule for %s\n",
			     BLANKS, true_target->string);
	for (pat_list= percent_list;
	     pat_list != NULL;
	     pat_list= pat_list->next) {
		/* Avoid infinite recursion when expanding patterns */
		if (pat_list->being_expanded == true)
			continue;
		/* Compare the target name with the head/tail of pattern */
		/* If the pattern head/tail refs macros they are expanded */
		if (is_false(pat_list->target_prefix->dollar)) {
			if (!is_equaln(true_target->string,
				       pat_list->target_prefix->string,
				       prefix_length= (int)pat_list->target_prefix->hash.length))
				continue;
		} else {
			init_string_from_stack(source_string, buffer);
			expand_value(pat_list->target_prefix, &source_string);
			if (!is_equaln(true_target->string,
				       source_string.buffer.start,
				       prefix_length= source_string.text.p-source_string.buffer.start))
				continue;
		};
		if (is_false(pat_list->target_suffix->dollar)) {
			suffix_length= pat_list->target_suffix->hash.length;
			if (!is_equal(true_target->string + true_target->hash.length -
						pat_list->target_suffix->hash.length,
				      pat_list->target_suffix->string))
				continue;
		} else {
			init_string_from_stack(source_string, buffer);
			expand_value(pat_list->target_suffix, &source_string);
			if (!is_equal(true_target->string + true_target->hash.length -
						(suffix_length= source_string.text.p-source_string.buffer.start),
				      source_string.buffer.start))
				continue;
		};
		/* The rule matched the target. Construct the source name as */
		/* "source head" + "target body" + "source tail" */
		init_string_from_stack(source_string, buffer);
		if (is_false(pat_list->source_prefix->dollar))
			append_string(pat_list->source_prefix->string,
				      &source_string,
				      (int)pat_list->source_prefix->hash.length);
		else
			expand_value(pat_list->source_prefix, &source_string);
		if (is_true(pat_list->source_percent))
			append_string(true_target->string + prefix_length,
				      &source_string,
				      (int)(true_target->hash.length - prefix_length - suffix_length));
		if (is_false(pat_list->source_suffix->dollar))
			append_string(pat_list->source_suffix->string,
				      &source_string,
				      (int)pat_list->source_suffix->hash.length);
		else
			expand_value(pat_list->source_suffix, &source_string);
		/* Internalize the synthesized source name */
		source= getname(source_string.buffer.start, FIND_LENGTH);
		if (is_true(source_string.free_after_use))
			Free(source_string.buffer.start);
		if (debug_level > 1)
			(void)printf("%*sTrying %s\n", BLANKS, source->string);
		/* Try to build the source */
		pat_list->being_expanded= true;
		switch (doname(source, true, true)) {
		    case build_dont_know:
			/* If make cant figure out how to build the source we */
			/* just try the next percent rule */
			pat_list->being_expanded= false;
			if (source->stat.time == FILE_DOESNT_EXIST)
				continue;
		    case build_ok:
			/* If we managed to build the source this rule is a match */
			break;
		    case build_failed:
			/* If the build of the source failed we give up */
			/* looking for a percent rule and propagate the error */
			pat_list->being_expanded= false;
			return(build_failed);
		};
		/* We matched the rule since the source exists */
		/* Now make sure "%.o: %.c" behaves the same as "foo.o: foo.c" */
		/* by saying that the target we matched has been mentioned in the makefile */
		if (true_target->colons == no_colon)
			true_target->colons= one_colon;
		pat_list->being_expanded= false;
		if (debug_level > 1)
			(void)printf("%*sMatched %s: %s from %s%%%s: %s%s%s\n",
				     BLANKS,
				     true_target->string,
				     source->string,
				     pat_list->target_prefix->string,
				     pat_list->target_suffix->string,
				     pat_list->source_prefix->string,
				     is_true(pat_list->source_percent) ? "%":"",
				     pat_list->source_suffix->string);
/* Since it is possible that the same target is built several times during the make run */
/* we have to patch the target with all information we found here */
/* Thus the target will have an explicit rule the next time around */
/* Enter the synthesized source as a dependency for the target in case the target is built again */
		enter_dependency(line= maybe_append_prop(target, line_prop),
				 source,
				 false);
		*command= line;
		if ((source->stat.time > (*command)->body.line.dependency_time) &&
		    (debug_level > 1))
			(void)printf("%*sDate(%s)=%s Date-dependencies(%s)=%s\n",
				     BLANKS,
				     source->string,
				     time_to_string(source->stat.time),
				     true_target->string,
				     time_to_string((*command)->body.line.dependency_time));
		/* Determine if this new dependency make the target out of date */
		(*command)->body.line.dependency_time=
			max((*command)->body.line.dependency_time, source->stat.time);
		if (out_of_date(true_target->stat.time,
				(*command)->body.line.dependency_time)) {
			line->body.line.is_out_of_date= true;
			if (debug_level > 0)
				(void)printf("%*sBuilding %s using percent rule for %s%%%s: %s%s%s because it is out of date relative to %s\n",
					     BLANKS,
					     true_target->string,
					     pat_list->target_prefix->string,
					     pat_list->target_suffix->string,
					     pat_list->source_prefix->string,
					     is_true(pat_list->source_percent) ? "%":"",
					     pat_list->source_suffix->string,
					     source->string);
		};
		/* And stuff the rule we found as an explicit rule for target */
		line->body.line.sccs_command= false;
		line->body.line.target= true_target;
		if (line->body.line.command_template == NULL) {
			line->body.line.command_template= pat_list->command_template;
			/* Also make sure the rule is build with $* and $< bound */
			/* $* is bound to the stuff that matched the "%" */
			line->body.line.star=
				getname(true_target->string + prefix_length,
					(int)(true_target->hash.length - prefix_length - suffix_length));
			line->body.line.less= source;
			line->body.line.percent= NULL;
		};
		return(build_ok);
	};
	/* This return is taken if no percent rule was found for the target */
	return(build_dont_know);
}

/*
 *	read_directory() reads the directory the specified file lives in.
 */
static void
read_directory_of_file(file)
	register Name		file;
{
	register Name		directory= cached_names.dot;
	register char		*p= rindex(file->string, SLASH);
	register int		length= p - file->string;

	/* If the filename contains a "/" we have to extract the path */
	/* Else the path defaults to "." */
	if (p != NULL) {
		/* Check a few popular directories first to possibly save time */
		/* Compare string length first to gain speed */
		if ((cached_names.usr_include->hash.length == length) &&
		    is_equaln(cached_names.usr_include->string, file->string, length))
			directory= cached_names.usr_include;
		else if ((cached_names.usr_include_sys->hash.length == length) &&
			 is_equaln(cached_names.usr_include_sys->string, file->string, length))
			directory= cached_names.usr_include_sys;
		else
			directory= getname(file->string, length);
	};
	read_dir(directory, (char *)NULL, (Property)NULL, (char *)NULL);
}

/*
 *	doname_check() will call doname() and then inspect the return value
 */
Doname
doname_check(target, do_get, implicit, automatic)
	register Name		target;
	register Boolean	do_get;
	register Boolean	implicit;
	register Boolean	automatic;
{
	(void)fflush(stdout);
	switch (doname(target, do_get, implicit)) {
	    case build_ok:
		return(build_ok);
	    case build_failed:
		if (is_false(flag.interactive) && is_false(flag.continue_after_error))
			fatal("Target `%s' not remade because of errors",
			      target->string);
		return(build_failed);
	    case build_dont_know:
		if (is_true(flag.interactive)) {
			(void)printf("Don't know how to make `%s'\n",
				     target->string);
			return(build_failed);
		};
		/* If we cant figure out how to build an automatic (hidden) */
		/* dependency we just ignore it. We later declare the target */
		/* to be out of date just in case something changed */
		if (is_true(automatic))
			return(build_dont_know);
		fatal("Don't know how to make target `%s'", target->string);
		break;
	};
#ifdef lint
	return(build_failed);
#endif
}

/*
 *	build_suffix_list() scans the .SUFFIXES list and figures out
 *	which suffixes this target can be derived from.
 *	The target itself is not know here, we just know the suffix of the target.
 *	For each suffix on the list the target can be derived iff
 *	a rule exists for the name "<suffix-on-list><target-suffix>".
 *	A list of all possible building suffixes is built, with the rule for each,
 *	and tacked to the target suffix nameblock.
 */
void
build_suffix_list(target_suffix)
	register Name		target_suffix;
{
	register Dependency	source_suffix;
	char			rule_name[MAXPATHLEN];
	register Property	line;
	register Property	suffix;
	Name			rule;

	/* If this is before default.mk has been read we just return to try */
	/* again later */
	if ((suffixes == NULL) || is_false(working_on_targets))
		return;
	if (debug_level > 1)
		(void)printf("%*sbuild_suffix_list(%s) ",
			     BLANKS, target_suffix->string);
	/* Mark the target suffix saying we cashed its list */
	target_suffix->has_read_suffixes= true;
	/* Scan the .SUFFIXES list */
	for (source_suffix= suffixes;
	     source_suffix != NULL;
	     source_suffix= source_suffix->next) {
		/* Build the name "<suffix-on-list><target-suffix>" */
		/* (a popular one would be ".c.o") */
		(void)strncpy(rule_name,
			      source_suffix->name->string,
			      (int)source_suffix->name->hash.length);
		(void)strncpy(rule_name + source_suffix->name->hash.length,
			      target_suffix->string,
			      (int)target_suffix->hash.length);
		/* Check if that name has a rule, if not it cannot match any */
		/* implicit rule scan and is ignored */
		/* The getname() call only check for presence, it will not */
		/* enter the name if it is not defined */
		if (((rule= getname_fn(rule_name,
				       (int)(source_suffix->name->hash.length +
						target_suffix->hash.length),
					DONT_ENTER)) != NULL) &&
		    ((line= get_prop(rule->prop, line_prop)) != NULL)) {
			if (debug_level > 1)
				(void)printf("%s ", rule->string);
			/* This makes it possible to quickly determine it it */
			/* will pay to look for a suffix property */
			target_suffix->has_suffixes= true;
			/* Add the suffix property to the target suffix and */
			/* save the rule with it */
			/* All information the implicit rule scanner need is */
			/* save in the suffix property */
			suffix= append_prop(target_suffix, suffix_prop);
			suffix->body.suffix.suffix= source_suffix->name;
			suffix->body.suffix.command_template=
				line->body.line.command_template;
		};
	};
	if (debug_level > 1)
		(void)printf("\n");
}

/*
 *	find_suffix_rule() does the lookup for single and double suffix rules.
 *	It calls build_suffix_list() to build the list of possible suffixes
 *	for the given target.
 *	It then scans the list to find the first possible source file that exists.
 *	This is done by concatenating the body of the target name (target name less target
 *	suffix) and the source suffix and checking if the resulting file exists.
 */
static Doname
find_suffix_rule(target, target_body, target_suffix, command)
	Name			target;
	Name			target_body;
	Name			target_suffix;
	Property		*command;
{
	register Name		source;
	register Property	source_suffix;
	char			sourcename[MAXPATHLEN];
	register char		*put_suffix;
	register Property	line;
	Name			true_target= target;

	/* If the target is a constructed one for a "::" target we need to */
	/* consider that */
	if (target->has_target_prop)
		true_target= get_prop(target->prop, target_prop)->body.target.target;
	if (debug_level > 1)
		(void)printf("%*sfind_suffix_rule(%s,%s,%s)\n",
			     BLANKS,
			     true_target->string,
			     target_body->string,
			     target_suffix->string);
	if ((true_target->suffix_scan_done == true) && (*command == NULL))
		return(build_ok);
	true_target->suffix_scan_done= true;
	/* Enter all names from the directory where the target lives as files */
	/* in makes sense */
	/* This will make finding the synthesized source possible */
	read_directory_of_file(target_body);
	/* Cache the suffixes for this target suffix if not done */
	if (is_false(target_suffix->has_read_suffixes))
		build_suffix_list(target_suffix);
	/* Preload the sourcename vector with the head of the target name */
	(void)strncpy(sourcename,
		      target_body->string,
		      (int)target_body->hash.length);
	put_suffix= sourcename + target_body->hash.length;
	/* Scan the suffix list for the target if one exists */
	if (is_true(target_suffix->has_suffixes))
		for (source_suffix= get_prop(target_suffix->prop, suffix_prop);
		     source_suffix != NULL;
		     source_suffix= get_prop(source_suffix->next, suffix_prop)) {
			/* Build the synthesized source name */
			(void)strncpy(put_suffix,
				      source_suffix->body.suffix.suffix->string,
				      (int)source_suffix->body.suffix.suffix->hash.length);
			put_suffix[source_suffix->body.suffix.suffix->hash.length]= NUL;
			if (debug_level > 1)
				(void)printf("%*sTrying %s\n",
					     BLANKS, sourcename);
			source= getname(sourcename, FIND_LENGTH);
			/* If the synth source file is not registered as a file */
			/* this source suffix did not match */
			if (is_false(source->stat.is_file))
				continue;
			/* The synth source is a file. Make sure it is up to date */
			switch (doname(source,
				       source_suffix->body.suffix.suffix->with_squiggle,
				       true)) {
			    case build_dont_know:
				/* If we still cant build the synth source this */
				/* rule is not a match, try the next one */
				if (source->stat.time == FILE_DOESNT_EXIST)
					continue;
			    case build_ok:
				break;
			    case build_failed:
				return(build_failed);
			};
			if (debug_level > 1)
				(void)printf("%*sFound %s\n", BLANKS, sourcename);
			if (is_true(source->depends_on_conditional))
				target->depends_on_conditional= true;
/* Since it is possible that the same target is built several times during the make run */
/* we have to patch the target with all information we found here */
/* Thus the target will have an explicit rule the next time around */
/* Enter the synthesized source as a dependency for the target in case the target is built again */
			enter_dependency(line= maybe_append_prop(target, line_prop),
					 source,
					 false);
			if (*command == NULL)
				*command= line;
			if ((source->stat.time > (*command)->body.line.dependency_time) &&
			    (debug_level > 1))
				(void)printf("%*sDate(%s)=%s Date-dependencies(%s)=%s\n",
					     BLANKS,
					     source->string,
					     time_to_string(source->stat.time),
					     true_target->string,
					     time_to_string((*command)->body.line.dependency_time));
			/* Determine if this new dependency made the target out of date */
			(*command)->body.line.dependency_time=
				max((*command)->body.line.dependency_time,
				    source->stat.time);
			if (out_of_date(target->stat.time,
					(*command)->body.line.dependency_time)) {
			    line->body.line.is_out_of_date= true;
			    if (debug_level > 0)
				(void)printf("%*sBuilding %s using suffix rule for %s%s because it is out of date relative to %s\n",
					     BLANKS,
					     true_target->string,
					     source_suffix->body.suffix.suffix->string,
					     target_suffix->string, source->string);
			};
			/* Add the implicit rule as the targets explicit rule */
			line->body.line.sccs_command= false;
			if (line->body.line.command_template == NULL)
				line->body.line.command_template=
					source_suffix->body.suffix.command_template;
			line->body.line.target= true_target;
			/* Also make sure the rule is build with $* and $< bound properly */
			line->body.line.star= target_body;
			line->body.line.less= source;
			line->body.line.percent= NULL;
			return(build_ok);
		};
	/* Return here in case no rule matched the target */
	return(build_dont_know);
}

/*
 *	find_ar_suffix_rule() scans the .SUFFIXES list and tries
 *	to find a suffix on it that matches the tail of the target member name.
 *	If it finds a matching suffix it calls find_suffix_rule() to find
 *	a rule for the target using the suffix ".a".
 */
static Doname
find_ar_suffix_rule(target, true_target, command)
	register Name		target;
	Name			true_target;
	Property		*command;
{
	register Dependency	suffix;
	register int		suffix_length;
	register int		target_end;
	register char		*cap;
	register char		*cbp;
	Property		line;
	Name			body;

	/* We compare the tail of the target name with the suffixes from .SUFFIXES */
	target_end= (int)true_target->string + true_target->hash.length;
	if (debug_level > 1)
		(void)printf("%*sfind_ar_suffix_rule(%s)\n",
			     BLANKS, true_target->string);
	/* Scan the .SUFFIXES list to see if the target matches any of those suffixes */
	for (suffix= suffixes; suffix != NULL; suffix= suffix->next) {
		/* Compare one suffix */
		suffix_length= suffix->name->hash.length;
		compare_string(suffix->name->string,
			       (char *)(target_end - suffix_length),
			       suffix_length,
			       not_this_one,
			       cap, cbp);
		/* The target tail matched a suffix from the .SUFFIXES list. */
		/* Now check for a rule to match. */
		if (find_suffix_rule(target,
				     body= getname(true_target->string,
					     (int)(true_target->hash.length -
						suffix_length)),
					     cached_names.dot_a,
					     command) == build_ok) {
			line= get_prop(target->prop, line_prop);
			line->body.line.star= body;
			line->body.line.query= alloc(Chain);
			line->body.line.query->next= NULL;
			line->body.line.query->name= line->body.line.less;
			return(build_ok);
		};
		/* If no rule is found we try the next suffix to see if it */
		/* matched the target tail. And so on. */
		/* Go here if the suffix did not match the target tail */
not_this_one:	;			 
	};
	return(build_dont_know);
}

/*
 *	find_double_suffix_rule() scans the .SUFFIXES list and tries
 *	to find a suffix on it that matches the tail of the target name.
 *	If it finds a matching suffix it calls find_suffix_rule() to find
 *	a rule for the target.
 */
static Doname
find_double_suffix_rule(target, command)
	register Name		target;
	Property		*command;
{
	register Dependency	suffix;
	register int		suffix_length;
	register int		target_end;
	Name			true_target= target;
	register char		*cap;
	register char		*cbp;

	/* If the target is a constructed one for a "::" target we need to */
	/* consider that */
	if (target->has_target_prop)
		true_target= get_prop(target->prop, target_prop)->body.target.target;
	/* We compare the tail of the target name with the suffixes from .SUFFIXES */
	target_end= (int)true_target->string + true_target->hash.length;
	if (debug_level > 1)
		(void)printf("%*sfind_double_suffix_rule(%s)\n",
			     BLANKS, true_target->string);
	/* Scan the .SUFFIXES list to see if the target matches any of those suffixes */
	for (suffix= suffixes; suffix != NULL; suffix= suffix->next) {
		/* Compare one suffix */
		suffix_length= suffix->name->hash.length;
		compare_string(suffix->name->string,
			       (char *)(target_end - suffix_length),
			       suffix_length,
			       not_this_one,
			       cap, cbp);
		/* The target tail matched a suffix from the .SUFFIXES list. */
		/* Now check for a rule to match. */
		if (find_suffix_rule(target,
				     getname(true_target->string,
					     (int)(true_target->hash.length -
						suffix_length)),
					     suffix->name,
					     command) == build_ok)
			return(build_ok);
		/* If no rule is found we try the next suffix to see if it */
		/* matched the target tail. And so on. */
		/* Go here if the suffix did not match the target tail */
not_this_one:	;			 
	};
	return(build_dont_know);
}

/*
 *	build_command_strings() builds the command string to used when
 *	building a target. If the string is different from the previous one
 *	is_out_of_date is set.
 */
static void
build_command_strings(target, line)
	Name			target;
	register Property	line;
{
	String			command_line;
	register Cmd_line	template= line->body.line.command_template;
	register Cmd_line	*insert= &line->body.line.command_used;
	register Cmd_line	used= *insert;
	char			buffer[STRING_BUFFER_LENGTH];
	char			*start;
	Name			new;
	register Boolean	new_command_longer= false;
	register Boolean	ignore_all_command_dependency= true;
	Property		member;

	/* We have to check if a target depends on conditional macros */
	/* Targets that do must be reprocessed by doname() each time around */
	/* since the macro values used when building the target might have */
	/* changed */
	is_conditional= false;
	/* If we are building a lib.a(member) target $@ should be bound to lib.a */
	if ((target->member_class != not_member) &&
	    ((member= get_prop(target->prop, member_prop)) != NULL))
		target= member->body.member.library;
	/* If we are building a "::" help target $@ should be bound to the real */
	/* target name */
	/* A lib.a(member) target is never :: */
	if (target->has_target_prop)
		target= get_prop(target->prop, target_prop)->body.target.target;
	/* Bind the magic macros that make supplies */
	(void)setvar(cached_names.c_at, target, false);
	(void)setvar(cached_names.star, line->body.line.star, false);
	(void)setvar(cached_names.less, line->body.line.less, false);
	(void)setvar(cached_names.percent, line->body.line.percent, false);
	/* $? is seldom used and it is expensive to build */
	/* so we store the list form and build the string on demand */
	(void)setvar_daemon(cached_names.query,
			    (Name)line->body.line.query,
			    false,
			    chain_daemon);

/* We have two command sequences we need to handle */
/* The old one that we probably read from .make.state */
/* and the new one we are building that will replace the old one */
/* Even when KEEP_STATE is not on we build a new command sequence and store it */
/*  in the line prop. This command sequence is then executed by run_command() */
/* If KEEP_STATE is on it is also later written to .make.state */
/* The routine replaces the old command line by line with the new one trying to reuse Cmd_lines */

	/* If there is no old command_used we have to start creating Cmd_lines */
	/* to keep the new cmd in */
	if (used == NULL) {
		new_command_longer= true;
		*insert= used= alloc(Cmd_line);
		used->next= NULL;
		used->command_line= NULL;
		insert= &used->next;
	};
	/* Run thru the template for the new command and build the expanded new */
	/* command lines */
	for (;
	     template != NULL;
	     template= template->next, insert= &used->next, used= *insert) {
		/* If there is no old command_used Cmd_line we need to create */
		/* one and say that cmd consistency failed */
		if (used == NULL) {
			new_command_longer= true;
			*insert= used= alloc(Cmd_line);
			used->next= NULL;
			used->command_line= cached_names.empty_suffix_name;
		};
		/* Prepare the Cmd_line for the processing */
		/* The command line prefixes "@-=?" are stripped and that */
		/* information is saved in the Cmd_line */
		used->assign= false;
		used->ignore_error= flag.ignore_errors;
		used->silent= flag.silent;
		/* Expand the macros in the command line */
		init_string_from_stack(command_line, buffer);
		make_word_mentioned= false;
		query_mentioned= false;
		expand_value(template->command_line, &command_line);
		/* If the macro $(MAKE) is mentioned in the command "make -n" */
		/* runs actually execute the command */
		used->make_refd= make_word_mentioned;
		used->ignore_command_dependency= query_mentioned;
		/* Strip the prefixes */
		start= command_line.buffer.start;
		for (;
		     isspace(*start) || (funny[*start] & COMMAND_PREFIX_FUNNY);
		     start++)
			switch (*start) {
			    case QUESTION:
				used->ignore_command_dependency= true;
				break;
			    case EXCLAM:
				used->ignore_command_dependency= false;
				break;
			    case EQUAL:
				used->assign= true;
				break;
			    case HYPHEN:
				used->ignore_error= true;
				break;
			    case AT:
				if (is_false(flag.do_not_exec_rule))
					used->silent= true;
				break;
			};
		/* If all command lines of the template are prefixed with "?" */
		/* the VIRTUAL_ROOT is not used for cmd consistency checks */
		if (is_false(used->ignore_command_dependency))
			ignore_all_command_dependency= false;
		/* Internalize the expanded and stripped command line */
		new= getname(start, FIND_LENGTH);
		/* Compare it with the old one for command consistency */
		if (used->command_line != new) {
		    if (is_true(flag.keep_state) &&
			is_false(used->ignore_command_dependency)) {
			    if (debug_level > 0)
				if (used->command_line != NULL
				    && *used->command_line->string != '\0')
					(void)printf("%*sBuilding %s because new command \n\t%s\n%*sdifferent from old\n\t%s\n",
						     BLANKS,
						     target->string,
						     new->string,
						     BLANKS,
						     used->command_line->string);
				else
					(void)printf("%*sBuilding %s because new command \n\t%s\n%*sdifferent from empty old command\n",
						     BLANKS,
						     target->string,
						     new->string,
						     BLANKS);
			    command_changed= line->body.line.is_out_of_date= true;
		    };
		    used->command_line= new;
		};
		if (is_true(command_line.free_after_use))
			Free(command_line.buffer.start);
	};
	/* Check if the old command is longer than the new for command consistency */
	if (used != NULL) {
		*insert= NULL;
		if (is_true(flag.keep_state) &&
		    is_false(ignore_all_command_dependency)) {
			if (debug_level > 0)
				(void)printf("%*sBuilding %s because new command shorter than old\n",
					     BLANKS, target->string);
			command_changed= line->body.line.is_out_of_date= true;
		};
	};
	/* Check if the new command is longer than the old command for command */
	/* consistency */
	if (is_true(new_command_longer) &&
	    is_false(ignore_all_command_dependency) &&
	    is_true(flag.keep_state)) {
		if (debug_level > 0)
			(void)printf("%*sBuilding %s because new command longer than old\n",
				     BLANKS, target->string);
		command_changed= line->body.line.is_out_of_date= true;
	};
	/* Unbind the magic macros */
	(void)setvar(cached_names.c_at, (Name) NULL, false);
	(void)setvar(cached_names.star, (Name) NULL, false);
	(void)setvar(cached_names.less, (Name) NULL, false);
	(void)setvar(cached_names.percent, (Name) NULL, false);
	(void)setvar(cached_names.query, (Name) NULL, false);
	if (is_true(is_conditional))
		target->depends_on_conditional= true;
}

/*
 *	do_assign() handles runtime assignments
 *	For command lines prefixed with "="
 */
static void
do_assign(line, target)
	register Name		line;
	register Name		target;
{
	register char		*equal;
	register char		*string= line->string;
	register Boolean	append= false;
	register Name		name;

	/* If any runtime assignments are done doname() must reprocess all */
	/* targets in the future */
	/* since the macro values used to build the command lines for the */
	/* targets might have changed */
	assign_done= true;
	/* Skip white space */
	while (isspace(*string))
		string++;
	equal= string;
	/* Find "+=" or "=" */
	while (!isspace(*equal) && (*equal != PLUS) && (*equal != EQUAL))
		equal++;
	/* Internalize macro name */
	name= getname(string, equal - string);
	/* Skip over "+=" "=" */
	while (!((*equal == NUL) || (*equal == EQUAL) || (*equal == PLUS)))
		equal++;
	switch (*equal) {
	    case NUL:
		fatal("= expected in rule `%s' for target `%s'",
		      line->string,
		      target->string);
	    case PLUS:
		append= true;
		equal++;
		break;
	};
	equal++;
	/* Skip over whitespace in front of value */
	while (isspace(*equal))
		equal++;
	/* Enter new macro value */
	enter_equal(name,
		    getname(equal, line->string + line->hash.length - equal),
		    append);
}

/*
 *	vpath_translation() translates one command line by
 *	checking each word. If the word has an alias it is translated.
 */
static Name
vpath_translation(cmd)
	register Name		cmd;
{
	char			buffer[STRING_BUFFER_LENGTH];
	String			new_cmd;
	char			*p;
	char			*start;

	if ((cmd == NULL) || (cmd->hash.length == 0))
		return(cmd);
	init_string_from_stack(new_cmd, buffer);
	p= cmd->string;
	while (*p != NUL) {
		while (isspace(*p) && (*p != NUL))
			append_char(*p++, &new_cmd);
		start= p;
		while (!isspace(*p) && (*p != NUL))
			p++;
		cmd= getname(start, p - start);
		if (is_true(cmd->has_vpath_alias_prop)) {
			cmd= get_prop(cmd->prop, vpath_alias_prop)->
						body.vpath_alias.alias;
			append_string(cmd->string, &new_cmd, (int)cmd->hash.length);
		} else
			append_string(start, &new_cmd, p - start);
	};
	cmd= getname(new_cmd.buffer.start, FIND_LENGTH);
	if (is_true(new_cmd.free_after_use))
		Free(new_cmd.buffer.start);
	return(cmd);
}

/*
 *	run_command() Takes one Cmd_line and runs the commands from it.
 */
static Boolean
run_command(line)
	register Property	line;
{
	register Cmd_line	rule;
	register Boolean	result= succeeded;
	register Boolean	remember_only= false;
	register Chain		target_group;
	register Name		target= line->body.line.target;
	String			touch_string;
	char			buffer[MAXPATHLEN];
	char			string[MAXPATHLEN];
	Name			touch;
	Name			name;
	Property		line2;

	/* Build the command if we know the target is out of date or if we want */
	/* to check cmd consistency */
	if (is_true(line->body.line.is_out_of_date) || is_true(flag.keep_state))
		build_command_strings(target, line);
	/* Never mind */
	if (is_false(line->body.line.is_out_of_date))
		return(succeeded);
	/* If quest is on we just need to know that something is out of date */
	if (is_true(flag.quest))
		exit(1);
	/* We actually had to do something this time */
	rewrite_statefile= commands_done= true;
/* If this is a sccs command we have do do some extra checking and possibly complain */
/* If the file cannt be gotten because it is checked out we complain and then */
/* behave as if the command was executed even when we ignore the command */
	if (is_false(flag.touch) &&
	    is_true(line->body.line.sccs_command) &&
	    (target->stat.time != FILE_DOESNT_EXIST) &&
	    ((target->stat.mode & 0222) != 0)) {
		warning("%s is writable so it cannot be sccs gotten",
			     target->string);
		target->has_complained= remember_only= true;
	};
/* If KEEP_STATE is on we make sure we have the timestamp for .make.state. */
/* If .make.state changes during the command run we reread .make.state after the command */
/* We also setup the environment variable that asks utilities to report dependencies */
	if (is_false(flag.touch) &&
	    is_true(flag.keep_state) &&
	    is_false(remember_only)) {
		(void)exists(cached_names.make_state);
		(void)sprintf(string,
			      "/tmp/make.dependency.%d.%d %s",
			      getpid(),
			      file_number++,
			      target->string);
		(void)setvar(cached_names.sunpro_dependencies,
			     getname(string, FIND_LENGTH),
			     false);
		*index(string, SPACE)= NUL;
		temp_file_name= getname_fn(string, FIND_LENGTH, FILE_TYPE);
	} else
		temp_file_name= NULL;

	/* In case we are interrupted we need to know what was going on */
	current_target= target;
	/* We also need to be able to save an empty command instead of the */
	/* interrupted one in .make.state */
	current_line= line;
	/* If this is an sccs get command we need to ignore we skip all the */
	/* command execution stuff */
	if (is_true(remember_only))
		goto do_not_run_command;
	/* If this is an "make -t" run we do this */
	if (is_true(flag.touch)) {
	    /* We touch all targets in the target group ("foo + fie:"), if any */
	    for (name= target, target_group= NULL; name != NULL;) {
		if (name->member_class == not_member) {
			/* Build a touch command that can be passed to dosys() */
			/* If KEEP_STATE is on "make -t" will */
			/* save the proper command, not the */
			/* "touch" in .make.state */
			init_string_from_stack(touch_string, buffer);
			append_string("touch ", &touch_string, FIND_LENGTH);
			touch= name;
			if (name->has_vpath_alias_prop)
				touch= get_prop(name->prop, vpath_alias_prop)->
							body.vpath_alias.alias;
			append_string(touch->string, &touch_string, FIND_LENGTH);
			touch= getname(touch_string.buffer.start, FIND_LENGTH);
			if (is_true(touch_string.free_after_use))
				Free(touch_string.buffer.start);
			if (is_false(flag.silent) ||
			    is_true(flag.do_not_exec_rule) && (target_group == NULL))
				(void)printf("%s\n", touch->string);
			/* Run the touch command, or simulate it */
			if (is_false(flag.do_not_exec_rule))
				result= dosys(touch, false, false, false);
			else
				result= succeeded;
		    } else
			result= succeeded;
		    if (target_group == NULL)
			target_group= line->body.line.target_group;
		    else
			target_group= target_group->next;
		    if (target_group != NULL)
			name= target_group->name;
		    else
			name= NULL;
		};
	    goto do_not_run_command;
	};
	/* If this is not a touch run we need to execute the */
	/* proper command for the target */
	/* Run thru all the command line for the target and */
	/* execute then one by one */
	for (rule= line->body.line.command_used;
	     rule != NULL;
	     rule= rule->next) {
		if (is_true(vpath_defined))
			rule->command_line= vpath_translation(rule->command_line);
		/* Echo command line, maybe */
		if ((rule->command_line->hash.length > 0) &&
		    is_false(flag.silent) &&
		    (is_false(rule->silent) || is_true(flag.do_not_exec_rule))) {
			(void)printf("%s\n", rule->command_line->string);
		};
		if (rule->command_line->hash.length > 0) {
			/* Do assignment if command line was prefixed with "=" */
			if (is_true(rule->assign)) {
				result= succeeded;
				do_assign(rule->command_line, target);
			} else {
				/* Execute command line */
				result= dosys(rule->command_line,
					      rule->ignore_error,
					      rule->make_refd,
					      boolean(is_true(rule->silent) &&
						      is_true(rule->ignore_error)));
				if (result == succeeded) {
					/* Then read the temp file that now might */
					/* contain dependency reports from utilities */
					read_dependency_file(temp_file_name);
					/* And reread .make.state if it */
					/* changed (the command ran recursive makes) */
					check_read_state_file();
				};
				if (temp_file_name != NULL)
					(void)unlink(temp_file_name->string);
			};
		} else
			result= succeeded;
		if (result == failed) {
		    if (is_true(flag.silent))
			(void)printf("The following command caused the error:\n%s\n",
				     rule->command_line->string);
		    if (is_false(rule->ignore_error)) {
			if (is_false(flag.continue_after_error))
				fatal("Command failed for target `%s'", target->string);
			/* Make sure a failing command is not save in .make.state */
			line->body.line.command_used= NULL;
			break;
		    } else
			result= succeeded;
		};
	    };
do_not_run_command:
	temp_file_name= NULL;
	if ((result == succeeded) && (line->body.line.command_used != NULL)) {
		if (is_true(flag.do_not_exec_rule) || is_true(flag.touch))
			/* If we are simulating execution we need to fake a new */
			/* timestamp for the target we didnt build */
			target->stat.time= FILE_MAX_TIME;
		else {
			/* If we really built the target we read the new timestamp */
			target->stat.time= FILE_NO_TIME;
			(void)exists(target);
		};
		/* If the target is part of a group we need to propagate the */
		/* result of the run to all members */
		for (target_group= line->body.line.target_group;
		     target_group != NULL;
		     target_group= target_group->next) {
			target_group->name->stat.time= target->stat.time;
			line2= maybe_append_prop(target_group->name, line_prop);
			line2->body.line.command_used= line->body.line.command_used;
			line2->body.line.target= target_group->name;
		};
	};
	current_target= NULL;
	current_line= NULL;
	target->has_built= true;
	return(result);
}

/*
 *	sccs_get figures out if it possible to sccs get a file
 *	and builds the command to do it if it is.
 */
static Doname
sccs_get(target, command)
	register Name		target;
	register Property	*command;
{
	register Timetype	sccs_time;
	register Property	line;
	char			link[MAXPATHLEN];
	char			name[MAXPATHLEN];
	String			string;
	register int		result;
	register char		*p;
	Name			orig_target= target;

	/* For sccs we need to chase symlinks */
	while (is_true(target->stat.is_sym_link)) {
		/* Read the value of the link */
		result= readlink_vroot(target->string, link, sizeof(link), NULL, VROOT_DEFAULT);
		if (result == -1)
			fatal("Can't read symbolic link `%s': %s",
				target->string, errmsg(errno));
		/* Use the value to build the proper filename */
		init_string_from_stack(string, name);
		if ((link[0] != SLASH) &&
		    ((p= rindex(target->string, SLASH)) != NULL))
			append_string(target->string, &string, p - target->string + 1);
		append_string(link, &string, result);
		/* Replace the old name with the translated name */
		(void)exists(target= getname(string.buffer.start, FIND_LENGTH));
		if (is_true(string.free_after_use))
			Free(string.buffer.start);
	};

	/* read_dir() also reads the ?/SCCS dir and saves information about */
	/* which files have SCSC/s. files */
	if (target->stat.has_sccs == dont_know_sccs)
		read_directory_of_file(target);
	switch (target->stat.has_sccs) {
	    case dont_know_sccs:
		/* We dont know by now there is no SCCS/s.* */
		target->stat.has_sccs= no_sccs;
	    case no_sccs:
		/* If there is no SCCS/s.* but the plain file exists we say */
		/* thing are OK */
		if (target->stat.time > FILE_DOESNT_EXIST)
			return(build_ok);
		/* If we cant find the plain file we give up */
		return(build_dont_know);
	    case has_sccs:
		/* Pay dirt. We now need to figure out if the plain file is out */
		/* of date relative to the SCCS/s.* file */
		sccs_time= exists(get_prop(target->prop, sccs_prop)->body.sccs.file);
		break;
	};
	if (is_false(target->has_complained) &&
	    (sccs_time != FILE_DOESNT_EXIST) &&
	    (sccs_get_rule != NULL)) {
		/* We provide a command line for the target. The line is a */
		/* "sccs get" command from default.mk */
		line= maybe_append_prop(target, line_prop);
		*command= line;
		if (sccs_time > target->stat.time) {
			/* And only if the plain file is out of date do we */
			/* request execution of the command */
			line->body.line.is_out_of_date= true;
			if (debug_level > 0)
				(void)printf("%*sSccs getting %s because s. file is younger than source file\n",
					      BLANKS, target->string);
		};
		line->body.line.sccs_command= true;
		line->body.line.command_template= sccs_get_rule;
		line->body.line.target= orig_target;
		/* Also make sure the rule is build with $* and $< bound properly */
		line->body.line.star= NULL;
		line->body.line.less= NULL;
		line->body.line.percent= NULL;
		/* If we chased any symlinks we also need to make sure $@ is */
		/* later bound OK */
		if (orig_target != target) {
			maybe_append_prop(orig_target,
					  target_prop)->body.target.target= target;
			orig_target->has_target_prop= true;
		};
		return(build_ok);
	};
	return(build_dont_know);
}

/*
 *	set_locals() sets any conditional macros for the target.
 *	Each target carries a possibly empty set of conditional properties.
 */
static void
set_locals(target, old_locals)
	register Name		target;
	register Property	old_locals;
{
	register Property	conditional;
	register int		i;
	register Boolean	saved_is_conditional= is_conditional;

	/* Scan the list of conditional properties and apply each one */
	for (conditional= get_prop(target->prop, conditional_prop), i= 0;
	     conditional != NULL;
	     conditional= get_prop(conditional->next, conditional_prop), i++) {
		/* Save the old value */
		old_locals[i].body.macro=
			maybe_append_prop(conditional->body.conditional.name,
					  macro_prop)->body.macro;
		if (debug_level > 1)
			(void)printf("%*sActivating conditional value: ",
				     BLANKS);
		/* Set the conditional value. Macros are expanded when the */
		/* macro is refd as usual */
		(void)setvar(conditional->body.conditional.name,
			     conditional->body.conditional.value,
			     conditional->body.conditional.append);
	};
	is_conditional= saved_is_conditional;
}

/*
 *	reset_locals() removes any conditional macros for the target.
 */
static void
reset_locals(target, old_locals, conditional, index)
	register Name		target;
	register Property	old_locals;
	register Property	conditional;
	register int		index;
{
	register Property	this_conditional;

	/* Scan the list of conditional properties and restore the old value to each one */
	/* Reverse the order relative to when we assigned macros */
	this_conditional= get_prop(conditional->next, conditional_prop);
	if (this_conditional != NULL)
		reset_locals(target, old_locals, this_conditional, index+1);
	get_prop(conditional->body.conditional.name->prop,
			macro_prop)->body.macro=
				old_locals[index].body.macro;
	if (debug_level > 1) {
	    if (old_locals[index].body.macro.value != NULL)
		(void)printf("%*sdeactivating conditional value: %s= %s\n",
			     BLANKS,
			     conditional->body.conditional.name->string,
			     old_locals[index].body.macro.value->string);
	    else
		(void)printf("%*sdeactivating conditional value: %s =\n",
			     BLANKS,
			     conditional->body.conditional.name->string);
	};
}

/*
 *	create_sym_link() creates a symbolic link from the target to the specified file
 */
static void
create_sym_link(target)
	register Name		target;
{
	struct Property		prop;
	register Property	line;

	if (target->stat.time > FILE_DOESNT_EXIST)
		fatal("File `%s' is supposed to be a symbolic link to `%s'",
		      target->string,
		      get_prop(target->prop, sym_link_to_prop)->
					body.sym_link_to.link_to->string);
	/* Make sure the user didnt sneak a rule in for the target */
	line= get_prop(target->prop, line_prop);
	if (line->body.line.command_template != NULL) {
		warning("Rule for `%s: %s' line ignored",
				target->string, cached_names.sym_link_to->string);
		line->body.line.command_template= NULL;
		};
	/* Build a line property and stuff it with an "ln -s" command from default.mk */
	prop.next= NULL;
	prop.type= line_prop;
	prop.body.line.is_out_of_date= true;
	prop.body.line.sccs_command= false;
	prop.body.line.command_template_redefined= false;
	prop.body.line.command_template= sym_link_to_rule;
	prop.body.line.command_used= NULL;
	prop.body.line.dependencies= NULL;
	prop.body.line.target_group= NULL;
	prop.body.line.target= target;
	/* Also make sure the rule is build with $* and $< bound properly */
	prop.body.line.star= NULL;
	prop.body.line.less=
		get_prop(target->prop, sym_link_to_prop)->body.sym_link_to.link_to;
	prop.body.line.percent= NULL;
	prop.body.line.query= NULL;
	/* Execute the "ln -s" command */
	(void)run_command(&prop);
	/* Get the new timestamp for the file pointed to */
	(void)exists(target);
}

/*
 *	doname() chases all files the target depends on and builds any that are out
 *	of date. If the target is out of date it is then rebuilt.
 */
Doname
doname(target, do_get, implicit)
	register Name		target;
	register Boolean	do_get;
	register Boolean	implicit;
{
	register Doname		result= build_dont_know;
	Chain			out_of_date_list= NULL;
	register Chain		*out_of_date_tail= &out_of_date_list;
	Property		old_locals;
	register Property	line;
	Property		command= NULL;
	register Dependency	dependency;
	Name			less= NULL;
	Name			true_target= target;
	Name			*automatics;
	register int		auto_count;
	Boolean			rechecking_target= false;
	Boolean			saved_commands_done;

	/* If the target is a constructed one for a "::" target we need to */
	/* consider that */
	if (target->has_target_prop)
		true_target= get_prop(target->prop, target_prop)->body.target.target;
	if (target->has_target_prop)
		/* Make sure we have a valid time for :: targets */
		true_target->stat.time= FILE_NO_TIME;
	(void)exists(true_target);
	/* If the target has been processed we dont need to do it again */
	/* Unless it depends on conditional macros or a delayed assignment */
	/* has been done when KEEP_STATE is on */
	if ((target->state == build_ok) &&
	    (is_false(flag.keep_state) ||
	     (is_false(target->depends_on_conditional) && is_false(assign_done))))
		return(build_ok);
	/* If KEEP_STATE is on we have to rebuild the target if the building of it */
	/* caused new automatic dependencies to be reported. This is where we */
	/* restart the build */
recheck_target:
	/* Init all local variables */
	result= build_dont_know;
	out_of_date_list= NULL;
	out_of_date_tail= &out_of_date_list;
	command= NULL;
	less= NULL;
	auto_count= 0;
	/* Save the set of automatic depes defined for this target */
	if (is_true(flag.keep_state) &&
	    ((line= get_prop(target->prop, line_prop)) != NULL) &&
	    (line->body.line.dependencies != NULL)) {
		Name *p;

		/* First run thru the dependency list to see how many autos there are */
		for (dependency= line->body.line.dependencies;
		     dependency != NULL;
		     dependency= dependency->next)
			if (is_true(dependency->automatic))
				auto_count++;
		/* Create vector to hold the current autos */
		automatics= (Name *) alloca(auto_count * sizeof(Name));
		/* Copy them */
		for (p= automatics, dependency= line->body.line.dependencies;
		     dependency != NULL;
		     dependency= dependency->next)
			if (is_true(dependency->automatic))
				*p++= dependency->name;
	};
	if (debug_level > 1)
		(void)printf("%*sdoname(%s)\n", BLANKS, true_target->string);
	recursion_level++;
	/* avoid infinite loops */
	if (target->state == build_in_progress) {
		warning("Infinite loop: Target `%s' depends on itself",
				target->string);
		return(build_ok);};
	target->state= build_in_progress;
	/* If the target should be a symbolic link we create it */
	if (is_true(true_target->stat.should_be_sym_link) &&
	    is_false(true_target->stat.is_sym_link))
		create_sym_link(true_target);

	/* Activate conditional macros for the target */
	if (target->conditional_cnt > 0) {
		old_locals= (Property) alloca(target->conditional_cnt *
							sizeof(struct Property));
		set_locals(target, old_locals);
	};
	if (is_false(target->has_depe_list_expanded))
		dynamic_dependencies(target);
/* If the target is a "lib.a(member)" form we add a dependency to "member" to it */
/* We also patch in a rule for "lib.a(member)" that is fetched from default.mk */
/* We also fudge the date of "member" if it does not exist. It is set to the date of "lib.a(member)" */
	if ((target->member_class == new_member) &&
	    is_false(target->has_member_depe) &&
	    (get_prop(target->prop, member_prop) != NULL)) {
		target->has_member_depe= true;
		/* Fool doname() to think this doesnt have to exist */
		target->colons= one_colon;
		line= maybe_append_prop(target, line_prop);
		line->body.line.target= target;
		if (line->body.line.command_template == NULL)
			line->body.line.command_template= ar_replace_rule;
		dependency= alloc(Dependency);
		dependency->next= line->body.line.dependencies;
		line->body.line.dependencies= dependency;
		line= get_prop(target->prop, member_prop);
		less= dependency->name= line->body.member.member;
		dependency->automatic= false;
		(void)exists(line->body.member.member);
		if (line->body.member.member->stat.time == FILE_DOESNT_EXIST)
			line->body.member.member->stat.time= target->stat.time;
		if (debug_level > 0)
			(void)printf("Adding dependency `%s: %s'\n",
			    target->string, line->body.member.member->string);
	};

/*
 *	FIRST SECTION -- GO THROUGH DEPENDENCIES AND COLLECT EXPLICIT
 *	COMMANDS TO RUN
 */
	if ((line= get_prop(target->prop, line_prop)) != NULL) {
		line->body.line.dependency_time= FILE_DOESNT_EXIST;
		line->body.line.query= NULL;
		line->body.line.is_out_of_date= false;
		/* Run thru all the dependencies and call doname() recursively on them */
		for (dependency= line->body.line.dependencies;
		     dependency != NULL;
		     dependency= dependency->next) {
			switch (doname_check(dependency->name,
					     do_get,
					     false,
					     dependency->automatic)) {
			    case build_failed:
				result= build_failed;
				break;
			    case build_dont_know:
/* If make cant figure out how to make a dependency maybe the dependency is out of date */
/* In this case we just declare the target out of date and go on */
/* If we really need the dependency the makeing of the target will fail */
/* This will only happen for automatic (hidden) dependencies */
				line->body.line.is_out_of_date= true;
				/* Make sure the dependency is not saved in the state file */
				dependency->stale= true;
				rewrite_statefile= command_changed= true;
				if (debug_level > 0)
					(void)printf("Target %s rebuilt because dependency %s does not exist\n",
						     true_target->string,
						     dependency->name->string);
				break;
			};
			if (is_true(dependency->name->depends_on_conditional))
				target->depends_on_conditional= true;
			/* Propagate new timestamp from "member" to "lib.a(member)" */
			(void)exists(dependency->name);
			if ((dependency->name->member_class == new_member) &&
			    (get_prop(dependency->name->prop, member_prop) != NULL))
				dependency->name->stat.time =
					get_prop(dependency->name->prop, member_prop)->
						body.member.member->stat.time;
			if ((dependency->name->stat.time > line->body.line.dependency_time) &&
			    (debug_level > 1))
				(void)printf("%*sDate(%s)=%s Date-dependencies(%s)=%s\n",
					     BLANKS,
					     dependency->name->string,
					     time_to_string(dependency->name->stat.time),
					     true_target->string,
					     time_to_string(line->body.line.dependency_time));
			/* Collect the timestamp of the youngest dependency */
			line->body.line.dependency_time=
				max(dependency->name->stat.time,
				    line->body.line.dependency_time);
			/* Build the $? list */
			(void)exists(true_target);
			if ((true_target->stat.time <= dependency->name->stat.time) &&
			    (dependency->name != cached_names.force) &&
			    (dependency->stale == false)) {
				*out_of_date_tail= (Chain) alloca(sizeof(struct Chain));
				if ((dependency->name->member_class != not_member) &&
				    (get_prop(dependency->name->prop, member_prop) != NULL))
					(*out_of_date_tail)->name =
						get_prop(dependency->name->prop, member_prop)->
							body.member.member;
				else
				    (*out_of_date_tail)->name= dependency->name;
				(*out_of_date_tail)->next= NULL;
				out_of_date_tail= &(*out_of_date_tail)->next;
				if (debug_level > 0)
					(void)printf("%*sBuilding %s because it is out of date relative to %s\n",
						 BLANKS, true_target->string, dependency->name->string);
			};
		};
		/* After scanning all the dependencies we check the rule if we found one */
		if (line->body.line.command_template != NULL) {
			if (is_true(line->body.line.command_template_redefined))
				warning("Too many rules defined for target %s",
					       target->string);
			command= line;
			/* Check if the target is out of date */
			if (out_of_date(true_target->stat.time,
					line->body.line.dependency_time))
				line->body.line.is_out_of_date= true;
			line->body.line.sccs_command= false;
			line->body.line.target= true_target;
			if (less != NULL)
				line->body.line.less= less;
			line->body.line.percent= NULL;
		};
		line->body.line.query= out_of_date_list;
	};

/* Do not try to find rule for target if target is :: type */
/* All actions will be taken by separate branches */
/* Else we try to find an implicit rule using various methods */
/* We quit doing that as soon as one is found */
	if (target->colon_splits == 0) {
		/* Look for percent matched rule */
		if (result == build_dont_know)
			if (find_percent_rule(target, &command) == build_failed)
				result= build_failed;

		/* Look for double suffix rule */
		if (result == build_dont_know) { Property member;
			if ((target->member_class == old_member) &&
			    ((member= get_prop(target->prop, member_prop)) != NULL)) {
				if (find_ar_suffix_rule(target, member->body.member.member, &command) == build_failed)
					result= build_failed;
				else {
					/* ALWAYS bind $% for old style ar rules */
					if (line == NULL)
						line= maybe_append_prop(target, line_prop);
					line->body.line.percent= member->body.member.member;
				};
			} else {
				if (find_double_suffix_rule(target, &command) == build_failed)
					result= build_failed;
			};
		};

		/* Look for single suffix rule */
		if ((result == build_dont_know) && is_false(implicit))
			if (find_suffix_rule(target,
					     target,
					     cached_names.empty_suffix_name,
					     &command) == build_failed)
				result= build_failed;

		/* Try to sccs get */
		if ((command == NULL) && (result == build_dont_know) && is_true(do_get))
			result= sccs_get(target, &command);

		/* Use .DEFAULT rule if it is defined. */
		if ((command == NULL) &&
		    (result == build_dont_know) &&
		    (true_target->colons == no_colon) &&
		    default_rule && is_false(implicit)) {
			/* Make sure we have a line prop */
			line= maybe_append_prop(target, line_prop);
			command= line;
			if (out_of_date(true_target->stat.time,
					line->body.line.dependency_time)) {
				line->body.line.is_out_of_date= true;
				if (debug_level > 0)
					(void)printf("%*sBuilding %s using .DEFAULT because it is out of date\n",
						 BLANKS, true_target->string);
			};
			line->body.line.sccs_command= false;
			line->body.line.command_template= default_rule;
			line->body.line.target= true_target;
			line->body.line.star= NULL;
			line->body.line.less= true_target;
			line->body.line.percent= NULL;
		};
	};

	/* We say "target up to date" if no commands were executed for the target */
	commands_done= false;
	/* Run commands if any. */
	if ((command != NULL) && (command->body.line.command_template != NULL)) {
		if (is_true(is_out_of_date)) {
			if (debug_level > 0)
				(void)printf("%*sBuilding %s because eveything is being rebuilt\n",
					      BLANKS, true_target->string);
			command->body.line.is_out_of_date= true;
		};
		if ((result == build_failed) || (run_command(command) == failed))
			result= build_failed;
		else {
			/* If all went OK set a nice timestamp and */
			/* return OK */
			if (true_target->stat.time == FILE_DOESNT_EXIST)
				true_target->stat.time= FILE_MAX_TIME;
			result= build_ok;
		};
	} else {
		/* If no command was found for the target and it doesnt exist */
		/* and it is mentioned as a target in the makefile we say it is extremely */
		/* new and that it is OK */
		if (target->colons != no_colon) {
			if (true_target->stat.time == FILE_DOESNT_EXIST)
				true_target->stat.time= FILE_MAX_TIME;
			result= build_ok;
		};
		/* If the file exists it is OK that we couldnt figure out how to build it */
		(void)exists(target);
		if ((target->stat.time != FILE_DOESNT_EXIST) && (result == build_dont_know))
			result= build_ok;
	};

	if ((line= get_prop(target->prop, line_prop)) != NULL)
		line->body.line.query= NULL;
	target->state= result;
	if (target->conditional_cnt > 0)
		reset_locals(target, old_locals, get_prop(target->prop, conditional_prop), 0);
	recursion_level--;
	if (target->member_class != not_member) { Property member;
		/* Propagate the timestamp from the member file to the member */
		if ((target->stat.time != FILE_MAX_TIME) &&
		    ((member= get_prop(target->prop, member_prop)) != NULL))
			target->stat.time= exists(member->body.member.member);
	};
	/* Check if we found any new auto dependencies when we built the target */
	if (is_true(flag.keep_state) && (result == build_ok)) {
	    Name *p;
	    int n;

	    /* Go thru new list of automatic depes */
	    if ((line= get_prop(target->prop, line_prop)) == NULL)
		return(result);
	    for (dependency= line->body.line.dependencies;
		 dependency != NULL;
		 dependency= dependency->next, p++) {
		/* And make sure that each one existed before we built the target */
		if (is_true(dependency->automatic)) {
			for (n= auto_count, p= automatics; n > 0; n--)
				if (*p++ == dependency->name)
					/* If we can find it on the */
					/* saved list of autos we are OK  */
					goto not_new;
			/* But if we scan over the old list of auto */
			/* without finding it it is new and we must check it */
			if (debug_level > 0)
				(void)printf("%*sTarget `%s' acquired new dependencies from build, checking those\n",
					 BLANKS, true_target->string);
			rechecking_target= true;
			saved_commands_done= commands_done;
			goto recheck_target;
		};
not_new:;
	    };
	};

	if (is_true(rechecking_target) && is_false(commands_done))
		commands_done= saved_commands_done;
	return(result);
}
