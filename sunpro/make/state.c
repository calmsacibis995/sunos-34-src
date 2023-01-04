#ifndef lint
static char sccsid[]= "@(#)state.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc
 */


#include "defs.h"
#include <setjmp.h>

static jmp_buf          long_jump;

/*
 *	Check if .make.state has changed
 *	If it has we reread it
 */
void
check_read_state_file()
{
	register Timetype	previous= cached_names.make_state->stat.time;
	register Makefile_type	save_makefile_type;
	register Property	makefile;

	cached_names.make_state->stat.time= FILE_NO_TIME;
	if ((exists(cached_names.make_state) == FILE_DOESNT_EXIST) ||
	    (cached_names.make_state->stat.time == previous))
		return;
	save_makefile_type= makefile_type;
	makefile_type= rereading_statefile;
	/* Make sure we clear the old cached contents of .make.state */
	makefile= maybe_append_prop(cached_names.make_state, makefile_prop);
	if (makefile->body.makefile.contents != NULL) {
		Free(makefile->body.makefile.contents);
		makefile->body.makefile.contents= NULL;
	};
	if (read_trace_level > 1)
		flag.trace_reader= true;
	(void)read_simple_file(cached_names.make_state, false, false, false, false, false, true);
	flag.trace_reader= false;
	makefile_type= save_makefile_type;
}

/*
 *	Apply a function to each element on a dependency list
 *	Abort when the function fails
 */
static int
map_dependencies(lines, fn, arg)
	register Property	lines;
	register int		(*fn) ();
	register Name		arg;

{
	register Dependency     dependency;
	register int            result;

	if (lines != NULL)
		for (dependency= lines->body.line.dependencies;
		     dependency != NULL;
		     dependency= dependency->next)
			if (result= (*fn) (dependency, arg))
				return(result);
	return(0);
}

/*
 *	Passed to map_dependencies()
 *	This finds out if there are any dependencies that should be save in .make.state
 */
static int
check_for_dependencies(dependency)
	register Dependency	dependency;
{
	return(is_true(dependency->automatic) &&
	       is_false(dependency->stale) &&
	       (dependency->name != cached_names.force));
}

#define LONGJUMP_VALUE 17
#define xfwrite(string, length, fd) {if (fwrite(string, 1, length, fd) == 0) \
					longjmp(long_jump, LONGJUMP_VALUE);}
#define xputc(ch, fd) {if (putc(ch, fd) == EOF) longjmp(long_jump, LONGJUMP_VALUE);}
#define xfputs(string, fd) fputs(string, fd)

/*
 *	Passed to map_dependencies()
 *	Will print a dependency list
 */
static int
print_depes(dependency, fd)
	register Dependency	dependency;
	register FILE		*fd;
{
	if (is_false(dependency->automatic) ||
	    is_true(dependency->stale) ||
	    (dependency->name == cached_names.force))
		return(0);
	xfwrite(dependency->name->string, (int)dependency->name->hash.length, fd);
	/* Check if the dependency line is too long. If so break it and start a */
	/* new one */
	if ((line_length += dependency->name->hash.length + 1) > 450) {
		line_length= 0;
		xputc(NEWLINE, fd);
		xfputs(target_name, fd);
		xputc(COLON, fd);
		xputc(TAB, fd);
	} else {
		xfputs(" ", fd);
	};
	return(0);
}

/*
 *	Write a new  version of .make.state
 */
void
write_state_file()
{
	register int		n;
	register int		m;
	register Boolean	name_printed;
	register Name		np;
	register Cmd_line	cp;
	register FILE		*fd;
	register Property	lines;
	char			buffer[MAXPATHLEN];
	register int		attempts= 0;
	Dependency		dp;

	if (is_false(rewrite_statefile) ||
	    is_false(command_changed) ||
	    is_false(flag.keep_state) ||
	    is_true(flag.do_not_exec_rule))
		return;
	if ((fd= fopen(cached_names.make_state->string, "w")) == NULL)
		fatal("Could not open statefile `%s': %s",
			cached_names.make_state->string, errmsg(errno));
	(void)fchmod(fileno(fd), 0666);
	/* Lock the file for writing */
	lock_file(cached_names.make_state->string);
	/* Set a trap for failed writes. If a write fails the routine will try */
	/* saving the .make.state */
	/* file under another name in /tmp */
	if (setjmp(long_jump)) {
		(void)fclose(fd);
		if (attempts++ > 5) {
			if (lock_file_name != NULL) {
				(void)unlink(lock_file_name);
				lock_file_name = NULL;
			};
			fatal("Giving up on writing statefile");
		};
		sleep(10);
		(void)sprintf(buffer, "/tmp/.make.state.%d", getpid());
		if ((fd= fopen(buffer, "w")) == NULL)
			fatal("Could not open statefile `%s': %s", buffer,
				errmsg(errno));
		warning("Initial write of statefile failed. Trying again on %s",
			buffer);
	};

	/* Write the version stamp */
	xfwrite(cached_names.make_version->string,
		(int)cached_names.make_version->hash.length,
		fd);
	xputc(COLON, fd);
	xputc(TAB, fd);
	xfwrite(cached_names.current_make_version->string,
		(int)cached_names.current_make_version->hash.length,
		fd);
	xputc(NEWLINE, fd);

	/* Go thru all targets and dump their dependencies and command used */
	for (n= HASHSIZE - 1; n >= 0; n--)
	    for (np= hashtab[n]; np != NULL; np= np->next) {
		/* Check if any .RECURSIVE lines should be written */
		if (is_true(np->has_recursive_dependency))
		    /* Go thru the property list and dump all */
		    /* .RECURSIVE lines */
		    for (lines= get_prop(np->prop, recursive_prop);
			 lines != NULL;
			 lines= get_prop(lines->next, recursive_prop)) {
			/* If this target was built during this */
			/* make run we mark it */
			if (is_true(np->has_built)) {
				xfwrite(cached_names.built_last_make_run->string,
					(int)cached_names.built_last_make_run->hash.length,
					fd);
				xputc(COLON, fd);
				xputc(NEWLINE, fd);
			};
			/* Write the .RECURSIVE line */
			xfwrite(np->string, (int)np->hash.length, fd);
			xputc(COLON, fd);
			xfwrite(cached_names.recursive->string,
				(int)cached_names.recursive->hash.length,
				fd);
			xputc(SPACE, fd);
			/* Directory the recursive make ran in */
			xfwrite(lines->body.recursive.directory->string,
				(int)lines->body.recursive.directory->hash.length,
				fd);
			xputc(SPACE, fd);
			/* Target made there */
			xfwrite(lines->body.recursive.target->string,
				(int)lines->body.recursive.target->hash.length,
				fd);
			/* Complete list of makefiles used */
			for (dp= lines->body.recursive.makefiles;
			     dp != NULL;
			     dp= dp->next) {
				xputc(SPACE, fd);
				xfwrite(dp->name->string, (int)dp->name->hash.length, fd);
			};
			xputc(NEWLINE, fd);
		    };
		/* If the target has no command used nor dependencies */
		/* we can go to the next one */
		if ((lines= get_prop(np->prop, line_prop)) == NULL)
			continue;
		/* Find out if any of the targets dependencies should */
		/* be written to .make.state */
		m= map_dependencies(lines, check_for_dependencies, (Name) NULL);
		if (m ||
		    (lines->body.line.command_used != NULL)) {
			name_printed= false;
			/* If this target was built during this make */
			/* run we mark it */
			if (is_true(np->has_built)) {
				xfwrite(cached_names.built_last_make_run->string,
					(int)cached_names.built_last_make_run->hash.length,
					fd);
				xputc(COLON, fd);
				xputc(NEWLINE, fd);
			};
			/* If the target has dependencies we dump them */
			if (m) {
				xfwrite(target_name= np->string, (int)np->hash.length, fd);
				xputc(COLON, fd);
				xfputs("\t", fd);
				name_printed= true;
				line_length= 0;
				(void)map_dependencies(lines, print_depes, (Name) fd);
				xfputs("\n", fd);
			};
			/* If there is a command used we dump it */
			if (lines->body.line.command_used != NULL) {
				/* Only write the target name if it */
				/* wasnt done for the dependencies */
				if (is_false(name_printed)) {
					xfwrite(np->string, (int)np->hash.length, fd);
					xputc(COLON, fd);
					xputc(NEWLINE, fd);
				};
				/* Write the command lines. Prefix each */
				/* textual line with a tab */
				for (cp= lines->body.line.command_used; cp != NULL; cp= cp->next) {
					char                   *csp;
					int                     n;
					xputc(TAB, fd);
					if (cp->command_line != NULL)
						for (csp= cp->command_line->string,
							n= cp->command_line->hash.length;
						     n > 0;
						     n--, csp++) {
							xputc(*csp, fd);
							if (*csp == NEWLINE)
								xputc(TAB, fd);
						};
					xputc(NEWLINE, fd);
				};
			};
		};
	    };
	if (fclose(fd) == EOF)
		longjmp(long_jump, LONGJUMP_VALUE);
	if (lock_file_name != NULL) {
		(void)unlink(lock_file_name);
		lock_file_name = NULL;
	};
}

/*
 *	Read the temp file used for reporting dependencies to make
 */
void
read_dependency_file(filename)
	register Name		filename;
{
	register Makefile_type	save_makefile_type;

	if (filename == NULL)
		return;
	filename->stat.time= FILE_NO_TIME;
	if (exists(filename) > FILE_DOESNT_EXIST) {
		save_makefile_type= makefile_type;
		makefile_type= reading_cpp_file;
		if (read_trace_level > 1)
			flag.trace_reader= true;
		temp_file_number++;
		(void)read_simple_file(filename, false, false, false, false, false, false);
		flag.trace_reader= false;
		makefile_type= save_makefile_type;
	};
}
