/* @(#)setup.c 1.1 87/01/08 SMI */

#include "driver.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>


/**** clear_program_flags ****/
void clear_program_flags()
{	programpt p;

	for (p= (programpt)&program; p->name != NULL; p++)
		p->flags= p->infile= NULL;
}	/* clear_program_flags */

/**** compile_c ****/
void compile_c(source) char *source;
{
	set_flag(do_cpp);
	compile_i(source);
	reset_flag(do_cpp);
}	/* compile_c */

/**** compile_def ****/
void compile_def(source) char *source;
{
	clear_program_flags();
	run_steps(source, def_steps);
}	/* compile_def */

/**** compile_f_for_cc ****/
void compile_f_for_cc(source) char *source;
{
	clear_program_flags();
	if (requested_suffix == &suffix.s)
		set_flag(do_cat);
	run_steps(source, is_on(statement_count) ? f_iropt_steps : f_no_iropt_steps);
	reset_flag(do_cat);
}	/* compile_f_for_cc */

/**** compile_f ****/
void compile_f(source) char *source;
{	int inline= is_on(do_inline) ? 1:0;
	char path[MAXPATHLEN];
	char *il;

	if (driver.value != &f77_driver) {
		compile_f_for_cc(source);
		return;};
	clear_program_flags();
	if (requested_suffix == &suffix.s)
		set_flag(do_cat);
	run_steps(source, is_on(optimize) || is_on(statement_count) ? f_iropt_steps : f_no_iropt_steps);
	if (!inline) reset_flag(do_inline);
	reset_flag(do_cat);
}	/* compile_f */

/**** compile_F ****/
void compile_F(source) char *source;
{
	set_flag(do_cpp);
	compile_f(source);
	reset_flag(do_cpp);
}	/* compile_F */

/**** compile_i ****/
void compile_i(source) char *source;
{
	if (requested_suffix == &suffix.s)
		set_flag(do_cat);
	clear_program_flags();
	run_steps(source, c_steps);
	reset_flag(do_cat);
}	/* compile_i */

/**** compile_mod ****/
void compile_mod(source) char *source;
{
	clear_program_flags();
	run_steps(source, mod_steps);
}	/* compile_mod */

/**** compile_p ****/
void compile_p(source) char *source;
{
	set_flag(do_cpp);
	clear_program_flags();
	run_steps(source, p_steps);
	reset_flag(do_cpp);
}	/* compile_p */

/**** compile_pi ****/
void compile_pi(source) char *source;
{
	clear_program_flags();
	run_steps(source, p_steps);
}	/* compile_pi */

/**** compile_r ****/
void compile_r(source) char *source;
{
	set_flag(do_ratfor);
	compile_f(source);
	reset_flag(do_ratfor);
}	/* compile_r */

/**** compile_S ****/
void compile_S(source) char *source;
{
	set_flag(do_cpp);
	clear_program_flags();
	run_steps(source, s_steps);
	reset_flag(do_cpp);
}	/* compile_S */

/**** compile_s ****/
void compile_s(source) char *source;
{
	clear_program_flags();
	run_steps(source, s_steps);
}	/* compile_s */

/**** init_programs ****/
void init_programs()
{	char *flags= get_memory(128), *p= flags;
	conspt cp;
	char filename[MAXPATHLEN];

/* cpp */
	(void)add_to_list_tail(&program.cpp.permanent_flags, "-undef");
	if (driver.value == &lint_driver) {
		(void)add_to_list_tail(&program.cpp.permanent_flags, "-C");
		(void)add_to_list_tail(&program.cpp.permanent_flags, "-Dlint");};
/* m2cfe */
	if (debugger.touched)
		(void)add_to_list_tail(&program.m2cfe.permanent_flags, "-g");
	if (profile.touched)
		(void)add_to_list_tail(&program.m2cfe.permanent_flags, "-p");
	if (is_on(trace))
		(void)add_to_list_tail(&program.m2cfe.permanent_flags, "-v");
	(void)strcpy(filename, "-M");
	for (cp= module_list; cp != NULL; cp= cp->next) {
		(void)strcat(filename, cp->value+2);
		(void)strcat(filename, " ");};
	if (is_off(no_default_module_list)) {
		(void)strcat(filename, ". /usr/lib/modula2");
		if (profile.touched)
			(void)strcat(filename, "_p");}
	else
		if (module_list)
			filename[strlen(filename)-1]= 0;
	if (filename[2] != 0) { char *q;
		q= get_memory(strlen(filename)+1);
		(void)strcpy(q, filename);
		(void)add_to_list_tail(&program.m2cfe.permanent_flags, q);};
/* ccom */
	if (profile.touched)
		(void)add_to_list_tail(&program.ccom.permanent_flags, "-XP");
	if (debugger.value == &adb)
		(void)add_to_list_tail(&program.ccom.permanent_flags, "-XG");
	if (debugger.value == &dbx)
		(void)add_to_list_tail(&program.ccom.permanent_flags, "-Xg");
	if (is_on(long_offset))
		(void)add_to_list_tail(&program.ccom.permanent_flags, "-XJ");
	(void)add_to_list_tail(&program.ccom.permanent_flags, float_mode.value->name);
	(void)add_to_list_tail(&program.ccom.permanent_flags, target.value->name);
/* pc0 */
	if (is_on(long_offset))
		(void)add_to_list_tail(&program.pc0.permanent_flags, "-J");
	if (is_on(warning))
		(void)add_to_list_tail(&program.pc0.permanent_flags, "-w");
	if (is_on(checkC))
		(void)add_to_list_tail(&program.pc0.permanent_flags, "-C");
	if (is_on(checkH))
		(void)add_to_list_tail(&program.pc0.permanent_flags, "-H");
	if (is_on(checkV))
		(void)add_to_list_tail(&program.pc0.permanent_flags, "-V");
	if (profile.touched)
		(void)add_to_list_tail(&program.pc0.permanent_flags, "-p");
	if (debugger.value)
		(void)add_to_list_tail(&program.pc0.permanent_flags, "-g");
/* f1 */
	if (is_on(checkC) || is_on(checkH) || is_on(checkV))
		(void)add_to_list_tail(&program.f1.permanent_flags, "-V");
	(void)add_to_list_tail(&program.f1.permanent_flags, float_mode.value->name);
	(void)add_to_list_tail(&program.f1.permanent_flags, target.value->name);
/* mf1 */
	(void)add_to_list_tail(&program.mf1.permanent_flags, float_mode.value->name);
	(void)add_to_list_tail(&program.mf1.permanent_flags, target.value->name);
/* f77pass1 */
	if (is_on(statement_count))
		(void)add_to_list_tail(&program.f77pass1.permanent_flags, "-a");
	if (is_on(optimize) && (driver.value == &f77_driver))
		(void)add_to_list_tail(&program.f77pass1.permanent_flags, "-O");
	if (is_on(onetrip))
		(void)add_to_list_tail(&program.f77pass1.permanent_flags, "-1");
	if (profile.touched)
		(void)add_to_list_tail(&program.f77pass1.permanent_flags, "-p");
	if (float_mode.value == &fsky)
		(void)add_to_list_tail(&program.f77pass1.permanent_flags, "-F");
	if (debugger.value == &dbx)
		(void)add_to_list_tail(&program.f77pass1.permanent_flags, "-g");
	if ((is_off(optimize) || (driver.value != &f77_driver)) && is_off(statement_count)) { char *p;
		p= get_memory(100);
		(void)sprintf(p, "-P %s %s", target.value->name, float_mode.value->name);
		(void)add_to_list_tail(&program.f77pass1.permanent_flags, p);};
/* iropt */
	*p++= '-';
	if (float_mode.value == &ffpa) *p++= 'f';
	if (float_mode.value == &fsky) *p++= 'F';
	if ((float_mode.value == &f68881) && is_off(fstore)) *p++= 'm';
	if (target.value == &mc68020_arch) *p++= 'c';
	*p= 0;
	(void)add_to_list_tail(&program.iropt.permanent_flags, flags);
/* cg */
	(void)add_to_list_tail(&program.cg.permanent_flags, float_mode.value->name);
	(void)add_to_list_tail(&program.cg.permanent_flags, target.value->name);
/* c2 */
	switch (target.value->value) {
		case Z_MC68010:
			(void)add_to_list_tail(&program.c2.permanent_flags, "-10");
			break;
		case Z_MC68020:
			(void)add_to_list_tail(&program.c2.permanent_flags, "-20");
			break;};
	if ((float_mode.value == &ffpa) && (is_on(optimize)))
		(void)add_to_list_tail(&program.c2.permanent_flags, "-dscheduling");
/* as */
	(void)add_to_list_tail(&program.as.permanent_flags, target.value->name);
	if (is_on(as_R))
		(void)add_to_list_tail(&program.as.permanent_flags, "-R");
/* pc3 */
	if (is_on(warning))
		(void)add_to_list_tail(&program.pc3.permanent_flags, "-w");
	if (driver.value == &pc_driver)
		scan_path("/usr/lib/pcexterns.o", filename);
	(void)add_to_list_tail(&program.pc3.permanent_flags, strcpy(get_memory(strlen(filename)+1), filename));
/* ld */
	if (driver.value != &m2c_driver) {
		(void)add_to_list_tail(&program.ld.permanent_flags, "-e");
		(void)add_to_list_tail(&program.ld.permanent_flags, "start");
		if (driver.value == &f77_driver) {
			(void)add_to_list_tail(&program.ld.permanent_flags, "-u");
			(void)add_to_list_tail(&program.ld.permanent_flags, "_MAIN_");};};
	(void)add_to_list_tail(&program.ld.permanent_flags, "-X");
/* m2l */
	for (cp= module_list; cp != NULL; cp= cp->next)
		(void)add_to_list_tail(&program.m2l.permanent_flags, cp->value);
	if (is_off(no_default_module_list)) {
		(void)add_to_list_tail(&program.m2l.permanent_flags, "-M.");
		if (profile.touched)
			(void)add_to_list_tail(&program.m2l.permanent_flags, "-M/usr/lib/modula2_p");
		else
			(void)add_to_list_tail(&program.m2l.permanent_flags, "-M/usr/lib/modula2");};
	if (is_on(verbose))
		(void)add_to_list_tail(&program.m2l.permanent_flags, "-v");
	if (is_on(trace))
		(void)add_to_list_tail(&program.m2l.permanent_flags, "-trace");
	for (cp= program.ld.permanent_flags; cp != NULL; cp= cp->next)
		(void)add_to_list_tail(&program.m2l.permanent_flags, cp->value);
}	/* init_programs */

/**** setup_cpp ****/
char *setup_cpp()
{
	(void)add_to_list_tail(&program.cpp.flags, "-Dunix");
	(void)add_to_list_tail(&program.cpp.flags, "-Dsun");
	switch (target.value->value) {
		case Z_MC68010:
			(void)add_to_list_tail(&program.cpp.flags, "-Dmc68000");
			(void)add_to_list_tail(&program.cpp.flags, "-Dmc68010");
			break;
		case Z_MC68020:
			(void)add_to_list_tail(&program.cpp.flags, "-Dmc68000");
			(void)add_to_list_tail(&program.cpp.flags, "-Dmc68020");
			break;};
	if (is_on(do_dependency))
		(void)add_to_list_tail(&program.cpp.flags, "-M");
#ifdef S5EMUL
	(void)add_to_list_tail(&program.cpp.infile, "-I/usr/5include");
#endif S5EMUL
	return(NULL);
}	/* setup_cpp */

/**** setup_cpp_for_cc ****/
char *setup_cpp_for_cc(original_source, source) char *original_source, *source;
{
#ifdef lint
	source= original_source;
#endif lint
	(void)setup_cpp();
	if (produce.value == &preprocessed2)
		(void)add_to_list_tail(&program.cpp.flags, "-P");
	if (produce.value == &preprocessed) {
		if (outfile == NULL) {
			(void)add_to_list_tail(&program.cpp.infile, source);
			return("");};};
	return(NULL);
}	/* setup_cpp_for_cc */

/**** setup_bb_count ****/
char *setup_bb_count(original_source, source) char *original_source, *source;
{
#ifdef lint
	(void)printf(source);
#endif lint
	(void)add_to_list_tail(&program.bb_count.flags, original_source);
	return(NULL);
}	/* setup_bb_count */

/**** setup_cat_for_f77 ****/
char *setup_cat_for_f77(source, file) char *source, *file;
{
	(void)add_to_list_tail(&program.cat.infile, iropt_files[0]);
	(void)add_to_list_tail(&program.cat.infile, file);
	(void)add_to_list_tail(&program.cat.infile, iropt_files[2]);
	return(program.cat.outfile= outfile_name(source, &suffix.s, (programpt)NULL));
}	/* setup_cat_for_f77 */

/**** setup_cat_for_lint ****/
char *setup_cat_for_lint()
{	conspt p;

	for (p= infile_ln; p != NULL; p= p->next)
		(void)add_to_list_tail(&program.cat.infile, p->value);
	return(program.cat.outfile= temp_file_name(&program.cat, &suffix.ln, 0));
}	/* setup_cat_for_lint */
	
/**** setup_m2cfe_for_mod ****/
char *setup_m2cfe_for_mod(source) char *source;
{	char *p, *q;

	p= get_memory(strlen(source)+1);
	if ((q= rindex(source, '/')) == NULL) q= source; else q++;
	(void)strcpy(p, q);
	if ((q= rindex(p, '.')) != NULL) *q= 0;
	(void)add_to_list_tail(&program.m2cfe.flags, "-mod");
	(void)add_to_list_tail(&program.m2cfe.flags, p);
	return(NULL);
}	/* setup_m2cfe_for_mod */

/**** setup_m2cfe_for_def ****/
char *setup_m2cfe_for_def(source) char *source;
{	char *p, *q;

	p= get_memory(strlen(source)+1);
	if ((q= rindex(source, '/')) == NULL) q= source;
	(void)strcpy(p, q);
	if ((q= rindex(p, '.')) != NULL) *q= 0;
	(void)add_to_list_tail(&program.m2cfe.flags, "-def");
	(void)add_to_list_tail(&program.m2cfe.flags, p);
	return(NULL);
}	/* setup_m2cfe_for_def */

/**** setup_f77pass1 ****/
char *setup_f77pass1(source, file) char *source, *file;
{
#ifdef lint
	(void)printf(source);
#endif lint
	if (requested_suffix == &suffix.s) set_flag(do_cat);
	(void)add_to_list_tail(&program.f77pass1.infile, file);
	iropt_files[0]= temp_file_name(&program.f77pass1, &suffix.s, 's');
	iropt_files[1]= temp_file_name(&program.f77pass1, &suffix.s, 'i');
	iropt_files[2]= temp_file_name(&program.f77pass1, &suffix.s, 'd');
	(void)add_to_list_tail(&program.f77pass1.infile, iropt_files[0]);
	(void)add_to_list_tail(&program.f77pass1.infile, iropt_files[1]);
	(void)add_to_list_tail(&program.f77pass1.infile, iropt_files[2]);
	if (is_on(statement_count)) {	char *p, *q;
		p= get_memory(MAXPATHLEN);
		(void)strcpy(p, "-A");
		(void)strcat(p, pwd);
		(void)strcat(p, "/");
		(void)strcat(p, source);
		if ((q= rindex(p, '.')) == NULL)
			(void)strcat(p, ".d");
		else {
			*(q+1)= 'd'; *(q+2)= 0;};
		tcov_file= p;
		(void)add_to_list_tail(&program.f77pass1.infile, tcov_file+2);};
	return(iropt_files[1]);
}	/* setup_f77pass1 */

/**** setup_cg_for_tcov ****/
char *setup_cg_for_tcov()
{
	if (is_on(statement_count))
		(void)add_to_list_tail(&program.cg.infile, tcov_file);
	return(NULL);
}	/* setup_cg_for_tcov */

/**** setup_inline_for_pc ****/
char *setup_inline_for_pc()
{	char filename[MAXPATHLEN];

	scan_path("/usr/lib/pc2.il", filename);
	(void)add_to_list_tail(&program.inline.flags, "-i");
	(void)add_to_list_tail(&program.inline.flags, strcpy(get_memory(strlen(filename)+1), filename));
	return(NULL);
}	/* setup_inline_for_pc */

/**** setup_as_for_f77 ****/
char *setup_as_for_f77(source, file) char *source, *file;
{
	(void)add_to_list_tail(&program.as.infile, iropt_files[0]);
	(void)add_to_list_tail(&program.as.infile, file);
	(void)add_to_list_tail(&program.as.infile, iropt_files[2]);
	program.as.outfile= outfile_name(source, &suffix.o, (programpt)NULL);
	if ((source_infile_count == 1) && (produce.value == &executable))
		(void)add_to_list_tail(&files_to_unlink, program.as.outfile);
	return(program.as.outfile);
}	/* setup_as_for_f77 */

/**** setup_pc3****/
char *setup_pc3()
{	conspt p;

	for (p= infile_o; p != NULL; p= p->next)
		(void)add_to_list_tail(&program.pc3.infile, p->value);
}	/* setup_pc3 */

/**** setup_ld ****/
char *setup_ld()
{	conspt p;
	char filename[MAXPATHLEN];

	program.ld.outfile= (outfile == NULL) ? "a.out" : outfile;
	for (p= infile; p != NULL; p= p->next)
		if (!strcmp(program.ld.outfile, p->value))
			fatal1("Outfile %s would overwrite infile\n", p->value);
	scan_path(profile.value->extra,  profile_name);
	(void)add_to_list_tail(&program.ld.infile, profile_name);
	if (float_mode.value->extra) {
		scan_path(float_mode.value->extra,  float_mode_name);
		(void)add_to_list_tail(&program.ld.infile, float_mode_name);};
	if (is_on(statement_count) && (driver.value == &cc_driver)) {
		scan_path("/usr/lib/bb_link.o", filename);
		(void)add_to_list_tail(&program.ld.infile, strcpy(get_memory(strlen(filename)+1), filename));};
	for (p= infile_o; p != NULL; p= p->next)
		(void)add_to_list_tail(&program.ld.infile, p->value);
	if (debugger.touched)
		(void)add_to_list_tail(&program.ld.infile, "-lg");
	if (driver.value == &f77_driver) {
		(void)add_to_list_tail(&program.ld.infile, profile.touched ? "-lF77_p":"-lF77");
		(void)add_to_list_tail(&program.ld.infile, profile.touched ? "-lI77_p":"-lI77");
		(void)add_to_list_tail(&program.ld.infile, profile.touched ? "-lU77_p":"-lU77");};
	if (driver.value == &pc_driver)
		(void)add_to_list_tail(&program.ld.infile, profile.touched ? "-lpc_p":"-lpc");
	if ((driver.value == &pc_driver) || (driver.value == &f77_driver))
		(void)add_to_list_tail(&program.ld.infile, profile.touched ? "-lm_p":"-lm");
	(void)add_to_list_tail(&program.ld.infile, profile.touched ? "-lc_p":"-lc");
#ifdef S5EMUL
	(void)add_to_list_tail(&program.ld.infile, "-L/usr/5lib");
#endif S5EMUL
	return("");
}	/* setup_ld */

/**** setup_m2l****/
char *setup_m2l()
{	conspt p;

	program.m2l.outfile= (outfile == NULL) ? "a.out" : outfile;
	for (p= infile; p != NULL; p= p->next)
		if (!strcmp(program.m2l.outfile, p->value))
			fatal1("Outfile %s would overwrite infile\n", p->value);
	scan_path(profile.value->extra,  profile_name);
	(void)add_to_list_tail(&program.m2l.infile, profile_name);
	if (float_mode.value->extra) {
		scan_path(float_mode.value->extra,  float_mode_name);
		(void)add_to_list_tail(&program.m2l.infile, float_mode_name);};
	for (p= infile_o; p != NULL; p= p->next)
		(void)add_to_list_tail(&program.m2l.infile, p->value);
	if (debugger.touched)
		(void)add_to_list_tail(&program.m2l.infile, "-lg");
	(void)add_to_list_tail(&program.m2l.infile, profile.touched ? "-lc_p":"-lc");
	return("");
}	/* setup_m2l */

/**** collect_ln ****/
void collect_ln(file, suffix) char *file; suffixpt suffix;
{
	(void)add_to_list_tail3(&infile_ln, file, suffix);
}	/* collect_ln */

/**** collect_o ****/
void collect_o(file, suffix) char *file; suffixpt suffix;
{
	if (is_off(doing_mod_file))
		(void)add_to_list_tail3(&infile_o, file, suffix);
}	/* collect_o */

/**** do_infiles ****/
static void do_infiles()
{	conspt cp;
	suffixpt save_suffix;
	super_intt save_produce;

	for (cp= infile; cp != NULL; cp= cp->next) {
		if ((source_infile_count > 1) && ((cp->suffix->in_drivers&SOURCE_SUFFIX) == SOURCE_SUFFIX))
			(void)printf("%s:\n", cp->value);
		status= 0;
		reset_flag(doing_mod_file);
		save_suffix= requested_suffix; save_produce= produce;
		if (cp->suffix == &suffix.def) {
			requested_suffix= &suffix.sym; produce.value= NULL;};
		if ((cp->suffix == &suffix.mod) && (driver.value == &m2c_driver))
			set_flag(doing_mod_file);
		if (cp->suffix->compile != NULL)
			(*cp->suffix->compile)(cp->value);
		else {
			if (cp->suffix->collect != NULL)
				(*cp->suffix->collect)(cp->value, cp->suffix);};
		exit_status|= status;
		requested_suffix= save_suffix; produce= save_produce;};
}	/* do_infiles */

/**** set_requested_suffix ****/
static void set_requested_suffix(suf) suffixpt suf;
{
	if (requested_suffix == NULL)
		switch (produce.value->value) {
			case Y_I: requested_suffix= suf; break;
			case Y_EXE: if (driver.value != &lint_driver) {
				requested_suffix= &suffix.o; break;};
			/* Fall into */
			case Y_L: requested_suffix= &suffix.ln; break;
			case Y_O: requested_suffix= &suffix.o; break;
			case Y_S: requested_suffix= &suffix.s; break;};
}	/* set_requested_suffix */

/**** cc_doit ****/
void cc_doit()
{
	set_requested_suffix(&suffix.i);
	do_infiles();
	if (!exit_status && (produce.value == &executable) && (infile_o != NULL)) {
		compile_stepsp= compile_steps;
		requested_suffix= &suffix.none;
		status= 0;
		if (source_infile_count > 1)
			(void)printf("Linking:\n");
		clear_program_flags();
		run_steps("", ld_steps);
		exit_status|= status;};
}	/* cc_doit */

/**** f77_doit ****/
void f77_doit()
{
	set_requested_suffix(&suffix.f);
	do_infiles();
	if (!exit_status && (produce.value == &executable) && (infile_o != NULL)) {
		compile_stepsp= compile_steps;
		requested_suffix= &suffix.none;
		status= 0;
		if (source_infile_count > 1)
			(void)printf("Linking:\n");
		clear_program_flags();
		run_steps("", ld_steps);
		exit_status|= status;};
}	/* f77_doit */

/**** lint_doit ****/
void lint_doit()
{
	if (build_lint_lib) {
		if (is_on(dryrun))
			build_lint_lib= 1;
		else
			build_lint_lib= open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (build_lint_lib == -1)
			fatal1("Cannot open %s for write\n", outfile);};
	if (requested_suffix == NULL)
		set_requested_suffix(&suffix.i);
	else
		if (requested_suffix == &suffix.ln)
			set_int(produce, &lint1_file);
	if (is_off(ignore_lc) && (produce.value != &lint1_file))
		collect_ln(lint_lib("-lc"), &suffix.ln);
	do_infiles();
	if (build_lint_lib)
		(void)close(build_lint_lib);
	else
		if ((produce.value != &lint1_file) && (infile_ln != NULL)) {
			requested_suffix= &suffix.none;
			compile_stepsp= compile_steps;
			status= 0;
			if (source_infile_count > 1)
				(void)printf("Lint pass2:\n");
			clear_program_flags();
			run_steps("", lint_steps);
			exit_status|= status;};
}	/* lint_doit */

/**** m2c_doit ****/
void m2c_doit()
{
	if (produce.value == &executable)
		produce.value= &object;
	if (is_on(do_m2l))
		produce.value= &executable;
	set_requested_suffix(&suffix.none);
	do_infiles();
	if (!exit_status && (produce.value == &executable)) {
		compile_stepsp= compile_steps;
		requested_suffix= &suffix.none;
		status= 0;
		if (source_infile_count > 1)
			(void)printf("Linking:\n");
		clear_program_flags();
		run_steps("", m2c_steps);
		exit_status|= status;};
}	/* m2c_doit */

/**** pc_doit ****/
void pc_doit()
{
	set_requested_suffix(&suffix.pi);
	do_infiles();
	if (!exit_status && (produce.value == &executable) && (infile_o != NULL)) {
		compile_stepsp= compile_steps;
		requested_suffix= &suffix.none;
		status= 0;
		if (source_infile_count > 1)
			(void)printf("Linking:\n");
		clear_program_flags();
		run_steps("", pc_steps);
		exit_status|= status;};
}	/* pc_doit */
