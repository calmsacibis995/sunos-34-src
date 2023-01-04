#ifndef lint
static char sccsid[]= "@(#)main.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc	[Remotely from S5R2]
 */

#include "defs.h"
#include "report.h"
#include <signal.h>

	Cmd_line	ar_replace_rule;
	Boolean		assign_done;
	Boolean		built_last_make_run_seen;
	Cached_names	cached_names;
	Property	current_line;
	Boolean		command_changed;
	Boolean		commands_done;
	Name		current_path;
	Name		current_target;
	short		debug_level;
	Cmd_line	default_rule;
	Name		default_target_to_build;
	int		depth;
	int		file_number;
	FILE		*fin;
	Flag		flag;
	char		funny[128];
	Name		hashtab[HASHSIZE];
	Boolean		is_conditional;
	Boolean		is_out_of_date;
	int		line_length;
	Makefile_type	makefile_type;
	pathpt		makefile_path;
	Dependency	makefiles_used;
	String		makeflags_string;
	Boolean		make_word_mentioned;
	Percent		percent_list;
	Boolean		query_mentioned;
	short		read_trace_level;
	int		recursion_level;
	Boolean		rewrite_statefile;
	Cmd_line	sccs_get_rule;
	Dependency	suffixes;
	Cmd_line	sym_link_to_rule;
	char		*target_name;
	Name		temp_file_name;
	short		temp_file_number;
	Boolean		vpath_defined;
	Boolean		working_on_targets;
	int		process_running= -1;
	int		(*sigivalue)()= SIG_DFL;
	int		(*sigqvalue)()= SIG_DFL;
	pathpt		vroot_path = VROOT_DEFAULT;
	char		*lock_file_name = NULL;

#ifdef lint
	char		**environ;
	int		errno;
	char		*sys_siglist[1];
#endif

/*
 *	myexit() is called from exit() and performs cleanup actions
 */
static int
myexit()
{
	/* Build the target .DONE */
	if (is_false(flag.quest))
		(void)doname(cached_names.done, false, true);
	/* Remove the temp file utilities report dependencies thru if it is */
	/* still around */
	if (temp_file_name != NULL) {
		(void)unlink(temp_file_name->string);
	};
	/* Do not save the current command in .make.state if make was interrupted */
	if (current_line != NULL) {
		command_changed= true;
		current_line->body.line.command_used= NULL;
	};
	/* Write .make.state */
	write_state_file();
}

/*
 *	This is where C-C traps are caught
 */
int
handle_interrupt()
{
	Property		member;

	(void)fflush(stdout);
	/* Make sure the process running under us terminates first */
	if (process_running != -1)
		(void)await(false, true);
	/* Delete the current target unless it is precious */
	if ((current_target != NULL) &&
	    (current_target->member_class != not_member) &&
	    ((member= get_prop(current_target->prop, member_prop)) != NULL))
		current_target= member->body.member.library;
	if (is_false(flag.do_not_exec_rule) && is_false(flag.touch) &&
	    is_false(flag.quest) &&
	    (current_target != NULL) &&
	    is_false(current_target->stat.is_precious)) {
		if (exists(current_target) != FILE_DOESNT_EXIST) {
		    (void)fprintf(stderr, "\n*** %s ", current_target->string);
		    if (current_target->stat.is_dir)
			(void)fprintf(stderr, "not removed.\n", current_target->string);
		    else if (unlink(current_target->string) == 0)
			(void)fprintf(stderr, "removed.\n", current_target->string);
		    else
			(void)fprintf(stderr, "could not be removed: %s.\n",
					current_target->string,
					errmsg(errno));
		}
	};
	exit(2);
}

/*
 *	This routine sets a new interrupt handler for the signals make want to deal with
 *	It is used to turn signals off around vroot() calls
 */
void
enable_interrupt(handler)
	register int		(*handler) ();

{
	if (sigivalue != SIG_IGN)
		(void)signal(SIGINT, handler);
	if (sigqvalue != SIG_IGN)
		(void)signal(SIGQUIT, handler);
}

/*
 *	This routine reads the process environment when make starts and enters it as make macros
 *	The environment variable SHELL is ignored
 */
static void
read_environment()
{
	register char		**environment;
	register char		*value;
	register char		*name;
	register Name		macro;

	environment= environ;
	for (; *environment; environment++) {
		name= *environment;
		value= index(name, EQUAL);
		if (is_equaln(name, "SHELL=", 6))
			continue;
		if (is_equaln(name, "MAKEFLAGS=", 6))
			flag.report_pwd= true;
		macro= getname(name, value - name);
		maybe_append_prop(macro, macro_prop)->body.macro.exported= true;
		if ((value == NULL) || ((value + 1)[0] == NUL))
			(void)setvar(macro, (Name) NULL, false);
		else
			(void)setvar(macro, getname(value + 1, FIND_LENGTH), false);
	};
}

/*
 *	Parse make command line options
 */
static int
parse_command_option(ch)
	register char		ch;
{
	switch (ch) {
	    case 'B':			 /* Obsolete */
		return(0);
	    case 'b':			 /* Obsolete */
		append_char(ch, &makeflags_string);
		return(0);
	    case 'D':			 /* Show lines read */
		read_trace_level++;
		return(0);
	    case 'd':			 /* debug flag */
		debug_level++;
		return(0);
	    case 'e':			 /* environment override flag */
		if (is_false(flag.env_wins))
			append_char(ch, &makeflags_string);
		flag.env_wins= true;
		return(0);
	    case 'F':			 /* Read alternative .make.state */
		return(2);
	    case 'f':			 /* Read alternative makefile(s) */
		return(1);
	    case 'g':			 /* sccs get files not found */
		return(0);
	    case 'i':			 /* ignore errors */
		if (is_false(flag.ignore_errors))
			append_char(ch, &makeflags_string);
		flag.ignore_errors= true;
		return(0);
	    case 'k':			 /* Keep making even after errors */
		if (is_false(flag.continue_after_error))
			append_char(ch, &makeflags_string);
		flag.continue_after_error= true;
		return(0);
	    case 'l':			 /* Interactive mode */
		flag.interactive= true;
		return(0);
	    case 'm':			 /* Obsolete */
		return(0);
	    case 'n':			 /* print, not exec commands */
		if (is_false(flag.do_not_exec_rule))
			append_char(ch, &makeflags_string);
		flag.do_not_exec_rule= true;
		return(0);
	    case 'N':			 /* Reverse -n */
		if (is_true(flag.do_not_exec_rule))
			append_char(ch, &makeflags_string);
		flag.do_not_exec_rule= false;
		return(0);
	    case 'P':			 /* print for selected targets */
		flag.report_dependencies= true;
		return(0);
	    case 'p':			 /* print description */
		flag.trace_status= true;
		return(0);
	    case 'q':			 /* question flag */
		if (is_false(flag.quest))
			append_char(ch, &makeflags_string);
		flag.quest= true;
		return(0);
	    case 'r':			 /* turn off internal rules */
		flag.ignore_default_mk= true;
		return(0);
	    case 'S':			 /* Reverse -k */
		if (is_true(flag.continue_after_error))
			append_char(ch, &makeflags_string);
		flag.continue_after_error= false;
		return(0);
	    case 's':			 /* silent flag */
		if (is_false(flag.silent))
			append_char(ch, &makeflags_string);
		flag.silent= true;
		return(0);
	    case 't':			 /* touch flag */
		if (is_false(flag.touch))
			append_char(ch, &makeflags_string);
		flag.touch= true;
		return(0);
	    default:
		if (is_false(flag.interactive))
			fatal("Unknown option `-%c'", ch);
	}
	return(0);
}

/*
 *	Scan the command line options and process the ones that start with "-"
 */
static void
read_command_options(argc, argv)
	register int		argc;
	register char		**argv;
{
	register int		i;
	register int		j;
	register char		ch;
	register int		makefile_next= 0;	/* flag to note -f option. */

	for (i= 1; i < argc; i++) {
		switch (makefile_next) {
		    case 2:
			cached_names.make_state= getname(argv[i], FIND_LENGTH);
		    case 1:
			makefile_next= 0;
			continue;
		};
		if ((argv[i] != NULL) && (argv[i][0] == HYPHEN)) {
			for (j= 1; (ch= argv[i][j]) != NUL; j++)
				makefile_next |= parse_command_option(ch);
			switch (makefile_next) {
			    case 0:
				argv[i]= NULL;
				break;
			    case 1:	 /* -f seen */
				argv[i]= "-f";
				break;
			    case 2:	/* -F seen */
				argv[i]= "-F";
				break;
			    case 3:	 /* -f & -F seen */
				fatal("Illegal command line. Both `-f' and `-F' given in the same argument group");
			};
		};
	};
}

/*
 *	Read one makefile and check the result
 */
static Boolean
read_makefile(makefile, complain, must_exist, report_file)
	register Name		makefile;
	Boolean			complain;
	Boolean			must_exist;
	Boolean			report_file;
{
	makefile_type= reading_makefile;
	recursion_level= 0;
	return (read_simple_file(makefile, true, true, complain,
		must_exist, report_file, false));
}

/*
 *	If this is a recursive make and the parent make has KEEP_STATE on
 *	this routine reports the dependency to the parent make
 */
static void
report_recursion(target)
	register Name		target;
{
	register Dependency	dp;
	register FILE		*report_file= get_report_file();

	if (report_file == NULL)
		return;
	check_current_path();
	(void)fprintf(report_file,
		      "%s: %s ",
		      get_target_being_reported_for(),
		      cached_names.recursive->string);
	report_dependency(current_path->string);
	report_dependency(target->string);
	for (dp= makefiles_used; dp != NULL; dp= dp->next) {
		report_dependency(dp->name->string);
	};
	(void)fprintf(report_file, "\n");
}

/*
 *	Set the magic macros TARGET_ARCH & HOST_ARCH
 */
void
set_target_host_arch()
{
	String		mach_name;
	char		buffer[STRING_BUFFER_LENGTH];
	FILE		*pipe;
	Name		name;
	register int	ch;

	init_string_from_stack(mach_name, buffer);
	append_char('-', &mach_name);
	if ((pipe= popen("/bin/mach", "r")) == NULL)
		fatal("Can not figure out which architecture we are on");
	while ((ch= getc(pipe)) != EOF)
		append_char(ch, &mach_name);
	if (pclose(pipe) != NULL)
		fatal("Can not figure out which architecture we are on");

	name= getname(mach_name.buffer.start, strlen(mach_name.buffer.start)-1);
	(void)setvar(cached_names.host_arch, name, false);
	(void)setvar(cached_names.target_arch, name, false);
}

void
main(argc, argv)
	register int		argc;
	register char		*argv[];
{
	register Name		name;
	register char		*cp;
	register int		i;
	register Boolean	makefile_read= false;
	register Boolean	target_to_make_found= false;
	char			command[128];
	register Property	macro;
	Doname			result;
	Boolean			do_not_exec_rule;

	(void)on_exit(myexit, (char *)0);
	/* Make the vroot package scan the path using shell semantics */
	set_path_style(0);
	/* Prepare to build the MAKEFLAGS and MFLAGS strings */
	append_char(HYPHEN, &makeflags_string);

	/* Load the vector funny[] with lexical markers */
	for (cp= "=@-?!"; *cp; ++cp)
		funny[*cp] |= COMMAND_PREFIX_FUNNY;
	funny[DOLLAR] |= DOLLAR_FUNNY;
	for (cp= "#|=^();&<>*?[]:$`'\"\\\n"; *cp; ++cp)
		funny[*cp] |= META_FUNNY;
	funny[PERCENT] |= PERCENT_FUNNY;
	for (cp= "@*<%?"; *cp; ++cp)
		funny[*cp] |= SPECIAL_MACRO_FUNNY;
	for (cp= "?[*"; *cp; ++cp)
		funny[*cp] |= WILDCARD_FUNNY;

	/* Load the cached_names struct */
	cached_names.ar_replace= getname(".AR_REPLACE", FIND_LENGTH);
	cached_names.built_last_make_run= getname(".BUILT_LAST_MAKE_RUN", FIND_LENGTH);
	cached_names.c_at= getname("@", FIND_LENGTH);
	cached_names.current_make_version= getname("VERSION-1.0", FIND_LENGTH);
	cached_names.default_makefile= getname_fn("default.mk", FIND_LENGTH, FILE_TYPE);
	cached_names.default_rule= getname(".DEFAULT", FIND_LENGTH);
	cached_names.dollar= getname("$", FIND_LENGTH);
	cached_names.done= getname(".DONE", FIND_LENGTH);
	cached_names.dot= getname(".", FIND_LENGTH);
	cached_names.dotdot= getname("..", FIND_LENGTH);
	cached_names.dot_a= getname(".a", FIND_LENGTH);
	cached_names.dot_keep_state= getname(".KEEP_STATE", FIND_LENGTH);
	cached_names.empty_suffix_name= getname("", FIND_LENGTH);
	cached_names.force= getname(" FORCE", FIND_LENGTH);
	cached_names.host_arch= getname("HOST_ARCH", FIND_LENGTH);
	cached_names.ignore= getname(".IGNORE", FIND_LENGTH);
	cached_names.init= getname(".INIT", FIND_LENGTH);
	cached_names.keep_state= getname("KEEP_STATE", FIND_LENGTH);
	cached_names.less= getname("<", FIND_LENGTH);
	cached_names.make= getname("MAKE", FIND_LENGTH);
	cached_names.make_state= getname(".make.state", FIND_LENGTH);
	cached_names.makefile= getname("makefile", FIND_LENGTH);
	cached_names.Makefile= getname("Makefile", FIND_LENGTH);
	cached_names.makeflags= getname("MAKEFLAGS", FIND_LENGTH);
	cached_names.make_version= getname(".MAKE_VERSION", FIND_LENGTH);
	cached_names.mflags= getname("MFLAGS", FIND_LENGTH);
	cached_names.path= getname("PATH", FIND_LENGTH);
	cached_names.percent= getname("%", FIND_LENGTH);
	cached_names.plus= getname("+", FIND_LENGTH);
	cached_names.precious= getname(".PRECIOUS", FIND_LENGTH);
	cached_names.query= getname("?", FIND_LENGTH);
	cached_names.quit= getname("quit", FIND_LENGTH);
	cached_names.recursive= getname(".RECURSIVE", FIND_LENGTH);
	cached_names.sccs_get= getname(".SCCS_GET", FIND_LENGTH);
	cached_names.sh= getname("/bin/sh", FIND_LENGTH);
	cached_names.shell= getname("SHELL", FIND_LENGTH);
	cached_names.silent= getname(".SILENT", FIND_LENGTH);
	cached_names.standard_in= getname("Standard in", FIND_LENGTH);
	cached_names.star= getname("*", FIND_LENGTH);
	cached_names.suffixes= getname(".SUFFIXES", FIND_LENGTH);
	cached_names.sunpro_dependencies= getname(SUNPRO_DEPENDENCIES, FIND_LENGTH);
	cached_names.sym_link_to= getname(".SYM_LINK_TO", FIND_LENGTH);
	cached_names.target_arch= getname("TARGET_ARCH", FIND_LENGTH);
	cached_names.usr_include= getname("/usr/include", FIND_LENGTH);
	cached_names.usr_include_sys= getname("/usr/include/sys", FIND_LENGTH);
	cached_names.virtual_root= getname("VIRTUAL_ROOT", FIND_LENGTH);
	cached_names.vpath= getname("VPATH", FIND_LENGTH);

	/* Mark special targets so that the reader treats them properly */
	cached_names.ar_replace->special_reader= ar_replace_special;
	cached_names.built_last_make_run->special_reader= built_last_make_run_special;
	cached_names.default_rule->special_reader= default_special;
	cached_names.dot_keep_state->special_reader= keep_state_special;
	cached_names.ignore->special_reader= ignore_special;
	cached_names.make_version->special_reader= make_version_special;
	cached_names.precious->special_reader= precious_special;
	cached_names.sccs_get->special_reader= sccs_get_special;
	cached_names.silent->special_reader= silent_special;
	cached_names.suffixes->special_reader= suffixes_special;
	cached_names.sym_link_to->special_reader= sym_link_to_special;

	set_target_host_arch();
	/* The value of $$ is $ */
	(void)setvar(cached_names.dollar, cached_names.dollar, false);
	cached_names.dollar->dollar= false;
	/* Set the value of $(SHELL) */
	(void)setvar(cached_names.shell, cached_names.sh, false);
	/* Use " FORCE" to simulate a FRC dependency for :: type targets with no dependencies */
	(void)append_prop(cached_names.force, line_prop);
	cached_names.force->stat.time= FILE_MAX_TIME;
	/* Make sure VPATH is defined before current dir is read */
	if ((cp= getenv(cached_names.vpath->string)) != NULL)
		(void)setvar(cached_names.vpath, getname(cp, FIND_LENGTH), false);
	/* Check if there is NO PATH variable. If not we construct one. */
	if (getenv(cached_names.path->string) == NULL) {
		vroot_path = NULL;
		add_dir_to_path(".", &vroot_path, -1);
		add_dir_to_path("/bin", &vroot_path, -1);
		add_dir_to_path("/usr/bin", &vroot_path, -1);
	};
/*
 *	Set command line flags
 */
	for (cp= getenv(cached_names.makeflags->string);
	     (cp != NULL) && (*cp != NUL);
	     cp++)
		(void)parse_command_option(*cp);
	read_command_options(argc, argv);
	if (debug_level > 0) {
		cp= getenv(cached_names.makeflags->string);
		(void)printf("MAKEFLAGS value: %s\n", cp == NULL ? "":cp);
	};
			     

/*
 *	Make sure MAKEFLAGS is exported
 */
	maybe_append_prop(cached_names.makeflags, macro_prop)->body.macro.exported= true;

/*
 *	Read internal definitions and rules.
 */
	if (read_trace_level > 1)
		flag.trace_reader= true;
	if (is_false(flag.ignore_default_mk))
		(void)read_makefile(cached_names.default_makefile, true, false, false);
	flag.trace_reader= false;

/*
 *	Read environment args.  Let file args which follow override unless -e option seen.
 *	If -e option is not mentioned.
 */
	if (is_false(flag.env_wins)) {
		read_environment();
		if (getvar(cached_names.virtual_root)->hash.length == 0)
			(void)setvar(cached_names.virtual_root,
				     getname("/", FIND_LENGTH), false);
	};

/*
 *	Read state file
 */
	makefile_type= reading_statefile;
	if (read_trace_level > 1)
		flag.trace_reader= true;
	(void)read_simple_file(cached_names.make_state, false, false, false, false, false, true);
	flag.trace_reader= false;
	default_target_to_build= NULL;

/*
 *	Set MFLAGS and MAKEFLAGS
 */
	if (makeflags_string.buffer.start[1] != NUL)
		(void)setvar(cached_names.mflags,
			     getname(makeflags_string.buffer.start, FIND_LENGTH), false);
	(void)setvar(cached_names.makeflags,
		     getname(makeflags_string.buffer.start + 1, FIND_LENGTH), false);
	Free(makeflags_string.buffer.start);
	makeflags_string.buffer.start= NULL;

/*
 *	Read command line "=" type args and make them readonly.
 */
	for (i= 1; i < argc; ++i)
	    if ((argv[i] != NULL) &&
		(argv[i][0] != HYPHEN) &&
		((cp= index(argv[i], EQUAL)) != NULL)) {
		*cp= NUL;
		maybe_append_prop(name= getname(argv[i], FIND_LENGTH), macro_prop)->
						body.macro.exported= true;
		if ((cp + 1)[0] == NUL)
			setvar(name, (Name) NULL, false)->body.macro.read_only= true;
		else
			setvar(name, getname(cp + 1, FIND_LENGTH), false)->
						body.macro.read_only= true;
		argv[i]= NULL;
	    };

/*
 *	Read command line "-f" arguments and ignore "-F" arguments.
 */
	do_not_exec_rule= flag.do_not_exec_rule;
	flag.do_not_exec_rule= false;
	if (read_trace_level > 0)
		flag.trace_reader= true;
	for (i= 1; i < argc; i++)
		if (argv[i] &&
		    (argv[i][0] == HYPHEN) &&
		    (argv[i][1] == 'f') &&
		    (argv[i][2] == NUL)) {
			argv[i]= NULL;		/* Remove -f */
			if (i >= argc - 1)
				fatal("No filename argument after -f flag");
			(void)read_makefile(getname(argv[++i], FIND_LENGTH), true, true, true);
			argv[i]= NULL;		/* Remove filename */
			makefile_read= true;
		} else
		if (argv[i] &&
		    (argv[i][0] == HYPHEN) &&
		    (argv[i][1] == 'F') &&
		    (argv[i][2] == NUL)) {
			argv[i++]= NULL;
			argv[i]= NULL;
		};

/*
 *	If no command line "-f" args then look for "makefile", and then for
 *	"Makefile" if "makefile" isn't found
 */
	if (is_false(makefile_read)) {
		read_dir(cached_names.dot, (char *)NULL, (Property)NULL, (char *)NULL);
		if (is_true(cached_names.makefile->stat.is_file))
			makefile_read= read_makefile(cached_names.makefile, false, false, true);
		if (is_false(makefile_read) && is_true(cached_names.Makefile->stat.is_file))
			makefile_read= read_makefile(cached_names.Makefile,
				false, false, true);
	};
	flag.trace_reader= false;
	flag.do_not_exec_rule= do_not_exec_rule;

/*
 *	Read environment vars.  Let file args which follow override unless -e option seen.
 *	If -e option is mentioned.
 */
	if (is_true(flag.env_wins)) {
		read_environment();
		if (getvar(cached_names.virtual_root)->hash.length == 0)
			(void)setvar(cached_names.virtual_root,
				     getname("/", FIND_LENGTH), false);
	};

/*
 *	Make sure KEEP_STATE is in the environment if KEEP_STATE is on
 */
	if (((macro= get_prop(cached_names.keep_state->prop, macro_prop)) != NULL) &&
	    is_true(macro->body.macro.exported))
		flag.keep_state= true;
	if (is_true(flag.keep_state)) {
		if (macro == NULL)
			macro= maybe_append_prop(cached_names.keep_state, macro_prop);
		macro->body.macro.exported= true;
		(void)setvar(cached_names.keep_state, cached_names.empty_suffix_name, false);
	};

	sigivalue= signal(SIGINT, (int (*) ()) SIG_IGN);
	sigqvalue= signal(SIGQUIT, (int (*) ()) SIG_IGN);
	enable_interrupt(handle_interrupt);

/*
 *	Check if make should report
 */
	if (getenv(cached_names.sunpro_dependencies->string) != NULL) {
		report_dependency("");
		(void)fprintf(get_report_file(), "\n");
	};

/*
 *	Make sure SUNPRO_DEPENDENCIES is exported (or not) properly
 */
	if (is_true(flag.keep_state)) {
		maybe_append_prop(cached_names.sunpro_dependencies, macro_prop)->
			body.macro.exported= true;
		(void)setvar(cached_names.sunpro_dependencies,
			     getname("", FIND_LENGTH),
			     false);
	} else
		maybe_append_prop(cached_names.sunpro_dependencies, macro_prop)->
			body.macro.exported= false;

	working_on_targets= true;
	makefile_type= reading_nothing;
	if (is_true(flag.trace_status))
		printdesc();
	flag.trace_reader= false;
	(void)doname(cached_names.init, true, true);
	recursion_level= 1;
/*
 *	make remaining args
 */
	for (i= 1; i < argc; i++)
	    if ((cp= argv[i]) != NULL) {
		target_to_make_found= true;
		default_target_to_build= getname(cp, FIND_LENGTH);
		commands_done= false;
		if (is_true(flag.report_dependencies))
			print_dependencies(default_target_to_build,
					   get_prop(default_target_to_build->prop, line_prop),
					   true,
					   true);
		else {
			if ((result= doname_check(default_target_to_build, true, false, false)) == build_failed)
				fatal("Target `%s' not remade because of errors",
				      default_target_to_build->string);
			else
				report_recursion(default_target_to_build);
		};
		default_target_to_build->stat.time= FILE_NO_TIME;
		if (is_false(commands_done) &&
		    (result == build_ok) &&
		    is_false(flag.quest) &&
		    is_false(flag.report_dependencies) &&
		    (exists(default_target_to_build) > FILE_DOESNT_EXIST))
			(void)printf("`%s' is up to date.\n",
				     default_target_to_build->string);
	    };

/*
 *	If no file arguments have been encountered,
 *	make the first name encountered that doesnt start with a dot
 */
	if (is_false(flag.interactive) && is_false(target_to_make_found)) {
		if (default_target_to_build == NULL)
			fatal("No arguments to build");
		commands_done= false;
		if (is_true(flag.report_dependencies))
			print_dependencies(default_target_to_build,
					   get_prop(default_target_to_build->prop, line_prop),
					   true,
					   true);
		else {
			if ((result= doname_check(default_target_to_build, true, false, false)) == build_failed)
				fatal("Target `%s' not remade because of errors",
				      default_target_to_build->string);
			else
				report_recursion(default_target_to_build);
			report_recursion(default_target_to_build);
		};
		default_target_to_build->stat.time= FILE_NO_TIME;
		if (is_false(commands_done) &&
		    (result == build_ok) &&
		    is_false(flag.quest) &&
		    is_false(flag.report_dependencies) &&
		    (exists(default_target_to_build) > FILE_DOESNT_EXIST))
			(void)printf("`%s' is up to date.\n",
				     default_target_to_build->string);
	};

	if (is_true(flag.interactive))
	    while (1) {
		(void)fflush(stderr);
		(void)printf("Target: ");
		(void)fflush(stdout);
		if (gets(command) == NULL)
			break;
		if (command[0] == HYPHEN) {
			(void)parse_command_option(command[1]);
			continue;
		};
		if ((name= getname(command, FIND_LENGTH)) == cached_names.quit)
			break;
		commands_done= false;
		if (is_true(flag.report_dependencies))
			print_dependencies(name, get_prop(name->prop, line_prop), true, true);
		else {
			if (doname_check(name, true, false, false) == build_failed)
				(void)printf("Target `%s' not remade because of errors\n",
					     name->string);
			report_recursion(name);
		};
		name->stat.time= FILE_NO_TIME;
		if (is_false(commands_done) &&
		    is_false(flag.quest) &&
		    is_false(flag.report_dependencies) &&
		    (exists(name) > FILE_DOESNT_EXIST))
			(void)printf("`%s' is up to date.\n", name->string);
		for (i= HASHSIZE - 1; i >= 0; i--)
			for (name= hashtab[i]; name != NULL; name= name->next) {
				name->stat.time= FILE_NO_TIME;
				name->state= build_dont_know;
			};
	    };

	exit(0);
}
