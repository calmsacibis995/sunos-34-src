/* @(#)compile.c 1.12 86/12/15 SMI */

#include "driver.h"
#include <sys/file.h>
#include <signal.h>
#include <ctype.h>

#ifdef lint
vroot_datat	vroot_data;
#endif

#define super_int(name, value, help) super_intt name= {(value), (help), 0, 0};
	define_super_int()

#define regular_int(name) int name;
	define_regular_int()

#define variable_2(type, name) type name;
#define variable_3(type, name, value) type name = value;
	define_regular_variable()

#define PROGRAM3(name, path, driver, setup, t1, t2, t3) \
	{"name", (driver), NULL, NULL, NULL, NULL, (path), (setup), {t1, t2, t3, NULL}},
#define PROGRAM4(name, path, driver, setup, t1, t2, t3, t4) \
	{"name", (driver), NULL, NULL, NULL, NULL, (path), (setup), {t1, t2, t3, t4, NULL}},
programvt program= {
	PROGRAMS
	{ NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

/**** add_to_list_tail3 ****/
char *add_to_list_tail3(cons, valuep, suffix) conspt *cons; char *valuep; suffixpt suffix;
{	conspt p= (conspt)get_memory(sizeof(const)), q;

	p->value= valuep;
	p->next= (conspt)NULL;
	p->suffix= suffix;
	if (*cons == NULL)
		*cons= p;
	else {
		for (q= *cons; q->next != NULL; q= q->next);
		q->next= p;};
	return(valuep);
}	/* add_to_list_tail3 */

/**** get_file_suffix ****/
char *get_file_suffix(name) char *name;
{	char *p= rindex(name, '.'), *result;

	if (p == NULL) return("");
	result= get_memory(strlen(p+1)+1);
	(void)strcpy(result, p+1);
	return(result);
}	/* get_file_suffix */

/**** print_help_prefix ****/
static void print_help_prefix(drivers) int drivers;
{	int flags= 0;

	if ((drivers & Z_C) != 0) { (void)printf("C"); flags++;};
	if ((drivers & Z_F) != 0) { (void)printf("F"); flags++;};
	if ((drivers & Z_L) != 0) { (void)printf("L"); flags++;};
	if ((drivers & Z_M) != 0) { (void)printf("M"); flags++;};
	if ((drivers & Z_P) != 0) { (void)printf("P"); flags++;};
	if ((drivers & HIDE_OPTION) != 0) { (void)printf("[H]"); flags+=3;};
	(void)printf("%*s", 10-flags, "");
}	/* print_help_prefix */

#define maybe_print_prefix(bits) \
	if (driver.value == NULL) \
		print_help_prefix(bits); \
	else { \
		if ((((bits) & driver.value->value) == 0) || (((bits) & HIDE_OPTION) != 0)) continue;}
#define print_standard_help(string, arg1, arg2) \
	maybe_print_prefix(op->drivers); \
	(void)printf("%s:%*s", op->name, indent-strlen(op->name), "");\
	if (op->help) (void)printf("%s\n", op->help); else (void)printf(string, arg1, arg2); break

/**** print_help ****/
static void print_help()
{	optionpt op= options;
	int indent= 21, head;
	suffixpt sp;
	char *rest1;

	for (;op && (op->type != end_of_list); op++) {
	    if ((op->drivers&OBSOLETE_OPTION) != 0) {
		maybe_print_prefix(op->drivers);
		(void)printf("%s:%*sObsolete option\n", op->name, indent-strlen(op->name), "");
		continue;};
	    switch (op->type) {
		case help_option:
			print_standard_help("Prints this message\n", NULL, NULL);
		case infile_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%sX:%*s%s\n",
				op->name, indent-1-strlen(op->name), "", op->help);
			break;
		case make_lint_option:
			print_standard_help("Make lint library\n", NULL, NULL);
		case module_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%s X:%*sForce module to load from specified file\n",
				op->name, indent-2-strlen(op->name), "");
			break;
		case module_list_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%sX:%*sAdd directory to path used when looking for modules\n",
				op->name, indent-1-strlen(op->name), "");
			break;
		case optimize_option:
			print_standard_help("Generate optimized code\n", NULL, NULL);
		case outfile_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%s file:%*sSet name of output file\n",
					op->name, indent-5-strlen(op->name), "");
			break;
		case pass_on_lint_option:
			print_standard_help("Pass option %s on to program lint\n", op->name, NULL);
		case pass_on_select_option: { programpt pp;
			for (pp= (programpt)&program; pp != &program.sentinel_program_field; pp++) {
				if (pp->name == NULL) continue;
				maybe_print_prefix((op->drivers&pp->drivers)|
						   ((op->drivers|pp->drivers)&HIDE_OPTION));
				(void)printf("%s %s X:%*sPass option X on to program %s\n",
						op->name, pp->name,
						indent-3-strlen(op->name)-strlen(pp->name), "",pp->name);};
			break;};
			case pass_on_1_option:
			head= 1; rest1= ""; goto pass_on;
		case pass_on_1t_option:
			head= 1; rest1= "X"; goto pass_on;
		case pass_on_12_option:
			head= 1; rest1= " X"; goto pass_on;
		case pass_on_1to_option:
			head= 0; rest1= "X"; goto pass_on;
		case pass_on_2_option:
			head= 0; rest1= " X"; goto pass_on;
	pass_on:
			maybe_print_prefix(op->drivers);
			(void)printf("%s%s:%*s",
				op->name, rest1, indent-strlen(op->name)-strlen(rest1), "");
			if (op->help)
				(void)printf("%s\n", op->help);
			else {
				if (!head && (rest1[0] == ' ')) rest1++;
				(void)printf("Pass option '%s%s' on to program %s\n",
					head ? op->name : "", rest1, op->program->name);};
			break;
		case produce_option:
			for (sp= (suffixpt)&suffix; sp->suffix != NULL; sp++) {
				if (sp->suffix[0] == 0) continue;
				maybe_print_prefix((op->drivers&sp->out_drivers)|
						   ((op->drivers|sp->out_drivers)&HIDE_OPTION));
				(void)printf("%s .%s:%*sProduce type .%s file (%s)\n",
					op->name, sp->suffix,
					indent-2-strlen(op->name)-strlen(sp->suffix), "",
					sp->suffix, sp->help);};
			break;
		case replace_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%s X:%*sLook for compiler passes in directory X first\n",
					op->name, indent-2-strlen(op->name), "");
			break;
		case load_m2l_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%s X:%*sLoad the specified modula-2 module\n",
					op->name, indent-2-strlen(op->name), "");
			break;
		case run_m2l_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%s X:%*sRun the modula-2 linker on the specified root module\n",
					op->name, indent-2-strlen(op->name), "");
			break;
		case set_int_arg_option:
			print_standard_help("Sets %s\n", op->pointer->help, op->arg->name);
		case set_super_int_arg_option:
			print_standard_help("Sets value of %s to %s\n", op->pointer->help, op->arg->name);
		case target_option:
			print_standard_help("Sets target architecture\n", NULL, NULL);
		case temp_dir_option:
			maybe_print_prefix(op->drivers);
			(void)printf("%sdir:%*sSet directory for temporary files to <dir>\n",
				op->name, indent-3-strlen(op->name), "");
			break;
		default:
			fatal0("Internal error\n");};};
	if (driver.value != &lint_driver)
		(void)printf("All other options are passed down to ld\n");
	for (sp= (suffixpt)&suffix; sp->suffix != NULL; sp++) {
		if (sp->suffix[0] == 0) continue;
		maybe_print_prefix(sp->in_drivers);
		(void)printf("Suffix '%s':%*s%s\n",
			sp->suffix, indent-9-strlen(sp->suffix), "", sp->help);};
	do { /* This makes the continues in maybe_print_prefix legal */
		maybe_print_prefix(Z_all);
		(void)printf("'file.X=.Y' will read the file 'file.X' but treat it as if it had suffix 'Y'\n");}
		while (0);
	do { /* This makes the continues in maybe_print_prefix legal */
		maybe_print_prefix(Z_CFMP);
		(void)printf("%s:%*sEnvironment variable with floating point option\n",
			FLOAT_OPTION, indent-strlen(FLOAT_OPTION), "");} while (0);
}	/* print_help */

/**** lint_lib ****/
char *lint_lib(lib) char *lib;
{	char path[MAXPATHLEN];
	char path2[MAXPATHLEN];

	(void)sprintf(path, "%s/llib%s.ln", lint_library_dir, lib);
	scan_path(path, path2);
	return(strcpy(get_memory(strlen(path2)+1), path2));
}	/* lint_lib */

/**** handle_infile ****/
static void handle_infile(file, extra_drivers) char *file; int extra_drivers;
{	char *s, *p;
	suffixpt sp;

	if (file[0] == '-') {
		if (driver.value == &lint_driver) {
			file= lint_lib(file);
			s= suffix.ln.suffix;}
		else
			s= suffix.a.suffix;}
	else {
		s= get_file_suffix(file);
		if ((p= index(file, '=')) != NULL) *p= 0;};
	if (!strcmp(s, suffix.il.suffix)) {
		set_flag(do_inline);
		(void)add_to_list_tail(&program.inline.permanent_flags, "-i");
		(void)add_to_list_tail(&program.inline.permanent_flags, file);
		return;}
	for (sp= (suffixpt)&suffix; sp->suffix != NULL; sp++)
		if (!strcmp(sp->suffix, s) && (((sp->in_drivers|extra_drivers) & driver.value->value) != 0)) {
		    suffix_cheat:
			(void)add_to_list_tail3(&infile, file, sp);
			infile_count++;
			if ((sp->in_drivers&SOURCE_SUFFIX) == SOURCE_SUFFIX)
				source_infile_count++;
			return;};
	if (driver.value == &lint_driver) {
		(void)printf("%s: Warning: File with unknown suffix (%s) passed to lint1\n", program_name, file);
		sp= &suffix.c;
		goto suffix_cheat;}
	else {
		(void)printf("%s: Warning: File with unknown suffix (%s) passed to ld\n", program_name, file);
		(void)add_to_list_tail3(&infile, file, &suffix.o);
		infile_count++;};
}	/* handle_infile */

/**** lookup_option ****/
static int lookup_option(options, argc, argv, infile_ok, extra_drivers)
	optiont		*options;
	int		*argc, infile_ok;
	char		*argv[];
{	int args_used= 1, length;
	char *p;
	suffixpt sp;

	if (*argc <= 0) fatal0("No more arguments\n");
	for (;options && (options->type != end_of_list); options++) {
	    if (driver.value && ((options->drivers|extra_drivers) & driver.value->value) == 0) continue;
	    if ((strncmp(argv[0], options->name, length= strlen(options->name)) == 0)) {
		if ((options->drivers&OBSOLETE_OPTION) != 0)
			(void)printf("%s: Warning: Obsolete option %s\n", program_name, options->name);
		if (((options->drivers&NO_MINUS_OPTION) != 0) && (*argc > 1) && (argv[1][0] == '-'))
			(void)printf("%s: Warning: Extra argument for %s starts with '-'\n", program_name, options->name);
		switch (options->type) {
			case help_option:
				print_help();
				exit(0);
			case infile_option:
				if ((argv[0][0] == '-') && (argv[0][1] == 'l') &&
				    (argv[0][2] == 0) && (driver.value == &pc_driver)) {
					(void)add_to_list_tail(&program.pc0.permanent_flags, "-l");
					break;};
				handle_infile(argv[0], extra_drivers);
				goto exit_fn;
			case make_lint_option:
				(void)add_to_list_tail(&program.lint1.permanent_flags, "-L");
				p= get_memory(strlen(argv[0])+8);
				(void)sprintf(p, "-Cllib-l%s", argv[0]+2);
				(void)add_to_list_tail(&program.lint1.permanent_flags, p);
				p= get_memory(strlen(argv[0])+8);
				(void)sprintf(p, "llib-l%s.ln", argv[0]+2);
				outfile= p;
				requested_suffix= &suffix.ln;
				if (build_lint_lib++)
					fatal0("Multiple -C options\n");
				break;
			case module_option:
				if (*argc <= 1) fatal0("-m option with no argument\n");
				args_used++;
				(void)add_to_list_tail(&program.m2cfe.permanent_flags, argv[0]);
				(void)add_to_list_tail(&program.m2cfe.permanent_flags, argv[1]);
				(void)add_to_list_tail(&program.m2l.permanent_flags, argv[0]);
				(void)add_to_list_tail(&program.m2l.permanent_flags, argv[1]);
				break;
			case module_list_option:
				if (argv[0][2] == 0) {
					module_list= NULL;
					set_flag(no_default_module_list);}
				else
					(void)add_to_list_tail(&module_list, argv[0]);
				break;
			case optimize_option:
				(void)add_to_list_tail(&program.pc0.permanent_flags, argv[0]);
				(void)add_to_list_tail(&program.iropt.permanent_flags, argv[0]);
				set_flag(optimize);
				break;
			case outfile_option:
				if (*argc <= 1) fatal0("-o option but no filename\n");
				args_used++;
				outfile= argv[1];
				break;
			case pass_on_lint_option:
				(void)add_to_list_tail(&program.lint1.permanent_flags, argv[0]);
				(void)add_to_list_tail(&program.lint2.permanent_flags, argv[0]);
				break;
			case pass_on_select_option: { programpt pp;
				if (*argc <= 2) fatal1("%s option without arguments\n", argv[0]);
				args_used+= 2;
				for (pp= (programpt)&program; pp != &program.sentinel_program_field; pp++)
					if ((pp->name != NULL) &&
						strncmp(argv[1], pp->name, strlen(pp->name)) == 0) {
						(void)add_to_list_tail(&pp->permanent_flags, argv[2]);
						goto found_prog;};
				fatal2("%s option with unknown program %s\n", argv[0], argv[1]);
			    found_prog:
				break;};
			case pass_on_1_option:	/* -x		=> -x */
				if (length != strlen(argv[0]))
					fatal2("Garbage trailing option %s (%s)\n",
						options->name, argv[0]);
				/* Fall into */
			case pass_on_1t_option:	/* -xREST	=> -xREST */
				(void)add_to_list_tail(&options->program->permanent_flags, argv[0]);
				break;
			case pass_on_12_option:	/* -x REST	=> -x REST */
				if (*argc <= 1) fatal1("%s option without arguments\n", argv[0]);
				args_used++;
				(void)add_to_list_tail(&options->program->permanent_flags, argv[0]);
				(void)add_to_list_tail(&options->program->permanent_flags, argv[1]);
				break;
			case pass_on_1to_option:	/* -xREST	=> REST */
				if (argv[0][length] != 0)
					(void)add_to_list_tail(&options->program->permanent_flags,
								argv[0]+length);
				break;
 			case pass_on_2_option:		/* -x REST	=> compile    */
				break;
			case produce_option:
				if (*argc <= 1) fatal1("%s option without argument\n", argv[0]);
				args_used++;
				for (sp= (suffixpt)&suffix; sp->suffix != NULL; sp++)
					if (!strncmp(argv[1]+1, sp->suffix, strlen(sp->suffix))) {
						set_int(produce, NULL);
						requested_suffix= sp;
						goto found_suffix;};
				fatal1("Don't know how to produce %s\n", argv[1]);
			    found_suffix:
				break;
			case replace_option:
				if (*argc <= 1) fatal1("%s option without argument\n", argv[0]);
				args_used++;
				add_path_dir(argv[1], &program_path, -1);
				last_program_path++;
				break;
			case run_m2l_option:
				if (is_on(root_module_seen)) fatal1("Multiple %s options\n", argv[0]);
				set_flag(root_module_seen);
				/* Fall into */
			case load_m2l_option:
				if (*argc <= 1) fatal1("%s option without argument\n", argv[0]);
				if (produce.touched && !produce.redefine_ok &&
				    (produce.value != &executable)) {
					(void)fflush(stdout);
					(void)fprintf(stderr, "%s: %s redefines %s from %s to %s\n",
						program_name,
						argv[0],
						produce.help,
						produce.value->name,
						executable.name);
					(void)fflush(stderr);};
				set_int(produce, &executable);
				(void)add_to_list_tail3(&infile, argv[0], &suffix.o);
				(void)add_to_list_tail3(&infile, argv[1], &suffix.o);
				args_used++;
				set_flag(do_m2l);
				break;
			case set_int_arg_option:
				if ((int)(options->arg) != 0)
					set_flag((int)(options->pointer));
				else
					reset_flag((int)(options->pointer));
				break;
			case set_super_int_arg_option:
				if (options->pointer->touched && !options->pointer->redefine_ok &&
				    (options->pointer->value != options->arg)) {
					(void)fflush(stdout);
					(void)fprintf(stderr, "%s: %s redefines %s from %s to %s\n",
						program_name,
						argv[0],
						options->pointer->help,
						options->pointer->value->name,
						options->arg->name);
					(void)fflush(stderr);};
				set_int(*(options->pointer), options->arg);
				break;
			case temp_dir_option:
				if (argv[0][length] == 0)
					fatal1("Bad %s option\n", argv[0]);
				temp_dir= get_memory(strlen(argv[0])-length+1);
				(void)strcpy(temp_dir, argv[0]+length);
				break;
			case target_option:
				if (*argc <= 1) fatal1("%s option without target argument\n", argv[0]);
				target_architecture= argv[1];
				args_used++;
				break;
			default:
				fatal0("Internal error\n");};
		goto exit_fn;};};

	if (!infile_ok || (argv[0][0] == '-')) {
		(void)add_to_list_tail(&program.ld.permanent_flags, argv[0]);
		if (driver.value == &lint_driver)
			fatal1("Option %s not recognized\n", argv[0])
		else
			(void)printf("%s: Warning: Option %s passed to ld\n", program_name, argv[0]);}
	else
		handle_infile(argv[0], extra_drivers);
exit_fn:
	*argc-= args_used;
	return(args_used);
}	/* lookup_option */

/**** default_float_mode ****/
static const_intpt default_float_mode()
{	char *float_option= getenv(FLOAT_OPTION);

	if (float_option && (driver.value != &lint_driver)) { char option[128], *argv[2]; int argc= 1;
		if (float_option[0] != '-') {
			option[0]= '-';
			(void)strcpy(option+1, float_option);}
		else
			(void)strcpy(option, float_option);
		argv[0]= option; argv[1]= NULL;
		(void)lookup_option(options, &argc, argv, 0, 0);
		if (!float_mode.value) fatal2("%s value not ok:%s\n", FLOAT_OPTION, float_option);
		return(float_mode.value);};
	return(&fsoft);
}	/* default_float_mode */

/**** check_if_supported ****/
static void check_if_supported()
{
	if (((target.value == &mc68010_arch) && (float_mode.value == &ffpa)) ||
	    ((target.value == &mc68020_arch) && (float_mode.value == &fsky))) {
		(void)fflush(stdout);
		(void)fprintf(stderr, "%s: The %s and %s combination is not supported\n",
					program_name, target.value->name, float_mode.value->name);
		cleanup(0);
		exit(1);};
}	/* check_if_supported */

/**** cleanup ****/
void cleanup(abort) int abort;
{	conspt cp;

	for (cp= files_to_unlink; cp != NULL; cp= cp->next)
		if (((int)cp->value) > 0) {
			if (is_on(verbose) || is_on(dryrun) || is_on(time_run)) {
				(void)fflush(stdout);
				(void)fprintf(stderr, "rm %s\n", cp->value);
				(void)fflush(stderr);};
			if (abort)
				(void)truncate(cp->value, 0);
			(void)unlink(cp->value);};
}	/* cleanup */

/**** abort_program ****/
static int abort_program()
{
	cleanup(1);
	exit(1);
}	/* abort_program */

/**** main ****/
void main(argc, argv)
	int		argc;
	char		*argv[];
{	char *p;
	char filename[MAXPATHLEN];

	(void)signal(SIGINT, abort_program);
	(void)signal(SIGTERM, abort_program);
	(void)signal(SIGQUIT, abort_program);
	(void)signal(SIGHUP, abort_program);
	arg_list_size= argc+32;
	process_number= getpid();
	if ((p= rindex(argv[0], '/')) != NULL) argv[0]= p+1;
	program_name= argv[0];
	if (argv[0][0] == 'x') {
		set_flag(dryrun);
		(argv[0])++;};
	argv+= lookup_option(drivers, &argc, argv, 0, Z_all);
	if (driver.value == NULL) fatal1("Unknown driver %s\n", argv[-1]);
	if (driver.value == &lint_driver) {
		temp_dir= "/usr/tmp";
		(void)dup2(fileno(stdout), fileno(stderr));};
	while (argc > 0)
		argv+= lookup_option(options, &argc, argv, 1, 0);
	if (driver.value == &dummy_driver) exit(0);
	if (is_on(dryrun) && target_architecture)
		fatal0("Can't combine -Qtarget and -dryrun\n");
	if (is_on(do_dependency))
		set_int(produce, &preprocessed);
	if ((outfile != NULL) && (infile_count > 1) && (produce.value != &executable)) {
		(void)fflush(stdout);
		(void)fprintf(stderr, "%s: Warning: -o option ignored\n",  program_name);
		(void)fflush(stderr);
		outfile= NULL;};
	if (target_architecture)
		fatal0("-Qtarget not implemented yet\n");
	host.value= is68020() ? &mc68020_arch : &mc68010_arch;
	if (!target.value)
		target= host;
	if (!float_mode.value)
		float_mode.value= default_float_mode();
	check_if_supported();
	(void)fflush(stdout);
	if (target.value != host.value) { char *p= getenv(vroot_data.vroot.env_var), *q;
		if (!p) p= "";
		q= get_memory(32+strlen(target.value->name)+strlen(p));
		(void)sprintf(q, "/usr/arch/%s:%s", target.value->name, p);
		setenv(vroot_data.vroot.env_var, q);
		if (is_on(verbose) || is_on(dryrun))
			(void)fprintf(stderr, "setenv %s \"%s\"\n", vroot_data.vroot.env_var, q);};
	if (debugger.touched) {
		if (is_on(statement_count)) {
			reset_flag(statement_count);
			(void)fprintf(stderr, "%s: Warning: dbx cannot handle -a. -a turned off.\n", program_name);};
		if (is_on(optimize)) {
			reset_flag(optimize);
			(void)fprintf(stderr, "%s: Warning: dbx cannot handle -O. -O turned off.\n", program_name);};
		if (is_on(as_R)) {
			reset_flag(as_R);
			(void)fprintf(stderr, "%s: Warning: dbx cannot handle -R. -R turned off.\n", program_name);};};
	if (is_on(statement_count) && is_on(optimize)) {
		reset_flag(optimize);
		(void)fprintf(stderr, "%s: Warning: -a conflicts with -O. -O turned off.\n", program_name);};
	(void)fflush(stderr);
/* Get current dir */
	if (is_on(statement_count)) {
		(void)getwd(filename);
		pwd= get_memory(strlen(filename)+1);
		(void)strcpy(pwd, filename);};
	init_programs();
	if (driver.value->name != NULL)
		(*((void (*)())(driver.value->name)))();
	cleanup(0);
	exit(exit_status);
}	/* main */
